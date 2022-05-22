/*
 * Copyright © 2010 Joonas Pihlaja
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
 */
#ifndef CAIRO_FREELIST_TYPE_H
#define CAIRO_FREELIST_TYPE_H

typedef struct _cairo_freelist_node cairo_freelist_node_t;
struct _cairo_freelist_node {
	cairo_freelist_node_t * next;
};

typedef struct _cairo_freelist {
	cairo_freelist_node_t * first_free_node;
	unsigned nodesize;
} cairo_freelist_t;

typedef struct _cairo_freelist_pool cairo_freelist_pool_t;
struct _cairo_freelist_pool {
	cairo_freelist_pool_t * next;
	unsigned size, rem;
	uint8 * data;
};

typedef struct _cairo_freepool {
	cairo_freelist_node_t * first_free_node;
	cairo_freelist_pool_t * pools;
	cairo_freelist_pool_t * freepools;
	unsigned nodesize;
	cairo_freelist_pool_t embedded_pool;
	uint8 embedded_data[1000];
} cairo_freepool_t;

#endif /* CAIRO_FREELIST_TYPE_H */
