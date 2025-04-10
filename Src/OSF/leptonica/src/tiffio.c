// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file tiffio.c
 * <pre>
 *
 *     TIFFClientOpen() wrappers for FILE*:
 *      static tsize_t    lept_read_proc()
 *      static tsize_t    lept_write_proc()
 *      static toff_t     lept_seek_proc()
 *      static int        lept_close_proc()
 *      static toff_t     lept_size_proc()
 *
 *     Reading tiff:
 *             PIX *pixReadTiff()             [ special top level ]
 *             PIX *pixReadStreamTiff()
 *      static PIX *pixReadFromTiffStream()
 *
 *     Writing tiff:
 *             int32    pixWriteTiff()            [ special top level ]
 *             int32    pixWriteTiffCustom()      [ special top level ]
 *             int32    pixWriteStreamTiff()
 *             int32    pixWriteStreamTiffWA()
 *      static int32    pixWriteToTiffStream()
 *      static int32    writeCustomTiffTags()
 *
 *     Reading and writing multipage tiff
 *             PIX *pixReadFromMultipageTiff()
 *             PIXA      *pixaReadMultipageTiff()   [ special top level ]
 *             int32    pixaWriteMultipageTiff()  [ special top level ]
 *             int32    writeMultipageTiff()      [ special top level ]
 *             int32    writeMultipageTiffSA()
 *
 *     Information about tiff file
 *             int32    fprintTiffInfo()
 *             int32    tiffGetCount()
 *             int32    getTiffResolution()
 *      static int32    getTiffStreamResolution()
 *             int32    readHeaderTiff()
 *             int32    freadHeaderTiff()
 *             int32    readHeaderMemTiff()
 *      static int32    tiffReadHeaderTiff()
 *             int32    findTiffCompression()
 *      static int32    getTiffCompressedFormat()
 *
 *     Extraction of tiff g4 data:
 *             int32    extractG4DataFromFile()
 *
 *     Open tiff stream from file stream
 *      static TIFF      *fopenTiff()
 *
 *     Wrapper for TIFFOpen:
 *      static TIFF      *openTiff()
 *
 *     Memory I/O: reading memory --> pix and writing pix --> memory
 *             [10 static helper functions]
 *             PIX *pixReadMemTiff();
 *             PIX *pixReadMemFromMultipageTiff();
 *             PIXA      *pixaReadMemMultipageTiff()    [ special top level ]
 *             int32    pixaWriteMemMultipageTiff()   [ special top level ]
 *             int32    pixWriteMemTiff();
 *             int32    pixWriteMemTiffCustom();
 *
 *  Note 1: To include all necessary functions, use libtiff version 3.7.4
 *          (from 2005) or later.
 *  Note 2: What compression methods in tiff are supported?
 *          * We support most methods that are fully implemented in the
 *            tiff library, such as G3, G4, RLE and LZW.
 *          * The exception is the old-style jpeg tiff format (OJPEG), which
 *            is not supported.
 *          * We support only one format, ZIP, that uses an external library.
 *          * At present we do not support WEBP in tiff, which uses
 *            libwebp and was added in tifflib 4.1.0 in 2019.
 *  Note 3: On Windows with 2 bpp or 4 bpp images, the bytes in the
 *          tiff-compressed file depend on the pad bits, but not on the
 *          decoded raster image when read.  Because it is sometimes
 *          convenient to use a golden file with a byte-by-byte check
 *          to verify invariance, we set the pad bits to 0 before writing,
 *          in pixWriteToTiffStream().
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop
#ifndef _MSC_VER
#include <unistd.h>
#else  /* _MSC_VER */
#include <io.h>
#endif  /* _MSC_VER */
#include <fcntl.h>

/* --------------------------------------------*/
#if  HAVE_LIBTIFF   /* defined in environ.h */
/* --------------------------------------------*/

#include "..\slib\libtiff\tiff.h"
#include "..\slib\libtiff\tiffio.h"

static const int32 DefaultResolution = 300;  /* ppi */
static const int32 ManyPagesInTiffFile = 3000;   /* warn if big */

/* Verified that tiflib makes valid g4 files of this size */
static const int32 MaxTiffWidth = 1 << 20;   /* 1M pixels */
static const int32 MaxTiffHeight = 1 << 20;   /* 1M pixels */

/* Check g4 data size */
static const size_t MaxNumTiffBytes = (1 << 28) - 1;   /* 256 MB */

/* All functions with TIFF interfaces are static. */
static PIX * pixReadFromTiffStream(TIFF * tif);
static int32   getTiffStreamResolution(TIFF * tif, int32 * pxres, int32 * pyres);
static int32   tiffReadHeaderTiff(TIFF * tif, int32 * pwidth, int32 * pheight, int32 * pbps, int32 * pspp, int32 * pres, int32 * pcmap, int32 * pformat);
static int32   writeCustomTiffTags(TIFF * tif, NUMA * natags, SARRAY * savals, SARRAY * satypes, NUMA * nasizes);
static int32   pixWriteToTiffStream(TIFF * tif, PIX * pix, int32 comptype, NUMA * natags, SARRAY * savals, SARRAY * satypes, NUMA * nasizes);
static TIFF     * fopenTiff(FILE * fp, const char * modestring);
static TIFF     * openTiff(const char * filename, const char * modestring);

/* Static helper for tiff compression type */
static int32   getTiffCompressedFormat(uint16 tiffcomp);

/* Static function for memory I/O */
static TIFF     * fopenTiffMemstream(const char * filename, const char * operation,
    uint8 ** pdata, size_t * pdatasize);

/* This structure defines a transform to be performed on a TIFF image
 * (note that the same transformation can be represented in
 * several different ways using this structure since
 * vflip + hflip + counterclockwise == clockwise). */
struct tiff_transform {
	int vflip; /* if non-zero, image needs a vertical fip */
	int hflip; /* if non-zero, image needs a horizontal flip */
	int rotate; /* -1 -> counterclockwise 90-degree rotation,
	                  0 -> no rotation
	                  1 -> clockwise 90-degree rotation */
};

/* This describes the transformations needed for a given orientation
 * tag.  The tag values start at 1, so you need to subtract 1 to get a
 * valid index into this array.  It is only valid when not using
 * TIFFReadRGBAImageOriented(). */
static struct tiff_transform tiff_orientation_transforms[] = {
	{0, 0, 0},
	{0, 1, 0},
	{1, 1, 0},
	{1, 0, 0},
	{0, 1, -1},
	{0, 0, 1},
	{0, 1, 1},
	{0, 0, -1}
};

/* Same as above, except that test transformations are only valid
 * when using TIFFReadRGBAImageOriented().  Transformations
 * were determined empirically.  See the libtiff mailing list for
 * more discussion: http://www.asmail.be/msg0054683875.html  */
static struct tiff_transform tiff_partial_orientation_transforms[] = {
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{0, 1, -1},
	{0, 1, 1},
	{1, 0, 1},
	{0, 1, -1}
};

/*-----------------------------------------------------------------------*
*             TIFFClientOpen() wrappers for FILE*                       *
*             Provided by Jürgen Buchmüller                             *
*                                                                       *
*  We previously used TIFFFdOpen(), which used low-level file           *
*  descriptors.  It had portability issues with Windows, along          *
*  with other limitations from lack of stream control operations.       *
*  These callbacks to TIFFClientOpen() avoid the problems.              *
*                                                                       *
*  Jürgen made the functions use 64 bit file operations where possible  *
*  or required, namely for seek and size. On Windows there are specific *
*  _fseeki64() and _ftelli64() functions.  On unix it is common to look *
*  for a macro _LARGEFILE64_SOURCE being defined, which makes available *
*  the off64_t type, and to use fseeko() and ftello() in this case.     *
*-----------------------------------------------------------------------*/
static tsize_t lept_read_proc(thandle_t cookie,
    tdata_t buff,
    tsize_t size)
{
	FILE* fp = (FILE*)cookie;
	tsize_t done;
	if(!buff || !cookie || !fp)
		return (tsize_t)-1;
	done = fread(buff, 1, size, fp);
	return done;
}

static tsize_t lept_write_proc(thandle_t cookie,
    tdata_t buff,
    tsize_t size)
{
	FILE* fp = (FILE*)cookie;
	tsize_t done;
	if(!buff || !cookie || !fp)
		return (tsize_t)-1;
	done = fwrite(buff, 1, size, fp);
	return done;
}

static toff_t lept_seek_proc(thandle_t cookie,
    toff_t offs,
    int whence)
{
	FILE* fp = (FILE*)cookie;
#if defined(_MSC_VER)
	__int64 pos = 0;
	if(!cookie || !fp)
		return (tsize_t)-1;
	switch(whence) {
		case SEEK_SET:
		    pos = 0;
		    break;
		case SEEK_CUR:
		    pos = ftell(fp);
		    break;
		case SEEK_END:
		    _fseeki64(fp, 0, SEEK_END);
		    pos = _ftelli64(fp);
		    break;
	}
	pos = (__int64)(pos + offs);
	_fseeki64(fp, pos, SEEK_SET);
	if(pos == _ftelli64(fp))
		return (tsize_t)pos;
#elif defined(_LARGEFILE64_SOURCE)
	off64_t pos = 0;
	if(!cookie || !fp)
		return (tsize_t)-1;
	switch(whence) {
		case SEEK_SET:
		    pos = 0;
		    break;
		case SEEK_CUR:
		    pos = ftello(fp);
		    break;
		case SEEK_END:
		    fseeko(fp, 0, SEEK_END);
		    pos = ftello(fp);
		    break;
	}
	pos = (off64_t)(pos + offs);
	fseeko(fp, pos, SEEK_SET);
	if(pos == ftello(fp))
		return (tsize_t)pos;
#else
	off_t pos = 0;
	if(!cookie || !fp)
		return (tsize_t)-1;
	switch(whence) {
		case SEEK_SET:
		    pos = 0;
		    break;
		case SEEK_CUR:
		    pos = ftell(fp);
		    break;
		case SEEK_END:
		    fseek(fp, 0, SEEK_END);
		    pos = ftell(fp);
		    break;
	}
	pos = (off_t)(pos + offs);
	fseek(fp, pos, SEEK_SET);
	if(pos == ftell(fp))
		return (tsize_t)pos;
#endif
	return (tsize_t)-1;
}

static int lept_close_proc(thandle_t cookie)
{
	FILE* fp = (FILE*)cookie;
	if(!cookie || !fp)
		return 0;
	fseek(fp, 0, SEEK_SET);
	return 0;
}

static toff_t lept_size_proc(thandle_t cookie)
{
	FILE* fp = (FILE*)cookie;
#if defined(_MSC_VER)
	__int64 pos;
	__int64 size;
	if(!cookie || !fp)
		return (tsize_t)-1;
	pos = _ftelli64(fp);
	_fseeki64(fp, 0, SEEK_END);
	size = _ftelli64(fp);
	_fseeki64(fp, pos, SEEK_SET);
#elif defined(_LARGEFILE64_SOURCE)
	off64_t pos;
	off64_t size;
	if(!fp)
		return (tsize_t)-1;
	pos = ftello(fp);
	fseeko(fp, 0, SEEK_END);
	size = ftello(fp);
	fseeko(fp, pos, SEEK_SET);
#else
	off_t pos;
	off_t size;
	if(!cookie || !fp)
		return (tsize_t)-1;
	pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, pos, SEEK_SET);
#endif
	return (toff_t)size;
}

/*--------------------------------------------------------------*
*                      Reading from file                       *
*--------------------------------------------------------------*/
/*!
 * \brief   pixReadTiff()
 *
 * \param[in]    filename
 * \param[in]    n           page number 0 based
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a version of pixRead(), specialized for tiff
 *          files, that allows specification of the page to be returned
 *      (2) No warning messages on failure, because of how multi-page
 *          TIFF reading works. You are supposed to keep trying until
 *          it stops working.
 * </pre>
 */
PIX * pixReadTiff(const char * filename, int32 n)
{
	FILE * fp;
	PIX * pix;
	PROCNAME(__FUNCTION__);
	if(!filename)
		return (PIX *)ERROR_PTR("filename not defined", procName, NULL);

	if((fp = fopenReadStream(filename)) == NULL)
		return (PIX *)ERROR_PTR("image file not found", procName, NULL);
	pix = pixReadStreamTiff(fp, n);
	fclose(fp);
	return pix;
}

/*--------------------------------------------------------------*
*                     Reading from stream                      *
*--------------------------------------------------------------*/
/*!
 * \brief   pixReadStreamTiff()
 *
 * \param[in]    fp    file stream
 * \param[in]    n     page number: 0 based
 * \return  pix, or NULL on error or if there are no more images in the file
 *
 * <pre>
 * Notes:
 *      (1) No warning messages on failure, because of how multi-page
 *          TIFF reading works. You are supposed to keep trying until
 *          it stops working.
 * </pre>
 */
PIX * pixReadStreamTiff(FILE * fp, int32 n)
{
	PIX * pix;
	TIFF  * tif;
	PROCNAME(__FUNCTION__);
	if(!fp)
		return (PIX *)ERROR_PTR("stream not defined", procName, NULL);
	if((tif = fopenTiff(fp, "r")) == NULL)
		return (PIX *)ERROR_PTR("tif not opened", procName, NULL);
	if(TIFFSetDirectory(tif, n) == 0) {
		TIFFCleanup(tif);
		return NULL;
	}
	if((pix = pixReadFromTiffStream(tif)) == NULL) {
		TIFFCleanup(tif);
		return NULL;
	}
	TIFFCleanup(tif);
	return pix;
}

/*!
 * \brief   pixReadFromTiffStream()
 *
 * \param[in]    tif    TIFF handle
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) We can read the following images (up to 32 bits/pixel):
 *          1 spp (grayscale): 1, 2, 4, 8, 16 bps
 *          1 spp (colormapped): 1, 2, 4, 8 bps
 *          2 spp (gray+alpha): 8 bps
 *          3 spp (rgb) and 4 spp (rgba): 8 or 16 bps
 *      (2) We do not handle 16 bps for spp == 2.
 *      (3) We do not support tiled format or webp encoded tiff.
 *      (4) 2 bpp gray+alpha are rasterized as 32 bit/pixel rgba, with
 *          the gray value replicated in r, g and b.
 *      (5) For colormapped images, we support 8 bits/color in the palette.
 *          Tiff colormaps have 16 bits/color, and we reduce them to 8.
 *      (6) Quoting the libtiff documentation at
 *               http://libtiff.maptools.org/libtiff.html
 *          "libtiff provides a high-level interface for reading image data
 *          from a TIFF file. This interface handles the details of data
 *          organization and format for a wide variety of TIFF files;
 *          at least the large majority of those files that one would
 *          normally encounter. Image data is, by default, returned as
 *          ABGR pixels packed into 32-bit words (8 bits per sample).
 *          Rectangular rasters can be read or data can be intercepted
 *          at an intermediate level and packed into memory in a format
 *          more suitable to the application. The library handles all
 *          the details of the format of data stored on disk and,
 *          in most cases, if any colorspace conversions are required:
 *          bilevel to RGB, greyscale to RGB, CMYK to RGB, YCbCr to RGB,
 *          16-bit samples to 8-bit samples, associated/unassociated alpha,
 *          etc."
 * </pre>
 */
static PIX * pixReadFromTiffStream(TIFF  * tif)
{
	char      * text;
	uint8   * linebuf, * data, * rowptr;
	uint16 spp, bps, photometry, tiffcomp, orientation, sample_fmt;
	uint16  * redmap, * greenmap, * bluemap;
	int32 d, wpl, bpl, comptype, i, j, k, ncolors, rval, gval, bval, aval;
	int32 xres, yres, tiffbpl, packedbpl, half_size, twothirds_size;
	uint32 w, h, tiffword, read_oriented;
	uint32  * line, * ppixel, * tiffdata, * pixdata;
	PIX * pix, * pix1;
	PIXCMAP   * cmap;

	PROCNAME(__FUNCTION__);

	if(!tif)
		return (PIX *)ERROR_PTR("tif not defined", procName, NULL);

	read_oriented = 0;

	/* Only accept uint image data:
	*   SAMPLEFORMAT_UINT = 1;
	*   SAMPLEFORMAT_INT = 2;
	*   SAMPLEFORMAT_IEEEFP = 3;
	*   SAMPLEFORMAT_VOID = 4;   */
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &sample_fmt);
	if(sample_fmt != SAMPLEFORMAT_UINT) {
		L_ERROR("sample format = %d is not uint\n", procName, sample_fmt);
		return NULL;
	}

	/* Can't read tiff in tiled format. For what is involved, see, e.g:
	 *   https://www.cs.rochester.edu/~nelson/courses/vision/\
	 *     resources/tiff/libtiff.html#Tiles
	 * A tiled tiff can be converted to a normal (strip) tif:
	 *   tiffcp -s <input-tiled-tif> <output-strip-tif>    */
	if(TIFFIsTiled(tif)) {
		L_ERROR("tiled format is not supported\n", procName);
		return NULL;
	}

	/* Old style jpeg is not supported.  We tried supporting 8 bpp.
	 * TIFFReadScanline() fails on this format, so we used RGBA
	 * reading, which generates a 4 spp image, and pulled out the
	 * red component.  However, there were problems with double-frees
	 * in cleanup.  For RGB, tiffbpl is exactly half the size that
	 * you would expect for the raster data in a scanline, which
	 * is 3 * w.  */
	TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &tiffcomp);
	if(tiffcomp == COMPRESSION_OJPEG) {
		L_ERROR("old style jpeg format is not supported\n", procName);
		return NULL;
	}

	/* webp in tiff is in 4.1.0 and not yet supported in Adobe registry */
#if defined(COMPRESSION_WEBP)
	if(tiffcomp == COMPRESSION_WEBP) {
		L_ERROR("webp in tiff not generally supported yet\n", procName);
		return NULL;
	}
#endif  /* COMPRESSION_WEBP */

	/* Use default fields for bps and spp */
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bps);
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	if(bps != 1 && bps != 2 && bps != 4 && bps != 8 && bps != 16) {
		L_ERROR("invalid bps = %d\n", procName, bps);
		return NULL;
	}
	if(spp == 2 && bps != 8) {
		L_WARNING("for 2 spp, only handle 8 bps\n", procName);
		return NULL;
	}
	if(spp == 1) {
		d = bps;
	}
	else if(spp == 2) { /* gray plus alpha */
		d = 32; /* will convert to RGBA */
	}
	else if(spp == 3 || spp == 4) {
		d = 32;
	}
	else {
		L_ERROR("spp = %d; not in {1,2,3,4}\n", procName, spp);
		return NULL;
	}

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	if(w > MaxTiffWidth) {
		L_ERROR("width = %d pixels; too large\n", procName, w);
		return NULL;
	}
	if(h > MaxTiffHeight) {
		L_ERROR("height = %d pixels; too large\n", procName, h);
		return NULL;
	}

	/* The relation between the size of a byte buffer required to hold
	   a raster of image pixels (packedbpl) and the size of the tiff
	   buffer (tiffbuf) is either 1:1 or approximately 1.5:1 or 2:1,
	   depending on how the data is stored and subsampled.  For security,
	   we test this relation between tiffbuf and the image parameters
	   w, spp and bps. */
	tiffbpl = TIFFScanlineSize(tif);
	packedbpl = (bps * spp * w + 7) / 8;
	half_size = (L_ABS(2 * tiffbpl - packedbpl) <= 8);
	twothirds_size = (L_ABS(3 * tiffbpl - 2 * packedbpl) <= 8);
#if 0
	if(half_size)
		L_INFO("half_size: packedbpl = %d is approx. twice tiffbpl = %d\n",
		    procName, packedbpl, tiffbpl);
	if(twothirds_size)
		L_INFO("twothirds_size: packedbpl = %d is approx. 1.5 tiffbpl = %d\n",
		    procName, packedbpl, tiffbpl);
	lept_stderr("tiffbpl = %d, packedbpl = %d, bps = %d, spp = %d, w = %d\n",
	    tiffbpl, packedbpl, bps, spp, w);
#endif
	if(tiffbpl != packedbpl && !half_size && !twothirds_size) {
		L_ERROR("invalid tiffbpl: tiffbpl = %d, packedbpl = %d, "
		    "bps = %d, spp = %d, w = %d\n",
		    procName, tiffbpl, packedbpl, bps, spp, w);
		return NULL;
	}

	/* Use a linebuf that will hold all the pixels generated
	   by tiff when reading (decompressing) a scanline. */
	if((pix = pixCreate(w, h, d)) == NULL)
		return (PIX *)ERROR_PTR("pix not made", procName, NULL);
	pixSetInputFormat(pix, IFF_TIFF);
	data = (uint8 *)pixGetData(pix);
	wpl = pixGetWpl(pix);
	bpl = 4 * wpl;
	if(spp == 1) {
		linebuf = (uint8 *)SAlloc::C(4 * wpl, sizeof(uint8));
		for(i = 0; i < h; i++) {
			if(TIFFReadScanline(tif, linebuf, i, 0) < 0) {
				SAlloc::F(linebuf);
				pixDestroy(&pix);
				L_ERROR("spp = 1, read fail at line %d\n", procName, i);
				return NULL;
			}
			memcpy(data, linebuf, tiffbpl);
			data += bpl;
		}
		if(bps <= 8)
			pixEndianByteSwap(pix);
		else /* bps == 16 */
			pixEndianTwoByteSwap(pix);
		SAlloc::F(linebuf);
	}
	else if(spp == 2 && bps == 8) { /* gray plus alpha */
		L_INFO("gray+alpha is not supported; converting to RGBA\n", procName);
		pixSetSpp(pix, 4);
		linebuf = (uint8 *)SAlloc::C(4 * wpl, sizeof(uint8));
		pixdata = pixGetData(pix);
		for(i = 0; i < h; i++) {
			if(TIFFReadScanline(tif, linebuf, i, 0) < 0) {
				SAlloc::F(linebuf);
				pixDestroy(&pix);
				L_ERROR("spp = 2, read fail at line %d\n", procName, i);
				return NULL;
			}
			rowptr = linebuf;
			ppixel = pixdata + i * wpl;
			for(j = k = 0; j < w; j++) {
				/* Copy gray value into r, g and b */
				SET_DATA_BYTE(ppixel, COLOR_RED, rowptr[k]);
				SET_DATA_BYTE(ppixel, COLOR_GREEN, rowptr[k]);
				SET_DATA_BYTE(ppixel, COLOR_BLUE, rowptr[k++]);
				SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL, rowptr[k++]);
				ppixel++;
			}
		}
		SAlloc::F(linebuf);
	}
	else { /* rgb and rgba */
		if((tiffdata = (uint32*)SAlloc::C((size_t)w * h,
		    sizeof(uint32))) == NULL) {
			pixDestroy(&pix);
			return (PIX *)ERROR_PTR("calloc fail for tiffdata", procName, NULL);
		}
		/* TIFFReadRGBAImageOriented() converts to 8 bps */
		if(!TIFFReadRGBAImageOriented(tif, w, h, reinterpret_cast<uint32 *>(tiffdata), ORIENTATION_TOPLEFT, 0)) {
			SAlloc::F(tiffdata);
			pixDestroy(&pix);
			return (PIX *)ERROR_PTR("failed to read tiffdata", procName, NULL);
		}
		else {
			read_oriented = 1;
		}

		if(spp == 4) pixSetSpp(pix, 4);
		line = pixGetData(pix);
		for(i = 0; i < h; i++, line += wpl) {
			for(j = 0, ppixel = line; j < w; j++) {
				/* TIFFGet* are macros */
				tiffword = tiffdata[i * w + j];
				rval = TIFFGetR(tiffword);
				gval = TIFFGetG(tiffword);
				bval = TIFFGetB(tiffword);
				if(spp == 3) {
					composeRGBPixel(rval, gval, bval, ppixel);
				}
				else { /* spp == 4 */
					aval = TIFFGetA(tiffword);
					composeRGBAPixel(rval, gval, bval, aval, ppixel);
				}
				ppixel++;
			}
		}
		SAlloc::F(tiffdata);
	}

	if(getTiffStreamResolution(tif, &xres, &yres) == 0) {
		pixSetXRes(pix, xres);
		pixSetYRes(pix, yres);
	}

	/* Find and save the compression type */
	comptype = getTiffCompressedFormat(tiffcomp);
	pixSetInputFormat(pix, comptype);

	if(TIFFGetField(tif, TIFFTAG_COLORMAP, &redmap, &greenmap, &bluemap)) {
		/* Save the colormap as a pix cmap.  Because the
		 * tiff colormap components are 16 bit unsigned,
		 * and go from black (0) to white (0xffff), the
		 * the pix cmap takes the most significant byte. */
		if(bps > 8) {
			pixDestroy(&pix);
			return (PIX *)ERROR_PTR("colormap size > 256", procName, NULL);
		}
		if((cmap = pixcmapCreate(bps)) == NULL) {
			pixDestroy(&pix);
			return (PIX *)ERROR_PTR("colormap not made", procName, NULL);
		}
		ncolors = 1 << bps;
		for(i = 0; i < ncolors; i++)
			pixcmapAddColor(cmap, redmap[i] >> 8, greenmap[i] >> 8,
			    bluemap[i] >> 8);
		if(pixSetColormap(pix, cmap)) {
			pixDestroy(&pix);
			return (PIX *)ERROR_PTR("invalid colormap", procName, NULL);
		}

		/* Remove the colormap for 1 bpp. */
		if(bps == 1) {
			pix1 = pixRemoveColormap(pix, REMOVE_CMAP_BASED_ON_SRC);
			pixDestroy(&pix);
			pix = pix1;
		}
	}
	else { /* No colormap: check photometry and invert if necessary */
		if(!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometry)) {
			/* Guess default photometry setting.  Assume min_is_white
			 * if compressed 1 bpp; min_is_black otherwise. */
			if(tiffcomp == COMPRESSION_CCITTFAX3 ||
			    tiffcomp == COMPRESSION_CCITTFAX4 ||
			    tiffcomp == COMPRESSION_CCITTRLE ||
			    tiffcomp == COMPRESSION_CCITTRLEW) {
				photometry = PHOTOMETRIC_MINISWHITE;
			}
			else {
				photometry = PHOTOMETRIC_MINISBLACK;
			}
		}
		if((d == 1 && photometry == PHOTOMETRIC_MINISBLACK) ||
		    (d == 8 && photometry == PHOTOMETRIC_MINISWHITE))
			pixInvert(pix, pix);
	}

	if(TIFFGetField(tif, TIFFTAG_ORIENTATION, &orientation)) {
		if(orientation >= 1 && orientation <= 8) {
			struct tiff_transform * transform = (read_oriented) ?
			    &tiff_partial_orientation_transforms[orientation - 1] :
			    &tiff_orientation_transforms[orientation - 1];
			if(transform->vflip) pixFlipTB(pix, pix);
			if(transform->hflip) pixFlipLR(pix, pix);
			if(transform->rotate) {
				PIX * oldpix = pix;
				pix = pixRotate90(oldpix, transform->rotate);
				pixDestroy(&oldpix);
			}
		}
	}

	text = NULL;
	TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &text);
	if(text) pixSetText(pix, text);
	return pix;
}

/*--------------------------------------------------------------*
*                       Writing to file                        *
*--------------------------------------------------------------*/
/*!
 * \brief   pixWriteTiff()
 *
 * \param[in]    filename   to write to
 * \param[in]    pix        any depth, colormap will be removed
 * \param[in]    comptype   IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                          IFF_TIFF_G3, IFF_TIFF_G4,
 *                          IFF_TIFF_LZW, IFF_TIFF_ZIP, IFF_TIFF_JPEG
 * \param[in]    modestr    "a" or "w"
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) For multipage tiff, write the first pix with mode "w" and
 *          all subsequent pix with mode "a".
 *      (2) For multipage tiff, there is considerable overhead in the
 *          machinery to append an image and add the directory entry,
 *          and the time required for each image increases linearly
 *          with the number of images in the file.
 * </pre>
 */
l_ok pixWriteTiff(const char * filename,
    PIX         * pix,
    int32 comptype,
    const char * modestr)
{
	return pixWriteTiffCustom(filename, pix, comptype, modestr,
		   NULL, NULL, NULL, NULL);
}

/*!
 * \brief   pixWriteTiffCustom()
 *
 * \param[in]    filename   to write to
 * \param[in]    pix
 * \param[in]    comptype   IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                          IFF_TIFF_G3, IFF_TIFF_G4,
 *                          IFF_TIFF_LZW, IFF_TIFF_ZIP, IFF_TIFF_JPEG
 * \param[in]    modestr    "a" or "w"
 * \param[in]    natags [optional] NUMA of custom tiff tags
 * \param[in]    savals [optional] SARRAY of values
 * \param[in]    satypes [optional] SARRAY of types
 * \param[in]    nasizes [optional] NUMA of sizes
 * \return  0 if OK, 1 on error
 *
 *  Usage:
 *      1 This writes a page image to a tiff file, with optional
 *          extra tags defined in tiff.h
 *      2 For multipage tiff, write the first pix with mode "w" and
 *          all subsequent pix with mode "a".
 *      3 For the custom tiff tags:
 *          a The three arrays {natags, savals, satypes} must all be
 *              either NULL or defined and of equal size.
 *          b If they are defined, the tags are an array of integers,
 *              the vals are an array of values in string format, and
 *              the types are an array of types in string format.
 *          c All valid tags are definined in tiff.h.
 *          d The types allowed are the set of strings:
 *                "char*"
 *                "uint8*"
 *                "uint16"
 *                "uint32"
 *                "int32"
 *                "double"
 *                "uint16-uint16" note the dash; use it between the
 *                                    two uint16 vals in the val string
 *              Of these, "char*" and "uint16" are the most commonly used.
 *          e The last array, nasizes, is also optional.  It is for
 *              tags that take an array of bytes for a value, a number of
 *              elements in the array, and a type that is either "char*"
 *              or "uint8*" probably either will work.
 *              Use NULL if there are no such tags.
 *          f VERY IMPORTANT: if there are any tags that require the
 *              extra size value, stored in nasizes, they must be
 *              written first!
 */
l_ok pixWriteTiffCustom(const char * filename,
    PIX         * pix,
    int32 comptype,
    const char * modestr,
    NUMA        * natags,
    SARRAY * savals,
    SARRAY * satypes,
    NUMA        * nasizes)
{
	int32 ret;
	TIFF    * tif;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return ERROR_INT("filename not defined", procName, 1);
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);

	if((tif = openTiff(filename, modestr)) == NULL)
		return ERROR_INT("tif not opened", procName, 1);
	ret = pixWriteToTiffStream(tif, pix, comptype, natags, savals,
		satypes, nasizes);
	TIFFClose(tif);
	return ret;
}

/*--------------------------------------------------------------*
*                       Writing to stream                      *
*--------------------------------------------------------------*/
/*!
 * \brief   pixWriteStreamTiff()
 *
 * \param[in]    fp       file stream
 * \param[in]    pix
 * \param[in]    comptype IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4,
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP, IFF_TIFF_JPEG
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This writes a single image to a file stream opened for writing.
 *      (2) If the pix has a colormap, it is preserved in the output file.
 *      (3) For images with bpp > 1, this resets the comptype, if
 *          necessary, to write uncompressed data.
 *      (4) G3 and G4 are only defined for 1 bpp.
 *      (5) We only allow PACKBITS for bpp = 1, because for bpp > 1
 *          it typically expands images that are not synthetically generated.
 *      (6) G4 compression is typically about twice as good as G3.
 *          G4 is excellent for binary compression of text/line-art,
 *          but terrible for halftones and dithered patterns.  (In
 *          fact, G4 on halftones can give a file that is larger
 *          than uncompressed!)  If a binary image has dithered
 *          regions, it is usually better to compress with png.
 * </pre>
 */
l_ok pixWriteStreamTiff(FILE * fp,
    PIX     * pix,
    int32 comptype)
{
	return pixWriteStreamTiffWA(fp, pix, comptype, "w");
}

/*!
 * \brief   pixWriteStreamTiffWA()
 *
 * \param[in]    fp       file stream opened for append or write
 * \param[in]    pix
 * \param[in]    comptype IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                        IFF_TIFF_G3, IFF_TIFF_G4,
 *                        IFF_TIFF_LZW, IFF_TIFF_ZIP, IFF_TIFF_JPEG
 * \param[in]    modestr  "w" or "a"
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See pixWriteStreamTiff()
 * </pre>
 */
l_ok pixWriteStreamTiffWA(FILE        * fp,
    PIX         * pix,
    int32 comptype,
    const char * modestr)
{
	TIFF  * tif;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);
	if(strcmp(modestr, "w") && strcmp(modestr, "a")) {
		L_ERROR("modestr = %s; not 'w' or 'a'\n", procName, modestr);
		return 1;
	}

	if(pixGetDepth(pix) != 1 && comptype != IFF_TIFF &&
	    comptype != IFF_TIFF_LZW && comptype != IFF_TIFF_ZIP &&
	    comptype != IFF_TIFF_JPEG) {
		L_WARNING("invalid compression type %d for bpp > 1; using TIFF_ZIP\n", procName, comptype);
		comptype = IFF_TIFF_ZIP;
	}

	if((tif = fopenTiff(fp, modestr)) == NULL)
		return ERROR_INT("tif not opened", procName, 1);

	if(pixWriteToTiffStream(tif, pix, comptype, NULL, NULL, NULL, NULL)) {
		TIFFCleanup(tif);
		return ERROR_INT("tif write error", procName, 1);
	}

	TIFFCleanup(tif);
	return 0;
}

/*!
 * \brief   pixWriteToTiffStream()
 *
 * \param[in]    tif       data structure, opened to a file
 * \param[in]    pix
 * \param[in]    comptype  IFF_TIFF: for any image; no compression
 *                         IFF_TIFF_RLE, IFF_TIFF_PACKBITS: for 1 bpp only
 *                         IFF_TIFF_G4 and IFF_TIFF_G3: for 1 bpp only
 *                         IFF_TIFF_LZW, IFF_TIFF_ZIP: lossless for any image
 *                         IFF_TIFF_JPEG: lossy 8 bpp gray or rgb
 * \param[in]    natags    [optional] NUMA of custom tiff tags
 * \param[in]    savals    [optional] SARRAY of values
 * \param[in]    satypes   [optional] SARRAY of types
 * \param[in]    nasizes   [optional] NUMA of sizes
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This static function should only be called through higher
 *          level functions in this file; namely, pixWriteTiffCustom(),
 *          pixWriteTiff(), pixWriteStreamTiff(), pixWriteMemTiff()
 *          and pixWriteMemTiffCustom().
 *      (2) We only allow PACKBITS for bpp = 1, because for bpp > 1
 *          it typically expands images that are not synthetically generated.
 *      (3) See pixWriteTiffCustom() for details on how to use
 *          the last four parameters for customized tiff tags.
 *      (4) The only valid pixel depths in leptonica are 1, 2, 4, 8, 16
 *          and 32.  However, it is possible, and in some cases desirable,
 *          to write out a tiff file using an rgb pix that has 24 bpp.
 *          This can be created by appending the raster data for a 24 bpp
 *          image (with proper scanline padding) directly to a 24 bpp
 *          pix that was created without a data array.  See note in
 *          pixWriteStreamPng() for an example.
 * </pre>
 */
static int32 pixWriteToTiffStream(TIFF    * tif,
    PIX     * pix,
    int32 comptype,
    NUMA * natags,
    SARRAY * savals,
    SARRAY * satypes,
    NUMA * nasizes)
{
	uint8   * linebuf, * data;
	uint16 redmap[256], greenmap[256], bluemap[256];
	int32 w, h, d, spp, i, j, k, wpl, bpl, tiffbpl, ncolors, cmapsize;
	int32 * rmap, * gmap, * bmap;
	int32 xres, yres;
	uint32  * line, * ppixel;
	PIX * pixt;
	PIXCMAP   * cmap;
	char      * text;

	PROCNAME(__FUNCTION__);

	if(!tif)
		return ERROR_INT("tif stream not defined", procName, 1);
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);

	pixSetPadBits(pix, 0);
	pixGetDimensions(pix, &w, &h, &d);
	spp = pixGetSpp(pix);
	xres = pixGetXRes(pix);
	yres = pixGetYRes(pix);
	if(xres == 0) xres = DefaultResolution;
	if(yres == 0) yres = DefaultResolution;

	/* ------------------ Write out the header -------------  */
	TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, (uint32)RESUNIT_INCH);
	TIFFSetField(tif, TIFFTAG_XRESOLUTION, (double)xres);
	TIFFSetField(tif, TIFFTAG_YRESOLUTION, (double)yres);

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32)w);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32)h);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

	if((text = pixGetText(pix)) != NULL)
		TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, text);

	if(d == 1 && !pixGetColormap(pix)) {
		/* If d == 1, preserve the colormap.  Note that when
		 * d == 1 pix with colormaps are read, the colormaps
		 * are removed.  The only pix in leptonica that have
		 * colormaps are made programmatically. */
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
	}
	else if((d == 32 && spp == 3) || d == 24) {
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16)3);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
		    (uint16)8, (uint16)8, (uint16)8);
	}
	else if(d == 32 && spp == 4) {
		uint16 val[1];
		val[0] = EXTRASAMPLE_ASSOCALPHA;
		TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, (uint16)1, &val);
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16)4);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
		    (uint16)8, (uint16)8, (uint16)8, (uint16)8);
	}
	else if(d == 16) { /* we only support spp = 1, bps = 16 */
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	}
	else if((cmap = pixGetColormap(pix)) == NULL) {
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	}
	else { /* Save colormap in the tiff; not more than 256 colors */
		if(d > 8) {
			L_ERROR("d = %d > 8 with colormap!; reducing to 8\n", procName, d);
			d = 8;
		}
		pixcmapToArrays(cmap, &rmap, &gmap, &bmap, NULL);
		ncolors = pixcmapGetCount(cmap);
		ncolors = MIN(256, ncolors); /* max 256 */
		cmapsize = 1 << d;
		cmapsize = MIN(256, cmapsize); /* power of 2; max 256 */
		if(ncolors > cmapsize) {
			L_WARNING("too many colors in cmap for tiff; truncating\n", procName);
			ncolors = cmapsize;
		}
		for(i = 0; i < ncolors; i++) {
			redmap[i] = (rmap[i] << 8) | rmap[i];
			greenmap[i] = (gmap[i] << 8) | gmap[i];
			bluemap[i] = (bmap[i] << 8) | bmap[i];
		}
		for(i = ncolors; i < cmapsize; i++) /* init, even though not used */
			redmap[i] = greenmap[i] = bluemap[i] = 0;
		SAlloc::F(rmap);
		SAlloc::F(gmap);
		SAlloc::F(bmap);

		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16)d);
		TIFFSetField(tif, TIFFTAG_COLORMAP, redmap, greenmap, bluemap);
	}

	if(d <= 16) {
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16)d);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
	}

	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	if(comptype == IFF_TIFF) { /* no compression */
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	}
	else if(comptype == IFF_TIFF_G4) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
	}
	else if(comptype == IFF_TIFF_G3) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX3);
	}
	else if(comptype == IFF_TIFF_RLE) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTRLE);
	}
	else if(comptype == IFF_TIFF_PACKBITS) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
	}
	else if(comptype == IFF_TIFF_LZW) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
	}
	else if(comptype == IFF_TIFF_ZIP) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
	}
	else if(comptype == IFF_TIFF_JPEG) {
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
	}
	else {
		L_WARNING("unknown tiff compression; using none\n", procName);
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	}

	/* This is a no-op if arrays are NULL */
	writeCustomTiffTags(tif, natags, savals, satypes, nasizes);

	/* ------------- Write out the image data -------------  */
	tiffbpl = TIFFScanlineSize(tif);
	wpl = pixGetWpl(pix);
	bpl = 4 * wpl;
	if(tiffbpl > bpl)
		lept_stderr("Big trouble: tiffbpl = %d, bpl = %d\n", tiffbpl, bpl);
	if((linebuf = (uint8 *)SAlloc::C(1, bpl)) == NULL)
		return ERROR_INT("calloc fail for linebuf", procName, 1);

	/* Use single strip for image */
	TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);

	if(d != 24 && d != 32) {
		if(d == 16)
			pixt = pixEndianTwoByteSwapNew(pix);
		else
			pixt = pixEndianByteSwapNew(pix);
		data = (uint8 *)pixGetData(pixt);
		for(i = 0; i < h; i++, data += bpl) {
			memcpy(linebuf, data, tiffbpl);
			if(TIFFWriteScanline(tif, linebuf, i, 0) < 0)
				break;
		}
		pixDestroy(&pixt);
	}
	else if(d == 24) { /* See note 4 above: special case of 24 bpp rgb */
		for(i = 0; i < h; i++) {
			line = pixGetData(pix) + i * wpl;
			if(TIFFWriteScanline(tif, (uint8 *)line, i, 0) < 0)
				break;
		}
	}
	else { /* 32 bpp rgb or rgba */
		for(i = 0; i < h; i++) {
			line = pixGetData(pix) + i * wpl;
			for(j = 0, k = 0, ppixel = line; j < w; j++) {
				linebuf[k++] = GET_DATA_BYTE(ppixel, COLOR_RED);
				linebuf[k++] = GET_DATA_BYTE(ppixel, COLOR_GREEN);
				linebuf[k++] = GET_DATA_BYTE(ppixel, COLOR_BLUE);
				if(spp == 4)
					linebuf[k++] = GET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL);
				ppixel++;
			}
			if(TIFFWriteScanline(tif, linebuf, i, 0) < 0)
				break;
		}
	}

/*    TIFFWriteDirectory(tif); */
	SAlloc::F(linebuf);

	return 0;
}

/*!
 * \brief   writeCustomTiffTags()
 *
 * \param[in]    tif
 * \param[in]    natags   [optional] NUMA of custom tiff tags
 * \param[in]    savals   [optional] SARRAY of values
 * \param[in]    satypes  [optional] SARRAY of types
 * \param[in]    nasizes  [optional] NUMA of sizes
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This static function should be called indirectly through
 *          higher level functions, such as pixWriteTiffCustom(),
 *          which call pixWriteToTiffStream().  See details in
 *          pixWriteTiffCustom() for using the 4 input arrays.
 *      (2) This is a no-op if the first 3 arrays are all NULL.
 *      (3) Otherwise, the first 3 arrays must be defined and all
 *          of equal size.
 *      (4) The fourth array is always optional.
 *      (5) The most commonly used types are "char*" and "u_int16".
 *          See tiff.h for a full listing of the tiff tags.
 *          Note that many of these tags, in particular the bit tags,
 *          are intended to be private, and cannot be set by this function.
 *          Examples are the STRIPOFFSETS and STRIPBYTECOUNTS tags,
 *          which are bit tags that are automatically set in the header,
 *          and can be extracted using tiffdump.
 * </pre>
 */
static int32 writeCustomTiffTags(TIFF    * tif,
    NUMA * natags,
    SARRAY * savals,
    SARRAY * satypes,
    NUMA * nasizes)
{
	char      * sval, * type;
	int32 i, n, ns, size, tagval, val;
	double dval;
	uint32 uval, uval2;

	PROCNAME(__FUNCTION__);

	if(!tif)
		return ERROR_INT("tif stream not defined", procName, 1);
	if(!natags && !savals && !satypes)
		return 0;
	if(!natags || !savals || !satypes)
		return ERROR_INT("not all arrays defined", procName, 1);
	n = numaGetCount(natags);
	if((sarrayGetCount(savals) != n) || (sarrayGetCount(satypes) != n))
		return ERROR_INT("not all sa the same size", procName, 1);

	/* The sized arrays (4 args to TIFFSetField) are written first */
	if(nasizes) {
		ns = numaGetCount(nasizes);
		if(ns > n)
			return ERROR_INT("too many 4-arg tag calls", procName, 1);
		for(i = 0; i < ns; i++) {
			numaGetIValue(natags, i, &tagval);
			sval = sarrayGetString(savals, i, L_NOCOPY);
			type = sarrayGetString(satypes, i, L_NOCOPY);
			numaGetIValue(nasizes, i, &size);
			if(strcmp(type, "char*") && strcmp(type, "uint8*"))
				L_WARNING("array type not char* or uint8*; ignore\n", procName);
			TIFFSetField(tif, tagval, size, sval);
		}
	}
	else {
		ns = 0;
	}
	/* The typical tags (3 args to TIFFSetField) are now written */
	for(i = ns; i < n; i++) {
		numaGetIValue(natags, i, &tagval);
		sval = sarrayGetString(savals, i, L_NOCOPY);
		type = sarrayGetString(satypes, i, L_NOCOPY);
		if(sstreq(type, "char*") || sstreq(type, "const char*")) {
			TIFFSetField(tif, tagval, sval);
		}
		else if(sstreq(type, "uint16")) {
			if(sscanf(sval, "%u", &uval) == 1) {
				TIFFSetField(tif, tagval, (uint16)uval);
			}
			else {
				lept_stderr("val %s not of type %s\n", sval, type);
				return ERROR_INT("custom tag(s) not written", procName, 1);
			}
		}
		else if(sstreq(type, "uint32")) {
			if(sscanf(sval, "%u", &uval) == 1) {
				TIFFSetField(tif, tagval, uval);
			}
			else {
				lept_stderr("val %s not of type %s\n", sval, type);
				return ERROR_INT("custom tag(s) not written", procName, 1);
			}
		}
		else if(sstreq(type, "int32")) {
			if(sscanf(sval, "%d", &val) == 1) {
				TIFFSetField(tif, tagval, val);
			}
			else {
				lept_stderr("val %s not of type %s\n", sval, type);
				return ERROR_INT("custom tag(s) not written", procName, 1);
			}
		}
		else if(sstreq(type, "double")) {
			if(sscanf(sval, "%lf", &dval) == 1) {
				TIFFSetField(tif, tagval, dval);
			}
			else {
				lept_stderr("val %s not of type %s\n", sval, type);
				return ERROR_INT("custom tag(s) not written", procName, 1);
			}
		}
		else if(sstreq(type, "uint16-uint16")) {
			if(sscanf(sval, "%u-%u", &uval, &uval2) == 2) {
				TIFFSetField(tif, tagval, (uint16)uval, (uint16)uval2);
			}
			else {
				lept_stderr("val %s not of type %s\n", sval, type);
				return ERROR_INT("custom tag(s) not written", procName, 1);
			}
		}
		else {
			lept_stderr("unknown type %s\n", type);
			return ERROR_INT("unknown type; tag(s) not written", procName, 1);
		}
	}
	return 0;
}

/*--------------------------------------------------------------*
*               Reading and writing multipage tiff             *
*--------------------------------------------------------------*/
/*!
 * \brief   pixReadFromMultipageTiff()
 *
 * \param[in]      fname     filename
 * \param[in,out]  poffset   set offset to 0 for first image
 * \return  pix, or NULL on error or if previous call returned the last image
 *
 * <pre>
 * Notes:
 *      (1) This allows overhead for traversal of a multipage tiff file
 *          to be linear in the number of images.  This will also work
 *          with a singlepage tiff file.
 *      (2) No TIFF internal data structures are exposed to the caller
 *          (thanks to Jeff Breidenbach).
 *      (3) offset is the byte offset of a particular image in a multipage
 *          tiff file. To get the first image in the file, input the
 *          special offset value of 0.
 *      (4) The offset is updated to point to the next image, for a
 *          subsequent call.
 *      (5) On the last image, the offset returned is 0.  Exit the loop
 *          when the returned offset is 0.
 *      (6) For reading a multipage tiff from a memory buffer, see
 *            pixReadMemFromMultipageTiff()
 *      (7) Example usage for reading all the images in the tif file:
 *            size_t offset = 0;
 *            do {
 *                Pix *pix = pixReadFromMultipageTiff(filename, &offset);
 *                // do something with pix
 *            } while (offset != 0);
 * </pre>
 */
PIX * pixReadFromMultipageTiff(const char * fname,
    size_t * poffset)
{
	int32 retval;
	size_t offset;
	PIX     * pix;
	TIFF    * tif;

	PROCNAME(__FUNCTION__);

	if(!fname)
		return (PIX *)ERROR_PTR("fname not defined", procName, NULL);
	if(!poffset)
		return (PIX *)ERROR_PTR("&offset not defined", procName, NULL);

	if((tif = openTiff(fname, "r")) == NULL) {
		L_ERROR("tif open failed for %s\n", procName, fname);
		return NULL;
	}

	/* Set ptrs in the TIFF to the beginning of the image */
	offset = *poffset;
	retval = (offset == 0) ? TIFFSetDirectory(tif, 0)
	    : TIFFSetSubDirectory(tif, offset);
	if(retval == 0) {
		TIFFClose(tif);
		return NULL;
	}

	if((pix = pixReadFromTiffStream(tif)) == NULL) {
		TIFFClose(tif);
		return NULL;
	}

	/* Advance to the next image and return the new offset */
	TIFFReadDirectory(tif);
	*poffset = TIFFCurrentDirOffset(tif);
	TIFFClose(tif);
	return pix;
}

/*!
 * \brief   pixaReadMultipageTiff()
 *
 * \param[in]    filename    input tiff file
 * \return  pixa of page images, or NULL on error
 */
PIXA * pixaReadMultipageTiff(const char * filename)
{
	int32 i, npages;
	FILE * fp;
	PIX     * pix;
	PIXA    * pixa;
	TIFF    * tif;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return (PIXA*)ERROR_PTR("filename not defined", procName, NULL);

	if((fp = fopenReadStream(filename)) == NULL)
		return (PIXA*)ERROR_PTR("stream not opened", procName, NULL);
	if(fileFormatIsTiff(fp)) {
		tiffGetCount(fp, &npages);
		L_INFO(" Tiff: %d pages\n", procName, npages);
	}
	else {
		return (PIXA*)ERROR_PTR("file not tiff", procName, NULL);
	}

	if((tif = fopenTiff(fp, "r")) == NULL)
		return (PIXA*)ERROR_PTR("tif not opened", procName, NULL);

	pixa = pixaCreate(npages);
	pix = NULL;
	for(i = 0; i < npages; i++) {
		if((pix = pixReadFromTiffStream(tif)) != NULL) {
			pixaAddPix(pixa, pix, L_INSERT);
		}
		else {
			L_WARNING("pix not read for page %d\n", procName, i);
		}

		/* Advance to the next directory (i.e., the next image) */
		if(TIFFReadDirectory(tif) == 0)
			break;
	}

	fclose(fp);
	TIFFCleanup(tif);
	return pixa;
}

/*!
 * \brief   pixaWriteMultipageTiff()
 *
 * \param[in]    fname      input tiff file
 * \param[in]    pixa       any depth; colormap will be removed
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The tiff directory overhead is O(n^2).  I have not been
 *          able to reduce it to O(n).  The overhead for n = 2000 is
 *          about 1 second.
 * </pre>
 */
l_ok pixaWriteMultipageTiff(const char * fname,
    PIXA        * pixa)
{
	const char * modestr;
	int32 i, n;
	PIX         * pix1;

	PROCNAME(__FUNCTION__);

	if(!fname)
		return ERROR_INT("fname not defined", procName, 1);
	if(!pixa)
		return ERROR_INT("pixa not defined", procName, 1);

	n = pixaGetCount(pixa);
	for(i = 0; i < n; i++) {
		modestr = (i == 0) ? "w" : "a";
		pix1 = pixaGetPix(pixa, i, L_CLONE);
		if(pixGetDepth(pix1) == 1)
			pixWriteTiff(fname, pix1, IFF_TIFF_G4, modestr);
		else
			pixWriteTiff(fname, pix1, IFF_TIFF_ZIP, modestr);
		pixDestroy(&pix1);
	}

	return 0;
}

/*!
 * \brief   writeMultipageTiff()
 *
 * \param[in]    dirin   input directory
 * \param[in]    substr  [optional] substring filter on filenames; can be NULL
 * \param[in]    fileout output multipage tiff file
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This writes a set of image files in a directory out
 *          as a multipage tiff file.  The images can be in any
 *          initial file format.
 *      (2) Images with a colormap have the colormap removed before
 *          re-encoding as tiff.
 *      (3) All images are encoded losslessly.  Those with 1 bpp are
 *          encoded 'g4'.  The rest are encoded as 'zip' (flate encoding).
 *          Because it is lossless, this is an expensive method for
 *          saving most rgb images.
 *      (4) The tiff directory overhead is quadratic in the number of
 *          images.  To avoid this for very large numbers of images to be
 *          written, apply the method used in pixaWriteMultipageTiff().
 * </pre>
 */
l_ok writeMultipageTiff(const char * dirin,
    const char * substr,
    const char * fileout)
{
	SARRAY * sa;

	PROCNAME(__FUNCTION__);

	if(!dirin)
		return ERROR_INT("dirin not defined", procName, 1);
	if(!fileout)
		return ERROR_INT("fileout not defined", procName, 1);

	/* Get all filtered and sorted full pathnames. */
	sa = getSortedPathnamesInDirectory(dirin, substr, 0, 0);

	/* Generate the tiff file */
	writeMultipageTiffSA(sa, fileout);
	sarrayDestroy(&sa);
	return 0;
}

/*!
 * \brief   writeMultipageTiffSA()
 *
 * \param[in]    sa       string array of full path names
 * \param[in]    fileout  output ps file
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See writeMultipageTiff()
 * </pre>
 */
l_ok writeMultipageTiffSA(SARRAY * sa,
    const char * fileout)
{
	char * fname;
	const char * op;
	int32 i, nfiles, firstfile, format;
	PIX         * pix;

	PROCNAME(__FUNCTION__);

	if(!sa)
		return ERROR_INT("sa not defined", procName, 1);
	if(!fileout)
		return ERROR_INT("fileout not defined", procName, 1);

	nfiles = sarrayGetCount(sa);
	firstfile = TRUE;
	for(i = 0; i < nfiles; i++) {
		op = (firstfile) ? "w" : "a";
		fname = sarrayGetString(sa, i, L_NOCOPY);
		findFileFormat(fname, &format);
		if(format == IFF_UNKNOWN) {
			L_INFO("format of %s not known\n", procName, fname);
			continue;
		}

		if((pix = pixRead(fname)) == NULL) {
			L_WARNING("pix not made for file: %s\n", procName, fname);
			continue;
		}
		if(pixGetDepth(pix) == 1)
			pixWriteTiff(fileout, pix, IFF_TIFF_G4, op);
		else
			pixWriteTiff(fileout, pix, IFF_TIFF_ZIP, op);
		firstfile = FALSE;
		pixDestroy(&pix);
	}

	return 0;
}

/*--------------------------------------------------------------*
*                    Print info to stream                      *
*--------------------------------------------------------------*/
/*!
 * \brief   fprintTiffInfo()
 *
 * \param[in]    fpout    stream for output of tag data
 * \param[in]    tiffile  input
 * \return  0 if OK; 1 on error
 */
l_ok fprintTiffInfo(FILE        * fpout,
    const char * tiffile)
{
	TIFF  * tif;

	PROCNAME(__FUNCTION__);

	if(!tiffile)
		return ERROR_INT("tiffile not defined", procName, 1);
	if(!fpout)
		return ERROR_INT("stream out not defined", procName, 1);

	if((tif = openTiff(tiffile, "rb")) == NULL)
		return ERROR_INT("tif not open for read", procName, 1);

	TIFFPrintDirectory(tif, fpout, 0);
	TIFFClose(tif);

	return 0;
}

/*--------------------------------------------------------------*
*                        Get page count                        *
*--------------------------------------------------------------*/
/*!
 * \brief   tiffGetCount()
 *
 * \param[in]    fp   file stream opened for read
 * \param[out]   pn   number of images
 * \return  0 if OK; 1 on error
 */
l_ok tiffGetCount(FILE * fp,
    int32 * pn)
{
	int32 i;
	TIFF    * tif;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);
	if(!pn)
		return ERROR_INT("&n not defined", procName, 1);
	*pn = 0;

	if((tif = fopenTiff(fp, "r")) == NULL)
		return ERROR_INT("tif not open for read", procName, 1);

	for(i = 1;; i++) {
		if(TIFFReadDirectory(tif) == 0)
			break;
		if(i == ManyPagesInTiffFile + 1) {
			L_WARNING("big file: more than %d pages\n", procName,
			    ManyPagesInTiffFile);
		}
	}
	*pn = i;
	TIFFCleanup(tif);
	return 0;
}

/*--------------------------------------------------------------*
*                   Get resolution from tif                    *
*--------------------------------------------------------------*/
/*!
 * \brief   getTiffResolution()
 *
 * \param[in]    fp            file stream opened for read
 * \param[out]   pxres, pyres  resolution in ppi
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 * </pre>
 */
l_ok getTiffResolution(FILE * fp,
    int32 * pxres,
    int32 * pyres)
{
	TIFF  * tif;

	PROCNAME(__FUNCTION__);

	if(!pxres || !pyres)
		return ERROR_INT("&xres and &yres not both defined", procName, 1);
	*pxres = *pyres = 0;
	if(!fp)
		return ERROR_INT("stream not opened", procName, 1);

	if((tif = fopenTiff(fp, "r")) == NULL)
		return ERROR_INT("tif not open for read", procName, 1);
	getTiffStreamResolution(tif, pxres, pyres);
	TIFFCleanup(tif);
	return 0;
}

/*!
 * \brief   getTiffStreamResolution()
 *
 * \param[in]    tif            TIFF handle opened for read
 * \param[out]   pxres, pyres   resolution in ppi
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If neither resolution field is set, this is not an error;
 *          the returned resolution values are 0 (designating 'unknown').
 * </pre>
 */
static int32 getTiffStreamResolution(TIFF     * tif,
    int32 * pxres,
    int32 * pyres)
{
	uint16 resunit;
	int32 foundxres, foundyres;
	float fxres, fyres;

	PROCNAME(__FUNCTION__);

	if(!tif)
		return ERROR_INT("tif not opened", procName, 1);
	if(!pxres || !pyres)
		return ERROR_INT("&xres and &yres not both defined", procName, 1);
	*pxres = *pyres = 0;

	TIFFGetFieldDefaulted(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
	foundxres = TIFFGetField(tif, TIFFTAG_XRESOLUTION, &fxres);
	foundyres = TIFFGetField(tif, TIFFTAG_YRESOLUTION, &fyres);
	if(!foundxres && !foundyres) return 1;
	if(isnan(fxres) || isnan(fyres)) return 1;
	if(!foundxres && foundyres)
		fxres = fyres;
	else if(foundxres && !foundyres)
		fyres = fxres;

	/* Avoid overflow into int32; set max fxres and fyres to 5 x 10^8 */
	if(fxres < 0 || fxres > (1L << 29) || fyres < 0 || fyres > (1L << 29))
		return ERROR_INT("fxres and/or fyres values are invalid", procName, 1);

	if(resunit == RESUNIT_CENTIMETER) { /* convert to ppi */
		*pxres = (int32)(2.54 * fxres + 0.5);
		*pyres = (int32)(2.54 * fyres + 0.5);
	}
	else {
		*pxres = (int32)fxres;
		*pyres = (int32)fyres;
	}

	return 0;
}

/*--------------------------------------------------------------*
*              Get some tiff header information                *
*--------------------------------------------------------------*/
/*!
 * \brief   readHeaderTiff()
 *
 * \param[in]    filename
 * \param[in]    n          page image number: 0-based
 * \param[out]   pw         [optional] width
 * \param[out]   ph         [optional] height
 * \param[out]   pbps       [optional] bits per sample -- 1, 2, 4 or 8
 * \param[out]   pspp       [optional] samples per pixel -- 1 or 3
 * \param[out]   pres       [optional] resolution in x dir; NULL to ignore
 * \param[out]   pcmap      [optional] colormap exists; input NULL to ignore
 * \param[out]   pformat    [optional] tiff format; input NULL to ignore
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If there is a colormap, cmap is returned as 1; else 0.
 *      (2) If %n is equal to or greater than the number of images, returns 1.
 * </pre>
 */
l_ok readHeaderTiff(const char * filename,
    int32 n,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * pres,
    int32 * pcmap,
    int32 * pformat)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(pw) *pw = 0;
	if(ph) *ph = 0;
	if(pbps) *pbps = 0;
	if(pspp) *pspp = 0;
	if(pres) *pres = 0;
	if(pcmap) *pcmap = 0;
	if(pformat) *pformat = 0;
	if(!filename)
		return ERROR_INT("filename not defined", procName, 1);
	if(!pw && !ph && !pbps && !pspp && !pres && !pcmap && !pformat)
		return ERROR_INT("no results requested", procName, 1);

	if((fp = fopenReadStream(filename)) == NULL)
		return ERROR_INT("image file not found", procName, 1);
	ret = freadHeaderTiff(fp, n, pw, ph, pbps, pspp, pres, pcmap, pformat);
	fclose(fp);
	return ret;
}

/*!
 * \brief   freadHeaderTiff()
 *
 * \param[in]    fp       file stream
 * \param[in]    n        page image number: 0-based
 * \param[out]   pw       [optional] width
 * \param[out]   ph       [optional] height
 * \param[out]   pbps     [optional] bits per sample -- 1, 2, 4 or 8
 * \param[out]   pspp     [optional] samples per pixel -- 1 or 3
 * \param[out]   pres     [optional] resolution in x dir; NULL to ignore
 * \param[out]   pcmap    [optional] colormap exists; input NULL to ignore
 * \param[out]   pformat  [optional] tiff format; input NULL to ignore
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) If there is a colormap, cmap is returned as 1; else 0.
 *      (2) If %n is equal to or greater than the number of images, returns 1.
 * </pre>
 */
l_ok freadHeaderTiff(FILE * fp,
    int32 n,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * pres,
    int32 * pcmap,
    int32 * pformat)
{
	int32 i, ret, format;
	TIFF    * tif;

	PROCNAME(__FUNCTION__);

	if(pw) *pw = 0;
	if(ph) *ph = 0;
	if(pbps) *pbps = 0;
	if(pspp) *pspp = 0;
	if(pres) *pres = 0;
	if(pcmap) *pcmap = 0;
	if(pformat) *pformat = 0;
	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);
	if(n < 0)
		return ERROR_INT("image index must be >= 0", procName, 1);
	if(!pw && !ph && !pbps && !pspp && !pres && !pcmap && !pformat)
		return ERROR_INT("no results requested", procName, 1);

	findFileFormatStream(fp, &format);
	if(!L_FORMAT_IS_TIFF(format))
		return ERROR_INT("file not tiff format", procName, 1);

	if((tif = fopenTiff(fp, "r")) == NULL)
		return ERROR_INT("tif not open for read", procName, 1);

	for(i = 0; i < n; i++) {
		if(TIFFReadDirectory(tif) == 0)
			return ERROR_INT("image n not found in file", procName, 1);
	}

	ret = tiffReadHeaderTiff(tif, pw, ph, pbps, pspp, pres, pcmap, pformat);
	TIFFCleanup(tif);
	return ret;
}

/*!
 * \brief   readHeaderMemTiff()
 *
 * \param[in]    cdata     const; tiff-encoded
 * \param[in]    size      size of data
 * \param[in]    n         page image number: 0-based
 * \param[out]   pw        [optional] width
 * \param[out]   ph        [optional] height
 * \param[out]   pbps      [optional] bits per sample -- 1, 2, 4 or 8
 * \param[out]   pspp      [optional] samples per pixel -- 1 or 3
 * \param[out]   pres      [optional] resolution in x dir; NULL to ignore
 * \param[out]   pcmap     [optional] colormap exists; input NULL to ignore
 * \param[out]   pformat   [optional] tiff format; input NULL to ignore
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Use TIFFClose(); TIFFCleanup() doesn't free internal memstream.
 * </pre>
 */
l_ok readHeaderMemTiff(const uint8  * cdata,
    size_t size,
    int32 n,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * pres,
    int32 * pcmap,
    int32 * pformat)
{
	uint8  * data;
	int32 i, ret;
	TIFF     * tif;

	PROCNAME(__FUNCTION__);

	if(pw) *pw = 0;
	if(ph) *ph = 0;
	if(pbps) *pbps = 0;
	if(pspp) *pspp = 0;
	if(pres) *pres = 0;
	if(pcmap) *pcmap = 0;
	if(pformat) *pformat = 0;
	if(!pw && !ph && !pbps && !pspp && !pres && !pcmap && !pformat)
		return ERROR_INT("no results requested", procName, 1);
	if(!cdata)
		return ERROR_INT("cdata not defined", procName, 1);

	/* Open a tiff stream to memory */
	data = (uint8 *)cdata; /* we're really not going to change this */
	if((tif = fopenTiffMemstream("tifferror", "r", &data, &size)) == NULL)
		return ERROR_INT("tiff stream not opened", procName, 1);

	for(i = 0; i < n; i++) {
		if(TIFFReadDirectory(tif) == 0) {
			TIFFClose(tif);
			return ERROR_INT("image n not found in file", procName, 1);
		}
	}

	ret = tiffReadHeaderTiff(tif, pw, ph, pbps, pspp, pres, pcmap, pformat);
	TIFFClose(tif);
	return ret;
}

/*!
 * \brief   tiffReadHeaderTiff()
 *
 * \param[in]    tif
 * \param[out]   pw        [optional] width
 * \param[out]   ph        [optional] height
 * \param[out]   pbps      [optional] bits per sample -- 1, 2, 4 or 8
 * \param[out]   pspp      [optional] samples per pixel -- 1 or 3
 * \param[out]   pres      [optional] resolution in x dir; NULL to ignore
 * \param[out]   pcmap     [optional] cmap exists; input NULL to ignore
 * \param[out]   pformat   [optional] tiff format; input NULL to ignore
 * \return  0 if OK, 1 on error
 */
static int32 tiffReadHeaderTiff(TIFF     * tif,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * pres,
    int32 * pcmap,
    int32 * pformat)
{
	uint16 tiffcomp;
	uint16 bps, spp;
	uint16  * rmap, * gmap, * bmap;
	int32 xres, yres;
	uint32 w, h;

	PROCNAME(__FUNCTION__);

	if(!tif)
		return ERROR_INT("tif not opened", procName, 1);

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bps);
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	if(w < 1 || h < 1)
		return ERROR_INT("tif w and h not both > 0", procName, 1);
	if(bps != 1 && bps != 2 && bps != 4 && bps != 8 && bps != 16)
		return ERROR_INT("bps not in set {1,2,4,8,16}", procName, 1);
	if(spp != 1 && spp != 2 && spp != 3 && spp != 4)
		return ERROR_INT("spp not in set {1,2,3,4}", procName, 1);
	if(pw) *pw = w;
	if(ph) *ph = h;
	if(pbps) *pbps = bps;
	if(pspp) *pspp = spp;
	if(pres) {
		*pres = 300; /* default ppi */
		if(getTiffStreamResolution(tif, &xres, &yres) == 0)
			*pres = (int32)xres;
	}
	if(pcmap) {
		*pcmap = 0;
		if(TIFFGetField(tif, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap))
			*pcmap = 1;
	}
	if(pformat) {
		TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &tiffcomp);
		*pformat = getTiffCompressedFormat(tiffcomp);
	}
	return 0;
}

/*!
 * \brief   findTiffCompression()
 *
 * \param[in]    fp         file stream; must be rewound to BOF
 * \param[out]   pcomptype  compression type
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) The returned compression type is that defined in
 *          the enum in imageio.h.  It is not the tiff flag value.
 *      (2) The compression type is initialized to IFF_UNKNOWN.
 *          If it is not one of the specified types, the returned
 *          type is IFF_TIFF, which indicates no compression.
 *      (3) When this function is called, the stream must be at BOF.
 *          If the opened stream is to be used again to read the
 *          file, it must be rewound to BOF after calling this function.
 * </pre>
 */
l_ok findTiffCompression(FILE * fp,
    int32 * pcomptype)
{
	uint16 tiffcomp;
	TIFF     * tif;

	PROCNAME(__FUNCTION__);

	if(!pcomptype)
		return ERROR_INT("&comptype not defined", procName, 1);
	*pcomptype = IFF_UNKNOWN; /* init */
	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);

	if((tif = fopenTiff(fp, "r")) == NULL)
		return ERROR_INT("tif not opened", procName, 1);
	TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &tiffcomp);
	*pcomptype = getTiffCompressedFormat(tiffcomp);
	TIFFCleanup(tif);
	return 0;
}

/*!
 * \brief   getTiffCompressedFormat()
 *
 * \param[in]    tiffcomp    defined in tiff.h
 * \return  compression format defined in imageio.h
 *
 * <pre>
 * Notes:
 *      (1) The input must be the actual tiff compression type
 *          returned by a tiff library call.  It should always be
 *          a valid tiff type.
 *      (2) The return type is defined in the enum in imageio.h.
 * </pre>
 */
static int32 getTiffCompressedFormat(uint16 tiffcomp)
{
	int32 comptype;

	switch(tiffcomp)
	{
		case COMPRESSION_CCITTFAX4:
		    comptype = IFF_TIFF_G4;
		    break;
		case COMPRESSION_CCITTFAX3:
		    comptype = IFF_TIFF_G3;
		    break;
		case COMPRESSION_CCITTRLE:
		    comptype = IFF_TIFF_RLE;
		    break;
		case COMPRESSION_PACKBITS:
		    comptype = IFF_TIFF_PACKBITS;
		    break;
		case COMPRESSION_LZW:
		    comptype = IFF_TIFF_LZW;
		    break;
		case COMPRESSION_ADOBE_DEFLATE:
		    comptype = IFF_TIFF_ZIP;
		    break;
		case COMPRESSION_JPEG:
		    comptype = IFF_TIFF_JPEG;
		    break;
		default:
		    comptype = IFF_TIFF;
		    break;
	}
	return comptype;
}

/*--------------------------------------------------------------*
*                   Extraction of tiff g4 data                 *
*--------------------------------------------------------------*/
/*!
 * \brief   extractG4DataFromFile()
 *
 * \param[in]    filein
 * \param[out]   pdata         binary data of ccitt g4 encoded stream
 * \param[out]   pnbytes       size of binary data
 * \param[out]   pw            [optional] image width
 * \param[out]   ph            [optional] image height
 * \param[out]   pminisblack   [optional] boolean
 * \return  0 if OK, 1 on error
 */
l_ok extractG4DataFromFile(const char * filein,
    uint8 ** pdata,
    size_t * pnbytes,
    int32     * pw,
    int32     * ph,
    int32     * pminisblack)
{
	uint8  * inarray, * data;
	uint16 minisblack, comptype; /* accessors require uint16 */
	int32 istiff;
	uint32 w, h, rowsperstrip; /* accessors require uint32 */
	uint32 diroff;
	size_t fbytes, nbytes;
	FILE * fpin;
	TIFF     * tif;

	PROCNAME(__FUNCTION__);

	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	if(!pnbytes)
		return ERROR_INT("&nbytes not defined", procName, 1);
	if(!pw && !ph && !pminisblack)
		return ERROR_INT("no output data requested", procName, 1);
	*pdata = NULL;
	*pnbytes = 0;

	if((fpin = fopenReadStream(filein)) == NULL)
		return ERROR_INT("stream not opened to file", procName, 1);
	istiff = fileFormatIsTiff(fpin);
	fclose(fpin);
	if(!istiff)
		return ERROR_INT("filein not tiff", procName, 1);

	if((inarray = l_binaryRead(filein, &fbytes)) == NULL)
		return ERROR_INT("inarray not made", procName, 1);

	/* Get metadata about the image */
	if((tif = openTiff(filein, "rb")) == NULL) {
		SAlloc::F(inarray);
		return ERROR_INT("tif not open for read", procName, 1);
	}
	TIFFGetField(tif, TIFFTAG_COMPRESSION, &comptype);
	if(comptype != COMPRESSION_CCITTFAX4) {
		SAlloc::F(inarray);
		TIFFClose(tif);
		return ERROR_INT("filein is not g4 compressed", procName, 1);
	}

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	if(h != rowsperstrip)
		L_WARNING("more than 1 strip\n", procName);
	TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &minisblack); /* for 1 bpp */
/*    TIFFPrintDirectory(tif, stderr, 0); */
	TIFFClose(tif);
	if(pw) *pw = (int32)w;
	if(ph) *ph = (int32)h;
	if(pminisblack) *pminisblack = (int32)minisblack;

	/* The header has 8 bytes: the first 2 are the magic number,
	* the next 2 are the version, and the last 4 are the
	* offset to the first directory.  That's what we want here.
	* We have to test the byte order before decoding 4 bytes! */
	if(inarray[0] == 0x4d) { /* big-endian */
		diroff = (inarray[4] << 24) | (inarray[5] << 16) |
		    (inarray[6] << 8) | inarray[7];
	}
	else {  /* inarray[0] == 0x49 :  little-endian */
		diroff = (inarray[7] << 24) | (inarray[6] << 16) |
		    (inarray[5] << 8) | inarray[4];
	}
/*    lept_stderr(" diroff = %d, %x\n", diroff, diroff); */

	/* Extract the ccittg4 encoded data from the tiff file.
	 * We skip the 8 byte header and take nbytes of data,
	 * up to the beginning of the directory (at diroff)  */
	nbytes = diroff - 8;
	if(nbytes > MaxNumTiffBytes) {
		SAlloc::F(inarray);
		L_ERROR("requesting %zu bytes > %zu\n", procName,
		    nbytes, MaxNumTiffBytes);
		return 1;
	}
	*pnbytes = nbytes;
	if((data = (uint8 *)SAlloc::C(nbytes, sizeof(uint8))) == NULL) {
		SAlloc::F(inarray);
		return ERROR_INT("data not allocated", procName, 1);
	}
	*pdata = data;
	memcpy(data, inarray + 8, nbytes);
	SAlloc::F(inarray);

	return 0;
}

/*--------------------------------------------------------------*
*               Open tiff stream from file stream              *
*--------------------------------------------------------------*/
/*!
 * \brief   fopenTiff()
 *
 * \param[in]    fp           file stream
 * \param[in]    modestring   "r", "w", ...
 * \return  tiff data structure, opened for a file descriptor
 *
 * <pre>
 * Notes:
 *      (1) Why is this here?  Leffler did not provide a function that
 *          takes a stream and gives a TIFF.  He only gave one that
 *          generates a TIFF starting with a file descriptor.  So we
 *          need to make it here, because it is useful to have functions
 *          that take a stream as input.
 *      (2) We use TIFFClientOpen() together with a set of static wrapper
 *          functions which map TIFF read, write, seek, close and size.
 *          to functions expecting a cookie of type stream (i.e. FILE *).
 *          This implementation was contributed by Jürgen Buchmüller.
 * </pre>
 */
static TIFF * fopenTiff(FILE        * fp,
    const char * modestring)
{
	PROCNAME(__FUNCTION__);

	if(!fp)
		return (TIFF*)ERROR_PTR("stream not opened", procName, NULL);
	if(!modestring)
		return (TIFF*)ERROR_PTR("modestring not defined", procName, NULL);

	TIFFSetWarningHandler(NULL); /* disable warnings */
	TIFFSetErrorHandler(NULL); /* disable error messages */

	fseek(fp, 0, SEEK_SET);
	return TIFFClientOpen("TIFFstream", modestring, (thandle_t)fp,
		   lept_read_proc, lept_write_proc, lept_seek_proc,
		   lept_close_proc, lept_size_proc, NULL, NULL);
}

/*--------------------------------------------------------------*
*                      Wrapper for TIFFOpen                    *
*--------------------------------------------------------------*/
/*!
 * \brief   openTiff()
 *
 * \param[in]    filename
 * \param[in]    modestring   "r", "w", ...
 * \return  tiff data structure
 *
 * <pre>
 * Notes:
 *      (1) This handles multi-platform file naming.
 * </pre>
 */
static TIFF * openTiff(const char * filename,
    const char * modestring)
{
	char * fname;
	TIFF  * tif;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return (TIFF*)ERROR_PTR("filename not defined", procName, NULL);
	if(!modestring)
		return (TIFF*)ERROR_PTR("modestring not defined", procName, NULL);

	TIFFSetWarningHandler(NULL); /* disable warnings */
	TIFFSetErrorHandler(NULL); /* disable error messages */

	fname = genPathname(filename, NULL);
	tif = TIFFOpen(fname, modestring);
	SAlloc::F(fname);
	return tif;
}

/*----------------------------------------------------------------------*
*     Memory I/O: reading memory --> pix and writing pix --> memory    *
*----------------------------------------------------------------------*/
/*  It would be nice to use open_memstream() and fmemopen()
 *  for writing and reading to memory, rsp.  These functions manage
 *  memory for writes and reads that use a file streams interface.
 *  Unfortunately, the tiff library only has an interface for reading
 *  and writing to file descriptors, not to file streams.  The tiff
 *  library procedure is to open a "tiff stream" and read/write to it.
 *  The library provides a client interface for managing the I/O
 *  from memory, which requires seven callbacks.  See the TIFFClientOpen
 *  man page for callback signatures.  Adam Langley provided the code
 *  to do this.  */

/*!
 * \brief   Memory stream buffer used with TIFFClientOpen()
 *
 *  The L_Memstram %buffer has different functions in writing and reading.
 *
 *     * In reading, it is assigned to the data and read from as
 *       the tiff library uncompresses the data and generates the pix.
 *       The %offset points to the current read position in the data,
 *       and the %hw always gives the number of bytes of data.
 *       The %outdata and %outsize ptrs are not used.
 *       When finished, tiffCloseCallback() simply frees the L_Memstream.
 *
 *     * In writing, it accepts the data that the tiff library
 *       produces when a pix is compressed.  the buffer points to a
 *       malloced area of %bufsize bytes.  The current writing position
 *       in the buffer is %offset and the most ever written is %hw.
 *       The buffer is expanded as necessary.  When finished,
 *       tiffCloseCallback() assigns the %outdata and %outsize ptrs
 *       to the %buffer and %bufsize results, and frees the L_Memstream.
 */
struct L_Memstream {
	uint8   * buffer; /* expands to hold data when written to;         */
	                  /* fixed size when read from.                    */
	size_t bufsize; /* current size allocated when written to;       */
	                /* fixed size of input data when read from.      */
	size_t offset; /* byte offset from beginning of buffer.         */
	size_t hw; /* high-water mark; max bytes in buffer.         */
	uint8  ** poutdata; /* input param for writing; data goes here.      */
	size_t * poutsize; /* input param for writing; data size goes here. */
};

typedef struct L_Memstream L_MEMSTREAM;

/* These are static functions for memory I/O */
static L_MEMSTREAM * memstreamCreateForRead(uint8 * indata, size_t pinsize);
static L_MEMSTREAM * memstreamCreateForWrite(uint8 ** poutdata, size_t * poutsize);
static tsize_t tiffReadCallback(thandle_t handle, tdata_t data, tsize_t length);
static tsize_t tiffWriteCallback(thandle_t handle, tdata_t data, tsize_t length);
static toff_t tiffSeekCallback(thandle_t handle, toff_t offset, int whence);
static int  tiffCloseCallback(thandle_t handle);
static toff_t tiffSizeCallback(thandle_t handle);
static int  tiffMapCallback(thandle_t handle, tdata_t * data, toff_t * length);
static void tiffUnmapCallback(thandle_t handle, tdata_t data, toff_t length);

static L_MEMSTREAM * memstreamCreateForRead(uint8  * indata, size_t insize)
{
	L_MEMSTREAM  * mstream = (L_MEMSTREAM*)SAlloc::C(1, sizeof(L_MEMSTREAM));
	mstream->buffer = indata; /* handle to input data array */
	mstream->bufsize = insize; /* amount of input data */
	mstream->hw = insize; /* high-water mark fixed at input data size */
	mstream->offset = 0; /* offset always starts at 0 */
	return mstream;
}

static L_MEMSTREAM * memstreamCreateForWrite(uint8  ** poutdata, size_t * poutsize)
{
	L_MEMSTREAM  * mstream = (L_MEMSTREAM*)SAlloc::C(1, sizeof(L_MEMSTREAM));
	mstream->buffer = (uint8 *)SAlloc::C(8 * 1024, 1);
	mstream->bufsize = 8 * 1024;
	mstream->poutdata = poutdata; /* used only at end of write */
	mstream->poutsize = poutsize; /* ditto  */
	mstream->hw = mstream->offset = 0;
	return mstream;
}

static tsize_t tiffReadCallback(thandle_t handle,
    tdata_t data,
    tsize_t length)
{
	L_MEMSTREAM  * mstream;
	size_t amount;

	mstream = (L_MEMSTREAM*)handle;
	amount = MIN((size_t)length, mstream->hw - mstream->offset);

	/* Fuzzed files can create this condition! */
	if(mstream->offset + amount < amount || /* overflow */
	    mstream->offset + amount > mstream->hw) {
		lept_stderr("Bad file: amount too big: %zu\n", amount);
		return 0;
	}

	memcpy(data, mstream->buffer + mstream->offset, amount);
	mstream->offset += amount;
	return amount;
}

static tsize_t tiffWriteCallback(thandle_t handle,
    tdata_t data,
    tsize_t length)
{
	L_MEMSTREAM  * mstream;
	size_t newsize;

	/* reallocNew() uses calloc to initialize the array.
	 * If malloc is used instead, for some of the encoding methods,
	 * not all the data in 'bufsize' bytes in the buffer will
	 * have been initialized by the end of the compression. */
	mstream = (L_MEMSTREAM*)handle;
	if(mstream->offset + length > mstream->bufsize) {
		newsize = 2 * (mstream->offset + length);
		mstream->buffer = (uint8 *)reallocNew((void**)&mstream->buffer,
			mstream->hw, newsize);
		mstream->bufsize = newsize;
	}

	memcpy(mstream->buffer + mstream->offset, data, length);
	mstream->offset += length;
	mstream->hw = MAX(mstream->offset, mstream->hw);
	return length;
}

static toff_t tiffSeekCallback(thandle_t handle, toff_t offset, int whence)
{
	L_MEMSTREAM  * mstream;
	PROCNAME(__FUNCTION__);
	mstream = (L_MEMSTREAM*)handle;
	switch(whence) {
		case SEEK_SET:
/*            lept_stderr("seek_set: offset = %d\n", offset); */
		    if((size_t)offset != offset) { /* size_t overflow on uint32 */
			    return (toff_t)ERROR_INT("too large offset value", procName, 1);
		    }
		    mstream->offset = offset;
		    break;
		case SEEK_CUR:
/*            lept_stderr("seek_cur: offset = %d\n", offset); */
		    mstream->offset += offset;
		    break;
		case SEEK_END:
/*            lept_stderr("seek end: hw = %d, offset = %d\n",
                    mstream->hw, offset); */
		    mstream->offset = mstream->hw - offset; /* offset >= 0 */
		    break;
		default:
		    return (toff_t)ERROR_INT("bad whence value", procName,
			       mstream->offset);
	}

	return mstream->offset;
}

static int tiffCloseCallback(thandle_t handle)
{
	L_MEMSTREAM  * mstream = (L_MEMSTREAM*)handle;
	if(mstream->poutdata) { /* writing: save the output data */
		*mstream->poutdata = mstream->buffer;
		*mstream->poutsize = mstream->hw;
	}
	SAlloc::F(mstream); /* never free the buffer! */
	return 0;
}

static toff_t tiffSizeCallback(thandle_t handle)
{
	L_MEMSTREAM  * mstream = (L_MEMSTREAM*)handle;
	return mstream->hw;
}

static int tiffMapCallback(thandle_t handle, tdata_t * data, toff_t * length)
{
	L_MEMSTREAM  * mstream = (L_MEMSTREAM*)handle;
	*data = mstream->buffer;
	*length = mstream->hw;
	return 0;
}

static void tiffUnmapCallback(thandle_t handle, tdata_t data, toff_t length)
{
	return;
}
/*!
 * \brief   fopenTiffMemstream()
 *
 * \param[in]    filename    for error output; can be ""
 * \param[in]    operation   "w" for write, "r" for read
 * \param[out]   pdata       written data
 * \param[out]   pdatasize   size of written data
 * \return  tiff data structure, opened for write to memory
 *
 * <pre>
 * Notes:
 *      (1) This wraps up a number of callbacks for either:
 *            * reading from tiff in memory buffer --> pix
 *            * writing from pix --> tiff in memory buffer
 *      (2) After use, the memstream is automatically destroyed when
 *          TIFFClose() is called.  TIFFCleanup() doesn't free the memstream.
 *      (3) This does not work in append mode, and in write mode it
 *          does not append.
 * </pre>
 */
static TIFF * fopenTiffMemstream(const char * filename, const char * operation, uint8 ** pdata, size_t * pdatasize)
{
	PROCNAME(__FUNCTION__);
	L_MEMSTREAM  * mstream;
	TIFF         * tif;
	if(!filename)
		return (TIFF*)ERROR_PTR("filename not defined", procName, NULL);
	if(!operation)
		return (TIFF*)ERROR_PTR("operation not defined", procName, NULL);
	if(!pdata)
		return (TIFF*)ERROR_PTR("&data not defined", procName, NULL);
	if(!pdatasize)
		return (TIFF*)ERROR_PTR("&datasize not defined", procName, NULL);
	if(strcmp(operation, "r") && strcmp(operation, "w"))
		return (TIFF*)ERROR_PTR("op not 'r' or 'w'", procName, NULL);
	if(sstreq(operation, "r"))
		mstream = memstreamCreateForRead(*pdata, *pdatasize);
	else
		mstream = memstreamCreateForWrite(pdata, pdatasize);
	TIFFSetWarningHandler(NULL); /* disable warnings */
	TIFFSetErrorHandler(NULL); /* disable error messages */
	tif = TIFFClientOpen(filename, operation, (thandle_t)mstream, tiffReadCallback, tiffWriteCallback, tiffSeekCallback, tiffCloseCallback, tiffSizeCallback, tiffMapCallback, tiffUnmapCallback);
	if(!tif)
		SAlloc::F(mstream);
	return tif;
}
/*!
 * \brief   pixReadMemTiff()
 *
 * \param[in]    cdata    const; tiff-encoded
 * \param[in]    size     size of cdata
 * \param[in]    n        page image number: 0-based
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is a version of pixReadTiff(), where the data is read
 *          from a memory buffer and uncompressed.
 *      (2) Use TIFFClose(); TIFFCleanup() doesn't free internal memstream.
 *      (3) No warning messages on failure, because of how multi-page
 *          TIFF reading works. You are supposed to keep trying until
 *          it stops working.
 *      (4) Tiff directory overhead is linear in the input page number.
 *          If reading many images, use pixReadMemFromMultipageTiff().
 * </pre>
 */
PIX * pixReadMemTiff(const uint8  * cdata,
    size_t size,
    int32 n)
{
	uint8  * data;
	int32 i;
	PIX * pix;
	TIFF     * tif;

	PROCNAME(__FUNCTION__);

	if(!cdata)
		return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

	data = (uint8 *)cdata; /* we're really not going to change this */
	if((tif = fopenTiffMemstream("tifferror", "r", &data, &size)) == NULL)
		return (PIX *)ERROR_PTR("tiff stream not opened", procName, NULL);

	pix = NULL;
	for(i = 0;; i++) {
		if(i == n) {
			if((pix = pixReadFromTiffStream(tif)) == NULL) {
				TIFFClose(tif);
				return NULL;
			}
			pixSetInputFormat(pix, IFF_TIFF);
			break;
		}
		if(TIFFReadDirectory(tif) == 0)
			break;
		if(i == ManyPagesInTiffFile + 1) {
			L_WARNING("big file: more than %d pages\n", procName,
			    ManyPagesInTiffFile);
		}
	}

	TIFFClose(tif);
	return pix;
}

/*!
 * \brief   pixReadMemFromMultipageTiff()
 *
 * \param[in]    cdata      const; tiff-encoded
 * \param[in]    size       size of cdata
 * \param[in,out]  poffset  set offset to 0 for first image
 * \return  pix, or NULL on error or if previous call returned the last image
 *
 * <pre>
 * Notes:
 *      (1) This is a read-from-memory version of pixReadFromMultipageTiff().
 *          See that function for usage.
 *      (2) If reading sequentially from the tiff data, this is more
 *          efficient than pixReadMemTiff(), which has an overhead
 *          proportional to the image index n.
 *      (3) Example usage for reading all the images:
 *            size_t offset = 0;
 *            do {
 *                Pix *pix = pixReadMemFromMultipageTiff(data, size, &offset);
 *                // do something with pix
 *            } while (offset != 0);
 * </pre>
 */
PIX * pixReadMemFromMultipageTiff(const uint8  * cdata,
    size_t size,
    size_t         * poffset)
{
	uint8  * data;
	int32 retval;
	size_t offset;
	PIX * pix;
	TIFF     * tif;

	PROCNAME(__FUNCTION__);

	if(!cdata)
		return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);
	if(!poffset)
		return (PIX *)ERROR_PTR("&offset not defined", procName, NULL);

	data = (uint8 *)cdata; /* we're really not going to change this */
	if((tif = fopenTiffMemstream("tifferror", "r", &data, &size)) == NULL)
		return (PIX *)ERROR_PTR("tiff stream not opened", procName, NULL);

	/* Set ptrs in the TIFF to the beginning of the image */
	offset = *poffset;
	retval = (offset == 0) ? TIFFSetDirectory(tif, 0)
	    : TIFFSetSubDirectory(tif, offset);
	if(retval == 0) {
		TIFFClose(tif);
		return NULL;
	}

	if((pix = pixReadFromTiffStream(tif)) == NULL) {
		TIFFClose(tif);
		return NULL;
	}

	/* Advance to the next image and return the new offset */
	TIFFReadDirectory(tif);
	*poffset = TIFFCurrentDirOffset(tif);
	TIFFClose(tif);
	return pix;
}

/*!
 * \brief   pixaReadMemMultipageTiff()
 *
 * \param[in]    data    const; multiple pages; tiff-encoded
 * \param[in]    size    size of cdata
 * \return  pixa, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This is an O(n) read-from-memory version of pixaReadMultipageTiff().
 * </pre>
 */
PIXA * pixaReadMemMultipageTiff(const uint8  * data,
    size_t size)
{
	size_t offset;
	PIX    * pix;
	PIXA   * pixa;

	PROCNAME(__FUNCTION__);

	if(!data)
		return (PIXA*)ERROR_PTR("data not defined", procName, NULL);

	offset = 0;
	pixa = pixaCreate(0);
	do {
		pix = pixReadMemFromMultipageTiff(data, size, &offset);
		pixaAddPix(pixa, pix, L_INSERT);
	} while(offset != 0);
	return pixa;
}

/*!
 * \brief   pixaWriteMemMultipageTiff()
 *
 * \param[out]    pdata   const; tiff-encoded
 * \param[out]    psize   size of data
 * \param[in]     pixa    any depth; colormap will be removed
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) fopenTiffMemstream() does not work in append mode, so we
 *          must work-around with a temporary file.
 *      (2) Getting a file stream from
 *            open_memstream((char **)pdata, psize)
 *          does not work with the tiff directory.
 * </pre>
 */
l_ok pixaWriteMemMultipageTiff(uint8  ** pdata,
    size_t * psize,
    PIXA      * pixa)
{
	const char * modestr;
	int32 i, n;
	FILE        * fp;
	PIX         * pix1;

	PROCNAME(__FUNCTION__);

	if(pdata) *pdata = NULL;
	if(!pdata)
		return ERROR_INT("pdata not defined", procName, 1);
	if(!pixa)
		return ERROR_INT("pixa not defined", procName, 1);

#ifdef _WIN32
	if((fp = fopenWriteWinTempfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
#else
	if((fp = tmpfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
#endif  /* _WIN32 */

	n = pixaGetCount(pixa);
	for(i = 0; i < n; i++) {
		modestr = (i == 0) ? "w" : "a";
		pix1 = pixaGetPix(pixa, i, L_CLONE);
		if(pixGetDepth(pix1) == 1)
			pixWriteStreamTiffWA(fp, pix1, IFF_TIFF_G4, modestr);
		else
			pixWriteStreamTiffWA(fp, pix1, IFF_TIFF_ZIP, modestr);
		pixDestroy(&pix1);
	}

	rewind(fp);
	*pdata = l_binaryReadStream(fp, psize);
	fclose(fp);
	return 0;
}

/*!
 * \brief   pixWriteMemTiff()
 *
 * \param[out]   pdata     data of tiff compressed image
 * \param[out]   psize     size of returned data
 * \param[in]    pix
 * \param[in]    comptype  IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                         IFF_TIFF_G3, IFF_TIFF_G4,
 *                         IFF_TIFF_LZW, IFF_TIFF_ZIP, IFF_TIFF_JPEG
 * \return  0 if OK, 1 on error
 *
 *  Usage:
 *      1) See pixWriteTiff(.  This version writes to
 *          memory instead of to a file.
 */
l_ok pixWriteMemTiff(uint8  ** pdata,
    size_t * psize,
    PIX * pix,
    int32 comptype)
{
	return pixWriteMemTiffCustom(pdata, psize, pix, comptype,
		   NULL, NULL, NULL, NULL);
}

/*!
 * \brief   pixWriteMemTiffCustom()
 *
 * \param[out]   pdata     data of tiff compressed image
 * \param[out]   psize     size of returned data
 * \param[in]    pix
 * \param[in]    comptype  IFF_TIFF, IFF_TIFF_RLE, IFF_TIFF_PACKBITS,
 *                         IFF_TIFF_G3, IFF_TIFF_G4,
 *                         IFF_TIFF_LZW, IFF_TIFF_ZIP, IFF_TIFF_JPEG
 * \param[in]    natags    [optional] NUMA of custom tiff tags
 * \param[in]    savals    [optional] SARRAY of values
 * \param[in]    satypes   [optional] SARRAY of types
 * \param[in]    nasizes   [optional] NUMA of sizes
 * \return  0 if OK, 1 on error
 *
 *  Usage:
 *      1) See pixWriteTiffCustom(.  This version writes to
 *          memory instead of to a file.
 *      2) Use TIFFClose(); TIFFCleanup( doesn't free internal memstream.
 */
l_ok pixWriteMemTiffCustom(uint8  ** pdata,
    size_t * psize,
    PIX * pix,
    int32 comptype,
    NUMA * natags,
    SARRAY    * savals,
    SARRAY    * satypes,
    NUMA * nasizes)
{
	int32 ret;
	TIFF    * tif;

	PROCNAME(__FUNCTION__);

	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	if(!pix)
		return ERROR_INT("&pix not defined", procName, 1);
	if(pixGetDepth(pix) != 1 && comptype != IFF_TIFF &&
	    comptype != IFF_TIFF_LZW && comptype != IFF_TIFF_ZIP &&
	    comptype != IFF_TIFF_JPEG) {
		L_WARNING("invalid compression type for bpp > 1\n", procName);
		comptype = IFF_TIFF_ZIP;
	}

	if((tif = fopenTiffMemstream("tifferror", "w", pdata, psize)) == NULL)
		return ERROR_INT("tiff stream not opened", procName, 1);
	ret = pixWriteToTiffStream(tif, pix, comptype, natags, savals,
		satypes, nasizes);

	TIFFClose(tif);
	return ret;
}

/* --------------------------------------------*/
#endif  /* HAVE_LIBTIFF */
/* --------------------------------------------*/
