/*
 * Copyright © 2014  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Google Author(s): Behdad Esfahbod, Roozbeh Pournader
 */
#ifndef HB_OT_H_IN
	#error "Include <hb-ot.h> instead."
#endif
#ifndef HB_OT_FONT_H
	#define HB_OT_FONT_H

	#include "hb.h"

	HB_BEGIN_DECLS
		HB_EXTERN void hb_ot_font_set_funcs(hb_font_t * font);
	HB_END_DECLS
#endif /* HB_OT_FONT_H */
