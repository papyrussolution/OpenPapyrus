/// \file       crc32.c
/// \brief      CRC32 calculation
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
// Calculate the CRC32 using the slice-by-eight algorithm.
// It is explained in this document:
// http://www.intel.com/technology/comms/perfnet/download/CRC_generators.pdf
// The code in this file is not the same as in Intel's paper, but
// the basic principle is identical.
//
#include "common.h"
#pragma hdrstop
//#include "check-internal.h"
//#include "crc_macros.h"
//
// If you make any changes, do some benchmarking! Seemingly unrelated
// changes can very easily ruin the performance (and very probably is
// very compiler dependent).
//
uint32_t lzma_crc32(const uint8 *buf, size_t size, uint32_t crc)
{
	crc = ~crc;
#ifdef SL_BIGENDIAN
	crc = bswap32(crc);
#endif
	if(size > 8) {
		// Fix the alignment, if needed. The if statement above
		// ensures that this won't read past the end of buf[].
		while((uintptr_t)(buf) & 7) {
			crc = lzma_crc32_table[0][*buf++ ^ CRC_A(crc)] ^ CRC_S8(crc);
			--size;
		}
		// Calculate the position where to stop.
		const uint8 * const limit = buf + (size & ~(size_t)(7));
		// Calculate how many bytes must be calculated separately before returning the result.
		size &= (size_t)(7);
		// Calculate the CRC32 using the slice-by-eight algorithm.
		while(buf < limit) {
			crc ^= aligned_read32ne(buf);
			buf += 4;
			crc = lzma_crc32_table[7][CRC_A(crc)] ^ lzma_crc32_table[6][CRC_B(crc)] ^ lzma_crc32_table[5][CRC_C(crc)] ^ lzma_crc32_table[4][CRC_D(crc)];
			const uint32_t tmp = aligned_read32ne(buf);
			buf += 4;
			// At least with some compilers, it is critical for
			// performance, that the crc variable is XORed
			// between the two table-lookup pairs.
			crc = lzma_crc32_table[3][CRC_A(tmp)] ^ lzma_crc32_table[2][CRC_B(tmp)] ^ crc ^ lzma_crc32_table[1][CRC_C(tmp)] ^ lzma_crc32_table[0][CRC_D(tmp)];
		}
	}
	while(size-- != 0)
		crc = lzma_crc32_table[0][*buf++ ^ CRC_A(crc)] ^ CRC_S8(crc);
#ifdef SL_BIGENDIAN
	crc = bswap32(crc);
#endif
	return ~crc;
}
