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

#ifndef SVG_CAIRO_H
#define SVG_CAIRO_H

#include <cairo.h>
#include "svg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum svg_cairo_status {
	SVG_CAIRO_STATUS_SUCCESS = SVG_STATUS_SUCCESS,
	SVG_CAIRO_STATUS_NO_MEMORY = SVG_STATUS_NO_MEMORY,
	SVG_CAIRO_STATUS_IO_ERROR = SVG_STATUS_IO_ERROR,
	SVG_CAIRO_STATUS_FILE_NOT_FOUND = SVG_STATUS_FILE_NOT_FOUND,
	SVG_CAIRO_STATUS_INVALID_VALUE = SVG_STATUS_INVALID_VALUE,
	SVG_CAIRO_STATUS_INVALID_CALL = SVG_STATUS_INVALID_CALL,
	SVG_CAIRO_STATUS_PARSE_ERROR = SVG_STATUS_PARSE_ERROR
} svg_cairo_status_t;

typedef struct svg_cairo svg_cairo_t;

svg_cairo_status_t svg_cairo_create(svg_cairo_t ** svg_cairo);
svg_cairo_status_t svg_cairo_destroy(svg_cairo_t * svg_cairo);
svg_cairo_status_t svg_cairo_parse(svg_cairo_t * svg_cairo, const char * filename);
svg_cairo_status_t svg_cairo_parse_file(svg_cairo_t * svg_cairo, FILE * file);
svg_cairo_status_t svg_cairo_parse_buffer(svg_cairo_t * svg_cairo, const char * buf, size_t count);
svg_cairo_status_t svg_cairo_parse_chunk_begin(svg_cairo_t * svg_cairo);
svg_cairo_status_t svg_cairo_parse_chunk(svg_cairo_t * svg_cairo, const char * buf, size_t count);
svg_cairo_status_t svg_cairo_parse_chunk_end(svg_cairo_t * svg_cairo);
svg_cairo_status_t svg_cairo_render(svg_cairo_t * svg_cairo, cairo_t * xrs);
/* XXX: Ugh... this inconsistent interface needs to be cleaned up. */
svg_cairo_status_t svg_cairo_set_viewport_dimension(svg_cairo_t * svg_cairo, unsigned int width, unsigned int height);
void svg_cairo_get_size(svg_cairo_t * svg_cairo, unsigned int * width, unsigned int * height);

#ifdef __cplusplus
}
#endif

#endif
