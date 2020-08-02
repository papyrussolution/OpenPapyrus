/// \file       delta_common.h
/// \brief      Common stuff for Delta encoder and decoder
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#ifndef LZMA_DELTA_COMMON_H
#define LZMA_DELTA_COMMON_H

struct lzma_delta_coder {
	lzma_next_coder next; /// Next coder in the chain
	size_t distance; /// Delta distance
	uint8_t pos; /// Position in history[]
	uint8_t history[LZMA_DELTA_DIST_MAX]; /// Buffer to hold history of the original data
};

extern lzma_ret lzma_delta_coder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern uint64_t lzma_delta_coder_memusage(const void *options);
extern lzma_ret lzma_delta_encoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern lzma_ret lzma_delta_props_encode(const void *options, uint8_t *out);
extern lzma_ret lzma_delta_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern lzma_ret lzma_delta_props_decode(void **options, const lzma_allocator *allocator, const uint8_t *props, size_t props_size);

#endif
