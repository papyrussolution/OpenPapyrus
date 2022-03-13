/*
 * Copyright © 2010,2012  Google, Inc.
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

#ifndef HB_NO_OT_SHAPE

#include "hb-ot-shape-complex.hh"

const hb_ot_complex_shaper_t _hb_ot_complex_shaper_default =
{
	nullptr, /* collect_features */
	nullptr, /* override_features */
	nullptr, /* data_create */
	nullptr, /* data_destroy */
	nullptr, /* preprocess_text */
	nullptr, /* postprocess_glyphs */
	HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
	nullptr, /* decompose */
	nullptr, /* compose */
	nullptr, /* setup_masks */
	HB_TAG_NONE, /* gpos_tag */
	nullptr, /* reorder_marks */
	HB_OT_SHAPE_ZERO_WIDTH_MARKS_BY_GDEF_LATE,
	true, /* fallback_position */
};

/* Same as default but no mark advance zeroing / fallback positioning.
 * Dumbest shaper ever, basically. */
const hb_ot_complex_shaper_t _hb_ot_complex_shaper_dumber =
{
	nullptr, /* collect_features */
	nullptr, /* override_features */
	nullptr, /* data_create */
	nullptr, /* data_destroy */
	nullptr, /* preprocess_text */
	nullptr, /* postprocess_glyphs */
	HB_OT_SHAPE_NORMALIZATION_MODE_DEFAULT,
	nullptr, /* decompose */
	nullptr, /* compose */
	nullptr, /* setup_masks */
	HB_TAG_NONE, /* gpos_tag */
	nullptr, /* reorder_marks */
	HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
	false, /* fallback_position */
};

#endif
