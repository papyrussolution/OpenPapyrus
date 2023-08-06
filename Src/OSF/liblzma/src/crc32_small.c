/// \file       crc32_small.c
/// \brief      CRC32 calculation (size-optimized)
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop

#if 0 // @sobolev {

uint32_t lzma_crc32_table[1][256];

static void crc32_init(void)
{
	//static const uint32_t poly32 = UINT32_C(0xEDB88320);
	for(size_t b = 0; b < 256; ++b) {
		uint32_t r = b;
		for(size_t i = 0; i < 8; ++i) {
			if(r & 1)
				r = (r >> 1) ^ /*poly32*/SlConst::CrcPoly_CCITT32;
			else
				r >>= 1;
		}
		lzma_crc32_table[0][b] = r;
	}
}

extern void lzma_crc32_init(void)
{
	mythread_once(crc32_init);
}

uint32_t lzma_crc32(const uint8 *buf, size_t size, uint32_t crc)
{
	lzma_crc32_init();
	crc = ~crc;
	while(size != 0) {
		crc = lzma_crc32_table[0][*buf++ ^ (crc & 0xFF)] ^ (crc >> 8);
		--size;
	}
	return ~crc;
}

#endif // } 0 @sobolev