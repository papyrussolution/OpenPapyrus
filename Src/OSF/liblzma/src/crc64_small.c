/// \file       crc64_small.c
/// \brief      CRC64 calculation (size-optimized)
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop

static  uint64 crc64_table[256];

static void crc64_init(void)
{
	//static const  uint64 poly64 = /*0xC96C5795D7870F42ULL*/SlConst::CrcPoly_64;
	for(size_t b = 0; b < 256; ++b) {
		uint64 r = b;
		for(size_t i = 0; i < 8; ++i) {
			if(r & 1)
				r = (r >> 1) ^ SlConst::CrcPoly_64;
			else
				r >>= 1;
		}
		crc64_table[b] = r;
	}
}

 uint64 lzma_crc64(const uint8 *buf, size_t size,  uint64 crc)
{
	mythread_once(crc64_init);
	crc = ~crc;
	while(size != 0) {
		crc = crc64_table[*buf++ ^ (crc & 0xFF)] ^ (crc >> 8);
		--size;
	}
	return ~crc;
}
