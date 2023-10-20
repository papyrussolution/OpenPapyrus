/// \file       lzma2-internal.h
/// \brief      LZMA2 encoder & decoder
//  Authors:    Igor Pavlov
//              Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#ifndef LZMA_LZMA2_INTERNAL_H
#define LZMA_LZMA2_INTERNAL_H

#define LZMA2_CHUNK_MAX (UINT32_C(1) << 16) /// Maximum number of bytes of actual data per chunk (no headers)
#define LZMA2_UNCOMPRESSED_MAX (UINT32_C(1) << 21) /// Maximum uncompressed size of LZMA chunk (no headers)
#define LZMA2_HEADER_MAX 6 /// Maximum size of LZMA2 headers
#define LZMA2_HEADER_UNCOMPRESSED 3 /// Size of a header for uncompressed chunk

extern lzma_ret lzma_lzma2_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern  uint64 lzma_lzma2_encoder_memusage(const void * options);
extern lzma_ret lzma_lzma2_props_encode(const void * options, uint8 * out);
extern  uint64 lzma_lzma2_block_size(const void * options);
extern lzma_ret lzma_lzma2_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern  uint64 lzma_lzma2_decoder_memusage(const void *options);
extern lzma_ret lzma_lzma2_props_decode(void **options, const lzma_allocator *allocator, const uint8 *props, size_t props_size);

#endif
