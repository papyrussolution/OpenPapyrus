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
#ifndef HB_OT_VAR_HVAR_TABLE_HH
#define HB_OT_VAR_HVAR_TABLE_HH

#include "hb-ot-layout-common.hh"

namespace OT {
	struct DeltaSetIndexMap {
		bool sanitize(hb_sanitize_context_t * c) const
		{
			TRACE_SANITIZE(this);
			return_trace(c->check_struct(this) &&
			    c->check_range(mapDataZ.arrayZ,
			    mapCount,
			    get_width()));
		}

		template <typename T>
		bool serialize(hb_serialize_context_t * c, const T &plan)
		{
			uint width = plan.get_width();
			uint inner_bit_count = plan.get_inner_bit_count();
			const hb_array_t<const uint> output_map = plan.get_output_map();

			TRACE_SERIALIZE(this);
			if(UNLIKELY(output_map.length && ((((inner_bit_count-1)&~0xF)!=0) || (((width-1)&~0x3)!=0))))
				return_trace(false);
			if(UNLIKELY(!c->extend_min(*this))) return_trace(false);

			format = ((width-1)<<4)|(inner_bit_count-1);
			mapCount = output_map.length;
			HBUINT8 * p = c->allocate_size<HBUINT8> (width * output_map.length);
			if(UNLIKELY(!p)) return_trace(false);
			for(uint i = 0; i < output_map.length; i++) {
				uint v = output_map[i];
				uint outer = v >> 16;
				uint inner = v & 0xFFFF;
				uint u = (outer << inner_bit_count) | inner;
				for(uint w = width; w > 0;) {
					p[--w] = u;
					u >>= 8;
				}
				p += width;
			}
			return_trace(true);
		}

		uint map(uint v) const /* Returns 16.16 outer.inner. */
		{
			/* If count is zero, pass value unchanged.  This takes
			 * care of direct mapping for advance map. */
			if(!mapCount)
				return v;

			if(v >= mapCount)
				v = mapCount - 1;

			uint u = 0;
			{ /* Fetch it. */
				uint w = get_width();
				const HBUINT8 * p = mapDataZ.arrayZ + w * v;
				for(; w; w--)
					u = (u << 8) + *p++;
			}

			{ /* Repack it. */
				uint n = get_inner_bit_count();
				uint outer = u >> n;
				uint inner = u & ((1 << n) - 1);
				u = (outer<<16) | inner;
			}

			return u;
		}

		uint get_map_count() const {
			return mapCount;
		}

		uint get_width() const {
			return ((format >> 4) & 3) + 1;
		}

		uint get_inner_bit_count() const {
			return (format & 0xF) + 1;
		}

protected:
		HBUINT16 format; /* A packed field that describes the compressed
		 * representation of delta-set indices. */
		HBUINT16 mapCount; /* The number of mapping entries. */
		UnsizedArrayOf<HBUINT8>
		mapDataZ; /* The delta-set index mapping data. */

public:
		DEFINE_SIZE_ARRAY(4, mapDataZ);
	};

	struct index_map_subset_plan_t {
		enum index_map_index_t {
			ADV_INDEX,
			LSB_INDEX, /* dual as TSB */
			RSB_INDEX, /* dual as BSB */
			VORG_INDEX
		};

		void init(const DeltaSetIndexMap  &index_map,
		    hb_inc_bimap_t          &outer_map,
		    hb_vector_t<hb_set_t *> &inner_sets,
		    const hb_subset_plan_t * plan)
		{
			map_count = 0;
			outer_bit_count = 0;
			inner_bit_count = 1;
			max_inners.init();
			output_map.init();

			if(&index_map == &Null(DeltaSetIndexMap)) return;

			uint last_val = (uint)-1;
			hb_codepoint_t last_gid = (hb_codepoint_t)-1;
			hb_codepoint_t gid = (hb_codepoint_t)hb_min(index_map.get_map_count(), plan->num_output_glyphs());

			outer_bit_count = (index_map.get_width() * 8) - index_map.get_inner_bit_count();
			max_inners.resize(inner_sets.length);
			for(uint i = 0; i < inner_sets.length; i++) max_inners[i] = 0;

			/* Search backwards for a map value different from the last map value */
			for(; gid > 0; gid--) {
				hb_codepoint_t old_gid;
				if(!plan->old_gid_for_new_gid(gid - 1, &old_gid)) {
					if(last_gid == (hb_codepoint_t)-1)
						continue;
					else
						break;
				}

				uint v = index_map.map(old_gid);
				if(last_gid == (hb_codepoint_t)-1) {
					last_val = v;
					last_gid = gid;
					continue;
				}
				if(v != last_val) break;

				last_gid = gid;
			}

			if(UNLIKELY(last_gid == (hb_codepoint_t)-1)) return;
			map_count = last_gid;
			for(gid = 0; gid < map_count; gid++) {
				hb_codepoint_t old_gid;
				if(plan->old_gid_for_new_gid(gid, &old_gid)) {
					uint v = index_map.map(old_gid);
					uint outer = v >> 16;
					uint inner = v & 0xFFFF;
					outer_map.add(outer);
					if(inner > max_inners[outer]) max_inners[outer] = inner;
					if(outer >= inner_sets.length) return;
					inner_sets[outer]->add(inner);
				}
			}
		}

		void fini()
		{
			max_inners.fini();
			output_map.fini();
		}

		void remap(const DeltaSetIndexMap * input_map,
		    const hb_inc_bimap_t &outer_map,
		    const hb_vector_t<hb_inc_bimap_t> &inner_maps,
		    const hb_subset_plan_t * plan)
		{
			if(input_map == &Null(DeltaSetIndexMap)) return;

			for(uint i = 0; i < max_inners.length; i++) {
				if(inner_maps[i].get_population() == 0) 
					continue;
				uint bit_count = (max_inners[i]==0) ? 1 : hb_bit_storage(inner_maps[i][max_inners[i]]);
				if(bit_count > inner_bit_count) inner_bit_count = bit_count;
			}

			output_map.resize(map_count);
			for(hb_codepoint_t gid = 0; gid < output_map.length; gid++) {
				hb_codepoint_t old_gid;
				if(plan->old_gid_for_new_gid(gid, &old_gid)) {
					uint v = input_map->map(old_gid);
					uint outer = v >> 16;
					output_map[gid] = (outer_map[outer] << 16) | (inner_maps[outer][v & 0xFFFF]);
				}
				else
					output_map[gid] = 0; /* Map unused glyph to outer/inner=0/0 */
			}
		}
		uint get_inner_bit_count() const { return inner_bit_count; }
		uint get_width() const { return ((outer_bit_count + inner_bit_count + 7) / 8); }
		uint get_map_count() const { return map_count; }
		uint get_size() const { return (map_count ? (DeltaSetIndexMap::min_size + get_width() * map_count) : 0); }
		bool is_identity() const { return get_output_map().length == 0; }
		hb_array_t<const uint> get_output_map() const { return output_map.as_array(); }
protected:
		uint map_count;
		hb_vector_t<uint> max_inners;
		uint outer_bit_count;
		uint inner_bit_count;
		hb_vector_t<uint> output_map;
	};

	struct hvarvvar_subset_plan_t {
		hvarvvar_subset_plan_t() : inner_maps(), index_map_plans() 
		{
		}
		~hvarvvar_subset_plan_t() 
		{
			fini();
		}
		void init(const hb_array_t<const DeltaSetIndexMap *> &index_maps, const VariationStore &_var_store, const hb_subset_plan_t * plan)
		{
			index_map_plans.resize(index_maps.length);
			var_store = &_var_store;
			inner_sets.resize(var_store->get_sub_table_count());
			for(uint i = 0; i < inner_sets.length; i++)
				inner_sets[i] = hb_set_create();
			adv_set = hb_set_create();
			inner_maps.resize(var_store->get_sub_table_count());
			for(uint i = 0; i < inner_maps.length; i++)
				inner_maps[i].init();
			if(UNLIKELY(!index_map_plans.length || !inner_sets.length || !inner_maps.length)) return;
			bool retain_adv_map = false;
			index_map_plans[0].init(*index_maps[0], outer_map, inner_sets, plan);
			if(index_maps[0] == &Null(DeltaSetIndexMap)) {
				retain_adv_map = plan->retain_gids;
				outer_map.add(0);
				for(hb_codepoint_t gid = 0; gid < plan->num_output_glyphs(); gid++) {
					hb_codepoint_t old_gid;
					if(plan->old_gid_for_new_gid(gid, &old_gid))
						inner_sets[0]->add(old_gid);
				}
				hb_set_union(adv_set, inner_sets[0]);
			}

			for(uint i = 1; i < index_maps.length; i++)
				index_map_plans[i].init(*index_maps[i], outer_map, inner_sets, plan);
			outer_map.sort();
			if(retain_adv_map) {
				for(hb_codepoint_t gid = 0; gid < plan->num_output_glyphs(); gid++)
					if(inner_sets[0]->has(gid))
						inner_maps[0].add(gid);
					else
						inner_maps[0].skip();
			}
			else {
				inner_maps[0].add_set(adv_set);
				hb_set_subtract(inner_sets[0], adv_set);
				inner_maps[0].add_set(inner_sets[0]);
			}
			for(uint i = 1; i < inner_maps.length; i++)
				inner_maps[i].add_set(inner_sets[i]);
			for(uint i = 0; i < index_maps.length; i++)
				index_map_plans[i].remap(index_maps[i], outer_map, inner_maps, plan);
		}
		void fini()
		{
			for(uint i = 0; i < inner_sets.length; i++)
				hb_set_destroy(inner_sets[i]);
			hb_set_destroy(adv_set);
			inner_maps.fini_deep();
			index_map_plans.fini_deep();
		}
		hb_inc_bimap_t outer_map;
		hb_vector_t<hb_inc_bimap_t> inner_maps;
		hb_vector_t<index_map_subset_plan_t> index_map_plans;
		const VariationStore * var_store;
protected:
		hb_vector_t<hb_set_t *> inner_sets;
		hb_set_t * adv_set;
	};

/*
 * HVAR -- Horizontal Metrics Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/hvar
 * VVAR -- Vertical Metrics Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/vvar
 */
#define HB_OT_TAG_HVAR HB_TAG('H', 'V', 'A', 'R')
#define HB_OT_TAG_VVAR HB_TAG('V', 'V', 'A', 'R')

	struct HVARVVAR {
		static constexpr hb_tag_t HVARTag = HB_OT_TAG_HVAR;
		static constexpr hb_tag_t VVARTag = HB_OT_TAG_VVAR;

		bool sanitize(hb_sanitize_context_t * c) const
		{
			TRACE_SANITIZE(this);
			return_trace(version.sanitize(c) &&
			    LIKELY(version.major == 1) &&
			    varStore.sanitize(c, this) &&
			    advMap.sanitize(c, this) &&
			    lsbMap.sanitize(c, this) &&
			    rsbMap.sanitize(c, this));
		}

		void listup_index_maps(hb_vector_t<const DeltaSetIndexMap *> &index_maps) const
		{
			index_maps.push(&(this+advMap));
			index_maps.push(&(this+lsbMap));
			index_maps.push(&(this+rsbMap));
		}

		bool serialize_index_maps(hb_serialize_context_t * c, const hb_array_t<index_map_subset_plan_t> &im_plans)
		{
			TRACE_SERIALIZE(this);
			if(im_plans[index_map_subset_plan_t::ADV_INDEX].is_identity())
				advMap = 0;
			else if(UNLIKELY(!advMap.serialize(c, this).serialize(c, im_plans[index_map_subset_plan_t::ADV_INDEX])))
				return_trace(false);
			if(im_plans[index_map_subset_plan_t::LSB_INDEX].is_identity())
				lsbMap = 0;
			else if(UNLIKELY(!lsbMap.serialize(c, this).serialize(c, im_plans[index_map_subset_plan_t::LSB_INDEX])))
				return_trace(false);
			if(im_plans[index_map_subset_plan_t::RSB_INDEX].is_identity())
				rsbMap = 0;
			else if(UNLIKELY(!rsbMap.serialize(c, this).serialize(c, im_plans[index_map_subset_plan_t::RSB_INDEX])))
				return_trace(false);

			return_trace(true);
		}

		template <typename T> bool _subset(hb_subset_context_t * c) const
		{
			TRACE_SUBSET(this);
			hvarvvar_subset_plan_t hvar_plan;
			hb_vector_t<const DeltaSetIndexMap *> index_maps;
			((T*)this)->listup_index_maps(index_maps);
			hvar_plan.init(index_maps.as_array(), this+varStore, c->plan);

			T * out = c->serializer->allocate_min<T> ();
			if(UNLIKELY(!out)) return_trace(false);

			out->version.major = 1;
			out->version.minor = 0;

			if(UNLIKELY(!out->varStore.serialize(c->serializer, out)
			    .serialize(c->serializer, hvar_plan.var_store, hvar_plan.inner_maps.as_array())))
				return_trace(false);

			return_trace(out->T::serialize_index_maps(c->serializer,
			    hvar_plan.index_map_plans.as_array()));
		}

		float get_advance_var(hb_codepoint_t glyph, hb_font_t * font) const
		{
			uint varidx = (this+advMap).map(glyph);
			return (this+varStore).get_delta(varidx, font->coords, font->num_coords);
		}

		float get_side_bearing_var(hb_codepoint_t glyph,
		    const int * coords, uint coord_count) const
		{
			if(!has_side_bearing_deltas()) return 0.f;
			uint varidx = (this+lsbMap).map(glyph);
			return (this+varStore).get_delta(varidx, coords, coord_count);
		}

		bool has_side_bearing_deltas() const {
			return lsbMap && rsbMap;
		}

protected:
		FixedVersion<>version; /* Version of the metrics variation table
		 * initially set to 0x00010000u */
		LOffsetTo<VariationStore>
		varStore; /* Offset to item variation store table. */
		LOffsetTo<DeltaSetIndexMap>
		advMap; /* Offset to advance var-idx mapping. */
		LOffsetTo<DeltaSetIndexMap>
		lsbMap; /* Offset to lsb/tsb var-idx mapping. */
		LOffsetTo<DeltaSetIndexMap>
		rsbMap; /* Offset to rsb/bsb var-idx mapping. */

public:
		DEFINE_SIZE_STATIC(20);
	};

	struct HVAR : HVARVVAR {
		static constexpr hb_tag_t tableTag = HB_OT_TAG_HVAR;
		bool subset(hb_subset_context_t * c) const {
			return HVARVVAR::_subset<HVAR> (c);
		}
	};
	struct VVAR : HVARVVAR {
		static constexpr hb_tag_t tableTag = HB_OT_TAG_VVAR;

		bool sanitize(hb_sanitize_context_t * c) const
		{
			TRACE_SANITIZE(this);
			return_trace(static_cast<const HVARVVAR *> (this)->sanitize(c) &&
			    vorgMap.sanitize(c, this));
		}

		void listup_index_maps(hb_vector_t<const DeltaSetIndexMap *> &index_maps) const
		{
			HVARVVAR::listup_index_maps(index_maps);
			index_maps.push(&(this+vorgMap));
		}

		bool serialize_index_maps(hb_serialize_context_t * c,
		    const hb_array_t<index_map_subset_plan_t> &im_plans)
		{
			TRACE_SERIALIZE(this);
			if(UNLIKELY(!HVARVVAR::serialize_index_maps(c, im_plans)))
				return_trace(false);
			if(!im_plans[index_map_subset_plan_t::VORG_INDEX].get_map_count())
				vorgMap = 0;
			else if(UNLIKELY(!vorgMap.serialize(c, this).serialize(c, im_plans[index_map_subset_plan_t::VORG_INDEX])))
				return_trace(false);

			return_trace(true);
		}

		bool subset(hb_subset_context_t * c) const {
			return HVARVVAR::_subset<VVAR> (c);
		}

protected:
		LOffsetTo<DeltaSetIndexMap>
		vorgMap; /* Offset to vertical-origin var-idx mapping. */

public:
		DEFINE_SIZE_STATIC(24);
	};
} /* namespace OT */

#endif /* HB_OT_VAR_HVAR_TABLE_HH */
