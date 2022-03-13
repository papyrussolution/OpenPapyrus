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
#ifndef HB_OT_SHAPE_FALLBACK_HH
#define HB_OT_SHAPE_FALLBACK_HH

#include "hb.hh"
#include "hb-ot-shape.hh"

HB_INTERNAL void _hb_ot_shape_fallback_mark_position(const hb_ot_shape_plan_t * plan, hb_font_t * font,
    hb_buffer_t * buffer, bool adjust_offsets_when_zeroing);
HB_INTERNAL void _hb_ot_shape_fallback_mark_position_recategorize_marks(const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer);
HB_INTERNAL void _hb_ot_shape_fallback_kern(const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer);
HB_INTERNAL void _hb_ot_shape_fallback_spaces(const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer);

#endif /* HB_OT_SHAPE_FALLBACK_HH */
