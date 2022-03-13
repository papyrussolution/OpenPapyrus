/*
 * Copyright © 2019  Ebrahim Byagowi
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 */
#ifndef HB_H_IN
	#error "Include <hb.h> instead."
#endif
#ifndef HB_STYLE_H
#define HB_STYLE_H

#include "hb.h"

HB_BEGIN_DECLS
	#ifdef HB_EXPERIMENTAL_API
		HB_EXTERN float hb_style_get_value (hb_font_t *font, hb_tag_t style_tag);
	#endif
HB_END_DECLS
#endif /* HB_STYLE_H */
