/*====================================================================*
   -  Copyright (C) 2001 Leptonica.  All rights reserved.
   -
   -  Redistribution and use in source and binary forms, with or without
   -  modification, are permitted provided that the following conditions
   -  are met:
   -  1. Redistributions of source code must retain the above copyright
   -     notice, this list of conditions and the following disclaimer.
   -  2. Redistributions in binary form must reproduce the above
   -     copyright notice, this list of conditions and the following
   -     disclaimer in the documentation and/or other materials
   -     provided with the distribution.
*====================================================================*/
/*!
 * \file gifio.c
 * <pre>
 *
 *    Reading gif
 *          PIX            *pixReadStreamGif()
 *          PIX            *pixReadMemGif()
 *          static int32  gifReadFunc()
 *          static PIX     *gifToPix()
 *
 *    Writing gif
 *          int32         pixWriteStreamGif()
 *          int32         pixWriteMemGif()
 *          static int32  gifWriteFunc()
 *          static int32  pixToGif()
 *
 *    The initial version of this module was generously contribued by
 *    Antony Dovgal.
 *
 *    The functions that read and write from pix to gif-compressed memory,
 *    using gif internal functions DGifOpen() and EGifOpen() that are
 *    available in 5.1 and later, were contributed by Tobias Peirick.
 *
 *    Version information:
 *
 *    (1) This supports the gif library, version 5.1 or later, for which
 *        gif read-from-mem and write-to-mem allow these operations
 *        without writing temporary files.
 *    (2) There has never been a gif stream interface.  For versions
 *        before 5.1, it was necessary to use a file descriptor, and to
 *        generate a file stream from the low-level descriptor.  With the
 *        memory interface in 5.1 that can be used on all platforms, it
 *        is no longer necessary to use any API code with file descriptors.
 *    (3) The public interface changed with 5.0 and with 5.1, and we
 *        no longer support 4.6.1 and 5.0.
 *    (4) Version 5.1.2 came out on Jan 7, 2016.  Leptonica cannot
 *        successfully read gif files that it writes with this version;
 *        DGifSlurp() gets an internal error from an uninitialized array
 *        and returns failure.  The problem was fixed in 5.1.3.
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------------------------------*/
#if  HAVE_LIBGIF  || HAVE_LIBUNGIF             /* defined in environ.h */
/* --------------------------------------------------------------------*/

#include "gif_lib.h"

/* Interface that enables low-level GIF support for reading from memory */
static PIX * gifToPix(GifFileType * gif);
/* Interface that enables low-level GIF support for writing to memory */
static int32 pixToGif(PIX * pix, GifFileType * gif);

/*! For in-memory decoding of GIF; 5.1+ */
typedef struct GifReadBuffer {
	size_t size;           /*!< size of buffer                           */
	size_t pos;            /*!< position relative to beginning of buffer */
	const uint8 * cdata; /*!< data in the buffer                       */
} GifReadBuffer;

/*! Low-level callback for in-memory decoding */
static int   gifReadFunc(GifFileType * gif, uint8 * dest, int bytesToRead);
/*! Low-level callback for in-memory encoding */
static int   gifWriteFunc(GifFileType * gif, const uint8 * src, int bytesToWrite);

/*---------------------------------------------------------------------*
*                            Reading gif                              *
*---------------------------------------------------------------------*/
/*!
 * \brief   pixReadStreamGif()
 *
 * \param[in]  fp   file stream opened for reading
 * \return  pix, or NULL on error
 */
PIX * pixReadStreamGif(FILE * fp)
{
	uint8  * filedata;
	size_t filesize;
	PIX * pix;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return (PIX *)ERROR_PTR("fp not defined", procName, NULL);

	/* Read data into memory from file */
	rewind(fp);
	if((filedata = l_binaryReadStream(fp, &filesize)) == NULL)
		return (PIX *)ERROR_PTR("filedata not read", procName, NULL);

	/* Uncompress from memory */
	pix = pixReadMemGif(filedata, filesize);
	SAlloc::F(filedata);
	if(!pix)
		L_ERROR("failed to read gif from file data\n", procName);
	return pix;
}

/*!
 * \brief   pixReadMemGif()
 *
 * \param[in]  cdata    const; gif-encoded
 * \param[in]  size     bytes data
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *     (1) For libgif version >= 5.1, this uses the DGifOpen() buffer
 *         interface.  No temp files are required.
 *     (2) For libgif version < 5.1, it was necessary to write the compressed
 *         data to file and read it back, and we couldn't use the GNU
 *         runtime extension fmemopen() because libgif doesn't have a file
 *         stream interface.
 * </pre>
 */
PIX * pixReadMemGif(const uint8  * cdata,
    size_t size)
{
	GifFileType   * gif;
	GifReadBuffer buffer;

	PROCNAME(__FUNCTION__);

	/* 5.1+ and not 5.1.2 */
#if (GIFLIB_MAJOR < 5 || (GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 0))
	L_ERROR("Require giflib-5.1 or later\n", procName);
	return NULL;
#endif  /* < 5.1 */
#if GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 1 && GIFLIB_RELEASE == 2  /* 5.1.2 */
	L_ERROR("Can't use giflib-5.1.2; suggest 5.1.3 or later\n", procName);
	return NULL;
#endif  /* 5.1.2 */

	if(!cdata)
		return (PIX *)ERROR_PTR("cdata not defined", procName, NULL);

	buffer.cdata = cdata;
	buffer.size = size;
	buffer.pos = 0;
	if((gif = DGifOpen((void*)&buffer, gifReadFunc, NULL)) == NULL)
		return (PIX *)ERROR_PTR("could not open gif stream from memory", procName, NULL);

	return gifToPix(gif);
}

static int gifReadFunc(GifFileType  * gif, uint8 * dest, int bytesToRead)
{
	GifReadBuffer  * buffer;
	int32 bytesRead;
	PROCNAME(__FUNCTION__);
	if((buffer = (GifReadBuffer*)gif->UserData) == NULL)
		return ERROR_INT("UserData not set", procName, -1);
	if(buffer->pos >= buffer->size || bytesToRead > buffer->size)
		return -1;
	bytesRead = (buffer->pos < buffer->size - bytesToRead) ? bytesToRead : buffer->size - buffer->pos;
	memcpy(dest, buffer->cdata + buffer->pos, bytesRead);
	buffer->pos += bytesRead;
	return bytesRead;
}

/*!
 * \brief   gifToPix()
 *
 * \param[in]  gif   opened gif stream
 * \return  pix, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This decodes the pix from the compressed gif stream and
 *          closes the stream.
 *      (2) It is static so that the stream is not exposed to clients.
 * </pre>
 */
static PIX * gifToPix(GifFileType  * gif)
{
	int32 wpl, i, j, w, h, d, cindex, ncolors, valid, nimages;
	int32 rval, gval, bval;
	uint32        * data, * line;
	PIX             * pixd;
	PIXCMAP         * cmap;
	ColorMapObject  * gif_cmap;
	GifSavedImage si;
	int giferr;
	PROCNAME(__FUNCTION__);
	// Read all the data, but use only the first image found 
	if(DGifSlurp(gif) != GIF_OK) {
		DGifCloseFile2(gif, &giferr);
		return (PIX *)ERROR_PTR("failed to read GIF data", procName, NULL);
	}
	if(gif->SavedImages == NULL) {
		DGifCloseFile2(gif, &giferr);
		return (PIX *)ERROR_PTR("no images found in GIF", procName, NULL);
	}
	nimages = gif->ImageCount;
	if(nimages > 1)
		L_WARNING("There are %d images in the file; we only read the first\n", procName, nimages);
	si = gif->SavedImages[0];
	w = si.ImageDesc.Width;
	h = si.ImageDesc.Height;
	if(w <= 0 || h <= 0) {
		DGifCloseFile2(gif, &giferr);
		return (PIX *)ERROR_PTR("invalid image dimensions", procName, NULL);
	}
	if(si.RasterBits == NULL) {
		DGifCloseFile2(gif, &giferr);
		return (PIX *)ERROR_PTR("no raster data in GIF", procName, NULL);
	}

	if(si.ImageDesc.ColorMap) {
		// private cmap for this image 
		gif_cmap = si.ImageDesc.ColorMap;
	}
	else if(gif->SColorMap) {
		// global cmap for whole picture 
		gif_cmap = gif->SColorMap;
	}
	else {
		// don't know where to take cmap from 
		DGifCloseFile2(gif, &giferr);
		return (PIX *)ERROR_PTR("color map is missing", procName, NULL);
	}
	ncolors = gif_cmap->ColorCount;
	if(ncolors <= 0 || ncolors > 256) {
		DGifCloseFile2(gif, &giferr);
		return (PIX *)ERROR_PTR("ncolors is invalid", procName, NULL);
	}
	if(ncolors <= 2)
		d = 1;
	else if(ncolors <= 4)
		d = 2;
	else if(ncolors <= 16)
		d = 4;
	else /* [17 ... 256] */
		d = 8;
	cmap = pixcmapCreate(d);
	for(cindex = 0; cindex < ncolors; cindex++) {
		rval = gif_cmap->Colors[cindex].R;
		gval = gif_cmap->Colors[cindex].G;
		bval = gif_cmap->Colors[cindex].B;
		pixcmapAddColor(cmap, rval, gval, bval);
	}

	if((pixd = pixCreate(w, h, d)) == NULL) {
		DGifCloseFile2(gif, &giferr);
		pixcmapDestroy(&cmap);
		return (PIX *)ERROR_PTR("failed to allocate pixd", procName, NULL);
	}
	pixSetInputFormat(pixd, IFF_GIF);
	pixSetColormap(pixd, cmap);
	pixcmapIsValid(cmap, pixd, &valid);
	if(!valid) {
		DGifCloseFile2(gif, &giferr);
		pixDestroy(&pixd);
		pixcmapDestroy(&cmap);
		return (PIX *)ERROR_PTR("colormap is invalid", procName, NULL);
	}

	wpl = pixGetWpl(pixd);
	data = pixGetData(pixd);
	for(i = 0; i < h; i++) {
		line = data + i * wpl;
		if(d == 1) {
			for(j = 0; j < w; j++) {
				if(si.RasterBits[i * w + j])
					SET_DATA_BIT(line, j);
			}
		}
		else if(d == 2) {
			for(j = 0; j < w; j++)
				SET_DATA_DIBIT(line, j, si.RasterBits[i * w + j]);
		}
		else if(d == 4) {
			for(j = 0; j < w; j++)
				SET_DATA_QBIT(line, j, si.RasterBits[i * w + j]);
		}
		else { /* d == 8 */
			for(j = 0; j < w; j++)
				SET_DATA_BYTE(line, j, si.RasterBits[i * w + j]);
		}
	}
	/* Versions before 5.0 required un-interlacing to restore
	 * the raster lines to normal order if the image
	 * had been interlaced (for viewing in a browser):
	     if (gif->Image.Interlace) {
	         PIX *pixdi = pixUninterlaceGIF(pixd);
	         pixTransferAllData(pixd, &pixdi, 0, 0);
	     }
	 * This is no longer required. */
	DGifCloseFile2(gif, &giferr);
	return pixd;
}

/*---------------------------------------------------------------------*
*                            Writing gif                              *
*---------------------------------------------------------------------*/
/*!
 * \brief   pixWriteStreamGif()
 *
 * \param[in]  fp    file stream opened for writing
 * \param[in]  pix   1, 2, 4, 8, 16 or 32 bpp
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) All output gif have colormaps.  If the pix is 32 bpp rgb,
 *          this quantizes the colors and writes out 8 bpp.
 *          If the pix is 16 bpp grayscale, it converts to 8 bpp first.
 * </pre>
 */
l_ok pixWriteStreamGif(FILE * fp,
    PIX * pix)
{
	uint8  * filedata;
	size_t filebytes, nbytes;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return ERROR_INT("stream not open", procName, 1);
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);

	pixSetPadBits(pix, 0);
	if(pixWriteMemGif(&filedata, &filebytes, pix) != 0) {
		SAlloc::F(filedata);
		return ERROR_INT("failure to gif encode pix", procName, 1);
	}

	rewind(fp);
	nbytes = fwrite(filedata, 1, filebytes, fp);
	SAlloc::F(filedata);
	if(nbytes != filebytes)
		return ERROR_INT("write error", procName, 1);
	return 0;
}

/*!
 * \brief   pixWriteMemGif()
 *
 * \param[out]   pdata data of gif compressed image
 * \param[out]   psize size of returned data
 * \param[in]    pix
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) See comments in pixReadMemGif()
 * </pre>
 */
l_ok pixWriteMemGif(uint8  ** pdata, size_t * psize, PIX * pix)
{
	int giferr;
	int32 result;
	GifFileType  * gif;
	L_BBUFFER    * buffer;
	PROCNAME(__FUNCTION__);
	/* 5.1+ and not 5.1.2 */
#if (GIFLIB_MAJOR < 5 || (GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 0))
	L_ERROR("Require giflib-5.1 or later\n", procName);
	return 1;
#endif  /* < 5.1 */
#if GIFLIB_MAJOR == 5 && GIFLIB_MINOR == 1 && GIFLIB_RELEASE == 2  /* 5.1.2 */
	L_ERROR("Can't use giflib-5.1.2; suggest 5.1.3 or later\n", procName);
	return 1;
#endif  /* 5.1.2 */
	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	*pdata = NULL;
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	*psize = 0;
	if(!pix)
		return ERROR_INT("&pix not defined", procName, 1);
	if((buffer = bbufferCreate(NULL, 0)) == NULL)
		return ERROR_INT("failed to create buffer", procName, 1);
	if((gif = EGifOpen((void*)buffer, gifWriteFunc, NULL)) == NULL) {
		bbufferDestroy(&buffer);
		return ERROR_INT("failed to create GIF image handle", procName, 1);
	}
	result = pixToGif(pix, gif);
	EGifCloseFile(gif/*, &giferr*/);
	if(result == 0) {
		*pdata = bbufferDestroyAndSaveData(&buffer, psize);
	}
	else {
		bbufferDestroy(&buffer);
	}
	return result;
}

static int gifWriteFunc(GifFileType * gif, const uint8 * src, int bytesToWrite)
{
	PROCNAME(__FUNCTION__);
	L_BBUFFER  * buffer;
	if((buffer = (L_BBUFFER*)gif->UserData) == NULL)
		return ERROR_INT("UserData not set", procName, -1);
	if(bbufferRead(buffer, (uint8 *)src, bytesToWrite) == 0)
		return bytesToWrite;
	return 0;
}
/*!
 * \brief   pixToGif()
 *
 * \param[in]  pix    1, 2, 4, 8, 16 or 32 bpp
 * \param[in]  gif    opened gif stream
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This encodes the pix to the gif stream. The stream is not
 *          closed by this function.
 *      (2) It is static to make this function private.
 * </pre>
 */
static int32 pixToGif(PIX * pix, GifFileType  * gif)
{
	char * text;
	int32 wpl, i, j, w, h, d, ncolor, rval, gval, bval, valid;
	int32 gif_ncolor = 0;
	uint32 * data, * line;
	PIX * pixd;
	PIXCMAP * cmap;
	ColorMapObject  * gif_cmap;
	uint8 * gif_line;
	PROCNAME(__FUNCTION__);
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);
	if(!gif)
		return ERROR_INT("gif not defined", procName, 1);

	d = pixGetDepth(pix);
	if(d == 32) {
		pixd = pixConvertRGBToColormap(pix, 1);
	}
	else if(d > 1) {
		pixd = pixConvertTo8(pix, TRUE);
	}
	else { /* d == 1; make sure there's a colormap */
		pixd = pixClone(pix);
		if(!pixGetColormap(pixd)) {
			cmap = pixcmapCreate(1);
			pixcmapAddColor(cmap, 255, 255, 255);
			pixcmapAddColor(cmap, 0, 0, 0);
			pixSetColormap(pixd, cmap);
		}
	}

	if(!pixd)
		return ERROR_INT("failed to convert to colormapped pix", procName, 1);
	d = pixGetDepth(pixd);
	cmap = pixGetColormap(pixd);
	if(!cmap) {
		pixDestroy(&pixd);
		return ERROR_INT("cmap is missing", procName, 1);
	}
	pixcmapIsValid(cmap, pixd, &valid);
	if(!valid) {
		pixDestroy(&pixd);
		return ERROR_INT("colormap is not valid", procName, 1);
	}

	/* 'Round' the number of gif colors up to a power of 2 */
	ncolor = pixcmapGetCount(cmap);
	for(i = 0; i <= 8; i++) {
		if((1 << i) >= ncolor) {
			gif_ncolor = (1 << i);
			break;
		}
	}
	if(gif_ncolor < 1) {
		pixDestroy(&pixd);
		return ERROR_INT("number of colors is invalid", procName, 1);
	}

	/* Save the cmap colors in a gif_cmap */
	if((gif_cmap = GifMakeMapObject(gif_ncolor, NULL)) == NULL) {
		pixDestroy(&pixd);
		return ERROR_INT("failed to create GIF color map", procName, 1);
	}
	for(i = 0; i < gif_ncolor; i++) {
		rval = gval = bval = 0;
		if(ncolor > 0) {
			if(pixcmapGetColor(cmap, i, &rval, &gval, &bval) != 0) {
				pixDestroy(&pixd);
				GifFreeMapObject(gif_cmap);
				return ERROR_INT("failed to get color from color map",
					   procName, 1);
			}
			ncolor--;
		}
		gif_cmap->Colors[i].R = rval;
		gif_cmap->Colors[i].G = gval;
		gif_cmap->Colors[i].B = bval;
	}
	pixGetDimensions(pixd, &w, &h, NULL);
	if(EGifPutScreenDesc(gif, w, h, gif_cmap->BitsPerPixel, 0, gif_cmap) != GIF_OK) {
		pixDestroy(&pixd);
		GifFreeMapObject(gif_cmap);
		return ERROR_INT("failed to write screen description", procName, 1);
	}
	GifFreeMapObject(gif_cmap); /* not needed after this point */
	if(EGifPutImageDesc(gif, 0, 0, w, h, FALSE, NULL) != GIF_OK) {
		pixDestroy(&pixd);
		return ERROR_INT("failed to image screen description", procName, 1);
	}
	data = pixGetData(pixd);
	wpl = pixGetWpl(pixd);
	if(d != 1 && d != 2 && d != 4 && d != 8) {
		pixDestroy(&pixd);
		return ERROR_INT("image depth is not in {1, 2, 4, 8}", procName, 1);
	}
	if((gif_line = (uint8 *)SAlloc::C(sizeof(uint8), w)) == NULL) {
		pixDestroy(&pixd);
		return ERROR_INT("mem alloc fail for data line", procName, 1);
	}
	for(i = 0; i < h; i++) {
		line = data + i * wpl;
		/* Gif's way of setting the raster line up for compression */
		for(j = 0; j < w; j++) {
			switch(d)
			{
				case 8:
				    gif_line[j] = GET_DATA_BYTE(line, j);
				    break;
				case 4:
				    gif_line[j] = GET_DATA_QBIT(line, j);
				    break;
				case 2:
				    gif_line[j] = GET_DATA_DIBIT(line, j);
				    break;
				case 1:
				    gif_line[j] = GET_DATA_BIT(line, j);
				    break;
			}
		}

		/* Compress and save the line */
		if(EGifPutLine(gif, gif_line, w) != GIF_OK) {
			SAlloc::F(gif_line);
			pixDestroy(&pixd);
			return ERROR_INT("failed to write data line into GIF", procName, 1);
		}
	}

	/* Write a text comment.  This must be placed after writing the
	 * data (!!)  Note that because libgif does not provide a function
	 * for reading comments from file, you will need another way
	 * to read comments. */
	if((text = pixGetText(pix)) != NULL) {
		if(EGifPutComment(gif, text) != GIF_OK)
			L_WARNING("gif comment not written\n", procName);
	}

	SAlloc::F(gif_line);
	pixDestroy(&pixd);
	return 0;
}

#if 0
/*---------------------------------------------------------------------*
*         Removing interlacing (reference only; not used)             *
*---------------------------------------------------------------------*/
/* GIF supports 4-way interlacing by raster lines.
 * Before 5.0, it was necessary for leptonica to restore interlaced
 * data to normal raster order when reading to a pix. With 5.0,
 * the de-interlacing is done by the library read function.
 * It is here only as a reference. */
static const int32 InterlacedOffset[] = {0, 4, 2, 1};
static const int32 InterlacedJumps[] = {8, 8, 4, 2};

static PIX * pixUninterlaceGIF(PIX  * pixs)
{
	int32 w, h, d, wpl, j, k, srow, drow;
	uint32  * datas, * datad, * lines, * lined;
	PIX * pixd;

	PROCNAME(__FUNCTION__);

	if(!pixs)
		return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);

	pixGetDimensions(pixs, &w, &h, &d);
	wpl = pixGetWpl(pixs);
	pixd = pixCreateTemplate(pixs);
	datas = pixGetData(pixs);
	datad = pixGetData(pixd);
	for(k = 0, srow = 0; k < 4; k++) {
		for(drow = InterlacedOffset[k]; drow < h;
		    drow += InterlacedJumps[k], srow++) {
			lines = datas + srow * wpl;
			lined = datad + drow * wpl;
			for(j = 0; j < w; j++)
				memcpy(lined, lines, 4 * wpl);
		}
	}

	return pixd;
}

#endif

/* -----------------------------------------------------------------*/
#endif    /* HAVE_LIBGIF || HAVE_LIBUNGIF  */
