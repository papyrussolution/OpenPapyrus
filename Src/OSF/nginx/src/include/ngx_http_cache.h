/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#ifndef _NGX_HTTP_CACHE_H_INCLUDED_
#define _NGX_HTTP_CACHE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

#define NGX_HTTP_CACHE_MISS          1
#define NGX_HTTP_CACHE_BYPASS        2
#define NGX_HTTP_CACHE_EXPIRED       3
#define NGX_HTTP_CACHE_STALE         4
#define NGX_HTTP_CACHE_UPDATING      5
#define NGX_HTTP_CACHE_REVALIDATED   6
#define NGX_HTTP_CACHE_HIT           7
#define NGX_HTTP_CACHE_SCARCE        8

#define NGX_HTTP_CACHE_KEY_LEN       16
#define NGX_HTTP_CACHE_ETAG_LEN      128
#define NGX_HTTP_CACHE_VARY_LEN      128

#define NGX_HTTP_CACHE_VERSION       5

struct ngx_http_cache_valid_t {
	ngx_uint_t status;
	time_t valid;
};

struct ngx_http_file_cache_node_t {
	ngx_rbtree_node_t node;
	ngx_queue_t queue;
	uchar key[NGX_HTTP_CACHE_KEY_LEN - sizeof(ngx_rbtree_key_t)];
	unsigned count : 20;
	unsigned uses : 10;
	unsigned valid_msec : 10;
	unsigned error : 10;
	unsigned exists : 1;
	unsigned updating : 1;
	unsigned deleting : 1;
	unsigned purged : 1;
	/* 10 unused bits */
	ngx_file_uniq_t uniq;
	time_t expire;
	time_t valid_sec;
	size_t body_start;
	nginx_off_t fs_size;
	ngx_msec_t lock_time;
};

struct ngx_http_cache_s {
	ngx_file_t file;
	ngx_array_t keys;
	uint32_t crc32;
	uchar key[NGX_HTTP_CACHE_KEY_LEN];
	uchar main[NGX_HTTP_CACHE_KEY_LEN];
	ngx_file_uniq_t uniq;
	time_t valid_sec;
	time_t updating_sec;
	time_t error_sec;
	time_t last_modified;
	time_t date;
	ngx_str_t etag;
	ngx_str_t vary;
	uchar variant[NGX_HTTP_CACHE_KEY_LEN];
	size_t header_start;
	size_t body_start;
	nginx_off_t length;
	nginx_off_t fs_size;
	ngx_uint_t min_uses;
	ngx_uint_t error;
	ngx_uint_t valid_msec;
	ngx_uint_t vary_tag;
	ngx_buf_t   * buf;
	ngx_http_file_cache_t * file_cache;
	ngx_http_file_cache_node_t * node;
#if (NGX_THREADS || NGX_COMPAT)
	ngx_thread_task_t * thread_task;
#endif
	ngx_msec_t lock_timeout;
	ngx_msec_t lock_age;
	ngx_msec_t lock_time;
	ngx_msec_t wait_time;
	ngx_event_t wait_event;
	unsigned lock : 1;
	unsigned waiting : 1;
	unsigned updated : 1;
	unsigned updating : 1;
	unsigned exists : 1;
	unsigned temp_file : 1;
	unsigned purged : 1;
	unsigned reading : 1;
	unsigned secondary : 1;
	unsigned background : 1;
	unsigned stale_updating : 1;
	unsigned stale_error : 1;
};

struct ngx_http_file_cache_header_t {
	ngx_uint_t version;
	time_t valid_sec;
	time_t updating_sec;
	time_t error_sec;
	time_t last_modified;
	time_t date;
	uint32_t crc32;
	ushort valid_msec;
	ushort header_start;
	ushort body_start;
	uchar etag_len;
	uchar etag[NGX_HTTP_CACHE_ETAG_LEN];
	uchar vary_len;
	uchar vary[NGX_HTTP_CACHE_VARY_LEN];
	uchar variant[NGX_HTTP_CACHE_KEY_LEN];
};

struct ngx_http_file_cache_sh_t {
	ngx_rbtree_t rbtree;
	ngx_rbtree_node_t sentinel;
	ngx_queue_t queue;
	ngx_atomic_t cold;
	ngx_atomic_t loading;
	nginx_off_t size;
	ngx_uint_t count;
	ngx_uint_t watermark;
};

struct ngx_http_file_cache_s {
	ngx_http_file_cache_sh_t * sh;
	ngx_slab_pool_t * shpool;
	ngx_path_t  * path;
	nginx_off_t max_size;
	size_t bsize;
	time_t inactive;
	time_t fail_time;
	ngx_uint_t files;
	ngx_uint_t loader_files;
	ngx_msec_t last;
	ngx_msec_t loader_sleep;
	ngx_msec_t loader_threshold;
	ngx_uint_t manager_files;
	ngx_msec_t manager_sleep;
	ngx_msec_t manager_threshold;
	ngx_shm_zone_t  * shm_zone;
	ngx_uint_t use_temp_path;
	/* unsigned use_temp_path:1 */
};

ngx_int_t ngx_http_file_cache_new(ngx_http_request_t * r);
ngx_int_t ngx_http_file_cache_create(ngx_http_request_t * r);
void ngx_http_file_cache_create_key(ngx_http_request_t * r);
ngx_int_t ngx_http_file_cache_open(ngx_http_request_t * r);
ngx_int_t ngx_http_file_cache_set_header(ngx_http_request_t * r, uchar * buf);
void ngx_http_file_cache_update(ngx_http_request_t * r, ngx_temp_file_t * tf);
void ngx_http_file_cache_update_header(ngx_http_request_t * r);
ngx_int_t ngx_http_cache_send(ngx_http_request_t *);
void ngx_http_file_cache_free(ngx_http_cache_t * c, ngx_temp_file_t * tf);
time_t ngx_http_file_cache_valid(ngx_array_t * cache_valid, ngx_uint_t status); 
const char * ngx_http_file_cache_set_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler
const char * ngx_http_file_cache_valid_set_slot(ngx_conf_t * cf, const ngx_command_t * cmd, void * conf); // F_SetHandler

extern ngx_str_t ngx_http_cache_status[];

#endif /* _NGX_HTTP_CACHE_H_INCLUDED_ */
