/* $Id: tif_thunder.c,v 1.13 2016-09-04 21:32:56 erouault Exp $ */
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 */
#include <slib-internal.h>
#pragma hdrstop
#include "tiffiop.h"
#ifdef THUNDER_SUPPORT
/*
 * TIFF Library.
 *
 * ThunderScan 4-bit Compression Algorithm Support
 */

/*
 * ThunderScan uses an encoding scheme designed for
 * 4-bit pixel values.  Data is encoded in bytes, with
 * each byte split into a 2-bit code word and a 6-bit
 * data value.  The encoding gives raw data, runs of
 * pixels, or pixel values encoded as a delta from the
 * previous pixel value.  For the latter, either 2-bit
 * or 3-bit delta values are used, with the deltas packed
 * into a single byte.
 */
#define THUNDER_DATA            0x3f    /* mask for 6-bit data */
#define THUNDER_CODE            0xc0    /* mask for 2-bit code word */
/* code values */
#define THUNDER_RUN             0x00    /* run of pixels w/ encoded count */
#define THUNDER_2BITDELTAS      0x40    /* 3 pixels w/ encoded 2-bit deltas */
#define DELTA2_SKIP         2       /* skip code for 2-bit deltas */
#define THUNDER_3BITDELTAS      0x80    /* 2 pixels w/ encoded 3-bit deltas */
#define DELTA3_SKIP         4       /* skip code for 3-bit deltas */
#define THUNDER_RAW             0xc0    /* raw data encoded */

static const int twobitdeltas[4] = { 0, 1, 0, -1 };
static const int threebitdeltas[8] = { 0, 1, 2, 3, 0, -3, -2, -1 };

#define SETPIXEL(op, v) {		      \
		lastpixel = (v) & 0xf;		      \
		if(npixels < maxpixels) { \
			if(npixels++ & 1)		   \
				*op++ |= lastpixel;		  \
			else				    \
				op[0] = static_cast<uint8>(lastpixel << 4); \
		}				      \
}

static int ThunderSetupDecode(TIFF * tif)
{
	static const char module[] = __FUNCTION__;
	if(tif->tif_dir.td_bitspersample != 4) {
		TIFFErrorExt(tif->tif_clientdata, module, "Wrong bitspersample value (%d), Thunder decoder only supports 4bits per sample.",
		    (int)tif->tif_dir.td_bitspersample);
		return 0;
	}
	return 1;
}

static int ThunderDecode(TIFF * tif, uint8 * op, tmsize_t maxpixels)
{
	static const char module[] = __FUNCTION__;
	uchar * bp = (uchar *)tif->tif_rawcp;
	tmsize_t cc = tif->tif_rawcc;
	unsigned int lastpixel = 0;
	tmsize_t npixels = 0;
	while(cc > 0 && npixels < maxpixels) {
		int delta;
		int n = *bp++;
		cc--;
		switch(n & THUNDER_CODE) {
			case THUNDER_RUN:       /* pixel run */
			    /*
			 * Replicate the last pixel n times,
			 * where n is the lower-order 6 bits.
			     */
			    if(npixels & 1) {
				    op[0] |= lastpixel;
				    lastpixel = *op++; npixels++; n--;
			    }
			    else
				    lastpixel |= lastpixel << 4;
			    npixels += n;
			    if(npixels < maxpixels) {
				    for(; n > 0; n -= 2)
					    *op++ = (uint8)lastpixel;
			    }
			    if(n == -1)
				    *--op &= 0xf0;
			    lastpixel &= 0xf;
			    break;
			case THUNDER_2BITDELTAS: /* 2-bit deltas */
			    if((delta = ((n >> 4) & 3)) != DELTA2_SKIP)
				    SETPIXEL(op, lastpixel + twobitdeltas[delta]);
			    if((delta = ((n >> 2) & 3)) != DELTA2_SKIP)
				    SETPIXEL(op, lastpixel + twobitdeltas[delta]);
			    if((delta = (n & 3)) != DELTA2_SKIP)
				    SETPIXEL(op, lastpixel + twobitdeltas[delta]);
			    break;
			case THUNDER_3BITDELTAS: /* 3-bit deltas */
			    if((delta = ((n >> 3) & 7)) != DELTA3_SKIP)
				    SETPIXEL(op, lastpixel + threebitdeltas[delta]);
			    if((delta = (n & 7)) != DELTA3_SKIP)
				    SETPIXEL(op, lastpixel + threebitdeltas[delta]);
			    break;
			case THUNDER_RAW:       /* raw data */
			    SETPIXEL(op, n);
			    break;
		}
	}
	tif->tif_rawcp = reinterpret_cast<uint8 *>(bp);
	tif->tif_rawcc = cc;
	if(npixels != maxpixels) {
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
		TIFFErrorExt(tif->tif_clientdata, module, "%s data at scanline %lu (%I64u != %I64u)", npixels < maxpixels ? "Not enough" : "Too much",
		    (ulong)tif->tif_row, (uint64)npixels, (uint64)maxpixels);
#else
		TIFFErrorExt(tif->tif_clientdata, module, "%s data at scanline %lu (%llu != %llu)", npixels < maxpixels ? "Not enough" : "Too much",
		    (ulong)tif->tif_row, (uint64)npixels, (uint64)maxpixels);
#endif
		return 0;
	}

	return 1;
}

static int ThunderDecodeRow(TIFF * tif, uint8 * buf, tmsize_t occ, uint16 s)
{
	static const char module[] = __FUNCTION__;
	uint8 * row = buf;
	(void)s;
	if(occ % tif->tif_scanlinesize) {
		TIFFErrorExt(tif->tif_clientdata, module, "Fractional scanlines cannot be read");
		return 0;
	}
	while(occ > 0) {
		if(!ThunderDecode(tif, row, tif->tif_dir.td_imagewidth))
			return 0;
		occ -= tif->tif_scanlinesize;
		row += tif->tif_scanlinesize;
	}
	return 1;
}

int TIFFInitThunderScan(TIFF * tif, int scheme)
{
	(void)scheme;
	tif->tif_setupdecode = ThunderSetupDecode;
	tif->tif_decoderow = ThunderDecodeRow;
	tif->tif_decodestrip = ThunderDecodeRow;
	return 1;
}

#endif /* THUNDER_SUPPORT */
