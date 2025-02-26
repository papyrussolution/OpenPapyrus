/* $Header: /cvs/maptools/cvsroot/libtiff/libtiff/tif_dumpmode.c,v 1.15 2015-12-12 18:04:26 erouault Exp $ */
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
/*
 * TIFF Library.
 *
 * "Null" Compression Algorithm Support.
 */
#include <slib-internal.h>
#pragma hdrstop
#include "tiffiop.h"

static int DumpFixupTags(TIFF * tif)
{
	(void)tif;
	return 1;
}

/*
 * Encode a hunk of pixels.
 */
static int DumpModeEncode(TIFF * tif, uint8 * pp, tmsize_t cc, uint16 s)
{
	(void)s;
	while(cc > 0) {
		tmsize_t n = cc;
		if(tif->tif_rawcc + n > tif->tif_rawdatasize)
			n = tif->tif_rawdatasize - tif->tif_rawcc;
		assert(n > 0);
		/*
		 * Avoid copy if client has setup raw
		 * data buffer to avoid extra copy.
		 */
		if(tif->tif_rawcp != pp)
			memcpy(tif->tif_rawcp, pp, n);
		tif->tif_rawcp += n;
		tif->tif_rawcc += n;
		pp += n;
		cc -= n;
		if(tif->tif_rawcc >= tif->tif_rawdatasize && !TIFFFlushData1(tif))
			return 0;
	}
	return 1;
}
/*
 * Decode a hunk of pixels.
 */
static int DumpModeDecode(TIFF * tif, uint8 * buf, tmsize_t cc, uint16 s)
{
	static const char module[] = __FUNCTION__;
	(void)s;
	if(tif->tif_rawcc < cc) {
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
		TIFFErrorExt(tif->tif_clientdata, module, "Not enough data for scanline %lu, expected a request for at most %I64d bytes, got a request for %I64d bytes",
		    (ulong)tif->tif_row, (signed __int64)tif->tif_rawcc, (signed __int64)cc);
#else
		TIFFErrorExt(tif->tif_clientdata, module, "Not enough data for scanline %lu, expected a request for at most %lld bytes, got a request for %lld bytes",
		    (ulong)tif->tif_row, (signed long long)tif->tif_rawcc, (signed long long)cc);
#endif
		return 0;
	}
	/*
	 * Avoid copy if client has setup raw
	 * data buffer to avoid extra copy.
	 */
	if(tif->tif_rawcp != buf)
		memcpy(buf, tif->tif_rawcp, cc);
	tif->tif_rawcp += cc;
	tif->tif_rawcc -= cc;
	return 1;
}

/*
 * Seek forwards nrows in the current strip.
 */
static int DumpModeSeek(TIFF * tif, uint32 nrows)
{
	tif->tif_rawcp += nrows * tif->tif_scanlinesize;
	tif->tif_rawcc -= nrows * tif->tif_scanlinesize;
	return 1;
}

/*
 * Initialize dump mode.
 */
int TIFFInitDumpMode(TIFF * tif, int scheme)
{
	(void)scheme;
	tif->tif_fixuptags = DumpFixupTags;
	tif->tif_decoderow = DumpModeDecode;
	tif->tif_decodestrip = DumpModeDecode;
	tif->tif_decodetile = DumpModeDecode;
	tif->tif_encoderow = DumpModeEncode;
	tif->tif_encodestrip = DumpModeEncode;
	tif->tif_encodetile = DumpModeEncode;
	tif->tif_seek = DumpModeSeek;
	return 1;
}
