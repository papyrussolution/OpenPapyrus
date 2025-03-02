/*
 * Copyright © 1998-2004  David Turner and Werner Lemberg
 * Copyright © 2006  Behdad Esfahbod
 * Copyright © 2007,2008,2009  Red Hat, Inc.
 * Copyright © 2012,2013  Google, Inc.
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
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifndef HB_NO_OT_LAYOUT
#ifdef HB_NO_OT_TAG
	#error "Cannot compile hb-ot-layout.cc with HB_NO_OT_TAG."
#endif
/**
 * SECTION:hb-ot-layout
 * @title: hb-ot-layout
 * @short_description: OpenType Layout
 * @include: hb-ot.h
 *
 * Functions for querying OpenType Layout features in the font face.
 **/

/*
 * kern
 */
#ifndef HB_NO_OT_KERN
/**
 * hb_ot_layout_has_kerning:
 * @face: The #hb_face_t to work on
 *
 * Tests whether a face includes any kerning data in the 'kern' table.
 * Does NOT test for kerning lookups in the GPOS table.
 *
 * Return value: true if data found, false otherwise
 *
 **/
bool hb_ot_layout_has_kerning(const hb_face_t * face) { return face->table.kern->has_data(); }
/**
 * hb_ot_layout_has_machine_kerning:
 * @face: The #hb_face_t to work on
 *
 * Tests whether a face includes any state-machine kerning in the 'kern' table.
 * Does NOT examine the GPOS table.
 *
 * Return value: true if data found, false otherwise
 *
 **/
bool hb_ot_layout_has_machine_kerning(hb_face_t * face) { return face->table.kern->has_state_machine(); }
/**
 * hb_ot_layout_has_cross_kerning:
 * @face: The #hb_face_t to work on
 *
 * Tests whether a face has any cross-stream kerning (i.e., kerns
 * that make adjustments perpendicular to the direction of the text
 * flow: Y adjustments in horizontal text or X adjustments in
 * vertical text) in the 'kern' table.
 *
 * Does NOT examine the GPOS table.
 *
 * Return value: true is data found, false otherwise
 *
 **/
bool hb_ot_layout_has_cross_kerning(hb_face_t * face) { return face->table.kern->has_cross_stream(); }

void hb_ot_layout_kern(const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer)
{
	hb_blob_t * blob = font->face->table.kern.get_blob();
	const AAT::kern& kern = *blob->as<AAT::kern> ();
	AAT::hb_aat_apply_context_t c(plan, font, buffer, blob);
	kern.apply(&c);
}

#endif

/*
 * GDEF
 */
bool OT::GDEF::is_blocklisted(hb_blob_t *blob, hb_face_t *face) const
{
#ifdef HB_NO_OT_LAYOUT_BLACKLIST
	return false;
#endif
	/* The ugly business of blocklisting individual fonts' tables happen here!
	 * See this thread for why we finally had to bend in and do this:
	 * https://lists.freedesktop.org/archives/harfbuzz/2016-February/005489.html
	 *
	 * In certain versions of Times New Roman Italic and Bold Italic,
	 * ASCII double quotation mark U+0022 has wrong glyph class 3 (mark)
	 * in GDEF.  Many versions of Tahoma have bad GDEF tables that
	 * incorrectly classify some spacing marks such as certain IPA
	 * symbols as glyph class 3. So do older versions of Microsoft
	 * Himalaya, and the version of Cantarell shipped by Ubuntu 16.04.
	 *
	 * Nuke the GDEF tables of to avoid unwanted width-zeroing.
	 *
	 * See https://bugzilla.mozilla.org/show_bug.cgi?id=1279925
	 *     https://bugzilla.mozilla.org/show_bug.cgi?id=1279693
	 *     https://bugzilla.mozilla.org/show_bug.cgi?id=1279875
	 */
	switch HB_CODEPOINT_ENCODE3(blob->length, face->table.GSUB->table.get_length(), face->table.GPOS->table.get_length()) {
		/* sha1sum:c5ee92f0bca4bfb7d06c4d03e8cf9f9cf75d2e8a Windows 7? timesi.ttf */
		case HB_CODEPOINT_ENCODE3(442, 2874, 42038):
		/* sha1sum:37fc8c16a0894ab7b749e35579856c73c840867b Windows 7? timesbi.ttf */
		case HB_CODEPOINT_ENCODE3(430, 2874, 40662):
		/* sha1sum:19fc45110ea6cd3cdd0a5faca256a3797a069a80 Windows 7 timesi.ttf */
		case HB_CODEPOINT_ENCODE3(442, 2874, 39116):
		/* sha1sum:6d2d3c9ed5b7de87bc84eae0df95ee5232ecde26 Windows 7 timesbi.ttf */
		case HB_CODEPOINT_ENCODE3(430, 2874, 39374):
		/* sha1sum:8583225a8b49667c077b3525333f84af08c6bcd8 OS X 10.11.3 Times New Roman Italic.ttf */
		case HB_CODEPOINT_ENCODE3(490, 3046, 41638):
		/* sha1sum:ec0f5a8751845355b7c3271d11f9918a966cb8c9 OS X 10.11.3 Times New Roman Bold Italic.ttf */
		case HB_CODEPOINT_ENCODE3(478, 3046, 41902):
		/* sha1sum:96eda93f7d33e79962451c6c39a6b51ee893ce8c  tahoma.ttf from Windows 8 */
		case HB_CODEPOINT_ENCODE3(898, 12554, 46470):
		/* sha1sum:20928dc06014e0cd120b6fc942d0c3b1a46ac2bc  tahomabd.ttf from Windows 8 */
		case HB_CODEPOINT_ENCODE3(910, 12566, 47732):
		/* sha1sum:4f95b7e4878f60fa3a39ca269618dfde9721a79e  tahoma.ttf from Windows 8.1 */
		case HB_CODEPOINT_ENCODE3(928, 23298, 59332):
		/* sha1sum:6d400781948517c3c0441ba42acb309584b73033  tahomabd.ttf from Windows 8.1 */
		case HB_CODEPOINT_ENCODE3(940, 23310, 60732):
		/* tahoma.ttf v6.04 from Windows 8.1 x64, see https://bugzilla.mozilla.org/show_bug.cgi?id=1279925 */
		case HB_CODEPOINT_ENCODE3(964, 23836, 60072):
		/* tahomabd.ttf v6.04 from Windows 8.1 x64, see https://bugzilla.mozilla.org/show_bug.cgi?id=1279925 */
		case HB_CODEPOINT_ENCODE3(976, 23832, 61456):
		/* sha1sum:e55fa2dfe957a9f7ec26be516a0e30b0c925f846  tahoma.ttf from Windows 10 */
		case HB_CODEPOINT_ENCODE3(994, 24474, 60336):
		/* sha1sum:7199385abb4c2cc81c83a151a7599b6368e92343  tahomabd.ttf from Windows 10 */
		case HB_CODEPOINT_ENCODE3(1006, 24470, 61740):
		/* tahoma.ttf v6.91 from Windows 10 x64, see https://bugzilla.mozilla.org/show_bug.cgi?id=1279925 */
		case HB_CODEPOINT_ENCODE3(1006, 24576, 61346):
		/* tahomabd.ttf v6.91 from Windows 10 x64, see https://bugzilla.mozilla.org/show_bug.cgi?id=1279925 */
		case HB_CODEPOINT_ENCODE3(1018, 24572, 62828):
		/* sha1sum:b9c84d820c49850d3d27ec498be93955b82772b5  tahoma.ttf from Windows 10 AU */
		case HB_CODEPOINT_ENCODE3(1006, 24576, 61352):
		/* sha1sum:2bdfaab28174bdadd2f3d4200a30a7ae31db79d2  tahomabd.ttf from Windows 10 AU */
		case HB_CODEPOINT_ENCODE3(1018, 24572, 62834):
		/* sha1sum:b0d36cf5a2fbe746a3dd277bffc6756a820807a7  Tahoma.ttf from Mac OS X 10.9 */
		case HB_CODEPOINT_ENCODE3(832, 7324, 47162):
		/* sha1sum:12fc4538e84d461771b30c18b5eb6bd434e30fba  Tahoma Bold.ttf from Mac OS X 10.9 */
		case HB_CODEPOINT_ENCODE3(844, 7302, 45474):
		/* sha1sum:eb8afadd28e9cf963e886b23a30b44ab4fd83acc  himalaya.ttf from Windows 7 */
		case HB_CODEPOINT_ENCODE3(180, 13054, 7254):
		/* sha1sum:73da7f025b238a3f737aa1fde22577a6370f77b0  himalaya.ttf from Windows 8 */
		case HB_CODEPOINT_ENCODE3(192, 12638, 7254):
		/* sha1sum:6e80fd1c0b059bbee49272401583160dc1e6a427  himalaya.ttf from Windows 8.1 */
		case HB_CODEPOINT_ENCODE3(192, 12690, 7254):
		/* 8d9267aea9cd2c852ecfb9f12a6e834bfaeafe44  cantarell-fonts-0.0.21/otf/Cantarell-Regular.otf */
		/* 983988ff7b47439ab79aeaf9a45bd4a2c5b9d371  cantarell-fonts-0.0.21/otf/Cantarell-Oblique.otf */
		case HB_CODEPOINT_ENCODE3(188, 248, 3852):
		/* 2c0c90c6f6087ffbfea76589c93113a9cbb0e75f  cantarell-fonts-0.0.21/otf/Cantarell-Bold.otf */
		/* 55461f5b853c6da88069ffcdf7f4dd3f8d7e3e6b  cantarell-fonts-0.0.21/otf/Cantarell-Bold-Oblique.otf */
		case HB_CODEPOINT_ENCODE3(188, 264, 3426):
		/* d125afa82a77a6475ac0e74e7c207914af84b37a padauk-2.80/Padauk.ttf RHEL 7.2 */
		case HB_CODEPOINT_ENCODE3(1058, 47032, 11818):
		/* 0f7b80437227b90a577cc078c0216160ae61b031 padauk-2.80/Padauk-Bold.ttf RHEL 7.2*/
		case HB_CODEPOINT_ENCODE3(1046, 47030, 12600):
		/* d3dde9aa0a6b7f8f6a89ef1002e9aaa11b882290 padauk-2.80/Padauk.ttf Ubuntu 16.04 */
		case HB_CODEPOINT_ENCODE3(1058, 71796, 16770):
		/* 5f3c98ccccae8a953be2d122c1b3a77fd805093f padauk-2.80/Padauk-Bold.ttf Ubuntu 16.04 */
		case HB_CODEPOINT_ENCODE3(1046, 71790, 17862):
		/* 6c93b63b64e8b2c93f5e824e78caca555dc887c7 padauk-2.80/Padauk-book.ttf */
		case HB_CODEPOINT_ENCODE3(1046, 71788, 17112):
		/* d89b1664058359b8ec82e35d3531931125991fb9 padauk-2.80/Padauk-bookbold.ttf */
		case HB_CODEPOINT_ENCODE3(1058, 71794, 17514):
		/* 824cfd193aaf6234b2b4dc0cf3c6ef576c0d00ef padauk-3.0/Padauk-book.ttf */
		case HB_CODEPOINT_ENCODE3(1330, 109904, 57938):
		/* 91fcc10cf15e012d27571e075b3b4dfe31754a8a padauk-3.0/Padauk-bookbold.ttf */
		case HB_CODEPOINT_ENCODE3(1330, 109904, 58972):
		/* sha1sum: c26e41d567ed821bed997e937bc0c41435689e85  Padauk.ttf
		 *  "Padauk Regular" "Version 2.5", see https://crbug.com/681813 */
		case HB_CODEPOINT_ENCODE3(1004, 59092, 14836):
		    return true;
	}
	return false;
}

static void _hb_ot_layout_set_glyph_props(hb_font_t * font, hb_buffer_t * buffer)
{
	_hb_buffer_assert_gsubgpos_vars(buffer);
	const OT::GDEF &gdef = *font->face->table.GDEF->table;
	uint count = buffer->len;
	for(uint i = 0; i < count; i++) {
		_hb_glyph_info_set_glyph_props(&buffer->info[i], gdef.get_glyph_props(buffer->info[i].codepoint));
		_hb_glyph_info_clear_lig_props(&buffer->info[i]);
		buffer->info[i].syllable() = 0;
	}
}

/* Public API */

/**
 * hb_ot_layout_has_glyph_classes:
 * @face: #hb_face_t to work upon
 *
 * Tests whether a face has any glyph classes defined in its GDEF table.
 *
 * Return value: true if data found, false otherwise
 *
 **/
hb_bool_t hb_ot_layout_has_glyph_classes(hb_face_t * face) { return face->table.GDEF->table->has_glyph_classes(); }
/**
 * hb_ot_layout_get_glyph_class:
 * @face: The #hb_face_t to work on
 * @glyph: The #hb_codepoint_t code point to query
 *
 * Fetches the GDEF class of the requested glyph in the specified face.
 *
 * Return value: The #hb_ot_layout_glyph_class_t glyph class of the given code
 * point in the GDEF table of the face.
 *
 * Since: 0.9.7
 **/
hb_ot_layout_glyph_class_t hb_ot_layout_get_glyph_class(hb_face_t * face, hb_codepoint_t glyph)
{
	return (hb_ot_layout_glyph_class_t)face->table.GDEF->table->get_glyph_class(glyph);
}
/**
 * hb_ot_layout_get_glyphs_in_class:
 * @face: The #hb_face_t to work on
 * @klass: The #hb_ot_layout_glyph_class_t GDEF class to retrieve
 * @glyphs: (out): The #hb_set_t set of all glyphs belonging to the requested
 *    class.
 *
 * Retrieves the set of all glyphs from the face that belong to the requested
 * glyph class in the face's GDEF table.
 *
 * Since: 0.9.7
 **/
void hb_ot_layout_get_glyphs_in_class(hb_face_t * face, hb_ot_layout_glyph_class_t klass, hb_set_t * glyphs /*OUT*/)
{
	return face->table.GDEF->table->get_glyphs_in_class(klass, glyphs);
}
#ifndef HB_NO_LAYOUT_UNUSED
/**
 * hb_ot_layout_get_attach_points:
 * @face: The #hb_face_t to work on
 * @glyph: The #hb_codepoint_t code point to query
 * @start_offset: offset of the first attachment point to retrieve
 * @point_count: (inout) (allow-none): Input = the maximum number of attachment points to return;
 *  Output = the actual number of attachment points returned (may be zero)
 * @point_array: (out) (array length=point_count): The array of attachment points found for the query
 *
 * Fetches a list of all attachment points for the specified glyph in the GDEF
 * table of the face. The list returned will begin at the offset provided.
 *
 * Useful if the client program wishes to cache the list.
 *
 **/
uint hb_ot_layout_get_attach_points(hb_face_t * face, hb_codepoint_t glyph, uint start_offset, uint * point_count /* IN/OUT */, uint * point_array /*OUT*/)
{
	return face->table.GDEF->table->get_attach_points(glyph, start_offset, point_count, point_array);
}
/**
 * hb_ot_layout_get_ligature_carets:
 * @font: The #hb_font_t to work on
 * @direction: The #hb_direction_t text direction to use
 * @glyph: The #hb_codepoint_t code point to query
 * @start_offset: offset of the first caret position to retrieve
 * @caret_count: (inout) (allow-none): Input = the maximum number of caret positions to return;
 *  Output = the actual number of caret positions returned (may be zero)
 * @caret_array: (out) (array length=caret_count): The array of caret positions found for the query
 *
 * Fetches a list of the caret positions defined for a ligature glyph in the GDEF
 * table of the font. The list returned will begin at the offset provided.
 *
 **/
uint hb_ot_layout_get_ligature_carets(hb_font_t * font, hb_direction_t direction, hb_codepoint_t glyph, uint start_offset,
    uint * caret_count /* IN/OUT */, hb_position_t * caret_array /*OUT*/)
{
	return font->face->table.GDEF->table->get_lig_carets(font, direction, glyph, start_offset, caret_count, caret_array);
}

#endif

/*
 * GSUB/GPOS
 */
bool OT::GSUB::is_blocklisted(hb_blob_t *blob CXX_UNUSED_PARAM, hb_face_t *face) const
{
#ifdef HB_NO_OT_LAYOUT_BLACKLIST
	return false;
#endif
	return false;
}

bool OT::GPOS::is_blocklisted(hb_blob_t *blob CXX_UNUSED_PARAM, hb_face_t *face CXX_UNUSED_PARAM) const
{
#ifdef HB_NO_OT_LAYOUT_BLACKLIST
	return false;
#endif
	return false;
}

static const OT::GSUBGPOS&get_gsubgpos_table(hb_face_t * face, hb_tag_t table_tag)
{
	switch(table_tag) {
		case HB_OT_TAG_GSUB: return *face->table.GSUB->table;
		case HB_OT_TAG_GPOS: return *face->table.GPOS->table;
		default: return Null(OT::GSUBGPOS);
	}
}

/**
 * hb_ot_layout_table_get_script_tags:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @start_offset: offset of the first script tag to retrieve
 * @script_count: (inout) (allow-none): Input = the maximum number of script tags to return;
 *   Output = the actual number of script tags returned (may be zero)
 * @script_tags: (out) (array length=script_count): The array of #hb_tag_t script tags found for the query
 *
 * Fetches a list of all scripts enumerated in the specified face's GSUB table
 * or GPOS table. The list returned will begin at the offset provided.
 *
 **/
uint hb_ot_layout_table_get_script_tags(hb_face_t * face, hb_tag_t table_tag, uint start_offset, uint * script_count /* IN/OUT */, hb_tag_t * script_tags /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	return g.get_script_tags(start_offset, script_count, script_tags);
}

#define HB_OT_TAG_LATIN_SCRIPT          HB_TAG('l', 'a', 't', 'n')

/**
 * hb_ot_layout_table_find_script:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_tag: #hb_tag_t of the script tag requested
 * @script_index: (out): The index of the requested script tag
 *
 * Fetches the index if a given script tag in the specified face's GSUB table
 * or GPOS table.
 *
 * Return value: true if the script is found, false otherwise
 *
 **/
hb_bool_t hb_ot_layout_table_find_script(hb_face_t * face, hb_tag_t table_tag, hb_tag_t script_tag, uint * script_index /*OUT*/)
{
	static_assert((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_SCRIPT_INDEX), "");
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	if(g.find_script_index(script_tag, script_index))
		return true;
	/* try finding 'DFLT' */
	if(g.find_script_index(HB_OT_TAG_DEFAULT_SCRIPT, script_index))
		return false;
	/* try with 'dflt'; MS site has had typos and many fonts use it now :(.
	 * including many versions of DejaVu Sans Mono! */
	if(g.find_script_index(HB_OT_TAG_DEFAULT_LANGUAGE, script_index))
		return false;
	/* try with 'latn'; some old fonts put their features there even though
	   they're really trying to support Thai, for example :( */
	if(g.find_script_index(HB_OT_TAG_LATIN_SCRIPT, script_index))
		return false;
	ASSIGN_PTR(script_index, HB_OT_LAYOUT_NO_SCRIPT_INDEX);
	return false;
}

#ifndef HB_DISABLE_DEPRECATED
/**
 * hb_ot_layout_table_choose_script:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_tags: Array of #hb_tag_t script tags
 * @script_index: (out): The index of the requested script tag
 * @chosen_script: (out): #hb_tag_t of the script tag requested
 *
 * Deprecated since 2.0.0
 **/
hb_bool_t hb_ot_layout_table_choose_script(hb_face_t * face, hb_tag_t table_tag, const hb_tag_t * script_tags,
    uint * script_index /*OUT*/, hb_tag_t * chosen_script /*OUT*/)
{
	const hb_tag_t * t;
	for(t = script_tags; *t; t++)
		;
	return hb_ot_layout_table_select_script(face, table_tag, t - script_tags, script_tags, script_index, chosen_script);
}

#endif

/**
 * hb_ot_layout_table_select_script:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_count: Number of script tags in the array
 * @script_tags: Array of #hb_tag_t script tags
 * @script_index: (out): The index of the requested script
 * @chosen_script: (out): #hb_tag_t of the requested script
 *
 * Since: 2.0.0
 **/
hb_bool_t hb_ot_layout_table_select_script(hb_face_t * face, hb_tag_t table_tag, uint script_count,
    const hb_tag_t * script_tags, uint * script_index /*OUT*/, hb_tag_t * chosen_script /*OUT*/)
{
	static_assert((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_SCRIPT_INDEX), "");
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	for(uint i = 0; i < script_count; i++) {
		if(g.find_script_index(script_tags[i], script_index)) {
			ASSIGN_PTR(chosen_script, script_tags[i]);
			return true;
		}
	}
	/* try finding 'DFLT' */
	if(g.find_script_index(HB_OT_TAG_DEFAULT_SCRIPT, script_index)) {
		ASSIGN_PTR(chosen_script, HB_OT_TAG_DEFAULT_SCRIPT);
		return false;
	}
	/* try with 'dflt'; MS site has had typos and many fonts use it now :( */
	if(g.find_script_index(HB_OT_TAG_DEFAULT_LANGUAGE, script_index)) {
		ASSIGN_PTR(chosen_script, HB_OT_TAG_DEFAULT_LANGUAGE);
		return false;
	}
	/* try with 'latn'; some old fonts put their features there even though
	   they're really trying to support Thai, for example :( */
	if(g.find_script_index(HB_OT_TAG_LATIN_SCRIPT, script_index)) {
		ASSIGN_PTR(chosen_script, HB_OT_TAG_LATIN_SCRIPT);
		return false;
	}
	ASSIGN_PTR(script_index, HB_OT_LAYOUT_NO_SCRIPT_INDEX);
	ASSIGN_PTR(chosen_script, HB_OT_LAYOUT_NO_SCRIPT_INDEX);
	return false;
}
/**
 * hb_ot_layout_table_get_feature_tags:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @start_offset: offset of the first feature tag to retrieve
 * @feature_count: (inout) (allow-none): Input = the maximum number of feature tags to return;
 *    Output = the actual number of feature tags returned (may be zero)
 * @feature_tags: (out) (array length=feature_count): Array of feature tags found in the table
 *
 * Fetches a list of all feature tags in the given face's GSUB or GPOS table.
 *
 **/
uint hb_ot_layout_table_get_feature_tags(hb_face_t * face, hb_tag_t table_tag, uint start_offset,
    uint * feature_count /* IN/OUT */, hb_tag_t * feature_tags /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	return g.get_feature_tags(start_offset, feature_count, feature_tags);
}
/**
 * hb_ot_layout_table_find_feature:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @feature_tag: The #hb_tag_t og the requested feature tag
 * @feature_index: (out): The index of the requested feature
 *
 * Fetches the index for a given feature tag in the specified face's GSUB table
 * or GPOS table.
 *
 * Return value: true if the feature is found, false otherwise
 **/
bool hb_ot_layout_table_find_feature(hb_face_t * face, hb_tag_t table_tag, hb_tag_t feature_tag, uint * feature_index /*OUT*/)
{
	static_assert((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_FEATURE_INDEX), "");
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	uint num_features = g.get_feature_count();
	for(uint i = 0; i < num_features; i++) {
		if(feature_tag == g.get_feature_tag(i)) {
			ASSIGN_PTR(feature_index, i);
			return true;
		}
	}
	ASSIGN_PTR(feature_index, HB_OT_LAYOUT_NO_FEATURE_INDEX);
	return false;
}
/**
 * hb_ot_layout_script_get_language_tags:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @start_offset: offset of the first language tag to retrieve
 * @language_count: (inout) (allow-none): Input = the maximum number of language tags to return;
 *     Output = the actual number of language tags returned (may be zero)
 * @language_tags: (out) (array length=language_count): Array of language tags found in the table
 *
 * Fetches a list of language tags in the given face's GSUB or GPOS table, underneath
 * the specified script index. The list returned will begin at the offset provided.
 *
 **/
uint hb_ot_layout_script_get_language_tags(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    uint start_offset, uint * language_count /* IN/OUT */, hb_tag_t * language_tags /*OUT*/)
{
	const OT::Script &s = get_gsubgpos_table(face, table_tag).get_script(script_index);
	return s.get_lang_sys_tags(start_offset, language_count, language_tags);
}

#ifndef HB_DISABLE_DEPRECATED
/**
 * hb_ot_layout_script_find_language:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_tag: The #hb_tag_t of the requested language
 * @language_index: The index of the requested language
 *
 * Fetches the index of a given language tag in the specified face's GSUB table
 * or GPOS table, underneath the specified script tag.
 *
 * Return value: true if the language tag is found, false otherwise
 *
 * Since: ??
 * Deprecated: ??
 **/
hb_bool_t hb_ot_layout_script_find_language(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    hb_tag_t language_tag, uint * language_index)
{
	return hb_ot_layout_script_select_language(face, table_tag, script_index, 1, &language_tag, language_index);
}

#endif

/**
 * hb_ot_layout_script_select_language:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_count: The number of languages in the specified script
 * @language_tags: The array of language tags
 * @language_index: (out): The index of the requested language
 *
 * Fetches the index of a given language tag in the specified face's GSUB table
 * or GPOS table, underneath the specified script index.
 *
 * Return value: true if the language tag is found, false otherwise
 *
 * Since: 2.0.0
 **/
hb_bool_t hb_ot_layout_script_select_language(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    uint language_count, const hb_tag_t * language_tags, uint * language_index /*OUT*/)
{
	static_assert((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX), "");
	const OT::Script &s = get_gsubgpos_table(face, table_tag).get_script(script_index);
	uint i;
	for(i = 0; i < language_count; i++) {
		if(s.find_lang_sys_index(language_tags[i], language_index))
			return true;
	}
	/* try finding 'dflt' */
	if(s.find_lang_sys_index(HB_OT_TAG_DEFAULT_LANGUAGE, language_index))
		return false;
	ASSIGN_PTR(language_index, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX);
	return false;
}

/**
 * hb_ot_layout_language_get_required_feature_index:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_index: The index of the requested language tag
 * @feature_index: (out): The index of the requested feature
 *
 * Fetches the index of a requested feature in the given face's GSUB or GPOS table,
 * underneath the specified script and language.
 *
 * Return value: true if the feature is found, false otherwise
 *
 **/
hb_bool_t hb_ot_layout_language_get_required_feature_index(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    uint language_index, uint * feature_index /*OUT*/)
{
	return hb_ot_layout_language_get_required_feature(face, table_tag, script_index, language_index, feature_index, nullptr);
}
/**
 * hb_ot_layout_language_get_required_feature:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_index: The index of the requested language tag
 * @feature_index: (out): The index of the requested feature
 * @feature_tag: (out): The #hb_tag_t of the requested feature
 *
 * Fetches the tag of a requested feature index in the given face's GSUB or GPOS table,
 * underneath the specified script and language.
 *
 * Return value: true if the feature is found, false otherwise
 *
 * Since: 0.9.30
 **/
hb_bool_t hb_ot_layout_language_get_required_feature(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    uint language_index, uint * feature_index /*OUT*/, hb_tag_t * feature_tag /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	const OT::LangSys &l = g.get_script(script_index).get_lang_sys(language_index);
	uint index = l.get_required_feature_index();
	ASSIGN_PTR(feature_index, index);
	ASSIGN_PTR(feature_tag, g.get_feature_tag(index));
	return l.has_required_feature();
}
/**
 * hb_ot_layout_language_get_feature_indexes:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_index: The index of the requested language tag
 * @start_offset: offset of the first feature tag to retrieve
 * @feature_count: (inout) (allow-none): Input = the maximum number of feature tags to return;
 *    Output: the actual number of feature tags returned (may be zero)
 * @feature_indexes: (out) (array length=feature_count): The array of feature indexes found for the query
 *
 * Fetches a list of all features in the specified face's GSUB table
 * or GPOS table, underneath the specified script and language. The list
 * returned will begin at the offset provided.
 **/
uint hb_ot_layout_language_get_feature_indexes(hb_face_t * face, hb_tag_t table_tag,
    uint script_index, uint language_index, uint start_offset, uint * feature_count /* IN/OUT */, uint * feature_indexes /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	const OT::LangSys &l = g.get_script(script_index).get_lang_sys(language_index);
	return l.get_feature_indexes(start_offset, feature_count, feature_indexes);
}
/**
 * hb_ot_layout_language_get_feature_tags:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_index: The index of the requested language tag
 * @start_offset: offset of the first feature tag to retrieve
 * @feature_count: (inout) (allow-none): Input = the maximum number of feature tags to return;
 *    Output = the actual number of feature tags returned (may be zero)
 * @feature_tags: (out) (array length=feature_count): The array of #hb_tag_t feature tags found for the query
 *
 * Fetches a list of all features in the specified face's GSUB table
 * or GPOS table, underneath the specified script and language. The list
 * returned will begin at the offset provided.
 *
 **/
uint hb_ot_layout_language_get_feature_tags(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    uint language_index, uint start_offset, uint * feature_count /* IN/OUT */, hb_tag_t * feature_tags /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	const OT::LangSys &l = g.get_script(script_index).get_lang_sys(language_index);
	static_assert((sizeof(uint) == sizeof(hb_tag_t)), "");
	uint ret = l.get_feature_indexes(start_offset, feature_count, (uint *)feature_tags);
	if(feature_tags) {
		uint count = *feature_count;
		for(uint i = 0; i < count; i++)
			feature_tags[i] = g.get_feature_tag((uint)feature_tags[i]);
	}
	return ret;
}
/**
 * hb_ot_layout_language_find_feature:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @script_index: The index of the requested script tag
 * @language_index: The index of the requested language tag
 * @feature_tag: #hb_tag_t of the feature tag requested
 * @feature_index: (out): The index of the requested feature
 *
 * Fetches the index of a given feature tag in the specified face's GSUB table
 * or GPOS table, underneath the specified script and language.
 *
 * Return value: true if the feature is found, false otherwise
 *
 **/
hb_bool_t hb_ot_layout_language_find_feature(hb_face_t * face, hb_tag_t table_tag, uint script_index,
    uint language_index, hb_tag_t feature_tag, uint * feature_index /*OUT*/)
{
	static_assert((OT::Index::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_FEATURE_INDEX), "");
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	const OT::LangSys &l = g.get_script(script_index).get_lang_sys(language_index);
	uint num_features = l.get_feature_count();
	for(uint i = 0; i < num_features; i++) {
		uint f_index = l.get_feature_index(i);
		if(feature_tag == g.get_feature_tag(f_index)) {
			ASSIGN_PTR(feature_index, f_index);
			return true;
		}
	}
	ASSIGN_PTR(feature_index, HB_OT_LAYOUT_NO_FEATURE_INDEX);
	return false;
}
/**
 * hb_ot_layout_feature_get_lookups:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @feature_index: The index of the requested feature
 * @start_offset: offset of the first lookup to retrieve
 * @lookup_count: (inout) (allow-none): Input = the maximum number of lookups to return;
 *   Output = the actual number of lookups returned (may be zero)
 * @lookup_indexes: (out) (array length=lookup_count): The array of lookup indexes found for the query
 *
 * Fetches a list of all lookups enumerated for the specified feature, in
 * the specified face's GSUB table or GPOS table. The list returned will
 * begin at the offset provided.
 *
 * Since: 0.9.7
 **/
uint hb_ot_layout_feature_get_lookups(hb_face_t * face, hb_tag_t table_tag, uint feature_index, uint start_offset,
    uint * lookup_count /* IN/OUT */, uint * lookup_indexes /*OUT*/)
{
	return hb_ot_layout_feature_with_variations_get_lookups(face, table_tag, feature_index,
		   HB_OT_LAYOUT_NO_VARIATIONS_INDEX, start_offset, lookup_count, lookup_indexes);
}
/**
 * hb_ot_layout_table_get_lookup_count:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 *
 * Fetches the total number of lookups enumerated in the specified
 * face's GSUB table or GPOS table.
 *
 * Since: 0.9.22
 **/
uint hb_ot_layout_table_get_lookup_count(hb_face_t * face, hb_tag_t table_tag)
{
	return get_gsubgpos_table(face, table_tag).get_lookup_count();
}

struct hb_collect_features_context_t {
	hb_collect_features_context_t (hb_face_t * face, hb_tag_t table_tag, hb_set_t * feature_indexes_) : g(get_gsubgpos_table(face, table_tag)),
		feature_indexes(feature_indexes_), script_count(0), langsys_count(0), feature_index_count(0) 
	{
	}
	bool visited(const OT::Script &s)
	{
		/* We might have Null() object here.  Don't want to involve
		 * that in the memoize.  So, detect empty objects and return. */
		if(UNLIKELY(!s.has_default_lang_sys() && !s.get_lang_sys_count()))
			return true;
		if(script_count++ > HB_MAX_SCRIPTS)
			return true;
		return visited(s, visited_script);
	}
	bool visited(const OT::LangSys &l)
	{
		/* We might have Null() object here.  Don't want to involve
		 * that in the memoize.  So, detect empty objects and return. */
		if(UNLIKELY(!l.has_required_feature() && !l.get_feature_count()))
			return true;
		if(langsys_count++ > HB_MAX_LANGSYS)
			return true;
		return visited(l, visited_langsys);
	}
	bool visited_feature_indices(uint count)
	{
		feature_index_count += count;
		return feature_index_count > HB_MAX_FEATURE_INDICES;
	}
private:
	template <typename T>
	bool visited(const T &p, hb_set_t &visited_set)
	{
		hb_codepoint_t delta = (hb_codepoint_t)((uintptr_t)&p - (uintptr_t)&g);
		if(visited_set.has(delta))
			return true;
		visited_set.add(delta);
		return false;
	}
public:
	const OT::GSUBGPOS &g;
	hb_set_t * feature_indexes;
private:
	hb_set_t visited_script;
	hb_set_t visited_langsys;
	uint script_count;
	uint langsys_count;
	uint feature_index_count;
};

static void langsys_collect_features(hb_collect_features_context_t * c, const OT::LangSys  &l, const hb_tag_t * features)
{
	if(c->visited(l)) return;
	if(!features) {
		/* All features. */
		if(l.has_required_feature() && !c->visited_feature_indices(1))
			c->feature_indexes->add(l.get_required_feature_index());
		if(!c->visited_feature_indices(l.featureIndex.len))
			l.add_feature_indexes_to(c->feature_indexes);
	}
	else {
		/* Ugh. Any faster way? */
		for(; *features; features++) {
			hb_tag_t feature_tag = *features;
			uint num_features = l.get_feature_count();
			for(uint i = 0; i < num_features; i++) {
				uint feature_index = l.get_feature_index(i);
				if(feature_tag == c->g.get_feature_tag(feature_index)) {
					c->feature_indexes->add(feature_index);
					break;
				}
			}
		}
	}
}

static void script_collect_features(hb_collect_features_context_t * c, const OT::Script   &s,
    const hb_tag_t * languages, const hb_tag_t * features)
{
	if(c->visited(s)) return;
	if(!languages) {
		/* All languages. */
		if(s.has_default_lang_sys())
			langsys_collect_features(c, s.get_default_lang_sys(), features);
		uint count = s.get_lang_sys_count();
		for(uint language_index = 0; language_index < count; language_index++)
			langsys_collect_features(c, s.get_lang_sys(language_index), features);
	}
	else {
		for(; *languages; languages++) {
			uint language_index;
			if(s.find_lang_sys_index(*languages, &language_index))
				langsys_collect_features(c, s.get_lang_sys(language_index), features);
		}
	}
}

/**
 * hb_ot_layout_collect_features:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @scripts: The array of scripts to collect features for
 * @languages: The array of languages to collect features for
 * @features: The array of features to collect
 * @feature_indexes: (out): The array of feature indexes found for the query
 *
 * Fetches a list of all feature indexes in the specified face's GSUB table
 * or GPOS table, underneath the specified scripts, languages, and features.
 * If no list of scripts is provided, all scripts will be queried. If no list
 * of languages is provided, all languages will be queried. If no list of
 * features is provided, all features will be queried.
 *
 * Since: 1.8.5
 **/
void hb_ot_layout_collect_features(hb_face_t * face, hb_tag_t table_tag, const hb_tag_t * scripts,
    const hb_tag_t * languages, const hb_tag_t * features, hb_set_t * feature_indexes /*OUT*/)
{
	hb_collect_features_context_t c(face, table_tag, feature_indexes);
	if(!scripts) {
		/* All scripts. */
		uint count = c.g.get_script_count();
		for(uint script_index = 0; script_index < count; script_index++)
			script_collect_features(&c, c.g.get_script(script_index), languages, features);
	}
	else {
		for(; *scripts; scripts++) {
			uint script_index;
			if(c.g.find_script_index(*scripts, &script_index))
				script_collect_features(&c, c.g.get_script(script_index), languages, features);
		}
	}
}

/**
 * hb_ot_layout_collect_lookups:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @scripts: The array of scripts to collect lookups for
 * @languages: The array of languages to collect lookups for
 * @features: The array of features to collect lookups for
 * @lookup_indexes: (out): The array of lookup indexes found for the query
 *
 * Fetches a list of all feature-lookup indexes in the specified face's GSUB
 * table or GPOS table, underneath the specified scripts, languages, and
 * features. If no list of scripts is provided, all scripts will be queried.
 * If no list of languages is provided, all languages will be queried. If no
 * list of features is provided, all features will be queried.
 *
 * Since: 0.9.8
 **/
void hb_ot_layout_collect_lookups(hb_face_t * face, hb_tag_t table_tag, const hb_tag_t * scripts,
    const hb_tag_t * languages, const hb_tag_t * features, hb_set_t * lookup_indexes /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	hb_set_t feature_indexes;
	hb_ot_layout_collect_features(face, table_tag, scripts, languages, features, &feature_indexes);
	for(hb_codepoint_t feature_index = HB_SET_VALUE_INVALID; hb_set_next(&feature_indexes, &feature_index);)
		g.get_feature(feature_index).add_lookup_indexes_to(lookup_indexes);
	g.feature_variation_collect_lookups(&feature_indexes, lookup_indexes);
}

#ifndef HB_NO_LAYOUT_COLLECT_GLYPHS
/**
 * hb_ot_layout_lookup_collect_glyphs:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @lookup_index: The index of the feature lookup to query
 * @glyphs_before: (out): Array of glyphs preceding the substitution range
 * @glyphs_input: (out): Array of input glyphs that would be substituted by the lookup
 * @glyphs_after: (out): Array of glyphs following the substitution range
 * @glyphs_output: (out): Array of glyphs that would be the substitued output of the lookup
 *
 * Fetches a list of all glyphs affected by the specified lookup in the
 * specified face's GSUB table or GPOS table.
 *
 * Since: 0.9.7
 **/
void hb_ot_layout_lookup_collect_glyphs(hb_face_t * face, hb_tag_t table_tag, uint lookup_index,
    hb_set_t * glyphs_before, /*OUT  May be NULL */
    hb_set_t * glyphs_input, /*OUT  May be NULL */
    hb_set_t * glyphs_after, /*OUT  May be NULL */
    hb_set_t * glyphs_output /*OUT  May be NULL */)
{
	OT::hb_collect_glyphs_context_t c(face, glyphs_before, glyphs_input, glyphs_after, glyphs_output);
	switch(table_tag) {
		case HB_OT_TAG_GSUB:
	    {
		    const OT::SubstLookup& l = face->table.GSUB->table->get_lookup(lookup_index);
		    l.collect_glyphs(&c);
		    return;
	    }
		case HB_OT_TAG_GPOS:
	    {
		    const OT::PosLookup& l = face->table.GPOS->table->get_lookup(lookup_index);
		    l.collect_glyphs(&c);
		    return;
	    }
	}
}

#endif

/* Variations support */

/**
 * hb_ot_layout_table_find_feature_variations:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @coords: The variation coordinates to query
 * @num_coords: The number of variation coorinates
 * @variations_index: (out): The array of feature variations found for the query
 *
 * Fetches a list of feature variations in the specified face's GSUB table
 * or GPOS table, at the specified variation coordinates.
 *
 **/
hb_bool_t hb_ot_layout_table_find_feature_variations(hb_face_t * face, hb_tag_t table_tag, const int * coords, uint num_coords, uint * variations_index /*OUT*/)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	return g.find_variations_index(coords, num_coords, variations_index);
}
/**
 * hb_ot_layout_feature_with_variations_get_lookups:
 * @face: #hb_face_t to work upon
 * @table_tag: HB_OT_TAG_GSUB or HB_OT_TAG_GPOS
 * @feature_index: The index of the feature to query
 * @variations_index: The index of the feature variation to query
 * @start_offset: offset of the first lookup to retrieve
 * @lookup_count: (inout) (allow-none): Input = the maximum number of lookups to return;
 *   Output = the actual number of lookups returned (may be zero)
 * @lookup_indexes: (out) (array length=lookup_count): The array of lookups found for the query
 *
 * Fetches a list of all lookups enumerated for the specified feature, in
 * the specified face's GSUB table or GPOS table, enabled at the specified
 * variations index. The list returned will begin at the offset provided.
 *
 **/
uint hb_ot_layout_feature_with_variations_get_lookups(hb_face_t * face,
    hb_tag_t table_tag,
    uint feature_index,
    uint variations_index,
    uint start_offset,
    uint * lookup_count /* IN/OUT */,
    uint * lookup_indexes /*OUT*/)
{
	static_assert((OT::FeatureVariations::NOT_FOUND_INDEX == HB_OT_LAYOUT_NO_VARIATIONS_INDEX), "");
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	const OT::Feature &f = g.get_feature_variation(feature_index, variations_index);
	return f.get_lookup_indexes(start_offset, lookup_count, lookup_indexes);
}

/*
 * OT::GSUB
 */

/**
 * hb_ot_layout_has_substitution:
 * @face: #hb_face_t to work upon
 *
 * Tests whether the specified face includes any GSUB substitutions.
 *
 * Return value: true if data found, false otherwise
 *
 **/
hb_bool_t hb_ot_layout_has_substitution(hb_face_t * face)
{
	return face->table.GSUB->table->has_data();
}

/**
 * hb_ot_layout_lookup_would_substitute:
 * @face: #hb_face_t to work upon
 * @lookup_index: The index of the lookup to query
 * @glyphs: The sequence of glyphs to query for substitution
 * @glyphs_length: The length of the glyph sequence
 * @zero_context: #hb_bool_t indicating whether substitutions should be context-free
 *
 * Tests whether a specified lookup in the specified face would
 * trigger a substitution on the given glyph sequence.
 *
 * Return value: true if a substitution would be triggered, false otherwise
 *
 * Since: 0.9.7
 **/
hb_bool_t hb_ot_layout_lookup_would_substitute(hb_face_t * face, uint lookup_index,
    const hb_codepoint_t * glyphs, uint glyphs_length, hb_bool_t zero_context)
{
	if(UNLIKELY(lookup_index >= face->table.GSUB->lookup_count)) return false;
	OT::hb_would_apply_context_t c(face, glyphs, glyphs_length, (bool)zero_context);

	const OT::SubstLookup& l = face->table.GSUB->table->get_lookup(lookup_index);
	return l.would_apply(&c, &face->table.GSUB->accels[lookup_index]);
}

/**
 * hb_ot_layout_substitute_start:
 * @font: #hb_font_t to use
 * @buffer: #hb_buffer_t buffer to work upon
 *
 * Called before substitution lookups are performed, to ensure that glyph
 * class and other properties are set on the glyphs in the buffer.
 *
 **/
void hb_ot_layout_substitute_start(hb_font_t * font, hb_buffer_t * buffer)
{
	_hb_ot_layout_set_glyph_props(font, buffer);
}

void hb_ot_layout_delete_glyphs_inplace(hb_buffer_t * buffer, bool (*filter)(const hb_glyph_info_t * info))
{
	/* Merge clusters and delete filtered glyphs.
	 * NOTE! We can't use out-buffer as we have positioning data. */
	uint j = 0;
	uint count = buffer->len;
	hb_glyph_info_t * info = buffer->info;
	hb_glyph_position_t * pos = buffer->pos;
	for(uint i = 0; i < count; i++) {
		if(filter(&info[i])) {
			/* Merge clusters.
			 * Same logic as buffer->delete_glyph(), but for in-place removal. */

			uint cluster = info[i].cluster;
			if(i + 1 < count && cluster == info[i+1].cluster)
				continue; /* Cluster survives; do nothing. */

			if(j) {
				/* Merge cluster backward. */
				if(cluster < info[j - 1].cluster) {
					uint mask = info[i].mask;
					uint old_cluster = info[j - 1].cluster;
					for(unsigned k = j; k && info[k - 1].cluster == old_cluster; k--)
						buffer->set_cluster(info[k - 1], cluster, mask);
				}
				continue;
			}

			if(i + 1 < count)
				buffer->merge_clusters(i, i + 2); /* Merge cluster forward. */

			continue;
		}

		if(j != i) {
			info[j] = info[i];
			pos[j] = pos[i];
		}
		j++;
	}
	buffer->len = j;
}

/**
 * hb_ot_layout_lookup_substitute_closure:
 * @face: #hb_face_t to work upon
 * @lookup_index: index of the feature lookup to query
 * @glyphs: (out): Array of glyphs comprising the transitive closure of the lookup
 *
 * Compute the transitive closure of glyphs needed for a
 * specified lookup.
 *
 * Since: 0.9.7
 **/
void hb_ot_layout_lookup_substitute_closure(hb_face_t * face,
    uint lookup_index,
    hb_set_t * glyphs /*OUT*/)
{
	hb_map_t done_lookups;
	OT::hb_closure_context_t c(face, glyphs, &done_lookups);

	const OT::SubstLookup& l = face->table.GSUB->table->get_lookup(lookup_index);

	l.closure(&c, lookup_index);
}

/**
 * hb_ot_layout_lookups_substitute_closure:
 * @face: #hb_face_t to work upon
 * @lookups: The set of lookups to query
 * @glyphs: (out): Array of glyphs comprising the transitive closure of the lookups
 *
 * Compute the transitive closure of glyphs needed for all of the
 * provided lookups.
 *
 * Since: 1.8.1
 **/
void hb_ot_layout_lookups_substitute_closure(hb_face_t * face,
    const hb_set_t * lookups,
    hb_set_t * glyphs /*OUT*/)
{
	hb_map_t done_lookups;
	OT::hb_closure_context_t c(face, glyphs, &done_lookups);
	const OT::GSUB& gsub = *face->table.GSUB->table;

	uint iteration_count = 0;
	uint glyphs_length;
	do {
		glyphs_length = glyphs->get_population();
		if(lookups) {
			for(hb_codepoint_t lookup_index = HB_SET_VALUE_INVALID; hb_set_next(lookups, &lookup_index);)
				gsub.get_lookup(lookup_index).closure(&c, lookup_index);
		}
		else {
			for(uint i = 0; i < gsub.get_lookup_count(); i++)
				gsub.get_lookup(i).closure(&c, i);
		}
	} while(iteration_count++ <= HB_CLOSURE_MAX_STAGES &&
	    glyphs_length != glyphs->get_population());
}

/*
 * OT::GPOS
 */

/**
 * hb_ot_layout_has_positioning:
 * @face: #hb_face_t to work upon
 *
 * Return value: true if the face has GPOS data, false otherwise
 *
 **/
hb_bool_t hb_ot_layout_has_positioning(hb_face_t * face)
{
	return face->table.GPOS->table->has_data();
}

/**
 * hb_ot_layout_position_start:
 * @font: #hb_font_t to use
 * @buffer: #hb_buffer_t buffer to work upon
 *
 * Called before positioning lookups are performed, to ensure that glyph
 * attachment types and glyph-attachment chains are set for the glyphs in the buffer.
 *
 **/
void hb_ot_layout_position_start(hb_font_t * font, hb_buffer_t * buffer)
{
	OT::GPOS::position_start(font, buffer);
}

/**
 * hb_ot_layout_position_finish_advances:
 * @font: #hb_font_t to use
 * @buffer: #hb_buffer_t buffer to work upon
 *
 * Called after positioning lookups are performed, to finish glyph advances.
 *
 **/
void hb_ot_layout_position_finish_advances(hb_font_t * font, hb_buffer_t * buffer)
{
	OT::GPOS::position_finish_advances(font, buffer);
}

/**
 * hb_ot_layout_position_finish_offsets:
 * @font: #hb_font_t to use
 * @buffer: #hb_buffer_t buffer to work upon
 *
 * Called after positioning lookups are performed, to finish glyph offsets.
 *
 **/
void hb_ot_layout_position_finish_offsets(hb_font_t * font, hb_buffer_t * buffer)
{
	OT::GPOS::position_finish_offsets(font, buffer);
}

#ifndef HB_NO_LAYOUT_FEATURE_PARAMS
/**
 * hb_ot_layout_get_size_params:
 * @face: #hb_face_t to work upon
 * @design_size: (out): The design size of the face
 * @subfamily_id: (out): The identifier of the face within the font subfamily
 * @subfamily_name_id: (out): The ‘name’ table name ID of the face within the font subfamily
 * @range_start: (out): The minimum size of the recommended size range for the face
 * @range_end: (out): The maximum size of the recommended size range for the face
 *
 * Fetches optical-size feature data (i.e., the `size` feature from GPOS). Note that
 * the subfamily_id and the subfamily name string (accessible via the subfamily_name_id)
 * as used here are defined as pertaining only to fonts within a font family that differ
 * specifically in their respective size ranges; other ways to differentiate fonts within
 * a subfamily are not covered by the `size` feature.
 *
 * For more information on this distinction, see the [`size` feature documentation](
 * https://docs.microsoft.com/en-us/typography/opentype/spec/features_pt#tag-size).
 *
 * Return value: true if data found, false otherwise
 *
 * Since: 0.9.10
 **/
hb_bool_t hb_ot_layout_get_size_params(hb_face_t * face,
    uint * design_size,                                /*OUT  May be NULL */
    uint * subfamily_id,                               /*OUT  May be NULL */
    hb_ot_name_id_t * subfamily_name_id,                          /*OUT  May be NULL */
    uint * range_start,                                /*OUT  May be NULL */
    uint * range_end /*OUT  May be NULL */)
{
	const OT::GPOS &gpos = *face->table.GPOS->table;
	const hb_tag_t tag = HB_TAG('s', 'i', 'z', 'e');

	uint num_features = gpos.get_feature_count();
	for(uint i = 0; i < num_features; i++) {
		if(tag == gpos.get_feature_tag(i)) {
			const OT::Feature &f = gpos.get_feature(i);
			const OT::FeatureParamsSize &params = f.get_feature_params().get_size_params(tag);

			if(params.designSize) {
				if(design_size) *design_size = params.designSize;
				if(subfamily_id) *subfamily_id = params.subfamilyID;
				if(subfamily_name_id) *subfamily_name_id = params.subfamilyNameID;
				if(range_start) *range_start = params.rangeStart;
				if(range_end) *range_end = params.rangeEnd;

				return true;
			}
		}
	}

	if(design_size) *design_size = 0;
	if(subfamily_id) *subfamily_id = 0;
	if(subfamily_name_id) *subfamily_name_id = HB_OT_NAME_ID_INVALID;
	if(range_start) *range_start = 0;
	if(range_end) *range_end = 0;

	return false;
}

/**
 * hb_ot_layout_feature_get_name_ids:
 * @face: #hb_face_t to work upon
 * @table_tag: table tag to query, "GSUB" or "GPOS".
 * @feature_index: index of feature to query.
 * @label_id: (out) (allow-none): The ‘name’ table name ID that specifies a string
 *      for a user-interface label for this feature. (May be NULL.)
 * @tooltip_id: (out) (allow-none): The ‘name’ table name ID that specifies a string
 * that an application can use for tooltip text for this
 * feature. (May be NULL.)
 * @sample_id: (out) (allow-none): The ‘name’ table name ID that specifies sample text
 *       that illustrates the effect of this feature. (May be NULL.)
 * @num_named_parameters: (out) (allow-none):  Number of named parameters. (May be zero.)
 * @first_param_id: (out) (allow-none): The first ‘name’ table name ID used to specify
 *     strings for user-interface labels for the feature
 *     parameters. (Must be zero if numParameters is zero.)
 *
 * Fetches name indices from feature parameters for "Stylistic Set" ('ssXX') or
 * "Character Variant" ('cvXX') features.
 *
 * Return value: true if data found, false otherwise
 *
 * Since: 2.0.0
 **/
hb_bool_t hb_ot_layout_feature_get_name_ids(hb_face_t * face,
    hb_tag_t table_tag,
    uint feature_index,
    hb_ot_name_id_t * label_id,                                           /*OUT  May be NULL */
    hb_ot_name_id_t * tooltip_id,                                         /*OUT  May be NULL */
    hb_ot_name_id_t * sample_id,                                          /*OUT  May be NULL */
    uint * num_named_parameters,                               /*OUT  May be NULL */
    hb_ot_name_id_t * first_param_id /*OUT  May be NULL */)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);

	hb_tag_t feature_tag = g.get_feature_tag(feature_index);
	const OT::Feature &f = g.get_feature(feature_index);

	const OT::FeatureParams &feature_params = f.get_feature_params();
	if(&feature_params != &Null(OT::FeatureParams)) {
		const OT::FeatureParamsStylisticSet& ss_params =
		    feature_params.get_stylistic_set_params(feature_tag);
		if(&ss_params != &Null(OT::FeatureParamsStylisticSet)) { /* ssXX */
			if(label_id) *label_id = ss_params.uiNameID;
			// ssXX features don't have the rest
			if(tooltip_id) *tooltip_id = HB_OT_NAME_ID_INVALID;
			if(sample_id) *sample_id = HB_OT_NAME_ID_INVALID;
			if(num_named_parameters) *num_named_parameters = 0;
			if(first_param_id) *first_param_id = HB_OT_NAME_ID_INVALID;
			return true;
		}
		const OT::FeatureParamsCharacterVariants& cv_params =
		    feature_params.get_character_variants_params(feature_tag);
		if(&cv_params != &Null(OT::FeatureParamsCharacterVariants)) { /* cvXX */
			if(label_id) *label_id = cv_params.featUILableNameID;
			if(tooltip_id) *tooltip_id = cv_params.featUITooltipTextNameID;
			if(sample_id) *sample_id = cv_params.sampleTextNameID;
			if(num_named_parameters) *num_named_parameters = cv_params.numNamedParameters;
			if(first_param_id) *first_param_id = cv_params.firstParamUILabelNameID;
			return true;
		}
	}

	if(label_id) *label_id = HB_OT_NAME_ID_INVALID;
	if(tooltip_id) *tooltip_id = HB_OT_NAME_ID_INVALID;
	if(sample_id) *sample_id = HB_OT_NAME_ID_INVALID;
	if(num_named_parameters) *num_named_parameters = 0;
	if(first_param_id) *first_param_id = HB_OT_NAME_ID_INVALID;
	return false;
}

/**
 * hb_ot_layout_feature_get_characters:
 * @face: #hb_face_t to work upon
 * @table_tag: table tag to query, "GSUB" or "GPOS".
 * @feature_index: index of feature to query.
 * @start_offset: offset of the first character to retrieve
 * @char_count: (inout) (allow-none): Input = the maximum number of characters to return;
 * Output = the actual number of characters returned (may be zero)
 * @characters: (out caller-allocates) (array length=char_count): A buffer pointer.
 * The Unicode codepoints of the characters for which this feature provides
 *  glyph variants.
 *
 * Fetches a list of the characters defined as having a variant under the specified
 * "Character Variant" ("cvXX") feature tag.
 *
 * Return value: Number of total sample characters in the cvXX feature.
 *
 * Since: 2.0.0
 **/
uint hb_ot_layout_feature_get_characters(hb_face_t * face,
    hb_tag_t table_tag,
    uint feature_index,
    uint start_offset,
    uint * char_count,                                 /*IN/OUT  May be NULL */
    hb_codepoint_t * characters /*OUT     May be NULL */)
{
	const OT::GSUBGPOS &g = get_gsubgpos_table(face, table_tag);
	return g.get_feature(feature_index)
	       .get_feature_params()
	       .get_character_variants_params(g.get_feature_tag(feature_index))
	       .get_characters(start_offset, char_count, characters);
}

#endif

/*
 * Parts of different types are implemented here such that they have direct
 * access to GSUB/GPOS lookups.
 */

struct GSUBProxy {
	static constexpr unsigned table_index = 0u;
	static constexpr bool inplace = false;
	typedef OT::SubstLookup Lookup;

	GSUBProxy (hb_face_t * face) :
		table(*face->table.GSUB->table),
		accels(face->table.GSUB->accels) {
	}

	const OT::GSUB &table;
	const OT::hb_ot_layout_lookup_accelerator_t * accels;
};

struct GPOSProxy {
	static constexpr unsigned table_index = 1u;
	static constexpr bool inplace = true;
	typedef OT::PosLookup Lookup;

	GPOSProxy (hb_face_t * face) :
		table(*face->table.GPOS->table),
		accels(face->table.GPOS->accels) {
	}

	const OT::GPOS &table;
	const OT::hb_ot_layout_lookup_accelerator_t * accels;
};

static inline bool apply_forward(OT::hb_ot_apply_context_t * c,
    const OT::hb_ot_layout_lookup_accelerator_t &accel)
{
	bool ret = false;
	hb_buffer_t * buffer = c->buffer;
	while(buffer->idx < buffer->len && buffer->successful) {
		bool applied = false;
		if(accel.may_have(buffer->cur().codepoint) &&
		    (buffer->cur().mask & c->lookup_mask) &&
		    c->check_glyph_property(&buffer->cur(), c->lookup_props)) {
			applied = accel.apply(c);
		}

		if(applied)
			ret = true;
		else
			buffer->next_glyph();
	}
	return ret;
}

static inline bool apply_backward(OT::hb_ot_apply_context_t * c,
    const OT::hb_ot_layout_lookup_accelerator_t &accel)
{
	bool ret = false;
	hb_buffer_t * buffer = c->buffer;
	do {
		if(accel.may_have(buffer->cur().codepoint) &&
		    (buffer->cur().mask & c->lookup_mask) &&
		    c->check_glyph_property(&buffer->cur(), c->lookup_props))
			ret |= accel.apply(c);

		/* The reverse lookup doesn't "advance" cursor (for good reason). */
		buffer->idx--;
	}
	while((int)buffer->idx >= 0);
	return ret;
}

template <typename Proxy>
static inline void apply_string(OT::hb_ot_apply_context_t * c,
    const typename Proxy::Lookup &lookup,
    const OT::hb_ot_layout_lookup_accelerator_t &accel)
{
	hb_buffer_t * buffer = c->buffer;
	if(UNLIKELY(!buffer->len || !c->lookup_mask))
		return;
	c->set_lookup_props(lookup.get_props());
	if(LIKELY(!lookup.is_reverse())) {
		/* in/out forward substitution/positioning */
		if(Proxy::table_index == 0u)
			buffer->clear_output();
		buffer->idx = 0;
		bool ret = apply_forward(c, accel);
		if(ret) {
			if(!Proxy::inplace)
				buffer->swap_buffers();
			else
				assert(!buffer->has_separate_output());
		}
	}
	else {
		/* in-place backward substitution/positioning */
		if(Proxy::table_index == 0u)
			buffer->remove_output();
		buffer->idx = buffer->len - 1;
		apply_backward(c, accel);
	}
}

template <typename Proxy> inline void hb_ot_map_t::apply(const Proxy &proxy,
    const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer) const
{
	const uint table_index = proxy.table_index;
	uint i = 0;
	OT::hb_ot_apply_context_t c(table_index, font, buffer);
	c.set_recurse_func(Proxy::Lookup::apply_recurse_func);
	for(uint stage_index = 0; stage_index < stages[table_index].length; stage_index++) {
		const stage_map_t * stage = &stages[table_index][stage_index];
		for(; i < stage->last_lookup; i++) {
			uint lookup_index = lookups[table_index][i].index;
			if(!buffer->message(font, "start lookup %d", lookup_index)) continue;
			c.set_lookup_index(lookup_index);
			c.set_lookup_mask(lookups[table_index][i].mask);
			c.set_auto_zwj(lookups[table_index][i].auto_zwj);
			c.set_auto_zwnj(lookups[table_index][i].auto_zwnj);
			if(lookups[table_index][i].random) {
				c.set_random(true);
				buffer->unsafe_to_break_all();
			}
			apply_string<Proxy> (&c, proxy.table.get_lookup(lookup_index), proxy.accels[lookup_index]);
			(void)buffer->message(font, "end lookup %d", lookup_index);
		}
		if(stage->pause_func) {
			buffer->clear_output();
			stage->pause_func(plan, font, buffer);
		}
	}
}

void hb_ot_map_t::substitute(const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer) const
{
	GSUBProxy proxy(font->face);
	if(!buffer->message(font, "start table GSUB")) return;
	apply(proxy, plan, font, buffer);
	(void)buffer->message(font, "end table GSUB");
}

void hb_ot_map_t::position(const hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer) const
{
	GPOSProxy proxy(font->face);
	if(!buffer->message(font, "start table GPOS")) return;
	apply(proxy, plan, font, buffer);
	(void)buffer->message(font, "end table GPOS");
}

void hb_ot_layout_substitute_lookup(OT::hb_ot_apply_context_t * c,
    const OT::SubstLookup &lookup,
    const OT::hb_ot_layout_lookup_accelerator_t &accel)
{
	apply_string<GSUBProxy> (c, lookup, accel);
}

#ifndef HB_NO_BASE
/**
 * hb_ot_layout_get_baseline:
 * @font: a font
 * @baseline_tag: a baseline tag
 * @direction: text direction.
 * @script_tag:  script tag.
 * @language_tag: language tag.
 * @coord: (out): baseline value if found.
 *
 * Fetches a baseline value from the face.
 *
 * Return value: if found baseline value in the font.
 *
 * Since: 2.6.0
 **/
hb_bool_t hb_ot_layout_get_baseline(hb_font_t * font,
    hb_ot_layout_baseline_tag_t baseline_tag,
    hb_direction_t direction,
    hb_tag_t script_tag,
    hb_tag_t language_tag,
    hb_position_t * coord /*OUT  May be NULL*/)
{
	bool result = font->face->table.BASE->get_baseline(font, baseline_tag, direction, script_tag, language_tag, coord);

	if(result && coord)
		*coord = HB_DIRECTION_IS_HORIZONTAL(direction) ? font->em_scale_y(*coord) : font->em_scale_x(*coord);

	return result;
}

#endif

struct hb_get_glyph_alternates_dispatch_t :
	hb_dispatch_context_t<hb_get_glyph_alternates_dispatch_t, unsigned> {
	static return_t default_return_value() {
		return 0;
	}

	bool stop_sublookup_iteration(return_t r) const {
		return r;
	}

	hb_face_t * face;

	hb_get_glyph_alternates_dispatch_t (hb_face_t * face) :
		face(face) {
	}

private:
	template <typename T, typename ... Ts> auto _dispatch(const T &obj, hb_priority<1>, Ts&& ... ds) HB_AUTO_RETURN
		(obj.get_glyph_alternates(hb_forward<Ts> (ds) ...) )
	template <typename T, typename ... Ts> auto _dispatch(const T &obj, hb_priority<0>, Ts&& ... ds) HB_AUTO_RETURN
		(default_return_value() )
public:
	template <typename T, typename ... Ts> auto dispatch(const T &obj, Ts&& ... ds) HB_AUTO_RETURN
		(_dispatch(obj, hb_prioritize, hb_forward<Ts> (ds) ...) )
};

/**
 * hb_ot_layout_lookup_get_glyph_alternates:
 * @face: a face.
 * @lookup_index: index of the feature lookup to query.
 * @glyph: a glyph id.
 * @start_offset: starting offset.
 * @alternate_count: (inout) (allow-none): Input = the maximum number of alternate glyphs to return;
 *      Output = the actual number of alternate glyphs returned (may be zero).
 * @alternate_glyphs: (out caller-allocates) (array length=alternate_count): A glyphs buffer.
 * Alternate glyphs associated with the glyph id.
 *
 * Fetches alternates of a glyph from a given GSUB lookup index.
 *
 * Return value: total number of alternates found in the specific lookup index for the given glyph id.
 *
 * Since: 2.6.8
 **/
HB_EXTERN unsigned hb_ot_layout_lookup_get_glyph_alternates(hb_face_t * face, unsigned lookup_index,
    hb_codepoint_t glyph, unsigned start_offset, unsigned * alternate_count /*IN/OUT May be NULL*/, hb_codepoint_t * alternate_glyphs /*OUT May be NULL*/)
{
	hb_get_glyph_alternates_dispatch_t c(face);
	const OT::SubstLookup &lookup = face->table.GSUB->table->get_lookup(lookup_index);
	auto ret = lookup.dispatch(&c, glyph, start_offset, alternate_count, alternate_glyphs);
	if(!ret && alternate_count) 
		*alternate_count = 0;
	return ret;
}

#endif
