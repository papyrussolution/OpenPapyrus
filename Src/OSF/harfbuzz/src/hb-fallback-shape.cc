/*
 * Copyright © 2011  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Google Author(s): Behdad Esfahbod
 */
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifndef HB_NO_FALLBACK_SHAPE
/*
 * shaper face data
 */
struct hb_fallback_face_data_t {};

hb_fallback_face_data_t * _hb_fallback_shaper_face_data_create(hb_face_t * face CXX_UNUSED_PARAM)
{
	return (hb_fallback_face_data_t*)HB_SHAPER_DATA_SUCCEEDED;
}

void _hb_fallback_shaper_face_data_destroy(hb_fallback_face_data_t * data CXX_UNUSED_PARAM)
{
}

/*
 * shaper font data
 */

struct hb_fallback_font_data_t {};

hb_fallback_font_data_t * _hb_fallback_shaper_font_data_create(hb_font_t * font CXX_UNUSED_PARAM)
{
	return (hb_fallback_font_data_t*)HB_SHAPER_DATA_SUCCEEDED;
}

void _hb_fallback_shaper_font_data_destroy(hb_fallback_font_data_t * data CXX_UNUSED_PARAM)
{
}

/*
 * shaper
 */

hb_bool_t _hb_fallback_shape(hb_shape_plan_t * shape_plan CXX_UNUSED_PARAM,
    hb_font_t * font,
    hb_buffer_t * buffer,
    const hb_feature_t * features CXX_UNUSED_PARAM,
    uint num_features CXX_UNUSED_PARAM)
{
	/* TODO
	 *
	 * - Apply fallback kern.
	 * - Handle Variation Selectors?
	 * - Apply normalization?
	 *
	 * This will make the fallback shaper into a dumb "TrueType"
	 * shaper which many people unfortunately still request.
	 */

	hb_codepoint_t space;
	bool has_space = (bool)font->get_nominal_glyph(' ', &space);

	buffer->clear_positions();

	hb_direction_t direction = buffer->props.direction;
	hb_unicode_funcs_t * unicode = buffer->unicode;
	uint count = buffer->len;
	hb_glyph_info_t * info = buffer->info;
	hb_glyph_position_t * pos = buffer->pos;
	for(uint i = 0; i < count; i++) {
		if(has_space && unicode->is_default_ignorable(info[i].codepoint)) {
			info[i].codepoint = space;
			pos[i].x_advance = 0;
			pos[i].y_advance = 0;
			continue;
		}
		(void)font->get_nominal_glyph(info[i].codepoint, &info[i].codepoint);
		font->get_glyph_advance_for_direction(info[i].codepoint,
		    direction,
		    &pos[i].x_advance,
		    &pos[i].y_advance);
		font->subtract_glyph_origin_for_direction(info[i].codepoint,
		    direction,
		    &pos[i].x_offset,
		    &pos[i].y_offset);
	}

	if(HB_DIRECTION_IS_BACKWARD(direction))
		hb_buffer_reverse(buffer);

	buffer->safe_to_break_all();

	return true;
}

#endif
