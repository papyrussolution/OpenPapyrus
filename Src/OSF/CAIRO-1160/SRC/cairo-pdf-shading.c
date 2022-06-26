/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2009 Adrian Johnson
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Adrian Johnson.
 *
 * Contributor(s): Adrian Johnson <ajohnson@redneon.com>
 */
#include "cairoint.h"
#pragma hdrstop
#if CAIRO_HAS_PDF_OPERATORS

#include "cairo-pdf-shading-private.h"

static uchar * FASTCALL encode_coordinate(uchar * p, double c)
{
	uint32 f = static_cast<uint32>(c);
	*p++ = f >> 24;
	*p++ = (f >> 16) & 0xff;
	*p++ = (f >> 8)  & 0xff;
	*p++ = f & 0xff;
	return p;
}

static uchar * encode_point(uchar * p, const cairo_point_double_t * point)
{
	p = encode_coordinate(p, point->x);
	p = encode_coordinate(p, point->y);
	return p;
}

static uchar * FASTCALL encode_color_component(uchar * p, double color)
{
	uint16 c = _cairo_color_double_to_short(color);
	*p++ = c >> 8;
	*p++ = c & 0xff;
	return p;
}

static uchar * encode_color(uchar * p, const cairo_color_t * color)
{
	p = encode_color_component(p, color->red);
	p = encode_color_component(p, color->green);
	p = encode_color_component(p, color->blue);
	return p;
}

static uchar * encode_alpha(uchar * p, const cairo_color_t * color)
{
	p = encode_color_component(p, color->alpha);
	return p;
}

static cairo_status_t _cairo_pdf_shading_generate_decode_array(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * mesh, boolint is_alpha)
{
	uint i;
	boolint is_valid;
	uint num_color_components = is_alpha ? 1 : 3;
	shading->decode_array_length = 4 + num_color_components * 2;
	shading->decode_array = static_cast<double *>(_cairo_malloc_ab(shading->decode_array_length, sizeof(double)));
	if(UNLIKELY(shading->decode_array == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	is_valid = _cairo_mesh_pattern_coord_box(mesh, &shading->decode_array[0], &shading->decode_array[2], &shading->decode_array[1], &shading->decode_array[3]);
	assert(is_valid);
	assert(shading->decode_array[1] - shading->decode_array[0] >= DBL_EPSILON);
	assert(shading->decode_array[3] - shading->decode_array[2] >= DBL_EPSILON);
	for(i = 0; i < num_color_components; i++) {
		shading->decode_array[4 + 2*i] = 0;
		shading->decode_array[5 + 2*i] = 1;
	}
	return CAIRO_STATUS_SUCCESS;
}

/* The ISO32000 specification mandates this order for the points which
 * define the patch. */
static const int pdf_points_order_i[16] = { 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2 };
static const int pdf_points_order_j[16] = { 0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1 };

static cairo_status_t _cairo_pdf_shading_generate_data(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * mesh, boolint is_alpha)
{
	double x_off, y_off, x_scale, y_scale;
	uchar * p;
	uint i, j;
	uint num_color_components = is_alpha ? 1 : 3;
	uint num_patches = _cairo_array_num_elements(&mesh->patches);
	const cairo_mesh_patch_t * patch = (const cairo_mesh_patch_t *)_cairo_array_index_const(&mesh->patches, 0);
	/* Each patch requires:
	 *
	 * 1 flag - 1 byte
	 * 16 points. Each point is 2 coordinates. Each coordinate is
	 * stored in 4 bytes.
	 *
	 * 4 colors. Each color is stored in 2 bytes * num_color_components.
	 */
	shading->data_length = num_patches * (1 + 16 * 2 * 4 + 4 * 2 * num_color_components);
	shading->data = (uchar *)_cairo_malloc(shading->data_length);
	if(UNLIKELY(shading->data == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	x_off = shading->decode_array[0];
	y_off = shading->decode_array[2];
	x_scale = UINT32_MAX / (shading->decode_array[1] - x_off);
	y_scale = UINT32_MAX / (shading->decode_array[3] - y_off);
	p = shading->data;
	for(i = 0; i < num_patches; i++) {
		// edge flag 
		*p++ = 0;
		// 16 points 
		for(j = 0; j < 16; j++) {
			int pi = pdf_points_order_i[j];
			int pj = pdf_points_order_j[j];
			cairo_point_double_t point = patch[i].points[pi][pj];
			// Transform the point as specified in the decode array 
			point.x -= x_off;
			point.y -= y_off;
			point.x *= x_scale;
			point.y *= y_scale;
			// Make sure that rounding errors don't cause wraparounds 
			point.x = sclamp(point.x, 0.0, static_cast<double>(UINT32_MAX));
			point.y = sclamp(point.y, 0.0, static_cast<double>(UINT32_MAX));
			p = encode_point(p, &point);
		}
		// 4 colors 
		for(j = 0; j < 4; j++) {
			p = is_alpha ? encode_alpha(p, &patch[i].colors[j]) : encode_color(p, &patch[i].colors[j]);
		}
	}
	assert(p == shading->data + shading->data_length);
	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t _cairo_pdf_shading_init(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * mesh, boolint is_alpha)
{
	cairo_status_t status;
	assert(mesh->base.status == CAIRO_STATUS_SUCCESS);
	assert(mesh->current_patch == NULL);
	shading->shading_type = 7;
	/*
	 * Coordinates from the minimum to the maximum value of the mesh
	 * map to the [0..UINT32_MAX] range and are represented as
	 * uint32 values.
	 *
	 * Color components are represented as uint16 (in a 0.16 fixed
	 * point format, as in the rest of cairo).
	 */
	shading->bits_per_coordinate = 32;
	shading->bits_per_component = 16;
	shading->bits_per_flag = 8;
	shading->decode_array = NULL;
	shading->data = NULL;
	status = _cairo_pdf_shading_generate_decode_array(shading, mesh, is_alpha);
	if(UNLIKELY(status))
		return status;
	return _cairo_pdf_shading_generate_data(shading, mesh, is_alpha);
}

cairo_status_t _cairo_pdf_shading_init_color(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * pattern)
{
	return _cairo_pdf_shading_init(shading, pattern, FALSE);
}

cairo_status_t _cairo_pdf_shading_init_alpha(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * pattern)
{
	return _cairo_pdf_shading_init(shading, pattern, TRUE);
}

void _cairo_pdf_shading_fini(cairo_pdf_shading_t * shading)
{
	SAlloc::F(shading->data);
	SAlloc::F(shading->decode_array);
}

#endif /* CAIRO_HAS_PDF_OPERATORS */
