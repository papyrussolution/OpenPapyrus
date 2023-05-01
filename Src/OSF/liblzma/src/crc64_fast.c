/// \file       crc64.c
/// \brief      CRC64 calculation
///
/// Calculate the CRC64 using the slice-by-four algorithm. This is the same
/// idea that is used in crc32_fast.c, but for CRC64 we use only four tables
/// instead of eight to avoid increasing CPU cache usage.
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
//#include "check-internal.h"
//#include "crc_macros.h"

#ifdef WORDS_BIGENDIAN
	#define A1(x) ((x) >> 56)
#else
	#define A1 CRC_A
#endif

// See the comments in crc32_fast.c. They aren't duplicated here.
uint64_t lzma_crc64(const uint8 *buf, size_t size, uint64_t crc)
{
	crc = ~crc;
#ifdef WORDS_BIGENDIAN
	crc = bswap64(crc);
#endif
	if(size > 4) {
		while((uintptr_t)(buf) & 3) {
			crc = lzma_crc64_table[0][*buf++ ^ A1(crc)] ^ CRC_S8(crc);
			--size;
		}
		const uint8 * const limit = buf + (size & ~(size_t)(3));
		size &= (size_t)(3);
		while(buf < limit) {
#ifdef WORDS_BIGENDIAN
			const uint32_t tmp = (crc >> 32) ^ aligned_read32ne(buf);
#else
			const uint32_t tmp = static_cast<uint32_t>(crc ^ aligned_read32ne(buf));
#endif
			buf += 4;
			crc = lzma_crc64_table[3][CRC_A(tmp)] ^ lzma_crc64_table[2][CRC_B(tmp)] ^ CRC_S32(crc) ^ lzma_crc64_table[1][CRC_C(tmp)] ^ lzma_crc64_table[0][CRC_D(tmp)];
		}
	}
	while(size-- != 0)
		crc = lzma_crc64_table[0][*buf++ ^ A1(crc)] ^ CRC_S8(crc);
#ifdef WORDS_BIGENDIAN
	crc = bswap64(crc);
#endif
	return ~crc;
}
