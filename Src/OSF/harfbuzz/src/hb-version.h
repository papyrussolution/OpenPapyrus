/*
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
 * Google Author(s): Behdad Esfahbod
 */
#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_VERSION_H
#define HB_VERSION_H

#include "hb-common.h"

HB_BEGIN_DECLS

#define HB_VERSION_MAJOR 2
#define HB_VERSION_MINOR 7
#define HB_VERSION_MICRO 2
#define HB_VERSION_STRING "2.7.2"
#define HB_VERSION_ATLEAST(major, minor, micro) ((major)*10000+(minor)*100+(micro) <= HB_VERSION_MAJOR*10000+HB_VERSION_MINOR*100+HB_VERSION_MICRO)

HB_EXTERN void hb_version(uint * major, uint * minor, uint * micro);
HB_EXTERN const char * hb_version_string(void);
HB_EXTERN hb_bool_t hb_version_atleast(uint major, uint minor, uint micro);

HB_END_DECLS

#endif /* HB_VERSION_H */
