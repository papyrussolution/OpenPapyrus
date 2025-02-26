/*
 * Copyright © 2009,2010  Red Hat, Inc.
 * Copyright © 2010,2011,2013  Google, Inc.
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

#ifndef HB_NO_OT_SHAPE

void hb_ot_map_t::collect_lookups(uint table_index, hb_set_t * lookups_out) const
{
	for(uint i = 0; i < lookups[table_index].length; i++)
		lookups_out->add(lookups[table_index][i].index);
}

hb_ot_map_builder_t::hb_ot_map_builder_t(hb_face_t * face_, const hb_segment_properties_t * props_)
{
	THISZERO();
	feature_infos.init();
	for(uint table_index = 0; table_index < 2; table_index++)
		stages[table_index].init();
	face = face_;
	props = *props_;
	/* Fetch script/language indices for GSUB/GPOS.  We need these later to skip
	 * features not available in either table and not waste precious bits for them. */
	uint script_count = HB_OT_MAX_TAGS_PER_SCRIPT;
	uint language_count = HB_OT_MAX_TAGS_PER_LANGUAGE;
	hb_tag_t script_tags[HB_OT_MAX_TAGS_PER_SCRIPT];
	hb_tag_t language_tags[HB_OT_MAX_TAGS_PER_LANGUAGE];
	hb_ot_tags_from_script_and_language(props.script, props.language, &script_count, script_tags, &language_count, language_tags);
	for(uint table_index = 0; table_index < 2; table_index++) {
		hb_tag_t table_tag = table_tags[table_index];
		found_script[table_index] = (bool)hb_ot_layout_table_select_script(face, table_tag, script_count,
			script_tags, &script_index[table_index], &chosen_script[table_index]);
		hb_ot_layout_script_select_language(face, table_tag, script_index[table_index], language_count,
		    language_tags, &language_index[table_index]);
	}
}

hb_ot_map_builder_t::~hb_ot_map_builder_t ()
{
	feature_infos.fini();
	for(uint table_index = 0; table_index < 2; table_index++)
		stages[table_index].fini();
}

void hb_ot_map_builder_t::add_feature(hb_tag_t tag, hb_ot_map_feature_flags_t flags, uint value)
{
	if(UNLIKELY(!tag)) return;
	feature_info_t * info = feature_infos.push();
	info->tag = tag;
	info->seq = feature_infos.length;
	info->max_value = value;
	info->flags = flags;
	info->default_value = (flags & F_GLOBAL) ? value : 0;
	info->stage[0] = current_stage[0];
	info->stage[1] = current_stage[1];
}

void hb_ot_map_builder_t::add_lookups(hb_ot_map_t  &m, uint table_index, uint feature_index,
    uint variations_index, hb_mask_t mask, bool auto_zwnj, bool auto_zwj, bool random)
{
	uint lookup_indices[32];
	uint len;
	uint table_lookup_count = hb_ot_layout_table_get_lookup_count(face, table_tags[table_index]);
	uint offset = 0;
	do {
		len = ARRAY_LENGTH(lookup_indices);
		hb_ot_layout_feature_with_variations_get_lookups(face, table_tags[table_index], feature_index,
		    variations_index, offset, &len, lookup_indices);
		for(uint i = 0; i < len; i++) {
			if(lookup_indices[i] >= table_lookup_count)
				continue;
			hb_ot_map_t::lookup_map_t * lookup = m.lookups[table_index].push();
			lookup->mask = mask;
			lookup->index = lookup_indices[i];
			lookup->auto_zwnj = auto_zwnj;
			lookup->auto_zwj = auto_zwj;
			lookup->random = random;
		}
		offset += len;
	} while(len == ARRAY_LENGTH(lookup_indices));
}

void hb_ot_map_builder_t::add_pause(uint table_index, hb_ot_map_t::pause_func_t pause_func)
{
	stage_info_t * s = stages[table_index].push();
	s->index = current_stage[table_index];
	s->pause_func = pause_func;
	current_stage[table_index]++;
}

void hb_ot_map_builder_t::compile(hb_ot_map_t &m, const hb_ot_shape_plan_key_t &key)
{
	static_assert((!(HB_GLYPH_FLAG_DEFINED & (HB_GLYPH_FLAG_DEFINED + 1))), "");
	uint global_bit_mask = HB_GLYPH_FLAG_DEFINED + 1;
	uint global_bit_shift = /*hb_popcount*/SBits::Cpop(static_cast<uint>(HB_GLYPH_FLAG_DEFINED));
	m.global_mask = global_bit_mask;
	uint required_feature_index[2];
	hb_tag_t required_feature_tag[2];
	/* We default to applying required feature in stage 0.  If the required
	 * feature has a tag that is known to the shaper, we apply required feature
	 * in the stage for that tag.
	 */
	uint required_feature_stage[2] = {0, 0};
	for(uint table_index = 0; table_index < 2; table_index++) {
		m.chosen_script[table_index] = chosen_script[table_index];
		m.found_script[table_index] = found_script[table_index];
		hb_ot_layout_language_get_required_feature(face,
		    table_tags[table_index],
		    script_index[table_index],
		    language_index[table_index],
		    &required_feature_index[table_index],
		    &required_feature_tag[table_index]);
	}
	/* Sort features and merge duplicates */
	if(feature_infos.length) {
		feature_infos.qsort();
		uint j = 0;
		for(uint i = 1; i < feature_infos.length; i++)
			if(feature_infos[i].tag != feature_infos[j].tag)
				feature_infos[++j] = feature_infos[i];
			else {
				if(feature_infos[i].flags & F_GLOBAL) {
					feature_infos[j].flags |= F_GLOBAL;
					feature_infos[j].max_value = feature_infos[i].max_value;
					feature_infos[j].default_value = feature_infos[i].default_value;
				}
				else {
					if(feature_infos[j].flags & F_GLOBAL)
						feature_infos[j].flags ^= F_GLOBAL;
					feature_infos[j].max_value = hb_max(feature_infos[j].max_value, feature_infos[i].max_value);
					/* Inherit default_value from j */
				}
				feature_infos[j].flags |= (feature_infos[i].flags & F_HAS_FALLBACK);
				feature_infos[j].stage[0] = hb_min(feature_infos[j].stage[0], feature_infos[i].stage[0]);
				feature_infos[j].stage[1] = hb_min(feature_infos[j].stage[1], feature_infos[i].stage[1]);
			}
		feature_infos.shrink(j + 1);
	}

	/* Allocate bits now */
	uint next_bit = global_bit_shift + 1;
	for(uint i = 0; i < feature_infos.length; i++) {
		const feature_info_t * info = &feature_infos[i];
		uint bits_needed;
		if((info->flags & F_GLOBAL) && info->max_value == 1)
			/* Uses the global bit */
			bits_needed = 0;
		else
			/* Limit bits per feature. */
			bits_needed = hb_min(HB_OT_MAP_MAX_BITS, hb_bit_storage(info->max_value));
		if(!info->max_value || next_bit + bits_needed > 8 * sizeof(hb_mask_t))
			continue; /* Feature disabled, or not enough bits. */
		bool found = false;
		uint feature_index[2];
		for(uint table_index = 0; table_index < 2; table_index++) {
			if(required_feature_tag[table_index] == info->tag)
				required_feature_stage[table_index] = info->stage[table_index];
			found |= (bool)hb_ot_layout_language_find_feature(face, table_tags[table_index], script_index[table_index], language_index[table_index], info->tag, &feature_index[table_index]);
		}
		if(!found && (info->flags & F_GLOBAL_SEARCH)) {
			for(uint table_index = 0; table_index < 2; table_index++) {
				found |= (bool)hb_ot_layout_table_find_feature(face, table_tags[table_index], info->tag, &feature_index[table_index]);
			}
		}
		if(!found && !(info->flags & F_HAS_FALLBACK))
			continue;
		hb_ot_map_t::feature_map_t * map = m.features.push();
		map->tag = info->tag;
		map->index[0] = feature_index[0];
		map->index[1] = feature_index[1];
		map->stage[0] = info->stage[0];
		map->stage[1] = info->stage[1];
		map->auto_zwnj = !(info->flags & F_MANUAL_ZWNJ);
		map->auto_zwj = !(info->flags & F_MANUAL_ZWJ);
		map->random = !!(info->flags & F_RANDOM);
		if((info->flags & F_GLOBAL) && info->max_value == 1) {
			/* Uses the global bit */
			map->shift = global_bit_shift;
			map->mask = global_bit_mask;
		}
		else {
			map->shift = next_bit;
			map->mask = (1u << (next_bit + bits_needed)) - (1u << next_bit);
			next_bit += bits_needed;
			m.global_mask |= (info->default_value << map->shift) & map->mask;
		}
		map->_1_mask = (1u << map->shift) & map->mask;
		map->needs_fallback = !found;
	}
	feature_infos.shrink(0); /* Done with these */
	add_gsub_pause(nullptr);
	add_gpos_pause(nullptr);
	for(uint table_index = 0; table_index < 2; table_index++) {
		/* Collect lookup indices for features */
		uint stage_index = 0;
		uint last_num_lookups = 0;
		for(uint stage = 0; stage < current_stage[table_index]; stage++) {
			if(required_feature_index[table_index] != HB_OT_LAYOUT_NO_FEATURE_INDEX && required_feature_stage[table_index] == stage)
				add_lookups(m, table_index, required_feature_index[table_index], key.variations_index[table_index], global_bit_mask);

			for(uint i = 0; i < m.features.length; i++)
				if(m.features[i].stage[table_index] == stage)
					add_lookups(m, table_index, m.features[i].index[table_index], key.variations_index[table_index],
					    m.features[i].mask, m.features[i].auto_zwnj, m.features[i].auto_zwj, m.features[i].random);

			/* Sort lookups and merge duplicates */
			if(last_num_lookups < m.lookups[table_index].length) {
				m.lookups[table_index].qsort(last_num_lookups, m.lookups[table_index].length);
				uint j = last_num_lookups;
				for(uint i = j + 1; i < m.lookups[table_index].length; i++)
					if(m.lookups[table_index][i].index != m.lookups[table_index][j].index)
						m.lookups[table_index][++j] = m.lookups[table_index][i];
					else {
						m.lookups[table_index][j].mask |= m.lookups[table_index][i].mask;
						m.lookups[table_index][j].auto_zwnj &= m.lookups[table_index][i].auto_zwnj;
						m.lookups[table_index][j].auto_zwj &= m.lookups[table_index][i].auto_zwj;
					}
				m.lookups[table_index].shrink(j + 1);
			}
			last_num_lookups = m.lookups[table_index].length;
			if(stage_index < stages[table_index].length && stages[table_index][stage_index].index == stage) {
				hb_ot_map_t::stage_map_t * stage_map = m.stages[table_index].push();
				stage_map->last_lookup = last_num_lookups;
				stage_map->pause_func = stages[table_index][stage_index].pause_func;
				stage_index++;
			}
		}
	}
}

#endif
