/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2011  Google, Inc.
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
#ifndef HB_ICU_H
#define HB_ICU_H

#include "hb.h"
#include <unicode/uscript.h>

HB_BEGIN_DECLS
	HB_EXTERN hb_script_t hb_icu_script_to_script(UScriptCode script);
	HB_EXTERN UScriptCode hb_icu_script_from_script(hb_script_t script);
	HB_EXTERN hb_unicode_funcs_t * hb_icu_get_unicode_funcs(void);
HB_END_DECLS

#endif /* HB_ICU_H */
