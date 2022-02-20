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
#ifndef HB_KERN_HH
#define HB_KERN_HH

#include "hb-open-type.hh"
#include "hb-aat-layout-common.hh"
#include "hb-ot-layout-gpos-table.hh"

namespace OT {
	template <typename Driver>
	struct hb_kern_machine_t {
		hb_kern_machine_t(const Driver &driver_, bool crossStream_ = false) : driver(driver_), crossStream(crossStream_) 
		{
		}
		HB_NO_SANITIZE_SIGNED_INTEGER_OVERFLOW
		void kern(hb_font_t * font, hb_buffer_t * buffer, hb_mask_t kern_mask, bool scale = true) const
		{
			OT::hb_ot_apply_context_t c(1, font, buffer);
			c.set_lookup_mask(kern_mask);
			c.set_lookup_props(OT::LookupFlag::IgnoreMarks);
			auto &skippy_iter = c.iter_input;

			bool horizontal = HB_DIRECTION_IS_HORIZONTAL(buffer->props.direction);
			uint count = buffer->len;
			hb_glyph_info_t * info = buffer->info;
			hb_glyph_position_t * pos = buffer->pos;
			for(uint idx = 0; idx < count;) {
				if(!(info[idx].mask & kern_mask)) {
					idx++;
					continue;
				}

				skippy_iter.reset(idx, 1);
				if(!skippy_iter.next()) {
					idx++;
					continue;
				}

				uint i = idx;
				uint j = skippy_iter.idx;

				hb_position_t kern = driver.get_kerning(info[i].codepoint,
					info[j].codepoint);

				if(LIKELY(!kern))
					goto skip;

				if(horizontal) {
					if(scale)
						kern = font->em_scale_x(kern);
					if(crossStream) {
						pos[j].y_offset = kern;
						buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT;
					}
					else {
						hb_position_t kern1 = kern >> 1;
						hb_position_t kern2 = kern - kern1;
						pos[i].x_advance += kern1;
						pos[j].x_advance += kern2;
						pos[j].x_offset += kern2;
					}
				}
				else {
					if(scale)
						kern = font->em_scale_y(kern);
					if(crossStream) {
						pos[j].x_offset = kern;
						buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT;
					}
					else {
						hb_position_t kern1 = kern >> 1;
						hb_position_t kern2 = kern - kern1;
						pos[i].y_advance += kern1;
						pos[j].y_advance += kern2;
						pos[j].y_offset += kern2;
					}
				}

				buffer->unsafe_to_break(i, j + 1);

skip:
				idx = skippy_iter.idx;
			}
		}

		const Driver &driver;
		bool crossStream;
	};
} /* namespace OT */

#endif /* HB_KERN_HH */
