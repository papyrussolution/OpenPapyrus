/*
 * Copyright © 2015  Mozilla Foundation.
 * Copyright © 2015  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Mozilla Author(s): Jonathan Kew
 * Google Author(s): Behdad Esfahbod
 */
#ifndef HB_OT_SHAPE_COMPLEX_ARABIC_HH
#define HB_OT_SHAPE_COMPLEX_ARABIC_HH

#include "hb.hh"
#include "hb-ot-shape-complex.hh"

struct arabic_shape_plan_t;

HB_INTERNAL void * data_create_arabic(const hb_ot_shape_plan_t * plan);
HB_INTERNAL void data_destroy_arabic(void * data);
HB_INTERNAL void setup_masks_arabic_plan(const arabic_shape_plan_t * arabic_plan, hb_buffer_t * buffer, hb_script_t script);

#endif /* HB_OT_SHAPE_COMPLEX_ARABIC_HH */
