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
#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_DEPRECATED_H
#define HB_OT_DEPRECATED_H

#include "hb.h"
#include "hb-ot-name.h"

HB_BEGIN_DECLS

#ifndef HB_DISABLE_DEPRECATED

/* https://github.com/harfbuzz/harfbuzz/issues/1734 */
#define HB_MATH_GLYPH_PART_FLAG_EXTENDER HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER

/* Like hb_ot_layout_table_find_script, but takes zero-terminated array of scripts to test */
HB_EXTERN HB_DEPRECATED_FOR(hb_ot_layout_table_select_script) hb_bool_t hb_ot_layout_table_choose_script(hb_face_t * face,
    hb_tag_t table_tag,
    const hb_tag_t * script_tags,
    uint * script_index,
    hb_tag_t * chosen_script);

HB_EXTERN HB_DEPRECATED_FOR(hb_ot_layout_script_select_language) hb_bool_t hb_ot_layout_script_find_language(hb_face_t * face,
    hb_tag_t table_tag,
    uint script_index,
    hb_tag_t language_tag,
    uint * language_index);

HB_EXTERN HB_DEPRECATED_FOR(hb_ot_tags_from_script_and_language) void hb_ot_tags_from_script(hb_script_t script,
    hb_tag_t * script_tag_1,
    hb_tag_t * script_tag_2);

HB_EXTERN HB_DEPRECATED_FOR(hb_ot_tags_from_script_and_language) hb_tag_t hb_ot_tag_from_language(hb_language_t language);

/**
 * HB_OT_VAR_NO_AXIS_INDEX:
 *
 * Since: 1.4.2
 * Deprecated: 2.2.0
 */
#define HB_OT_VAR_NO_AXIS_INDEX         0xFFFFFFFFu

/**
 * hb_ot_var_axis_t:
 *
 * Since: 1.4.2
 * Deprecated: 2.2.0
 */
typedef struct hb_ot_var_axis_t {
	hb_tag_t tag;
	hb_ot_name_id_t name_id;
	float min_value;
	float default_value;
	float max_value;
} hb_ot_var_axis_t;

HB_EXTERN HB_DEPRECATED_FOR(hb_ot_var_get_axis_infos) uint hb_ot_var_get_axes(hb_face_t * face,
    uint start_offset,
    uint * axes_count /* IN/OUT */,
    hb_ot_var_axis_t * axes_array /*OUT*/);

HB_EXTERN HB_DEPRECATED_FOR(hb_ot_var_find_axis_info) hb_bool_t hb_ot_var_find_axis(hb_face_t * face,
    hb_tag_t axis_tag,
    uint * axis_index,
    hb_ot_var_axis_t * axis_info);

#endif

HB_END_DECLS

#endif /* HB_OT_DEPRECATED_H */
