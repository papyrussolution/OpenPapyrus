/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

ngx_int_t ngx_list_init(ngx_list_t * list, ngx_pool_t * pool, ngx_uint_t n, size_t size)
{
	list->part.elts = ngx_palloc(pool, n * size);
	if(list->part.elts == NULL) {
		return NGX_ERROR;
	}
	else {
		list->part.nelts = 0;
		list->part.next = NULL;
		list->last = &list->part;
		list->size = size;
		list->nalloc = n;
		list->pool = pool;
		return NGX_OK;
	}
}

ngx_list_t * ngx_list_create(ngx_pool_t * pool, ngx_uint_t n, size_t size)
{
	ngx_list_t  * list = (ngx_list_t *)ngx_palloc(pool, sizeof(ngx_list_t));
	if(list == NULL) {
		return NULL;
	}
	if(ngx_list_init(list, pool, n, size) != NGX_OK) {
		return NULL;
	}
	return list;
}

void * ngx_list_push(ngx_list_t * l)
{
	void * elt;
	ngx_list_part_t * last = l->last;
	if(last->nelts == l->nalloc) {
		/* the last part is full, allocate a new list part */
		last = (ngx_list_part_t *)ngx_palloc(l->pool, sizeof(ngx_list_part_t));
		if(last == NULL) {
			return NULL;
		}
		last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
		if(last->elts == NULL) {
			return NULL;
		}
		last->nelts = 0;
		last->next = NULL;
		l->last->next = last;
		l->last = last;
	}
	elt = (char*)last->elts + l->size * last->nelts;
	last->nelts++;
	return elt;
}

