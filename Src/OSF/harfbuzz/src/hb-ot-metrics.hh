/*
 * Copyright © 2018  Ebrahim Byagowi
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 */

#ifndef HB_OT_METRICS_HH
#define HB_OT_METRICS_HH

#include "hb.hh"

HB_INTERNAL bool _hb_ot_metrics_get_position_common(hb_font_t * font, hb_ot_metrics_tag_t metrics_tag, hb_position_t * position /*OUT  May be NULL*/);

#endif /* HB_OT_METRICS_HH */
