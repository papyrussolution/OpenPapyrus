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
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifndef HB_NO_META

#include "hb-ot-meta-table.hh"

/**
 * SECTION:hb-ot-meta
 * @title: hb-ot-meta
 * @short_description: OpenType Metadata
 * @include: hb-ot.h
 *
 * Functions for fetching metadata from fonts.
 **/

/**
 * hb_ot_meta_get_entry_tags:
 * @face: a face object
 * @start_offset: iteration's start offset
 * @entries_count:(inout) (allow-none): buffer size as input, filled size as output
 * @entries: (out caller-allocates) (array length=entries_count): entries tags buffer
 *
 * Return value: Number of all available feature types.
 *
 * Since: 2.6.0
 **/
uint hb_ot_meta_get_entry_tags(hb_face_t * face,
    uint start_offset,
    uint * entries_count,                       /*IN/OUT May be NULL*/
    hb_ot_meta_tag_t * entries /*OUT May be NULL*/)
{
	return face->table.meta->get_entries(start_offset, entries_count, entries);
}

/**
 * hb_ot_meta_reference_entry:
 * @face: a #hb_face_t object.
 * @meta_tag: tag of metadata you like to have.
 *
 * It fetches metadata entry of a given tag from a font.
 *
 * Returns: (transfer full): A blob containing the blob.
 *
 * Since: 2.6.0
 **/
hb_blob_t * hb_ot_meta_reference_entry(hb_face_t * face, hb_ot_meta_tag_t meta_tag)
{
	return face->table.meta->reference_entry(meta_tag);
}

#endif
