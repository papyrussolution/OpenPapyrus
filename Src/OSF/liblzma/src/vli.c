// VLI.C
// variable-length integers
// Author:     Lasse Collin
// This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
//
// Descr: Calculates the encoded size of a variable-length integer
//
uint32_t lzma_vli_size(lzma_vli vli)
{
	if(vli > LZMA_VLI_MAX)
		return 0;
	uint32_t i = 0;
	do {
		vli >>= 7;
		++i;
	} while(vli != 0);
	assert(i <= LZMA_VLI_BYTES_MAX);
	return i;
}
//
// Descr: Encodes variable-length integers
//
lzma_ret lzma_vli_encode(lzma_vli vli, size_t * vli_pos, uint8_t * out, size_t * out_pos, size_t out_size)
{
	// If we haven't been given vli_pos, work in single-call mode.
	size_t vli_pos_internal = 0;
	if(vli_pos == NULL) {
		vli_pos = &vli_pos_internal;
		// In single-call mode, we expect that the caller has
		// reserved enough output space.
		if(*out_pos >= out_size)
			return LZMA_PROG_ERROR;
	}
	else {
		// This never happens when we are called by liblzma, but
		// may happen if called directly from an application.
		if(*out_pos >= out_size)
			return LZMA_BUF_ERROR;
	}
	// Validate the arguments.
	if(*vli_pos >= LZMA_VLI_BYTES_MAX || vli > LZMA_VLI_MAX)
		return LZMA_PROG_ERROR;
	// Shift vli so that the next bits to encode are the lowest. In
	// single-call mode this never changes vli since *vli_pos is zero.
	vli >>= *vli_pos * 7;
	// Write the non-last bytes in a loop.
	while(vli >= 0x80) {
		// We don't need *vli_pos during this function call anymore,
		// but update it here so that it is ready if we need to
		// return before the whole integer has been decoded.
		++*vli_pos;
		assert(*vli_pos < LZMA_VLI_BYTES_MAX);
		// Write the next byte.
		out[*out_pos] = (uint8_t)(vli) | 0x80;
		vli >>= 7;
		if(++*out_pos == out_size)
			return vli_pos == &vli_pos_internal ? LZMA_PROG_ERROR : LZMA_OK;
	}
	// Write the last byte.
	out[*out_pos] = (uint8_t)(vli);
	++*out_pos;
	++*vli_pos;
	return vli_pos == &vli_pos_internal ? LZMA_OK : LZMA_STREAM_END;
}
//
// Descr: Decodes variable-length integers
//
lzma_ret lzma_vli_decode(lzma_vli * vli, size_t * vli_pos, const uint8_t * in, size_t * in_pos, size_t in_size)
{
	// If we haven't been given vli_pos, work in single-call mode.
	size_t vli_pos_internal = 0;
	if(vli_pos == NULL) {
		vli_pos = &vli_pos_internal;
		*vli = 0;
		// If there's no input, use LZMA_DATA_ERROR. This way it is
		// easy to decode VLIs from buffers that have known size,
		// and get the correct error code in case the buffer is
		// too short.
		if(*in_pos >= in_size)
			return LZMA_DATA_ERROR;
	}
	else {
		// Initialize *vli when starting to decode a new integer.
		if(*vli_pos == 0)
			*vli = 0;
		// Validate the arguments.
		if(*vli_pos >= LZMA_VLI_BYTES_MAX || (*vli >> (*vli_pos * 7)) != 0)
			return LZMA_PROG_ERROR; ;
		if(*in_pos >= in_size)
			return LZMA_BUF_ERROR;
	}
	do {
		// Read the next byte. Use a temporary variable so that we
		// can update *in_pos immediately.
		const uint8_t byte = in[*in_pos];
		++*in_pos;
		// Add the newly read byte to *vli.
		*vli += (lzma_vli)(byte & 0x7F) << (*vli_pos * 7);
		++*vli_pos;
		// Check if this is the last byte of a multibyte integer.
		if((byte & 0x80) == 0) {
			// We don't allow using variable-length integers as
			// padding i.e. the encoding must use the most the
			// compact form.
			if(byte == 0x00 && *vli_pos > 1)
				return LZMA_DATA_ERROR;
			return vli_pos == &vli_pos_internal ? LZMA_OK : LZMA_STREAM_END;
		}
		// There is at least one more byte coming. If we have already
		// read maximum number of bytes, the integer is considered
		// corrupt.
		//
		// If we need bigger integers in future, old versions liblzma
		// will confusingly indicate the file being corrupt instead of
		// unsupported. I suppose it's still better this way, because
		// in the foreseeable future (writing this in 2008) the only
		// reason why files would appear having over 63-bit integers
		// is that the files are simply corrupt.
		if(*vli_pos == LZMA_VLI_BYTES_MAX)
			return LZMA_DATA_ERROR;
	} while(*in_pos < in_size);
	return vli_pos == &vli_pos_internal ? LZMA_DATA_ERROR : LZMA_OK;
}

