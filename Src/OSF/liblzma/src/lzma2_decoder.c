/// \file       lzma2_decoder.c
/// \brief      LZMA2 decoder
//  Authors:    Igor Pavlov
//              Lasse Collin
//
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop

struct lzma_lzma2_decoder_coder {
	enum sequence {
		SEQ_CONTROL,
		SEQ_UNCOMPRESSED_1,
		SEQ_UNCOMPRESSED_2,
		SEQ_COMPRESSED_0,
		SEQ_COMPRESSED_1,
		SEQ_PROPERTIES,
		SEQ_LZMA,
		SEQ_COPY,
	} sequence;
	enum sequence next_sequence; /// Sequence after the size fields have been decoded.
	lzma_lz_decoder lzma; /// LZMA decoder
	size_t uncompressed_size; /// Uncompressed size of LZMA chunk
	size_t compressed_size; /// Compressed size of the chunk (naturally equals to uncompressed size of uncompressed chunk)
	bool need_properties; /// True if properties are needed. This is false before the first LZMA chunk.
	/// True if dictionary reset is needed. This is false before the
	/// first chunk (LZMA or uncompressed).
	bool need_dictionary_reset;
	lzma_options_lzma options;
};

static lzma_ret lzma2_decode(void * coder_ptr, lzma_dict * dict,
    const uint8 * in, size_t * in_pos, size_t in_size)
{
	lzma_lzma2_decoder_coder * coder = (lzma_lzma2_decoder_coder *)coder_ptr;
	// With SEQ_LZMA it is possible that no new input is needed to do
	// some progress. The rest of the sequences assume that there is
	// at least one byte of input.
	while(*in_pos < in_size || coder->sequence == lzma_lzma2_decoder_coder::SEQ_LZMA)
		switch(coder->sequence) {
			case lzma_lzma2_decoder_coder::SEQ_CONTROL: {
			    const uint32_t control = in[*in_pos];
			    ++*in_pos;
			    // End marker
			    if(control == 0x00)
				    return LZMA_STREAM_END;
			    if(control >= 0xE0 || control == 1) {
				    // Dictionary reset implies that next LZMA chunk has to set new properties.
				    coder->need_properties = true;
				    coder->need_dictionary_reset = true;
			    }
			    else if(coder->need_dictionary_reset) {
				    return LZMA_DATA_ERROR;
			    }
			    if(control >= 0x80) {
				    // LZMA chunk. The highest five bits of the
				    // uncompressed size are taken from the control byte.
				    coder->uncompressed_size = (control & 0x1F) << 16;
				    coder->sequence = lzma_lzma2_decoder_coder::SEQ_UNCOMPRESSED_1;
				    // See if there are new properties or if we need to
				    // reset the state.
				    if(control >= 0xC0) {
					    // When there are new properties, state reset
					    // is done at SEQ_PROPERTIES.
					    coder->need_properties = false;
					    coder->next_sequence = lzma_lzma2_decoder_coder::SEQ_PROPERTIES;
				    }
				    else if(coder->need_properties) {
					    return LZMA_DATA_ERROR;
				    }
				    else {
					    coder->next_sequence = lzma_lzma2_decoder_coder::SEQ_LZMA;
					    // If only state reset is wanted with old
					    // properties, do the resetting here for
					    // simplicity.
					    if(control >= 0xA0)
						    coder->lzma.reset(coder->lzma.coder, &coder->options);
				    }
			    }
			    else {
				    // Invalid control values
				    if(control > 2)
					    return LZMA_DATA_ERROR;
				    // It's uncompressed chunk
				    coder->sequence = lzma_lzma2_decoder_coder::SEQ_COMPRESSED_0;
				    coder->next_sequence = lzma_lzma2_decoder_coder::SEQ_COPY;
			    }
			    if(coder->need_dictionary_reset) {
				    // Finish the dictionary reset and let the caller
				    // flush the dictionary to the actual output buffer.
				    coder->need_dictionary_reset = false;
				    dict_reset(dict);
				    return LZMA_OK;
			    }

			    break;
		    }
			case lzma_lzma2_decoder_coder::SEQ_UNCOMPRESSED_1:
			    coder->uncompressed_size += (uint32_t)(in[(*in_pos)++]) << 8;
			    coder->sequence = lzma_lzma2_decoder_coder::SEQ_UNCOMPRESSED_2;
			    break;
			case lzma_lzma2_decoder_coder::SEQ_UNCOMPRESSED_2:
			    coder->uncompressed_size += in[(*in_pos)++] + 1U;
			    coder->sequence = lzma_lzma2_decoder_coder::SEQ_COMPRESSED_0;
			    coder->lzma.set_uncompressed(coder->lzma.coder,
				coder->uncompressed_size);
			    break;
			case lzma_lzma2_decoder_coder::SEQ_COMPRESSED_0:
			    coder->compressed_size = (uint32_t)(in[(*in_pos)++]) << 8;
			    coder->sequence = lzma_lzma2_decoder_coder::SEQ_COMPRESSED_1;
			    break;
			case lzma_lzma2_decoder_coder::SEQ_COMPRESSED_1:
			    coder->compressed_size += in[(*in_pos)++] + 1U;
			    coder->sequence = coder->next_sequence;
			    break;
			case lzma_lzma2_decoder_coder::SEQ_PROPERTIES:
			    if(lzma_lzma_lclppb_decode(&coder->options, in[(*in_pos)++]))
				    return LZMA_DATA_ERROR;
			    coder->lzma.reset(coder->lzma.coder, &coder->options);
			    coder->sequence = lzma_lzma2_decoder_coder::SEQ_LZMA;
			    break;
			case lzma_lzma2_decoder_coder::SEQ_LZMA: {
			    // Store the start offset so that we can update
			    // coder->compressed_size later.
			    const size_t in_start = *in_pos;
			    // Decode from in[] to *dict.
			    const lzma_ret ret = coder->lzma.code(coder->lzma.coder, dict, in, in_pos, in_size);
			    // Validate and update coder->compressed_size.
			    const size_t in_used = *in_pos - in_start;
			    if(in_used > coder->compressed_size)
				    return LZMA_DATA_ERROR;
			    coder->compressed_size -= in_used;
			    // Return if we didn't finish the chunk, or an error occurred.
			    if(ret != LZMA_STREAM_END)
				    return ret;
			    // The LZMA decoder must have consumed the whole chunk now.
			    // We don't need to worry about uncompressed size since it
			    // is checked by the LZMA decoder.
			    if(coder->compressed_size != 0)
				    return LZMA_DATA_ERROR;
			    coder->sequence = lzma_lzma2_decoder_coder::SEQ_CONTROL;
			    break;
		    }
			case lzma_lzma2_decoder_coder::SEQ_COPY: {
			    // Copy from input to the dictionary as is.
			    dict_write(dict, in, in_pos, in_size, &coder->compressed_size);
			    if(coder->compressed_size != 0)
				    return LZMA_OK;
			    coder->sequence = lzma_lzma2_decoder_coder::SEQ_CONTROL;
			    break;
		    }
			default:
			    assert(0);
			    return LZMA_PROG_ERROR;
		}
	return LZMA_OK;
}

static void lzma2_decoder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_lzma2_decoder_coder * coder = (lzma_lzma2_decoder_coder *)coder_ptr;
	assert(coder->lzma.end == NULL);
	lzma_free(coder->lzma.coder, allocator);
	lzma_free(coder, allocator);
}

static lzma_ret lzma2_decoder_init(lzma_lz_decoder * lz, const lzma_allocator * allocator, const void * opt, lzma_lz_decoder_options * lz_options)
{
	lzma_lzma2_decoder_coder * coder = (lzma_lzma2_decoder_coder *)lz->coder;
	if(!coder) {
		coder = (lzma_lzma2_decoder_coder *)lzma_alloc(sizeof(lzma_lzma2_decoder_coder), allocator);
		if(!coder)
			return LZMA_MEM_ERROR;
		lz->coder = coder;
		lz->code = &lzma2_decode;
		lz->end = &lzma2_decoder_end;
		coder->lzma.SetDefault();// = LZMA_LZ_DECODER_INIT;
	}
	const lzma_options_lzma * options = (const lzma_options_lzma *)opt;
	coder->sequence = lzma_lzma2_decoder_coder::SEQ_CONTROL;
	coder->need_properties = true;
	coder->need_dictionary_reset = options->preset_dict == NULL || options->preset_dict_size == 0;
	return lzma_lzma_decoder_create(&coder->lzma, allocator, options, lz_options);
}

extern lzma_ret lzma_lzma2_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters)
{
	// LZMA2 can only be the last filter in the chain. This is enforced
	// by the raw_decoder initialization.
	assert(filters[1].init == NULL);
	return lzma_lz_decoder_init(next, allocator, filters, &lzma2_decoder_init);
}

extern  uint64 lzma_lzma2_decoder_memusage(const void * options)
{
	return sizeof(lzma_lzma2_decoder_coder) + lzma_lzma_decoder_memusage_nocheck(options);
}

extern lzma_ret lzma_lzma2_props_decode(void ** options, const lzma_allocator * allocator, const uint8 * props, size_t props_size)
{
	if(props_size != 1)
		return LZMA_OPTIONS_ERROR;
	// Check that reserved bits are unset.
	if(props[0] & 0xC0)
		return LZMA_OPTIONS_ERROR;
	// Decode the dictionary size.
	if(props[0] > 40)
		return LZMA_OPTIONS_ERROR;
	lzma_options_lzma * opt = (lzma_options_lzma *)lzma_alloc(sizeof(lzma_options_lzma), allocator);
	if(opt == NULL)
		return LZMA_MEM_ERROR;
	if(props[0] == 40) {
		opt->dict_size = UINT32_MAX;
	}
	else {
		opt->dict_size = 2 | (props[0] & 1U);
		opt->dict_size <<= props[0] / 2U + 11;
	}
	opt->preset_dict = NULL;
	opt->preset_dict_size = 0;
	*options = opt;
	return LZMA_OK;
}
