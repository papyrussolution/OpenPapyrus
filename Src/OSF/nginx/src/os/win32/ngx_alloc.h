/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#ifndef _NGX_ALLOC_H_INCLUDED_
#define _NGX_ALLOC_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

void * ngx_alloc(size_t size, ngx_log_t *log);
void * ngx_calloc(size_t size, ngx_log_t *log);

// @v11.9.1 #define ngx_free_Removed /*free*/SAlloc::F
#define ngx_memalign(alignment, size, log)  ngx_alloc(size, log)

extern ngx_uint_t  ngx_pagesize; // @global
extern ngx_uint_t  ngx_pagesize_shift; // @global
extern ngx_uint_t  ngx_cacheline_size; // @global

#endif /* _NGX_ALLOC_H_INCLUDED_ */
