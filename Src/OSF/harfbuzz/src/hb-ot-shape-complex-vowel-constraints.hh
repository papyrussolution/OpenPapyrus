/*
 * Copyright © 2018  Google, Inc.
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
#ifndef HB_OT_SHAPE_COMPLEX_VOWEL_CONSTRAINTS_HH
#define HB_OT_SHAPE_COMPLEX_VOWEL_CONSTRAINTS_HH

#include "hb.hh"
#include "hb-ot-shape-complex.hh"

HB_INTERNAL void _hb_preprocess_text_vowel_constraints(const hb_ot_shape_plan_t * plan, hb_buffer_t * buffer, hb_font_t * font);

#endif /* HB_OT_SHAPE_COMPLEX_VOWEL_CONSTRAINTS_HH */
