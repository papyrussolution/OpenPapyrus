/*
 * Copyright © 2016  Google, Inc.
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
#ifndef HB_OT_POST_TABLE_HH
#define HB_OT_POST_TABLE_HH

#include "hb-open-type.hh"

#define HB_STRING_ARRAY_NAME format1_names
#define HB_STRING_ARRAY_LIST "hb-ot-post-macroman.hh"
#include "hb-string-array.hh"
#undef HB_STRING_ARRAY_LIST
#undef HB_STRING_ARRAY_NAME

/*
 * post -- PostScript
 * https://docs.microsoft.com/en-us/typography/opentype/spec/post
 */
#define HB_OT_TAG_post HB_TAG('p', 'o', 's', 't')

namespace OT {
	struct postV2Tail {
		friend struct post;

		bool sanitize(hb_sanitize_context_t * c) const
		{
			TRACE_SANITIZE(this);
			return_trace(glyphNameIndex.sanitize(c));
		}

protected:
		ArrayOf<HBUINT16>     glyphNameIndex; /* This is not an offset, but is the
		        * ordinal number of the glyph in 'post'
		        * string tables. */
/*UnsizedArrayOf<HBUINT8>
                        namesX;*/	/* Glyph names with length bytes [variable]
 * (a Pascal string). */

public:
		DEFINE_SIZE_ARRAY(2, glyphNameIndex);
	};

	struct post {
		static constexpr hb_tag_t tableTag = HB_OT_TAG_post;

		void serialize(hb_serialize_context_t * c) const
		{
			post * post_prime = c->allocate_min<post> ();
			if(UNLIKELY(!post_prime)) return;

			memcpy(post_prime, this, post::min_size);
			post_prime->version.major = 3; // Version 3 does not have any glyph names.
		}

		bool subset(hb_subset_context_t * c) const
		{
			TRACE_SUBSET(this);
			post * post_prime = c->serializer->start_embed<post> ();
			if(UNLIKELY(!post_prime)) return_trace(false);

			serialize(c->serializer);
			if(c->serializer->in_error() || c->serializer->ran_out_of_room) return_trace(false);

			return_trace(true);
		}

		struct accelerator_t {
			void init(hb_face_t * face)
			{
				index_to_offset.init();
				table = hb_sanitize_context_t().reference_table<post> (face);
				uint table_length = table.get_length();
				version = table->version.to_int();
				if(version != 0x00020000) 
					return;
				const postV2Tail &v2 = table->v2X;
				glyphNameIndex = &v2.glyphNameIndex;
				pool = &StructAfter<uint8> (v2.glyphNameIndex);
				const uint8 * end = (const uint8 *)(const void*)table + table_length;
				for(const uint8 * data = pool; index_to_offset.length < 65535 && data < end && data + *data < end; data += 1 + *data)
					index_to_offset.push(data - pool);
			}
			void fini()
			{
				index_to_offset.fini();
				SAlloc::F(gids_sorted_by_name.get());
				table.destroy();
			}
			bool get_glyph_name(hb_codepoint_t glyph, char * buf, uint buf_len) const
			{
				hb_bytes_t s = find_glyph_name(glyph);
				if(!s.length) 
					return false;
				else {
					if(buf_len) {
						// @sobolev uint len = hb_min(buf_len - 1, s.length);
						// @sobolev strncpy(buf, s.arrayZ, len);
						// @sobolev buf[len] = '\0';
						strnzcpy(buf, s.arrayZ, buf_len); // @sobolev
					}
					return true;
				}
			}
			bool get_glyph_from_name(const char * name, int len, hb_codepoint_t * glyph) const
			{
				uint count = get_glyph_count();
				if(UNLIKELY(!count)) return false;
				if(len < 0) len = strlen(name);
				if(UNLIKELY(!len)) return false;
retry:
				uint16_t *gids = gids_sorted_by_name.get();
				if(UNLIKELY(!gids)) {
					gids = (uint16_t*)SAlloc::M(count * sizeof(gids[0]));
					if(UNLIKELY(!gids))
						return false; /* Anything better?! */
					for(uint i = 0; i < count; i++)
						gids[i] = i;
					hb_qsort(gids, count, sizeof(gids[0]), cmp_gids, (void *)this);
					if(UNLIKELY(!gids_sorted_by_name.cmpexch(nullptr, gids))) {
						SAlloc::F(gids);
						goto retry;
					}
				}
				hb_bytes_t st(name, len);
				auto* gid = hb_bsearch(st, gids, count, sizeof(gids[0]), cmp_key, (void *)this);
				if(gid) {
					* glyph = *gid;
					return true;
				}
				return false;
			}
			hb_blob_ptr_t<post> table;
protected:
			uint get_glyph_count() const
			{
				if(version == 0x00010000)
					return format1_names_length;
				if(version == 0x00020000)
					return glyphNameIndex->len;
				return 0;
			}
			static int cmp_gids(const void * pa, const void * pb, void * arg)
			{
				const accelerator_t * thiz = (const accelerator_t*)arg;
				uint16_t a = *(const uint16_t*)pa;
				uint16_t b = *(const uint16_t*)pb;
				return thiz->find_glyph_name(b).cmp(thiz->find_glyph_name(a));
			}

			static int cmp_key(const void * pk, const void * po, void * arg)
			{
				const accelerator_t * thiz = (const accelerator_t*)arg;
				const hb_bytes_t * key = (const hb_bytes_t*)pk;
				uint16_t o = *(const uint16_t*)po;
				return thiz->find_glyph_name(o).cmp(*key);
			}

			hb_bytes_t find_glyph_name(hb_codepoint_t glyph) const
			{
				if(version == 0x00010000) {
					if(glyph >= format1_names_length)
						return hb_bytes_t();

					return format1_names(glyph);
				}

				if(version != 0x00020000 || glyph >= glyphNameIndex->len)
					return hb_bytes_t();

				uint index = glyphNameIndex->arrayZ[glyph];
				if(index < format1_names_length)
					return format1_names(index);
				index -= format1_names_length;

				if(index >= index_to_offset.length)
					return hb_bytes_t();
				uint offset = index_to_offset[index];

				const uint8 * data = pool + offset;
				uint name_length = *data;
				data++;

				return hb_bytes_t((const char *)data, name_length);
			}
private:
			uint32_t version;
			const ArrayOf<HBUINT16> * glyphNameIndex;
			hb_vector_t<uint32_t> index_to_offset;
			const uint8 * pool;
			hb_atomic_ptr_t<uint16_t *> gids_sorted_by_name;
		};

		bool has_data() const { return version.to_int(); }
		bool sanitize(hb_sanitize_context_t * c) const
		{
			TRACE_SANITIZE(this);
			return_trace(LIKELY(c->check_struct(this) && (version.to_int() == 0x00010000 || (version.to_int() == 0x00020000 && v2X.sanitize(c)) || version.to_int() == 0x00030000)));
		}
public:
		FixedVersion<>version; /* 0x00010000 for version 1.0
		 * 0x00020000 for version 2.0
		 * 0x00025000 for version 2.5 (deprecated)
		 * 0x00030000 for version 3.0 */
		HBFixed italicAngle; /* Italic angle in counter-clockwise degrees
		 * from the vertical. Zero for upright text,
		 * negative for text that leans to the right
		 * (forward). */
		FWORD underlinePosition; /* This is the suggested distance of the top
		  * of the underline from the baseline
		  * (negative values indicate below baseline).
		  * The PostScript definition of this FontInfo
		  * dictionary key (the y coordinate of the
		  * center of the stroke) is not used for
		  * historical reasons. The value of the
		  * PostScript key may be calculated by
		  * subtracting half the underlineThickness
		  * from the value of this field. */
		FWORD underlineThickness; /* Suggested values for the underline
		                             thickness. */
		HBUINT32 isFixedPitch; /* Set to 0 if the font is proportionally
		 * spaced, non-zero if the font is not
		 * proportionally spaced (i.e. monospaced). */
		HBUINT32 minMemType42; /* Minimum memory usage when an OpenType font
		 * is downloaded. */
		HBUINT32 maxMemType42; /* Maximum memory usage when an OpenType font
		 * is downloaded. */
		HBUINT32 minMemType1; /* Minimum memory usage when an OpenType font
		 * is downloaded as a Type 1 font. */
		HBUINT32 maxMemType1; /* Maximum memory usage when an OpenType font
		 * is downloaded as a Type 1 font. */
		postV2Tail v2X;
		DEFINE_SIZE_MIN(32);
	};

	struct post_accelerator_t : post::accelerator_t {};
} /* namespace OT */

#endif /* HB_OT_POST_TABLE_HH */
