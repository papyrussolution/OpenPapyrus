/// \file       check.c
/// \brief      Single API to access different integrity checks
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
//#include "check-internal.h"

bool lzma_check_is_supported(lzma_check type)
{
	if((uint)(type) > LZMA_CHECK_ID_MAX)
		return false;
	static const bool available_checks[LZMA_CHECK_ID_MAX + 1] = {
		true,   // LZMA_CHECK_NONE

#ifdef HAVE_CHECK_CRC32
		true,
#else
		false,
#endif
		false,  // Reserved
		false,  // Reserved
#ifdef HAVE_CHECK_CRC64
		true,
#else
		false,
#endif
		false,  // Reserved
		false,  // Reserved
		false,  // Reserved
		false,  // Reserved
		false,  // Reserved
#ifdef HAVE_CHECK_SHA256
		true,
#else
		false,
#endif
		false,  // Reserved
		false,  // Reserved
		false,  // Reserved
		false,  // Reserved
		false,  // Reserved
	};
	return available_checks[(uint)(type)];
}

uint32_t lzma_check_size(lzma_check type)
{
	if((uint)(type) > LZMA_CHECK_ID_MAX)
		return UINT32_MAX;
	// See file-format.txt section 2.1.1.2.
	static const uint8 check_sizes[LZMA_CHECK_ID_MAX + 1] = {
		0,
		4, 4, 4,
		8, 8, 8,
		16, 16, 16,
		32, 32, 32,
		64, 64, 64
	};
	return check_sizes[(uint)(type)];
}

extern void lzma_check_init(lzma_check_state * check, lzma_check type)
{
	switch(type) {
		case LZMA_CHECK_NONE:
		    break;
#ifdef HAVE_CHECK_CRC32
		case LZMA_CHECK_CRC32: check->state.crc32 = 0; break;
#endif
#ifdef HAVE_CHECK_CRC64
		case LZMA_CHECK_CRC64: check->state.crc64 = 0; break;
#endif
#ifdef HAVE_CHECK_SHA256
		case LZMA_CHECK_SHA256: lzma_sha256_init(check); break;
#endif
		default:
		    break;
	}
}

extern void lzma_check_update(lzma_check_state * check, lzma_check type, const uint8 * buf, size_t size)
{
	switch(type) {
#ifdef HAVE_CHECK_CRC32
		case LZMA_CHECK_CRC32: check->state.crc32 = lzma_crc32(buf, size, check->state.crc32); break;
#endif
#ifdef HAVE_CHECK_CRC64
		case LZMA_CHECK_CRC64: check->state.crc64 = lzma_crc64(buf, size, check->state.crc64); break;
#endif
#ifdef HAVE_CHECK_SHA256
		case LZMA_CHECK_SHA256: lzma_sha256_update(buf, size, check); break;
#endif
		default:
		    break;
	}
}

extern void lzma_check_finish(lzma_check_state * check, lzma_check type)
{
	switch(type) {
#ifdef HAVE_CHECK_CRC32
		case LZMA_CHECK_CRC32: check->buffer.u32[0] = conv32le(check->state.crc32); break;
#endif
#ifdef HAVE_CHECK_CRC64
		case LZMA_CHECK_CRC64: check->buffer.u64[0] = conv64le(check->state.crc64); break;
#endif
#ifdef HAVE_CHECK_SHA256
		case LZMA_CHECK_SHA256: lzma_sha256_finish(check); break;
#endif
		default:
		    break;
	}
}
