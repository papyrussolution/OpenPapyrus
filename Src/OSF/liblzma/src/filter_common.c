/// \file       filter_common.c
/// \brief      Filter-specific stuff common for both encoder and decoder
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop

static const struct {
	lzma_vli id; /// Filter ID
	size_t options_size; /// Size of the filter-specific options structure
	bool non_last_ok; /// True if it is OK to use this filter as non-last filter in the chain.
	bool last_ok; /// True if it is OK to use this filter as the last filter in the chain.
	/// True if the filter may change the size of the data (that is, the
	/// amount of encoded output can be different than the amount of
	/// uncompressed input).
	bool changes_size;
} features[] = {
#if defined (HAVE_ENCODER_LZMA1) || defined(HAVE_DECODER_LZMA1)
	{ LZMA_FILTER_LZMA1, sizeof(lzma_options_lzma), false, true, true },
#endif
#if defined(HAVE_ENCODER_LZMA2) || defined(HAVE_DECODER_LZMA2)
	{ LZMA_FILTER_LZMA2, sizeof(lzma_options_lzma), false, true, true },
#endif
#if defined(HAVE_ENCODER_X86) || defined(HAVE_DECODER_X86)
	{ LZMA_FILTER_X86, sizeof(lzma_options_bcj), true, false, false },
#endif
#if defined(HAVE_ENCODER_POWERPC) || defined(HAVE_DECODER_POWERPC)
	{ LZMA_FILTER_POWERPC, sizeof(lzma_options_bcj), true, false, false },
#endif
#if defined(HAVE_ENCODER_IA64) || defined(HAVE_DECODER_IA64)
	{ LZMA_FILTER_IA64, sizeof(lzma_options_bcj), true, false, false },
#endif
#if defined(HAVE_ENCODER_ARM) || defined(HAVE_DECODER_ARM)
	{ LZMA_FILTER_ARM, sizeof(lzma_options_bcj), true, false, false },
#endif
#if defined(HAVE_ENCODER_ARMTHUMB) || defined(HAVE_DECODER_ARMTHUMB)
	{ LZMA_FILTER_ARMTHUMB, sizeof(lzma_options_bcj), true, false, false },
#endif
#if defined(HAVE_ENCODER_SPARC) || defined(HAVE_DECODER_SPARC)
	{ LZMA_FILTER_SPARC, sizeof(lzma_options_bcj), true, false, false },
#endif
#if defined(HAVE_ENCODER_DELTA) || defined(HAVE_DECODER_DELTA)
	{ LZMA_FILTER_DELTA, sizeof(lzma_options_delta), true, false, false },
#endif
	{ LZMA_VLI_UNKNOWN, 0, false, false, false }
};

lzma_ret lzma_filters_copy(const lzma_filter *src, lzma_filter *dest, const lzma_allocator *allocator)
{
	if(src == NULL || dest == NULL)
		return LZMA_PROG_ERROR;
	lzma_ret ret;
	size_t i;
	for(i = 0; src[i].id != LZMA_VLI_UNKNOWN; ++i) {
		// There must be a maximum of four filters plus
		// the array terminator.
		if(i == LZMA_FILTERS_MAX) {
			ret = LZMA_OPTIONS_ERROR;
			goto error;
		}
		dest[i].id = src[i].id;
		if(src[i].options == NULL) {
			dest[i].options = NULL;
		}
		else {
			// See if the filter is supported only when the
			// options is not NULL. This might be convenient
			// sometimes if the app is actually copying only
			// a partial filter chain with a place holder ID.
			//
			// When options is not NULL, the Filter ID must be
			// supported by us, because otherwise we don't know
			// how big the options are.
			size_t j;
			for(j = 0; src[i].id != features[j].id; ++j) {
				if(features[j].id == LZMA_VLI_UNKNOWN) {
					ret = LZMA_OPTIONS_ERROR;
					goto error;
				}
			}
			// Allocate and copy the options.
			dest[i].options = lzma_alloc(features[j].options_size, allocator);
			if(dest[i].options == NULL) {
				ret = LZMA_MEM_ERROR;
				goto error;
			}
			memcpy(dest[i].options, src[i].options, features[j].options_size);
		}
	}
	// Terminate the filter array.
	assert(i <= LZMA_FILTERS_MAX + 1);
	dest[i].id = LZMA_VLI_UNKNOWN;
	dest[i].options = NULL;
	return LZMA_OK;
error:
	// Free the options which we have already allocated.
	while(i-- > 0) {
		lzma_free(dest[i].options, allocator);
		dest[i].options = NULL;
	}
	return ret;
}

static lzma_ret validate_chain(const lzma_filter * filters, size_t * count)
{
	// There must be at least one filter.
	if(filters == NULL || filters[0].id == LZMA_VLI_UNKNOWN)
		return LZMA_PROG_ERROR;
	// Number of non-last filters that may change the size of the data
	// significantly (that is, more than 1-2 % or so).
	size_t changes_size_count = 0;
	// True if it is OK to add a new filter after the current filter.
	bool non_last_ok = true;
	// True if the last filter in the given chain is actually usable as
	// the last filter. Only filters that support embedding End of Payload
	// Marker can be used as the last filter in the chain.
	bool last_ok = false;
	size_t i = 0;
	do {
		size_t j;
		for(j = 0; filters[i].id != features[j].id; ++j)
			if(features[j].id == LZMA_VLI_UNKNOWN)
				return LZMA_OPTIONS_ERROR;

		// If the previous filter in the chain cannot be a non-last
		// filter, the chain is invalid.
		if(!non_last_ok)
			return LZMA_OPTIONS_ERROR;
		non_last_ok = features[j].non_last_ok;
		last_ok = features[j].last_ok;
		changes_size_count += features[j].changes_size;
	} while(filters[++i].id != LZMA_VLI_UNKNOWN);
	// There must be 1-4 filters. The last filter must be usable as
	// the last filter in the chain. A maximum of three filters are
	// allowed to change the size of the data.
	if(i > LZMA_FILTERS_MAX || !last_ok || changes_size_count > 3)
		return LZMA_OPTIONS_ERROR;
	*count = i;
	return LZMA_OK;
}

lzma_ret lzma_raw_coder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter * options, lzma_filter_find coder_find, bool is_encoder)
{
	// Do some basic validation and get the number of filters.
	size_t count;
	return_if_error(validate_chain(options, &count));
	// Set the filter functions and copy the options pointer.
	lzma_filter_info filters[LZMA_FILTERS_MAX + 1];
	if(is_encoder) {
		for(size_t i = 0; i < count; ++i) {
			// The order of the filters is reversed in the
			// encoder. It allows more efficient handling
			// of the uncompressed data.
			const size_t j = count - i - 1;
			const lzma_filter_coder * const fc = coder_find(options[i].id);
			if(fc == NULL || fc->init == NULL)
				return LZMA_OPTIONS_ERROR;
			filters[j].id = options[i].id;
			filters[j].init = fc->init;
			filters[j].options = options[i].options;
		}
	}
	else {
		for(size_t i = 0; i < count; ++i) {
			const lzma_filter_coder * const fc = coder_find(options[i].id);
			if(fc == NULL || fc->init == NULL)
				return LZMA_OPTIONS_ERROR;
			filters[i].id = options[i].id;
			filters[i].init = fc->init;
			filters[i].options = options[i].options;
		}
	}
	// Terminate the array.
	filters[count].id = LZMA_VLI_UNKNOWN;
	filters[count].init = NULL;
	// Initialize the filters.
	const lzma_ret ret = lzma_next_filter_init(next, allocator, filters);
	if(ret != LZMA_OK)
		lzma_next_end(next, allocator);
	return ret;
}

uint64_t lzma_raw_coder_memusage(lzma_filter_find coder_find, const lzma_filter * filters)
{
	// The chain has to have at least one filter.
	{
		size_t tmp;
		if(validate_chain(filters, &tmp) != LZMA_OK)
			return UINT64_MAX;
	}
	uint64_t total = 0;
	size_t i = 0;
	do {
		const lzma_filter_coder * const fc = coder_find(filters[i].id);
		if(fc == NULL)
			return UINT64_MAX; // Unsupported Filter ID
		if(fc->memusage == NULL) {
			// This filter doesn't have a function to calculate
			// the memory usage and validate the options. Such
			// filters need only little memory, so we use 1 KiB
			// as a good estimate. They also accept all possible
			// options, so there's no need to worry about lack
			// of validation.
			total += 1024;
		}
		else {
			// Call the filter-specific memory usage calculation
			// function.
			const uint64_t usage = fc->memusage(filters[i].options);
			if(usage == UINT64_MAX)
				return UINT64_MAX; // Invalid options
			total += usage;
		}
	} while(filters[++i].id != LZMA_VLI_UNKNOWN);
	// Add some fixed amount of extra. It's to compensate memory usage
	// of Stream, Block etc. coders, malloc() overhead, stack etc.
	return total + LZMA_MEMUSAGE_BASE;
}
//
//
//
lzma_ret lzma_raw_buffer_encode(const lzma_filter *filters, const lzma_allocator *allocator, const uint8_t *in, size_t in_size,
	uint8_t *out, size_t *out_pos, size_t out_size)
{
	// Validate what isn't validated later in filter_common.c.
	if((in == NULL && in_size != 0) || out == NULL || out_pos == NULL || *out_pos > out_size)
		return LZMA_PROG_ERROR;
	// Initialize the encoder
	lzma_next_coder next;// = LZMA_NEXT_CODER_INIT;
	return_if_error(lzma_raw_encoder_init(&next, allocator, filters));
	// Store the output position so that we can restore it if
	// something goes wrong.
	const size_t out_start = *out_pos;
	// Do the actual encoding and free coder's memory.
	size_t in_pos = 0;
	lzma_ret ret = next.code(next.coder, allocator, in, &in_pos, in_size, out, out_pos, out_size, LZMA_FINISH);
	lzma_next_end(&next, allocator);
	if(ret == LZMA_STREAM_END) {
		ret = LZMA_OK;
	}
	else {
		if(ret == LZMA_OK) {
			// Output buffer was too small.
			assert(*out_pos == out_size);
			ret = LZMA_BUF_ERROR;
		}
		*out_pos = out_start; // Restore the output position.
	}
	return ret;
}
//
//
//
lzma_ret lzma_raw_buffer_decode(const lzma_filter *filters, const lzma_allocator *allocator,
	const uint8_t *in, size_t *in_pos, size_t in_size, uint8_t *out, size_t *out_pos, size_t out_size)
{
	// Validate what isn't validated later in filter_common.c.
	if(in == NULL || in_pos == NULL || *in_pos > in_size || out == NULL || out_pos == NULL || *out_pos > out_size)
		return LZMA_PROG_ERROR;
	// Initialize the decoer.
	lzma_next_coder next;// = LZMA_NEXT_CODER_INIT;
	return_if_error(lzma_raw_decoder_init(&next, allocator, filters));
	// Store the positions so that we can restore them if something goes wrong.
	const size_t in_start = *in_pos;
	const size_t out_start = *out_pos;
	// Do the actual decoding and free decoder's memory.
	lzma_ret ret = next.code(next.coder, allocator, in, in_pos, in_size, out, out_pos, out_size, LZMA_FINISH);
	if(ret == LZMA_STREAM_END) {
		ret = LZMA_OK;
	}
	else {
		if(ret == LZMA_OK) {
			// Either the input was truncated or the
			// output buffer was too small.
			assert(*in_pos == in_size || *out_pos == out_size);
			if(*in_pos != in_size) {
				// Since input wasn't consumed completely,
				// the output buffer became full and is
				// too small.
				ret = LZMA_BUF_ERROR;
			}
			else if(*out_pos != out_size) {
				// Since output didn't became full, the input
				// has to be truncated.
				ret = LZMA_DATA_ERROR;
			}
			else {
				// All the input was consumed and output
				// buffer is full. Now we don't immediately
				// know the reason for the error. Try
				// decoding one more byte. If it succeeds,
				// then the output buffer was too small. If
				// we cannot get a new output byte, the input
				// is truncated.
				uint8_t tmp[1];
				size_t tmp_pos = 0;
				(void)next.code(next.coder, allocator, in, in_pos, in_size, tmp, &tmp_pos, 1, LZMA_FINISH);
				if(tmp_pos == 1)
					ret = LZMA_BUF_ERROR;
				else
					ret = LZMA_DATA_ERROR;
			}
		}
		// Restore the positions.
		*in_pos = in_start;
		*out_pos = out_start;
	}
	lzma_next_end(&next, allocator);
	return ret;
}
//
//
//
lzma_ret lzma_filter_flags_size(uint32 * size, const lzma_filter *filter)
{
	if(filter->id >= LZMA_FILTER_RESERVED_START)
		return LZMA_PROG_ERROR;
	return_if_error(lzma_properties_size(size, filter));
	*size += lzma_vli_size(filter->id) + lzma_vli_size(*size);
	return LZMA_OK;
}

lzma_ret lzma_filter_flags_encode(const lzma_filter *filter, uint8 *out, size_t *out_pos, size_t out_size)
{
	// Filter ID
	if(filter->id >= LZMA_FILTER_RESERVED_START)
		return LZMA_PROG_ERROR;
	return_if_error(lzma_vli_encode(filter->id, NULL, out, out_pos, out_size));
	// Size of Properties
	uint32 props_size;
	return_if_error(lzma_properties_size(&props_size, filter));
	return_if_error(lzma_vli_encode(props_size, NULL, out, out_pos, out_size));
	// Filter Properties
	if(out_size - *out_pos < props_size)
		return LZMA_PROG_ERROR;
	return_if_error(lzma_properties_encode(filter, out + *out_pos));
	*out_pos += props_size;
	return LZMA_OK;
}
//
//
//
lzma_ret lzma_filter_flags_decode(lzma_filter *filter, const lzma_allocator *allocator, const uint8_t *in, size_t *in_pos, size_t in_size)
{
	// Set the pointer to NULL so the caller can always safely free it.
	filter->options = NULL;
	// Filter ID
	return_if_error(lzma_vli_decode(&filter->id, NULL, in, in_pos, in_size));
	if(filter->id >= LZMA_FILTER_RESERVED_START)
		return LZMA_DATA_ERROR;
	// Size of Properties
	lzma_vli props_size;
	return_if_error(lzma_vli_decode(&props_size, NULL, in, in_pos, in_size));
	// Filter Properties
	if((in_size - *in_pos) < props_size)
		return LZMA_DATA_ERROR;
	const lzma_ret ret = lzma_properties_decode(filter, allocator, in + *in_pos, static_cast<size_t>(props_size));
	*in_pos += props_size;
	return ret;
}
//
// filter-encoder
//
struct lzma_filter_encoder {
	lzma_filter_encoder(lzma_vli _id, lzma_init_function initFunc, uint64_t (* memUsageFunc)(const void *), uint64 (* blockSizeFunc)(const void *),
		lzma_ret (* propsSizeGetFunc)(uint32 *, const void *), uint32 propsSizeFixed, lzma_ret (* propsEncodeFunc)(const void *, uint8 *)) : 
		id(_id), init(initFunc), memusage(memUsageFunc), block_size(blockSizeFunc), props_size_get(propsSizeGetFunc), props_size_fixed(propsSizeFixed),
		props_encode(propsEncodeFunc)
	{
	}
	lzma_vli id; /// Filter ID
	lzma_init_function init; /// Initializes the filter encoder and calls lzma_next_filter_init() for filters + 1.
	uint64_t (* memusage)(const void * options); /// Calculates memory usage of the encoder. If the options are invalid, UINT64_MAX is returned.
	/// Calculates the recommended Uncompressed Size for .xz Blocks to
	/// which the input data can be split to make multithreaded
	/// encoding possible. If this is NULL, it is assumed that
	/// the encoder is fast enough with single thread.
	uint64_t (* block_size)(const void * options);
	/// Tells the size of the Filter Properties field. If options are
	/// invalid, UINT32_MAX is returned. If this is NULL, props_size_fixed
	/// is used.
	lzma_ret (* props_size_get)(uint32 * size, const void * options);
	uint32_t props_size_fixed;
	/// Encodes Filter Properties.
	///
	/// \return     - LZMA_OK: Properties encoded successfully.
	///             - LZMA_OPTIONS_ERROR: Unsupported options
	///             - LZMA_PROG_ERROR: Invalid options or not enough
	///               output space
	lzma_ret (* props_encode)(const void * options, uint8_t * out);
};

static const lzma_filter_encoder * encoder_find(lzma_vli id)
{
	static TSCollection <lzma_filter_encoder> _encoders_list;
	if(!_encoders_list.getCount()) {
	#ifdef HAVE_ENCODER_LZMA1
		_encoders_list.insert(new lzma_filter_encoder(LZMA_FILTER_LZMA1, &lzma_lzma_encoder_init, &lzma_lzma_encoder_memusage, 0, 0, 5, &lzma_lzma_props_encode));
		/*.id = LZMA_FILTER_LZMA1,
		.init = &lzma_lzma_encoder_init,
		.memusage = &lzma_lzma_encoder_memusage,
		.block_size = NULL, // FIXME
		.props_size_get = NULL,
		.props_size_fixed = 5,
		.props_encode = &lzma_lzma_props_encode,*/
	#endif
	#ifdef HAVE_ENCODER_LZMA2
		_encoders_list.insert(new lzma_filter_encoder(LZMA_FILTER_LZMA2, &lzma_lzma2_encoder_init, &lzma_lzma2_encoder_memusage, &lzma_lzma2_block_size/*FIXME*/, 0, 1, &lzma_lzma2_props_encode));
		/*.id = LZMA_FILTER_LZMA2,
		.init = &lzma_lzma2_encoder_init,
		.memusage = &lzma_lzma2_encoder_memusage,
		.block_size = &lzma_lzma2_block_size, // FIXME
		.props_size_get = NULL,
		.props_size_fixed = 1,
		.props_encode = &lzma_lzma2_props_encode,*/
	#endif
	#ifdef HAVE_ENCODER_X86
		_encoders_list.insert(new lzma_filter_encoder(LZMA_FILTER_X86, &lzma_simple_x86_encoder_init, 0, 0, &lzma_simple_props_size, 0, &lzma_simple_props_encode));
		/*.id = LZMA_FILTER_X86,
		.init = &lzma_simple_x86_encoder_init,
		.memusage = NULL,
		.block_size = NULL,
		.props_size_get = &lzma_simple_props_size,
		.props_encode = &lzma_simple_props_encode,*/
	#endif
	#ifdef HAVE_ENCODER_POWERPC
		{
			.id = LZMA_FILTER_POWERPC,
			.init = &lzma_simple_powerpc_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_IA64
		_encoders_list.insert(new lzma_filter_encoder(LZMA_FILTER_IA64, &lzma_simple_ia64_encoder_init, 0, 0, &lzma_simple_props_size, 0, &lzma_simple_props_encode));
		/*.id = LZMA_FILTER_IA64,
		.init = &lzma_simple_ia64_encoder_init,
		.memusage = NULL,
		.block_size = NULL,
		.props_size_get = &lzma_simple_props_size,
		.props_encode = &lzma_simple_props_encode,*/
	#endif
	#ifdef HAVE_ENCODER_ARM
		{
			.id = LZMA_FILTER_ARM,
			.init = &lzma_simple_arm_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_ARMTHUMB
		{
			.id = LZMA_FILTER_ARMTHUMB,
			.init = &lzma_simple_armthumb_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_SPARC
		_encoders_list.insert(new lzma_filter_encoder(LZMA_FILTER_SPARC, &lzma_simple_sparc_encoder_init, 0, 0, &lzma_simple_props_size, 1, &lzma_simple_props_encode));
		/*.id = LZMA_FILTER_SPARC,
		.init = &lzma_simple_sparc_encoder_init,
		.memusage = NULL,
		.block_size = NULL,
		.props_size_get = &lzma_simple_props_size,
		.props_encode = &lzma_simple_props_encode,*/
	#endif
	#ifdef HAVE_ENCODER_DELTA
		_encoders_list.insert(new lzma_filter_encoder(LZMA_FILTER_DELTA, &lzma_delta_encoder_init, &lzma_delta_coder_memusage, 0, 0, 1, &lzma_delta_props_encode));
		/*.id = LZMA_FILTER_DELTA,
		.init = &lzma_delta_encoder_init,
		.memusage = &lzma_delta_coder_memusage,
		.block_size = NULL,
		.props_size_get = NULL,
		.props_size_fixed = 1,
		.props_encode = &lzma_delta_props_encode,*/
	#endif
	};
#if 0 // {
	static const lzma_filter_encoder encoders[] = {
	#ifdef HAVE_ENCODER_LZMA1
		{
			lzma_filter_encoder(LZMA_FILTER_LZMA1, &lzma_lzma_encoder_init, &lzma_lzma_encoder_memusage, 0, 0, 5, &lzma_lzma_props_encode)
			/*.id = LZMA_FILTER_LZMA1,
			.init = &lzma_lzma_encoder_init,
			.memusage = &lzma_lzma_encoder_memusage,
			.block_size = NULL, // FIXME
			.props_size_get = NULL,
			.props_size_fixed = 5,
			.props_encode = &lzma_lzma_props_encode,*/
		},
	#endif
	#ifdef HAVE_ENCODER_LZMA2
		{
			lzma_filter_encoder(LZMA_FILTER_LZMA2, &lzma_lzma2_encoder_init, &lzma_lzma2_encoder_memusage, &lzma_lzma2_block_size/*FIXME*/, 0, 1, &lzma_lzma2_props_encode)
			/*.id = LZMA_FILTER_LZMA2,
			.init = &lzma_lzma2_encoder_init,
			.memusage = &lzma_lzma2_encoder_memusage,
			.block_size = &lzma_lzma2_block_size, // FIXME
			.props_size_get = NULL,
			.props_size_fixed = 1,
			.props_encode = &lzma_lzma2_props_encode,*/
		},
	#endif
	#ifdef HAVE_ENCODER_X86
		{
			lzma_filter_encoder(LZMA_FILTER_X86, &lzma_simple_x86_encoder_init, 0, 0, &lzma_simple_props_size, 0, &lzma_simple_props_encode)
			/*.id = LZMA_FILTER_X86,
			.init = &lzma_simple_x86_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,*/
		},
	#endif
	#ifdef HAVE_ENCODER_POWERPC
		{
			.id = LZMA_FILTER_POWERPC,
			.init = &lzma_simple_powerpc_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_IA64
		{
			lzma_filter_encoder(LZMA_FILTER_IA64, &lzma_simple_ia64_encoder_init, 0, 0, &lzma_simple_props_size, 0, &lzma_simple_props_encode)
			/*.id = LZMA_FILTER_IA64,
			.init = &lzma_simple_ia64_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,*/
		},
	#endif
	#ifdef HAVE_ENCODER_ARM
		{
			.id = LZMA_FILTER_ARM,
			.init = &lzma_simple_arm_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_ARMTHUMB
		{
			.id = LZMA_FILTER_ARMTHUMB,
			.init = &lzma_simple_armthumb_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_SPARC
		{
			.id = LZMA_FILTER_SPARC,
			.init = &lzma_simple_sparc_encoder_init,
			.memusage = NULL,
			.block_size = NULL,
			.props_size_get = &lzma_simple_props_size,
			.props_encode = &lzma_simple_props_encode,
		},
	#endif
	#ifdef HAVE_ENCODER_DELTA
		{
			lzma_filter_encoder(LZMA_FILTER_DELTA, &lzma_delta_encoder_init, &lzma_delta_coder_memusage, 0, 0, 1, &lzma_delta_props_encode)
			/*.id = LZMA_FILTER_DELTA,
			.init = &lzma_delta_encoder_init,
			.memusage = &lzma_delta_coder_memusage,
			.block_size = NULL,
			.props_size_get = NULL,
			.props_size_fixed = 1,
			.props_encode = &lzma_delta_props_encode,*/
		},
	#endif
	};
	for(size_t i = 0; i < SIZEOFARRAY(encoders); ++i)
		if(encoders[i].id == id)
			return encoders + i;
#endif // } 0
	for(uint i = 0; i < _encoders_list.getCount(); i++) {
		const lzma_filter_encoder * p_encdr = _encoders_list.at(i);
		if(p_encdr && p_encdr->id == id)
			return p_encdr;
	}
	return 0;
}

bool lzma_filter_encoder_is_supported(lzma_vli id)
{
	return encoder_find(id) != NULL;
}

lzma_ret lzma_filters_update(lzma_stream *strm, const lzma_filter *filters)
{
	if(strm->internal->next.update == NULL)
		return LZMA_PROG_ERROR;
	// Validate the filter chain.
	if(lzma_raw_encoder_memusage(filters) == UINT64_MAX)
		return LZMA_OPTIONS_ERROR;
	// The actual filter chain in the encoder is reversed. Some things
	// still want the normal order chain, so we provide both.
	size_t count = 1;
	while(filters[count].id != LZMA_VLI_UNKNOWN)
		++count;
	lzma_filter reversed_filters[LZMA_FILTERS_MAX + 1];
	for(size_t i = 0; i < count; ++i)
		reversed_filters[count - i - 1] = filters[i];
	reversed_filters[count].id = LZMA_VLI_UNKNOWN;
	return strm->internal->next.update(strm->internal->next.coder, strm->allocator, filters, reversed_filters);
}

extern lzma_ret lzma_raw_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter * options)
{
	return lzma_raw_coder_init(next, allocator, options, (lzma_filter_find)(&encoder_find), true);
}

lzma_ret lzma_raw_encoder(lzma_stream *strm, const lzma_filter *options)
{
	lzma_next_strm_init3(lzma_raw_coder_init, strm, options, (lzma_filter_find)(&encoder_find), true);
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_SYNC_FLUSH] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;
	return LZMA_OK;
}

uint64_t lzma_raw_encoder_memusage(const lzma_filter *filters)
{
	return lzma_raw_coder_memusage((lzma_filter_find)(&encoder_find), filters);
}

extern uint64_t lzma_mt_block_size(const lzma_filter * filters)
{
	uint64_t max = 0;
	for(size_t i = 0; filters[i].id != LZMA_VLI_UNKNOWN; ++i) {
		const lzma_filter_encoder * const fe = encoder_find(filters[i].id);
		if(fe->block_size != NULL) {
			const uint64_t size = fe->block_size(filters[i].options);
			if(!size)
				return 0;
			if(size > max)
				max = size;
		}
	}
	return max;
}

lzma_ret lzma_properties_size(uint32 * size, const lzma_filter * filter)
{
	const lzma_filter_encoder * const fe = encoder_find(filter->id);
	if(fe == NULL) {
		// Unknown filter - if the Filter ID is a proper VLI,
		// return LZMA_OPTIONS_ERROR instead of LZMA_PROG_ERROR,
		// because it's possible that we just don't have support
		// compiled in for the requested filter.
		return filter->id <= LZMA_VLI_MAX ? LZMA_OPTIONS_ERROR : LZMA_PROG_ERROR;
	}
	if(fe->props_size_get == NULL) {
		// No props_size_get() function, use props_size_fixed.
		*size = fe->props_size_fixed;
		return LZMA_OK;
	}
	return fe->props_size_get(size, filter->options);
}

lzma_ret lzma_properties_encode(const lzma_filter *filter, uint8_t *props)
{
	const lzma_filter_encoder * const fe = encoder_find(filter->id);
	if(fe == NULL)
		return LZMA_PROG_ERROR;
	if(fe->props_encode == NULL)
		return LZMA_OK;
	return fe->props_encode(filter->options, props);
}
//
// filter-decoder
//
struct lzma_filter_decoder {
	lzma_filter_decoder(lzma_vli _id, lzma_init_function initFunc, uint64_t (* memUsageFunc)(const void *),
		lzma_ret (* propsDecodeFunc)(void **, const lzma_allocator *, const uint8_t *, size_t)) : id(_id), init(initFunc),
		memusage(memUsageFunc), props_decode(propsDecodeFunc)
	{
	}
	lzma_vli id; /// Filter ID
	lzma_init_function init; /// Initializes the filter encoder and calls lzma_next_filter_init() for filters + 1.
	uint64_t (* memusage)(const void * options); /// Calculates memory usage of the encoder. If the options are invalid, UINT64_MAX is returned.
	/// Decodes Filter Properties.
	///
	/// \return     - LZMA_OK: Properties decoded successfully.
	///             - LZMA_OPTIONS_ERROR: Unsupported properties
	///             - LZMA_MEM_ERROR: Memory allocation failed.
	lzma_ret (* props_decode)(void ** options, const lzma_allocator * allocator, const uint8_t * props, size_t props_size);
};

static const lzma_filter_decoder * decoder_find(lzma_vli id)
{
	static TSCollection <lzma_filter_decoder> _decoders_list;
	if(!_decoders_list.getCount()) {
	#ifdef HAVE_DECODER_LZMA1
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_LZMA1, &lzma_lzma_decoder_init, &lzma_lzma_decoder_memusage, &lzma_lzma_props_decode));
		//.id = LZMA_FILTER_LZMA1,
		//.init = &lzma_lzma_decoder_init,
		//.memusage = &lzma_lzma_decoder_memusage,
		//.props_decode = &lzma_lzma_props_decode,
	#endif
	#ifdef HAVE_DECODER_LZMA2
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_LZMA2, &lzma_lzma2_decoder_init, &lzma_lzma2_decoder_memusage, &lzma_lzma2_props_decode));
		//.id = LZMA_FILTER_LZMA2,
		//.init = &lzma_lzma2_decoder_init,
		//.memusage = &lzma_lzma2_decoder_memusage,
		//.props_decode = &lzma_lzma2_props_decode,
	#endif
	#ifdef HAVE_DECODER_X86
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_X86, &lzma_simple_x86_decoder_init, NULL, &lzma_simple_props_decode));
		//.id = LZMA_FILTER_X86,
		//.init = &lzma_simple_x86_decoder_init,
		//.memusage = NULL,
		//.props_decode = &lzma_simple_props_decode,
	#endif
	#ifdef HAVE_DECODER_POWERPC
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_POWERPC, &lzma_simple_powerpc_decoder_init, NULL, &lzma_simple_props_decode));
		//.id = LZMA_FILTER_POWERPC,
		//.init = &lzma_simple_powerpc_decoder_init,
		//.memusage = NULL,
		//.props_decode = &lzma_simple_props_decode,
	#endif
	#ifdef HAVE_DECODER_IA64
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_IA64, &lzma_simple_ia64_decoder_init, NULL, &lzma_simple_props_decode));
		//.id = LZMA_FILTER_IA64,
		//.init = &lzma_simple_ia64_decoder_init,
		//.memusage = NULL,
		//.props_decode = &lzma_simple_props_decode,
	#endif
	#ifdef HAVE_DECODER_ARM
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_ARM, &lzma_simple_arm_decoder_init, NULL, &lzma_simple_props_decode));
		//.id = LZMA_FILTER_ARM,
		//.init = &lzma_simple_arm_decoder_init,
		//.memusage = NULL,
		//.props_decode = &lzma_simple_props_decode,
	#endif
	#ifdef HAVE_DECODER_ARMTHUMB
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_ARMTHUMB, &lzma_simple_armthumb_decoder_init, NULL, &lzma_simple_props_decode));
		//.id = LZMA_FILTER_ARMTHUMB,
		//.init = &lzma_simple_armthumb_decoder_init,
		//.memusage = NULL,
		//.props_decode = &lzma_simple_props_decode,
	#endif
	#ifdef HAVE_DECODER_SPARC
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_SPARC, &lzma_simple_sparc_decoder_init, NULL, &lzma_simple_props_decode));
		//.id = LZMA_FILTER_SPARC,
		//.init = &lzma_simple_sparc_decoder_init,
		//.memusage = NULL,
		//.props_decode = &lzma_simple_props_decode,
	#endif
	#ifdef HAVE_DECODER_DELTA
		_decoders_list.insert(new lzma_filter_decoder(LZMA_FILTER_DELTA, &lzma_delta_decoder_init, &lzma_delta_coder_memusage, &lzma_delta_props_decode));
		//.id = LZMA_FILTER_DELTA,
		//.init = &lzma_delta_decoder_init,
		//.memusage = &lzma_delta_coder_memusage,
		//.props_decode = &lzma_delta_props_decode,
	#endif
	}
#if 0 // {
	static const lzma_filter_decoder decoders[] = {
	#ifdef HAVE_DECODER_LZMA1
		{
			lzma_filter_decoder(LZMA_FILTER_LZMA1, &lzma_lzma_decoder_init, &lzma_lzma_decoder_memusage, &lzma_lzma_props_decode)
			//.id = LZMA_FILTER_LZMA1,
			//.init = &lzma_lzma_decoder_init,
			//.memusage = &lzma_lzma_decoder_memusage,
			//.props_decode = &lzma_lzma_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_LZMA2
		{
			lzma_filter_decoder(LZMA_FILTER_LZMA2, &lzma_lzma2_decoder_init, &lzma_lzma2_decoder_memusage, &lzma_lzma2_props_decode)
			//.id = LZMA_FILTER_LZMA2,
			//.init = &lzma_lzma2_decoder_init,
			//.memusage = &lzma_lzma2_decoder_memusage,
			//.props_decode = &lzma_lzma2_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_X86
		{
			lzma_filter_decoder(LZMA_FILTER_X86, &lzma_simple_x86_decoder_init, NULL, &lzma_simple_props_decode)
			//.id = LZMA_FILTER_X86,
			//.init = &lzma_simple_x86_decoder_init,
			//.memusage = NULL,
			//.props_decode = &lzma_simple_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_POWERPC
		{
			lzma_filter_decoder(LZMA_FILTER_POWERPC, &lzma_simple_powerpc_decoder_init, NULL, &lzma_simple_props_decode)
			//.id = LZMA_FILTER_POWERPC,
			//.init = &lzma_simple_powerpc_decoder_init,
			//.memusage = NULL,
			//.props_decode = &lzma_simple_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_IA64
		{
			lzma_filter_decoder(LZMA_FILTER_IA64, &lzma_simple_ia64_decoder_init, NULL, &lzma_simple_props_decode)
			//.id = LZMA_FILTER_IA64,
			//.init = &lzma_simple_ia64_decoder_init,
			//.memusage = NULL,
			//.props_decode = &lzma_simple_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_ARM
		{
			lzma_filter_decoder(LZMA_FILTER_ARM, &lzma_simple_arm_decoder_init, NULL, &lzma_simple_props_decode)
			//.id = LZMA_FILTER_ARM,
			//.init = &lzma_simple_arm_decoder_init,
			//.memusage = NULL,
			//.props_decode = &lzma_simple_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_ARMTHUMB
		{
			lzma_filter_decoder(LZMA_FILTER_ARMTHUMB, &lzma_simple_armthumb_decoder_init, NULL, &lzma_simple_props_decode)
			//.id = LZMA_FILTER_ARMTHUMB,
			//.init = &lzma_simple_armthumb_decoder_init,
			//.memusage = NULL,
			//.props_decode = &lzma_simple_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_SPARC
		{
			lzma_filter_decoder(LZMA_FILTER_SPARC, &lzma_simple_sparc_decoder_init, NULL, &lzma_simple_props_decode)
			//.id = LZMA_FILTER_SPARC,
			//.init = &lzma_simple_sparc_decoder_init,
			//.memusage = NULL,
			//.props_decode = &lzma_simple_props_decode,
		},
	#endif
	#ifdef HAVE_DECODER_DELTA
		{
			lzma_filter_decoder(LZMA_FILTER_DELTA, &lzma_delta_decoder_init, &lzma_delta_coder_memusage, &lzma_delta_props_decode)
			//.id = LZMA_FILTER_DELTA,
			//.init = &lzma_delta_decoder_init,
			//.memusage = &lzma_delta_coder_memusage,
			//.props_decode = &lzma_delta_props_decode,
		},
	#endif
	};
#endif // } 0
	//for(size_t i = 0; i < SIZEOFARRAY(decoders); ++i)
		//if(decoders[i].id == id)
			//return decoders + i;
	for(uint i = 0; i < _decoders_list.getCount(); i++) {
		const lzma_filter_decoder * p_dcdr = _decoders_list.at(i);
		if(p_dcdr && p_dcdr->id == id)
			return p_dcdr;
	}
	return 0;
}

bool lzma_filter_decoder_is_supported(lzma_vli id)
{
	return decoder_find(id) != NULL;
}

extern lzma_ret lzma_raw_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter * options)
{
	return lzma_raw_coder_init(next, allocator, options, (lzma_filter_find)(&decoder_find), false);
}

lzma_ret lzma_raw_decoder(lzma_stream *strm, const lzma_filter *options)
{
	lzma_next_strm_init(lzma_raw_decoder_init, strm, options);
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;
	return LZMA_OK;
}

uint64_t lzma_raw_decoder_memusage(const lzma_filter *filters)
{
	return lzma_raw_coder_memusage((lzma_filter_find)(&decoder_find), filters);
}

lzma_ret lzma_properties_decode(lzma_filter *filter, const lzma_allocator *allocator, const uint8_t *props, size_t props_size)
{
	// Make it always NULL so that the caller can always safely free() it.
	filter->options = NULL;
	const lzma_filter_decoder * const fd = decoder_find(filter->id);
	if(fd == NULL)
		return LZMA_OPTIONS_ERROR;
	if(fd->props_decode == NULL)
		return props_size == 0 ? LZMA_OK : LZMA_OPTIONS_ERROR;
	return fd->props_decode(&filter->options, allocator, props, props_size);
}
