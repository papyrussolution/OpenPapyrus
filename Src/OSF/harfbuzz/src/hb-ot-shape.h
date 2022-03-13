/*
 * Copyright © 2013  Red Hat, Inc.
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Red Hat Author(s): Behdad Esfahbod
 */
#ifndef HB_OT_H_IN
	#error "Include <hb-ot.h> instead."
#endif
#ifndef HB_OT_SHAPE_H
#define HB_OT_SHAPE_H
	#include "hb.h"

	HB_BEGIN_DECLS
		/* TODO port to shape-plan / set. */
		HB_EXTERN void hb_ot_shape_glyphs_closure(hb_font_t * font, hb_buffer_t * buffer, const hb_feature_t * features, uint num_features, hb_set_t * glyphs);
		HB_EXTERN void hb_ot_shape_plan_collect_lookups(hb_shape_plan_t * shape_plan, hb_tag_t table_tag, hb_set_t * lookup_indexes /*OUT*/);
	HB_END_DECLS
#endif /* HB_OT_SHAPE_H */
