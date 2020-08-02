/// \file       easy_preset.h
/// \brief      Preset handling for easy encoder and decoder
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#ifndef __EASY_PRESET_H // {
#define __EASY_PRESET_H
struct lzma_options_easy {
	lzma_filter filters[LZMA_FILTERS_MAX + 1]; /// We need to keep the filters array available in case LZMA_FULL_FLUSH is used.
	lzma_options_lzma opt_lzma; /// Options for LZMA2
	// Options for more filters can be added later, so this struct is not ready to be put into the public API.
};

/// Set *easy to the settings given by the preset. Returns true on error,
/// false on success.
extern bool lzma_easy_preset(lzma_options_easy *easy, uint32_t preset);
#endif // } __EASY_PRESET_H
