/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2004 Red Hat, Inc
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

/*
 * Useful links:
 * http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6.html
 * http://www.microsoft.com/typography/specs/default.htm
 */

#include "cairoint.h"
#pragma hdrstop
#define _DEFAULT_SOURCE /* for snprintf(), sstrdup() */

#if CAIRO_HAS_FONT_SUBSET

#include "cairo-truetype-subset-private.h"

typedef struct subset_glyph subset_glyph_t;
struct subset_glyph {
	int parent_index;
	ulong location;
};

typedef struct _cairo_truetype_font cairo_truetype_font_t;

typedef struct table table_t;
struct table {
	ulong tag;
	cairo_status_t (* write) (cairo_truetype_font_t * font, ulong tag);
	int pos; /* position in the font directory */
};

struct _cairo_truetype_font {
	cairo_scaled_font_subset_t * scaled_font_subset;

	table_t truetype_tables[10];
	int num_tables;

	struct {
		char * font_name;
		char * ps_name;
		int num_glyphs_in_face; /* glyphs in font */
		long x_min, y_min, x_max, y_max;
		long ascent, descent;
		int units_per_em;
	} base;

	subset_glyph_t * glyphs; /* array size: num_glyphs_in_face + 2 */
	const cairo_scaled_font_backend_t * backend;
	uint num_glyphs; /* glyphs used */
	int * widths; /* array size: num_glyphs_in_face  + 1 */
	int checksum_index;
	cairo_array_t output;
	cairo_array_t string_offsets;
	ulong last_offset;
	ulong last_boundary;
	int * parent_to_subset; /* array size: num_glyphs_in_face + 1 */
	cairo_status_t status;
	boolint is_pdf;
};

/*
 * Test that the structs we define for TrueType tables have the
 * correct size, ie. they are not padded.
 */
#define check(T, S) COMPILE_TIME_ASSERT(sizeof(T) == (S))
check(tt_head_t,       54);
check(tt_hhea_t,       36);
check(tt_maxp_t,       32);
check(tt_name_record_t, 12);
check(tt_name_t,       18);
check(tt_composite_glyph_t, 16);
check(tt_glyph_data_t, 26);
#undef check

static cairo_status_t cairo_truetype_font_use_glyph(cairo_truetype_font_t * font, unsigned short glyph, unsigned short * out);

#define SFNT_VERSION                    0x00010000
#define SFNT_STRING_MAX_LENGTH  65535

static cairo_status_t FASTCALL _cairo_truetype_font_set_error(cairo_truetype_font_t * font, cairo_status_t status)
{
	if(status == CAIRO_STATUS_SUCCESS || status == (int)CAIRO_INT_STATUS_UNSUPPORTED)
		return status;
	_cairo_status_set_error(&font->status, status);
	return _cairo_error(status);
}

static cairo_status_t _cairo_truetype_font_create(cairo_scaled_font_subset_t * scaled_font_subset, boolint is_pdf, cairo_truetype_font_t  ** font_return)
{
	cairo_status_t status;
	boolint is_synthetic;
	cairo_truetype_font_t * font;
	tt_head_t head;
	tt_hhea_t hhea;
	tt_maxp_t maxp;
	ulong size;
	const cairo_scaled_font_backend_t * backend = scaled_font_subset->scaled_font->backend;
	if(!backend->load_truetype_table)
		return CAIRO_INT_STATUS_UNSUPPORTED;
	/* FIXME: We should either support subsetting vertical fonts, or fail on
	 * vertical.  Currently font_options_t doesn't have vertical flag, but
	 * it should be added in the future.  For now, the freetype backend
	 * returns UNSUPPORTED in load_truetype_table if the font is vertical.
	 *
	 *  if(cairo_font_options_get_vertical_layout (scaled_font_subset->scaled_font))
	 * return CAIRO_INT_STATUS_UNSUPPORTED;
	 */

	/* We need to use a fallback font if this font differs from the glyf outlines. */
	if(backend->is_synthetic) {
		status = backend->is_synthetic(scaled_font_subset->scaled_font, &is_synthetic);
		if(UNLIKELY(status))
			return status;
		if(is_synthetic)
			return CAIRO_INT_STATUS_UNSUPPORTED;
	}
	size = sizeof(tt_head_t);
	status = backend->load_truetype_table(scaled_font_subset->scaled_font, TT_TAG_head, 0, (uchar *)&head, &size);
	if(UNLIKELY(status))
		return status;
	size = sizeof(tt_maxp_t);
	status = backend->load_truetype_table(scaled_font_subset->scaled_font, TT_TAG_maxp, 0, (uchar *)&maxp, &size);
	if(UNLIKELY(status))
		return status;
	size = sizeof(tt_hhea_t);
	status = backend->load_truetype_table(scaled_font_subset->scaled_font, TT_TAG_hhea, 0, (uchar *)&hhea, &size);
	if(UNLIKELY(status))
		return status;
	font = (cairo_truetype_font_t *)SAlloc::M_zon0(sizeof(cairo_truetype_font_t));
	if(UNLIKELY(font == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	font->backend = backend;
	font->base.num_glyphs_in_face = be16_to_cpu(maxp.num_glyphs);
	font->scaled_font_subset = scaled_font_subset;
	font->last_offset = 0;
	font->last_boundary = 0;
	_cairo_array_init(&font->output, sizeof(char));
	status = _cairo_array_grow_by(&font->output, 4096);
	if(UNLIKELY(status))
		goto fail1;
	/* Add 2: +1 case font does not contain .notdef, and +1 because an extra
	 * entry is required to contain the end location of the last glyph.
	 */
	font->glyphs = (subset_glyph_t *)SAlloc::C(font->base.num_glyphs_in_face + 2, sizeof(subset_glyph_t));
	if(UNLIKELY(font->glyphs == NULL)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto fail1;
	}

	/* Add 1 in case font does not contain .notdef */
	font->parent_to_subset = (int *)SAlloc::C(font->base.num_glyphs_in_face + 1, sizeof(int));
	if(UNLIKELY(font->parent_to_subset == NULL)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto fail2;
	}

	font->is_pdf = is_pdf;
	font->num_glyphs = 0;
	font->base.x_min = (int16)be16_to_cpu(head.x_min);
	font->base.y_min = (int16)be16_to_cpu(head.y_min);
	font->base.x_max = (int16)be16_to_cpu(head.x_max);
	font->base.y_max = (int16)be16_to_cpu(head.y_max);
	font->base.ascent = (int16)be16_to_cpu(hhea.ascender);
	font->base.descent = (int16)be16_to_cpu(hhea.descender);
	font->base.units_per_em = (int16)be16_to_cpu(head.units_per_em);
	if(font->base.units_per_em == 0)
		font->base.units_per_em = 2048;

	font->base.ps_name = NULL;
	font->base.font_name = NULL;
	status = _cairo_truetype_read_font_name(scaled_font_subset->scaled_font,
		&font->base.ps_name,
		&font->base.font_name);
	if(_cairo_status_is_error(status))
		goto fail3;

	/* If the PS name is not found, create a CairoFont-x-y name. */
	if(font->base.ps_name == NULL) {
		font->base.ps_name = (char *)SAlloc::M_zon0(30);
		if(UNLIKELY(font->base.ps_name == NULL)) {
			status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
			goto fail3;
		}

		snprintf(font->base.ps_name, 30, "CairoFont-%u-%u",
		    scaled_font_subset->font_id,
		    scaled_font_subset->subset_id);
	}

	/* Add 1 in case font does not contain .notdef */
	font->widths = (int *)SAlloc::C(font->base.num_glyphs_in_face + 1, sizeof(int));
	if(UNLIKELY(font->widths == NULL)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto fail4;
	}

	_cairo_array_init(&font->string_offsets, sizeof(ulong));
	status = _cairo_array_grow_by(&font->string_offsets, 10);
	if(UNLIKELY(status))
		goto fail5;

	font->status = CAIRO_STATUS_SUCCESS;

	*font_return = font;

	return CAIRO_STATUS_SUCCESS;

fail5:
	_cairo_array_fini(&font->string_offsets);
	SAlloc::F(font->widths);
fail4:
	SAlloc::F(font->base.ps_name);
fail3:
	SAlloc::F(font->parent_to_subset);
	SAlloc::F(font->base.font_name);
fail2:
	SAlloc::F(font->glyphs);
fail1:
	_cairo_array_fini(&font->output);
	SAlloc::F(font);

	return status;
}

static void cairo_truetype_font_destroy(cairo_truetype_font_t * font)
{
	_cairo_array_fini(&font->string_offsets);
	SAlloc::F(font->widths);
	SAlloc::F(font->base.ps_name);
	SAlloc::F(font->base.font_name);
	SAlloc::F(font->parent_to_subset);
	SAlloc::F(font->glyphs);
	_cairo_array_fini(&font->output);
	SAlloc::F(font);
}

static cairo_status_t cairo_truetype_font_allocate_write_buffer(cairo_truetype_font_t * font,
    size_t length,
    uchar  ** buffer)
{
	cairo_status_t status;

	if(font->status)
		return font->status;

	status = _cairo_array_allocate(&font->output, length, (void **)buffer);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);

	return CAIRO_STATUS_SUCCESS;
}

static void cairo_truetype_font_write(cairo_truetype_font_t * font,
    const void * data,
    size_t length)
{
	cairo_status_t status;

	if(font->status)
		return;

	status = _cairo_array_append_multiple(&font->output, data, length);
	if(UNLIKELY(status))
		status = _cairo_truetype_font_set_error(font, status);
}

static void cairo_truetype_font_write_be16(cairo_truetype_font_t * font,
    uint16 value)
{
	uint16 be16_value;

	if(font->status)
		return;

	be16_value = cpu_to_be16(value);
	cairo_truetype_font_write(font, &be16_value, sizeof be16_value);
}

static void cairo_truetype_font_write_be32(cairo_truetype_font_t * font,
    uint32 value)
{
	uint32 be32_value;

	if(font->status)
		return;

	be32_value = cpu_to_be32(value);
	cairo_truetype_font_write(font, &be32_value, sizeof be32_value);
}

static cairo_status_t cairo_truetype_font_align_output(cairo_truetype_font_t * font, ulong * aligned)
{
	int pad;
	uchar * padding;
	int length = _cairo_array_num_elements(&font->output);
	*aligned = (length + 3) & ~3;
	pad = *aligned - length;
	if(pad) {
		cairo_status_t status = cairo_truetype_font_allocate_write_buffer(font, pad, &padding);
		if(UNLIKELY(status))
			return status;
		memzero(padding, pad);
	}
	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_check_boundary(cairo_truetype_font_t * font, ulong boundary)
{
	cairo_status_t status;
	if(font->status)
		return font->status;
	if(boundary - font->last_offset > SFNT_STRING_MAX_LENGTH) {
		status = _cairo_array_append(&font->string_offsets, &font->last_boundary);
		if(UNLIKELY(status))
			return _cairo_truetype_font_set_error(font, status);
		font->last_offset = font->last_boundary;
	}
	font->last_boundary = boundary;
	return CAIRO_STATUS_SUCCESS;
}

typedef struct _cmap_unicode_range {
	uint start;
	uint end;
} cmap_unicode_range_t;

static cmap_unicode_range_t winansi_unicode_ranges[] = {
	{ 0x0020, 0x007f },
	{ 0x00a0, 0x00ff },
	{ 0x0152, 0x0153 },
	{ 0x0160, 0x0161 },
	{ 0x0178, 0x0178 },
	{ 0x017d, 0x017e },
	{ 0x0192, 0x0192 },
	{ 0x02c6, 0x02c6 },
	{ 0x02dc, 0x02dc },
	{ 0x2013, 0x2026 },
	{ 0x2030, 0x2030 },
	{ 0x2039, 0x203a },
	{ 0x20ac, 0x20ac },
	{ 0x2122, 0x2122 },
};

static cairo_status_t cairo_truetype_font_write_cmap_table(cairo_truetype_font_t * font,
    ulong tag)
{
	int i;
	uint j;
	int range_offset;
	int num_ranges;
	int entry_selector;
	int length;

	num_ranges = ARRAY_LENGTH(winansi_unicode_ranges);

	length = 16 + (num_ranges + 1)*8;
	for(i = 0; i < num_ranges; i++)
		length += (winansi_unicode_ranges[i].end - winansi_unicode_ranges[i].start + 1)*2;

	entry_selector = 0;
	while((1 << entry_selector) <= (num_ranges + 1))
		entry_selector++;

	entry_selector--;

	cairo_truetype_font_write_be16(font, 0); /* Table version */
	cairo_truetype_font_write_be16(font, 1); /* Num tables */

	cairo_truetype_font_write_be16(font, 3); /* Platform */
	cairo_truetype_font_write_be16(font, 1); /* Encoding */
	cairo_truetype_font_write_be32(font, 12); /* Offset to start of table */

	/* Output a format 4 encoding table for the winansi encoding */

	cairo_truetype_font_write_be16(font, 4); /* Format */
	cairo_truetype_font_write_be16(font, length); /* Length */
	cairo_truetype_font_write_be16(font, 0); /* Version */
	cairo_truetype_font_write_be16(font, num_ranges*2 + 2); /* 2*segcount */
	cairo_truetype_font_write_be16(font, (1 << (entry_selector + 1))); /* searchrange */
	cairo_truetype_font_write_be16(font, entry_selector); /* entry selector */
	cairo_truetype_font_write_be16(font, num_ranges*2 + 2 - (1 << (entry_selector + 1))); /* rangeshift */
	for(i = 0; i < num_ranges; i++)
		cairo_truetype_font_write_be16(font, winansi_unicode_ranges[i].end); /* end count[] */
	cairo_truetype_font_write_be16(font, 0xffff); /* end count[] */

	cairo_truetype_font_write_be16(font, 0); /* reserved */

	for(i = 0; i < num_ranges; i++)
		cairo_truetype_font_write_be16(font, winansi_unicode_ranges[i].start); /* startCode[] */
	cairo_truetype_font_write_be16(font, 0xffff); /* startCode[] */

	for(i = 0; i < num_ranges; i++)
		cairo_truetype_font_write_be16(font, 0x0000); /* delta[] */
	cairo_truetype_font_write_be16(font, 1); /* delta[] */

	range_offset = num_ranges*2 + 2;
	for(i = 0; i < num_ranges; i++) {
		cairo_truetype_font_write_be16(font, range_offset); /* rangeOffset[] */
		range_offset += (winansi_unicode_ranges[i].end - winansi_unicode_ranges[i].start + 1)*2 - 2;
	}
	cairo_truetype_font_write_be16(font, 0); /* rangeOffset[] */

	for(i = 0; i < num_ranges; i++) {
		for(j = winansi_unicode_ranges[i].start; j < winansi_unicode_ranges[i].end + 1; j++) {
			int ch = _cairo_unicode_to_winansi(j);
			int glyph;
			if(ch > 0)
				glyph = font->scaled_font_subset->latin_to_subset_glyph_index[ch];
			else
				glyph = 0;
			cairo_truetype_font_write_be16(font, glyph);
		}
	}
	return font->status;
}

static cairo_status_t cairo_truetype_font_write_generic_table(cairo_truetype_font_t * font, ulong tag)
{
	cairo_status_t status;
	uchar * buffer;
	ulong size;

	if(font->status)
		return font->status;

	size = 0;
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
		tag, 0, NULL, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);

	status = cairo_truetype_font_allocate_write_buffer(font, size, &buffer);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);

	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
		tag, 0, buffer, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);

	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_remap_composite_glyph(cairo_truetype_font_t * font,
    uchar * buffer,
    ulong size)
{
	tt_glyph_data_t * glyph_data;
	tt_composite_glyph_t * composite_glyph;
	int num_args;
	int has_more_components;
	unsigned short flags;
	unsigned short index;
	cairo_status_t status;
	uchar * end = buffer + size;

	if(font->status)
		return font->status;

	glyph_data = (tt_glyph_data_t*)buffer;
	if((uchar *)(&glyph_data->data) >= end)
		return CAIRO_INT_STATUS_UNSUPPORTED;

	if((int16)be16_to_cpu(glyph_data->num_contours) >= 0)
		return CAIRO_STATUS_SUCCESS;

	composite_glyph = &glyph_data->glyph;
	do {
		if((uchar *)(&composite_glyph->args[1]) > end)
			return CAIRO_INT_STATUS_UNSUPPORTED;

		flags = be16_to_cpu(composite_glyph->flags);
		has_more_components = flags & TT_MORE_COMPONENTS;
		status = cairo_truetype_font_use_glyph(font, be16_to_cpu(composite_glyph->index), &index);
		if(UNLIKELY(status))
			return status;

		composite_glyph->index = cpu_to_be16(index);
		num_args = 1;
		if(flags & TT_ARG_1_AND_2_ARE_WORDS)
			num_args += 1;

		if(flags & TT_WE_HAVE_A_SCALE)
			num_args += 1;
		else if(flags & TT_WE_HAVE_AN_X_AND_Y_SCALE)
			num_args += 2;
		else if(flags & TT_WE_HAVE_A_TWO_BY_TWO)
			num_args += 4;

		composite_glyph = (tt_composite_glyph_t*)&(composite_glyph->args[num_args]);
	} while(has_more_components);

	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_write_glyf_table(cairo_truetype_font_t * font, ulong tag)
{
	ulong start_offset, index, size, next;
	tt_head_t header;
	ulong begin, end;
	uchar * buffer;
	uint i;
	union {
		uchar * bytes;
		uint16 * short_offsets;
		uint32 * long_offsets;
	} u;

	cairo_status_t status;
	if(font->status)
		return font->status;
	size = sizeof(tt_head_t);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
		TT_TAG_head, 0,
		(uchar *)&header, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	if(be16_to_cpu(header.index_to_loc_format) == 0)
		size = sizeof(int16) * (font->base.num_glyphs_in_face + 1);
	else
		size = sizeof(int32) * (font->base.num_glyphs_in_face + 1);
	u.bytes = static_cast<uchar *>(SAlloc::M_zon0(size));
	if(UNLIKELY(u.bytes == NULL))
		return _cairo_truetype_font_set_error(font, CAIRO_STATUS_NO_MEMORY);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, TT_TAG_loca, 0, u.bytes, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	start_offset = _cairo_array_num_elements(&font->output);
	for(i = 0; i < font->num_glyphs; i++) {
		index = font->glyphs[i].parent_index;
		if(be16_to_cpu(header.index_to_loc_format) == 0) {
			begin = be16_to_cpu(u.short_offsets[index]) * 2;
			end = be16_to_cpu(u.short_offsets[index + 1]) * 2;
		}
		else {
			begin = be32_to_cpu(u.long_offsets[index]);
			end = be32_to_cpu(u.long_offsets[index + 1]);
		}

		/* quick sanity check... */
		if(end < begin) {
			status = CAIRO_INT_STATUS_UNSUPPORTED;
			goto FAIL;
		}

		size = end - begin;
		status = cairo_truetype_font_align_output(font, &next);
		if(UNLIKELY(status))
			goto FAIL;

		status = cairo_truetype_font_check_boundary(font, next);
		if(UNLIKELY(status))
			goto FAIL;

		font->glyphs[i].location = next - start_offset;

		status = cairo_truetype_font_allocate_write_buffer(font, size, &buffer);
		if(UNLIKELY(status))
			goto FAIL;

		if(size > 1) {
			tt_glyph_data_t * glyph_data;
			int num_contours;
			status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, TT_TAG_glyf, begin, buffer, &size);
			if(UNLIKELY(status))
				goto FAIL;
			glyph_data = (tt_glyph_data_t*)buffer;
			num_contours = (int16)be16_to_cpu(glyph_data->num_contours);
			if(num_contours < 0) {
				status = cairo_truetype_font_remap_composite_glyph(font, buffer, size);
				if(UNLIKELY(status))
					goto FAIL;
			}
			else if(num_contours == 0) {
				/* num_contours == 0 is undefined in the Opentype
				 * spec. There are some embedded fonts that have a
				 * space glyph with num_contours = 0 that fails on
				 * some printers. The spec requires glyphs without
				 * contours to have a 0 size glyph entry in the loca
				 * table.
				 *
				 * If num_contours == 0, truncate the glyph to 0 size.
				 */
				_cairo_array_truncate(&font->output, _cairo_array_num_elements(&font->output) - size);
			}
		}
	}

	status = cairo_truetype_font_align_output(font, &next);
	if(UNLIKELY(status))
		goto FAIL;

	font->glyphs[i].location = next - start_offset;

	status = font->status;
FAIL:
	SAlloc::F(u.bytes);

	return _cairo_truetype_font_set_error(font, status);
}

static cairo_status_t cairo_truetype_font_write_head_table(cairo_truetype_font_t * font, ulong tag)
{
	uchar * buffer;
	ulong size;
	cairo_status_t status;
	if(font->status)
		return font->status;
	size = 0;
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
		tag, 0, NULL, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	font->checksum_index = _cairo_array_num_elements(&font->output) + 8;
	status = cairo_truetype_font_allocate_write_buffer(font, size, &buffer);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, tag, 0, buffer, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	/* set checkSumAdjustment to 0 for table checksum calculation */
	*(uint32 *)(buffer + 8) = 0;
	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_write_hhea_table(cairo_truetype_font_t * font, ulong tag)
{
	tt_hhea_t * hhea;
	ulong size;
	cairo_status_t status;
	if(font->status)
		return font->status;
	size = sizeof(tt_hhea_t);
	status = cairo_truetype_font_allocate_write_buffer(font, size, (uchar **)&hhea);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, tag, 0, (uchar *)hhea, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	hhea->num_hmetrics = cpu_to_be16((uint16)(font->num_glyphs));
	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_write_hmtx_table(cairo_truetype_font_t * font, ulong tag)
{
	ulong size;
	ulong long_entry_size;
	ulong short_entry_size;
	short * p;
	uint i;
	tt_hhea_t hhea;
	int num_hmetrics;
	cairo_status_t status;
	if(font->status)
		return font->status;
	size = sizeof(tt_hhea_t);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
		TT_TAG_hhea, 0,
		(uchar *)&hhea, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);

	num_hmetrics = be16_to_cpu(hhea.num_hmetrics);

	for(i = 0; i < font->num_glyphs; i++) {
		long_entry_size = 2 * sizeof(int16);
		short_entry_size = sizeof(int16);
		status = cairo_truetype_font_allocate_write_buffer(font,
			long_entry_size,
			(uchar **)&p);
		if(UNLIKELY(status))
			return _cairo_truetype_font_set_error(font, status);

		if(font->glyphs[i].parent_index < num_hmetrics) {
			status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
				TT_TAG_hmtx,
				font->glyphs[i].parent_index * long_entry_size,
				(uchar *)p, &long_entry_size);
			if(UNLIKELY(status))
				return _cairo_truetype_font_set_error(font, status);
		}
		else {
			status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
				TT_TAG_hmtx,
				(num_hmetrics - 1) * long_entry_size,
				(uchar *)p, &short_entry_size);
			if(UNLIKELY(status))
				return _cairo_truetype_font_set_error(font, status);

			status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font,
				TT_TAG_hmtx,
				num_hmetrics * long_entry_size +
				(font->glyphs[i].parent_index - num_hmetrics) * short_entry_size,
				(uchar *)(p + 1), &short_entry_size);
			if(UNLIKELY(status))
				return _cairo_truetype_font_set_error(font, status);
		}
		font->widths[i] = be16_to_cpu(p[0]);
	}

	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_write_loca_table(cairo_truetype_font_t * font, ulong tag)
{
	uint i;
	tt_head_t header;
	ulong size;
	cairo_status_t status;
	if(font->status)
		return font->status;
	size = sizeof(tt_head_t);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, TT_TAG_head, 0, reinterpret_cast<uchar *>(&header), &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);

	if(be16_to_cpu(header.index_to_loc_format) == 0) {
		for(i = 0; i < font->num_glyphs + 1; i++)
			cairo_truetype_font_write_be16(font, static_cast<uint16>(font->glyphs[i].location / 2));
	}
	else {
		for(i = 0; i < font->num_glyphs + 1; i++)
			cairo_truetype_font_write_be32(font, font->glyphs[i].location);
	}
	return font->status;
}

static cairo_status_t cairo_truetype_font_write_maxp_table(cairo_truetype_font_t * font, ulong tag)
{
	tt_maxp_t * maxp;
	ulong size;
	cairo_status_t status;
	if(font->status)
		return font->status;
	size = sizeof(tt_maxp_t);
	status = cairo_truetype_font_allocate_write_buffer(font, size, (uchar **)&maxp);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	status = font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, tag, 0, (uchar *)maxp, &size);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	maxp->num_glyphs = cpu_to_be16(font->num_glyphs);
	return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t cairo_truetype_font_write_offset_table(cairo_truetype_font_t * font)
{
	cairo_status_t status;
	uchar * table_buffer;
	size_t table_buffer_length;
	unsigned short search_range, entry_selector, range_shift;
	if(font->status)
		return font->status;
	search_range = 1;
	entry_selector = 0;
	while(search_range * 2 <= font->num_tables) {
		search_range *= 2;
		entry_selector++;
	}
	search_range *= 16;
	range_shift = font->num_tables * 16 - search_range;
	cairo_truetype_font_write_be32(font, SFNT_VERSION);
	cairo_truetype_font_write_be16(font, font->num_tables);
	cairo_truetype_font_write_be16(font, search_range);
	cairo_truetype_font_write_be16(font, entry_selector);
	cairo_truetype_font_write_be16(font, range_shift);
	/* Allocate space for the table directory. Each directory entry
	 * will be filled in by cairo_truetype_font_update_entry() after
	 * the table is written. */
	table_buffer_length = font->num_tables * 16;
	status = cairo_truetype_font_allocate_write_buffer(font, table_buffer_length, &table_buffer);
	if(UNLIKELY(status))
		return _cairo_truetype_font_set_error(font, status);
	return CAIRO_STATUS_SUCCESS;
}

static uint32 cairo_truetype_font_calculate_checksum(cairo_truetype_font_t * font, ulong start, ulong end)
{
	uint32 checksum = 0;
	char * data = (char *)_cairo_array_index(&font->output, 0);
	uint32 * p = (uint32 *)(data + start);
	uint32 * padded_end = (uint32 *)(data + ((end + 3) & ~3));
	while(p < padded_end)
		checksum += be32_to_cpu(*p++);
	return checksum;
}

static void cairo_truetype_font_update_entry(cairo_truetype_font_t * font, int index, ulong tag, ulong start, ulong end)
{
	uint32 * entry = (uint32 *)_cairo_array_index(&font->output, 12 + 16 * index);
	entry[0] = cpu_to_be32((uint32)tag);
	entry[1] = cpu_to_be32(cairo_truetype_font_calculate_checksum(font, start, end));
	entry[2] = cpu_to_be32((uint32)start);
	entry[3] = cpu_to_be32((uint32)(end - start));
}

static cairo_status_t cairo_truetype_font_generate(cairo_truetype_font_t * font, const char  ** data,
    ulong * length, const ulong  ** string_offsets, ulong * num_strings)
{
	cairo_status_t status;
	ulong start, end, next;
	uint32 checksum, * checksum_location;
	int i;
	if(font->status)
		return font->status;
	status = cairo_truetype_font_write_offset_table(font);
	if(UNLIKELY(status))
		goto FAIL;
	status = cairo_truetype_font_align_output(font, &start);
	if(UNLIKELY(status))
		goto FAIL;
	end = 0;
	for(i = 0; i < font->num_tables; i++) {
		status = font->truetype_tables[i].write(font, font->truetype_tables[i].tag);
		if(UNLIKELY(status))
			goto FAIL;
		end = _cairo_array_num_elements(&font->output);
		status = cairo_truetype_font_align_output(font, &next);
		if(UNLIKELY(status))
			goto FAIL;
		cairo_truetype_font_update_entry(font, font->truetype_tables[i].pos, font->truetype_tables[i].tag, start, end);
		status = cairo_truetype_font_check_boundary(font, next);
		if(UNLIKELY(status))
			goto FAIL;

		start = next;
	}
	checksum = 0xb1b0afba - cairo_truetype_font_calculate_checksum(font, 0, end);
	checksum_location = (uint32 *)_cairo_array_index(&font->output, font->checksum_index);
	*checksum_location = cpu_to_be32(checksum);
	*data = (const char *)_cairo_array_index(&font->output, 0);
	*length = _cairo_array_num_elements(&font->output);
	*num_strings = _cairo_array_num_elements(&font->string_offsets);
	if(*num_strings != 0)
		*string_offsets = (const ulong *)_cairo_array_index(&font->string_offsets, 0);
	else
		*string_offsets = NULL;
FAIL:
	return _cairo_truetype_font_set_error(font, status);
}

static cairo_status_t cairo_truetype_font_use_glyph(cairo_truetype_font_t * font, unsigned short glyph, unsigned short * out)
{
	if(glyph >= font->base.num_glyphs_in_face)
		return CAIRO_INT_STATUS_UNSUPPORTED;
	if(font->parent_to_subset[glyph] == 0) {
		font->parent_to_subset[glyph] = font->num_glyphs;
		font->glyphs[font->num_glyphs].parent_index = glyph;
		font->num_glyphs++;
	}
	*out = font->parent_to_subset[glyph];
	return CAIRO_STATUS_SUCCESS;
}

static void FASTCALL cairo_truetype_font_add_truetype_table(cairo_truetype_font_t * font, ulong tag, cairo_status_t (*write)(cairo_truetype_font_t * font, ulong tag), int pos)
{
	font->truetype_tables[font->num_tables].tag = tag;
	font->truetype_tables[font->num_tables].write = write;
	font->truetype_tables[font->num_tables].pos = pos;
	font->num_tables++;
}

/* cairo_truetype_font_create_truetype_table_list() builds the list of
 * truetype tables to be embedded in the subsetted font. Each call to
 * cairo_truetype_font_add_truetype_table() adds a table, the callback
 * for generating the table, and the position in the table directory
 * to the truetype_tables array.
 *
 * As we write out the glyf table we remap composite glyphs.
 * Remapping composite glyphs will reference the sub glyphs the
 * composite glyph is made up of. The "glyf" table callback needs to
 * be called first so we have all the glyphs in the subset before
 * going further.
 *
 * The order in which tables are added to the truetype_table array
 * using cairo_truetype_font_add_truetype_table() specifies the order
 * in which the callback functions will be called.
 *
 * The tables in the table directory must be listed in alphabetical
 * order.  The "cvt", "fpgm", and "prep" are optional tables. They
 * will only be embedded in the subset if they exist in the source
 * font. "cmap" is only embedded for latin fonts. The pos parameter of
 * cairo_truetype_font_add_truetype_table() specifies the position of
 * the table in the table directory.
 */
static void cairo_truetype_font_create_truetype_table_list(cairo_truetype_font_t * font)
{
	boolint has_cvt = FALSE;
	boolint has_fpgm = FALSE;
	boolint has_prep = FALSE;
	int pos;
	ulong size = 0;
	if(font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, TT_TAG_cvt, 0, NULL, &size) == CAIRO_INT_STATUS_SUCCESS)
		has_cvt = TRUE;
	size = 0;
	if(font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, TT_TAG_fpgm, 0, NULL, &size) == CAIRO_INT_STATUS_SUCCESS)
		has_fpgm = TRUE;
	size = 0;
	if(font->backend->load_truetype_table(font->scaled_font_subset->scaled_font, TT_TAG_prep, 0, NULL, &size) == CAIRO_INT_STATUS_SUCCESS)
		has_prep = TRUE;
	font->num_tables = 0;
	pos = 0;
	if(font->is_pdf && font->scaled_font_subset->is_latin)
		pos++;
	if(has_cvt)
		pos++;
	if(has_fpgm)
		pos++;
	cairo_truetype_font_add_truetype_table(font, TT_TAG_glyf, cairo_truetype_font_write_glyf_table, pos);
	pos = 0;
	if(font->is_pdf && font->scaled_font_subset->is_latin)
		cairo_truetype_font_add_truetype_table(font, TT_TAG_cmap, cairo_truetype_font_write_cmap_table, pos++);
	if(has_cvt)
		cairo_truetype_font_add_truetype_table(font, TT_TAG_cvt, cairo_truetype_font_write_generic_table, pos++);
	if(has_fpgm)
		cairo_truetype_font_add_truetype_table(font, TT_TAG_fpgm, cairo_truetype_font_write_generic_table, pos++);
	pos++;
	cairo_truetype_font_add_truetype_table(font, TT_TAG_head, cairo_truetype_font_write_head_table, pos++);
	cairo_truetype_font_add_truetype_table(font, TT_TAG_hhea, cairo_truetype_font_write_hhea_table, pos++);
	cairo_truetype_font_add_truetype_table(font, TT_TAG_hmtx, cairo_truetype_font_write_hmtx_table, pos++);
	cairo_truetype_font_add_truetype_table(font, TT_TAG_loca, cairo_truetype_font_write_loca_table, pos++);
	cairo_truetype_font_add_truetype_table(font, TT_TAG_maxp, cairo_truetype_font_write_maxp_table, pos++);
	if(has_prep)
		cairo_truetype_font_add_truetype_table(font, TT_TAG_prep, cairo_truetype_font_write_generic_table, pos);
}

static cairo_status_t cairo_truetype_subset_init_internal(cairo_truetype_subset_t * truetype_subset, cairo_scaled_font_subset_t * font_subset, boolint is_pdf)
{
	cairo_truetype_font_t * font = NULL;
	const char * data = NULL; /* squelch bogus compiler warning */
	ulong length = 0; /* squelch bogus compiler warning */
	ulong offsets_length;
	uint i;
	const ulong * string_offsets = NULL;
	ulong num_strings = 0;
	cairo_status_t status = _cairo_truetype_font_create(font_subset, is_pdf, &font);
	if(UNLIKELY(status))
		return status;
	for(i = 0; i < font->scaled_font_subset->num_glyphs; i++) {
		ushort parent_glyph = static_cast<ushort>(font->scaled_font_subset->glyphs[i]);
		status = cairo_truetype_font_use_glyph(font, parent_glyph, &parent_glyph);
		if(UNLIKELY(status))
			goto fail1;
	}
	cairo_truetype_font_create_truetype_table_list(font);
	status = cairo_truetype_font_generate(font, &data, &length, &string_offsets, &num_strings);
	if(UNLIKELY(status))
		goto fail1;
	truetype_subset->ps_name = sstrdup(font->base.ps_name);
	if(UNLIKELY(truetype_subset->ps_name == NULL)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto fail1;
	}
	if(font->base.font_name != NULL) {
		truetype_subset->family_name_utf8 = sstrdup(font->base.font_name);
		if(UNLIKELY(truetype_subset->family_name_utf8 == NULL)) {
			status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
			goto fail2;
		}
	}
	else {
		truetype_subset->family_name_utf8 = NULL;
	}
	/* The widths array returned must contain only widths for the
	 * glyphs in font_subset. Any subglyphs appended after
	 * font_subset->num_glyphs are omitted. */
	truetype_subset->widths = static_cast<double *>(SAlloc::C(sizeof(double), font->scaled_font_subset->num_glyphs));
	if(UNLIKELY(truetype_subset->widths == NULL)) {
		status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
		goto fail3;
	}
	for(i = 0; i < font->scaled_font_subset->num_glyphs; i++)
		truetype_subset->widths[i] = (double)font->widths[i]/font->base.units_per_em;
	truetype_subset->x_min = (double)font->base.x_min/font->base.units_per_em;
	truetype_subset->y_min = (double)font->base.y_min/font->base.units_per_em;
	truetype_subset->x_max = (double)font->base.x_max/font->base.units_per_em;
	truetype_subset->y_max = (double)font->base.y_max/font->base.units_per_em;
	truetype_subset->ascent = (double)font->base.ascent/font->base.units_per_em;
	truetype_subset->descent = (double)font->base.descent/font->base.units_per_em;
	if(length) {
		truetype_subset->data = static_cast<uchar *>(SAlloc::M_zon0(length));
		if(UNLIKELY(truetype_subset->data == NULL)) {
			status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
			goto fail4;
		}
		memcpy(truetype_subset->data, data, length);
	}
	else
		truetype_subset->data = NULL;
	truetype_subset->data_length = length;
	if(num_strings) {
		offsets_length = num_strings * sizeof(ulong);
		truetype_subset->string_offsets = (ulong *)SAlloc::M_zon0(offsets_length);
		if(UNLIKELY(truetype_subset->string_offsets == NULL)) {
			status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
			goto fail5;
		}
		memcpy(truetype_subset->string_offsets, string_offsets, offsets_length);
		truetype_subset->num_string_offsets = num_strings;
	}
	else {
		truetype_subset->string_offsets = NULL;
		truetype_subset->num_string_offsets = 0;
	}
	cairo_truetype_font_destroy(font);
	return CAIRO_STATUS_SUCCESS;
fail5:
	SAlloc::F(truetype_subset->data);
fail4:
	SAlloc::F(truetype_subset->widths);
fail3:
	SAlloc::F(truetype_subset->family_name_utf8);
fail2:
	SAlloc::F(truetype_subset->ps_name);
fail1:
	cairo_truetype_font_destroy(font);
	return status;
}

cairo_status_t _cairo_truetype_subset_init_ps(cairo_truetype_subset_t * truetype_subset, cairo_scaled_font_subset_t * font_subset)
{
	return cairo_truetype_subset_init_internal(truetype_subset, font_subset, FALSE);
}

cairo_status_t _cairo_truetype_subset_init_pdf(cairo_truetype_subset_t * truetype_subset, cairo_scaled_font_subset_t * font_subset)
{
	return cairo_truetype_subset_init_internal(truetype_subset, font_subset, TRUE);
}

void FASTCALL _cairo_truetype_subset_fini(cairo_truetype_subset_t * subset)
{
	SAlloc::F(subset->ps_name);
	SAlloc::F(subset->family_name_utf8);
	SAlloc::F(subset->widths);
	SAlloc::F(subset->data);
	SAlloc::F(subset->string_offsets);
}

static cairo_int_status_t _cairo_truetype_reverse_cmap(cairo_scaled_font_t * scaled_font, ulong table_offset, ulong index, uint32 * ucs4)
{
	tt_segment_map_t * map;
	char buf[4];
	uint num_segments, i;
	uint16 * start_code;
	uint16 * end_code;
	uint16 * delta;
	uint16 * range_offset;
	uint16 c;
	const cairo_scaled_font_backend_t * backend = scaled_font->backend;
	ulong size = 4;
	cairo_status_t status = backend->load_truetype_table(scaled_font, TT_TAG_cmap, table_offset, (uchar *)&buf, &size);
	if(UNLIKELY(status))
		return status;
	/* All table formats have the same first two words */
	map = reinterpret_cast<tt_segment_map_t *>(buf);
	if(be16_to_cpu(map->format) != 4)
		return CAIRO_INT_STATUS_UNSUPPORTED;
	size = be16_to_cpu(map->length);
	map = static_cast<tt_segment_map_t *>(SAlloc::M_zon0(size));
	if(UNLIKELY(map == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	status = backend->load_truetype_table(scaled_font, TT_TAG_cmap, table_offset, (uchar *)map, &size);
	if(UNLIKELY(status))
		goto fail;
	num_segments = be16_to_cpu(map->segCountX2)/2;
	// A Format 4 cmap contains 8 uint16 numbers and 4 arrays of uint16 each num_segments long. 
	if(size < (8 + 4*num_segments)*sizeof(uint16))
		return CAIRO_INT_STATUS_UNSUPPORTED;
	end_code = map->endCount;
	start_code = &(end_code[num_segments + 1]);
	delta = &(start_code[num_segments]);
	range_offset = &(delta[num_segments]);
	/* search for glyph in segments with rangeOffset=0 */
	for(i = 0; i < num_segments; i++) {
		uint16 start = be16_to_cpu(start_code[i]);
		uint16 end = be16_to_cpu(end_code[i]);
		if(start == 0xffff && end == 0xffff)
			break;
		c = static_cast<uint16>(index - be16_to_cpu(delta[i]));
		if(range_offset[i] == 0 && c >= start && c <= end) {
			*ucs4 = c;
			goto found;
		}
	}
	/* search for glyph in segments with rangeOffset=1 */
	for(i = 0; i < num_segments; i++) {
		uint16 start = be16_to_cpu(start_code[i]);
		uint16 end = be16_to_cpu(end_code[i]);
		if(start == 0xffff && end == 0xffff)
			break;
		if(range_offset[i] != 0) {
			uint16 * glyph_ids = &range_offset[i] + be16_to_cpu(range_offset[i])/2;
			int range_size = end - start + 1;
			uint16 g_id_be = cpu_to_be16(static_cast<uint16>(index));
			int j;
			if(range_size > 0) {
				if((char *)glyph_ids + 2*range_size > (char *)map + size)
					return CAIRO_INT_STATUS_UNSUPPORTED;
				for(j = 0; j < range_size; j++) {
					if(glyph_ids[j] == g_id_be) {
						*ucs4 = start + j;
						goto found;
					}
				}
			}
		}
	}
	/* glyph not found */
	*ucs4 = -1;
found:
	status = CAIRO_STATUS_SUCCESS;
fail:
	SAlloc::F(map);
	return status;
}

cairo_int_status_t _cairo_truetype_index_to_ucs4(cairo_scaled_font_t * scaled_font, ulong index, uint32 * ucs4)
{
	cairo_int_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;
	tt_cmap_t * cmap;
	char buf[4];
	int num_tables, i;
	ulong size;
	const cairo_scaled_font_backend_t * backend = scaled_font->backend;
	if(!backend->load_truetype_table)
		return CAIRO_INT_STATUS_UNSUPPORTED;
	size = 4;
	status = backend->load_truetype_table(scaled_font, TT_TAG_cmap, 0, (uchar *)&buf, &size);
	if(UNLIKELY(status))
		return status;
	cmap = (tt_cmap_t*)buf;
	num_tables = be16_to_cpu(cmap->num_tables);
	size = 4 + num_tables*sizeof(tt_cmap_index_t);
	cmap = static_cast<tt_cmap_t *>(_cairo_malloc_ab_plus_c(num_tables, sizeof(tt_cmap_index_t), 4));
	if(UNLIKELY(cmap == NULL))
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	status = backend->load_truetype_table(scaled_font, TT_TAG_cmap, 0, (uchar *)cmap, &size);
	if(UNLIKELY(status))
		goto cleanup;
	/* Find a table with Unicode mapping */
	for(i = 0; i < num_tables; i++) {
		if(be16_to_cpu(cmap->index[i].platform) == 3 && be16_to_cpu(cmap->index[i].encoding) == 1) {
			status = _cairo_truetype_reverse_cmap(scaled_font, be32_to_cpu(cmap->index[i].offset), index, ucs4);
			if(status != CAIRO_INT_STATUS_UNSUPPORTED)
				break;
		}
	}
cleanup:
	SAlloc::F(cmap);
	return status;
}
/*
 * Sanity check on font name length as some broken fonts may return very long
 * strings of garbage. 127 is maximum length of a PS name.
 */
#define MAX_FONT_NAME_LENGTH 127

static cairo_status_t find_name(tt_name_t * name, int name_id, int platform, int encoding, int language, char ** str_out)
{
	tt_name_record_t * record;
	int i, len;
	char * p;
	boolint has_tag;
	cairo_status_t status;
	char * str = NULL;
	for(i = 0; i < be16_to_cpu(name->num_records); i++) {
		record = &(name->records[i]);
		if(be16_to_cpu(record->name) == name_id && be16_to_cpu(record->platform) == platform &&
		    be16_to_cpu(record->encoding) == encoding && (language == -1 || be16_to_cpu(record->language) == language)) {
			len = be16_to_cpu(record->length);
			if(platform == 3 && len > MAX_FONT_NAME_LENGTH*2) /* UTF-16 name */
				break;
			if(len > MAX_FONT_NAME_LENGTH)
				break;
			str = static_cast<char *>(SAlloc::M_zon0(len+1));
			if(!str)
				return _cairo_error(CAIRO_STATUS_NO_MEMORY);
			memcpy(str, ((char *)name) + be16_to_cpu(name->strings_offset) + be16_to_cpu(record->offset), len);
			str[be16_to_cpu(record->length)] = 0;
			break;
		}
	}
	if(!str) {
		*str_out = NULL;
		return CAIRO_STATUS_SUCCESS;
	}
	if(platform == 3) { /* Win platform, unicode encoding */
		/* convert to utf8 */
		int size = 0;
		char * utf8;
		uint16 * u = (uint16 *)str;
		int u_len = len/2;
		for(i = 0; i < u_len; i++)
			size += _cairo_ucs4_to_utf8(be16_to_cpu(u[i]), NULL);

		utf8 = static_cast<char *>(SAlloc::M_zon0(size + 1));
		if(!utf8) {
			status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
			goto fail;
		}
		p = utf8;
		for(i = 0; i < u_len; i++)
			p += _cairo_ucs4_to_utf8(be16_to_cpu(u[i]), p);
		*p = 0;
		SAlloc::F(str);
		str = utf8;
	}
	else if(platform == 1) { /* Mac platform, Mac Roman encoding */
		/* Replace characters above 127 with underscores. We could use
		 * a lookup table to convert to unicode but since most fonts
		 * include a unicode name this is just a rarely used fallback. */
		for(i = 0; i < len; i++) {
			if((uchar)str[i] > 127)
				str[i] = '_';
		}
	}

	/* If font name is prefixed with a PDF subset tag, strip it off. */
	p = str;
	len = strlen(str);
	has_tag = FALSE;
	if(len > 7 && p[6] == '+') {
		has_tag = TRUE;
		for(i = 0; i < 6; i++) {
			if(p[i] < 'A' || p[i] > 'Z') {
				has_tag = FALSE;
				break;
			}
		}
	}
	if(has_tag) {
		p = static_cast<char *>(SAlloc::M_zon0(len - 6));
		if(UNLIKELY(p == NULL)) {
			status = _cairo_error(CAIRO_STATUS_NO_MEMORY);
			goto fail;
		}
		memcpy(p, str + 7, len - 7);
		p[len-7] = 0;
		SAlloc::F(str);
		str = p;
	}
	*str_out = str;
	return CAIRO_STATUS_SUCCESS;
fail:
	SAlloc::F(str);
	return status;
}

cairo_int_status_t _cairo_truetype_read_font_name(cairo_scaled_font_t * scaled_font, char ** ps_name_out, char ** font_name_out)
{
	cairo_status_t status;
	tt_name_t * name;
	ulong size;
	char * ps_name = NULL;
	char * family_name = NULL;
	const cairo_scaled_font_backend_t * backend = scaled_font->backend;
	if(!backend->load_truetype_table)
		return CAIRO_INT_STATUS_UNSUPPORTED;
	size = 0;
	status = backend->load_truetype_table(scaled_font, TT_TAG_name, 0, NULL, &size);
	if(status)
		return status;
	name = (tt_name_t *)SAlloc::M_zon0(size);
	if(!name)
		return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	status = backend->load_truetype_table(scaled_font, TT_TAG_name, 0, (uchar *)name, &size);
	if(status)
		goto fail;
	/* Find PS Name (name_id = 6). OT spec says PS name must be one of
	 * the following two encodings */
	status = find_name(name, 6, 3, 1, 0x409, &ps_name); /* win, unicode, english-us */
	if(UNLIKELY(status))
		goto fail;
	if(!ps_name) {
		status = find_name(name, 6, 1, 0, 0, &ps_name); /* mac, roman, english */
		if(UNLIKELY(status))
			goto fail;
	}
	/* Find Family name (name_id = 1) */
	status = find_name(name, 1, 3, 1, 0x409, &family_name); /* win, unicode, english-us */
	if(UNLIKELY(status))
		goto fail;

	if(!family_name) {
		status = find_name(name, 1, 3, 0, 0x409, &family_name); /* win, symbol, english-us */
		if(UNLIKELY(status))
			goto fail;
	}
	if(!family_name) {
		status = find_name(name, 1, 1, 0, 0, &family_name); /* mac, roman, english */
		if(UNLIKELY(status))
			goto fail;
	}
	if(!family_name) {
		status = find_name(name, 1, 3, 1, -1, &family_name); /* win, unicode, any language */
		if(UNLIKELY(status))
			goto fail;
	}
	status = _cairo_escape_ps_name(&ps_name);
	if(UNLIKELY(status))
		goto fail;
	SAlloc::F(name);
	*ps_name_out = ps_name;
	*font_name_out = family_name;
	return CAIRO_STATUS_SUCCESS;
fail:
	SAlloc::F(name);
	SAlloc::F(ps_name);
	SAlloc::F(family_name);
	*ps_name_out = NULL;
	*font_name_out = NULL;

	return status;
}

cairo_int_status_t _cairo_truetype_get_style(cairo_scaled_font_t * scaled_font,
    int * weight,
    boolint * bold,
    boolint * italic)
{
	cairo_status_t status;
	const cairo_scaled_font_backend_t * backend;
	tt_os2_t os2;
	ulong size;
	uint16 selection;

	backend = scaled_font->backend;
	if(!backend->load_truetype_table)
		return CAIRO_INT_STATUS_UNSUPPORTED;

	size = 0;
	status = backend->load_truetype_table(scaled_font,
		TT_TAG_OS2, 0,
		NULL,
		&size);
	if(status)
		return status;

	if(size < sizeof(os2))
		return CAIRO_INT_STATUS_UNSUPPORTED;

	size = sizeof(os2);
	status = backend->load_truetype_table(scaled_font,
		TT_TAG_OS2, 0,
		(uchar *)&os2,
		&size);
	if(status)
		return status;

	*weight = be16_to_cpu(os2.usWeightClass);
	selection = be16_to_cpu(os2.fsSelection);
	*bold = (selection & TT_FS_SELECTION_BOLD) ? TRUE : FALSE;
	*italic = (selection & TT_FS_SELECTION_ITALIC) ? TRUE : FALSE;

	return CAIRO_STATUS_SUCCESS;
}

#endif /* CAIRO_HAS_FONT_SUBSET */
