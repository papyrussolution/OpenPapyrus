/*
 * Copyright © 2017  Google, Inc.
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
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifndef HB_NO_VAR
/**
 * SECTION:hb-ot-var
 * @title: hb-ot-var
 * @short_description: OpenType Font Variations
 * @include: hb-ot.h
 *
 * Functions for fetching information about OpenType Variable Fonts.
 **/

/*
 * fvar/avar
 */

/**
 * hb_ot_var_has_data:
 * @face: #hb_face_t to test
 *
 * This function allows to verify the presence of OpenType variation data on the face.
 *
 * Return value: true if face has a `fvar' table and false otherwise
 *
 * Since: 1.4.2
 **/
hb_bool_t hb_ot_var_has_data(hb_face_t * face)
{
	return face->table.fvar->has_data();
}
/**
 * hb_ot_var_get_axis_count:
 *
 * Since: 1.4.2
 **/
uint hb_ot_var_get_axis_count(hb_face_t * face)
{
	return face->table.fvar->get_axis_count();
}
#ifndef HB_DISABLE_DEPRECATED
/**
 * hb_ot_var_get_axes:
 *
 * Since: 1.4.2
 * Deprecated: 2.2.0
 **/
uint hb_ot_var_get_axes(hb_face_t * face,
    uint start_offset,
    uint * axes_count /* IN/OUT */,
    hb_ot_var_axis_t * axes_array /*OUT*/)
{
	return face->table.fvar->get_axes_deprecated(start_offset, axes_count, axes_array);
}

/**
 * hb_ot_var_find_axis:
 *
 * Since: 1.4.2
 * Deprecated: 2.2.0
 **/
hb_bool_t hb_ot_var_find_axis(hb_face_t * face, hb_tag_t axis_tag, uint * axis_index, hb_ot_var_axis_t * axis_info)
{
	return face->table.fvar->find_axis_deprecated(axis_tag, axis_index, axis_info);
}
#endif

/**
 * hb_ot_var_get_axis_infos:
 *
 * Since: 2.2.0
 **/
HB_EXTERN uint hb_ot_var_get_axis_infos(hb_face_t * face, uint start_offset, uint * axes_count/* IN/OUT */,
    hb_ot_var_axis_info_t * axes_array /*OUT*/)
{
	return face->table.fvar->get_axis_infos(start_offset, axes_count, axes_array);
}
/**
 * hb_ot_var_find_axis_info:
 *
 * Since: 2.2.0
 **/
HB_EXTERN hb_bool_t hb_ot_var_find_axis_info(hb_face_t   * face,
    hb_tag_t axis_tag,
    hb_ot_var_axis_info_t * axis_info)
{
	return face->table.fvar->find_axis_info(axis_tag, axis_info);
}

/*
 * Named instances.
 */

uint hb_ot_var_get_named_instance_count(hb_face_t * face)
{
	return face->table.fvar->get_instance_count();
}

hb_ot_name_id_t hb_ot_var_named_instance_get_subfamily_name_id(hb_face_t * face, uint instance_index)
{
	return face->table.fvar->get_instance_subfamily_name_id(instance_index);
}

hb_ot_name_id_t hb_ot_var_named_instance_get_postscript_name_id(hb_face_t * face, uint instance_index)
{
	return face->table.fvar->get_instance_postscript_name_id(instance_index);
}

uint hb_ot_var_named_instance_get_design_coords(hb_face_t * face, uint instance_index,
    uint * coords_length/* IN/OUT */, float * coords /*OUT*/)
{
	return face->table.fvar->get_instance_coords(instance_index, coords_length, coords);
}
/**
 * hb_ot_var_normalize_variations:
 *
 * Since: 1.4.2
 **/
void hb_ot_var_normalize_variations(hb_face_t * face,
    const hb_variation_t * variations,                            /* IN */
    uint variations_length,
    int * coords,                            /*OUT*/
    uint coords_length)
{
	for(uint i = 0; i < coords_length; i++)
		coords[i] = 0;

	const OT::fvar &fvar = *face->table.fvar;
	for(uint i = 0; i < variations_length; i++) {
		hb_ot_var_axis_info_t info;
		if(hb_ot_var_find_axis_info(face, variations[i].tag, &info) &&
		    info.axis_index < coords_length)
			coords[info.axis_index] = fvar.normalize_axis_value(info.axis_index, variations[i].value);
	}

	face->table.avar->map_coords(coords, coords_length);
}

/**
 * hb_ot_var_normalize_coords:
 *
 * Since: 1.4.2
 **/
void hb_ot_var_normalize_coords(hb_face_t * face,
    uint coords_length,
    const float * design_coords,                        /* IN */
    int * normalized_coords /*OUT*/)
{
	const OT::fvar &fvar = *face->table.fvar;
	for(uint i = 0; i < coords_length; i++)
		normalized_coords[i] = fvar.normalize_axis_value(i, design_coords[i]);

	face->table.avar->map_coords(normalized_coords, coords_length);
}

#endif
