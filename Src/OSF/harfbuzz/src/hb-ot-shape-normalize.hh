/*
 * Copyright © 2012  Google, Inc.
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
#ifndef HB_OT_SHAPE_NORMALIZE_HH
#define HB_OT_SHAPE_NORMALIZE_HH

#include "hb.hh"

/* buffer var allocations, used during the normalization process */
#define glyph_index()   var1.u32

struct hb_ot_shape_plan_t;

enum hb_ot_shape_normalization_mode_t {
	HB_OT_SHAPE_NORMALIZATION_MODE_NONE,
	HB_OT_SHAPE_NORMALIZATION_MODE_DECOMPOSED,
	HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS, /* Never composes base-to-base */
	HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT, /* Always fully decomposes and then
	                                                                        recompose back */

	HB_OT_SHAPE_NORMALIZATION_MODE_AUTO, /* See hb-ot-shape-normalize.cc for logic. */
	HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT = HB_OT_SHAPE_NORMALIZATION_MODE_AUTO
};

HB_INTERNAL void _hb_ot_shape_normalize(const hb_ot_shape_plan_t * shaper,
    hb_buffer_t * buffer,
    hb_font_t * font);

struct hb_ot_shape_normalize_context_t {
	const hb_ot_shape_plan_t * plan;
	hb_buffer_t * buffer;
	hb_font_t * font;
	hb_unicode_funcs_t * unicode;
	bool (* decompose) (const hb_ot_shape_normalize_context_t * c,
	    hb_codepoint_t ab,
	    hb_codepoint_t * a,
	    hb_codepoint_t * b);
	bool (* compose) (const hb_ot_shape_normalize_context_t * c,
	    hb_codepoint_t a,
	    hb_codepoint_t b,
	    hb_codepoint_t * ab);
};

#endif /* HB_OT_SHAPE_NORMALIZE_HH */
