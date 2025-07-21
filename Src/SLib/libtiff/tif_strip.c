/* $Id: tif_strip.c,v 1.38 2016-12-03 11:02:15 erouault Exp $ */
/*
 * Copyright (c) 1991-1997 Sam Leffler
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
 * Strip-organized Image Support Routines.
 */
#include <slib-internal.h>
#pragma hdrstop
#include "tiffiop.h"
/*
 * Compute which strip a (row,sample) value is in.
 */
uint32 TIFFComputeStrip(TIFF * tif, uint32 row, uint16 sample)
{
	static const char module[] = __FUNCTION__;
	TIFFDirectory * td = &tif->tif_dir;
	uint32 strip = row / td->td_rowsperstrip;
	if(td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if(sample >= td->td_samplesperpixel) {
			TIFFErrorExt(tif->tif_clientdata, module, "%lu: Sample out of range, max %lu", (ulong)sample, (ulong)td->td_samplesperpixel);
			return 0;
		}
		strip += (uint32)sample*td->td_stripsperimage;
	}
	return (strip);
}

/*
 * Compute how many strips are in an image.
 */
uint32 TIFFNumberOfStrips(TIFF * tif)
{
	TIFFDirectory * td = &tif->tif_dir;
	uint32 nstrips = (td->td_rowsperstrip == (uint32) -1 ? 1 : TIFFhowmany_32(td->td_imagelength, td->td_rowsperstrip));
	if(td->td_planarconfig == PLANARCONFIG_SEPARATE)
		nstrips = _TIFFMultiply32(tif, nstrips, (uint32)td->td_samplesperpixel, "TIFFNumberOfStrips");
	return (nstrips);
}
/*
 * Compute the # bytes in a variable height, row-aligned strip.
 */
uint64 TIFFVStripSize64(TIFF * tif, uint32 nrows)
{
	static const char module[] = __FUNCTION__;
	TIFFDirectory * td = &tif->tif_dir;
	if(nrows==(uint32)(-1))
		nrows = td->td_imagelength;
	if((td->td_planarconfig==PLANARCONFIG_CONTIG)&&(td->td_photometric == PHOTOMETRIC_YCBCR)&& (!isUpSampled(tif))) {
		/*
		 * Packed YCbCr data contain one Cb+Cr for every
		 * HorizontalSampling*VerticalSampling Y values.
		 * Must also roundup width and height when calculating
		 * since images that are not a multiple of the
		 * horizontal/vertical subsampling area include
		 * YCbCr data for the extended image.
		 */
		uint16 ycbcrsubsampling[2];
		uint16 samplingblock_samples;
		uint32 samplingblocks_hor;
		uint32 samplingblocks_ver;
		uint64 samplingrow_samples;
		uint64 samplingrow_size;
		if(td->td_samplesperpixel!=3) {
			TIFFErrorExt(tif->tif_clientdata, module, "Invalid td_samplesperpixel value");
			return 0;
		}
		TIFFGetFieldDefaulted(tif, TIFFTAG_YCBCRSUBSAMPLING, ycbcrsubsampling+0, ycbcrsubsampling+1);
		if((ycbcrsubsampling[0] != 1 && ycbcrsubsampling[0] != 2 && ycbcrsubsampling[0] != 4)
		    ||(ycbcrsubsampling[1] != 1 && ycbcrsubsampling[1] != 2 && ycbcrsubsampling[1] != 4)) {
			TIFFErrorExt(tif->tif_clientdata, module, "Invalid YCbCr subsampling (%dx%d)", ycbcrsubsampling[0], ycbcrsubsampling[1]);
			return 0;
		}
		samplingblock_samples = ycbcrsubsampling[0]*ycbcrsubsampling[1]+2;
		samplingblocks_hor = TIFFhowmany_32(td->td_imagewidth, ycbcrsubsampling[0]);
		samplingblocks_ver = TIFFhowmany_32(nrows, ycbcrsubsampling[1]);
		samplingrow_samples = _TIFFMultiply64(tif, samplingblocks_hor, samplingblock_samples, module);
		samplingrow_size = TIFFhowmany8_64(_TIFFMultiply64(tif, samplingrow_samples, td->td_bitspersample, module));
		return (_TIFFMultiply64(tif, samplingrow_size, samplingblocks_ver, module));
	}
	else
		return (_TIFFMultiply64(tif, nrows, TIFFScanlineSize64(tif), module));
}

tmsize_t TIFFVStripSize(TIFF * tif, uint32 nrows)
{
	static const char module[] = __FUNCTION__;
	uint64 m = TIFFVStripSize64(tif, nrows);
	tmsize_t n = (tmsize_t)m;
	if((uint64)n!=m) {
		TIFFErrorExt(tif->tif_clientdata, module, "Integer overflow");
		n = 0;
	}
	return n;
}
/*
 * Compute the # bytes in a raw strip.
 */
uint64 TIFFRawStripSize64(TIFF * tif, uint32 strip)
{
	static const char module[] = __FUNCTION__;
	TIFFDirectory* td = &tif->tif_dir;
	uint64 bytecount = td->td_stripbytecount[strip];
	if(bytecount == 0) {
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
		TIFFErrorExt(tif->tif_clientdata, module, "%I64u: Invalid strip byte count, strip %lu", (uint64)bytecount, (ulong)strip);
#else
		TIFFErrorExt(tif->tif_clientdata, module, "%llu: Invalid strip byte count, strip %lu",
		    (uint64)bytecount, (ulong)strip);
#endif
		bytecount = (uint64) -1;
	}
	return bytecount;
}

tmsize_t TIFFRawStripSize(TIFF * tif, uint32 strip)
{
	static const char module[] = __FUNCTION__;
	tmsize_t n;
	uint64 m = TIFFRawStripSize64(tif, strip);
	if(m == _FFFF64)
		n = (tmsize_t)(-1);
	else {
		n = (tmsize_t)m;
		if((uint64)n != m) {
			TIFFErrorExt(tif->tif_clientdata, module, "Integer overflow");
			n = 0;
		}
	}
	return n;
}

/*
 * Compute the # bytes in a (row-aligned) strip.
 *
 * Note that if RowsPerStrip is larger than the
 * recorded ImageLength, then the strip size is
 * truncated to reflect the actual space required
 * to hold the strip.
 */
uint64 TIFFStripSize64(TIFF * tif)
{
	TIFFDirectory* td = &tif->tif_dir;
	uint32 rps = td->td_rowsperstrip;
	if(rps > td->td_imagelength)
		rps = td->td_imagelength;
	return (TIFFVStripSize64(tif, rps));
}

tmsize_t TIFFStripSize(TIFF * tif)
{
	static const char module[] = __FUNCTION__;
	uint64 m = TIFFStripSize64(tif);
	tmsize_t n = (tmsize_t)m;
	if((uint64)n!=m) {
		TIFFErrorExt(tif->tif_clientdata, module, "Integer overflow");
		n = 0;
	}
	return n;
}

/*
 * Compute a default strip size based on the image
 * characteristics and a requested value.  If the
 * request is <1 then we choose a strip size according
 * to certain heuristics.
 */
uint32 TIFFDefaultStripSize(TIFF * tif, uint32 request)
{
	return (*tif->tif_defstripsize)(tif, request);
}

uint32 _TIFFDefaultStripSize(TIFF * tif, uint32 s)
{
	if((int32)s < 1) {
		/*
		 * If RowsPerStrip is unspecified, try to break the
		 * image up into strips that are approximately
		 * STRIP_SIZE_DEFAULT bytes long.
		 */
		uint64 rows;
		uint64 scanlinesize = TIFFScanlineSize64(tif);
		SETIFZ(scanlinesize, 1);
		rows = (uint64)STRIP_SIZE_DEFAULT/scanlinesize;
		if(rows==0)
			rows = 1;
		else if(rows>0xFFFFFFFF)
			rows = 0xFFFFFFFF;
		s = (uint32)rows;
	}
	return (s);
}

/*
 * Return the number of bytes to read/write in a call to
 * one of the scanline-oriented i/o routines.  Note that
 * this number may be 1/samples-per-pixel if data is
 * stored as separate planes.
 * The ScanlineSize in case of YCbCrSubsampling is defined as the
 * strip size divided by the strip height, i.e. the size of a pack of vertical
 * subsampling lines divided by vertical subsampling. It should thus make
 * sense when multiplied by a multiple of vertical subsampling.
 */
uint64 TIFFScanlineSize64(TIFF * tif)
{
	static const char module[] = __FUNCTION__;
	TIFFDirectory * td = &tif->tif_dir;
	uint64 scanline_size;
	if(td->td_planarconfig==PLANARCONFIG_CONTIG) {
		if((td->td_photometric==PHOTOMETRIC_YCBCR) && (td->td_samplesperpixel==3) && (!isUpSampled(tif))) {
			uint16 ycbcrsubsampling[2];
			uint16 samplingblock_samples;
			uint32 samplingblocks_hor;
			uint64 samplingrow_samples;
			uint64 samplingrow_size;
			if(td->td_samplesperpixel!=3) {
				TIFFErrorExt(tif->tif_clientdata, module, "Invalid td_samplesperpixel value");
				return 0;
			}
			TIFFGetFieldDefaulted(tif, TIFFTAG_YCBCRSUBSAMPLING, ycbcrsubsampling+0, ycbcrsubsampling+1);
			if(((ycbcrsubsampling[0]!=1)&&(ycbcrsubsampling[0]!=2)&&(ycbcrsubsampling[0]!=4)) ||
			    ((ycbcrsubsampling[1]!=1)&&(ycbcrsubsampling[1]!=2)&&(ycbcrsubsampling[1]!=4))) {
				TIFFErrorExt(tif->tif_clientdata, module, "Invalid YCbCr subsampling");
				return 0;
			}
			samplingblock_samples = ycbcrsubsampling[0]*ycbcrsubsampling[1]+2;
			samplingblocks_hor = TIFFhowmany_32(td->td_imagewidth, ycbcrsubsampling[0]);
			samplingrow_samples = _TIFFMultiply64(tif, samplingblocks_hor, samplingblock_samples, module);
			samplingrow_size = TIFFhowmany_64(_TIFFMultiply64(tif, samplingrow_samples, td->td_bitspersample, module), 8);
			scanline_size = (samplingrow_size/ycbcrsubsampling[1]);
		}
		else {
			uint64 scanline_samples;
			scanline_samples = _TIFFMultiply64(tif, td->td_imagewidth, td->td_samplesperpixel, module);
			scanline_size = TIFFhowmany_64(_TIFFMultiply64(tif, scanline_samples, td->td_bitspersample, module), 8);
		}
	}
	else {
		scanline_size = TIFFhowmany_64(_TIFFMultiply64(tif, td->td_imagewidth, td->td_bitspersample, module), 8);
	}
	if(scanline_size == 0) {
		TIFFErrorExt(tif->tif_clientdata, module, "Computed scanline size is zero");
		return 0;
	}
	return (scanline_size);
}

tmsize_t TIFFScanlineSize(TIFF * tif)
{
	static const char module[] = __FUNCTION__;
	uint64 m = TIFFScanlineSize64(tif);
	tmsize_t n = (tmsize_t)m;
	if((uint64)n!=m) {
		TIFFErrorExt(tif->tif_clientdata, module, "Integer arithmetic overflow");
		n = 0;
	}
	return n;
}

/*
 * Return the number of bytes required to store a complete
 * decoded and packed raster scanline (as opposed to the
 * I/O size returned by TIFFScanlineSize which may be less
 * if data is store as separate planes).
 */
uint64 TIFFRasterScanlineSize64(TIFF * tif)
{
	static const char module[] = __FUNCTION__;
	TIFFDirectory * td = &tif->tif_dir;
	uint64 scanline = _TIFFMultiply64(tif, td->td_bitspersample, td->td_imagewidth, module);
	if(td->td_planarconfig == PLANARCONFIG_CONTIG) {
		scanline = _TIFFMultiply64(tif, scanline, td->td_samplesperpixel, module);
		return (TIFFhowmany8_64(scanline));
	}
	else
		return (_TIFFMultiply64(tif, TIFFhowmany8_64(scanline), td->td_samplesperpixel, module));
}

tmsize_t TIFFRasterScanlineSize(TIFF * tif)
{
	static const char module[] = __FUNCTION__;
	uint64 m = TIFFRasterScanlineSize64(tif);
	tmsize_t n = (tmsize_t)m;
	if((uint64)n!=m) {
		TIFFErrorExt(tif->tif_clientdata, module, "Integer arithmetic overflow");
		n = 0;
	}
	return n;
}
