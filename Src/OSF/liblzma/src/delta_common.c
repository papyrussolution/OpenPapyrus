/// \file       delta_common.c
/// \brief      Common stuff for Delta encoder and decoder
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop

static void delta_coder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_delta_coder * coder = (lzma_delta_coder *)coder_ptr;
	lzma_next_end(&coder->next, allocator);
	lzma_free(coder, allocator);
}

extern lzma_ret lzma_delta_coder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters)
{
	// Allocate memory for the decoder if needed.
	lzma_delta_coder * coder = (lzma_delta_coder *)next->coder;
	if(!coder) {
		coder = (lzma_delta_coder *)lzma_alloc(sizeof(lzma_delta_coder), allocator);
		if(!coder)
			return LZMA_MEM_ERROR;
		next->coder = coder;
		// End function is the same for encoder and decoder.
		next->end = &delta_coder_end;
		coder->next.SetDefault();// = LZMA_NEXT_CODER_INIT;
	}
	// Validate the options.
	if(lzma_delta_coder_memusage(filters[0].options) == UINT64_MAX)
		return LZMA_OPTIONS_ERROR;
	// Set the delta distance.
	const lzma_options_delta * opt = (const lzma_options_delta *)filters[0].options;
	coder->distance = opt->dist;
	// Initialize the rest of the variables.
	coder->pos = 0;
	memzero(coder->history, LZMA_DELTA_DIST_MAX);
	// Initialize the next decoder in the chain, if any.
	return lzma_next_filter_init(&coder->next, allocator, filters + 1);
}

extern uint64_t lzma_delta_coder_memusage(const void * options)
{
	const lzma_options_delta * opt = (const lzma_options_delta *)options;
	if(opt == NULL || opt->type != LZMA_DELTA_TYPE_BYTE || opt->dist < LZMA_DELTA_DIST_MIN || opt->dist > LZMA_DELTA_DIST_MAX)
		return UINT64_MAX;
	return sizeof(lzma_delta_coder);
}
//
//
//
/// Copies and encodes the data at the same time. This is used when Delta
/// is the first filter in the chain (and thus the last filter in the
/// encoder's filter stack).
static void copy_and_encode(lzma_delta_coder * coder, const uint8 * in, uint8 * out, size_t size)
{
	const size_t distance = coder->distance;
	for(size_t i = 0; i < size; ++i) {
		const uint8 tmp = coder->history[(distance + coder->pos) & 0xFF];
		coder->history[coder->pos-- & 0xFF] = in[i];
		out[i] = in[i] - tmp;
	}
}

/// Encodes the data in place. This is used when we are the last filter
/// in the chain (and thus non-last filter in the encoder's filter stack).
static void encode_in_place(lzma_delta_coder * coder, uint8 * buffer, size_t size)
{
	const size_t distance = coder->distance;
	for(size_t i = 0; i < size; ++i) {
		const uint8 tmp = coder->history[(distance + coder->pos) & 0xFF];
		coder->history[coder->pos-- & 0xFF] = buffer[i];
		buffer[i] -= tmp;
	}
}

static lzma_ret delta_encode(void * coder_ptr, const lzma_allocator * allocator,
    const uint8 * in, size_t * in_pos, size_t in_size, uint8 * out,
    size_t * out_pos, size_t out_size, lzma_action action)
{
	lzma_delta_coder * coder = (lzma_delta_coder *)coder_ptr;
	lzma_ret ret;
	if(coder->next.code == NULL) {
		const size_t in_avail = in_size - *in_pos;
		const size_t out_avail = out_size - *out_pos;
		const size_t size = MIN(in_avail, out_avail);
		copy_and_encode(coder, in + *in_pos, out + *out_pos, size);
		*in_pos += size;
		*out_pos += size;
		ret = action != LZMA_RUN && *in_pos == in_size ? LZMA_STREAM_END : LZMA_OK;
	}
	else {
		const size_t out_start = *out_pos;
		ret = coder->next.code(coder->next.coder, allocator, in, in_pos, in_size, out, out_pos, out_size, action);
		encode_in_place(coder, out + out_start, *out_pos - out_start);
	}
	return ret;
}

static lzma_ret delta_encoder_update(void * coder_ptr, const lzma_allocator * allocator,
    const lzma_filter * filters_null lzma_attribute((__unused__)), const lzma_filter * reversed_filters)
{
	lzma_delta_coder * coder = (lzma_delta_coder *)coder_ptr;
	// Delta doesn't and will never support changing the options in
	// the middle of encoding. If the app tries to change them, we
	// simply ignore them.
	return lzma_next_filter_update(&coder->next, allocator, reversed_filters + 1);
}

extern lzma_ret lzma_delta_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters)
{
	next->code = &delta_encode;
	next->update = &delta_encoder_update;
	return lzma_delta_coder_init(next, allocator, filters);
}

extern lzma_ret lzma_delta_props_encode(const void * options, uint8 * out)
{
	// The caller must have already validated the options, so it's
	// LZMA_PROG_ERROR if they are invalid.
	if(lzma_delta_coder_memusage(options) == UINT64_MAX)
		return LZMA_PROG_ERROR;
	const lzma_options_delta * opt = (const lzma_options_delta *)options;
	out[0] = opt->dist - LZMA_DELTA_DIST_MIN;
	return LZMA_OK;
}
//
//
//
static void decode_buffer(lzma_delta_coder * coder, uint8 * buffer, size_t size)
{
	const size_t distance = coder->distance;
	for(size_t i = 0; i < size; ++i) {
		buffer[i] += coder->history[(distance + coder->pos) & 0xFF];
		coder->history[coder->pos-- & 0xFF] = buffer[i];
	}
}

static lzma_ret delta_decode(void * coder_ptr, const lzma_allocator * allocator,
    const uint8 * in, size_t * in_pos,
    size_t in_size, uint8 * out,
    size_t * out_pos, size_t out_size, lzma_action action)
{
	lzma_delta_coder * coder = (lzma_delta_coder *)coder_ptr;
	assert(coder->next.code != NULL);
	const size_t out_start = *out_pos;
	const lzma_ret ret = coder->next.code(coder->next.coder, allocator, in, in_pos, in_size, out, out_pos, out_size, action);
	decode_buffer(coder, out + out_start, *out_pos - out_start);
	return ret;
}

extern lzma_ret lzma_delta_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator,
    const lzma_filter_info * filters)
{
	next->code = &delta_decode;
	return lzma_delta_coder_init(next, allocator, filters);
}

extern lzma_ret lzma_delta_props_decode(void ** options, const lzma_allocator * allocator,
    const uint8 * props, size_t props_size)
{
	if(props_size != 1)
		return LZMA_OPTIONS_ERROR;
	lzma_options_delta * opt = (lzma_options_delta *)lzma_alloc(sizeof(lzma_options_delta), allocator);
	if(opt == NULL)
		return LZMA_MEM_ERROR;
	opt->type = LZMA_DELTA_TYPE_BYTE;
	opt->dist = props[0] + 1U;
	*options = opt;
	return LZMA_OK;
}
