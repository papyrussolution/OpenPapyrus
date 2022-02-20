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
#ifndef HB_NUMBER_HH
#define HB_NUMBER_HH

HB_INTERNAL bool hb_parse_int(const char ** pp, const char * end, int * pv, bool whole_buffer = false);
HB_INTERNAL bool hb_parse_uint(const char ** pp, const char * end, uint * pv, bool whole_buffer = false, int base = 10);
HB_INTERNAL bool hb_parse_double(const char ** pp, const char * end, double * pv, bool whole_buffer = false);

#endif /* HB_NUMBER_HH */
