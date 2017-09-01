/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#ifndef _NGX_CORE_H_INCLUDED_
#define _NGX_CORE_H_INCLUDED_

#include <ngx_config.h>

struct /*ngx_event_s*/ngx_event_t;

typedef struct ngx_module_s          ngx_module_t;
typedef struct ngx_conf_s            ngx_conf_t;
typedef struct ngx_cycle_s           ngx_cycle_t;
typedef struct ngx_pool_s            ngx_pool_t;
typedef struct ngx_chain_s           ngx_chain_t;
struct ngx_log_s;
typedef struct ngx_log_s             ngx_log_t;
typedef struct ngx_open_file_s       ngx_open_file_t;
typedef struct ngx_command_s         ngx_command_t;
typedef struct ngx_file_s            ngx_file_t;
//typedef struct ngx_event_s           ngx_event_t;
typedef struct ngx_event_aio_s       ngx_event_aio_t;
typedef struct ngx_connection_s      ngx_connection_t;
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

#define ngx_abs(value)       (((value) >= 0) ? (value) : -(value))

#include <ngx_errno.h>
#include <ngx_atomic.h>
#include <ngx_thread.h>
#include <ngx_rbtree.h>
#include <ngx_time.h>
#include <ngx_socket.h>
#include <ngx_string.h>
#include <ngx_files.h>
#include <ngx_shmem.h>
#include <ngx_process.h>
#include <ngx_dlopen.h>
#include <ngx_user.h>
#include <ngx_log.h>
#include <ngx_alloc.h>
//
//#include <ngx_parse.h>
//
ssize_t ngx_parse_size(ngx_str_t *line);
nginx_off_t ngx_parse_offset(ngx_str_t *line);
ngx_int_t ngx_parse_time(ngx_str_t *line, ngx_uint_t is_sec);
//
//#include <ngx_parse_time.h>
//
time_t ngx_parse_http_time(u_char *value, size_t len);
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

struct ngx_pool_s {
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

ngx_pool_t * ngx_create_pool(size_t size, ngx_log_t * log);
void   ngx_destroy_pool(ngx_pool_t * pool);
void   ngx_reset_pool(ngx_pool_t * pool);
void * ngx_palloc(ngx_pool_t * pool, size_t size);
void * ngx_pnalloc(ngx_pool_t * pool, size_t size);
void * ngx_pcalloc(ngx_pool_t * pool, size_t size);
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
	u_char   * pos;
	u_char   * last;
	nginx_off_t file_pos;
	nginx_off_t file_last;
	u_char   * start;    /* start of buffer */
	u_char   * end;      /* end of buffer */
	ngx_buf_tag_t tag;
	ngx_file_t * file;
	ngx_buf_t  * shadow;
	unsigned temporary : 1; // the buf's content could be changed 
	unsigned memory : 1; // the buf's content is in a memory cache or in a read only memory and must not be changed
	unsigned mmap : 1; // the buf's content is mmap()ed and must not be changed 
	unsigned recycled : 1;
	unsigned in_file : 1;
	unsigned flush : 1;
	unsigned sync : 1;
	unsigned last_buf : 1;
	unsigned last_in_chain : 1;
	unsigned last_shadow : 1;
	unsigned temp_file : 1;
	/* STUB */ int num;
};

struct ngx_chain_s {
	ngx_buf_t   * buf;
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

ngx_buf_t * ngx_create_temp_buf(ngx_pool_t * pool, size_t size);
ngx_chain_t * ngx_create_chain_of_bufs(ngx_pool_t * pool, ngx_bufs_t * bufs);

#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t * ngx_alloc_chain_link(ngx_pool_t * pool);
#define ngx_free_chain(pool, cl) cl->next = pool->chain; pool->chain = cl

ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t * ctx, ngx_chain_t * in);
ngx_int_t ngx_chain_writer(void * ctx, ngx_chain_t * in);
ngx_int_t ngx_chain_add_copy(ngx_pool_t * pool, ngx_chain_t ** chain, ngx_chain_t * in);
ngx_chain_t * ngx_chain_get_free_buf(ngx_pool_t * p, ngx_chain_t ** free);
void ngx_chain_update_chains(ngx_pool_t * p, ngx_chain_t ** free, ngx_chain_t ** busy, ngx_chain_t ** out, ngx_buf_tag_t tag);
nginx_off_t ngx_chain_coalesce_file(ngx_chain_t ** in, nginx_off_t limit);
ngx_chain_t * ngx_chain_update_sent(ngx_chain_t * in, nginx_off_t sent);
//
//#include <ngx_queue.h>
//
//typedef struct ngx_queue_s ngx_queue_t;
struct ngx_queue_t;

struct ngx_queue_t {
	ngx_queue_t  * prev;
	ngx_queue_t  * next;
};

#define ngx_queue_init(q)           (q)->prev = q; (q)->next = q
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
void * ngx_array_push(ngx_array_t * a);
void * ngx_array_push_n(ngx_array_t * a, ngx_uint_t n);
ngx_int_t ngx_array_init(ngx_array_t * array, ngx_pool_t * pool, ngx_uint_t n, size_t size);
//
//#include <ngx_list.h>
//
//typedef struct ngx_list_part_s ngx_list_part_t;

struct /*ngx_list_part_s*/ngx_list_part_t {
	void   * elts;
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
void * ngx_list_push(ngx_list_t * list);
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

typedef ngx_uint_t (*ngx_hash_key_pt)(u_char * data, size_t len);

struct ngx_hash_combined_t {
	ngx_hash_t hash;
	ngx_hash_wildcard_t * wc_head;
	ngx_hash_wildcard_t * wc_tail;
};

struct ngx_hash_init_t {
	ngx_hash_t  * hash;
	ngx_hash_key_pt key;
	ngx_uint_t max_size;
	ngx_uint_t bucket_size;
	char   * name;
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

void * ngx_hash_find(ngx_hash_t * hash, ngx_uint_t key, u_char * name, size_t len);
void * ngx_hash_find_wc_head(ngx_hash_wildcard_t * hwc, u_char * name, size_t len);
void * ngx_hash_find_wc_tail(ngx_hash_wildcard_t * hwc, u_char * name, size_t len);
void * ngx_hash_find_combined(ngx_hash_combined_t * hash, ngx_uint_t key, u_char * name, size_t len);
ngx_int_t ngx_hash_init(ngx_hash_init_t * hinit, ngx_hash_key_t * names, ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t * hinit, ngx_hash_key_t * names, ngx_uint_t nelts);
ngx_uint_t ngx_hash_key(u_char * data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char * data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char * dst, u_char * src, size_t n);
ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t * ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t * ha, ngx_str_t * key, void * value, ngx_uint_t flags);

#define ngx_hash(key, c) ((ngx_uint_t)key * 31 + c)
//
//#include <ngx_file.h>
//
struct ngx_file_s {
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
char * ngx_conf_set_path_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
char * ngx_conf_merge_path_value(ngx_conf_t * cf, ngx_path_t ** path, ngx_path_t * prev, ngx_path_init_t * init);
char * ngx_conf_set_access_slot(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);

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
	char    * start;
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
#include <ngx_times.h>
#include <ngx_rwlock.h>
#include <ngx_shmtx.h>
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
#include <ngx_cycle.h>
#include <ngx_resolver.h>
#if (NGX_OPENSSL)
	#include <ngx_event_openssl.h>
#endif
#include <ngx_process_cycle.h>
#include <ngx_conf_file.h>
#include <ngx_module.h>
#include <ngx_open_file_cache.h>
#include <ngx_os.h>
#include <ngx_connection.h>
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
#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
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

#endif /* _NGX_CORE_H_INCLUDED_ */
