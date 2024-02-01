/*
 * Copyright © 2009,2010  Red Hat, Inc.
 * Copyright © 2010,2011,2012,2013  Google, Inc.
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
#ifndef HB_OT_MAP_HH
#define HB_OT_MAP_HH

#include "hb-buffer.hh"

#define HB_OT_MAP_MAX_BITS 8u
#define HB_OT_MAP_MAX_VALUE ((1u << HB_OT_MAP_MAX_BITS) - 1u)

struct hb_ot_shape_plan_t;

static const hb_tag_t table_tags[2] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS};

struct hb_ot_map_t {
	friend struct hb_ot_map_builder_t;

public:

	struct feature_map_t {
		hb_tag_t tag; /* should be first for our bsearch to work */
		uint index[2]; /* GSUB/GPOS */
		uint stage[2]; /* GSUB/GPOS */
		uint shift;
		hb_mask_t mask;
		hb_mask_t _1_mask; /* mask for value=1, for quick access */
		uint needs_fallback : 1;
		uint auto_zwnj : 1;
		uint auto_zwj : 1;
		uint random : 1;

		int cmp(const hb_tag_t tag_) const
		{
			return CMPSIGN(tag_, tag); // @v11.9.3 
			// @v11.9.3 return tag_ < tag ? -1 : tag_ > tag ? 1 : 0;
		}
	};

	struct lookup_map_t {
		ushort index;
		ushort auto_zwnj : 1;
		ushort auto_zwj : 1;
		ushort random : 1;
		hb_mask_t mask;
		HB_INTERNAL static int cmp(const void * pa, const void * pb)
		{
			const lookup_map_t * a = (const lookup_map_t*)pa;
			const lookup_map_t * b = (const lookup_map_t*)pb;
			return CMPSIGN(a->index, b->index); // @v11.9.3
			// @v11.9.3 return a->index < b->index ? -1 : a->index > b->index ? 1 : 0;
		}
	};

	typedef void (* pause_func_t) (const struct hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer);

	struct stage_map_t {
		uint last_lookup; /* Cumulative */
		pause_func_t pause_func;
	};
	void init()
	{
		THISZERO();
		features.init();
		for(uint table_index = 0; table_index < 2; table_index++) {
			lookups[table_index].init();
			stages[table_index].init();
		}
	}
	void fini()
	{
		features.fini();
		for(uint table_index = 0; table_index < 2; table_index++) {
			lookups[table_index].fini();
			stages[table_index].fini();
		}
	}

	hb_mask_t get_global_mask() const {
		return global_mask;
	}

	hb_mask_t get_mask(hb_tag_t feature_tag, uint * shift = nullptr) const
	{
		const feature_map_t * map = features.bsearch(feature_tag);
		if(shift) *shift = map ? map->shift : 0;
		return map ? map->mask : 0;
	}

	bool needs_fallback(hb_tag_t feature_tag) const
	{
		const feature_map_t * map = features.bsearch(feature_tag);
		return map ? map->needs_fallback : false;
	}

	hb_mask_t get_1_mask(hb_tag_t feature_tag) const
	{
		const feature_map_t * map = features.bsearch(feature_tag);
		return map ? map->_1_mask : 0;
	}

	uint get_feature_index(uint table_index, hb_tag_t feature_tag) const
	{
		const feature_map_t * map = features.bsearch(feature_tag);
		return map ? map->index[table_index] : HB_OT_LAYOUT_NO_FEATURE_INDEX;
	}

	uint get_feature_stage(uint table_index, hb_tag_t feature_tag) const
	{
		const feature_map_t * map = features.bsearch(feature_tag);
		return map ? map->stage[table_index] : UINT_MAX;
	}

	void get_stage_lookups(uint table_index, uint stage,
	    const struct lookup_map_t ** plookups, uint * lookup_count) const
	{
		if(UNLIKELY(stage == UINT_MAX)) {
			* plookups = nullptr;
			* lookup_count = 0;
			return;
		}
		assert(stage <= stages[table_index].length);
		uint start = stage ? stages[table_index][stage - 1].last_lookup : 0;
		uint end   = stage <
		    stages[table_index].length ? stages[table_index][stage].last_lookup : lookups[table_index].length;
		* plookups = end == start ? nullptr : &lookups[table_index][start];
		* lookup_count = end - start;
	}

	HB_INTERNAL void collect_lookups(uint table_index, hb_set_t * lookups) const;
	template <typename Proxy>
	HB_INTERNAL void apply(const Proxy &proxy,
	    const struct hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer) const;
	HB_INTERNAL void substitute(const struct hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer) const;
	HB_INTERNAL void position(const struct hb_ot_shape_plan_t * plan, hb_font_t * font, hb_buffer_t * buffer) const;

public:
	hb_tag_t chosen_script[2];
	bool found_script[2];

private:

	hb_mask_t global_mask;

	hb_sorted_vector_t<feature_map_t> features;
	hb_vector_t<lookup_map_t> lookups[2]; /* GSUB/GPOS */
	hb_vector_t<stage_map_t> stages[2]; /* GSUB/GPOS */
};

enum hb_ot_map_feature_flags_t {
	F_NONE        = 0x0000u,
	F_GLOBAL      = 0x0001u,/* Feature applies to all characters; results in no mask allocated for it. */
	F_HAS_FALLBACK        = 0x0002u,/* Has fallback implementation, so include mask bit even if feature not found. */
	F_MANUAL_ZWNJ = 0x0004u,/* Don't skip over ZWNJ when matching **context**. */
	F_MANUAL_ZWJ  = 0x0008u,/* Don't skip over ZWJ when matching **input**. */
	F_MANUAL_JOINERS      = F_MANUAL_ZWNJ | F_MANUAL_ZWJ,
	F_GLOBAL_MANUAL_JOINERS = F_GLOBAL | F_MANUAL_JOINERS,
	F_GLOBAL_HAS_FALLBACK = F_GLOBAL | F_HAS_FALLBACK,
	F_GLOBAL_SEARCH       = 0x0010u,/* If feature not found in LangSys, look for it in global feature list and pick one. */
	F_RANDOM      = 0x0020u/* Randomly select a glyph from an AlternateSubstFormat1 subtable. */
};

HB_MARK_AS_FLAG_T(hb_ot_map_feature_flags_t);

struct hb_ot_map_feature_t {
	hb_tag_t tag;
	hb_ot_map_feature_flags_t flags;
};

struct hb_ot_shape_plan_key_t;

struct hb_ot_map_builder_t {
public:

	HB_INTERNAL hb_ot_map_builder_t(hb_face_t * face_,
	    const hb_segment_properties_t * props_);

	HB_INTERNAL ~hb_ot_map_builder_t ();

	HB_INTERNAL void add_feature(hb_tag_t tag,
	    hb_ot_map_feature_flags_t flags = F_NONE,
	    uint value = 1);

	void add_feature(const hb_ot_map_feature_t &feat)
	{
		add_feature(feat.tag, feat.flags);
	}

	void enable_feature(hb_tag_t tag,
	    hb_ot_map_feature_flags_t flags = F_NONE,
	    uint value = 1)
	{
		add_feature(tag, F_GLOBAL | flags, value);
	}

	void disable_feature(hb_tag_t tag)
	{
		add_feature(tag, F_GLOBAL, 0);
	}

	void add_gsub_pause(hb_ot_map_t::pause_func_t pause_func)
	{
		add_pause(0, pause_func);
	}

	void add_gpos_pause(hb_ot_map_t::pause_func_t pause_func)
	{
		add_pause(1, pause_func);
	}

	HB_INTERNAL void compile(hb_ot_map_t                  &m,
	    const hb_ot_shape_plan_key_t &key);

private:

	HB_INTERNAL void add_lookups(hb_ot_map_t  &m,
	    uint table_index,
	    uint feature_index,
	    uint variations_index,
	    hb_mask_t mask,
	    bool auto_zwnj = true,
	    bool auto_zwj = true,
	    bool random = false);

	struct feature_info_t {
		hb_tag_t tag;
		uint seq; /* sequence#, used for stable sorting only */
		uint max_value;
		hb_ot_map_feature_flags_t flags;
		uint default_value; /* for non-global features, what should the unset glyphs take */
		uint stage[2]; /* GSUB/GPOS */

		HB_INTERNAL static int cmp(const void * pa, const void * pb)
		{
			const feature_info_t * a = (const feature_info_t*)pa;
			const feature_info_t * b = (const feature_info_t*)pb;
			return (a->tag != b->tag) ?  (a->tag < b->tag ? -1 : 1) :
			       (a->seq < b->seq ? -1 : a->seq > b->seq ? 1 : 0);
		}
	};

	struct stage_info_t {
		uint index;
		hb_ot_map_t::pause_func_t pause_func;
	};

	HB_INTERNAL void add_pause(uint table_index, hb_ot_map_t::pause_func_t pause_func);

public:

	hb_face_t * face;
	hb_segment_properties_t props;

	hb_tag_t chosen_script[2];
	bool found_script[2];
	uint script_index[2], language_index[2];

private:

	uint current_stage[2]; /* GSUB/GPOS */
	hb_vector_t<feature_info_t> feature_infos;
	hb_vector_t<stage_info_t> stages[2]; /* GSUB/GPOS */
};

#endif /* HB_OT_MAP_HH */
