/*
 * Copyright © 2006 Joonas Pihlaja
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "cairoint.h"
#pragma hdrstop
//#include "cairo-freelist-private.h"

void _cairo_freelist_init(cairo_freelist_t * freelist, unsigned nodesize)
{
	memzero(freelist, sizeof(cairo_freelist_t));
	freelist->nodesize = nodesize;
}

void _cairo_freelist_fini(cairo_freelist_t * freelist)
{
	cairo_freelist_node_t * node = freelist->first_free_node;
	while(node) {
		VG(VALGRIND_MAKE_MEM_DEFINED(node, sizeof(node->next)));
		cairo_freelist_node_t * next = node->next;
		SAlloc::F(node);
		node = next;
	}
}

void FASTCALL _cairo_freepool_reset(cairo_freepool_t * freepool)
{
	while(freepool->pools != &freepool->embedded_pool) {
		cairo_freelist_pool_t * pool = freepool->pools;
		freepool->pools = pool->next;
		pool->next = freepool->freepools;
		freepool->freepools = pool;
	}
	freepool->embedded_pool.rem = sizeof(freepool->embedded_data);
	freepool->embedded_pool.data = freepool->embedded_data;
}

void * FASTCALL _cairo_freepool_alloc_from_pool(cairo_freepool_t * freepool)
{
	cairo_freelist_pool_t * pool = freepool->pools;
	if(unlikely(freepool->nodesize > pool->rem))
		return _cairo_freepool_alloc_from_new_pool(freepool);
	else {
		uint8 * ptr = pool->data;
		pool->data += freepool->nodesize;
		pool->rem -= freepool->nodesize;
		VG(VALGRIND_MAKE_MEM_UNDEFINED(ptr, freepool->nodesize));
		return ptr;
	}
}

void * FASTCALL _cairo_freepool_alloc(cairo_freepool_t * freepool)
{
	cairo_freelist_node_t * node = freepool->first_free_node;
	if(!node)
		return _cairo_freepool_alloc_from_pool(freepool);
	else {
		VG(VALGRIND_MAKE_MEM_DEFINED(node, sizeof(node->next)));
		freepool->first_free_node = node->next;
		VG(VALGRIND_MAKE_MEM_UNDEFINED(node, freepool->nodesize));
		return node;
	}
}

void FASTCALL _cairo_freepool_free(cairo_freepool_t * freepool, void * ptr)
{
	cairo_freelist_node_t * node = (cairo_freelist_node_t *)ptr;
	node->next = freepool->first_free_node;
	freepool->first_free_node = node;
	VG(VALGRIND_MAKE_MEM_NOACCESS(node, freepool->nodesize));
}

void * _cairo_freelist_alloc(cairo_freelist_t * freelist)
{
	if(freelist->first_free_node) {
		cairo_freelist_node_t * node = freelist->first_free_node;
		VG(VALGRIND_MAKE_MEM_DEFINED(node, sizeof(node->next)));
		freelist->first_free_node = node->next;
		VG(VALGRIND_MAKE_MEM_UNDEFINED(node, freelist->nodesize));
		return node;
	}
	return SAlloc::M(freelist->nodesize);
}

void * _cairo_freelist_calloc(cairo_freelist_t * freelist)
{
	void * node = _cairo_freelist_alloc(freelist);
	return memzero(node, freelist->nodesize);
}

void _cairo_freelist_free(cairo_freelist_t * freelist, void * voidnode)
{
	cairo_freelist_node_t * node = (cairo_freelist_node_t *)voidnode;
	if(node) {
		node->next = freelist->first_free_node;
		freelist->first_free_node = node;
		VG(VALGRIND_MAKE_MEM_NOACCESS(node, freelist->nodesize));
	}
}

void FASTCALL _cairo_freepool_init(cairo_freepool_t * freepool, unsigned nodesize)
{
	freepool->first_free_node = NULL;
	freepool->pools = &freepool->embedded_pool;
	freepool->freepools = NULL;
	freepool->nodesize = nodesize;
	freepool->embedded_pool.next = NULL;
	freepool->embedded_pool.size = sizeof(freepool->embedded_data);
	freepool->embedded_pool.rem = sizeof(freepool->embedded_data);
	freepool->embedded_pool.data = freepool->embedded_data;
	VG(VALGRIND_MAKE_MEM_NOACCESS(freepool->embedded_data, sizeof(freepool->embedded_data)));
}

void FASTCALL _cairo_freepool_fini(cairo_freepool_t * freepool)
{
	cairo_freelist_pool_t * pool = freepool->pools;
	while(pool != &freepool->embedded_pool) {
		cairo_freelist_pool_t * next = pool->next;
		SAlloc::F(pool);
		pool = next;
	}
	pool = freepool->freepools;
	while(pool != NULL) {
		cairo_freelist_pool_t * next = pool->next;
		SAlloc::F(pool);
		pool = next;
	}
	VG(VALGRIND_MAKE_MEM_NOACCESS(freepool, sizeof(freepool)));
}

void * _cairo_freepool_alloc_from_new_pool(cairo_freepool_t * freepool)
{
	cairo_freelist_pool_t * pool;
	int poolsize;
	if(freepool->freepools != NULL) {
		pool = freepool->freepools;
		freepool->freepools = pool->next;
		poolsize = pool->size;
	}
	else {
		if(freepool->pools != &freepool->embedded_pool)
			poolsize = 2 * freepool->pools->size;
		else
			poolsize = (128 * freepool->nodesize + 8191) & -8192;
		pool = (cairo_freelist_pool_t *)SAlloc::M(sizeof(cairo_freelist_pool_t) + poolsize);
		if(unlikely(pool == NULL))
			return pool;
		pool->size = poolsize;
	}
	pool->next = freepool->pools;
	freepool->pools = pool;
	pool->rem = poolsize - freepool->nodesize;
	pool->data = (uint8*)(pool + 1) + freepool->nodesize;
	VG(VALGRIND_MAKE_MEM_NOACCESS(pool->data, pool->rem));
	return pool + 1;
}

cairo_status_t _cairo_freepool_alloc_array(cairo_freepool_t * freepool, int count, void ** array)
{
	int i;
	for(i = 0; i < count; i++) {
		cairo_freelist_node_t * node = freepool->first_free_node;
		if(likely(node != NULL)) {
			VG(VALGRIND_MAKE_MEM_DEFINED(node, sizeof(node->next)));
			freepool->first_free_node = node->next;
			VG(VALGRIND_MAKE_MEM_UNDEFINED(node, freepool->nodesize));
		}
		else {
			node = (cairo_freelist_node_t *)_cairo_freepool_alloc_from_pool(freepool);
			if(unlikely(node == NULL))
				goto CLEANUP;
		}
		array[i] = node;
	}
	return CAIRO_STATUS_SUCCESS;
CLEANUP:
	while(i--)
		_cairo_freepool_free(freepool, array[i]);
	return _cairo_error(CAIRO_STATUS_NO_MEMORY);
}

