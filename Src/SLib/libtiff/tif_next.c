/* $Id: tif_next.c,v 1.19 2016-09-04 21:32:56 erouault Exp $ */
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

#ifdef NEXT_SUPPORT
/*
 * TIFF Library.
 *
 * NeXT 2-bit Grey Scale Compression Algorithm Support
 */

#define SETPIXEL(op, v) {			\
		switch(npixels++ & 3) {		       \
			case 0: op[0]  = (uchar)((v) << 6); break;     \
			case 1: op[0] |= (v) << 4; break;	\
			case 2: op[0] |= (v) << 2; break;	\
			case 3: *op++ |= (v);      op_offset++; break;	\
		}					\
}

#define LITERALROW      0x00
#define LITERALSPAN     0x40
#define WHITE           ((1<<2)-1)

static int NeXTDecode(TIFF * tif, uint8 * buf, tmsize_t occ, uint16 s)
{
	static const char module[] = __FUNCTION__;
	uchar * bp, * op;
	tmsize_t cc;
	uint8 * row;
	tmsize_t scanline, n;
	(void)s;
	/*
	 * Each scanline is assumed to start off as all
	 * white (we assume a PhotometricInterpretation
	 * of ``min-is-black'').
	 */
	for(op = (uchar *)buf, cc = occ; cc-- > 0; )
		*op++ = 0xff;
	bp = (uchar *)tif->tif_rawcp;
	cc = tif->tif_rawcc;
	scanline = tif->tif_scanlinesize;
	if(occ % scanline) {
		TIFFErrorExt(tif->tif_clientdata, module, "Fractional scanlines cannot be read");
		return 0;
	}
	for(row = buf; cc > 0 && occ > 0; occ -= scanline, row += scanline) {
		n = *bp++;
		cc--;
		switch(n) {
			case LITERALROW:
			    /*
			 * The entire scanline is given as literal values.
			     */
			    if(cc < scanline)
				    goto bad;
			    memcpy(row, bp, scanline);
			    bp += scanline;
			    cc -= scanline;
			    break;
			case LITERALSPAN: {
			    tmsize_t off;
			    /*
			 * The scanline has a literal span that begins at some
			 * offset.
			     */
			    if(cc < 4)
				    goto bad;
			    off = (bp[0] * 256) + bp[1];
			    n = (bp[2] * 256) + bp[3];
			    if(cc < 4+n || off+n > scanline)
				    goto bad;
			    memcpy(row+off, bp+4, n);
			    bp += 4+n;
			    cc -= 4+n;
			    break;
		    }
			default: {
			    uint32 npixels = 0, grey;
			    tmsize_t op_offset = 0;
			    uint32 imagewidth = tif->tif_dir.td_imagewidth;
			    if(isTiled(tif) )
				    imagewidth = tif->tif_dir.td_tilewidth;

			    /*
			 * The scanline is composed of a sequence of constant
			 * color ``runs''.  We shift into ``run mode'' and
			 * interpret bytes as codes of the form
			 * <color><npixels> until we've filled the scanline.
			     */
			    op = row;
			    for(;;) {
				    grey = (uint32)((n>>6) & 0x3);
				    n &= 0x3f;
				    /*
				 * Ensure the run does not exceed the scanline
				 * bounds, potentially resulting in a security
				 * issue.
				     */
				    while(n-- > 0 && npixels < imagewidth && op_offset < scanline)
					    SETPIXEL(op, grey);
				    if(npixels >= imagewidth)
					    break;
				    if(op_offset >= scanline) {
					    TIFFErrorExt(tif->tif_clientdata, module, "Invalid data for scanline %ld",
						    (long)tif->tif_row);
					    return 0;
				    }
				    if(cc == 0)
					    goto bad;
				    n = *bp++;
				    cc--;
			    }
			    break;
		    }
		}
	}
	tif->tif_rawcp = reinterpret_cast<uint8 *>(bp);
	tif->tif_rawcc = cc;
	return 1;
bad:
	TIFFErrorExt(tif->tif_clientdata, module, "Not enough data for scanline %ld", (long)tif->tif_row);
	return 0;
}

static int NeXTPreDecode(TIFF * tif, uint16 s)
{
	static const char module[] = __FUNCTION__;
	TIFFDirectory * td = &tif->tif_dir;
	(void)s;
	if(td->td_bitspersample != 2) {
		TIFFErrorExt(tif->tif_clientdata, module, "Unsupported BitsPerSample = %d", td->td_bitspersample);
		return 0;
	}
	return 1;
}

int TIFFInitNeXT(TIFF * tif, int scheme)
{
	(void)scheme;
	tif->tif_predecode = NeXTPreDecode;
	tif->tif_decoderow = NeXTDecode;
	tif->tif_decodestrip = NeXTDecode;
	tif->tif_decodetile = NeXTDecode;
	return 1;
}

#endif /* NEXT_SUPPORT */
