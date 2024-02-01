/*
 * Copyright © 2011  Martin Hosken
 * Copyright © 2011  SIL International
 * Copyright © 2011,2012  Google, Inc.
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

#ifdef HAVE_GRAPHITE2

#include "hb-graphite2.h"
#include <graphite2/Segment.h>
#include "hb-ot-layout.h"

/**
 * SECTION:hb-graphite2
 * @title: hb-graphite2
 * @short_description: Graphite2 integration
 * @include: hb-graphite2.h
 *
 * Functions for using HarfBuzz with fonts that include Graphite features.
 *
 * For Graphite features to work, you must be sure that HarfBuzz was compiled
 * with the `graphite2` shaping engine enabled. Currently, the default is to
 * not enable `graphite2` shaping.
 **/
/*
 * shaper face data
 */
typedef struct hb_graphite2_tablelist_t {
	struct hb_graphite2_tablelist_t * next;
	hb_blob_t * blob;
	uint tag;
} hb_graphite2_tablelist_t;

struct hb_graphite2_face_data_t {
	hb_face_t * face;
	gr_face * grface;
	hb_atomic_ptr_t<hb_graphite2_tablelist_t> tlist;
};

static const void * hb_graphite2_get_table(const void * data, uint tag, size_t * len)
{
	hb_graphite2_face_data_t * face_data = (hb_graphite2_face_data_t*)data;
	hb_graphite2_tablelist_t * tlist = face_data->tlist;
	hb_blob_t * blob = nullptr;
	for(hb_graphite2_tablelist_t * p = tlist; p; p = p->next)
		if(p->tag == tag) {
			blob = p->blob;
			break;
		}
	if(UNLIKELY(!blob)) {
		blob = face_data->face->reference_table(tag);
		hb_graphite2_tablelist_t * p = (hb_graphite2_tablelist_t*)SAlloc::C(1, sizeof(hb_graphite2_tablelist_t));
		if(UNLIKELY(!p)) {
			hb_blob_destroy(blob);
			return nullptr;
		}
		p->blob = blob;
		p->tag = tag;
retry:
		hb_graphite2_tablelist_t *tlist = face_data->tlist;
		p->next = tlist;
		if(UNLIKELY(!face_data->tlist.cmpexch(tlist, p)))
			goto retry;
	}
	uint tlen;
	const char * d = hb_blob_get_data(blob, &tlen);
	*len = tlen;
	return d;
}

hb_graphite2_face_data_t * _hb_graphite2_shaper_face_data_create(hb_face_t * face)
{
	hb_blob_t * silf_blob = face->reference_table(HB_GRAPHITE2_TAG_SILF);
	/* Umm, we just reference the table to check whether it exists.
	 * Maybe add better API for this? */
	if(!hb_blob_get_length(silf_blob)) {
		hb_blob_destroy(silf_blob);
		return nullptr;
	}
	hb_blob_destroy(silf_blob);
	hb_graphite2_face_data_t * data = (hb_graphite2_face_data_t*)SAlloc::C(1, sizeof(hb_graphite2_face_data_t));
	if(UNLIKELY(!data))
		return nullptr;
	data->face = face;
	data->grface = gr_make_face(data, &hb_graphite2_get_table, gr_face_preloadAll);
	if(UNLIKELY(!data->grface)) {
		SAlloc::F(data);
		return nullptr;
	}
	return data;
}

void _hb_graphite2_shaper_face_data_destroy(hb_graphite2_face_data_t * data)
{
	hb_graphite2_tablelist_t * tlist = data->tlist;
	while(tlist) {
		hb_graphite2_tablelist_t * old = tlist;
		hb_blob_destroy(tlist->blob);
		tlist = tlist->next;
		SAlloc::F(old);
	}
	gr_face_destroy(data->grface);
	SAlloc::F(data);
}

/**
 * hb_graphite2_face_get_gr_face:
 * @face: @hb_face_t to query
 *
 * Fetches the Graphite2 gr_face corresponding to the specified
 * #hb_face_t face object.
 *
 * Return value: the gr_face found
 *
 * Since: 0.9.10
 */
gr_face * hb_graphite2_face_get_gr_face(hb_face_t * face)
{
	const hb_graphite2_face_data_t * data = face->data.graphite2;
	return data ? data->grface : nullptr;
}

/*
 * shaper font data
 */

struct hb_graphite2_font_data_t {};

hb_graphite2_font_data_t * _hb_graphite2_shaper_font_data_create(hb_font_t * font CXX_UNUSED_PARAM)
{
	return (hb_graphite2_font_data_t*)HB_SHAPER_DATA_SUCCEEDED;
}

void _hb_graphite2_shaper_font_data_destroy(hb_graphite2_font_data_t * data CXX_UNUSED_PARAM)
{
}

#ifndef HB_DISABLE_DEPRECATED
	/**
	 * hb_graphite2_font_get_gr_font:
	 *
	 * Since: 0.9.10
	 * Deprecated: 1.4.2
	 */
	gr_font * hb_graphite2_font_get_gr_font(hb_font_t * font CXX_UNUSED_PARAM) { return nullptr; }
#endif

/*
 * shaper
 */

struct hb_graphite2_cluster_t {
	uint base_char;
	uint num_chars;
	uint base_glyph;
	uint num_glyphs;
	uint cluster;
	uint advance;
};

hb_bool_t _hb_graphite2_shape(hb_shape_plan_t * shape_plan CXX_UNUSED_PARAM,
    hb_font_t * font, hb_buffer_t * buffer, const hb_feature_t * features, uint num_features)
{
	hb_face_t * face = font->face;
	gr_face * grface = face->data.graphite2->grface;
	const char * lang = hb_language_to_string(hb_buffer_get_language(buffer));
	const char * lang_end = lang ? strchr(lang, '-') : nullptr;
	int lang_len = lang_end ? lang_end - lang : -1;
	gr_feature_val * feats = gr_face_featureval_for_lang(grface, lang ? hb_tag_from_string(lang, lang_len) : 0);
	for(uint i = 0; i < num_features; i++) {
		const gr_feature_ref * fref = gr_face_find_fref(grface, features[i].tag);
		if(fref)
			gr_fref_set_feature_value(fref, features[i].value, feats);
	}
	gr_segment * seg = nullptr;
	const gr_slot * is;
	uint ci = 0, ic = 0;
	uint curradvx = 0, curradvy = 0;
	uint scratch_size;
	hb_buffer_t::scratch_buffer_t * scratch = buffer->get_scratch_buffer(&scratch_size);
	uint32_t * chars = (uint32_t*)scratch;
	for(uint i = 0; i < buffer->len; ++i)
		chars[i] = buffer->info[i].codepoint;
	/* TODO ensure_native_direction. */
	hb_tag_t script_tag[HB_OT_MAX_TAGS_PER_SCRIPT];
	uint count = HB_OT_MAX_TAGS_PER_SCRIPT;
	hb_ot_tags_from_script_and_language(hb_buffer_get_script(buffer), HB_LANGUAGE_INVALID, &count, script_tag, nullptr, nullptr);
	seg = gr_make_seg(nullptr, grface, count ? script_tag[count - 1] : HB_OT_TAG_DEFAULT_SCRIPT, feats,
		gr_utf32, chars, buffer->len, 2 | (hb_buffer_get_direction(buffer) == HB_DIRECTION_RTL ? 1 : 0));
	if(UNLIKELY(!seg)) {
		if(feats) gr_featureval_destroy(feats);
		return false;
	}
	uint glyph_count = gr_seg_n_slots(seg);
	if(UNLIKELY(!glyph_count)) {
		if(feats) gr_featureval_destroy(feats);
		gr_seg_destroy(seg);
		buffer->len = 0;
		return true;
	}
	buffer->ensure(glyph_count);
	scratch = buffer->get_scratch_buffer(&scratch_size);
	while((idivroundup(sizeof(hb_graphite2_cluster_t) * buffer->len, sizeof(*scratch)) +
	    idivroundup(sizeof(hb_codepoint_t) * glyph_count, sizeof(*scratch))) > scratch_size) {
		if(UNLIKELY(!buffer->ensure(buffer->allocated * 2))) {
			if(feats) gr_featureval_destroy(feats);
			gr_seg_destroy(seg);
			return false;
		}
		scratch = buffer->get_scratch_buffer(&scratch_size);
	}
#define ALLOCATE_ARRAY(Type, name, len) \
	Type *name = (Type*)scratch; \
	do { \
		uint _consumed = idivroundup((len) * sizeof(Type), sizeof(*scratch)); \
		assert(_consumed <= scratch_size); \
		scratch += _consumed; \
		scratch_size -= _consumed; \
	} while(0)

	ALLOCATE_ARRAY(hb_graphite2_cluster_t, clusters, buffer->len);
	ALLOCATE_ARRAY(hb_codepoint_t, gids, glyph_count);

#undef ALLOCATE_ARRAY
	memzero(clusters, sizeof(clusters[0]) * buffer->len);
	hb_codepoint_t * pg = gids;
	clusters[0].cluster = buffer->info[0].cluster;
	uint upem = hb_face_get_upem(face);
	float xscale = (float)font->x_scale / upem;
	float yscale = (float)font->y_scale / upem;
	yscale *= yscale / xscale;
	uint curradv = 0;
	if(HB_DIRECTION_IS_BACKWARD(buffer->props.direction)) {
		curradv = gr_slot_origin_X(gr_seg_first_slot(seg)) * xscale;
		clusters[0].advance = gr_seg_advance_X(seg) * xscale - curradv;
	}
	else
		clusters[0].advance = 0;
	for(is = gr_seg_first_slot(seg), ic = 0; is; is = gr_slot_next_in_segment(is), ic++) {
		uint before = gr_slot_before(is);
		uint after = gr_slot_after(is);
		*pg = gr_slot_gid(is);
		pg++;
		while(clusters[ci].base_char > before && ci) {
			clusters[ci-1].num_chars += clusters[ci].num_chars;
			clusters[ci-1].num_glyphs += clusters[ci].num_glyphs;
			clusters[ci-1].advance += clusters[ci].advance;
			ci--;
		}

		if(gr_slot_can_insert_before(is) && clusters[ci].num_chars && before >= clusters[ci].base_char + clusters[ci].num_chars) {
			hb_graphite2_cluster_t * c = clusters + ci + 1;
			c->base_char = clusters[ci].base_char + clusters[ci].num_chars;
			c->cluster = buffer->info[c->base_char].cluster;
			c->num_chars = before - c->base_char;
			c->base_glyph = ic;
			c->num_glyphs = 0;
			if(HB_DIRECTION_IS_BACKWARD(buffer->props.direction)) {
				c->advance = curradv - gr_slot_origin_X(is) * xscale;
				curradv -= c->advance;
			}
			else {
				c->advance = 0;
				clusters[ci].advance += gr_slot_origin_X(is) * xscale - curradv;
				curradv += clusters[ci].advance;
			}
			ci++;
		}
		clusters[ci].num_glyphs++;
		if(clusters[ci].base_char + clusters[ci].num_chars < after + 1)
			clusters[ci].num_chars = after + 1 - clusters[ci].base_char;
	}
	if(HB_DIRECTION_IS_BACKWARD(buffer->props.direction))
		clusters[ci].advance += curradv;
	else
		clusters[ci].advance += gr_seg_advance_X(seg) * xscale - curradv;
	ci++;
	for(uint i = 0; i < ci; ++i) {
		for(uint j = 0; j < clusters[i].num_glyphs; ++j) {
			hb_glyph_info_t * info = &buffer->info[clusters[i].base_glyph + j];
			info->codepoint = gids[clusters[i].base_glyph + j];
			info->cluster = clusters[i].cluster;
			info->var1.i32 = clusters[i].advance; // all glyphs in the cluster get the same advance
		}
	}
	buffer->len = glyph_count;
	/* Positioning. */
	uint currclus = UINT_MAX;
	const hb_glyph_info_t * info = buffer->info;
	hb_glyph_position_t * pPos = hb_buffer_get_glyph_positions(buffer, nullptr);
	if(!HB_DIRECTION_IS_BACKWARD(buffer->props.direction)) {
		curradvx = 0;
		for(is = gr_seg_first_slot(seg); is; pPos++, ++info, is = gr_slot_next_in_segment(is)) {
			pPos->x_offset = gr_slot_origin_X(is) * xscale - curradvx;
			pPos->y_offset = gr_slot_origin_Y(is) * yscale - curradvy;
			if(info->cluster != currclus) {
				pPos->x_advance = info->var1.i32;
				curradvx += pPos->x_advance;
				currclus = info->cluster;
			}
			else
				pPos->x_advance = 0.;
			pPos->y_advance = gr_slot_advance_Y(is, grface, nullptr) * yscale;
			curradvy += pPos->y_advance;
		}
	}
	else {
		curradvx = gr_seg_advance_X(seg) * xscale;
		for(is = gr_seg_first_slot(seg); is; pPos++, info++, is = gr_slot_next_in_segment(is)) {
			if(info->cluster != currclus) {
				pPos->x_advance = info->var1.i32;
				curradvx -= pPos->x_advance;
				currclus = info->cluster;
			}
			else
				pPos->x_advance = 0.;
			pPos->y_advance = gr_slot_advance_Y(is, grface, nullptr) * yscale;
			curradvy -= pPos->y_advance;
			pPos->x_offset = gr_slot_origin_X(is) * xscale - info->var1.i32 - curradvx + pPos->x_advance;
			pPos->y_offset = gr_slot_origin_Y(is) * yscale - curradvy;
		}
		hb_buffer_reverse_clusters(buffer);
	}
	if(feats) gr_featureval_destroy(feats);
	gr_seg_destroy(seg);
	buffer->unsafe_to_break_all();
	return true;
}

#endif
