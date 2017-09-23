/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#ifndef _NGX_CORE_H_INCLUDED_
#define _NGX_CORE_H_INCLUDED_

#include <ngx_config.h>
#include <nginx.h>

struct /*ngx_event_s*/ngx_event_t;
struct /*ngx_pool_s*/ngx_pool_t;
struct ngx_cycle_t;
struct ngx_module_t;
struct ngx_conf_t;
struct ngx_chain_t;
struct ngx_log_t;
struct ngx_command_t;
struct ngx_file_t;
struct ngx_connection_t;
struct ngx_open_file_t;
//typedef struct ngx_module_s          ngx_module_t;
//typedef struct ngx_conf_s            ngx_conf_t;
//typedef struct ngx_cycle_s           ngx_cycle_t;
//typedef struct ngx_pool_s            ngx_pool_t;
//typedef struct ngx_chain_s           ngx_chain_t;
//typedef struct ngx_log_s             ngx_log_t;
//typedef struct ngx_open_file_s       ngx_open_file_t;
//typedef struct ngx_command_s         ngx_command_t;
//typedef struct ngx_file_s            ngx_file_t;
//typedef struct ngx_event_s           ngx_event_t;
typedef struct ngx_event_aio_s       ngx_event_aio_t;
//typedef struct ngx_connection_s      ngx_connection_t;
typedef struct ngx_thread_task_s     ngx_thread_task_t;
typedef struct ngx_ssl_s             ngx_ssl_t;
typedef struct ngx_ssl_connection_s  ngx_ssl_connection_t;

typedef void (*ngx_event_handler_pt)(ngx_event_t *ev);
typedef void (*ngx_connection_handler_pt)(ngx_connection_t *c);

#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6
#define  NGX_DELEGATED  -7 // @sobolev ќтвет F_HttpHandler означающий, что запрос был делегирован другому потоку и основной цикл Nginx делать с ним ничего пока не должен.

#define ngx_abs(value)       (((value) >= 0) ? (value) : -(value))
//
//#include <ngx_rbtree.h>
//
typedef ngx_uint_t ngx_rbtree_key_t;
typedef ngx_int_t ngx_rbtree_key_int_t;
//typedef struct ngx_rbtree_node_s ngx_rbtree_node_t;

struct /*ngx_rbtree_node_s*/ngx_rbtree_node_t {
	ngx_rbtree_key_t key;
	ngx_rbtree_node_t   * left;
	ngx_rbtree_node_t   * right;
	ngx_rbtree_node_t   * parent;
	u_char color;
	u_char data;
};

typedef struct ngx_rbtree_s ngx_rbtree_t;
typedef void (*ngx_rbtree_insert_pt)(ngx_rbtree_node_t * root, ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel);

struct ngx_rbtree_s {
	ngx_rbtree_node_t   * root;
	ngx_rbtree_node_t   * sentinel;
	ngx_rbtree_insert_pt insert;
};

#define ngx_rbtree_init(tree, s, i)					      \
	ngx_rbtree_sentinel_init(s);						  \
	(tree)->root = s;							  \
	(tree)->sentinel = s;							  \
	(tree)->insert = i

void ngx_rbtree_insert(ngx_rbtree_t * tree, ngx_rbtree_node_t * node);
void ngx_rbtree_delete(ngx_rbtree_t * tree, ngx_rbtree_node_t * node);
void ngx_rbtree_insert_value(ngx_rbtree_node_t * root, ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel);
void ngx_rbtree_insert_timer_value(ngx_rbtree_node_t * root, ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel);
const ngx_rbtree_node_t * ngx_rbtree_next(const ngx_rbtree_t * tree, const ngx_rbtree_node_t * node);

#define ngx_rbt_red(node)               ((node)->color = 1)
#define ngx_rbt_black(node)             ((node)->color = 0)
#define ngx_rbt_is_red(node)            ((node)->color)
#define ngx_rbt_is_black(node)          (!ngx_rbt_is_red(node))
#define ngx_rbt_copy_color(n1, n2)      (n1->color = n2->color)

// a sentinel must be black 
#define ngx_rbtree_sentinel_init(node)  ngx_rbt_black(node)

static ngx_inline ngx_rbtree_node_t * ngx_rbtree_min(ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel)
{
	while(node->left != sentinel) {
		node = node->left;
	}
	return node;
}
//
//#include <ngx_string.h>
//
struct ngx_str_t {
	size_t len;
	u_char * data;
};

struct ngx_keyval_t {
	ngx_str_t key;
	ngx_str_t value;
};

struct ngx_variable_value_t {
	unsigned len : 28;
	unsigned valid : 1;
	unsigned no_cacheable : 1;
	unsigned not_found : 1;
	unsigned escape : 1;
	u_char * data;
};

#define ngx_string(str)     { sizeof(str) - 1, (u_char*)str }
#define ngx_null_string     { 0, NULL }
#define ngx_str_set(str, text) (str)->len = sizeof(text) - 1; (str)->data = (u_char*)text
#define ngx_str_null(str)   (str)->len = 0; (str)->data = NULL
#define ngx_tolower(c)      (u_char)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define ngx_toupper(c)      (u_char)((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

void FASTCALL ngx_strlow(u_char * dst, const u_char * src, size_t n);

#define ngx_strncmp(s1, s2, n)  strncmp((const char*)s1, (const char*)s2, n)

/* msvc and icc7 compile strcmp() to inline loop */
#define ngx_strcmp(s1, s2)  strcmp((const char*)s1, (const char*)s2)
#define ngx_strstr(s1, s2)  strstr((const char*)s1, (const char*)s2)
#define ngx_strlen(s)       strlen((const char*)s)
#define ngx_strchr(s1, c)   strchr((const char*)s1, (int)c)

static ngx_inline u_char * ngx_strlchr(u_char * p, u_char * last, u_char c)
{
	while(p < last) {
		if(*p == c) {
			return p;
		}
		p++;
	}
	return NULL;
}
/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
//#define ngx_memzero_Removed(buf, n)       (void)memset(buf, 0, n)
//#define ngx_memset_Removed(buf, c, n)     (void)memset(buf, c, n)

#if (NGX_MEMCPY_LIMIT)
	void * ngx_memcpy_Removed(void * dst, const void * src, size_t n);
	#define ngx_cpymem(dst, src, n)   (((u_char*)memcpy(dst, src, n)) + (n))
#else
// 
// gcc3, msvc, and icc7 compile memcpy() to the inline "rep movs".
// gcc3 compiles memcpy(d, s, 4) to the inline "mov"es.
// icc8 compile memcpy(d, s, 4) to the inline "mov"es or XMM moves.
// 
#define ngx_memcpy_Removed(dst, src, n)   (void)memcpy(dst, src, n)
#define ngx_cpymem(dst, src, n)   (((u_char*)memcpy(dst, src, n)) + (n))
#endif

#if ( __INTEL_COMPILER >= 800 )
	/*
	 * the simple inline cycle copies the variable length strings up to 16
	 * bytes faster than icc8 autodetecting _intel_fast_memcpy()
	 */
	static ngx_inline u_char * ngx_copy(u_char * dst, u_char * src, size_t len)
	{
		if(len < 17) {
			while(len) {
				*dst++ = *src++;
				len--;
			}
			return dst;
		}
		else {
			return ngx_cpymem(dst, src, len);
		}
	}
#else
	#define ngx_copy                  ngx_cpymem
#endif

#define ngx_memmove_Removed(dst, src, n)   (void)memmove(dst, src, n)
#define ngx_movemem(dst, src, n)   (((u_char*)memmove(dst, src, n)) + (n))
// msvc and icc7 compile memcmp() to the inline loop 
#define ngx_memcmp_Removed(s1, s2, n)  memcmp((const char*)s1, (const char*)s2, n)

int SStrDupToNgxStr(ngx_pool_t * pPool, const SString * pSrc, ngx_str_t * pDest);

u_char * FASTCALL ngx_cpystrn(u_char * dst, const u_char * src, size_t n);
u_char * FASTCALL ngx_pstrdup(ngx_pool_t * pool, const ngx_str_t * src);
u_char * ngx_cdecl ngx_sprintf(u_char * buf, const char * fmt, ...);
u_char * ngx_cdecl ngx_snprintf(u_char * buf, size_t max, const char * fmt, ...);
u_char * ngx_cdecl ngx_slprintf(u_char * buf, u_char * last, const char * fmt, ...);
u_char * ngx_vslprintf(u_char * buf, u_char * last, const char * fmt, va_list args);
#define ngx_vsnprintf(buf, max, fmt, args) ngx_vslprintf(buf, buf + (max), fmt, args)

// @sobolev ngx_int_t FASTCALL ngx_strcasecmp(const u_char * s1, const u_char * s2);
ngx_int_t FASTCALL ngx_strncasecmp(const u_char * s1, const u_char * s2, size_t n);
const u_char * FASTCALL ngx_strnstr(const u_char * s1, const char * s2, size_t n);
u_char * ngx_strstrn(u_char * s1, char * s2, size_t n);
const u_char * ngx_strcasestrn(const u_char * s1, const char * s2, size_t n);
u_char * ngx_strlcasestrn(u_char * s1, u_char * last, const u_char * s2, size_t n);
ngx_int_t ngx_rstrncmp(u_char * s1, u_char * s2, size_t n);
ngx_int_t ngx_rstrncasecmp(u_char * s1, u_char * s2, size_t n);
ngx_int_t ngx_memn2cmp(u_char * s1, u_char * s2, size_t n1, size_t n2);
ngx_int_t FASTCALL ngx_dns_strcmp(const u_char * s1, const u_char * s2);
ngx_int_t ngx_filename_cmp(const u_char * s1, const u_char * s2, size_t n);
ngx_int_t FASTCALL ngx_atoi(const u_char * line, size_t n);
ngx_int_t ngx_atofp(const u_char * line, size_t n, size_t point);
ssize_t FASTCALL ngx_atosz(const u_char * line, size_t n);
nginx_off_t FASTCALL ngx_atoof(const u_char * line, size_t n);
time_t FASTCALL ngx_atotm(const u_char * line, size_t n);
ngx_int_t FASTCALL ngx_hextoi(const u_char * line, size_t n);
u_char * ngx_hex_dump(u_char * dst, const u_char * src, size_t len);

#define ngx_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define ngx_base64_decoded_length(len)  (((len + 3) / 4) * 3)

void FASTCALL ngx_encode_base64(ngx_str_t * dst, const ngx_str_t * src);
void FASTCALL ngx_encode_base64url(ngx_str_t * dst, const ngx_str_t * src);
ngx_int_t FASTCALL ngx_decode_base64(ngx_str_t * dst, const ngx_str_t * src);
ngx_int_t FASTCALL ngx_decode_base64url(ngx_str_t * dst, const ngx_str_t * src);

uint32_t FASTCALL ngx_utf8_decode(const u_char ** p, size_t n);
size_t FASTCALL ngx_utf8_length(const u_char * p, size_t n);
u_char * ngx_utf8_cpystrn(u_char * dst, const u_char * src, size_t n, size_t len);

#define NGX_ESCAPE_URI            0
#define NGX_ESCAPE_ARGS           1
#define NGX_ESCAPE_URI_COMPONENT  2
#define NGX_ESCAPE_HTML           3
#define NGX_ESCAPE_REFRESH        4
#define NGX_ESCAPE_MEMCACHED      5
#define NGX_ESCAPE_MAIL_AUTH      6

#define NGX_UNESCAPE_URI       1
#define NGX_UNESCAPE_REDIRECT  2

uintptr_t ngx_escape_uri(u_char * dst, const u_char * src, size_t size, ngx_uint_t type);
void ngx_unescape_uri(u_char ** dst, u_char ** src, size_t size, ngx_uint_t type);
uintptr_t ngx_escape_html(u_char * dst, u_char * src, size_t size);
uintptr_t ngx_escape_json(u_char * dst, u_char * src, size_t size);

struct ngx_str_node_t {
	ngx_rbtree_node_t node;
	ngx_str_t str;
};

void ngx_str_rbtree_insert_value(ngx_rbtree_node_t * temp, ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel);
ngx_str_node_t * ngx_str_rbtree_lookup(ngx_rbtree_t * rbtree, ngx_str_t * name, uint32_t hash);
void ngx_sort(void * base, size_t n, size_t size, ngx_int_t (* cmp)(const void *, const void *));

#define ngx_qsort             qsort
#define ngx_value_helper(n)   # n
#define ngx_value(n)          ngx_value_helper(n)
//
#include <ngx_errno.h>
#include <ngx_atomic.h>
#include <ngx_thread.h>

#include <ngx_time.h>
#include <ngx_socket.h>
#include <ngx_files.h>
#include <ngx_shmem.h>
#include <ngx_process.h>
#include <ngx_dlopen.h>
#include <ngx_user.h>
#include <ngx_alloc.h> // os

#include <ngx_times.h>
//
//#include <ngx_log.h>
//
#define NGX_LOG_STDERR            0
#define NGX_LOG_EMERG             1
#define NGX_LOG_ALERT             2
#define NGX_LOG_CRIT              3
#define NGX_LOG_ERR               4
#define NGX_LOG_WARN              5
#define NGX_LOG_NOTICE            6
#define NGX_LOG_INFO              7
#define NGX_LOG_DEBUG             8

#define NGX_LOG_DEBUG_CORE        0x010
#define NGX_LOG_DEBUG_ALLOC       0x020
#define NGX_LOG_DEBUG_MUTEX       0x040
#define NGX_LOG_DEBUG_EVENT       0x080
#define NGX_LOG_DEBUG_HTTP        0x100
#define NGX_LOG_DEBUG_MAIL        0x200
#define NGX_LOG_DEBUG_STREAM      0x400
/*
 * do not forget to update debug_levels[] in src/core/ngx_log.c
 * after the adding a new debug level
 */
#define NGX_LOG_DEBUG_FIRST       NGX_LOG_DEBUG_CORE
#define NGX_LOG_DEBUG_LAST        NGX_LOG_DEBUG_STREAM
#define NGX_LOG_DEBUG_CONNECTION  0x80000000
#define NGX_LOG_DEBUG_ALL         0x7ffffff0

typedef u_char *(*ngx_log_handler_pt)(ngx_log_t * log, u_char * buf, size_t len);
typedef void (*ngx_log_writer_pt)(ngx_log_t * log, ngx_uint_t level, u_char * buf, size_t len);

struct ngx_log_t {
	ngx_uint_t Level/*log_level*/;
	ngx_open_file_t * file;
	ngx_atomic_uint_t connection;
	time_t disk_full_time;
	ngx_log_handler_pt handler;
	void * data;
	ngx_log_writer_pt writer;
	void * wdata;
	// 
	// we declare "action" as "char *" because the actions are usually
	// the static strings and in the "u_char *" case we have to override their types all the time
	// 
	char * action;
	ngx_log_t * next;
};

#define NGX_MAX_ERROR_STR   2048

/*********************************/

#if (NGX_HAVE_C99_VARIADIC_MACROS)
	#define NGX_HAVE_VARIADIC_MACROS  1
	#define ngx_log_error(level, log, ...) if((log)->Level >= level) ngx_log_error_core(level, log, __VA_ARGS__)
	void ngx_log_error_core(ngx_uint_t level, ngx_log_t * log, ngx_err_t err, const char * fmt, ...);
	#define ngx_log_debug(level, log, ...) if((log)->Level & level) ngx_log_error_core(NGX_LOG_DEBUG, log, __VA_ARGS__)
#elif (NGX_HAVE_GCC_VARIADIC_MACROS)
	#define NGX_HAVE_VARIADIC_MACROS  1
	#define ngx_log_error(level, log, args ...) if((log)->Level >= level) ngx_log_error_core(level, log, args)
	void ngx_log_error_core(ngx_uint_t level, ngx_log_t * log, ngx_err_t err, const char * fmt, ...);
	#define ngx_log_debug(level, log, args ...) if((log)->Level & level) ngx_log_error_core(NGX_LOG_DEBUG, log, args)
#else /* no variadic macros */
	#define NGX_HAVE_VARIADIC_MACROS  0
	void ngx_cdecl ngx_log_error(ngx_uint_t level, ngx_log_t * log, ngx_err_t err, const char * fmt, ...);
	void ngx_log_error_core(ngx_uint_t level, ngx_log_t * log, ngx_err_t err, const char * fmt, va_list args);
	void ngx_cdecl ngx_log_debug_core(ngx_log_t * log, ngx_err_t err, const char * fmt, ...);
#endif /* variadic macros */

/*********************************/

#if (NGX_DEBUG)
	#if (NGX_HAVE_VARIADIC_MACROS)
		#define ngx_log_debug0(level, log, err, fmt) ngx_log_debug(level, log, err, fmt)
		#define ngx_log_debug1(level, log, err, fmt, arg1) ngx_log_debug(level, log, err, fmt, arg1)
		#define ngx_log_debug2(level, log, err, fmt, arg1, arg2) ngx_log_debug(level, log, err, fmt, arg1, arg2)
		#define ngx_log_debug3(level, log, err, fmt, arg1, arg2, arg3) ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3)
		#define ngx_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4) ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4)
		#define ngx_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5) ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)
		#define ngx_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6) ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
		#define ngx_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7) ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
		#define ngx_log_debug8(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	#else /* no variadic macros */
		#define ngx_log_debug0(level, log, err, fmt) if((log)->Level & level) ngx_log_debug_core(log, err, fmt)
		#define ngx_log_debug1(level, log, err, fmt, arg1) if((log)->Level & level) ngx_log_debug_core(log, err, fmt, arg1)
		#define ngx_log_debug2(level, log, err, fmt, arg1, arg2) if((log)->Level & level) ngx_log_debug_core(log, err, fmt, arg1, arg2)
		#define ngx_log_debug3(level, log, err, fmt, arg1, arg2, arg3) if((log)->Level & level) ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3)
		#define ngx_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4) if((log)->Level & level) ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4)
		#define ngx_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5) if((log)->Level & level) ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5)
		#define ngx_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)			   \
			if((log)->Level & level)						 \
				ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
		#define ngx_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)		   \
			if((log)->Level & level)						 \
				ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
		#define ngx_log_debug8(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)	   \
			if((log)->Level & level)						 \
				ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
	#endif
#else /* !NGX_DEBUG */
	#define ngx_log_debug0(level, log, err, fmt)
	#define ngx_log_debug1(level, log, err, fmt, arg1)
	#define ngx_log_debug2(level, log, err, fmt, arg1, arg2)
	#define ngx_log_debug3(level, log, err, fmt, arg1, arg2, arg3)
	#define ngx_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)
	#define ngx_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)
	#define ngx_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
	#define ngx_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
	#define ngx_log_debug8(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#endif

/*********************************/

ngx_log_t * ngx_log_init(const u_char * prefix);
void ngx_cdecl ngx_log_abort(ngx_err_t err, const char * fmt, ...);
void ngx_cdecl ngx_log_stderr(ngx_err_t err, const char * fmt, ...);
u_char * ngx_log_errno(u_char * buf, u_char * last, ngx_err_t err);
ngx_int_t ngx_log_open_default(ngx_cycle_t * cycle);
ngx_int_t ngx_log_redirect_stderr(ngx_cycle_t * cycle);
ngx_log_t * ngx_log_get_file_log(ngx_log_t * head);
char * ngx_log_set_log(ngx_conf_t * cf, ngx_log_t ** head);
/*
 * ngx_write_stderr() cannot be implemented as macro, since
 * MSVC does not allow to use #ifdef inside macro parameters.
 *
 * ngx_write_fd() is used instead of ngx_write_console(), since
 * CharToOemBuff() inside ngx_write_console() cannot be used with
 * read only buffer as destination and CharToOemBuff() is not needed
 * for ngx_write_stderr() anyway.
 */
static ngx_inline void ngx_write_stderr(const char * text)
{
	(void)ngx_write_fd(ngx_stderr, text, ngx_strlen(text));
}

static ngx_inline void ngx_write_stdout(const char * text)
{
	(void)ngx_write_fd(ngx_stdout, text, ngx_strlen(text));
}

extern ngx_module_t ngx_errlog_module;
extern ngx_uint_t ngx_use_stderr;
//
//#include <ngx_parse.h>
//
ssize_t FASTCALL ngx_parse_size(const ngx_str_t *line);
nginx_off_t FASTCALL ngx_parse_offset(const ngx_str_t *line);
ngx_int_t FASTCALL ngx_parse_time(const ngx_str_t * line, ngx_uint_t is_sec);
//
//#include <ngx_parse_time.h>
//
time_t FASTCALL ngx_parse_http_time(const u_char * value, size_t len);
#define ngx_http_parse_time(value, len)  ngx_parse_http_time(value, len) // compatibility 
//
//#include <ngx_palloc.h>
//
// 
// NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
// On Windows NT it decreases a number of locked pages in a kernel.
// 
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)
#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)
#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE        ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)), NGX_POOL_ALIGNMENT)

typedef void (*ngx_pool_cleanup_pt)(void * data);

//typedef struct ngx_pool_cleanup_s ngx_pool_cleanup_t;
//typedef struct ngx_pool_large_s ngx_pool_large_t;

struct /*ngx_pool_cleanup_s*/ngx_pool_cleanup_t {
	ngx_pool_cleanup_pt handler;
	void * data;
	ngx_pool_cleanup_t * next;
};

struct /*ngx_pool_large_s*/ngx_pool_large_t {
	ngx_pool_large_t * next;
	void * alloc;
};

struct ngx_pool_data_t {
	u_char * last;
	u_char * end;
	ngx_pool_t * next;
	ngx_uint_t failed;
};

struct /*ngx_pool_s*/ngx_pool_t {
	ngx_pool_data_t d;
	size_t max;
	ngx_pool_t * current;
	ngx_chain_t * chain;
	ngx_pool_large_t * large;
	ngx_pool_cleanup_t * cleanup;
	ngx_log_t * log;
};

struct ngx_pool_cleanup_file_t {
	ngx_fd_t fd;
	u_char * name;
	ngx_log_t * log;
};

ngx_pool_t * FASTCALL ngx_create_pool(size_t size, ngx_log_t * log);
void   FASTCALL ngx_destroy_pool(ngx_pool_t * pool);
void   FASTCALL ngx_reset_pool(ngx_pool_t * pool);
void * FASTCALL ngx_palloc(ngx_pool_t * pool, size_t size);
void * FASTCALL ngx_pnalloc(ngx_pool_t * pool, size_t size);
void * FASTCALL ngx_pcalloc(ngx_pool_t * pool, size_t size);
void * ngx_pmemalign(ngx_pool_t * pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t * pool, void * p);
ngx_pool_cleanup_t * ngx_pool_cleanup_add(ngx_pool_t * p, size_t size);
void   ngx_pool_run_cleanup_file(ngx_pool_t * p, ngx_fd_t fd);
void   ngx_pool_cleanup_file(void * data);
void   ngx_pool_delete_file(void * data);
//
//#include <ngx_buf.h>
//
typedef void * ngx_buf_tag_t;
//typedef struct ngx_buf_s ngx_buf_t;

struct /*ngx_buf_s*/ngx_buf_t {
	u_char   * last;  // ”казатель на байт, следующий за последним заполненным данными байтом. last == pos означает, что данных нет.
		// @sobolev last перемещен на первое место ибо к нему больше всего обращений.
	u_char   * pos;   
	nginx_off_t file_pos;
	nginx_off_t file_last;
	u_char   * start;    // start of buffer 
	u_char   * end;      // end of buffer 
	ngx_buf_tag_t tag;
	ngx_file_t * file;
	ngx_buf_t  * shadow;
	unsigned temporary     : 1; // the buf's content could be changed 
	unsigned memory        : 1; // the buf's content is in a memory cache or in a read only memory and must not be changed
	unsigned mmap          : 1; // the buf's content is mmap()ed and must not be changed 
	unsigned recycled      : 1;
	unsigned in_file       : 1;
	unsigned flush         : 1;
	unsigned sync          : 1;
	unsigned last_buf      : 1;
	unsigned last_in_chain : 1;
	unsigned last_shadow   : 1;
	unsigned temp_file     : 1;
	/* STUB */ int num;
};

struct ngx_chain_t {
	ngx_chain_t()
	{
		buf = 0;
		next = 0;
	}
	ngx_chain_t(ngx_buf_t * pB, ngx_chain_t * pNext)
	{
		buf = pB;
		next = pNext;
	}
	ngx_buf_t * buf;
	ngx_chain_t * next;
};

struct ngx_bufs_t {
	ngx_int_t num;
	size_t size;
};

struct /*ngx_output_chain_ctx_s*/ngx_output_chain_ctx_t;

//typedef struct ngx_output_chain_ctx_s ngx_output_chain_ctx_t;
typedef ngx_int_t (*ngx_output_chain_filter_pt)(void * ctx, ngx_chain_t * in);
typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t * ctx, ngx_file_t * file);

struct /*ngx_output_chain_ctx_s*/ngx_output_chain_ctx_t {
	ngx_buf_t * buf;
	ngx_chain_t * in;
	ngx_chain_t * free;
	ngx_chain_t * busy;
	unsigned sendfile : 1;
	unsigned directio : 1;
	unsigned unaligned : 1;
	unsigned need_in_memory : 1;
	unsigned need_in_temp : 1;
	unsigned aio : 1;
#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
	ngx_output_chain_aio_pt aio_handler;
	#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
	ssize_t (* aio_preload)(ngx_buf_t * file);
	#endif
#endif
#if (NGX_THREADS || NGX_COMPAT)
	ngx_int_t (* thread_handler)(ngx_thread_task_t * task, ngx_file_t * file);
	ngx_thread_task_t * thread_task;
#endif
	nginx_off_t alignment;
	ngx_pool_t  * pool;
	ngx_int_t allocated;
	ngx_bufs_t bufs;
	ngx_buf_tag_t tag;
	ngx_output_chain_filter_pt output_filter;
	void * filter_ctx;
};

struct ngx_chain_writer_ctx_t {
	ngx_chain_t * out;
	ngx_chain_t ** last;
	ngx_connection_t  * connection;
	ngx_pool_t  * pool;
	nginx_off_t limit;
};

#define NGX_CHAIN_ERROR     (ngx_chain_t*)NGX_ERROR

#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)
#define ngx_buf_special(b)   ((b->flush || b->last_buf || b->sync) && !ngx_buf_in_memory(b) && !b->in_file)
#define ngx_buf_sync_only(b) (b->sync && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)
#define ngx_buf_size(b)      (ngx_buf_in_memory(b) ? (nginx_off_t)(b->last - b->pos) : (b->file_last - b->file_pos))

ngx_buf_t * FASTCALL ngx_create_temp_buf(ngx_pool_t * pool, size_t size);
ngx_chain_t * ngx_create_chain_of_bufs(ngx_pool_t * pool, ngx_bufs_t * bufs);

#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t * FASTCALL ngx_alloc_chain_link(ngx_pool_t * pool);
#define ngx_free_chain(pool, cl) cl->next = pool->chain; pool->chain = cl

ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t * ctx, ngx_chain_t * in);
ngx_int_t ngx_chain_writer(void * ctx, ngx_chain_t * in);
ngx_int_t ngx_chain_add_copy(ngx_pool_t * pool, ngx_chain_t ** chain, ngx_chain_t * in);
ngx_chain_t * FASTCALL ngx_chain_get_free_buf(ngx_pool_t * p, ngx_chain_t ** free);
void ngx_chain_update_chains(ngx_pool_t * p, ngx_chain_t ** free, ngx_chain_t ** busy, ngx_chain_t ** out, ngx_buf_tag_t tag);
nginx_off_t ngx_chain_coalesce_file(ngx_chain_t ** in, nginx_off_t limit);
ngx_chain_t * ngx_chain_update_sent(ngx_chain_t * in, nginx_off_t sent);
//
//#include <ngx_queue.h>
//
//typedef struct ngx_queue_s ngx_queue_t;

struct ngx_queue_t {
	void   Init()
	{
		prev = this;
		next = this;
	}
	ngx_queue_t * prev;
	ngx_queue_t * next;
};

//#define ngx_queue_init(q)           (q)->prev = q; (q)->next = q
#define ngx_queue_init(q)           (q)->Init()
#define ngx_queue_empty(h)          (h == (h)->prev)
#define ngx_queue_insert_head(h, x) (x)->next = (h)->next; (x)->next->prev = x; (x)->prev = h; (h)->next = x
#define ngx_queue_insert_after      ngx_queue_insert_head
#define ngx_queue_insert_tail(h, x) (x)->prev = (h)->prev; (x)->prev->next = x; (x)->next = h; (h)->prev = x
#define ngx_queue_head(h)           (h)->next
#define ngx_queue_last(h)           (h)->prev
#define ngx_queue_sentinel(h)       (h)
#define ngx_queue_next(q)           (q)->next
#define ngx_queue_prev(q)           (q)->prev
#if (NGX_DEBUG)
	#define ngx_queue_remove(x)     (x)->next->prev = (x)->prev; (x)->prev->next = (x)->next; (x)->prev = NULL; (x)->next = NULL
#else
	#define ngx_queue_remove(x)     (x)->next->prev = (x)->prev; (x)->prev->next = (x)->next
#endif
#define ngx_queue_split(h, q, n)					      \
	(n)->prev = (h)->prev;							  \
	(n)->prev->next = n;							  \
	(n)->next = q;								  \
	(h)->prev = (q)->prev;							  \
	(h)->prev->next = h;							  \
	(q)->prev = n;

#define ngx_queue_add(h, n)						      \
	(h)->prev->next = (n)->next;						  \
	(n)->next->prev = (h)->prev;						  \
	(h)->prev = (n)->prev;							  \
	(h)->prev->next = h;

#define ngx_queue_data(q, type, link)   (type*)((u_char*)q - offsetof(type, link))

ngx_queue_t * ngx_queue_middle(ngx_queue_t * queue);
void ngx_queue_sort(ngx_queue_t *queue, ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *));
//
//#include <ngx_array.h>
//
struct ngx_array_t {
	void * elts;
	ngx_uint_t nelts;
	size_t size;
	ngx_uint_t nalloc;
	ngx_pool_t * pool;
};

ngx_array_t * ngx_array_create(ngx_pool_t * p, ngx_uint_t n, size_t size);
void ngx_array_destroy(ngx_array_t * a);
void * FASTCALL ngx_array_push(ngx_array_t * a);
void * FASTCALL ngx_array_push_n(ngx_array_t * a, ngx_uint_t n);
ngx_int_t ngx_array_init(ngx_array_t * array, ngx_pool_t * pool, ngx_uint_t n, size_t size);
//
//#include <ngx_list.h>
//
//typedef struct ngx_list_part_s ngx_list_part_t;

struct /*ngx_list_part_s*/ngx_list_part_t {
	void * elts;
	ngx_uint_t nelts;
	ngx_list_part_t  * next;
};

struct ngx_list_t {
	ngx_list_part_t * last;
	ngx_list_part_t part;
	size_t size;
	ngx_uint_t nalloc;
	ngx_pool_t * pool;
};

ngx_list_t * ngx_list_create(ngx_pool_t * pool, ngx_uint_t n, size_t size);
ngx_int_t ngx_list_init(ngx_list_t * list, ngx_pool_t * pool, ngx_uint_t n, size_t size);
/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *  for(i = 0 ;; i++) {
 *      if(i >= part->nelts) {
 *          if(part->next == NULL) {
 *              break;
 *          }
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *      ...  data[i] ...
 *  }
 */
void * FASTCALL ngx_list_push(ngx_list_t * list);
//
//#include <ngx_hash.h>
//
struct ngx_hash_elt_t {
	void * value;
	u_short len;
	u_char name[1];
};

struct ngx_hash_t {
	ngx_hash_elt_t ** buckets;
	ngx_uint_t size;
};

struct ngx_hash_wildcard_t {
	ngx_hash_t hash;
	void * value;
};

struct ngx_hash_key_t {
	ngx_str_t key;
	ngx_uint_t key_hash;
	void * value;
};

typedef ngx_uint_t (*ngx_hash_key_pt)(const u_char * data, size_t len);

struct ngx_hash_combined_t {
	ngx_hash_t hash;
	ngx_hash_wildcard_t * wc_head;
	ngx_hash_wildcard_t * wc_tail;
};

struct ngx_hash_init_t {
	ngx_hash_t * hash;
	ngx_hash_key_pt key;
	ngx_uint_t max_size;
	ngx_uint_t bucket_size;
	char * name;
	ngx_pool_t  * pool;
	ngx_pool_t  * temp_pool;
};

#define NGX_HASH_SMALL            1
#define NGX_HASH_LARGE            2

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1
#define NGX_HASH_READONLY_KEY     2

struct ngx_hash_keys_arrays_t {
	ngx_uint_t hsize;
	ngx_pool_t * pool;
	ngx_pool_t * temp_pool;
	ngx_array_t keys;
	ngx_array_t * keys_hash;
	ngx_array_t dns_wc_head;
	ngx_array_t * dns_wc_head_hash;
	ngx_array_t dns_wc_tail;
	ngx_array_t * dns_wc_tail_hash;
};

struct ngx_table_elt_t {
	ngx_uint_t hash;
	ngx_str_t key;
	ngx_str_t value;
	u_char * lowcase_key;
};

void * ngx_hash_find(ngx_hash_t * hash, ngx_uint_t key, const u_char * name, size_t len);
void * ngx_hash_find_wc_head(ngx_hash_wildcard_t * hwc, const u_char * name, size_t len);
void * ngx_hash_find_wc_tail(ngx_hash_wildcard_t * hwc, const u_char * name, size_t len);
void * ngx_hash_find_combined(ngx_hash_combined_t * hash, ngx_uint_t key, const u_char * name, size_t len);
ngx_int_t ngx_hash_init(ngx_hash_init_t * hinit, ngx_hash_key_t * names, ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t * hinit, ngx_hash_key_t * names, ngx_uint_t nelts);
ngx_uint_t ngx_hash_key(const u_char * data, size_t len);
ngx_uint_t ngx_hash_key_lc(const u_char * data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char * dst, u_char * src, size_t n);
ngx_int_t FASTCALL ngx_hash_keys_array_init(ngx_hash_keys_arrays_t * ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t * ha, ngx_str_t * key, void * value, ngx_uint_t flags);

#define ngx_hash(key, c) ((ngx_uint_t)key * 31 + c)
//
//#include <ngx_file.h>
//
struct /*ngx_file_s*/ngx_file_t {
	ngx_fd_t fd;
	ngx_str_t name;
	ngx_file_info_t info;
	nginx_off_t offset;
	nginx_off_t sys_offset;
	ngx_log_t * log;
#if (NGX_THREADS || NGX_COMPAT)
	ngx_int_t (* thread_handler)(ngx_thread_task_t * task, ngx_file_t * file);
	void * thread_ctx;
	ngx_thread_task_t * thread_task;
#endif
#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
	ngx_event_aio_t * aio;
#endif
	unsigned valid_info : 1;
	unsigned directio : 1;
};

#define NGX_MAX_PATH_LEVEL  3

typedef ngx_msec_t (*ngx_path_manager_pt)(void * data);
typedef ngx_msec_t (*ngx_path_purger_pt)(void * data);
typedef void (*ngx_path_loader_pt)(void * data);

struct ngx_path_t {
	ngx_str_t name;
	size_t len;
	size_t level[NGX_MAX_PATH_LEVEL];
	ngx_path_manager_pt manager;
	ngx_path_purger_pt purger;
	ngx_path_loader_pt loader;
	void * data;
	u_char * conf_file;
	ngx_uint_t line;
};

struct ngx_path_init_t {
	ngx_str_t name;
	size_t level[NGX_MAX_PATH_LEVEL];
};

struct ngx_temp_file_t {
	ngx_file_t file;
	nginx_off_t offset;
	ngx_path_t * path;
	ngx_pool_t * pool;
	char * warn;
	ngx_uint_t access;
	unsigned log_level : 8;
	unsigned persistent : 1;
	unsigned clean : 1;
	unsigned thread_write : 1;
};

struct ngx_ext_rename_file_t {
	ngx_uint_t access;
	ngx_uint_t path_access;
	time_t time;
	ngx_fd_t fd;
	unsigned create_path : 1;
	unsigned delete_file : 1;
	ngx_log_t * log;
};

struct ngx_copy_file_t {
	nginx_off_t size;
	size_t buf_size;
	ngx_uint_t access;
	time_t time;
	ngx_log_t * log;
};

//typedef struct ngx_tree_ctx_s ngx_tree_ctx_t;
struct ngx_tree_ctx_t;

typedef ngx_int_t (*ngx_tree_init_handler_pt)(void * ctx, void * prev);
typedef ngx_int_t (*ngx_tree_handler_pt)(ngx_tree_ctx_t * ctx, ngx_str_t * name);

struct ngx_tree_ctx_t {
	nginx_off_t size;
	nginx_off_t fs_size;
	ngx_uint_t access;
	time_t mtime;
	ngx_tree_init_handler_pt init_handler;
	ngx_tree_handler_pt file_handler;
	ngx_tree_handler_pt pre_tree_handler;
	ngx_tree_handler_pt post_tree_handler;
	ngx_tree_handler_pt spec_handler;
	void  * data;
	size_t alloc;
	ngx_log_t * log;
};

ngx_int_t ngx_get_full_name(ngx_pool_t * pool, ngx_str_t * prefix, ngx_str_t * name);
ssize_t ngx_write_chain_to_temp_file(ngx_temp_file_t * tf, ngx_chain_t * chain);
ngx_int_t ngx_create_temp_file(ngx_file_t * file, ngx_path_t * path, ngx_pool_t * pool, ngx_uint_t persistent, ngx_uint_t clean, ngx_uint_t access);
void ngx_create_hashed_filename(ngx_path_t * path, u_char * file, size_t len);
ngx_int_t ngx_create_path(ngx_file_t * file, ngx_path_t * path);
ngx_err_t ngx_create_full_path(u_char * dir, ngx_uint_t access);
ngx_int_t ngx_add_path(ngx_conf_t * cf, ngx_path_t ** slot);
ngx_int_t ngx_create_paths(ngx_cycle_t * cycle, ngx_uid_t user);
ngx_int_t ngx_ext_rename_file(ngx_str_t * src, ngx_str_t * to, ngx_ext_rename_file_t * ext);
ngx_int_t ngx_copy_file(u_char * from, u_char * to, ngx_copy_file_t * cf);
ngx_int_t ngx_walk_tree(ngx_tree_ctx_t * ctx, ngx_str_t * tree);
ngx_atomic_uint_t ngx_next_temp_number(ngx_uint_t collision);
const char * ngx_conf_set_path_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
char * ngx_conf_merge_path_value(ngx_conf_t * cf, ngx_path_t ** path, ngx_path_t * prev, ngx_path_init_t * init);
const char * ngx_conf_set_access_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

extern ngx_atomic_t * ngx_temp_number;
extern ngx_atomic_int_t ngx_random_number;
//
//#include <ngx_crc.h>
//
// 32-bit crc16 
//
static ngx_inline uint32_t ngx_crc(u_char * data, size_t len)
{
	uint32_t sum = 0;
	for(; len; len--) {
		/*
		 * gcc 2.95.2 x86 and icc 7.1.006 compile that operator into the single "rol" opcode,
		 * msvc 6.0sp2 compiles it into four opcodes.
		 */
		sum = sum >> 1 | sum << 31;
		sum += *data++;
	}
	return sum;
}
//
//#include <ngx_crc32.h>
//
extern uint32_t * ngx_crc32_table_short;
extern uint32_t ngx_crc32_table256[];

static ngx_inline uint32_t ngx_crc32_short(u_char * p, size_t len)
{
	uint32_t crc = 0xffffffff;
	while(len--) {
		u_char c = *p++;
		crc = ngx_crc32_table_short[(crc ^ (c & 0xf)) & 0xf] ^ (crc >> 4);
		crc = ngx_crc32_table_short[(crc ^ (c >> 4)) & 0xf] ^ (crc >> 4);
	}
	return crc ^ 0xffffffff;
}

static ngx_inline uint32_t ngx_crc32_long(u_char * p, size_t len)
{
	uint32_t crc = 0xffffffff;
	while(len--) {
		crc = ngx_crc32_table256[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	}
	return crc ^ 0xffffffff;
}

static ngx_inline void ngx_crc32_update(uint32_t * crc, u_char * p, size_t len)
{
	uint32_t c = *crc;
	while(len--) {
		c = ngx_crc32_table256[(c ^ *p++) & 0xff] ^ (c >> 8);
	}
	*crc = c;
}

ngx_int_t ngx_crc32_table_init(void);

#define ngx_crc32_init(crc) crc = 0xffffffff
#define ngx_crc32_final(crc) crc ^= 0xffffffff
//
//#include <ngx_murmurhash.h>
//
uint32_t ngx_murmur_hash2(u_char *data, size_t len);
//
#if (NGX_PCRE)
	//
	//#include <ngx_regex.h>
	//
	#include <pcre.h>

	#define NGX_REGEX_NO_MATCHED  PCRE_ERROR_NOMATCH   /* -1 */
	#define NGX_REGEX_CASELESS    PCRE_CASELESS

	struct ngx_regex_t {
		pcre * code;
		pcre_extra  * extra;
	};

	struct ngx_regex_compile_t {
		ngx_str_t pattern;
		ngx_pool_t * pool;
		ngx_int_t options;
		ngx_regex_t  * regex;
		int captures;
		int named_captures;
		int name_size;
		u_char  * names;
		ngx_str_t err;
	};

	struct ngx_regex_elt_t {
		ngx_regex_t  * regex;
		u_char  * name;
	};

	void ngx_regex_init(void);
	ngx_int_t ngx_regex_compile(ngx_regex_compile_t * rc);
	ngx_int_t ngx_regex_exec_array(ngx_array_t * a, ngx_str_t * s, ngx_log_t * log);

	#define ngx_regex_exec(re, s, captures, size) pcre_exec(re->code, re->extra, (const char*)(s)->data, (s)->len, 0, 0, captures, size)
	#define ngx_regex_exec_n      "pcre_exec()"
	//
#endif
//
//#include <ngx_radix_tree.h>
//
#define NGX_RADIX_NO_VALUE   (uintptr_t)-1

//typedef struct ngx_radix_node_s ngx_radix_node_t;

struct ngx_radix_node_t {
	ngx_radix_node_t  * right;
	ngx_radix_node_t  * left;
	ngx_radix_node_t  * parent;
	uintptr_t value;
};

struct ngx_radix_tree_t {
	ngx_radix_node_t  * root;
	ngx_pool_t * pool;
	ngx_radix_node_t  * free;
	char * start;
	size_t size;
};

ngx_radix_tree_t * ngx_radix_tree_create(ngx_pool_t * pool, ngx_int_t preallocate);
ngx_int_t ngx_radix32tree_insert(ngx_radix_tree_t * tree, uint32_t key, uint32_t mask, uintptr_t value);
ngx_int_t ngx_radix32tree_delete(ngx_radix_tree_t * tree, uint32_t key, uint32_t mask);
uintptr_t ngx_radix32tree_find(ngx_radix_tree_t * tree, uint32_t key);
#if (NGX_HAVE_INET6)
	ngx_int_t ngx_radix128tree_insert(ngx_radix_tree_t * tree, u_char * key, u_char * mask, uintptr_t value);
	ngx_int_t ngx_radix128tree_delete(ngx_radix_tree_t * tree, u_char * key, u_char * mask);
	uintptr_t ngx_radix128tree_find(ngx_radix_tree_t * tree, u_char * key);
#endif
//
//#include <ngx_rwlock.h>
//
void FASTCALL ngx_rwlock_wlock(ngx_atomic_t * lock);
void FASTCALL ngx_rwlock_rlock(ngx_atomic_t * lock);
void FASTCALL ngx_rwlock_unlock(ngx_atomic_t * lock);
void FASTCALL ngx_rwlock_downgrade(ngx_atomic_t *lock);
//
//#include <ngx_shmtx.h>
//
struct ngx_shmtx_sh_t {
	ngx_atomic_t lock;
#if (NGX_HAVE_POSIX_SEM)
	ngx_atomic_t wait;
#endif
};

struct ngx_shmtx_t {
#if (NGX_HAVE_ATOMIC_OPS)
	ngx_atomic_t  * lock;
#if (NGX_HAVE_POSIX_SEM)
	ngx_atomic_t  * wait;
	ngx_uint_t semaphore;
	sem_t sem;
#endif
#else
	ngx_fd_t fd;
	u_char * name;
#endif
	ngx_uint_t spin;
};

ngx_int_t ngx_shmtx_create(ngx_shmtx_t * mtx, ngx_shmtx_sh_t * addr, u_char * name);
void FASTCALL ngx_shmtx_destroy(ngx_shmtx_t * mtx);
ngx_uint_t FASTCALL ngx_shmtx_trylock(ngx_shmtx_t * mtx);
void FASTCALL ngx_shmtx_lock(ngx_shmtx_t * mtx);
void FASTCALL ngx_shmtx_unlock(ngx_shmtx_t * mtx);
ngx_uint_t FASTCALL ngx_shmtx_force_unlock(ngx_shmtx_t * mtx, ngx_pid_t pid);
//
//#include <ngx_slab.h>
//
//typedef struct ngx_slab_page_s ngx_slab_page_t;

struct /*ngx_slab_page_s*/ngx_slab_page_t {
	uintptr_t slab;
	ngx_slab_page_t  * next;
	uintptr_t prev;
};

struct ngx_slab_stat_t {
	ngx_uint_t total;
	ngx_uint_t used;
	ngx_uint_t reqs;
	ngx_uint_t fails;
};

struct ngx_slab_pool_t {
	ngx_shmtx_sh_t lock;
	size_t min_size;
	size_t min_shift;
	ngx_slab_page_t  * pages;
	ngx_slab_page_t  * last;
	ngx_slab_page_t free;
	ngx_slab_stat_t  * stats;
	ngx_uint_t pfree;
	u_char * start;
	u_char * end;
	ngx_shmtx_t mutex;
	u_char * log_ctx;
	u_char zero;
	unsigned log_nomem : 1;
	void   * data;
	void   * addr;
};

void ngx_slab_init(ngx_slab_pool_t * pool);
void * ngx_slab_alloc(ngx_slab_pool_t * pool, size_t size);
void * ngx_slab_alloc_locked(ngx_slab_pool_t * pool, size_t size);
void * ngx_slab_calloc(ngx_slab_pool_t * pool, size_t size);
void * ngx_slab_calloc_locked(ngx_slab_pool_t * pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t * pool, void * p);
void ngx_slab_free_locked(ngx_slab_pool_t * pool, void * p);
//
//#include <ngx_inet.h>
//
#define NGX_INET_ADDRSTRLEN   (sizeof("255.255.255.255") - 1)
#define NGX_INET6_ADDRSTRLEN  (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") - 1)
#define NGX_UNIX_ADDRSTRLEN   (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#if (NGX_HAVE_UNIX_DOMAIN)
	#define NGX_SOCKADDR_STRLEN   (sizeof("unix:") - 1 + NGX_UNIX_ADDRSTRLEN)
#elif (NGX_HAVE_INET6)
	#define NGX_SOCKADDR_STRLEN   (NGX_INET6_ADDRSTRLEN + sizeof("[]:65535") - 1)
#else
	#define NGX_SOCKADDR_STRLEN   (NGX_INET_ADDRSTRLEN + sizeof(":65535") - 1)
#endif

/* compatibility */
#define NGX_SOCKADDRLEN       sizeof(ngx_sockaddr_t)

union ngx_sockaddr_t {
	struct sockaddr sockaddr;
	struct sockaddr_in sockaddr_in;
#if (NGX_HAVE_INET6)
	struct sockaddr_in6 sockaddr_in6;
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
	struct sockaddr_un sockaddr_un;
#endif
};

struct ngx_in_cidr_t {
	in_addr_t addr;
	in_addr_t mask;
};

#if (NGX_HAVE_INET6)
	struct ngx_in6_cidr_t {
		struct in6_addr addr;
		struct in6_addr mask;
	};
#endif

struct ngx_cidr_t {
	ngx_uint_t family;
	union {
		ngx_in_cidr_t in;
#if (NGX_HAVE_INET6)
		ngx_in6_cidr_t in6;
#endif
	} u;
};

struct ngx_addr_t {
	struct sockaddr * sockaddr;
	socklen_t socklen;
	ngx_str_t name;
};

struct ngx_url_t {
	ngx_str_t url;
	ngx_str_t host;
	ngx_str_t port_text;
	ngx_str_t uri;
	in_port_t port;
	in_port_t default_port;
	int family;
	unsigned listen : 1;
	unsigned uri_part : 1;
	unsigned no_resolve : 1;
	unsigned no_port : 1;
	unsigned wildcard : 1;
	socklen_t socklen;
	ngx_sockaddr_t sockaddr;
	ngx_addr_t * addrs;
	ngx_uint_t naddrs;
	char * err;
};

in_addr_t ngx_inet_addr(const u_char * text, size_t len);
#if (NGX_HAVE_INET6)
	ngx_int_t ngx_inet6_addr(u_char * p, size_t len, u_char * addr);
	size_t ngx_inet6_ntop(u_char * p, u_char * text, size_t len);
#endif
size_t ngx_sock_ntop(struct sockaddr * sa, socklen_t socklen, u_char * text, size_t len, ngx_uint_t port);
size_t ngx_inet_ntop(int family, void * addr, u_char * text, size_t len);
ngx_int_t ngx_ptocidr(ngx_str_t * text, ngx_cidr_t * cidr);
ngx_int_t ngx_cidr_match(struct sockaddr * sa, ngx_array_t * cidrs);
ngx_int_t ngx_parse_addr(ngx_pool_t * pool, ngx_addr_t * addr, u_char * text, size_t len);
ngx_int_t ngx_parse_addr_port(ngx_pool_t * pool, ngx_addr_t * addr, u_char * text, size_t len);
ngx_int_t ngx_parse_url(ngx_pool_t * pool, ngx_url_t * u);
ngx_int_t ngx_inet_resolve_host(ngx_pool_t * pool, ngx_url_t * u);
ngx_int_t ngx_cmp_sockaddr(struct sockaddr * sa1, socklen_t slen1, struct sockaddr * sa2, socklen_t slen2, ngx_uint_t cmp_port);
in_port_t ngx_inet_get_port(struct sockaddr * sa);
void ngx_inet_set_port(struct sockaddr * sa, in_port_t port);
//
//#include <ngx_cycle.h>
//
#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif

#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2

//typedef struct ngx_shm_zone_s ngx_shm_zone_t;
struct ngx_shm_zone_t;

typedef ngx_int_t (*ngx_shm_zone_init_pt)(ngx_shm_zone_t * zone, void * data);

struct /*ngx_shm_zone_s*/ngx_shm_zone_t {
	void * data;
	ngx_shm_t shm;
	ngx_shm_zone_init_pt F_Init/*init*/;
	void * tag;
	ngx_uint_t noreuse; // unsigned  noreuse:1;
};
//
// Descr: —труктура основного цикла обработки событий!
//
struct /*ngx_cycle_s*/ngx_cycle_t {
	void **** conf_ctx;
	ngx_pool_t * pool;
	ngx_log_t  * log;
	ngx_log_t new_log;
	ngx_uint_t log_use_stderr; // unsigned  log_use_stderr:1;
	ngx_connection_t ** files;
	ngx_connection_t * free_connections;
	ngx_uint_t free_connection_n;
	ngx_module_t ** modules;
	ngx_uint_t modules_n;
	ngx_uint_t modules_used; // unsigned  modules_used:1; 
	ngx_queue_t reusable_connections_queue;
	ngx_uint_t reusable_connections_n;
	ngx_array_t listening;
	ngx_array_t paths;
	ngx_array_t config_dump;
	ngx_rbtree_t config_dump_rbtree;
	ngx_rbtree_node_t config_dump_sentinel;
	ngx_list_t open_files;
	ngx_list_t shared_memory;
	ngx_uint_t connection_n;
	ngx_uint_t files_n;
	ngx_connection_t * connections;
	ngx_event_t * read_events;
	ngx_event_t * write_events;
	ngx_cycle_t * old_cycle;
	ngx_str_t conf_file;
	ngx_str_t conf_param;
	ngx_str_t conf_prefix;
	ngx_str_t prefix;
	ngx_str_t lock_file;
	ngx_str_t hostname;
};

struct ngx_core_conf_t {
	ngx_flag_t daemon;
	ngx_flag_t master;
	ngx_msec_t timer_resolution;
	ngx_msec_t shutdown_timeout;
	ngx_int_t worker_processes;
	ngx_int_t debug_points;
	ngx_int_t rlimit_nofile;
	nginx_off_t rlimit_core;
	int priority;
	ngx_uint_t cpu_affinity_auto;
	ngx_uint_t cpu_affinity_n;
	ngx_cpuset_t * cpu_affinity;
	char * username;
	ngx_uid_t user;
	ngx_gid_t group;
	ngx_str_t working_directory;
	ngx_str_t lock_file;
	ngx_str_t pid;
	ngx_str_t oldpid;
	ngx_array_t env;
	char ** environment;
};

#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)

class NgxStartUpOptions {
public:
	NgxStartUpOptions();
	int    ProcessCmdLine(int argc, const char * argv[]);
	const  char * GetSignalText() const;
	enum {
		fShowHelp  = 0x0001, // ngx_show_help
		fShowVer   = 0x0002, // ngx_show_version
		fShowConf  = 0x0004, // ngx_show_configure
		fTestConf  = 0x0008, // ngx_test_config
		fDumpConf  = 0x0010, // ngx_dump_config
		fQuietMode = 0x0020  // ngx_quiet_mode
	};
	enum {
		sigStop = 1, // "stop"
		sigQuit,     // "quit"
		sigReOpen,   // "reopen"
		sigReLoad    // "reload"
	};
	long   Flags;
	int    SigID;       // sigXXX  
	SString Prefix;     // ngx_prefix
	SString ConfFile;   // ngx_conf_file
	SString ConfParams; // ngx_conf_params
private:
	int    SetSignalString(const char * pSig);
};

ngx_cycle_t * ngx_init_cycle(ngx_cycle_t * old_cycle, const NgxStartUpOptions & rO);
ngx_int_t ngx_create_pidfile(ngx_str_t * name, ngx_log_t * log, const NgxStartUpOptions & rO);
void ngx_delete_pidfile(ngx_cycle_t * cycle);
//ngx_int_t ngx_signal_process(ngx_cycle_t * cycle, char * sig);
void ngx_reopen_files(ngx_cycle_t * cycle, ngx_uid_t user);
char ** ngx_set_environment(ngx_cycle_t * cycle, ngx_uint_t * last);
ngx_pid_t ngx_exec_new_binary(ngx_cycle_t * cycle, char * const * argv);
ngx_cpuset_t * ngx_get_cpu_affinity(ngx_uint_t n);
ngx_shm_zone_t * ngx_shared_memory_add(ngx_conf_t * cf, ngx_str_t * name, size_t size, void * tag);
void ngx_set_shutdown_timer(ngx_cycle_t * cycle);

extern volatile ngx_cycle_t  * ngx_cycle;
extern ngx_array_t ngx_old_cycles;
extern ngx_module_t ngx_core_module;
extern ngx_uint_t ngx_test_config__;
extern ngx_uint_t ngx_dump_config__;
//extern ngx_uint_t ngx_quiet_mode;
//
//#include <ngx_resolver.h>
//
#define NGX_RESOLVE_A         1
#define NGX_RESOLVE_CNAME     5
#define NGX_RESOLVE_PTR       12
#define NGX_RESOLVE_MX        15
#define NGX_RESOLVE_TXT       16
#if (NGX_HAVE_INET6)
	#define NGX_RESOLVE_AAAA      28
#endif
#define NGX_RESOLVE_SRV       33
#define NGX_RESOLVE_DNAME     39

#define NGX_RESOLVE_FORMERR   1
#define NGX_RESOLVE_SERVFAIL  2
#define NGX_RESOLVE_NXDOMAIN  3
#define NGX_RESOLVE_NOTIMP    4
#define NGX_RESOLVE_REFUSED   5
#define NGX_RESOLVE_TIMEDOUT  NGX_ETIMEDOUT

#define NGX_NO_RESOLVER       (void*)-1
#define NGX_RESOLVER_MAX_RECURSION    50

//typedef struct ngx_resolver_s ngx_resolver_t;
struct ngx_resolver_t;

struct ngx_resolver_connection_t {
	ngx_connection_t  * udp;
	ngx_connection_t  * tcp;
	struct sockaddr   * sockaddr;
	socklen_t socklen;
	ngx_str_t server;
	ngx_log_t log;
	ngx_buf_t * read_buf;
	ngx_buf_t * write_buf;
	ngx_resolver_t * resolver;
};

typedef struct ngx_resolver_ctx_s ngx_resolver_ctx_t;
typedef void (*ngx_resolver_handler_pt)(ngx_resolver_ctx_t * ctx);

struct ngx_resolver_addr_t {
	struct sockaddr * sockaddr;
	socklen_t socklen;
	ngx_str_t name;
	u_short priority;
	u_short weight;
};

struct ngx_resolver_srv_t {
	ngx_str_t name;
	u_short priority;
	u_short weight;
	u_short port;
};

struct ngx_resolver_srv_name_t {
	ngx_str_t name;
	u_short priority;
	u_short weight;
	u_short port;
	ngx_resolver_ctx_t  * ctx;
	ngx_int_t state;
	ngx_uint_t naddrs;
	ngx_addr_t * addrs;
};

struct ngx_resolver_node_t {
	ngx_rbtree_node_t node;
	ngx_queue_t queue;
	u_char * name; // PTR: resolved name, A: name to resolve 
#if (NGX_HAVE_INET6)
	struct in6_addr addr6; // PTR: IPv6 address to resolve (IPv4 address is in rbtree node key) 
#endif
	u_short nlen;
	u_short qlen;
	u_char * query;
#if (NGX_HAVE_INET6)
	u_char * query6;
#endif
	union {
		in_addr_t addr;
		in_addr_t * addrs;
		u_char * cname;
		ngx_resolver_srv_t  * srvs;
	} u;
	u_char code;
	u_short naddrs;
	u_short nsrvs;
	u_short cnlen;
#if (NGX_HAVE_INET6)
	union {
		struct in6_addr addr6;
		struct in6_addr * addrs6;
	} u6;
	u_short naddrs6;
#endif
	time_t expire;
	time_t valid;
	uint32_t ttl;
	unsigned tcp : 1;
#if (NGX_HAVE_INET6)
	unsigned tcp6 : 1;
#endif
	ngx_uint_t last_connection;
	ngx_resolver_ctx_t  * waiting;
};

struct /*ngx_resolver_s*/ngx_resolver_t {
	ngx_event_t * P_Ev/*event*/; // has to be pointer because of "incomplete type" 
	void * dummy;
	ngx_log_t  * log;
	ngx_int_t ident; // event ident must be after 3 pointers as in ngx_connection_t 
	// simple round robin DNS peers balancer 
	ngx_array_t connections;
	ngx_uint_t last_connection;
	ngx_rbtree_t name_rbtree;
	ngx_rbtree_node_t name_sentinel;
	ngx_rbtree_t srv_rbtree;
	ngx_rbtree_node_t srv_sentinel;
	ngx_rbtree_t addr_rbtree;
	ngx_rbtree_node_t addr_sentinel;
	ngx_queue_t name_resend_queue;
	ngx_queue_t srv_resend_queue;
	ngx_queue_t addr_resend_queue;
	ngx_queue_t name_expire_queue;
	ngx_queue_t srv_expire_queue;
	ngx_queue_t addr_expire_queue;
#if (NGX_HAVE_INET6)
	ngx_uint_t ipv6; // unsigned  ipv6:1; 
	ngx_rbtree_t addr6_rbtree;
	ngx_rbtree_node_t addr6_sentinel;
	ngx_queue_t addr6_resend_queue;
	ngx_queue_t addr6_expire_queue;
#endif
	time_t resend_timeout;
	time_t tcp_timeout;
	time_t expire;
	time_t valid;
	ngx_uint_t log_level;
};

struct ngx_resolver_ctx_s {
	ngx_resolver_ctx_t  * next;
	ngx_resolver_t * resolver;
	ngx_resolver_node_t * node;
	/* event ident must be after 3 pointers as in ngx_connection_t */
	ngx_int_t ident;
	ngx_int_t state;
	ngx_str_t name;
	ngx_str_t service;
	time_t valid;
	ngx_uint_t naddrs;
	ngx_resolver_addr_t * addrs;
	ngx_resolver_addr_t addr;
	struct sockaddr_in sin;
	ngx_uint_t count;
	ngx_uint_t nsrvs;
	ngx_resolver_srv_name_t  * srvs;
	ngx_resolver_handler_pt handler;
	void * data;
	ngx_msec_t timeout;
	unsigned quick : 1;
	unsigned async : 1;
	unsigned cancelable : 1;
	ngx_uint_t recursion;
	ngx_event_t    * event;
};

ngx_resolver_t * ngx_resolver_create(ngx_conf_t * cf, ngx_str_t * names, ngx_uint_t n);
ngx_resolver_ctx_t * ngx_resolve_start(ngx_resolver_t * r, ngx_resolver_ctx_t * temp);
ngx_int_t ngx_resolve_name(ngx_resolver_ctx_t * ctx);
void ngx_resolve_name_done(ngx_resolver_ctx_t * ctx);
ngx_int_t ngx_resolve_addr(ngx_resolver_ctx_t * ctx);
void ngx_resolve_addr_done(ngx_resolver_ctx_t * ctx);
char * ngx_resolver_strerror(ngx_int_t err);
//
#if (NGX_OPENSSL)
	//
	//#include <ngx_event_openssl.h>
	//
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <openssl/bn.h>
	#include <openssl/conf.h>
	#include <openssl/crypto.h>
	#include <openssl/dh.h>
	#ifndef OPENSSL_NO_ENGINE
		#include <openssl/engine.h>
	#endif
	#include <openssl/evp.h>
	#ifndef OPENSSL_NO_OCSP
		#include <openssl/ocsp.h>
	#endif
	#include <openssl/rand.h>
	#include <openssl/rsa.h>
	#include <openssl/x509.h>
	#include <openssl/x509v3.h>

	#define NGX_SSL_NAME     "OpenSSL"
	#if (defined LIBRESSL_VERSION_NUMBER && OPENSSL_VERSION_NUMBER == 0x20000000L)
		#undef OPENSSL_VERSION_NUMBER
		#define OPENSSL_VERSION_NUMBER  0x1000107fL
	#endif
	#if (OPENSSL_VERSION_NUMBER >= 0x10100001L)
		#define ngx_ssl_version()       OpenSSL_version(OPENSSL_VERSION)
	#else
		#define ngx_ssl_version()       SSLeay_version(SSLEAY_VERSION)
	#endif
	#define ngx_ssl_session_t       SSL_SESSION
	#define ngx_ssl_conn_t          SSL
	#if (OPENSSL_VERSION_NUMBER < 0x10002000L)
		#define SSL_is_server(s)        (s)->server
	#endif

	struct ngx_ssl_s {
		SSL_CTX  * ctx;
		ngx_log_t  * log;
		size_t buffer_size;
	};

	struct ngx_ssl_connection_s {
		ngx_ssl_conn_t   * connection;
		SSL_CTX  * session_ctx;
		ngx_int_t last;
		ngx_buf_t  * buf;
		size_t buffer_size;
		ngx_connection_handler_pt handler;
		ngx_event_handler_pt saved_read_handler;
		ngx_event_handler_pt saved_write_handler;
		unsigned handshaked : 1;
		unsigned renegotiation : 1;
		unsigned buffer : 1;
		unsigned no_wait_shutdown : 1;
		unsigned no_send_shutdown : 1;
		unsigned handshake_buffer_set : 1;
	};

	#define NGX_SSL_NO_SCACHE            -2
	#define NGX_SSL_NONE_SCACHE          -3
	#define NGX_SSL_NO_BUILTIN_SCACHE    -4
	#define NGX_SSL_DFLT_BUILTIN_SCACHE  -5

	#define NGX_SSL_MAX_SESSION_SIZE  4096

	//typedef struct ngx_ssl_sess_id_s ngx_ssl_sess_id_t;

	struct /*ngx_ssl_sess_id_s*/ngx_ssl_sess_id_t {
		ngx_rbtree_node_t node;
		u_char * id;
		size_t len;
		u_char * session;
		ngx_queue_t queue;
		time_t expire;
	#if (NGX_PTR_SIZE == 8)
		void * stub;
		u_char sess_id[32];
	#endif
	};

	struct ngx_ssl_session_cache_t {
		ngx_rbtree_t session_rbtree;
		ngx_rbtree_node_t sentinel;
		ngx_queue_t expire_queue;
	};

	#ifdef SSL_CTRL_SET_TLSEXT_TICKET_KEY_CB
	struct ngx_ssl_session_ticket_key_t {
		size_t size;
		u_char name[16];
		u_char hmac_key[32];
		u_char aes_key[32];
	};
	#endif

	#define NGX_SSL_SSLv2    0x0002
	#define NGX_SSL_SSLv3    0x0004
	#define NGX_SSL_TLSv1    0x0008
	#define NGX_SSL_TLSv1_1  0x0010
	#define NGX_SSL_TLSv1_2  0x0020
	#define NGX_SSL_TLSv1_3  0x0040

	#define NGX_SSL_BUFFER   1
	#define NGX_SSL_CLIENT   2

	#define NGX_SSL_BUFSIZE  16384

	ngx_int_t ngx_ssl_init(ngx_log_t * log);
	ngx_int_t ngx_ssl_create(ngx_ssl_t * ssl, ngx_uint_t protocols, void * data);
	ngx_int_t ngx_ssl_certificates(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_array_t * certs, ngx_array_t * keys, ngx_array_t * passwords);
	ngx_int_t ngx_ssl_certificate(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * cert, ngx_str_t * key, ngx_array_t * passwords);
	ngx_int_t ngx_ssl_ciphers(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * ciphers, ngx_uint_t prefer_server_ciphers);
	ngx_int_t ngx_ssl_client_certificate(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * cert, ngx_int_t depth);
	ngx_int_t ngx_ssl_trusted_certificate(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * cert, ngx_int_t depth);
	ngx_int_t ngx_ssl_crl(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * crl);
	ngx_int_t ngx_ssl_stapling(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * file, ngx_str_t * responder, ngx_uint_t verify);
	ngx_int_t ngx_ssl_stapling_resolver(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_resolver_t * resolver, ngx_msec_t resolver_timeout);
	RSA * ngx_ssl_rsa512_key_callback(ngx_ssl_conn_t * ssl_conn, int is_export, int key_length);
	ngx_array_t * ngx_ssl_read_password_file(ngx_conf_t * cf, ngx_str_t * file);
	ngx_int_t ngx_ssl_dhparam(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * file);
	ngx_int_t ngx_ssl_ecdh_curve(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_str_t * name);
	ngx_int_t ngx_ssl_session_cache(ngx_ssl_t * ssl, ngx_str_t * sess_ctx, ssize_t builtin_session_cache, ngx_shm_zone_t * shm_zone, time_t timeout);
	ngx_int_t ngx_ssl_session_ticket_keys(ngx_conf_t * cf, ngx_ssl_t * ssl, ngx_array_t * paths);
	ngx_int_t ngx_ssl_session_cache_init(ngx_shm_zone_t * shm_zone, void * data);
	ngx_int_t ngx_ssl_create_connection(ngx_ssl_t * ssl, ngx_connection_t * c, ngx_uint_t flags);

	void ngx_ssl_remove_cached_session(SSL_CTX * ssl, ngx_ssl_session_t * sess);
	ngx_int_t ngx_ssl_set_session(ngx_connection_t * c, ngx_ssl_session_t * session);
	#define ngx_ssl_get_session(c)      SSL_get1_session(c->ssl->connection)
	#define ngx_ssl_free_session        SSL_SESSION_free
	#define ngx_ssl_get_connection(ssl_conn) SSL_get_ex_data(ssl_conn, ngx_ssl_connection_index)
	#define ngx_ssl_get_server_conf(ssl_ctx) SSL_CTX_get_ex_data(ssl_ctx, ngx_ssl_server_conf_index)

	#define ngx_ssl_verify_error_optional(n) oneof5(n, X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT, X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN, \
		X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, X509_V_ERR_CERT_UNTRUSTED, X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)

	ngx_int_t ngx_ssl_check_host(ngx_connection_t * c, ngx_str_t * name);
	ngx_int_t ngx_ssl_get_protocol(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_cipher_name(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_ciphers(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_curves(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_session_id(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_session_reused(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_server_name(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_raw_certificate(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_certificate(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_subject_dn(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_issuer_dn(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_subject_dn_legacy(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_issuer_dn_legacy(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_serial_number(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_fingerprint(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_client_verify(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_client_v_start(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_client_v_end(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);
	ngx_int_t ngx_ssl_get_client_v_remain(ngx_connection_t * c, ngx_pool_t * pool, ngx_str_t * s);

	ngx_int_t ngx_ssl_handshake(ngx_connection_t * c);
	ssize_t ngx_ssl_recv(ngx_connection_t * c, u_char * buf, size_t size);
	ssize_t ngx_ssl_write(ngx_connection_t * c, u_char * data, size_t size);
	ssize_t ngx_ssl_recv_chain(ngx_connection_t * c, ngx_chain_t * cl, nginx_off_t limit);
	ngx_chain_t * ngx_ssl_send_chain(ngx_connection_t * c, ngx_chain_t * in, nginx_off_t limit);
	void ngx_ssl_free_buffer(ngx_connection_t * c);
	ngx_int_t ngx_ssl_shutdown(ngx_connection_t * c);
	void ngx_cdecl ngx_ssl_error(ngx_uint_t level, ngx_log_t * log, ngx_err_t err, char * fmt, ...);
	void ngx_ssl_cleanup_ctx(void * data);

	extern int ngx_ssl_connection_index;
	extern int ngx_ssl_server_conf_index;
	extern int ngx_ssl_session_cache_index;
	extern int ngx_ssl_session_ticket_keys_index;
	extern int ngx_ssl_certificate_index;
	extern int ngx_ssl_next_certificate_index;
	extern int ngx_ssl_certificate_name_index;
	extern int ngx_ssl_stapling_index;
	//
#endif
#include <ngx_process_cycle.h> // @os
//
//#include <ngx_conf_file.h>
//
/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */
#define NGX_CONF_NOARGS      0x00000001 // The directive does not take any arguments
#define NGX_CONF_TAKE1       0x00000002 // The directive takes 1 argument
#define NGX_CONF_TAKE2       0x00000004 // The directive takes 2 arguments
#define NGX_CONF_TAKE3       0x00000008 // The directive takes 3 arguments
#define NGX_CONF_TAKE4       0x00000010 // The directive takes 4 arguments
#define NGX_CONF_TAKE5       0x00000020 // The directive takes 5 arguments
#define NGX_CONF_TAKE6       0x00000040 // The directive takes 6 arguments
#define NGX_CONF_TAKE7       0x00000080 // The directive takes 7 arguments
#define NGX_CONF_MAX_ARGS    8
#define NGX_CONF_TAKE12      (NGX_CONF_TAKE1|NGX_CONF_TAKE2) // The directive takes 1 or 2 arguments
#define NGX_CONF_TAKE13      (NGX_CONF_TAKE1|NGX_CONF_TAKE3) // The directive takes 1 or 3 arguments
#define NGX_CONF_TAKE23      (NGX_CONF_TAKE2|NGX_CONF_TAKE3) // The directive takes 2 or 3 arguments
#define NGX_CONF_TAKE123     (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3) // The directive takes 1, 2 or 3 arguments
#define NGX_CONF_TAKE1234    (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3|NGX_CONF_TAKE4) // The directive takes 1, 2, 3 or 4 arguments

#define NGX_CONF_ARGS_NUMBER 0x000000ff
#define NGX_CONF_BLOCK       0x00000100 // An additional argument is a block
#define NGX_CONF_FLAG        0x00000200 // The directive is a flag (has only values СonТ and СoffТ)
#define NGX_CONF_ANY         0x00000400 // The directive takes 0 or more arguments
#define NGX_CONF_1MORE       0x00000800 // The directive takes 1 or more arguments
#define NGX_CONF_2MORE       0x00001000 // The directive takes 2 or more arguments

#define NGX_DIRECT_CONF      0x00010000

#define NGX_MAIN_CONF        0x01000000 // The directive might be specified only on the main configuration level
#define NGX_ANY_CONF         0x1F000000 // The directive might be specified on any configuration level

#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  (ngx_uint_t)-1
#define NGX_CONF_UNSET_PTR   (void*)-1
#define NGX_CONF_UNSET_SIZE  (size_t)-1
#define NGX_CONF_UNSET_MSEC  (ngx_msec_t)-1

#define NGX_CONF_OK          NULL
#define NGX_CONF_ERROR       (char*)-1  // @sobolev (void *)-->(char *)

#define NGX_CONF_BLOCK_START 1
#define NGX_CONF_BLOCK_DONE  2
#define NGX_CONF_FILE_DONE   3

#define NGX_CORE_MODULE      0x45524F43  /* "CORE" */
#define NGX_CONF_MODULE      0x464E4F43  /* "CONF" */

#define NGX_MAX_CONF_ERRSTR  1024

typedef const char * (*ngx_conf_handler_pt)(ngx_conf_t * cf, const ngx_command_t * pCmd, void * conf);

struct /*ngx_command_s*/ngx_command_t {
	const ngx_str_t Name;
	ngx_uint_t type;
	//const char * (* F_SetHandler)(ngx_conf_t *cf, const ngx_command_t *cmd, void * conf);
	ngx_conf_handler_pt F_SetHandler;
	ngx_uint_t conf;
	ngx_uint_t offset;
	void * P_Post;
};

#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }

struct /*ngx_open_file_s*/ngx_open_file_t {
	ngx_fd_t fd;
	ngx_str_t name;
	void (* flush)(ngx_open_file_t * file, ngx_log_t * log);
	void * data;
};

struct ngx_conf_file_t {
	ngx_file_t file;
	ngx_buf_t * buffer;
	ngx_buf_t * dump;
	ngx_uint_t line;
};

struct ngx_conf_dump_t {
	ngx_str_t name;
	ngx_buf_t * buffer;
};

struct /*ngx_conf_s*/ngx_conf_t {
	const char * name;
	ngx_array_t * args;
	ngx_cycle_t * cycle;
	ngx_pool_t  * pool;
	ngx_pool_t  * temp_pool;
	ngx_conf_file_t * conf_file;
	ngx_log_t * log;
	void * ctx;
	ngx_uint_t module_type;
	ngx_uint_t cmd_type;
	ngx_conf_handler_pt handler;
	char * handler_conf;
};

typedef char * (*ngx_conf_post_handler_pt)(ngx_conf_t * cf, void * data, void * conf);

struct ngx_conf_post_t {
	ngx_conf_post_handler_pt post_handler;
};

struct ngx_conf_deprecated_t {
	ngx_conf_post_handler_pt post_handler;
	char * old_name;
	char * new_name;
};

struct ngx_conf_num_bounds_t {
	ngx_conf_post_handler_pt post_handler;
	ngx_int_t low;
	ngx_int_t high;
};

struct ngx_conf_enum_t {
	ngx_str_t name;
	ngx_uint_t value;
};

#define NGX_CONF_BITMASK_SET  1

struct ngx_conf_bitmask_t {
	ngx_str_t name;
	ngx_uint_t mask;
};

char * ngx_conf_deprecated(ngx_conf_t * cf, void * post, void * data);
char * ngx_conf_check_num_bounds(ngx_conf_t * cf, void * post, void * data);

#define ngx_get_conf(conf_ctx, module)  conf_ctx[module.index]

#define ngx_conf_init_value(conf, _default)      if(conf == NGX_CONF_UNSET) { conf = _default; }
#define ngx_conf_init_ptr_value(conf, _default)  if(conf == NGX_CONF_UNSET_PTR) { conf = _default; }
#define ngx_conf_init_uint_value(conf, _default) if(conf == NGX_CONF_UNSET_UINT) { conf = _default; }
#define ngx_conf_init_size_value(conf, _default) if(conf == NGX_CONF_UNSET_SIZE) { conf = _default; }
#define ngx_conf_init_msec_value(conf, _default) if(conf == NGX_CONF_UNSET_MSEC) { conf = _default; }
#define ngx_conf_merge_value(conf, prev, _default) if(conf == NGX_CONF_UNSET) { conf = (prev == NGX_CONF_UNSET) ? _default : prev; }
#define ngx_conf_merge_ptr_value(conf, prev, _default)  if(conf == NGX_CONF_UNSET_PTR) { conf = (prev == NGX_CONF_UNSET_PTR) ? _default : prev; }
#define ngx_conf_merge_uint_value(conf, prev, _default) if(conf == NGX_CONF_UNSET_UINT) { conf = (prev == NGX_CONF_UNSET_UINT) ? _default : prev; }
#define ngx_conf_merge_msec_value(conf, prev, _default) if(conf == NGX_CONF_UNSET_MSEC) { conf = (prev == NGX_CONF_UNSET_MSEC) ? _default : prev; }
#define ngx_conf_merge_sec_value(conf, prev, _default)  if(conf == NGX_CONF_UNSET) { conf = (prev == NGX_CONF_UNSET) ? _default : prev; }
#define ngx_conf_merge_size_value(conf, prev, _default) if(conf == NGX_CONF_UNSET_SIZE) { conf = (prev == NGX_CONF_UNSET_SIZE) ? _default : prev; }
#define ngx_conf_merge_off_value(conf, prev, _default)  if(conf == NGX_CONF_UNSET) { conf = (prev == NGX_CONF_UNSET) ? _default : prev; }
#define ngx_conf_merge_str_value(conf, prev, _default)  if(!conf.data) { if(prev.data) { conf.len = prev.len; conf.data = prev.data; } else { conf.len = (sizeof(_default)-1); conf.data = (u_char*)_default; }}

#define ngx_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
	if(conf.num == 0) {							\
		if(prev.num) {							    \
			conf.num = prev.num;						 \
			conf.size = prev.size;						 \
		} else {							     \
			conf.num = default_num;						 \
			conf.size = default_size;					 \
		}								     \
	}

#define ngx_conf_merge_bitmask_value(conf, prev, _default) if(conf == 0) { conf = (prev == 0) ? _default : prev; }

char * ngx_conf_param(ngx_conf_t * cf);
char * ngx_conf_parse(ngx_conf_t * cf, ngx_str_t * filename);
const char * ngx_conf_include(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

ngx_int_t ngx_conf_full_name(ngx_cycle_t * cycle, ngx_str_t * name, ngx_uint_t conf_prefix);
ngx_open_file_t * FASTCALL ngx_conf_open_file(ngx_cycle_t * cycle, const ngx_str_t * name);
void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t * cf, ngx_err_t err, const char * fmt, ...);
const char * ngx_conf_set_flag_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_str_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_str_array_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_keyval_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_num_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_size_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_off_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_msec_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_sec_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_bufs_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_enum_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_conf_set_bitmask_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
//
//#include <ngx_module.h>
//
#define NGX_MODULE_UNSET_INDEX  (ngx_uint_t)-1
#define NGX_MODULE_SIGNATURE_0 ngx_value(NGX_PTR_SIZE) "," ngx_value(NGX_SIG_ATOMIC_T_SIZE) "," ngx_value(NGX_TIME_T_SIZE) ","

#if (NGX_HAVE_KQUEUE)
	#define NGX_MODULE_SIGNATURE_1   "1"
#else
	#define NGX_MODULE_SIGNATURE_1   "0"
#endif
#if (NGX_HAVE_IOCP)
	#define NGX_MODULE_SIGNATURE_2   "1"
#else
	#define NGX_MODULE_SIGNATURE_2   "0"
#endif
#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
	#define NGX_MODULE_SIGNATURE_3   "1"
#else
	#define NGX_MODULE_SIGNATURE_3   "0"
#endif
#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
	#define NGX_MODULE_SIGNATURE_4   "1"
#else
	#define NGX_MODULE_SIGNATURE_4   "0"
#endif
#if (NGX_HAVE_EVENTFD)
	#define NGX_MODULE_SIGNATURE_5   "1"
#else
	#define NGX_MODULE_SIGNATURE_5   "0"
#endif
#if (NGX_HAVE_EPOLL)
	#define NGX_MODULE_SIGNATURE_6   "1"
#else
	#define NGX_MODULE_SIGNATURE_6   "0"
#endif
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
	#define NGX_MODULE_SIGNATURE_7   "1"
#else
	#define NGX_MODULE_SIGNATURE_7   "0"
#endif
#if (NGX_HAVE_INET6)
	#define NGX_MODULE_SIGNATURE_8   "1"
#else
	#define NGX_MODULE_SIGNATURE_8   "0"
#endif
#define NGX_MODULE_SIGNATURE_9   "1"
#define NGX_MODULE_SIGNATURE_10  "1"
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
	#define NGX_MODULE_SIGNATURE_11  "1"
#else
	#define NGX_MODULE_SIGNATURE_11  "0"
#endif
#define NGX_MODULE_SIGNATURE_12  "1"
#if (NGX_HAVE_SETFIB)
	#define NGX_MODULE_SIGNATURE_13  "1"
#else
	#define NGX_MODULE_SIGNATURE_13  "0"
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
	#define NGX_MODULE_SIGNATURE_14  "1"
#else
	#define NGX_MODULE_SIGNATURE_14  "0"
#endif
#if (NGX_HAVE_UNIX_DOMAIN)
	#define NGX_MODULE_SIGNATURE_15  "1"
#else
	#define NGX_MODULE_SIGNATURE_15  "0"
#endif
#if (NGX_HAVE_VARIADIC_MACROS)
	#define NGX_MODULE_SIGNATURE_16  "1"
#else
	#define NGX_MODULE_SIGNATURE_16  "0"
#endif

#define NGX_MODULE_SIGNATURE_17  "0"
#define NGX_MODULE_SIGNATURE_18  "0"

#if (NGX_HAVE_OPENAT)
	#define NGX_MODULE_SIGNATURE_19  "1"
#else
	#define NGX_MODULE_SIGNATURE_19  "0"
#endif
#if (NGX_HAVE_ATOMIC_OPS)
	#define NGX_MODULE_SIGNATURE_20  "1"
#else
	#define NGX_MODULE_SIGNATURE_20  "0"
#endif
#if (NGX_HAVE_POSIX_SEM)
	#define NGX_MODULE_SIGNATURE_21  "1"
#else
	#define NGX_MODULE_SIGNATURE_21  "0"
#endif
#if (NGX_THREADS || NGX_COMPAT)
	#define NGX_MODULE_SIGNATURE_22  "1"
#else
	#define NGX_MODULE_SIGNATURE_22  "0"
#endif
#if (NGX_PCRE)
	#define NGX_MODULE_SIGNATURE_23  "1"
#else
	#define NGX_MODULE_SIGNATURE_23  "0"
#endif
#if (NGX_HTTP_SSL || NGX_COMPAT)
	#define NGX_MODULE_SIGNATURE_24  "1"
#else
	#define NGX_MODULE_SIGNATURE_24  "0"
#endif
#define NGX_MODULE_SIGNATURE_25  "1"
#if (NGX_HTTP_GZIP)
	#define NGX_MODULE_SIGNATURE_26  "1"
#else
	#define NGX_MODULE_SIGNATURE_26  "0"
#endif
#define NGX_MODULE_SIGNATURE_27  "1"
#if (NGX_HTTP_X_FORWARDED_FOR)
	#define NGX_MODULE_SIGNATURE_28  "1"
#else
#define NGX_MODULE_SIGNATURE_28  "0"
#endif
#if (NGX_HTTP_REALIP)
	#define NGX_MODULE_SIGNATURE_29  "1"
#else
	#define NGX_MODULE_SIGNATURE_29  "0"
#endif
#if (NGX_HTTP_HEADERS)
	#define NGX_MODULE_SIGNATURE_30  "1"
#else
	#define NGX_MODULE_SIGNATURE_30  "0"
#endif
#if (NGX_HTTP_DAV)
	#define NGX_MODULE_SIGNATURE_31  "1"
#else
	#define NGX_MODULE_SIGNATURE_31  "0"
#endif
#if (NGX_HTTP_CACHE)
	#define NGX_MODULE_SIGNATURE_32  "1"
#else
	#define NGX_MODULE_SIGNATURE_32  "0"
#endif
#if (NGX_HTTP_UPSTREAM_ZONE)
	#define NGX_MODULE_SIGNATURE_33  "1"
#else
	#define NGX_MODULE_SIGNATURE_33  "0"
#endif
#if (NGX_COMPAT)
	#define NGX_MODULE_SIGNATURE_34  "1"
#else
	#define NGX_MODULE_SIGNATURE_34  "0"
#endif

#define NGX_MODULE_SIGNATURE						      \
	NGX_MODULE_SIGNATURE_0 NGX_MODULE_SIGNATURE_1 NGX_MODULE_SIGNATURE_2	  \
	NGX_MODULE_SIGNATURE_3 NGX_MODULE_SIGNATURE_4 NGX_MODULE_SIGNATURE_5	  \
	NGX_MODULE_SIGNATURE_6 NGX_MODULE_SIGNATURE_7 NGX_MODULE_SIGNATURE_8	  \
	NGX_MODULE_SIGNATURE_9 NGX_MODULE_SIGNATURE_10 NGX_MODULE_SIGNATURE_11	  \
	NGX_MODULE_SIGNATURE_12 NGX_MODULE_SIGNATURE_13 NGX_MODULE_SIGNATURE_14	  \
	NGX_MODULE_SIGNATURE_15 NGX_MODULE_SIGNATURE_16 NGX_MODULE_SIGNATURE_17	  \
	NGX_MODULE_SIGNATURE_18 NGX_MODULE_SIGNATURE_19 NGX_MODULE_SIGNATURE_20	  \
	NGX_MODULE_SIGNATURE_21 NGX_MODULE_SIGNATURE_22 NGX_MODULE_SIGNATURE_23	  \
	NGX_MODULE_SIGNATURE_24 NGX_MODULE_SIGNATURE_25 NGX_MODULE_SIGNATURE_26	  \
	NGX_MODULE_SIGNATURE_27 NGX_MODULE_SIGNATURE_28 NGX_MODULE_SIGNATURE_29	  \
	NGX_MODULE_SIGNATURE_30 NGX_MODULE_SIGNATURE_31 NGX_MODULE_SIGNATURE_32	  \
	NGX_MODULE_SIGNATURE_33 NGX_MODULE_SIGNATURE_34

#define NGX_MODULE_V1          NGX_MODULE_UNSET_INDEX, NGX_MODULE_UNSET_INDEX, NULL, 0, 0, nginx_version, NGX_MODULE_SIGNATURE
#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0

struct /*ngx_module_s*/ngx_module_t {
	ngx_uint_t ctx_index;
	ngx_uint_t index;
	const char * name;
	ngx_uint_t spare0;
	ngx_uint_t spare1;
	ngx_uint_t version;
	const char * signature;
	void * ctx;
	ngx_command_t * commands;
	ngx_uint_t type;
	ngx_int_t (* init_master)(ngx_log_t * log);
	ngx_int_t (* init_module)(ngx_cycle_t * cycle);
	ngx_int_t (* init_process)(ngx_cycle_t * cycle);
	ngx_int_t (* init_thread)(ngx_cycle_t * cycle);
	void (* exit_thread)(ngx_cycle_t * cycle);
	void (* exit_process)(ngx_cycle_t * cycle);
	void (* exit_master)(ngx_cycle_t * cycle);
	uintptr_t spare_hook0;
	uintptr_t spare_hook1;
	uintptr_t spare_hook2;
	uintptr_t spare_hook3;
	uintptr_t spare_hook4;
	uintptr_t spare_hook5;
	uintptr_t spare_hook6;
	uintptr_t spare_hook7;
};

struct ngx_core_module_t {
	ngx_str_t name;
	void * (* create_conf)(ngx_cycle_t * cycle);
	const char * (* F_InitConf)(ngx_cycle_t * cycle, void * conf); // init_conf
};

ngx_int_t ngx_preinit_modules(void);
ngx_int_t ngx_cycle_modules(ngx_cycle_t * cycle);
ngx_int_t ngx_init_modules(ngx_cycle_t * cycle);
ngx_int_t ngx_count_modules(ngx_cycle_t * cycle, ngx_uint_t type);
ngx_int_t ngx_add_module(ngx_conf_t * cf, ngx_str_t * file, ngx_module_t * module, char ** order);

extern ngx_module_t * ngx_modules[];
extern ngx_uint_t ngx_max_module;
extern const char * ngx_module_names[];
//
//#include <ngx_open_file_cache.h>
//
#define NGX_OPEN_FILE_DIRECTIO_OFF  NGX_MAX_OFF_T_VALUE

struct ngx_open_file_info_t {
	ngx_fd_t fd;
	ngx_file_uniq_t uniq;
	time_t mtime;
	nginx_off_t size;
	nginx_off_t fs_size;
	nginx_off_t directio;
	size_t read_ahead;
	ngx_err_t err;
	char  * failed;
	time_t valid;
	ngx_uint_t min_uses;
#if (NGX_HAVE_OPENAT)
	size_t disable_symlinks_from;
	unsigned disable_symlinks : 2;
#endif
	unsigned test_dir : 1;
	unsigned test_only : 1;
	unsigned log : 1;
	unsigned errors : 1;
	unsigned events : 1;

	unsigned is_dir : 1;
	unsigned is_file : 1;
	unsigned is_link : 1;
	unsigned is_exec : 1;
	unsigned is_directio : 1;
};

typedef struct ngx_cached_open_file_s ngx_cached_open_file_t;

struct ngx_cached_open_file_s {
	ngx_rbtree_node_t node;
	ngx_queue_t queue;
	u_char  * name;
	time_t created;
	time_t accessed;
	ngx_fd_t fd;
	ngx_file_uniq_t uniq;
	time_t mtime;
	nginx_off_t size;
	ngx_err_t err;
	uint32_t uses;
#if (NGX_HAVE_OPENAT)
	size_t disable_symlinks_from;
	unsigned disable_symlinks : 2;
#endif
	unsigned count : 24;
	unsigned close : 1;
	unsigned use_event : 1;
	unsigned is_dir : 1;
	unsigned is_file : 1;
	unsigned is_link : 1;
	unsigned is_exec : 1;
	unsigned is_directio : 1;
	ngx_event_t * event;
};

struct ngx_open_file_cache_t {
	ngx_rbtree_t rbtree;
	ngx_rbtree_node_t sentinel;
	ngx_queue_t expire_queue;
	ngx_uint_t current;
	ngx_uint_t max;
	time_t inactive;
};

struct ngx_open_file_cache_cleanup_t {
	ngx_open_file_cache_t * cache;
	ngx_cached_open_file_t  * file;
	ngx_uint_t min_uses;
	ngx_log_t * log;
};

struct ngx_open_file_cache_event_t {
	// ngx_connection_t stub to allow use c->fd as event ident 
	void  * data;
	ngx_event_t   * read;
	ngx_event_t   * write;
	ngx_fd_t fd;
	ngx_cached_open_file_t  * file;
	ngx_open_file_cache_t * cache;
};

ngx_open_file_cache_t * ngx_open_file_cache_init(ngx_pool_t * pool, ngx_uint_t max, time_t inactive);
ngx_int_t ngx_open_cached_file(ngx_open_file_cache_t * cache, ngx_str_t * name, ngx_open_file_info_t * of, ngx_pool_t * pool);
//
#include <ngx_os.h>
//
//#include <ngx_connection.h>
//
//typedef struct ngx_listening_s ngx_listening_t;

struct /*ngx_listening_s*/ngx_listening_t {
	ngx_socket_t fd;
	struct sockaddr * sockaddr;
	socklen_t socklen;          /* size of sockaddr */
	size_t addr_text_max_len;
	ngx_str_t addr_text;
	int type;
	int backlog;
	int rcvbuf;
	int sndbuf;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
	int keepidle;
	int keepintvl;
	int keepcnt;
#endif
	ngx_connection_handler_pt handler; // handler of accepted connection 
	void * servers; // array of ngx_http_in_addr_t, for example 
	ngx_log_t log;
	ngx_log_t * logp;
	size_t pool_size;
	size_t post_accept_buffer_size; // should be here because of the AcceptEx() preread 
	ngx_msec_t post_accept_timeout; // should be here because of the deferred accept 
	ngx_listening_t  * previous;
	ngx_connection_t * connection;
	ngx_uint_t worker;
	unsigned open : 1;
	unsigned remain : 1;
	unsigned ignore : 1;
	unsigned bound : 1;            /* already bound */
	unsigned inherited : 1;        /* inherited from previous process */
	unsigned nonblocking_accept : 1;
	unsigned listen : 1;
	unsigned nonblocking : 1;
	unsigned shared : 1;         /* shared between threads or processes */
	unsigned addr_ntop : 1;
	unsigned wildcard : 1;
#if (NGX_HAVE_INET6)
	unsigned ipv6only : 1;
#endif
	unsigned reuseport : 1;
	unsigned add_reuseport : 1;
	unsigned keepalive : 2;
	unsigned deferred_accept : 1;
	unsigned delete_deferred : 1;
	unsigned add_deferred : 1;
#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
	char * accept_filter;
#endif
#if (NGX_HAVE_SETFIB)
	int setfib;
#endif
#if (NGX_HAVE_TCP_FASTOPEN)
	int fastopen;
#endif
};

typedef enum {
	NGX_ERROR_ALERT = 0,
	NGX_ERROR_ERR,
	NGX_ERROR_INFO,
	NGX_ERROR_IGNORE_ECONNRESET,
	NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;

typedef enum {
	NGX_TCP_NODELAY_UNSET = 0,
	NGX_TCP_NODELAY_SET,
	NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;

typedef enum {
	NGX_TCP_NOPUSH_UNSET = 0,
	NGX_TCP_NOPUSH_SET,
	NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;

#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01
#define NGX_HTTP_V2_BUFFERED   0x02

struct /*ngx_connection_s*/ngx_connection_t {
	void   SetLog(ngx_log_t * pLog)
	{
		log->file = pLog->file;
		log->next = pLog->next;
		log->writer = pLog->writer;
		log->wdata = pLog->wdata;
		if(!(log->Level & NGX_LOG_DEBUG_CONNECTION)) {
			log->Level = pLog->Level;
		}
	}
	void * data;
	ngx_event_t * P_EvRd;
	ngx_event_t * P_EvWr;
	ngx_socket_t fd;
	ngx_recv_pt recv;
	ngx_send_pt send;
	ngx_recv_chain_pt recv_chain;
	ngx_send_chain_pt send_chain;
	ngx_listening_t  * listening;
	nginx_off_t sent;
	ngx_log_t   * log;
	ngx_pool_t  * pool;
	int type;
	struct sockaddr  * sockaddr;
	socklen_t socklen;
	ngx_str_t addr_text;
	ngx_str_t proxy_protocol_addr;
	in_port_t proxy_protocol_port;
#if (NGX_SSL || NGX_COMPAT)
	ngx_ssl_connection_t  * ssl;
#endif
	struct sockaddr  * local_sockaddr;
	socklen_t local_socklen;
	ngx_buf_t * buffer;
	ngx_queue_t Queue;
	ngx_atomic_uint_t number;
	ngx_uint_t requests;
	unsigned buffered      : 8;
	unsigned log_error     : 3; // ngx_connection_log_error_e 
	unsigned tcp_nodelay   : 2; // ngx_connection_tcp_nodelay_e 
	unsigned tcp_nopush    : 2; // ngx_connection_tcp_nopush_e 
	unsigned timedout      : 1;
	unsigned error         : 1;
	unsigned destroyed     : 1;
	unsigned idle          : 1;
	unsigned reusable      : 1;
	unsigned close         : 1;
	unsigned shared        : 1;
	unsigned sendfile      : 1;
	unsigned sndlowat      : 1;
	unsigned need_last_buf : 1;
#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
	unsigned busy_count    : 2;
#endif
#if (NGX_THREADS || NGX_COMPAT)
	ngx_thread_task_t  * sendfile_task;
#endif
};

/* replaced with(ngx_connection_t::SetLog)
#define ngx_set_connection_log(c, l)					     \
	c->log->file = l->file;							 \
	c->log->next = l->next;							 \
	c->log->writer = l->writer;						 \
	c->log->wdata = l->wdata;						 \
	if(!(c->log->log_level & NGX_LOG_DEBUG_CONNECTION)) {			\
		c->log->log_level = l->log_level;				     \
	}
*/

ngx_listening_t * ngx_create_listening(ngx_conf_t * cf, struct sockaddr * sockaddr, socklen_t socklen);
ngx_int_t ngx_clone_listening(ngx_conf_t * cf, ngx_listening_t * ls);
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t * cycle);
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t * cycle, const NgxStartUpOptions & rO);
void ngx_configure_listening_sockets(ngx_cycle_t * cycle);
void ngx_close_listening_sockets(ngx_cycle_t * cycle);
void FASTCALL ngx_close_connection(ngx_connection_t * c);
void ngx_close_idle_connections(ngx_cycle_t * cycle);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t * c, ngx_str_t * s, ngx_uint_t port);
ngx_int_t ngx_tcp_nodelay(ngx_connection_t * c);
ngx_int_t ngx_connection_error(ngx_connection_t * c, ngx_err_t err, char * text);
ngx_connection_t * ngx_get_connection(ngx_socket_t s, ngx_log_t * log);
void FASTCALL ngx_free_connection(ngx_connection_t * c);
void ngx_reusable_connection(ngx_connection_t * c, ngx_uint_t reusable);
//
#include <ngx_http.h>
//
//#include <ngx_syslog.h>
//
struct ngx_syslog_peer_t {
	ngx_pool_t  * pool;
	ngx_uint_t facility;
	ngx_uint_t severity;
	ngx_str_t tag;
	ngx_addr_t server;
	ngx_connection_t conn;
	unsigned busy : 1;
	unsigned nohostname : 1;
};

char * ngx_syslog_process_conf(ngx_conf_t * cf, ngx_syslog_peer_t * peer);
u_char * ngx_syslog_add_header(ngx_syslog_peer_t * peer, u_char * buf);
void ngx_syslog_writer(ngx_log_t * log, ngx_uint_t level, u_char * buf, size_t len);
ssize_t ngx_syslog_send(ngx_syslog_peer_t * peer, u_char * buf, size_t len);
//
//#include <ngx_proxy_protocol.h>
//
#define NGX_PROXY_PROTOCOL_MAX_HEADER  107

u_char * ngx_proxy_protocol_read(ngx_connection_t *c, u_char *buf, u_char *last);
u_char * ngx_proxy_protocol_write(ngx_connection_t *c, u_char *buf, u_char *last);
//
//#include <ngx_crypt.h>
//
ngx_int_t ngx_crypt(ngx_pool_t *pool, u_char *key, u_char *salt, u_char **encrypted);
//
#define LF     (u_char)'\n'
#define __CR   (u_char)'\r'
#define CRLF   "\r\n"

// (moved above) #define ngx_abs(value)       (((value) >= 0) ? (value) : -(value))
// @soboelv #define ngx_max_Removed(val1, val2)  ((val1 < val2) ? (val2) : (val1))
// @soboelv #define ngx_min_Removed(val1, val2)  ((val1 > val2) ? (val2) : (val1))

void ngx_cpuinfo(void);

#if (NGX_HAVE_OPENAT)
	#define NGX_DISABLE_SYMLINKS_OFF        0
	#define NGX_DISABLE_SYMLINKS_ON         1
	#define NGX_DISABLE_SYMLINKS_NOTOWNER   2
#endif
//
//#include <ngx_stream.h>
//
#if (NGX_STREAM_SSL)
	//
	//#include <ngx_stream_ssl_module.h>
	//
	struct ngx_stream_ssl_conf_t {
		ngx_msec_t handshake_timeout;
		ngx_flag_t prefer_server_ciphers;
		ngx_ssl_t ssl;
		ngx_uint_t protocols;
		ngx_uint_t verify;
		ngx_uint_t verify_depth;
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
	};

	extern ngx_module_t ngx_stream_ssl_module;
	//
#endif

typedef struct ngx_stream_session_s ngx_stream_session_t;
//
//#include <ngx_stream_variables.h>
//
typedef ngx_variable_value_t ngx_stream_variable_value_t;

#define ngx_stream_variable(v)     { sizeof(v) - 1, 1, 0, 0, 0, (u_char*)v }

typedef struct ngx_stream_variable_s ngx_stream_variable_t;

typedef void (*ngx_stream_set_variable_pt)(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data);
typedef ngx_int_t (*ngx_stream_get_variable_pt)(ngx_stream_session_t * s, ngx_stream_variable_value_t * v, uintptr_t data);

#define NGX_STREAM_VAR_CHANGEABLE   1
#define NGX_STREAM_VAR_NOCACHEABLE  2
#define NGX_STREAM_VAR_INDEXED      4
#define NGX_STREAM_VAR_NOHASH       8
#define NGX_STREAM_VAR_WEAK         16
#define NGX_STREAM_VAR_PREFIX       32

struct ngx_stream_variable_s {
	ngx_str_t name;                   /* must be first to build the hash */
	ngx_stream_set_variable_pt set_handler;
	ngx_stream_get_variable_pt get_handler;
	uintptr_t data;
	ngx_uint_t flags;
	ngx_uint_t index;
};

ngx_stream_variable_t * ngx_stream_add_variable(ngx_conf_t * cf, ngx_str_t * name, ngx_uint_t flags);
ngx_int_t ngx_stream_get_variable_index(ngx_conf_t * cf, ngx_str_t * name);
ngx_stream_variable_value_t * ngx_stream_get_indexed_variable(ngx_stream_session_t * s, ngx_uint_t index);
ngx_stream_variable_value_t * ngx_stream_get_flushed_variable(ngx_stream_session_t * s, ngx_uint_t index);
ngx_stream_variable_value_t * ngx_stream_get_variable(ngx_stream_session_t * s, ngx_str_t * name, ngx_uint_t key);

#if (NGX_PCRE)
	struct ngx_stream_regex_variable_t {
		ngx_uint_t capture;
		ngx_int_t index;
	};

	struct ngx_stream_regex_t {
		ngx_regex_t  * regex;
		ngx_uint_t ncaptures;
		ngx_stream_regex_variable_t  * variables;
		ngx_uint_t nvariables;
		ngx_str_t name;
	};

	struct ngx_stream_map_regex_t {
		ngx_stream_regex_t * regex;
		void  * value;
	};

	ngx_stream_regex_t * ngx_stream_regex_compile(ngx_conf_t * cf, ngx_regex_compile_t * rc);
	ngx_int_t ngx_stream_regex_exec(ngx_stream_session_t * s, ngx_stream_regex_t * re, ngx_str_t * str);
#endif

struct ngx_stream_map_t {
	ngx_hash_combined_t hash;
#if (NGX_PCRE)
	ngx_stream_map_regex_t  * regex;
	ngx_uint_t nregex;
#endif
};

void * ngx_stream_map_find(ngx_stream_session_t * s, ngx_stream_map_t * map, ngx_str_t * match);
ngx_int_t ngx_stream_variables_add_core_vars(ngx_conf_t * cf);
ngx_int_t ngx_stream_variables_init_vars(ngx_conf_t * cf);

extern ngx_stream_variable_value_t ngx_stream_variable_null_value;
extern ngx_stream_variable_value_t ngx_stream_variable_true_value;
//
//#include <ngx_stream_script.h>
//
struct ngx_stream_script_engine_t {
	u_char * ip;
	u_char * pos;
	ngx_stream_variable_value_t  * sp;
	ngx_str_t buf;
	ngx_str_t line;
	unsigned flushed : 1;
	unsigned skip : 1;
	ngx_stream_session_t * session;
};

struct ngx_stream_script_compile_t {
	ngx_conf_t * cf;
	ngx_str_t  * source;
	ngx_array_t ** flushes;
	ngx_array_t ** lengths;
	ngx_array_t ** values;
	ngx_uint_t variables;
	ngx_uint_t ncaptures;
	ngx_uint_t size;
	void * main;
	unsigned complete_lengths : 1;
	unsigned complete_values : 1;
	unsigned zero : 1;
	unsigned conf_prefix : 1;
	unsigned root_prefix : 1;
};

struct ngx_stream_complex_value_t {
	ngx_str_t value;
	ngx_uint_t * flushes;
	void * lengths;
	void * values;
};

struct ngx_stream_compile_complex_value_t {
	ngx_conf_t * cf;
	ngx_str_t  * value;
	ngx_stream_complex_value_t * complex_value;
	unsigned zero : 1;
	unsigned conf_prefix : 1;
	unsigned root_prefix : 1;
};

typedef void (*ngx_stream_script_code_pt)(ngx_stream_script_engine_t * e);
typedef size_t (*ngx_stream_script_len_code_pt)(ngx_stream_script_engine_t * e);

struct ngx_stream_script_copy_code_t {
	ngx_stream_script_code_pt code;
	uintptr_t len;
};

struct ngx_stream_script_var_code_t {
	ngx_stream_script_code_pt code;
	uintptr_t index;
};

struct ngx_stream_script_copy_capture_code_t {
	ngx_stream_script_code_pt code;
	uintptr_t n;
};

struct ngx_stream_script_full_name_code_t {
	ngx_stream_script_code_pt code;
	uintptr_t conf_prefix;
};

void ngx_stream_script_flush_complex_value(ngx_stream_session_t * s, ngx_stream_complex_value_t * val);
ngx_int_t ngx_stream_complex_value(ngx_stream_session_t * s, ngx_stream_complex_value_t * val, ngx_str_t * value);
ngx_int_t ngx_stream_compile_complex_value(ngx_stream_compile_complex_value_t * ccv);
const char * ngx_stream_set_complex_value_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
ngx_uint_t FASTCALL ngx_stream_script_variables_count(const ngx_str_t * value);
ngx_int_t ngx_stream_script_compile(ngx_stream_script_compile_t * sc);
u_char * ngx_stream_script_run(ngx_stream_session_t * s, ngx_str_t * value, void * code_lengths, size_t reserved, void * code_values);
void ngx_stream_script_flush_no_cacheable_variables(ngx_stream_session_t * s, const ngx_array_t * indices);
void * ngx_stream_script_add_code(ngx_array_t * codes, size_t size, void * code);
size_t ngx_stream_script_copy_len_code(ngx_stream_script_engine_t * e);
void ngx_stream_script_copy_code(ngx_stream_script_engine_t * e);
size_t ngx_stream_script_copy_var_len_code(ngx_stream_script_engine_t * e);
void ngx_stream_script_copy_var_code(ngx_stream_script_engine_t * e);
size_t ngx_stream_script_copy_capture_len_code(ngx_stream_script_engine_t * e);
void ngx_stream_script_copy_capture_code(ngx_stream_script_engine_t * e);
//
//#include <ngx_stream_upstream.h>
//
#define NGX_STREAM_UPSTREAM_CREATE        0x0001
#define NGX_STREAM_UPSTREAM_WEIGHT        0x0002
#define NGX_STREAM_UPSTREAM_MAX_FAILS     0x0004
#define NGX_STREAM_UPSTREAM_FAIL_TIMEOUT  0x0008
#define NGX_STREAM_UPSTREAM_DOWN          0x0010
#define NGX_STREAM_UPSTREAM_BACKUP        0x0020
#define NGX_STREAM_UPSTREAM_MAX_CONNS     0x0100

#define NGX_STREAM_UPSTREAM_NOTIFY_CONNECT     0x1

struct ngx_stream_upstream_main_conf_t {
	ngx_array_t upstreams;
	/* ngx_stream_upstream_srv_conf_t */
};

typedef struct ngx_stream_upstream_srv_conf_s ngx_stream_upstream_srv_conf_t;

typedef ngx_int_t (*ngx_stream_upstream_init_pt)(ngx_conf_t * cf, ngx_stream_upstream_srv_conf_t * us);
typedef ngx_int_t (*ngx_stream_upstream_init_peer_pt)(ngx_stream_session_t * s, ngx_stream_upstream_srv_conf_t * us);

struct ngx_stream_upstream_peer_t {
	ngx_stream_upstream_init_pt init_upstream;
	ngx_stream_upstream_init_peer_pt init;
	void  * data;
};

struct ngx_stream_upstream_server_t {
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
	NGX_COMPAT_BEGIN(4)
	NGX_COMPAT_END
};

struct ngx_stream_upstream_srv_conf_s {
	ngx_stream_upstream_peer_t peer;
	void  ** srv_conf;
	ngx_array_t   * servers;
	/* ngx_stream_upstream_server_t */
	ngx_uint_t flags;
	ngx_str_t host;
	u_char   * file_name;
	ngx_uint_t line;
	in_port_t port;
	ngx_uint_t no_port;                      /* unsigned no_port:1 */
#if (NGX_STREAM_UPSTREAM_ZONE)
	ngx_shm_zone_t  * shm_zone;
#endif
};

struct ngx_stream_upstream_state_t {
	ngx_msec_t response_time;
	ngx_msec_t connect_time;
	ngx_msec_t first_byte_time;
	nginx_off_t bytes_sent;
	nginx_off_t bytes_received;
	ngx_str_t  * peer;
};

struct ngx_stream_upstream_resolved_t {
	ngx_str_t host;
	in_port_t port;
	ngx_uint_t no_port;                     /* unsigned no_port:1 */
	ngx_uint_t naddrs;
	ngx_resolver_addr_t * addrs;
	struct sockaddr * sockaddr;
	socklen_t socklen;
	ngx_str_t name;
	ngx_resolver_ctx_t  * ctx;
};

struct ngx_stream_upstream_t {
	ngx_peer_connection_t peer;
	ngx_buf_t downstream_buf;
	ngx_buf_t upstream_buf;
	ngx_chain_t * P_Free;
	ngx_chain_t * upstream_out;
	ngx_chain_t * upstream_busy;
	ngx_chain_t * downstream_out;
	ngx_chain_t * downstream_busy;
	nginx_off_t received;
	time_t start_sec;
	ngx_uint_t responses;
	ngx_str_t ssl_name;
	ngx_stream_upstream_srv_conf_t * upstream;
	ngx_stream_upstream_resolved_t * resolved;
	ngx_stream_upstream_state_t  * state;
	unsigned connected : 1;
	unsigned proxy_protocol : 1;
};

ngx_stream_upstream_srv_conf_t * ngx_stream_upstream_add(ngx_conf_t * cf, ngx_url_t * u, ngx_uint_t flags);

#define ngx_stream_conf_upstream_srv_conf(uscf, module) uscf->srv_conf[module.ctx_index]
extern ngx_module_t ngx_stream_upstream_module;
//
//#include <ngx_stream_upstream_round_robin.h>
//
//typedef struct ngx_stream_upstream_rr_peer_s ngx_stream_upstream_rr_peer_t;

struct /*ngx_stream_upstream_rr_peer_s*/ngx_stream_upstream_rr_peer_t {
	struct sockaddr * sockaddr;
	socklen_t socklen;
	ngx_str_t name;
	ngx_str_t server;
	ngx_int_t current_weight;
	ngx_int_t effective_weight;
	ngx_int_t weight;
	ngx_uint_t conns;
	ngx_uint_t max_conns;
	ngx_uint_t fails;
	time_t accessed;
	time_t checked;
	ngx_uint_t max_fails;
	time_t fail_timeout;
	ngx_msec_t slow_start;
	ngx_msec_t start_time;
	ngx_uint_t down;
	void * ssl_session;
	int ssl_session_len;
#if (NGX_STREAM_UPSTREAM_ZONE)
	ngx_atomic_t lock;
#endif
	ngx_stream_upstream_rr_peer_t   * next;
	NGX_COMPAT_BEGIN(25)
	NGX_COMPAT_END
};

typedef struct ngx_stream_upstream_rr_peers_s ngx_stream_upstream_rr_peers_t;

struct ngx_stream_upstream_rr_peers_s {
	ngx_uint_t number;
#if (NGX_STREAM_UPSTREAM_ZONE)
	ngx_slab_pool_t                 * shpool;
	ngx_atomic_t rwlock;
	ngx_stream_upstream_rr_peers_t  * zone_next;
#endif
	ngx_uint_t total_weight;
	unsigned single : 1;
	unsigned weighted : 1;
	ngx_str_t                       * name;
	ngx_stream_upstream_rr_peers_t  * next;
	ngx_stream_upstream_rr_peer_t   * peer;
};

#if (NGX_STREAM_UPSTREAM_ZONE)
	#define ngx_stream_upstream_rr_peers_rlock(peers)       if(peers->shpool) { ngx_rwlock_rlock(&peers->rwlock); }
	#define ngx_stream_upstream_rr_peers_wlock(peers)       if(peers->shpool) { ngx_rwlock_wlock(&peers->rwlock); }
	#define ngx_stream_upstream_rr_peers_unlock(peers)      if(peers->shpool) { ngx_rwlock_unlock(&peers->rwlock); }
	#define ngx_stream_upstream_rr_peer_lock(peers, peer)   if(peers->shpool) { ngx_rwlock_wlock(&peer->lock); }
	#define ngx_stream_upstream_rr_peer_unlock(peers, peer) if(peers->shpool) { ngx_rwlock_unlock(&peer->lock); }
#else
	#define ngx_stream_upstream_rr_peers_rlock(peers)
	#define ngx_stream_upstream_rr_peers_wlock(peers)
	#define ngx_stream_upstream_rr_peers_unlock(peers)
	#define ngx_stream_upstream_rr_peer_lock(peers, peer)
	#define ngx_stream_upstream_rr_peer_unlock(peers, peer)
#endif

struct ngx_stream_upstream_rr_peer_data_t {
	ngx_uint_t config;
	ngx_stream_upstream_rr_peers_t  * peers;
	ngx_stream_upstream_rr_peer_t   * current;
	uintptr_t                       * tried;
	uintptr_t data;
};

ngx_int_t ngx_stream_upstream_init_round_robin(ngx_conf_t * cf, ngx_stream_upstream_srv_conf_t * us);
ngx_int_t ngx_stream_upstream_init_round_robin_peer(ngx_stream_session_t * s, ngx_stream_upstream_srv_conf_t * us);
ngx_int_t ngx_stream_upstream_create_round_robin_peer(ngx_stream_session_t * s, ngx_stream_upstream_resolved_t * ur);
ngx_int_t ngx_stream_upstream_get_round_robin_peer(ngx_peer_connection_t * pc, void * data);
void ngx_stream_upstream_free_round_robin_peer(ngx_peer_connection_t * pc, void * data, ngx_uint_t state);
//
#define NGX_STREAM_OK                        200
#define NGX_STREAM_BAD_REQUEST               400
#define NGX_STREAM_FORBIDDEN                 403
#define NGX_STREAM_INTERNAL_SERVER_ERROR     500
#define NGX_STREAM_BAD_GATEWAY               502
#define NGX_STREAM_SERVICE_UNAVAILABLE       503

struct ngx_stream_conf_ctx_t {
	void ** main_conf;
	void ** srv_conf;
};

struct ngx_stream_listen_t {
	ngx_sockaddr_t sockaddr;
	socklen_t socklen;
	/* server ctx */
	ngx_stream_conf_ctx_t  * ctx;
	unsigned bind : 1;
	unsigned wildcard : 1;
	unsigned ssl : 1;
#if (NGX_HAVE_INET6)
	unsigned ipv6only : 1;
#endif
	unsigned reuseport : 1;
	unsigned so_keepalive : 2;
	unsigned proxy_protocol : 1;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
	int tcp_keepidle;
	int tcp_keepintvl;
	int tcp_keepcnt;
#endif
	int backlog;
	int rcvbuf;
	int sndbuf;
	int type;
};

struct ngx_stream_addr_conf_t {
	ngx_stream_conf_ctx_t  * ctx;
	ngx_str_t addr_text;
	unsigned ssl : 1;
	unsigned proxy_protocol : 1;
};

struct ngx_stream_in_addr_t {
	in_addr_t addr;
	ngx_stream_addr_conf_t conf;
};

#if (NGX_HAVE_INET6)

struct ngx_stream_in6_addr_t {
	struct in6_addr addr6;
	ngx_stream_addr_conf_t conf;
};

#endif

struct ngx_stream_port_t {
	/* ngx_stream_in_addr_t or ngx_stream_in6_addr_t */
	void * addrs;
	ngx_uint_t naddrs;
};

struct ngx_stream_conf_port_t {
	int family;
	int type;
	in_port_t port;
	ngx_array_t addrs; // array of ngx_stream_conf_addr_t 
};

struct ngx_stream_conf_addr_t {
	ngx_stream_listen_t opt;
};

typedef enum {
	NGX_STREAM_POST_ACCEPT_PHASE = 0,
	NGX_STREAM_PREACCESS_PHASE,
	NGX_STREAM_ACCESS_PHASE,
	NGX_STREAM_SSL_PHASE,
	NGX_STREAM_PREREAD_PHASE,
	NGX_STREAM_CONTENT_PHASE,
	NGX_STREAM_LOG_PHASE
} ngx_stream_phases;

typedef struct ngx_stream_phase_handler_s ngx_stream_phase_handler_t;
typedef ngx_int_t (*ngx_stream_phase_handler_pt)(ngx_stream_session_t * s, ngx_stream_phase_handler_t * ph);
typedef ngx_int_t (*ngx_stream_handler_pt)(ngx_stream_session_t * s);
typedef void (*ngx_stream_content_handler_pt)(ngx_stream_session_t * s);

struct ngx_stream_phase_handler_s {
	ngx_stream_phase_handler_pt checker;
	ngx_stream_handler_pt handler;
	ngx_uint_t next;
};

struct ngx_stream_phase_engine_t {
	ngx_stream_phase_handler_t  * handlers;
};

struct ngx_stream_phase_t {
	ngx_array_t handlers;
};

struct ngx_stream_core_main_conf_t {
	ngx_array_t servers;                    /* ngx_stream_core_srv_conf_t */
	ngx_array_t listen;                     /* ngx_stream_listen_t */
	ngx_stream_phase_engine_t phase_engine;
	ngx_hash_t variables_hash;
	ngx_array_t variables;                       /* ngx_stream_variable_t */
	ngx_array_t prefix_variables;                /* ngx_stream_variable_t */
	ngx_uint_t ncaptures;
	ngx_uint_t variables_hash_max_size;
	ngx_uint_t variables_hash_bucket_size;
	ngx_hash_keys_arrays_t * variables_keys;
	ngx_stream_phase_t phases[NGX_STREAM_LOG_PHASE + 1];
};

struct ngx_stream_core_srv_conf_t {
	ngx_stream_content_handler_pt handler;
	ngx_stream_conf_ctx_t  * ctx;
	u_char * file_name;
	ngx_uint_t line;
	ngx_flag_t tcp_nodelay;
	size_t preread_buffer_size;
	ngx_msec_t preread_timeout;
	ngx_log_t * error_log;
	ngx_msec_t resolver_timeout;
	ngx_resolver_t  * resolver;
	ngx_msec_t proxy_protocol_timeout;
	ngx_uint_t listen; /* unsigned  listen:1; */
};

struct ngx_stream_session_s {
	uint32_t signature; /* "STRM" */
	ngx_connection_t    * connection;
	nginx_off_t received;
	time_t start_sec;
	ngx_msec_t start_msec;
	ngx_log_handler_pt log_handler;
	void ** ctx;
	void ** main_conf;
	void ** srv_conf;
	ngx_stream_upstream_t  * upstream;
	ngx_array_t * upstream_states;
	/* of ngx_stream_upstream_state_t */
	ngx_stream_variable_value_t * variables;
#if (NGX_PCRE)
	ngx_uint_t ncaptures;
	int  * captures;
	u_char * captures_data;
#endif
	ngx_int_t phase_handler;
	ngx_uint_t status;
	unsigned ssl : 1;
	unsigned stat_processing : 1;
	unsigned health_check : 1;
};

struct ngx_stream_module_t {
	ngx_int_t (* preconfiguration)(ngx_conf_t * cf);
	ngx_int_t (* postconfiguration)(ngx_conf_t * cf);
	void * (*create_main_conf)(ngx_conf_t *cf);
	char * (*init_main_conf)(ngx_conf_t *cf, void * conf);
	void * (*create_srv_conf)(ngx_conf_t *cf);
	char * (*merge_srv_conf)(ngx_conf_t *cf, void * prev, void * conf);
};

#define NGX_STREAM_MODULE       0x4d525453     /* "STRM" */

#define NGX_STREAM_MAIN_CONF    0x02000000
#define NGX_STREAM_SRV_CONF     0x04000000
#define NGX_STREAM_UPS_CONF     0x08000000

#define NGX_STREAM_MAIN_CONF_OFFSET  offsetof(ngx_stream_conf_ctx_t, main_conf)
#define NGX_STREAM_SRV_CONF_OFFSET   offsetof(ngx_stream_conf_ctx_t, srv_conf)

#define ngx_stream_get_module_ctx(s, module)   (s)->ctx[module.ctx_index]
#define ngx_stream_set_ctx(s, c, module)       s->ctx[module.ctx_index] = c;
#define ngx_stream_delete_ctx(s, module)       s->ctx[module.ctx_index] = NULL;
#define ngx_stream_get_module_main_conf(s, module) (s)->main_conf[module.ctx_index]
#define ngx_stream_get_module_srv_conf(s, module)  (s)->srv_conf[module.ctx_index]
#define ngx_stream_conf_get_module_main_conf(cf, module) ((ngx_stream_conf_ctx_t*)cf->ctx)->main_conf[module.ctx_index]
#define ngx_stream_conf_get_module_srv_conf(cf, module)  ((ngx_stream_conf_ctx_t*)cf->ctx)->srv_conf[module.ctx_index]
#define ngx_stream_cycle_get_module_main_conf(cycle, module) (cycle->conf_ctx[ngx_stream_module.index] ? ((ngx_stream_conf_ctx_t*)cycle->conf_ctx[ngx_stream_module.index])->main_conf[module.ctx_index] : NULL)
#define NGX_STREAM_WRITE_BUFFERED  0x10

void ngx_stream_core_run_phases(ngx_stream_session_t * s);
ngx_int_t ngx_stream_core_generic_phase(ngx_stream_session_t * s, ngx_stream_phase_handler_t * ph);
ngx_int_t ngx_stream_core_preread_phase(ngx_stream_session_t * s, ngx_stream_phase_handler_t * ph);
ngx_int_t ngx_stream_core_content_phase(ngx_stream_session_t * s, ngx_stream_phase_handler_t * ph);

void ngx_stream_init_connection(ngx_connection_t * c);
void ngx_stream_session_handler(ngx_event_t * rev);
void ngx_stream_finalize_session(ngx_stream_session_t * s, ngx_uint_t rc);

extern ngx_module_t ngx_stream_module;
extern ngx_uint_t ngx_stream_max_module;
extern ngx_module_t ngx_stream_core_module;

typedef ngx_int_t (*ngx_stream_filter_pt)(ngx_stream_session_t * s, ngx_chain_t * chain, ngx_uint_t from_upstream);

extern ngx_stream_filter_pt ngx_stream_top_filter;
//
//#include <ngx_mail.h>
//
#if (NGX_MAIL_SSL)
	//
	//#include <ngx_mail_ssl_module.h>
	//
	#define NGX_MAIL_STARTTLS_OFF   0
	#define NGX_MAIL_STARTTLS_ON    1
	#define NGX_MAIL_STARTTLS_ONLY  2

	struct ngx_mail_ssl_conf_t {
		ngx_flag_t enable;
		ngx_flag_t prefer_server_ciphers;
		ngx_ssl_t ssl;
		ngx_uint_t starttls;
		ngx_uint_t protocols;
		ngx_uint_t verify;
		ngx_uint_t verify_depth;
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
		ngx_shm_zone_t  * shm_zone;
		ngx_flag_t session_tickets;
		ngx_array_t * session_ticket_keys;
		u_char * file;
		ngx_uint_t line;
	};

	extern ngx_module_t ngx_mail_ssl_module;
	//
#endif

struct ngx_mail_conf_ctx_t {
	void ** main_conf;
	void ** srv_conf;
};

struct ngx_mail_listen_t {
	ngx_sockaddr_t sockaddr;
	socklen_t socklen;
	ngx_mail_conf_ctx_t  * ctx; // server ctx 
	unsigned bind : 1;
	unsigned wildcard : 1;
	unsigned ssl : 1;
#if (NGX_HAVE_INET6)
	unsigned ipv6only : 1;
#endif
	unsigned so_keepalive : 2;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
	int tcp_keepidle;
	int tcp_keepintvl;
	int tcp_keepcnt;
#endif
	int backlog;
	int rcvbuf;
	int sndbuf;
};

struct ngx_mail_addr_conf_t {
	ngx_mail_conf_ctx_t  * ctx;
	ngx_str_t addr_text;
	ngx_uint_t ssl;             /* unsigned   ssl:1; */
};

struct ngx_mail_in_addr_t {
	in_addr_t addr;
	ngx_mail_addr_conf_t conf;
};

#if (NGX_HAVE_INET6)
	struct ngx_mail_in6_addr_t {
		struct in6_addr addr6;
		ngx_mail_addr_conf_t conf;
	};
#endif

struct ngx_mail_port_t {
	void * addrs; // ngx_mail_in_addr_t or ngx_mail_in6_addr_t 
	ngx_uint_t naddrs;
};

struct ngx_mail_conf_port_t {
	int family;
	in_port_t port;
	ngx_array_t addrs;               /* array of ngx_mail_conf_addr_t */
};

struct ngx_mail_conf_addr_t {
	ngx_mail_listen_t opt;
};

struct ngx_mail_core_main_conf_t {
	ngx_array_t servers;             /* ngx_mail_core_srv_conf_t */
	ngx_array_t listen;              /* ngx_mail_listen_t */
};

#define NGX_MAIL_POP3_PROTOCOL  0
#define NGX_MAIL_IMAP_PROTOCOL  1
#define NGX_MAIL_SMTP_PROTOCOL  2

typedef struct ngx_mail_protocol_s ngx_mail_protocol_t;

struct ngx_mail_core_srv_conf_t {
	ngx_mail_protocol_t  * protocol;
	ngx_msec_t timeout;
	ngx_msec_t resolver_timeout;
	ngx_str_t server_name;
	u_char * file_name;
	ngx_uint_t line;
	ngx_resolver_t  * resolver;
	ngx_log_t    * error_log;
	ngx_mail_conf_ctx_t  * ctx; // server ctx 
	ngx_uint_t listen;           /* unsigned  listen:1; */
};

typedef enum {
	ngx_pop3_start = 0,
	ngx_pop3_user,
	ngx_pop3_passwd,
	ngx_pop3_auth_login_username,
	ngx_pop3_auth_login_password,
	ngx_pop3_auth_plain,
	ngx_pop3_auth_cram_md5,
	ngx_pop3_auth_external
} ngx_pop3_state_e;

typedef enum {
	ngx_imap_start = 0,
	ngx_imap_auth_login_username,
	ngx_imap_auth_login_password,
	ngx_imap_auth_plain,
	ngx_imap_auth_cram_md5,
	ngx_imap_auth_external,
	ngx_imap_login,
	ngx_imap_user,
	ngx_imap_passwd
} ngx_imap_state_e;

typedef enum {
	ngx_smtp_start = 0,
	ngx_smtp_auth_login_username,
	ngx_smtp_auth_login_password,
	ngx_smtp_auth_plain,
	ngx_smtp_auth_cram_md5,
	ngx_smtp_auth_external,
	ngx_smtp_helo,
	ngx_smtp_helo_xclient,
	ngx_smtp_helo_from,
	ngx_smtp_xclient,
	ngx_smtp_xclient_from,
	ngx_smtp_xclient_helo,
	ngx_smtp_from,
	ngx_smtp_to
} ngx_smtp_state_e;

struct ngx_mail_proxy_ctx_t {
	ngx_peer_connection_t upstream;
	ngx_buf_t * buffer;
};

struct ngx_mail_session_t {
	uint32_t signature;                    /* "MAIL" */
	ngx_connection_t  * connection;
	ngx_str_t out;
	ngx_buf_t    * buffer;
	void   ** ctx;
	void   ** main_conf;
	void   ** srv_conf;
	ngx_resolver_ctx_t   * resolver_ctx;
	ngx_mail_proxy_ctx_t * proxy;
	ngx_uint_t mail_state;
	unsigned protocol : 3;
	unsigned blocked : 1;
	unsigned quit : 1;
	unsigned quoted : 1;
	unsigned backslash : 1;
	unsigned no_sync_literal : 1;
	unsigned starttls : 1;
	unsigned esmtp : 1;
	unsigned auth_method : 3;
	unsigned auth_wait : 1;
	ngx_str_t login;
	ngx_str_t passwd;
	ngx_str_t salt;
	ngx_str_t tag;
	ngx_str_t tagged_line;
	ngx_str_t text;
	ngx_str_t    * addr_text;
	ngx_str_t host;
	ngx_str_t smtp_helo;
	ngx_str_t smtp_from;
	ngx_str_t smtp_to;
	ngx_str_t cmd;
	ngx_uint_t command;
	ngx_array_t args;
	ngx_uint_t login_attempt;
	/* used to parse POP3/IMAP/SMTP command */
	ngx_uint_t state;
	u_char * cmd_start;
	u_char * arg_start;
	u_char * arg_end;
	ngx_uint_t literal_len;
};

struct ngx_mail_log_ctx_t {
	ngx_str_t * client;
	ngx_mail_session_t   * session;
};

#define NGX_POP3_USER          1
#define NGX_POP3_PASS          2
#define NGX_POP3_CAPA          3
#define NGX_POP3_QUIT          4
#define NGX_POP3_NOOP          5
#define NGX_POP3_STLS          6
#define NGX_POP3_APOP          7
#define NGX_POP3_AUTH          8
#define NGX_POP3_STAT          9
#define NGX_POP3_LIST          10
#define NGX_POP3_RETR          11
#define NGX_POP3_DELE          12
#define NGX_POP3_RSET          13
#define NGX_POP3_TOP           14
#define NGX_POP3_UIDL          15

#define NGX_IMAP_LOGIN         1
#define NGX_IMAP_LOGOUT        2
#define NGX_IMAP_CAPABILITY    3
#define NGX_IMAP_NOOP          4
#define NGX_IMAP_STARTTLS      5

#define NGX_IMAP_NEXT          6

#define NGX_IMAP_AUTHENTICATE  7

#define NGX_SMTP_HELO          1
#define NGX_SMTP_EHLO          2
#define NGX_SMTP_AUTH          3
#define NGX_SMTP_QUIT          4
#define NGX_SMTP_NOOP          5
#define NGX_SMTP_MAIL          6
#define NGX_SMTP_RSET          7
#define NGX_SMTP_RCPT          8
#define NGX_SMTP_DATA          9
#define NGX_SMTP_VRFY          10
#define NGX_SMTP_EXPN          11
#define NGX_SMTP_HELP          12
#define NGX_SMTP_STARTTLS      13

#define NGX_MAIL_AUTH_PLAIN             0
#define NGX_MAIL_AUTH_LOGIN             1
#define NGX_MAIL_AUTH_LOGIN_USERNAME    2
#define NGX_MAIL_AUTH_APOP              3
#define NGX_MAIL_AUTH_CRAM_MD5          4
#define NGX_MAIL_AUTH_EXTERNAL          5
#define NGX_MAIL_AUTH_NONE              6

#define NGX_MAIL_AUTH_PLAIN_ENABLED     0x0002
#define NGX_MAIL_AUTH_LOGIN_ENABLED     0x0004
#define NGX_MAIL_AUTH_APOP_ENABLED      0x0008
#define NGX_MAIL_AUTH_CRAM_MD5_ENABLED  0x0010
#define NGX_MAIL_AUTH_EXTERNAL_ENABLED  0x0020
#define NGX_MAIL_AUTH_NONE_ENABLED      0x0040

#define NGX_MAIL_PARSE_INVALID_COMMAND  20

typedef void (*ngx_mail_init_session_pt)(ngx_mail_session_t * s, ngx_connection_t * c);
typedef void (*ngx_mail_init_protocol_pt)(ngx_event_t * rev);
typedef void (*ngx_mail_auth_state_pt)(ngx_event_t * rev);
typedef ngx_int_t (*ngx_mail_parse_command_pt)(ngx_mail_session_t * s);

struct ngx_mail_protocol_s {
	ngx_str_t name;
	in_port_t port[4];
	ngx_uint_t type;
	ngx_mail_init_session_pt init_session;
	ngx_mail_init_protocol_pt init_protocol;
	ngx_mail_parse_command_pt parse_command;
	ngx_mail_auth_state_pt auth_state;
	ngx_str_t internal_server_error;
	ngx_str_t cert_error;
	ngx_str_t no_cert;
};

struct ngx_mail_module_t {
	ngx_mail_protocol_t * protocol;
	void *(*create_main_conf)(ngx_conf_t *cf);
	char *(*init_main_conf)(ngx_conf_t *cf, void * conf);
	void *(*create_srv_conf)(ngx_conf_t *cf);
	char *(*merge_srv_conf)(ngx_conf_t *cf, void * prev, void * conf);
};

#define NGX_MAIL_MODULE         0x4C49414D     /* "MAIL" */
#define NGX_MAIL_MAIN_CONF      0x02000000
#define NGX_MAIL_SRV_CONF       0x04000000
#define NGX_MAIL_MAIN_CONF_OFFSET  offsetof(ngx_mail_conf_ctx_t, main_conf)
#define NGX_MAIL_SRV_CONF_OFFSET   offsetof(ngx_mail_conf_ctx_t, srv_conf)
#define ngx_mail_get_module_ctx(s, module)     (s)->ctx[module.ctx_index]
#define ngx_mail_set_ctx(s, c, module)         s->ctx[module.ctx_index] = c;
#define ngx_mail_delete_ctx(s, module)         s->ctx[module.ctx_index] = NULL;
#define ngx_mail_get_module_main_conf(s, module) (s)->main_conf[module.ctx_index]
#define ngx_mail_get_module_srv_conf(s, module)  (s)->srv_conf[module.ctx_index]
#define ngx_mail_conf_get_module_main_conf(cf, module) ((ngx_mail_conf_ctx_t*)cf->ctx)->main_conf[module.ctx_index]
#define ngx_mail_conf_get_module_srv_conf(cf, module)  ((ngx_mail_conf_ctx_t*)cf->ctx)->srv_conf[module.ctx_index]
#if (NGX_MAIL_SSL)
	void ngx_mail_starttls_handler(ngx_event_t * rev);
	ngx_int_t ngx_mail_starttls_only(ngx_mail_session_t * s, ngx_connection_t * c);
#endif
void ngx_mail_init_connection(ngx_connection_t * c);

ngx_int_t ngx_mail_salt(ngx_mail_session_t * s, ngx_connection_t * c, ngx_mail_core_srv_conf_t * cscf);
ngx_int_t ngx_mail_auth_plain(ngx_mail_session_t * s, ngx_connection_t * c, ngx_uint_t n);
ngx_int_t ngx_mail_auth_login_username(ngx_mail_session_t * s, ngx_connection_t * c, ngx_uint_t n);
ngx_int_t ngx_mail_auth_login_password(ngx_mail_session_t * s, ngx_connection_t * c);
ngx_int_t ngx_mail_auth_cram_md5_salt(ngx_mail_session_t * s, ngx_connection_t * c, char * prefix, size_t len);
ngx_int_t ngx_mail_auth_cram_md5(ngx_mail_session_t * s, ngx_connection_t * c);
ngx_int_t ngx_mail_auth_external(ngx_mail_session_t * s, ngx_connection_t * c, ngx_uint_t n);
ngx_int_t ngx_mail_auth_parse(ngx_mail_session_t * s, ngx_connection_t * c);

void ngx_mail_send(ngx_event_t * wev);
ngx_int_t ngx_mail_read_command(ngx_mail_session_t * s, ngx_connection_t * c);
void ngx_mail_auth(ngx_mail_session_t * s, ngx_connection_t * c);
void ngx_mail_close_connection(ngx_connection_t * c);
void ngx_mail_session_internal_server_error(ngx_mail_session_t * s);
u_char * ngx_mail_log_error(ngx_log_t * log, u_char * buf, size_t len);
const char * ngx_mail_capabilities(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

/* STUB */
void ngx_mail_proxy_init(ngx_mail_session_t * s, ngx_addr_t * peer);
void ngx_mail_auth_http_init(ngx_mail_session_t * s);
/**/

extern ngx_uint_t ngx_mail_max_module;
extern ngx_module_t ngx_mail_core_module;
//
//#include <ngx_mail_pop3_module.h>
//
struct ngx_mail_pop3_srv_conf_t {
	ngx_str_t capability;
	ngx_str_t starttls_capability;
	ngx_str_t starttls_only_capability;
	ngx_str_t auth_capability;
	ngx_uint_t auth_methods;
	ngx_array_t capabilities;
};

void ngx_mail_pop3_init_session(ngx_mail_session_t * s, ngx_connection_t * c);
void ngx_mail_pop3_init_protocol(ngx_event_t * rev);
void ngx_mail_pop3_auth_state(ngx_event_t * rev);
ngx_int_t ngx_mail_pop3_parse_command(ngx_mail_session_t * s);

extern ngx_module_t ngx_mail_pop3_module;
//
//#include <ngx_mail_imap_module.h>
//
struct ngx_mail_imap_srv_conf_t {
	size_t client_buffer_size;
	ngx_str_t capability;
	ngx_str_t starttls_capability;
	ngx_str_t starttls_only_capability;
	ngx_uint_t auth_methods;
	ngx_array_t capabilities;
};

void ngx_mail_imap_init_session(ngx_mail_session_t * s, ngx_connection_t * c);
void ngx_mail_imap_init_protocol(ngx_event_t * rev);
void ngx_mail_imap_auth_state(ngx_event_t * rev);
ngx_int_t ngx_mail_imap_parse_command(ngx_mail_session_t * s);

extern ngx_module_t ngx_mail_imap_module;
//
//#include <ngx_mail_smtp_module.h>
//
struct ngx_mail_smtp_srv_conf_t {
	ngx_msec_t greeting_delay;
	size_t client_buffer_size;
	ngx_str_t capability;
	ngx_str_t starttls_capability;
	ngx_str_t starttls_only_capability;
	ngx_str_t server_name;
	ngx_str_t greeting;
	ngx_uint_t auth_methods;
	ngx_array_t capabilities;
};

void ngx_mail_smtp_init_session(ngx_mail_session_t * s, ngx_connection_t * c);
void ngx_mail_smtp_init_protocol(ngx_event_t * rev);
void ngx_mail_smtp_auth_state(ngx_event_t * rev);
ngx_int_t ngx_mail_smtp_parse_command(ngx_mail_session_t * s);

extern ngx_module_t ngx_mail_smtp_module;
//
#endif /* _NGX_CORE_H_INCLUDED_ */
