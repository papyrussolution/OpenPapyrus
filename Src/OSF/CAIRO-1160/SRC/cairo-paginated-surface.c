/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
 * Copyright © 2007 Adrian Johnson
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
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Carl Worth <cworth@cworth.org>
 *	Keith Packard <keithp@keithp.com>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

/* The paginated surface layer exists to provide as much code sharing
 * as possible for the various paginated surface backends in cairo
 * (PostScript, PDF, etc.). See cairo-paginated-private.h for
 * more details on how it works and how to use it.
 */
#include "cairoint.h"
#pragma hdrstop
#include "cairo-paginated-surface-private.h"
#include "cairo-surface-subsurface-inline.h"

extern const cairo_surface_backend_t cairo_paginated_surface_backend;

static cairo_int_status_t _cairo_paginated_surface_show_page(void * abstract_surface);

static cairo_surface_t * _cairo_paginated_surface_create_similar(void * abstract_surface, cairo_content_t content, int width, int height)
{
	cairo_rectangle_t rect;
	rect.x = rect.y = 0.;
	rect.width = width;
	rect.height = height;
	return cairo_recording_surface_create(content, &rect);
}

static cairo_surface_t * _create_recording_surface_for_target(cairo_surface_t * target, cairo_content_t content)
{
	cairo_rectangle_int_t rect;
	if(_cairo_surface_get_extents(target, &rect)) {
		cairo_rectangle_t recording_extents;
		recording_extents.x = rect.x;
		recording_extents.y = rect.y;
		recording_extents.width = rect.width;
		recording_extents.height = rect.height;
		return cairo_recording_surface_create(content, &recording_extents);
	}
	else {
		return cairo_recording_surface_create(content, NULL);
	}
}

cairo_surface_t * _cairo_paginated_surface_create(cairo_surface_t * target, cairo_content_t content, const cairo_paginated_surface_backend_t * backend)
{
	cairo_status_t status;
	cairo_paginated_surface_t * surface = (cairo_paginated_surface_t *)SAlloc::M_zon0(sizeof(cairo_paginated_surface_t));
	if(UNLIKELY(!surface)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto FAIL;
	}
	_cairo_surface_init(&surface->base, &cairo_paginated_surface_backend, NULL/* device */, content, target->is_vector);
	/* Override surface->base.type with target's type so we don't leak
	 * evidence of the paginated wrapper out to the user. */
	surface->base.type = target->type;
	surface->target = cairo_surface_reference(target);
	surface->content = content;
	surface->backend = backend;
	surface->recording_surface = _create_recording_surface_for_target(target, content);
	status = surface->recording_surface->status;
	if(UNLIKELY(status))
		goto FAIL_CLEANUP_SURFACE;
	surface->page_num = 1;
	surface->base.is_clear = TRUE;
	return &surface->base;
FAIL_CLEANUP_SURFACE:
	cairo_surface_destroy(target);
	SAlloc::F(surface);
FAIL:
	return _cairo_surface_create_in_error(status);
}

boolint FASTCALL _cairo_surface_is_paginated(const cairo_surface_t * surface)
{
	return surface->backend == &cairo_paginated_surface_backend;
}

cairo_surface_t * _cairo_paginated_surface_get_target(cairo_surface_t * surface)
{
	cairo_paginated_surface_t * paginated_surface;
	assert(_cairo_surface_is_paginated(surface));
	paginated_surface = (cairo_paginated_surface_t*)surface;
	return paginated_surface->target;
}

cairo_surface_t * _cairo_paginated_surface_get_recording(cairo_surface_t * surface)
{
	cairo_paginated_surface_t * paginated_surface;
	assert(_cairo_surface_is_paginated(surface));
	paginated_surface = (cairo_paginated_surface_t*)surface;
	return paginated_surface->recording_surface;
}

cairo_status_t _cairo_paginated_surface_set_size(cairo_surface_t * surface, int width, int height)
{
	cairo_paginated_surface_t * paginated_surface;
	cairo_status_t status;
	cairo_rectangle_t recording_extents;
	assert(_cairo_surface_is_paginated(surface));
	paginated_surface = (cairo_paginated_surface_t*)surface;
	recording_extents.x = 0;
	recording_extents.y = 0;
	recording_extents.width = width;
	recording_extents.height = height;
	cairo_surface_destroy(paginated_surface->recording_surface);
	paginated_surface->recording_surface = cairo_recording_surface_create(paginated_surface->content,
		&recording_extents);
	status = paginated_surface->recording_surface->status;
	if(UNLIKELY(status))
		return _cairo_surface_set_error(surface, status);

	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t _cairo_paginated_surface_finish(void * abstract_surface)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	cairo_status_t status = CAIRO_STATUS_SUCCESS;
	if(!surface->base.is_clear || surface->page_num == 1) {
		/* Bypass some of the sanity checking in cairo-surface.c, as we
		 * know that the surface is finished...
		 */
		status = _cairo_paginated_surface_show_page(surface);
	}

	/* XXX We want to propagate any errors from destroy(), but those are not
	 * returned via the api. So we need to explicitly finish the target,
	 * and check the status afterwards. However, we can only call finish()
	 * on the target, if we own it.
	 */
	if(CAIRO_REFERENCE_COUNT_GET_VALUE(&surface->target->ref_count) == 1)
		cairo_surface_finish(surface->target);
	if(status == CAIRO_STATUS_SUCCESS)
		status = cairo_surface_status(surface->target);
	cairo_surface_destroy(surface->target);

	cairo_surface_finish(surface->recording_surface);
	if(status == CAIRO_STATUS_SUCCESS)
		status = cairo_surface_status(surface->recording_surface);
	cairo_surface_destroy(surface->recording_surface);

	return status;
}

static cairo_surface_t * _cairo_paginated_surface_create_image_surface(void * abstract_surface,
    int width,
    int height)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	cairo_surface_t * image;
	cairo_font_options_t options;

	image = _cairo_image_surface_create_with_content(surface->content,
		width,
		height);

	cairo_surface_get_font_options(&surface->base, &options);
	_cairo_surface_set_font_options(image, &options);

	return image;
}

static cairo_surface_t * _cairo_paginated_surface_source(void * abstract_surface,
    cairo_rectangle_int_t * extents)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return _cairo_surface_get_source(surface->target, extents);
}

static cairo_status_t _cairo_paginated_surface_acquire_source_image(void * abstract_surface,
    cairo_image_surface_t ** image_out,
    void   ** image_extra)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	boolint is_bounded;
	cairo_surface_t * image;
	cairo_status_t status;
	cairo_rectangle_int_t extents;

	is_bounded = _cairo_surface_get_extents(surface->target, &extents);
	if(!is_bounded)
		return CAIRO_INT_STATUS_UNSUPPORTED;

	image = _cairo_paginated_surface_create_image_surface(surface,
		extents.width,
		extents.height);

	status = _cairo_recording_surface_replay(surface->recording_surface, image);
	if(UNLIKELY(status)) {
		cairo_surface_destroy(image);
		return status;
	}

	*image_out = (cairo_image_surface_t*)image;
	*image_extra = NULL;

	return CAIRO_STATUS_SUCCESS;
}

static void _cairo_paginated_surface_release_source_image(void * abstract_surface, cairo_image_surface_t * image, void * image_extra)
{
	cairo_surface_destroy(&image->base);
}

static cairo_int_status_t _paint_thumbnail_image(cairo_paginated_surface_t * surface, int width, int height)
{
	cairo_surface_pattern_t pattern;
	cairo_rectangle_int_t extents;
	double x_scale;
	double y_scale;
	cairo_surface_t * image = NULL;
	cairo_surface_t * opaque = NULL;
	cairo_status_t status = CAIRO_STATUS_SUCCESS;
	_cairo_surface_get_extents(surface->target, &extents);
	x_scale = (double)width / extents.width;
	y_scale = (double)height / extents.height;
	image = _cairo_paginated_surface_create_image_surface(surface, width, height);
	cairo_surface_set_device_scale(image, x_scale, y_scale);
	cairo_surface_set_device_offset(image, -extents.x*x_scale, -extents.y*y_scale);
	status = _cairo_recording_surface_replay(surface->recording_surface, image);
	if(UNLIKELY(status))
		goto cleanup;
	/* flatten transparency */
	opaque = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
	if(UNLIKELY(opaque->status)) {
		status = opaque->status;
		goto cleanup;
	}
	status = _cairo_surface_paint(opaque, CAIRO_OPERATOR_SOURCE, &_cairo_pattern_white.base, NULL);
	if(UNLIKELY(status))
		goto cleanup;

	_cairo_pattern_init_for_surface(&pattern, image);
	pattern.base.filter = CAIRO_FILTER_NEAREST;
	status = _cairo_surface_paint(opaque, CAIRO_OPERATOR_OVER, &pattern.base, NULL);
	_cairo_pattern_fini(&pattern.base);
	if(UNLIKELY(status))
		goto cleanup;
	status = surface->backend->set_thumbnail_image(surface->target, (cairo_image_surface_t*)opaque);
cleanup:
	if(image)
		cairo_surface_destroy(image);
	if(opaque)
		cairo_surface_destroy(opaque);
	return status;
}

static cairo_int_status_t _paint_fallback_image(cairo_paginated_surface_t * surface, const cairo_rectangle_int_t * rect)
{
	double x_scale = surface->base.x_fallback_resolution / surface->target->x_resolution;
	double y_scale = surface->base.y_fallback_resolution / surface->target->y_resolution;
	cairo_status_t status;
	cairo_surface_pattern_t pattern;
	cairo_clip_t * clip;
	int x = rect->x;
	int y = rect->y;
	int width = rect->width;
	int height = rect->height;
	cairo_surface_t * image = _cairo_paginated_surface_create_image_surface(surface, static_cast<int>(ceil(width * x_scale)), static_cast<int>(ceil(height * y_scale)));
	cairo_surface_set_device_scale(image, x_scale, y_scale);
	// set_device_offset just sets the x0/y0 components of the matrix; so we have to do the scaling manually. 
	cairo_surface_set_device_offset(image, -x*x_scale, -y*y_scale);
	status = _cairo_recording_surface_replay(surface->recording_surface, image);
	if(UNLIKELY(status))
		goto CLEANUP_IMAGE;
	_cairo_pattern_init_for_surface(&pattern, image);
	cairo_matrix_init(&pattern.base.matrix,
	    x_scale, 0, 0, y_scale, -x*x_scale, -y*y_scale);
	// the fallback should be rendered at native resolution, so disable filtering (if possible) to avoid introducing potential artifacts.
	pattern.base.filter = CAIRO_FILTER_NEAREST;
	clip = _cairo_clip_intersect_rectangle(NULL, rect);
	status = _cairo_surface_paint(surface->target, CAIRO_OPERATOR_SOURCE, &pattern.base, clip);
	_cairo_clip_destroy(clip);
	_cairo_pattern_fini(&pattern.base);
CLEANUP_IMAGE:
	cairo_surface_destroy(image);
	return status;
}

static cairo_int_status_t _paint_page(cairo_paginated_surface_t * surface)
{
	cairo_surface_t * analysis;
	cairo_int_status_t status;
	boolint has_supported, has_page_fallback, has_finegrained_fallback;
	if(UNLIKELY(surface->target->status))
		return surface->target->status;
	analysis = _cairo_analysis_surface_create(surface->target);
	if(UNLIKELY(analysis->status))
		return _cairo_surface_set_error(surface->target, analysis->status);
	status = surface->backend->set_paginated_mode(surface->target, CAIRO_PAGINATED_MODE_ANALYZE);
	if(UNLIKELY(status))
		goto FAIL;
	status = _cairo_recording_surface_replay_and_create_regions(surface->recording_surface,
		NULL, analysis, FALSE);
	if(status)
		goto FAIL;
	assert(analysis->status == CAIRO_STATUS_SUCCESS);
	if(surface->backend->set_bounding_box) {
		cairo_box_t bbox;
		_cairo_analysis_surface_get_bounding_box(analysis, &bbox);
		status = surface->backend->set_bounding_box(surface->target, &bbox);
		if(UNLIKELY(status))
			goto FAIL;
	}
	if(surface->backend->set_fallback_images_required) {
		boolint has_fallbacks = _cairo_analysis_surface_has_unsupported(analysis);

		status = surface->backend->set_fallback_images_required(surface->target,
			has_fallbacks);
		if(UNLIKELY(status))
			goto FAIL;
	}

	/* Finer grained fallbacks are currently only supported for some
	 * surface types */
	if(surface->backend->supports_fine_grained_fallbacks != NULL &&
	    surface->backend->supports_fine_grained_fallbacks(surface->target)) {
		has_supported = _cairo_analysis_surface_has_supported(analysis);
		has_page_fallback = FALSE;
		has_finegrained_fallback = _cairo_analysis_surface_has_unsupported(analysis);
	}
	else {
		if(_cairo_analysis_surface_has_unsupported(analysis)) {
			has_supported = FALSE;
			has_page_fallback = TRUE;
		}
		else {
			has_supported = TRUE;
			has_page_fallback = FALSE;
		}
		has_finegrained_fallback = FALSE;
	}
	if(has_supported) {
		status = surface->backend->set_paginated_mode(surface->target, CAIRO_PAGINATED_MODE_RENDER);
		if(UNLIKELY(status))
			goto FAIL;
		status = _cairo_recording_surface_replay_region(surface->recording_surface, NULL, surface->target, CAIRO_RECORDING_REGION_NATIVE);
		assert(status != CAIRO_INT_STATUS_UNSUPPORTED);
		if(UNLIKELY(status))
			goto FAIL;
	}
	if(has_page_fallback) {
		cairo_rectangle_int_t extents;
		boolint is_bounded;
		status = surface->backend->set_paginated_mode(surface->target, CAIRO_PAGINATED_MODE_FALLBACK);
		if(UNLIKELY(status))
			goto FAIL;
		is_bounded = _cairo_surface_get_extents(surface->target, &extents);
		if(!is_bounded) {
			status = CAIRO_INT_STATUS_UNSUPPORTED;
			goto FAIL;
		}
		status = _paint_fallback_image(surface, &extents);
		if(UNLIKELY(status))
			goto FAIL;
	}
	if(has_finegrained_fallback) {
		cairo_region_t * region;
		int num_rects, i;
		status = surface->backend->set_paginated_mode(surface->target, CAIRO_PAGINATED_MODE_FALLBACK);
		if(UNLIKELY(status))
			goto FAIL;
		region = _cairo_analysis_surface_get_unsupported(analysis);
		num_rects = cairo_region_num_rectangles(region);
		for(i = 0; i < num_rects; i++) {
			cairo_rectangle_int_t rect;
			cairo_region_get_rectangle(region, i, &rect);
			status = _paint_fallback_image(surface, &rect);
			if(UNLIKELY(status))
				goto FAIL;
		}
	}
	if(surface->backend->requires_thumbnail_image) {
		int width, height;
		if(surface->backend->requires_thumbnail_image(surface->target, &width, &height))
			_paint_thumbnail_image(surface, width, height);
	}
FAIL:
	cairo_surface_destroy(analysis);
	return _cairo_surface_set_error(surface->target, status);
}

static cairo_status_t _start_page(cairo_paginated_surface_t * surface)
{
	if(surface->target->status)
		return surface->target->status;
	if(!surface->backend->start_page)
		return CAIRO_STATUS_SUCCESS;
	return _cairo_surface_set_error(surface->target, surface->backend->start_page(surface->target));
}

static cairo_int_status_t _cairo_paginated_surface_copy_page(void * abstract_surface)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	cairo_status_t status = _start_page(surface);
	if(UNLIKELY(status))
		return status;
	status = _paint_page(surface);
	if(UNLIKELY(status))
		return status;
	surface->page_num++;
	/* XXX: It might make sense to add some support here for calling
	 * cairo_surface_copy_page on the target surface. It would be an
	 * optimization for the output, but the interaction with image
	 * fallbacks gets tricky. For now, we just let the target see a
	 * show_page and we implement the copying by simply not destroying
	 * the recording-surface. */

	cairo_surface_show_page(surface->target);
	return cairo_surface_status(surface->target);
}

static cairo_int_status_t _cairo_paginated_surface_show_page(void * abstract_surface)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	cairo_status_t status = _start_page(surface);
	if(UNLIKELY(status))
		return status;
	status = _paint_page(surface);
	if(UNLIKELY(status))
		return status;
	cairo_surface_show_page(surface->target);
	status = surface->target->status;
	if(UNLIKELY(status))
		return status;
	status = surface->recording_surface->status;
	if(UNLIKELY(status))
		return status;
	if(!surface->base.finished) {
		cairo_surface_destroy(surface->recording_surface);
		surface->recording_surface = _create_recording_surface_for_target(surface->target, surface->content);
		status = surface->recording_surface->status;
		if(UNLIKELY(status))
			return status;
		surface->page_num++;
		surface->base.is_clear = TRUE;
	}
	return CAIRO_STATUS_SUCCESS;
}

static boolint _cairo_paginated_surface_get_extents(void * abstract_surface, cairo_rectangle_int_t * rectangle)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return _cairo_surface_get_extents(surface->target, rectangle);
}

static void _cairo_paginated_surface_get_font_options(void * abstract_surface, cairo_font_options_t * options)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	cairo_surface_get_font_options(surface->target, options);
}

static cairo_int_status_t _cairo_paginated_surface_paint(void * abstract_surface, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_clip_t * clip)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return _cairo_surface_paint(surface->recording_surface, op, source, clip);
}

static cairo_int_status_t _cairo_paginated_surface_mask(void * abstract_surface, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return _cairo_surface_mask(surface->recording_surface, op, source, mask, clip);
}

static cairo_int_status_t _cairo_paginated_surface_stroke(void * abstract_surface,
    cairo_operator_t op,
    const cairo_pattern_t * source,
    const cairo_path_fixed_t * path,
    const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm,
    const cairo_matrix_t * ctm_inverse,
    double tolerance,
    cairo_antialias_t antialias,
    const cairo_clip_t * clip)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return _cairo_surface_stroke(surface->recording_surface, op, source, path, style, ctm, ctm_inverse, tolerance, antialias, clip);
}

static cairo_int_status_t _cairo_paginated_surface_fill(void * abstract_surface,
    cairo_operator_t op,
    const cairo_pattern_t * source,
    const cairo_path_fixed_t * path,
    cairo_fill_rule_t fill_rule,
    double tolerance,
    cairo_antialias_t antialias,
    const cairo_clip_t * clip)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return _cairo_surface_fill(surface->recording_surface, op, source, path, fill_rule, tolerance, antialias, clip);
}

static boolint _cairo_paginated_surface_has_show_text_glyphs(void * abstract_surface)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);
	return cairo_surface_has_show_text_glyphs(surface->target);
}

static cairo_int_status_t _cairo_paginated_surface_show_text_glyphs(void * abstract_surface,
    cairo_operator_t op,
    const cairo_pattern_t * source,
    const char * utf8,
    int utf8_len,
    cairo_glyph_t * glyphs,
    int num_glyphs,
    const cairo_text_cluster_t * clusters,
    int num_clusters,
    cairo_text_cluster_flags_t cluster_flags,
    cairo_scaled_font_t * scaled_font,
    const cairo_clip_t * clip)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);

	return _cairo_surface_show_text_glyphs(surface->recording_surface, op, source,
		   utf8, utf8_len,
		   glyphs, num_glyphs,
		   clusters, num_clusters,
		   cluster_flags,
		   scaled_font,
		   clip);
}

static const char ** _cairo_paginated_surface_get_supported_mime_types(void * abstract_surface)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);

	if(surface->target->backend->get_supported_mime_types)
		return surface->target->backend->get_supported_mime_types(surface->target);

	return NULL;
}

static cairo_int_status_t _cairo_paginated_surface_tag(void * abstract_surface,
    boolint begin,
    const char * tag_name,
    const char * attributes,
    const cairo_pattern_t * source,
    const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm,
    const cairo_matrix_t * ctm_inverse,
    const cairo_clip_t * clip)
{
	cairo_paginated_surface_t * surface = static_cast<cairo_paginated_surface_t *>(abstract_surface);

	return _cairo_surface_tag(surface->recording_surface,
		   begin, tag_name, attributes,
		   source, style,
		   ctm, ctm_inverse,
		   clip);
}

static cairo_surface_t * _cairo_paginated_surface_snapshot(void * abstract_other)
{
	cairo_paginated_surface_t * other = (cairo_paginated_surface_t *)abstract_other;
	return other->recording_surface->backend->snapshot(other->recording_surface);
}

static cairo_t * _cairo_paginated_context_create(void * target)
{
	cairo_paginated_surface_t * surface = (cairo_paginated_surface_t *)target;
	if(_cairo_surface_is_subsurface(&surface->base))
		surface = (cairo_paginated_surface_t*)_cairo_surface_subsurface_get_target(&surface->base);
	return surface->recording_surface->backend->create_context(target);
}

static const cairo_surface_backend_t cairo_paginated_surface_backend = {
	CAIRO_INTERNAL_SURFACE_TYPE_PAGINATED,
	_cairo_paginated_surface_finish,
	_cairo_paginated_context_create,
	_cairo_paginated_surface_create_similar,
	NULL, /* create simlar image */
	NULL, /* map to image */
	NULL, /* unmap image */
	_cairo_paginated_surface_source,
	_cairo_paginated_surface_acquire_source_image,
	_cairo_paginated_surface_release_source_image,
	_cairo_paginated_surface_snapshot,
	_cairo_paginated_surface_copy_page,
	_cairo_paginated_surface_show_page,
	_cairo_paginated_surface_get_extents,
	_cairo_paginated_surface_get_font_options,

	NULL, /* flush */
	NULL, /* mark_dirty_rectangle */

	_cairo_paginated_surface_paint,
	_cairo_paginated_surface_mask,
	_cairo_paginated_surface_stroke,
	_cairo_paginated_surface_fill,
	NULL, /* fill_stroke */
	NULL, /* show_glyphs */
	_cairo_paginated_surface_has_show_text_glyphs,
	_cairo_paginated_surface_show_text_glyphs,
	_cairo_paginated_surface_get_supported_mime_types,
	_cairo_paginated_surface_tag,
};
