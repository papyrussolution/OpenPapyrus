// BLOCK.C
// xz blocks
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
//
//
//
struct lzma_block_encoder_coder {
	lzma_next_coder next; /// The filters in the chain; initialized with lzma_raw_decoder_init().
	/// Encoding options; we also write Unpadded Size, Compressed Size,
	/// and Uncompressed Size back to this structure when the encoding has been finished.
	lzma_block * block;
	enum {
		SEQ_CODE,
		SEQ_PADDING,
		SEQ_CHECK,
	} sequence;
	lzma_vli compressed_size; /// Compressed Size calculated while encoding
	lzma_vli uncompressed_size; /// Uncompressed Size calculated while encoding
	size_t pos; /// Position in the Check field
	lzma_check_state check; /// Check of the uncompressed data
};

struct lzma_block_decoder_coder {
	enum {
		SEQ_CODE,
		SEQ_PADDING,
		SEQ_CHECK,
	} sequence;
	lzma_next_coder next; /// The filters in the chain; initialized with lzma_raw_decoder_init().
	/// Decoding options; we also write Compressed Size and Uncompressed
	/// Size back to this structure when the decoding has been finished.
	lzma_block * block;
	lzma_vli compressed_size; /// Compressed Size calculated while decoding
	lzma_vli uncompressed_size; /// Uncompressed Size calculated while decoding
	/// Maximum allowed Compressed Size; this takes into account the
	/// size of the Block Header and Check fields when Compressed Size is unknown.
	lzma_vli compressed_limit;
	size_t check_pos; /// Position when reading the Check field
	lzma_check_state check; /// Check of the uncompressed data
	bool ignore_check; /// True if the integrity check won't be calculated and verified.
};
//
// block_util
//
lzma_ret lzma_block_compressed_size(lzma_block *block, lzma_vli unpadded_size)
{
	// Validate everything but Uncompressed Size and filters.
	if(lzma_block_unpadded_size(block) == 0)
		return LZMA_PROG_ERROR;
	const uint32_t container_size = block->header_size + lzma_check_size(block->check);
	// Validate that Compressed Size will be greater than zero.
	if(unpadded_size <= container_size)
		return LZMA_DATA_ERROR;
	// Calculate what Compressed Size is supposed to be.
	// If Compressed Size was present in Block Header,
	// compare that the new value matches it.
	const lzma_vli compressed_size = unpadded_size - container_size;
	if(block->compressed_size != LZMA_VLI_UNKNOWN && block->compressed_size != compressed_size)
		return LZMA_DATA_ERROR;
	block->compressed_size = compressed_size;
	return LZMA_OK;
}

lzma_vli lzma_block_unpadded_size(const lzma_block *block)
{
	// Validate the values that we are interested in i.e. all but
	// Uncompressed Size and the filters.
	//
	// NOTE: This function is used for validation too, so it is
	// essential that these checks are always done even if
	// Compressed Size is unknown.
	if(block == NULL || block->version > 1 || block->header_size < LZMA_BLOCK_HEADER_SIZE_MIN || 
		block->header_size > LZMA_BLOCK_HEADER_SIZE_MAX || (block->header_size & 3) || 
		!lzma_vli_is_valid(block->compressed_size) || block->compressed_size == 0 || (uint)(block->check) > LZMA_CHECK_ID_MAX)
		return 0;
	// If Compressed Size is unknown, return that we cannot know
	// size of the Block either.
	if(block->compressed_size == LZMA_VLI_UNKNOWN)
		return LZMA_VLI_UNKNOWN;
	// Calculate Unpadded Size and validate it.
	const lzma_vli unpadded_size = block->compressed_size + block->header_size + lzma_check_size(block->check);
	assert(unpadded_size >= UNPADDED_SIZE_MIN);
	if(unpadded_size > UNPADDED_SIZE_MAX)
		return 0;
	return unpadded_size;
}

lzma_vli lzma_block_total_size(const lzma_block *block)
{
	lzma_vli unpadded_size = lzma_block_unpadded_size(block);
	if(unpadded_size != LZMA_VLI_UNKNOWN)
		unpadded_size = vli_ceil4(unpadded_size);
	return unpadded_size;
}
//
// block_header_encoder
//
lzma_ret lzma_block_header_size(lzma_block *block)
{
	if(block->version > 1)
		return LZMA_OPTIONS_ERROR;
	// Block Header Size + Block Flags + CRC32.
	uint32_t size = 1 + 1 + 4;
	// Compressed Size
	if(block->compressed_size != LZMA_VLI_UNKNOWN) {
		const uint32_t add = lzma_vli_size(block->compressed_size);
		if(add == 0 || block->compressed_size == 0)
			return LZMA_PROG_ERROR;
		size += add;
	}
	// Uncompressed Size
	if(block->uncompressed_size != LZMA_VLI_UNKNOWN) {
		const uint32_t add = lzma_vli_size(block->uncompressed_size);
		if(add == 0)
			return LZMA_PROG_ERROR;
		size += add;
	}
	// List of Filter Flags
	if(block->filters == NULL || block->filters[0].id == LZMA_VLI_UNKNOWN)
		return LZMA_PROG_ERROR;
	for(size_t i = 0; block->filters[i].id != LZMA_VLI_UNKNOWN; ++i) {
		// Don't allow too many filters.
		if(i == LZMA_FILTERS_MAX)
			return LZMA_PROG_ERROR;
		uint32 add;
		return_if_error(lzma_filter_flags_size(&add, block->filters + i));
		size += add;
	}
	// Pad to a multiple of four bytes.
	block->header_size = (size + 3) & ~UINT32_C(3);
	// NOTE: We don't verify that the encoded size of the Block stays
	// within limits. This is because it is possible that we are called
	// with exaggerated Compressed Size (e.g. LZMA_VLI_MAX) to reserve
	// space for Block Header, and later called again with lower,
	// real values.
	return LZMA_OK;
}

lzma_ret lzma_block_header_encode(const lzma_block *block, uint8 *out)
{
	// Validate everything but filters.
	if(lzma_block_unpadded_size(block) == 0 || !lzma_vli_is_valid(block->uncompressed_size))
		return LZMA_PROG_ERROR;
	// Indicate the size of the buffer _excluding_ the CRC32 field.
	const size_t out_size = block->header_size - 4;
	// Store the Block Header Size.
	out[0] = (uint8)(out_size / 4);
	// We write Block Flags in pieces.
	out[1] = 0x00;
	size_t out_pos = 2;
	// Compressed Size
	if(block->compressed_size != LZMA_VLI_UNKNOWN) {
		return_if_error(lzma_vli_encode(block->compressed_size, NULL, out, &out_pos, out_size));
		out[1] |= 0x40;
	}
	// Uncompressed Size
	if(block->uncompressed_size != LZMA_VLI_UNKNOWN) {
		return_if_error(lzma_vli_encode(block->uncompressed_size, NULL, out, &out_pos, out_size));
		out[1] |= 0x80;
	}
	// Filter Flags
	if(block->filters == NULL || block->filters[0].id == LZMA_VLI_UNKNOWN)
		return LZMA_PROG_ERROR;
	size_t filter_count = 0;
	do {
		// There can be a maximum of four filters.
		if(filter_count == LZMA_FILTERS_MAX)
			return LZMA_PROG_ERROR;
		return_if_error(lzma_filter_flags_encode(block->filters + filter_count, out, &out_pos, out_size));
	} while(block->filters[++filter_count].id != LZMA_VLI_UNKNOWN);
	out[1] |= filter_count - 1;
	// Padding
	memzero(out + out_pos, out_size - out_pos);
	// CRC32
	write32le(out + out_size, lzma_crc32(out, out_size, 0));
	return LZMA_OK;
}
//
// block_header_decoder
//
static void free_properties(lzma_block * block, const lzma_allocator * allocator)
{
	// Free allocated filter options. The last array member is not
	// touched after the initialization in the beginning of
	// lzma_block_header_decode(), so we don't need to touch that here.
	for(size_t i = 0; i < LZMA_FILTERS_MAX; ++i) {
		lzma_free(block->filters[i].options, allocator);
		block->filters[i].id = LZMA_VLI_UNKNOWN;
		block->filters[i].options = NULL;
	}
}

lzma_ret lzma_block_header_decode(lzma_block *block, const lzma_allocator *allocator, const uint8 *in)
{
	// NOTE: We consider the header to be corrupt not only when the
	// CRC32 doesn't match, but also when variable-length integers
	// are invalid or over 63 bits, or if the header is too small
	// to contain the claimed information.

	// Initialize the filter options array. This way the caller can
	// safely free() the options even if an error occurs in this function.
	for(size_t i = 0; i <= LZMA_FILTERS_MAX; ++i) {
		block->filters[i].id = LZMA_VLI_UNKNOWN;
		block->filters[i].options = NULL;
	}

	// Versions 0 and 1 are supported. If a newer version was specified,
	// we need to downgrade it.
	if(block->version > 1)
		block->version = 1;

	// This isn't a Block Header option, but since the decompressor will
	// read it if version >= 1, it's better to initialize it here than
	// to expect the caller to do it since in almost all cases this
	// should be false.
	block->ignore_check = false;

	// Validate Block Header Size and Check type. The caller must have
	// already set these, so it is a programming error if this test fails.
	if(lzma_block_header_size_decode(in[0]) != block->header_size || (uint)(block->check) > LZMA_CHECK_ID_MAX)
		return LZMA_PROG_ERROR;
	// Exclude the CRC32 field.
	const size_t in_size = block->header_size - 4;
	// Verify CRC32
	if(lzma_crc32(in, in_size, 0) != read32le(in + in_size)) {
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
		return LZMA_DATA_ERROR;
#endif
	}
	// Check for unsupported flags.
	if(in[1] & 0x3C)
		return LZMA_OPTIONS_ERROR;

	// Start after the Block Header Size and Block Flags fields.
	size_t in_pos = 2;

	// Compressed Size
	if(in[1] & 0x40) {
		return_if_error(lzma_vli_decode(&block->compressed_size,
		    NULL, in, &in_pos, in_size));

		// Validate Compressed Size. This checks that it isn't zero
		// and that the total size of the Block is a valid VLI.
		if(lzma_block_unpadded_size(block) == 0)
			return LZMA_DATA_ERROR;
	}
	else {
		block->compressed_size = LZMA_VLI_UNKNOWN;
	}

	// Uncompressed Size
	if(in[1] & 0x80)
		return_if_error(lzma_vli_decode(&block->uncompressed_size,
		    NULL, in, &in_pos, in_size));
	else
		block->uncompressed_size = LZMA_VLI_UNKNOWN;
	// Filter Flags
	const size_t filter_count = (in[1] & 3U) + 1;
	for(size_t i = 0; i < filter_count; ++i) {
		const lzma_ret ret = lzma_filter_flags_decode(
			&block->filters[i], allocator,
			in, &in_pos, in_size);
		if(ret != LZMA_OK) {
			free_properties(block, allocator);
			return ret;
		}
	}
	// Padding
	while(in_pos < in_size) {
		if(in[in_pos++] != 0x00) {
			free_properties(block, allocator);
			// Possibly some new field present so use
			// LZMA_OPTIONS_ERROR instead of LZMA_DATA_ERROR.
			return LZMA_OPTIONS_ERROR;
		}
	}
	return LZMA_OK;
}
//
// block_buffer_encoder
//
/// Estimate the maximum size of the Block Header and Check fields for
/// a Block that uses LZMA2 uncompressed chunks. We could use
/// lzma_block_header_size() but this is simpler.
///
/// Block Header Size + Block Flags + Compressed Size
/// + Uncompressed Size + Filter Flags for LZMA2 + CRC32 + Check
/// and round up to the next multiple of four to take Header Padding
/// into account.
#define HEADERS_BOUND ((1 + 1 + 2 * LZMA_VLI_BYTES_MAX + 3 + 4 + LZMA_CHECK_SIZE_MAX + 3) & ~3)

static  uint64 lzma2_bound( uint64 uncompressed_size)
{
	// Prevent integer overflow in overhead calculation.
	if(uncompressed_size > COMPRESSED_SIZE_MAX)
		return 0;
	// Calculate the exact overhead of the LZMA2 headers: Round
	// uncompressed_size up to the next multiple of LZMA2_CHUNK_MAX,
	// multiply by the size of per-chunk header, and add one byte for
	// the end marker.
	const  uint64 overhead = ((uncompressed_size + LZMA2_CHUNK_MAX - 1) / LZMA2_CHUNK_MAX) * LZMA2_HEADER_UNCOMPRESSED + 1;
	// Catch the possible integer overflow.
	if(COMPRESSED_SIZE_MAX - overhead < uncompressed_size)
		return 0;
	return uncompressed_size + overhead;
}

extern  uint64 lzma_block_buffer_bound64( uint64 uncompressed_size)
{
	// If the data doesn't compress, we always use uncompressed
	// LZMA2 chunks.
	 uint64 lzma2_size = lzma2_bound(uncompressed_size);
	if(lzma2_size == 0)
		return 0;
	// Take Block Padding into account.
	lzma2_size = (lzma2_size + 3) & ~UINT64_C(3);
	// No risk of integer overflow because lzma2_bound() already takes
	// into account the size of the headers in the Block.
	return HEADERS_BOUND + lzma2_size;
}

size_t lzma_block_buffer_bound(size_t uncompressed_size)
{
	 uint64 ret = lzma_block_buffer_bound64(uncompressed_size);
#if SIZE_MAX < UINT64_MAX
	// Catch the possible integer overflow on 32-bit systems.
	if(ret > SIZE_MAX)
		return 0;
#endif
	return static_cast<size_t>(ret);
}

static lzma_ret block_encode_uncompressed(lzma_block * block, const uint8 * in, size_t in_size,
    uint8 * out, size_t * out_pos, size_t out_size)
{
	// Use LZMA2 uncompressed chunks. We wouldn't need a dictionary at
	// all, but LZMA2 always requires a dictionary, so use the minimum
	// value to minimize memory usage of the decoder.
	lzma_options_lzma lzma2;
	lzma2.dict_size = LZMA_DICT_SIZE_MIN;
	lzma_filter filters[2];
	filters[0].id = LZMA_FILTER_LZMA2;
	filters[0].options = &lzma2;
	filters[1].id = LZMA_VLI_UNKNOWN;
	// Set the above filter options to *block temporarily so that we can
	// encode the Block Header.
	lzma_filter * filters_orig = block->filters;
	block->filters = filters;
	if(lzma_block_header_size(block) != LZMA_OK) {
		block->filters = filters_orig;
		return LZMA_PROG_ERROR;
	}
	// Check that there's enough output space. The caller has already
	// set block->compressed_size to what lzma2_bound() has returned,
	// so we can reuse that value. We know that compressed_size is a
	// known valid VLI and header_size is a small value so their sum
	// will never overflow.
	assert(block->compressed_size == lzma2_bound(in_size));
	if(out_size - *out_pos < block->header_size + block->compressed_size) {
		block->filters = filters_orig;
		return LZMA_BUF_ERROR;
	}
	if(lzma_block_header_encode(block, out + *out_pos) != LZMA_OK) {
		block->filters = filters_orig;
		return LZMA_PROG_ERROR;
	}
	block->filters = filters_orig;
	*out_pos += block->header_size;
	// Encode the data using LZMA2 uncompressed chunks.
	size_t in_pos = 0;
	uint8 control = 0x01; // Dictionary reset
	while(in_pos < in_size) {
		// Control byte: Indicate uncompressed chunk, of which
		// the first resets the dictionary.
		out[(*out_pos)++] = control;
		control = 0x02; // No dictionary reset

		// Size of the uncompressed chunk
		const size_t copy_size = MIN(in_size - in_pos, LZMA2_CHUNK_MAX);
		out[(*out_pos)++] = static_cast<uint8>((copy_size - 1) >> 8);
		out[(*out_pos)++] = (copy_size - 1) & 0xFF;
		// The actual data
		assert(*out_pos + copy_size <= out_size);
		memcpy(out + *out_pos, in + in_pos, copy_size);
		in_pos += copy_size;
		*out_pos += copy_size;
	}
	// End marker
	out[(*out_pos)++] = 0x00;
	assert(*out_pos <= out_size);
	return LZMA_OK;
}

static lzma_ret block_encode_normal(lzma_block * block, const lzma_allocator * allocator, const uint8 * in, size_t in_size, uint8 * out, size_t * out_pos, size_t out_size)
{
	// Find out the size of the Block Header.
	return_if_error(lzma_block_header_size(block));
	// Reserve space for the Block Header and skip it for now.
	if(out_size - *out_pos <= block->header_size)
		return LZMA_BUF_ERROR;
	const size_t out_start = *out_pos;
	*out_pos += block->header_size;
	// Limit out_size so that we stop encoding if the output would grow
	// bigger than what uncompressed Block would be.
	if(out_size - *out_pos > block->compressed_size)
		out_size = static_cast<size_t>(*out_pos + block->compressed_size);
	// TODO: In many common cases this could be optimized to use
	// significantly less memory.
	lzma_next_coder raw_encoder;// = LZMA_NEXT_CODER_INIT;
	lzma_ret ret = lzma_raw_encoder_init(&raw_encoder, allocator, block->filters);
	if(ret == LZMA_OK) {
		size_t in_pos = 0;
		ret = raw_encoder.code(raw_encoder.coder, allocator, in, &in_pos, in_size, out, out_pos, out_size, LZMA_FINISH);
	}
	// NOTE: This needs to be run even if lzma_raw_encoder_init() failed.
	lzma_next_end(&raw_encoder, allocator);
	if(ret == LZMA_STREAM_END) {
		// Compression was successful. Write the Block Header.
		block->compressed_size = *out_pos - (out_start + block->header_size);
		ret = lzma_block_header_encode(block, out + out_start);
		if(ret != LZMA_OK)
			ret = LZMA_PROG_ERROR;
	}
	else if(ret == LZMA_OK) {
		// Output buffer became full.
		ret = LZMA_BUF_ERROR;
	}
	// Reset *out_pos if something went wrong.
	if(ret != LZMA_OK)
		*out_pos = out_start;
	return ret;
}

static lzma_ret block_buffer_encode(lzma_block * block, const lzma_allocator * allocator, const uint8 * in, size_t in_size, uint8 * out, size_t * out_pos, size_t out_size, bool try_to_compress)
{
	// Validate the arguments.
	if(block == NULL || (in == NULL && in_size != 0) || out == NULL || out_pos == NULL || *out_pos > out_size)
		return LZMA_PROG_ERROR;
	// The contents of the structure may depend on the version so
	// check the version before validating the contents of *block.
	if(block->version > 1)
		return LZMA_OPTIONS_ERROR;
	if((uint)(block->check) > LZMA_CHECK_ID_MAX || (try_to_compress && block->filters == NULL))
		return LZMA_PROG_ERROR;
	if(!lzma_check_is_supported(block->check))
		return LZMA_UNSUPPORTED_CHECK;
	// Size of a Block has to be a multiple of four, so limit the size
	// here already. This way we don't need to check it again when adding
	// Block Padding.
	out_size -= (out_size - *out_pos) & 3;
	// Get the size of the Check field.
	const size_t check_size = lzma_check_size(block->check);
	assert(check_size != UINT32_MAX);
	// Reserve space for the Check field.
	if(out_size - *out_pos <= check_size)
		return LZMA_BUF_ERROR;
	out_size -= check_size;
	// Initialize block->uncompressed_size and calculate the worst-case
	// value for block->compressed_size.
	block->uncompressed_size = in_size;
	block->compressed_size = lzma2_bound(in_size);
	if(block->compressed_size == 0)
		return LZMA_DATA_ERROR;
	// Do the actual compression.
	lzma_ret ret = LZMA_BUF_ERROR;
	if(try_to_compress)
		ret = block_encode_normal(block, allocator, in, in_size, out, out_pos, out_size);
	if(ret != LZMA_OK) {
		// If the error was something else than output buffer
		// becoming full, return the error now.
		if(ret != LZMA_BUF_ERROR)
			return ret;
		// The data was uncompressible (at least with the options
		// given to us) or the output buffer was too small. Use the
		// uncompressed chunks of LZMA2 to wrap the data into a valid
		// Block. If we haven't been given enough output space, even
		// this may fail.
		return_if_error(block_encode_uncompressed(block, in, in_size, out, out_pos, out_size));
	}
	assert(*out_pos <= out_size);
	// Block Padding. No buffer overflow here, because we already adjusted
	// out_size so that (out_size - out_start) is a multiple of four.
	// Thus, if the buffer is full, the loop body can never run.
	for(size_t i = (size_t)(block->compressed_size); i &3; ++i) {
		assert(*out_pos < out_size);
		out[(*out_pos)++] = 0x00;
	}
	// If there's no Check field, we are done now.
	if(check_size > 0) {
		// Calculate the integrity check. We reserved space for
		// the Check field earlier so we don't need to check for
		// available output space here.
		lzma_check_state check;
		lzma_check_init(&check, block->check);
		lzma_check_update(&check, block->check, in, in_size);
		lzma_check_finish(&check, block->check);
		memcpy(block->raw_check, check.buffer.u8, check_size);
		memcpy(out + *out_pos, check.buffer.u8, check_size);
		*out_pos += check_size;
	}
	return LZMA_OK;
}

lzma_ret lzma_block_buffer_encode(lzma_block *block, const lzma_allocator *allocator, const uint8 *in, size_t in_size, uint8 *out, size_t *out_pos, size_t out_size)
{
	return block_buffer_encode(block, allocator, in, in_size, out, out_pos, out_size, true);
}

lzma_ret lzma_block_uncomp_encode(lzma_block *block, const uint8 *in, size_t in_size, uint8 *out, size_t *out_pos, size_t out_size)
{
	// It won't allocate any memory from heap so no need
	// for lzma_allocator.
	return block_buffer_encode(block, NULL, in, in_size, out, out_pos, out_size, false);
}
//
// block_buffer_decoder
//
lzma_ret lzma_block_buffer_decode(lzma_block *block, const lzma_allocator *allocator,
    const uint8 *in, size_t *in_pos, size_t in_size, uint8 *out, size_t *out_pos, size_t out_size)
{
	if(in_pos == NULL || (in == NULL && *in_pos != in_size) || *in_pos > in_size || out_pos == NULL || (out == NULL && *out_pos != out_size) || *out_pos > out_size)
		return LZMA_PROG_ERROR;
	// Initialize the Block decoder.
	lzma_next_coder block_decoder;// = LZMA_NEXT_CODER_INIT;
	lzma_ret ret = lzma_block_decoder_init(&block_decoder, allocator, block);
	if(ret == LZMA_OK) {
		// Save the positions so that we can restore them in case
		// an error occurs.
		const size_t in_start = *in_pos;
		const size_t out_start = *out_pos;
		// Do the actual decoding.
		ret = block_decoder.code(block_decoder.coder, allocator, in, in_pos, in_size, out, out_pos, out_size, LZMA_FINISH);
		if(ret == LZMA_STREAM_END) {
			ret = LZMA_OK;
		}
		else {
			if(ret == LZMA_OK) {
				// Either the input was truncated or the
				// output buffer was too small.
				assert(*in_pos == in_size
				   || *out_pos == out_size);

				// If all the input was consumed, then the
				// input is truncated, even if the output
				// buffer is also full. This is because
				// processing the last byte of the Block
				// never produces output.
				//
				// NOTE: This assumption may break when new
				// filters are added, if the end marker of
				// the filter doesn't consume at least one
				// complete byte.
				if(*in_pos == in_size)
					ret = LZMA_DATA_ERROR;
				else
					ret = LZMA_BUF_ERROR;
			}
			// Restore the positions.
			*in_pos = in_start;
			*out_pos = out_start;
		}
	}
	// Free the decoder memory. This needs to be done even if
	// initialization fails, because the internal API doesn't
	// require the initialization function to free its memory on error.
	lzma_next_end(&block_decoder, allocator);
	return ret;
}
//
// block_encoder
//
static lzma_ret block_encode(void * coder_ptr, const lzma_allocator * allocator, const uint8 * in, size_t * in_pos, size_t in_size, uint8 * out,
    size_t * out_pos, size_t out_size, lzma_action action)
{
	lzma_block_encoder_coder * coder = (lzma_block_encoder_coder *)coder_ptr;
	// Check that our amount of input stays in proper limits.
	if(LZMA_VLI_MAX - coder->uncompressed_size < in_size - *in_pos)
		return LZMA_DATA_ERROR;
	switch(coder->sequence) {
		case lzma_block_encoder_coder::SEQ_CODE: {
		    const size_t in_start = *in_pos;
		    const size_t out_start = *out_pos;
		    const lzma_ret ret = coder->next.code(coder->next.coder, allocator, in, in_pos, in_size, out, out_pos, out_size, action);
		    const size_t in_used = *in_pos - in_start;
		    const size_t out_used = *out_pos - out_start;
		    if(COMPRESSED_SIZE_MAX - coder->compressed_size < out_used)
			    return LZMA_DATA_ERROR;
		    coder->compressed_size += out_used;
		    // No need to check for overflow because we have already
		    // checked it at the beginning of this function.
		    coder->uncompressed_size += in_used;
		    lzma_check_update(&coder->check, coder->block->check, in + in_start, in_used);
		    if(ret != LZMA_STREAM_END || action == LZMA_SYNC_FLUSH)
			    return ret;

		    assert(*in_pos == in_size);
		    assert(action == LZMA_FINISH);

		    // Copy the values into coder->block. The caller
		    // may use this information to construct Index.
		    coder->block->compressed_size = coder->compressed_size;
		    coder->block->uncompressed_size = coder->uncompressed_size;
		    coder->sequence = lzma_block_encoder_coder::SEQ_PADDING;
	    }
		// @fallthrough
		case lzma_block_encoder_coder::SEQ_PADDING:
		    // Pad Compressed Data to a multiple of four bytes. We can
		    // use coder->compressed_size for this since we don't need
		    // it for anything else anymore.
		    while(coder->compressed_size & 3) {
			    if(*out_pos >= out_size)
				    return LZMA_OK;
			    out[*out_pos] = 0x00;
			    ++*out_pos;
			    ++coder->compressed_size;
		    }
		    if(coder->block->check == LZMA_CHECK_NONE)
			    return LZMA_STREAM_END;
		    lzma_check_finish(&coder->check, coder->block->check);
		    coder->sequence = lzma_block_encoder_coder::SEQ_CHECK;
		// @fallthrough
		case lzma_block_encoder_coder::SEQ_CHECK: {
		    const size_t check_size = lzma_check_size(coder->block->check);
		    lzma_bufcpy(coder->check.buffer.u8, &coder->pos, check_size,
			out, out_pos, out_size);
		    if(coder->pos < check_size)
			    return LZMA_OK;

		    memcpy(coder->block->raw_check, coder->check.buffer.u8,
			check_size);
		    return LZMA_STREAM_END;
	    }
	}
	return LZMA_PROG_ERROR;
}

static void block_encoder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_block_encoder_coder * coder = (lzma_block_encoder_coder *)coder_ptr;
	lzma_next_end(&coder->next, allocator);
	lzma_free(coder, allocator);
}

static lzma_ret block_encoder_update(void * coder_ptr, const lzma_allocator * allocator,
    const lzma_filter * filters lzma_attribute((__unused__)), const lzma_filter * reversed_filters)
{
	lzma_block_encoder_coder * coder = (lzma_block_encoder_coder *)coder_ptr;
	if(coder->sequence != lzma_block_encoder_coder::SEQ_CODE)
		return LZMA_PROG_ERROR;
	return lzma_next_filter_update(&coder->next, allocator, reversed_filters);
}

extern lzma_ret lzma_block_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, lzma_block * block)
{
	lzma_next_coder_init(&lzma_block_encoder_init, next, allocator);
	if(block == NULL)
		return LZMA_PROG_ERROR;
	// The contents of the structure may depend on the version so
	// check the version first.
	if(block->version > 1)
		return LZMA_OPTIONS_ERROR;
	// If the Check ID is not supported, we cannot calculate the check and
	// thus not create a proper Block.
	if((uint)(block->check) > LZMA_CHECK_ID_MAX)
		return LZMA_PROG_ERROR;
	if(!lzma_check_is_supported(block->check))
		return LZMA_UNSUPPORTED_CHECK;
	// Allocate and initialize *next->coder if needed.
	lzma_block_encoder_coder * coder = (lzma_block_encoder_coder *)next->coder;
	if(!coder) {
		coder = (lzma_block_encoder_coder *)lzma_alloc(sizeof(lzma_block_encoder_coder), allocator);
		if(!coder)
			return LZMA_MEM_ERROR;
		next->coder = coder;
		next->code = &block_encode;
		next->end = &block_encoder_end;
		next->update = &block_encoder_update;
		coder->next.SetDefault();// = LZMA_NEXT_CODER_INIT;
	}
	// Basic initializations
	coder->sequence = lzma_block_encoder_coder::SEQ_CODE;
	coder->block = block;
	coder->compressed_size = 0;
	coder->uncompressed_size = 0;
	coder->pos = 0;
	// Initialize the check
	lzma_check_init(&coder->check, block->check);
	// Initialize the requested filters.
	return lzma_raw_encoder_init(&coder->next, allocator, block->filters);
}

lzma_ret lzma_block_encoder(lzma_stream *strm, lzma_block *block)
{
	lzma_next_strm_init(lzma_block_encoder_init, strm, block);
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;
	return LZMA_OK;
}
//
// block_decoder
//
static inline bool update_size(lzma_vli * size, lzma_vli add, lzma_vli limit)
{
	if(limit > LZMA_VLI_MAX)
		limit = LZMA_VLI_MAX;
	if(limit < *size || limit - *size < add)
		return true;
	*size += add;
	return false;
}

static inline bool is_size_valid(lzma_vli size, lzma_vli reference)
{
	return reference == LZMA_VLI_UNKNOWN || reference == size;
}

static lzma_ret block_decode(void * coder_ptr, const lzma_allocator * allocator, const uint8 * in, size_t * in_pos,
    size_t in_size, uint8 * out, size_t * out_pos, size_t out_size, lzma_action action)
{
	lzma_block_decoder_coder * coder = (lzma_block_decoder_coder *)coder_ptr;
	switch(coder->sequence) {
		case lzma_block_decoder_coder::SEQ_CODE: {
		    const size_t in_start = *in_pos;
		    const size_t out_start = *out_pos;
		    const lzma_ret ret = coder->next.code(coder->next.coder, allocator, in, in_pos, in_size, out, out_pos, out_size, action);
		    const size_t in_used = *in_pos - in_start;
		    const size_t out_used = *out_pos - out_start;
		    // NOTE: We compare to compressed_limit here, which prevents
		    // the total size of the Block growing past LZMA_VLI_MAX.
		    if(update_size(&coder->compressed_size, in_used, coder->compressed_limit) || update_size(&coder->uncompressed_size,
				out_used, coder->block->uncompressed_size))
			    return LZMA_DATA_ERROR;
		    if(!coder->ignore_check)
			    lzma_check_update(&coder->check, coder->block->check, out + out_start, out_used);
		    if(ret != LZMA_STREAM_END)
			    return ret;
		    // Compressed and Uncompressed Sizes are now at their final
		    // values. Verify that they match the values given to us.
		    if(!is_size_valid(coder->compressed_size, coder->block->compressed_size) || !is_size_valid(coder->uncompressed_size, coder->block->uncompressed_size))
			    return LZMA_DATA_ERROR;
		    // Copy the values into coder->block. The caller
		    // may use this information to construct Index.
		    coder->block->compressed_size = coder->compressed_size;
		    coder->block->uncompressed_size = coder->uncompressed_size;
		    coder->sequence = lzma_block_decoder_coder::SEQ_PADDING;
	    }
		// @fallthrough
		case lzma_block_decoder_coder::SEQ_PADDING:
		    // Compressed Data is padded to a multiple of four bytes.
		    while(coder->compressed_size & 3) {
			    if(*in_pos >= in_size)
				    return LZMA_OK;
			    // We use compressed_size here just get the Padding
			    // right. The actual Compressed Size was stored to
			    // coder->block already, and won't be modified by
			    // us anymore.
			    ++coder->compressed_size;
			    if(in[(*in_pos)++] != 0x00)
				    return LZMA_DATA_ERROR;
		    }
		    if(coder->block->check == LZMA_CHECK_NONE)
			    return LZMA_STREAM_END;
		    if(!coder->ignore_check)
			    lzma_check_finish(&coder->check, coder->block->check);
		    coder->sequence = lzma_block_decoder_coder::SEQ_CHECK;
		// @fallthrough
		case lzma_block_decoder_coder::SEQ_CHECK: {
		    const size_t check_size = lzma_check_size(coder->block->check);
		    lzma_bufcpy(in, in_pos, in_size, coder->block->raw_check,
			&coder->check_pos, check_size);
		    if(coder->check_pos < check_size)
			    return LZMA_OK;
		    // Validate the Check only if we support it.
		    // coder->check.buffer may be uninitialized
		    // when the Check ID is not supported.
		    if(!coder->ignore_check && lzma_check_is_supported(coder->block->check) && memcmp(coder->block->raw_check, coder->check.buffer.u8, check_size) != 0)
			    return LZMA_DATA_ERROR;
		    return LZMA_STREAM_END;
	    }
	}
	return LZMA_PROG_ERROR;
}

static void block_decoder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_block_decoder_coder * coder = (lzma_block_decoder_coder *)coder_ptr;
	lzma_next_end(&coder->next, allocator);
	lzma_free(coder, allocator);
}

extern lzma_ret lzma_block_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, lzma_block * block)
{
	lzma_next_coder_init(&lzma_block_decoder_init, next, allocator);
	// Validate the options. lzma_block_unpadded_size() does that for us
	// except for Uncompressed Size and filters. Filters are validated
	// by the raw decoder.
	if(lzma_block_unpadded_size(block) == 0 || !lzma_vli_is_valid(block->uncompressed_size))
		return LZMA_PROG_ERROR;
	// Allocate *next->coder if needed.
	lzma_block_decoder_coder * coder = (lzma_block_decoder_coder *)next->coder;
	if(!coder) {
		coder = (lzma_block_decoder_coder *)lzma_alloc(sizeof(lzma_block_decoder_coder), allocator);
		if(!coder)
			return LZMA_MEM_ERROR;
		next->coder = coder;
		next->code = &block_decode;
		next->end = &block_decoder_end;
		coder->next.SetDefault(); // = LZMA_NEXT_CODER_INIT;
	}
	// Basic initializations
	coder->sequence = lzma_block_decoder_coder::SEQ_CODE;
	coder->block = block;
	coder->compressed_size = 0;
	coder->uncompressed_size = 0;
	// If Compressed Size is not known, we calculate the maximum allowed
	// value so that encoded size of the Block (including Block Padding)
	// is still a valid VLI and a multiple of four.
	coder->compressed_limit = (block->compressed_size == LZMA_VLI_UNKNOWN) ? (LZMA_VLI_MAX & ~LZMA_VLI_C(3)) - block->header_size - lzma_check_size(block->check) : block->compressed_size;
	// Initialize the check. It's caller's problem if the Check ID is not
	// supported, and the Block decoder cannot verify the Check field.
	// Caller can test lzma_check_is_supported(block->check).
	coder->check_pos = 0;
	lzma_check_init(&coder->check, block->check);
	coder->ignore_check = (block->version >= 1) ? LOGIC(block->ignore_check) : false;
	// Initialize the filter chain.
	return lzma_raw_decoder_init(&coder->next, allocator, block->filters);
}

lzma_ret lzma_block_decoder(lzma_stream *strm, lzma_block *block)
{
	lzma_next_strm_init(lzma_block_decoder_init, strm, block);
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;
	return LZMA_OK;
}
