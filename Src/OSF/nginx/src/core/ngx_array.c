
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

ngx_array_t * ngx_array_create(ngx_pool_t * p, ngx_uint_t n, size_t size)
{
	ngx_array_t * a = (ngx_array_t *)ngx_palloc(p, sizeof(ngx_array_t));
	if(a == NULL)
		return NULL;
	else if(ngx_array_init(a, p, n, size) != NGX_OK)
		return NULL;
	else 
		return a;
}

void ngx_array_destroy(ngx_array_t * a)
{
	ngx_pool_t  * p = a->pool;
	if((u_char*)a->elts + a->size * a->nalloc == p->d.last) {
		p->d.last -= a->size * a->nalloc;
	}
	if((u_char*)a + sizeof(ngx_array_t) == p->d.last) {
		p->d.last = (u_char*)a;
	}
}

ngx_int_t ngx_array_init(ngx_array_t * array, ngx_pool_t * pool, ngx_uint_t n, size_t size)
{
	/*
	 * set "array->nelts" before "array->elts", otherwise MSVC thinks
	 * that "array->nelts" may be used without having been initialized
	 */
	array->nelts = 0;
	array->size = size;
	array->nalloc = n;
	array->pool = pool;
	array->elts = ngx_palloc(pool, n * size);
	return (array->elts == NULL) ? NGX_ERROR : NGX_OK;
}

void * ngx_array_push(ngx_array_t * a)
{
	void * elt, * p_new;
	size_t size;
	ngx_pool_t  * p;
	if(a->nelts == a->nalloc) {
		/* the array is full */
		size = a->size * a->nalloc;
		p = a->pool;
		if((u_char*)a->elts + size == p->d.last && p->d.last + a->size <= p->d.end) {
			/*
			 * the array allocation is the last in the pool
			 * and there is space for new allocation
			 */
			p->d.last += a->size;
			a->nalloc++;
		}
		else {
			/* allocate a new array */
			p_new = ngx_palloc(p, 2 * size);
			if(p_new == NULL) {
				return NULL;
			}
			ngx_memcpy(p_new, a->elts, size);
			a->elts = p_new;
			a->nalloc *= 2;
		}
	}
	elt = (u_char*)a->elts + a->size * a->nelts;
	a->nelts++;
	return elt;
}

void * ngx_array_push_n(ngx_array_t * a, ngx_uint_t n)
{
	void * elt, * p_new;
	ngx_uint_t nalloc;
	ngx_pool_t  * p;
	size_t size = n * a->size;
	if(a->nelts + n > a->nalloc) {
		/* the array is full */
		p = a->pool;
		if((u_char*)a->elts + a->size * a->nalloc == p->d.last && p->d.last + size <= p->d.end) {
			/*
			 * the array allocation is the last in the pool
			 * and there is space for new allocation
			 */

			p->d.last += size;
			a->nalloc += n;
		}
		else {
			/* allocate a new array */
			nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);
			p_new = ngx_palloc(p, nalloc * a->size);
			if(p_new == NULL) {
				return NULL;
			}
			ngx_memcpy(p_new, a->elts, a->nelts * a->size);
			a->elts = p_new;
			a->nalloc = nalloc;
		}
	}
	elt = (u_char*)a->elts + a->size * a->nelts;
	a->nelts += n;
	return elt;
}

