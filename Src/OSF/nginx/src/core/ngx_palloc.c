/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

ngx_pool_t * FASTCALL ngx_create_pool(size_t size, ngx_log_t * log)
{
	ngx_pool_t * p = (ngx_pool_t *)ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
	if(p) {
		p->d.last = (u_char*)p + sizeof(ngx_pool_t);
		p->d.end = (u_char*)p + size;
		p->d.next = NULL;
		p->d.failed = 0;
		size = size - sizeof(ngx_pool_t);
		p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;
		p->current = p;
		p->chain = NULL;
		p->large = NULL;
		p->cleanup = NULL;
		p->log = log;
	}
	return p;
}

void FASTCALL ngx_destroy_pool(ngx_pool_t * pool)
{
	ngx_pool_t * p, * n;
	ngx_pool_large_t * l;
	ngx_pool_cleanup_t * c;
	for(c = pool->cleanup; c; c = c->next) {
		if(c->handler) {
			ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "run cleanup: %p", c);
			c->handler(c->data);
		}
	}
#if (NGX_DEBUG)
	// 
	// we could allocate the pool->log from this pool
	// so we cannot use this log while free()ing the pool
	// 
	for(l = pool->large; l; l = l->next) {
		ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
	}
	for(p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
		ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p, unused: %uz", p, p->d.end - p->d.last);
		if(!n)
			break;
	}
#endif
	for(l = pool->large; l; l = l->next) {
		if(l->alloc) {
			ngx_free(l->alloc);
		}
	}
	for(p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
		ngx_free(p);
		if(!n)
			break;
	}
}

void FASTCALL ngx_reset_pool(ngx_pool_t * pool)
{
	for(ngx_pool_large_t * l = pool->large; l; l = l->next) {
		if(l->alloc) {
			ngx_free(l->alloc);
		}
	}
	for(ngx_pool_t * p = pool; p; p = p->d.next) {
		p->d.last = (u_char*)p + sizeof(ngx_pool_t);
		p->d.failed = 0;
	}
	pool->current = pool;
	pool->chain = NULL;
	pool->large = NULL;
}

static void * FASTCALL ngx_palloc_block(ngx_pool_t * pool, size_t size)
{
	size_t psize = (size_t)(pool->d.end - (u_char*)pool);
	u_char * m = (u_char *)ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
	if(m) {
		ngx_pool_t * p_new = (ngx_pool_t*)m;
		p_new->d.end = m + psize;
		p_new->d.next = NULL;
		p_new->d.failed = 0;
		m += sizeof(ngx_pool_data_t);
		m = ngx_align_ptr(m, NGX_ALIGNMENT);
		p_new->d.last = m + size;
		{
			ngx_pool_t * p;
			for(p = pool->current; p->d.next; p = p->d.next) {
				if(p->d.failed++ > 4) {
					pool->current = p->d.next;
				}
			}
			p->d.next = p_new;
		}
	}
	return m;
}

static void * FASTCALL ngx_palloc_small(ngx_pool_t * pool, size_t size)
{
	ngx_pool_t * p = pool->current;
	do {
		u_char * m = p->d.last;
		if((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;
			return m;
		}
		p = p->d.next;
	} while(p);
	return ngx_palloc_block(pool, size);
}

static void * FASTCALL ngx_palloc_small_align(ngx_pool_t * pool, size_t size)
{
	ngx_pool_t * p = pool->current;
	do {
		u_char * m = p->d.last;
		m = ngx_align_ptr(m, NGX_ALIGNMENT);
		if((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;
			return m;
		}
		p = p->d.next;
	} while(p);
	return ngx_palloc_block(pool, size);
}

static void * FASTCALL ngx_palloc_large(ngx_pool_t * pool, size_t size)
{
	void * p = ngx_alloc(size, pool->log);
	if(p) {
		ngx_uint_t n = 0;
		ngx_pool_large_t * large;
		for(large = pool->large; large; large = large->next) {
			if(large->alloc == NULL) {
				large->alloc = p;
				return p;
			}
			if(n++ > 3) {
				break;
			}
		}
		large = (ngx_pool_large_t *)ngx_palloc_small_align(pool, sizeof(ngx_pool_large_t));
		if(large == NULL) {
			ngx_free(p);
			return NULL;
		}
		large->alloc = p;
		large->next = pool->large;
		pool->large = large;
	}
	return p;
}

void * FASTCALL ngx_palloc(ngx_pool_t * pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
	if(size <= pool->max) {
		return ngx_palloc_small_align(pool, size);
	}
#endif
	return ngx_palloc_large(pool, size);
}

void * FASTCALL ngx_pnalloc(ngx_pool_t * pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
	if(size <= pool->max) {
		return ngx_palloc_small(pool, size);
	}
#endif
	return ngx_palloc_large(pool, size);
}

void * ngx_pmemalign(ngx_pool_t * pool, size_t size, size_t alignment)
{
	void * p = ngx_memalign(alignment, size, pool->log);
	if(p) {
		ngx_pool_large_t * large = (ngx_pool_large_t *)ngx_palloc_small_align(pool, sizeof(ngx_pool_large_t));
		if(large == NULL) {
			ngx_free(p);
			return NULL;
		}
		large->alloc = p;
		large->next = pool->large;
		pool->large = large;
	}
	return p;
}

ngx_int_t ngx_pfree(ngx_pool_t * pool, void * p)
{
	for(ngx_pool_large_t * p_blk = pool->large; p_blk; p_blk = p_blk->next) {
		if(p == p_blk->alloc) {
			ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", p_blk->alloc);
			ngx_free(p_blk->alloc);
			p_blk->alloc = NULL;
			return NGX_OK;
		}
	}
	return NGX_DECLINED;
}

void * FASTCALL ngx_pcalloc(ngx_pool_t * pool, size_t size)
{
	void * p = ngx_palloc(pool, size);
	if(p)
		memzero(p, size);
	return p;
}

ngx_pool_cleanup_t * ngx_pool_cleanup_add(ngx_pool_t * p, size_t size)
{
	ngx_pool_cleanup_t * c = (ngx_pool_cleanup_t *)ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
	if(c) {
		if(size) {
			c->data = ngx_palloc(p, size);
			if(c->data == NULL) {
				return NULL;
			}
		}
		else {
			c->data = NULL;
		}
		c->handler = NULL;
		c->next = p->cleanup;
		p->cleanup = c;
		ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);
	}
	return c;
}

void ngx_pool_run_cleanup_file(ngx_pool_t * p, ngx_fd_t fd)
{
	for(ngx_pool_cleanup_t * c = p->cleanup; c; c = c->next) {
		if(c->handler == ngx_pool_cleanup_file) {
			ngx_pool_cleanup_file_t * cf = (ngx_pool_cleanup_file_t *)c->data;
			if(cf->fd == fd) {
				c->handler(cf);
				c->handler = NULL;
				return;
			}
		}
	}
}

void ngx_pool_cleanup_file(void * data)
{
	ngx_pool_cleanup_file_t * c = (ngx_pool_cleanup_file_t *)data;
	ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d", c->fd);
	if(ngx_close_file(c->fd) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno, ngx_close_file_n " \"%s\" failed", c->name);
	}
}

void ngx_pool_delete_file(void * data)
{
	ngx_pool_cleanup_file_t  * c = (ngx_pool_cleanup_file_t *)data;
	ngx_err_t err;
	ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s", c->fd, c->name);
	if(ngx_delete_file(c->name) == NGX_FILE_ERROR) {
		err = ngx_errno;
		if(err != NGX_ENOENT) {
			ngx_log_error(NGX_LOG_CRIT, c->log, err, ngx_delete_file_n " \"%s\" failed", c->name);
		}
	}

	if(ngx_close_file(c->fd) == NGX_FILE_ERROR) {
		ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno, ngx_close_file_n " \"%s\" failed", c->name);
	}
}

#if 0

static void * ngx_get_cached_block(size_t size)
{
	void * p;
	ngx_cached_block_slot_t  * slot;
	if(ngx_cycle->cache == NULL) {
		return NULL;
	}
	slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];
	slot->tries++;
	if(slot->number) {
		p = slot->block;
		slot->block = slot->block->next;
		slot->number--;
		return p;
	}
	return NULL;
}

#endif
