/*-
 * Copyright (c) 2009 Joerg  Sonnenberger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * $FreeBSD: head/lib/libarchive/archive_crc32.h 201102 2009-12-28 03:11:36Z kientzle $
 */

#ifndef ARCHIVE_CRC32_H
#define ARCHIVE_CRC32_H

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif
// 
// When zlib is unavailable, we should still be able to validate
// uncompressed zip archives.  That requires us to be able to compute
// the CRC32 check value.  This is a drop-in compatible replacement
// for crc32() from zlib.  It's slower than the zlib implementation,
// but still pretty fast: This runs about 300MB/s on my 3GHz P4
// compared to about 800MB/s for the zlib implementation.
// 
static ulong crc32(ulong crc, const void * _p, size_t len)
{
	ulong crc2, b, i;
	const uchar * p = _p;
	static volatile int crc_tbl_inited = 0;
	static ulong crc_tbl[256];
	if(!crc_tbl_inited) {
		for(b = 0; b < 256; ++b) {
			crc2 = b;
			for(i = 8; i > 0; --i) {
				if(crc2 & 1)
					crc2 = (crc2 >> 1) ^ /*0xedb88320UL*/SlConst::CrcPoly_CCITT32;
				else
					crc2 = (crc2 >> 1);
			}
			crc_tbl[b] = crc2;
		}
		crc_tbl_inited = 1;
	}
	crc = crc ^ 0xffffffffUL;
	// A use of this loop is about 20% - 30% faster than no use version in any optimization option of gcc.
	for(; len >= 8; len -= 8) {
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	}
	while(len--)
		crc = crc_tbl[(crc ^ *p++) & 0xff] ^ (crc >> 8);
	return (crc ^ 0xffffffffUL);
}

#endif
