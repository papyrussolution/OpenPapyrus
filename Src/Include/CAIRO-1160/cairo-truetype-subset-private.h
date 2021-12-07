/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2006 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 *	Kristian Høgsberg <krh@redhat.com>
 *	Adrian Johnson <ajohnson@redneon.com>
 */

#ifndef CAIRO_TRUETYPE_SUBSET_PRIVATE_H
#define CAIRO_TRUETYPE_SUBSET_PRIVATE_H

#include "cairoint.h"

#if CAIRO_HAS_FONT_SUBSET

/* The structs defined here should strictly follow the TrueType
 * specification and not be padded.  We use only 16-bit integer
 * in their definition to guarantee that.  The fields of type
 * "FIXED" in the TT spec are broken into two *_1 and *_2 16-bit
 * parts, and 64-bit members are broken into four.
 *
 * The test truetype-tables in the test suite makes sure that
 * these tables have the right size.  Please update that test
 * if you add new tables/structs that should be packed.
 */

#define MAKE_TT_TAG(a, b, c, d)    (a<<24 | b<<16 | c<<8 | d)
#define TT_TAG_CFF    MAKE_TT_TAG('C','F','F',' ')
#define TT_TAG_cmap   MAKE_TT_TAG('c','m','a','p')
#define TT_TAG_cvt    MAKE_TT_TAG('c','v','t',' ')
#define TT_TAG_fpgm   MAKE_TT_TAG('f','p','g','m')
#define TT_TAG_glyf   MAKE_TT_TAG('g','l','y','f')
#define TT_TAG_head   MAKE_TT_TAG('h','e','a','d')
#define TT_TAG_hhea   MAKE_TT_TAG('h','h','e','a')
#define TT_TAG_hmtx   MAKE_TT_TAG('h','m','t','x')
#define TT_TAG_loca   MAKE_TT_TAG('l','o','c','a')
#define TT_TAG_maxp   MAKE_TT_TAG('m','a','x','p')
#define TT_TAG_name   MAKE_TT_TAG('n','a','m','e')
#define TT_TAG_OS2    MAKE_TT_TAG('O','S','/','2')
#define TT_TAG_post   MAKE_TT_TAG('p','o','s','t')
#define TT_TAG_prep   MAKE_TT_TAG('p','r','e','p')

/* All tt_* structs are big-endian */
typedef struct _tt_cmap_index {
    uint16 platform;
    uint16 encoding;
    uint32 offset;
} tt_cmap_index_t;

typedef struct _tt_cmap {
    uint16        version;
    uint16        num_tables;
    tt_cmap_index_t index[1];
} tt_cmap_t;

typedef struct _segment_map {
    uint16 format;
    uint16 length;
    uint16 version;
    uint16 segCountX2;
    uint16 searchRange;
    uint16 entrySelector;
    uint16 rangeShift;
    uint16 endCount[1];
} tt_segment_map_t;

typedef struct _tt_head {
    int16     version_1;
    int16     version_2;
    int16     revision_1;
    int16     revision_2;
    uint16    checksum_1;
    uint16    checksum_2;
    uint16    magic_1;
    uint16    magic_2;
    uint16    flags;
    uint16    units_per_em;
    int16     created_1;
    int16     created_2;
    int16     created_3;
    int16     created_4;
    int16     modified_1;
    int16     modified_2;
    int16     modified_3;
    int16     modified_4;
    int16     x_min; /* FWORD */
    int16     y_min; /* FWORD */
    int16     x_max; /* FWORD */
    int16     y_max; /* FWORD */
    uint16    mac_style;
    uint16    lowest_rec_pppem;
    int16     font_direction_hint;
    int16     index_to_loc_format;
    int16     glyph_data_format;
} tt_head_t;

typedef struct _tt_hhea {
    int16     version_1;
    int16     version_2;
    int16     ascender; /* FWORD */
    int16     descender; /* FWORD */
    int16     line_gap; /* FWORD */
    uint16    advance_max_width; /* UFWORD */
    int16     min_left_side_bearing; /* FWORD */
    int16     min_right_side_bearing; /* FWORD */
    int16     x_max_extent; /* FWORD */
    int16     caret_slope_rise;
    int16     caret_slope_run;
    int16     reserved[5];
    int16     metric_data_format;
    uint16    num_hmetrics;
} tt_hhea_t;

typedef struct _tt_maxp {
    int16     version_1;
    int16     version_2;
    uint16    num_glyphs;
    uint16    max_points;
    uint16    max_contours;
    uint16    max_composite_points;
    uint16    max_composite_contours;
    uint16    max_zones;
    uint16    max_twilight_points;
    uint16    max_storage;
    uint16    max_function_defs;
    uint16    max_instruction_defs;
    uint16    max_stack_elements;
    uint16    max_size_of_instructions;
    uint16    max_component_elements;
    uint16    max_component_depth;
} tt_maxp_t;

typedef struct _tt_name_record {
    uint16 platform;
    uint16 encoding;
    uint16 language;
    uint16 name;
    uint16 length;
    uint16 offset;
} tt_name_record_t;

typedef struct _tt_name {
    uint16   format;
    uint16   num_records;
    uint16   strings_offset;
    tt_name_record_t records[1];
} tt_name_t;


/* bitmask for fsSelection field */
#define TT_FS_SELECTION_ITALIC   1
#define TT_FS_SELECTION_BOLD    32

/* _unused fields are defined in TT spec but not used by cairo */
typedef struct _tt_os2 {
    uint16   _unused1[2];
    uint16   usWeightClass;
    uint16   _unused2[28];
    uint16   fsSelection;
    uint16   _unused3[11];
} tt_os2_t;

/* composite_glyph_t flags */
#define TT_ARG_1_AND_2_ARE_WORDS     0x0001
#define TT_WE_HAVE_A_SCALE           0x0008
#define TT_MORE_COMPONENTS           0x0020
#define TT_WE_HAVE_AN_X_AND_Y_SCALE  0x0040
#define TT_WE_HAVE_A_TWO_BY_TWO      0x0080

typedef struct _tt_composite_glyph {
    uint16 flags;
    uint16 index;
    uint16 args[6]; /* 1 to 6 arguments depending on value of flags */
} tt_composite_glyph_t;

typedef struct _tt_glyph_data {
    int16           num_contours;
    int8_t            data[8];
    tt_composite_glyph_t glyph;
} tt_glyph_data_t;

#endif /* CAIRO_HAS_FONT_SUBSET */

#endif /* CAIRO_TRUETYPE_SUBSET_PRIVATE_H */
