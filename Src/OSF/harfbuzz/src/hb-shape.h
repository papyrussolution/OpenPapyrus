/*
 * Copyright © 2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */
#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif
#ifndef HB_SHAPE_H
#define HB_SHAPE_H
	#include "hb-common.h"
	#include "hb-buffer.h"
	#include "hb-font.h"

	HB_BEGIN_DECLS
		HB_EXTERN void hb_shape(hb_font_t * font, hb_buffer_t * buffer, const hb_feature_t * features, uint num_features);
		HB_EXTERN hb_bool_t hb_shape_full(hb_font_t * font, hb_buffer_t * buffer, const hb_feature_t * features, uint num_features, const char * const * shaper_list);
		HB_EXTERN const char ** hb_shape_list_shapers(void);
	HB_END_DECLS
#endif /* HB_SHAPE_H */
