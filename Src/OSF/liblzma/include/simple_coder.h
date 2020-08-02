/// \file       simple_coder.h
/// \brief      Wrapper for simple filters
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#ifndef LZMA_SIMPLE_CODER_H
#define LZMA_SIMPLE_CODER_H
extern lzma_ret lzma_simple_x86_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_x86_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_powerpc_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_powerpc_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_ia64_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_ia64_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_arm_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_arm_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_armthumb_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_armthumb_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_sparc_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_sparc_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_props_size(uint32_t *size, const void *options);
extern lzma_ret lzma_simple_props_encode(const void *options, uint8_t *out);
extern lzma_ret lzma_simple_props_decode(void **options, const lzma_allocator *allocator, const uint8_t *props, size_t props_size);
#endif
