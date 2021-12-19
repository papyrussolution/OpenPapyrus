/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

ngx_uint_t ngx_pagesize; // @global
ngx_uint_t ngx_pagesize_shift; // @global
ngx_uint_t ngx_cacheline_size; // @global

void * ngx_alloc(size_t size, ngx_log_t * log)
{
	void * p = SAlloc::M(size); // malloc(size);
	if(!p) {
		ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "malloc(%uz) failed", size);
	}
	ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);
	return p;
}

void * ngx_calloc(size_t size, ngx_log_t * log)
{
	void * p = ngx_alloc(size, log);
	if(p) {
		memzero(p, size);
	}
	return p;
}

