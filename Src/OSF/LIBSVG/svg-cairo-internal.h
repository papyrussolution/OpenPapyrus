/* libsvg-cairo - Render SVG documents using the cairo library
 *
 * Copyright © 2002 University of Southern California
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Carl D. Worth <cworth@isi.edu>
 */

#ifndef SVG_CAIRO_INTERNAL_H
#define SVG_CAIRO_INTERNAL_H

#include "svg-cairo.h"
#include "svg-cairo-version.h"
#include <stdarg.h>
#include <slib.h>

/* XXX: What should this actually be? */
#define SVG_CAIRO_FONT_FAMILY_DEFAULT "verdana"

typedef enum svg_cairo_render_type {
	SVG_CAIRO_RENDER_TYPE_FILL,
	SVG_CAIRO_RENDER_TYPE_STROKE
} svg_cairo_render_type_t;

typedef struct svg_cairo_state {
	cairo_surface_t * child_surface;
	cairo_t * saved_cr;
	svg_color_t color;
	svg_paint_t fill_paint;
	svg_paint_t stroke_paint;
	double fill_opacity;
	double stroke_opacity;
	char * font_family;
	double font_size;
	svg_font_style_t font_style;
	uint font_weight;
	int font_dirty;
	double * dash;
	int num_dashes;
	double dash_offset;
	double opacity;
	uint viewport_width;
	uint viewport_height;
	int bbox;
	svg_text_anchor_t text_anchor;
	struct svg_cairo_state * next;
} svg_cairo_state_t;

struct svg_cairo {
	svg_t * svg;
	cairo_t * cr;
	svg_cairo_state_t * state;
	uint viewport_width;
	uint viewport_height;
};
/* 
	svg_cairo_sprintf_alloc.c 
*/
int _svg_cairo_sprintf_alloc(char ** str, const char * fmt, ...);
int _svg_cairo_vsprintf_alloc(char ** str, const char * fmt, va_list ap);
/* 
	svg_cairo_state.c 
*/
svg_cairo_status_t _svg_cairo_state_create(svg_cairo_state_t ** state);
svg_cairo_status_t _svg_cairo_state_init(svg_cairo_state_t * state);
svg_cairo_status_t _svg_cairo_state_init_copy(svg_cairo_state_t * state, const svg_cairo_state_t * other);
svg_cairo_status_t _svg_cairo_state_deinit(svg_cairo_state_t * state);
svg_cairo_status_t _svg_cairo_state_destroy(svg_cairo_state_t * state);
svg_cairo_state_t * _svg_cairo_state_push(svg_cairo_state_t * state);
svg_cairo_state_t * _svg_cairo_state_pop(svg_cairo_state_t * state);

#endif
