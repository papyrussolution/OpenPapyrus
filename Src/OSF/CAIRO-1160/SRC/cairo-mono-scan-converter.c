/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/*
 * Copyright (c) 2011  Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 */
#include "cairoint.h"
#pragma hdrstop

/* @sobolev (replaced with Quorem_3232) struct quorem {
	int32 quo;
	int32 rem;
};*/

struct edge {
	struct edge * next, * prev;
	int32 height_left;
	int32 dir;
	int32 vertical;
	int32 dy;
	Quorem_3232 x;
	Quorem_3232 dxdy;
};

/* A collection of sorted and vertically clipped edges of the polygon.
 * Edges are moved from the polygon to an active list while scan
 * converting. */
struct polygon {
	/* The vertical clip extents. */
	int32 ymin, ymax;
	int num_edges;
	struct edge * edges;
	/* Array of edges all starting in the same bucket.	An edge is put
	 * into bucket EDGE_BUCKET_INDEX(edge->ytop, polygon->ymin) when
	 * it is added to the polygon. */
	struct edge ** y_buckets;
	struct edge * y_buckets_embedded[64];
	struct edge edges_embedded[32];
};

struct mono_scan_converter {
	struct polygon polygon[1];
	/* Leftmost edge on the current scan line. */
	struct edge head, tail;
	int is_vertical;
	cairo_half_open_span_t * spans;
	cairo_half_open_span_t spans_embedded[64];
	int num_spans;
	/* Clip box. */
	int32 xmin, xmax;
	int32 ymin, ymax;
};

#define I(x) _cairo_fixed_integer_round_down(x)
/* @sobolev (moved to cairoint.h)
//
// Compute the floored division a/b. Assumes / and % perform symmetric division. 
//
inline static Quorem_3232 floored_divrem(int a, int b)                            
{
	Quorem_3232 qr;
	qr.quo = a/b;
	qr.rem = a%b;
	if((a^b) < 0 && qr.rem) {
		qr.quo -= 1;
		qr.rem += b;
	}
	return qr;
}*/
//
// Compute the floored division (x*a)/b. Assumes / and % perform symmetric division. 
//
static Quorem_3232 floored_muldivrem(int x, int a, int b)                     
{
	Quorem_3232 qr;
	long long xa = (long long)x*a;
	qr.quo = static_cast<int32>(xa/b);
	qr.rem = xa%b;
	if((xa>=0) != (b>=0) && qr.rem) {
		qr.quo -= 1;
		qr.rem += b;
	}
	return qr;
}

static cairo_status_t polygon_init(struct polygon * polygon, int ymin, int ymax)
{
	uint   h = ymax - ymin + 1;
	polygon->y_buckets = polygon->y_buckets_embedded;
	if(h > ARRAY_LENGTH(polygon->y_buckets_embedded)) {
		polygon->y_buckets = static_cast<struct edge **>(_cairo_malloc_ab(h, sizeof(struct edge *)));
		if(UNLIKELY(NULL == polygon->y_buckets))
			return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	}
	memzero(polygon->y_buckets, h * sizeof(struct edge *));
	polygon->y_buckets[h-1] = (struct edge *)(void *)-1;
	polygon->ymin = ymin;
	polygon->ymax = ymax;
	return CAIRO_STATUS_SUCCESS;
}

static void polygon_fini(struct polygon * polygon)
{
	if(polygon->y_buckets != polygon->y_buckets_embedded)
		SAlloc::F(polygon->y_buckets);
	if(polygon->edges != polygon->edges_embedded)
		SAlloc::F(polygon->edges);
}

static void _polygon_insert_edge_into_its_y_bucket(struct polygon * polygon, struct edge * e, int y)
{
	struct edge ** ptail = &polygon->y_buckets[y - polygon->ymin];
	if(*ptail)
		(*ptail)->prev = e;
	e->next = *ptail;
	e->prev = NULL;
	*ptail = e;
}

inline static void polygon_add_edge(struct polygon * polygon,
    const cairo_edge_t * edge)
{
	struct edge * e;
	cairo_fixed_t dx;
	cairo_fixed_t dy;
	int y, ytop, ybot;
	int ymin = polygon->ymin;
	int ymax = polygon->ymax;

	y = I(edge->top);
	ytop = MAX(y, ymin);

	y = I(edge->bottom);
	ybot = MIN(y, ymax);

	if(ybot <= ytop)
		return;

	e = polygon->edges + polygon->num_edges++;
	e->height_left = ybot - ytop;
	e->dir = edge->dir;

	dx = edge->line.p2.x - edge->line.p1.x;
	dy = edge->line.p2.y - edge->line.p1.y;

	if(dx == 0) {
		e->vertical = TRUE;
		e->x.quo = edge->line.p1.x;
		e->x.rem = 0;
		e->dxdy.quo = 0;
		e->dxdy.rem = 0;
		e->dy = 0;
	}
	else {
		e->vertical = FALSE;
		e->dxdy = floored_muldivrem(dx, CAIRO_FIXED_ONE, dy);
		e->dy = dy;

		e->x = floored_muldivrem(ytop * CAIRO_FIXED_ONE + CAIRO_FIXED_FRAC_MASK/2 - edge->line.p1.y,
			dx, dy);
		e->x.quo += edge->line.p1.x;
	}
	e->x.rem -= dy;
	_polygon_insert_edge_into_its_y_bucket(polygon, e, ytop);
}

static struct edge * merge_sorted_edges(struct edge * head_a, struct edge * head_b)                      
{
	struct edge * head;
	int32 x;
	struct edge * prev = head_a->prev;
	struct edge ** next = &head;
	if(head_a->x.quo <= head_b->x.quo) {
		head = head_a;
	}
	else {
		head = head_b;
		head_b->prev = prev;
		goto start_with_b;
	}
	do {
		x = head_b->x.quo;
		while(head_a != NULL && head_a->x.quo <= x) {
			prev = head_a;
			next = &head_a->next;
			head_a = head_a->next;
		}
		head_b->prev = prev;
		*next = head_b;
		if(head_a == NULL)
			return head;
start_with_b:
		x = head_a->x.quo;
		while(head_b != NULL && head_b->x.quo <= x) {
			prev = head_b;
			next = &head_b->next;
			head_b = head_b->next;
		}

		head_a->prev = prev;
		*next = head_a;
		if(head_b == NULL)
			return head;
	} while(1);
}

static struct edge * sort_edges(struct edge * list, uint level, struct edge ** head_out)
{
	struct edge * remaining;
	uint i;
	struct edge * head_other = list->next;
	if(head_other == NULL) {
		*head_out = list;
		return NULL;
	}
	remaining = head_other->next;
	if(list->x.quo <= head_other->x.quo) {
		*head_out = list;
		head_other->next = NULL;
	}
	else {
		*head_out = head_other;
		head_other->prev = list->prev;
		head_other->next = list;
		list->prev = head_other;
		list->next = NULL;
	}
	for(i = 0; i < level && remaining; i++) {
		remaining = sort_edges(remaining, i, &head_other);
		*head_out = merge_sorted_edges(*head_out, head_other);
	}
	return remaining;
}

static struct edge * merge_unsorted_edges(struct edge * head, struct edge * unsorted)                      
{
	sort_edges(unsorted, UINT_MAX, &unsorted);
	return merge_sorted_edges(head, unsorted);
}

inline static void active_list_merge_edges(struct mono_scan_converter * c, struct edge * edges)
{
	struct edge * e;
	for(e = edges; c->is_vertical && e; e = e->next)
		c->is_vertical = e->vertical;
	c->head.next = merge_unsorted_edges(c->head.next, edges);
}

inline static void add_span(struct mono_scan_converter * c, int x1, int x2)
{
	int n;
	if(x1 < c->xmin)
		x1 = c->xmin;
	if(x2 > c->xmax)
		x2 = c->xmax;
	if(x2 <= x1)
		return;
	n = c->num_spans++;
	c->spans[n].x = x1;
	c->spans[n].coverage = 255;
	n = c->num_spans++;
	c->spans[n].x = x2;
	c->spans[n].coverage = 0;
}

inline static void row(struct mono_scan_converter * c, uint mask)
{
	struct edge * edge = c->head.next;
	int xstart = INT_MIN, prev_x = INT_MIN;
	int winding = 0;
	c->num_spans = 0;
	while(&c->tail != edge) {
		struct edge * next = edge->next;
		int xend = I(edge->x.quo);

		if(--edge->height_left) {
			if(!edge->vertical) {
				edge->x.quo += edge->dxdy.quo;
				edge->x.rem += edge->dxdy.rem;
				if(edge->x.rem >= 0) {
					++edge->x.quo;
					edge->x.rem -= edge->dy;
				}
			}

			if(edge->x.quo < prev_x) {
				struct edge * pos = edge->prev;
				pos->next = next;
				next->prev = pos;
				do {
					pos = pos->prev;
				} while(edge->x.quo < pos->x.quo);
				pos->next->prev = edge;
				edge->next = pos->next;
				edge->prev = pos;
				pos->next = edge;
			}
			else
				prev_x = edge->x.quo;
		}
		else {
			edge->prev->next = next;
			next->prev = edge->prev;
		}

		winding += edge->dir;
		if((winding & mask) == 0) {
			if(I(next->x.quo) > xend + 1) {
				add_span(c, xstart, xend);
				xstart = INT_MIN;
			}
		}
		else if(xstart == INT_MIN)
			xstart = xend;

		edge = next;
	}
}

inline static void dec(struct edge * e, int h)
{
	e->height_left -= h;
	if(e->height_left == 0) {
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
}

static cairo_status_t _mono_scan_converter_init(struct mono_scan_converter * c, int xmin, int ymin, int xmax, int ymax)
{
	int max_num_spans;
	cairo_status_t status = polygon_init(c->polygon, ymin, ymax);
	if(UNLIKELY(status))
		return status;
	max_num_spans = xmax - xmin + 1;
	if(max_num_spans > ARRAY_LENGTH(c->spans_embedded)) {
		c->spans = static_cast<cairo_half_open_span_t *>(_cairo_malloc_ab(max_num_spans, sizeof(cairo_half_open_span_t)));
		if(UNLIKELY(c->spans == NULL)) {
			polygon_fini(c->polygon);
			return _cairo_error(CAIRO_STATUS_NO_MEMORY);
		}
	}
	else
		c->spans = c->spans_embedded;
	c->xmin = xmin;
	c->xmax = xmax;
	c->ymin = ymin;
	c->ymax = ymax;
	c->head.vertical = 1;
	c->head.height_left = INT_MAX;
	c->head.x.quo = _cairo_fixed_from_int(_cairo_fixed_integer_part(INT_MIN));
	c->head.prev = NULL;
	c->head.next = &c->tail;
	c->tail.prev = &c->head;
	c->tail.next = NULL;
	c->tail.x.quo = _cairo_fixed_from_int(_cairo_fixed_integer_part(INT_MAX));
	c->tail.height_left = INT_MAX;
	c->tail.vertical = 1;

	c->is_vertical = 1;
	return CAIRO_STATUS_SUCCESS;
}

static void _mono_scan_converter_fini(struct mono_scan_converter * self)
{
	if(self->spans != self->spans_embedded)
		SAlloc::F(self->spans);
	polygon_fini(self->polygon);
}

static cairo_status_t mono_scan_converter_allocate_edges(struct mono_scan_converter * c, int num_edges)
{
	c->polygon->num_edges = 0;
	c->polygon->edges = c->polygon->edges_embedded;
	if(num_edges > ARRAY_LENGTH(c->polygon->edges_embedded)) {
		c->polygon->edges = static_cast<struct edge *>(_cairo_malloc_ab(num_edges, sizeof(struct edge)));
		if(UNLIKELY(c->polygon->edges == NULL))
			return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	}
	return CAIRO_STATUS_SUCCESS;
}

static void mono_scan_converter_add_edge(struct mono_scan_converter * c, const cairo_edge_t * edge)
{
	polygon_add_edge(c->polygon, edge);
}

static void step_edges(struct mono_scan_converter * c, int count)
{
	for(struct edge * edge = c->head.next; edge != &c->tail; edge = edge->next) {
		edge->height_left -= count;
		if(!edge->height_left) {
			edge->prev->next = edge->next;
			edge->next->prev = edge->prev;
		}
	}
}

static cairo_status_t mono_scan_converter_render(struct mono_scan_converter * c, uint winding_mask, cairo_span_renderer_t * renderer)
{
	struct polygon * polygon = c->polygon;
	int i, j, h = c->ymax - c->ymin;
	cairo_status_t status;
	for(i = 0; i < h; i = j) {
		j = i + 1;
		if(polygon->y_buckets[i])
			active_list_merge_edges(c, polygon->y_buckets[i]);

		if(c->is_vertical) {
			int min_height;
			struct edge * e;

			e = c->head.next;
			min_height = e->height_left;
			while(e != &c->tail) {
				if(e->height_left < min_height)
					min_height = e->height_left;
				e = e->next;
			}

			while(--min_height >= 1 && polygon->y_buckets[j] == NULL)
				j++;
			if(j != i + 1)
				step_edges(c, j - (i + 1));
		}

		row(c, winding_mask);
		if(c->num_spans) {
			status = renderer->render_rows(renderer, c->ymin+i, j-i,
				c->spans, c->num_spans);
			if(UNLIKELY(status))
				return status;
		}

		/* XXX recompute after dropping edges? */
		if(c->head.next == &c->tail)
			c->is_vertical = 1;
	}

	return CAIRO_STATUS_SUCCESS;
}

struct _cairo_mono_scan_converter {
	cairo_scan_converter_t base;
	struct mono_scan_converter converter[1];
	cairo_fill_rule_t fill_rule;
};

typedef struct _cairo_mono_scan_converter cairo_mono_scan_converter_t;

static void _cairo_mono_scan_converter_destroy(void * converter)
{
	cairo_mono_scan_converter_t * self = (cairo_mono_scan_converter_t *)converter;
	_mono_scan_converter_fini(self->converter);
	SAlloc::F(self);
}

cairo_status_t _cairo_mono_scan_converter_add_polygon(void * converter, const cairo_polygon_t * polygon)
{
	cairo_mono_scan_converter_t * self = (cairo_mono_scan_converter_t *)converter;
	cairo_status_t status;
	int i;
#if 0
	FILE * file = fopen("polygon.txt", "w");
	_cairo_debug_print_polygon(file, polygon);
	fclose(file);
#endif
	status = mono_scan_converter_allocate_edges(self->converter, polygon->num_edges);
	if(UNLIKELY(status))
		return status;
	for(i = 0; i < polygon->num_edges; i++)
		mono_scan_converter_add_edge(self->converter, &polygon->edges[i]);
	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t _cairo_mono_scan_converter_generate(void * converter, cairo_span_renderer_t * renderer)
{
	cairo_mono_scan_converter_t * self = (cairo_mono_scan_converter_t *)converter;
	return mono_scan_converter_render(self->converter, self->fill_rule == CAIRO_FILL_RULE_WINDING ? ~0 : 1, renderer);
}

cairo_scan_converter_t * _cairo_mono_scan_converter_create(int xmin, int ymin, int xmax, int ymax, cairo_fill_rule_t fill_rule)
{
	cairo_status_t status;
	cairo_mono_scan_converter_t * self = (cairo_mono_scan_converter_t *)_cairo_malloc(sizeof(struct _cairo_mono_scan_converter));
	if(UNLIKELY(self == NULL)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto bail_nomem;
	}
	self->base.destroy = _cairo_mono_scan_converter_destroy;
	self->base.generate = _cairo_mono_scan_converter_generate;
	status = _mono_scan_converter_init(self->converter, xmin, ymin, xmax, ymax);
	if(UNLIKELY(status))
		goto bail;
	self->fill_rule = fill_rule;
	return &self->base;
bail:
	self->base.destroy(&self->base);
bail_nomem:
	return _cairo_scan_converter_create_in_error(status);
}
