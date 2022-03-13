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

#ifndef HB_OT_NAME_LANGUAGE_HH
#define HB_OT_NAME_LANGUAGE_HH

#include "hb.hh"

HB_INTERNAL hb_language_t _hb_ot_name_language_for_ms_code(uint code);
HB_INTERNAL hb_language_t _hb_ot_name_language_for_mac_code(uint code);

#endif /* HB_OT_NAME_LANGUAGE_HH */
