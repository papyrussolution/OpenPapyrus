/// \file       easy.c
/// \brief      Easy single-call .xz Stream encoder/decoder
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
//#include "easy_preset.h"
//
// Descr: Preset handling for easy encoder and decoder
//
extern bool lzma_easy_preset(lzma_options_easy * opt_easy, uint32_t preset)
{
	if(lzma_lzma_preset(&opt_easy->opt_lzma, preset))
		return true;
	opt_easy->filters[0].id = LZMA_FILTER_LZMA2;
	opt_easy->filters[0].options = &opt_easy->opt_lzma;
	opt_easy->filters[1].id = LZMA_VLI_UNKNOWN;
	return false;
}

lzma_ret lzma_easy_buffer_encode(uint32_t preset, lzma_check check, const lzma_allocator *allocator, 
	const uint8_t *in, size_t in_size, uint8_t *out, size_t *out_pos, size_t out_size)
{
	lzma_options_easy opt_easy;
	if(lzma_easy_preset(&opt_easy, preset))
		return LZMA_OPTIONS_ERROR;
	return lzma_stream_buffer_encode(opt_easy.filters, check, allocator, in, in_size, out, out_pos, out_size);
}

lzma_ret lzma_easy_encoder(lzma_stream *strm, uint32_t preset, lzma_check check)
{
	lzma_options_easy opt_easy;
	if(lzma_easy_preset(&opt_easy, preset))
		return LZMA_OPTIONS_ERROR;
	return lzma_stream_encoder(strm, opt_easy.filters, check);
}

uint64_t lzma_easy_encoder_memusage(uint32_t preset)
{
	lzma_options_easy opt_easy;
	if(lzma_easy_preset(&opt_easy, preset))
		return UINT32_MAX;
	return lzma_raw_encoder_memusage(opt_easy.filters);
}

uint64_t lzma_easy_decoder_memusage(uint32_t preset)
{
	lzma_options_easy opt_easy;
	if(lzma_easy_preset(&opt_easy, preset))
		return UINT32_MAX;
	return lzma_raw_decoder_memusage(opt_easy.filters);
}
