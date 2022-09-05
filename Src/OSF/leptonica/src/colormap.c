// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file colormap.c
 * <pre>
 *
 *      Colormap creation, copy, destruction, addition
 *           PIXCMAP *pixcmapCreate()
 *           PIXCMAP *pixcmapCreateRandom()
 *           PIXCMAP *pixcmapCreateLinear()
 *           PIXCMAP *pixcmapCopy()
 *           void        pixcmapDestroy()
 *           int32     pixcmapIsValid()
 *           int32     pixcmapAddColor()
 *           int32     pixcmapAddRGBA()
 *           int32     pixcmapAddNewColor()
 *           int32     pixcmapAddNearestColor()
 *           int32     pixcmapUsableColor()
 *           int32     pixcmapAddBlackOrWhite()
 *           int32     pixcmapSetBlackAndWhite()
 *           int32     pixcmapGetCount()
 *           int32     pixcmapGetDepth()
 *           int32     pixcmapGetMinDepth()
 *           int32     pixcmapGetFreeCount()
 *           int32     pixcmapClear()
 *
 *      Colormap random access and test
 *           int32     pixcmapGetColor()
 *           int32     pixcmapGetColor32()
 *           int32     pixcmapGetRGBA()
 *           int32     pixcmapGetRGBA32()
 *           int32     pixcmapResetColor()
 *           int32     pixcmapSetAlpha()
 *           int32     pixcmapGetIndex()
 *           int32     pixcmapHasColor()
 *           int32     pixcmapIsOpaque()
 *           int32     pixcmapNonOpaqueColorsInfo()
 *           int32     pixcmapIsBlackAndWhite()
 *           int32     pixcmapCountGrayColors()
 *           int32     pixcmapGetRankIntensity()
 *           int32     pixcmapGetNearestIndex()
 *           int32     pixcmapGetNearestGrayIndex()
 *           int32     pixcmapGetDistanceToColor()
 *           int32     pixcmapGetRangeValues()
 *
 *      Colormap conversion
 *           PIXCMAP *pixcmapGrayToFalseColor()
 *           PIXCMAP *pixcmapGrayToColor()
 *           PIXCMAP *pixcmapColorToGray()
 *           PIXCMAP *pixcmapConvertTo4()
 *           PIXCMAP *pixcmapConvertTo8()
 *
 *      Colormap I/O
 *           int32     pixcmapRead()
 *           int32     pixcmapReadStream()
 *           int32     pixcmapReadMem()
 *           int32     pixcmapWrite()
 *           int32     pixcmapWriteStream()
 *           int32     pixcmapWriteMem()
 *
 *      Extract colormap arrays and serialization
 *           int32     pixcmapToArrays()
 *           int32     pixcmapToRGBTable()
 *           int32     pixcmapSerializeToMemory()
 *           PIXCMAP *pixcmapDeserializeFromMemory()
 *           char *pixcmapConvertToHex()
 *
 *      Colormap transforms
 *           int32     pixcmapGammaTRC()
 *           int32     pixcmapContrastTRC()
 *           int32     pixcmapShiftIntensity()
 *           int32     pixcmapShiftByComponent()
 *
 *  Note:
 *      (1) colormaps in leptonica have a maximum of 256 entries.
 *      (2) nalloc, the allocated size of the palette array, is related
 *          to the depth d of the pixels by:
 *                 nalloc = 2^(d)
 *
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/*-------------------------------------------------------------*
*                Colormap creation and addition               *
*-------------------------------------------------------------*/
/*!
 * \brief   pixcmapCreate()
 *
 * \param[in]    depth    of pix, in bpp
 * \return  cmap, or NULL on error
 */
PIXCMAP * pixcmapCreate(int32 depth)
{
	RGBA_QUAD  * cta;
	PIXCMAP * cmap;

	PROCNAME(__FUNCTION__);

	if(depth != 1 && depth != 2 && depth !=4 && depth != 8)
		return (PIXCMAP*)ERROR_PTR("depth not in {1,2,4,8}", procName, NULL);

	cmap = (PIXCMAP*)SAlloc::C(1, sizeof(PIXCMAP));
	cmap->depth = depth;
	cmap->nalloc = 1 << depth;
	cta = (RGBA_QUAD*)SAlloc::C(cmap->nalloc, sizeof(RGBA_QUAD));
	cmap->array = cta;
	cmap->n = 0;
	return cmap;
}

/*!
 * \brief   pixcmapCreateRandom()
 *
 * \param[in]    depth      of pix, in bpp: 2, 4 or 8
 * \param[in]    hasblack   1 if the first color is black; 0 if no black
 * \param[in]    haswhite   1 if the last color is white; 0 if no white
 * \return  cmap, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This sets up a colormap with random colors,
 *          where the first color is optionally black, the last color
 *          is optionally white, and the remaining colors are
 *          chosen randomly.
 *      (2) The number of randomly chosen colors is:
 *               2^(depth) - haswhite - hasblack
 *      (3) Because rand() is seeded, it might disrupt otherwise
 *          deterministic results if also used elsewhere in a program.
 *      (4) rand() is not threadsafe, and will generate garbage if run
 *          on multiple threads at once -- though garbage is generally
 *          what you want from a random number generator!
 *      (5) Modern rand()s have equal randomness in low and high order
 *          bits, but older ones don't.  Here, we're just using rand()
 *          to choose colors for output.
 * </pre>
 */
PIXCMAP * pixcmapCreateRandom(int32 depth,
    int32 hasblack,
    int32 haswhite)
{
	int32 ncolors, i;
	int32 red[256], green[256], blue[256];
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(depth != 2 && depth != 4 && depth != 8)
		return (PIXCMAP*)ERROR_PTR("depth not in {2, 4, 8}", procName, NULL);
	if(hasblack != 0) hasblack = 1;
	if(haswhite != 0) haswhite = 1;

	cmap = pixcmapCreate(depth);
	ncolors = 1 << depth;
	if(hasblack) /* first color is optionally black */
		pixcmapAddColor(cmap, 0, 0, 0);
	for(i = hasblack; i < ncolors - haswhite; i++) {
		red[i] = (uint32)rand() & 0xff;
		green[i] = (uint32)rand() & 0xff;
		blue[i] = (uint32)rand() & 0xff;
		pixcmapAddColor(cmap, red[i], green[i], blue[i]);
	}
	if(haswhite) /* last color is optionally white */
		pixcmapAddColor(cmap, 255, 255, 255);

	return cmap;
}

/*!
 * \brief   pixcmapCreateLinear()
 *
 * \param[in]    d          depth of pix for this colormap; 1, 2, 4 or 8
 * \param[in]    nlevels    valid in range [2, 2^d]
 * \return  cmap, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) Colormap has equally spaced gray color values
 *          from black (0, 0, 0) to white (255, 255, 255).
 * </pre>
 */
PIXCMAP * pixcmapCreateLinear(int32 d,
    int32 nlevels)
{
	int32 maxlevels, i, val;
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(d != 1 && d != 2 && d !=4 && d != 8)
		return (PIXCMAP*)ERROR_PTR("d not in {1, 2, 4, 8}", procName, NULL);
	maxlevels = 1 << d;
	if(nlevels < 2 || nlevels > maxlevels)
		return (PIXCMAP*)ERROR_PTR("invalid nlevels", procName, NULL);

	cmap = pixcmapCreate(d);
	for(i = 0; i < nlevels; i++) {
		val = (255 * i) / (nlevels - 1);
		pixcmapAddColor(cmap, val, val, val);
	}
	return cmap;
}

/*!
 * \brief   pixcmapCopy()
 *
 * \param[in]    cmaps
 * \return  cmapd, or NULL on error
 */
PIXCMAP * pixcmapCopy(const PIXCMAP  * cmaps)
{
	int32 nbytes, valid;
	PIXCMAP  * cmapd;

	PROCNAME(__FUNCTION__);

	if(!cmaps)
		return (PIXCMAP*)ERROR_PTR("cmaps not defined", procName, NULL);
	pixcmapIsValid(cmaps, NULL, &valid);
	if(!valid)
		return (PIXCMAP*)ERROR_PTR("invalid cmap", procName, NULL);

	cmapd = (PIXCMAP*)SAlloc::C(1, sizeof(PIXCMAP));
	nbytes = cmaps->nalloc * sizeof(RGBA_QUAD);
	cmapd->array = (void*)SAlloc::C(1, nbytes);
	memcpy(cmapd->array, cmaps->array, cmaps->n * sizeof(RGBA_QUAD));
	cmapd->n = cmaps->n;
	cmapd->nalloc = cmaps->nalloc;
	cmapd->depth = cmaps->depth;
	return cmapd;
}

/*!
 * \brief   pixcmapDestroy()
 *
 * \param[in,out]   pcmap    set to null on return
 * \return  void
 */
void pixcmapDestroy(PIXCMAP  ** pcmap)
{
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(pcmap == NULL) {
		L_WARNING("ptr address is null!\n", procName);
		return;
	}

	if((cmap = *pcmap) == NULL)
		return;

	SAlloc::F(cmap->array);
	SAlloc::F(cmap);
	*pcmap = NULL;
}

/*!
 * \brief   pixcmapIsValid()
 *
 * \param[in]    cmap
 * \param[in]    pix        optional; can be NULL
 * \param[out]   pvalid     return 1 if valid; 0 if not
 * \return  0 if OK, 1 on error or if cmap is not valid
 *
 * <pre>
 * Notes:
 *      (1) If %pix is input, this will verify that pixel values cannot
 *          overflow the colormap.  This is a relatively expensive operation
 *          that may need to check all the pixel values.
 *      (2) If %pix is input, there must be at least one color in the
 *          colormap if it is to be valid with any pix, even if the
 *          pixels are all 0.
 * </pre>
 */
l_ok pixcmapIsValid(const PIXCMAP  * cmap,
    PIX            * pix,
    int32 * pvalid)
{
	int32 d, depth, nalloc, maxindex, maxcolors;

	PROCNAME(__FUNCTION__);

	if(!pvalid)
		return ERROR_INT("&valid not defined", procName, 1);
	*pvalid = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(!cmap->array)
		return ERROR_INT("cmap array not defined", procName, 1);
	d = cmap->depth;
	if(d != 1 && d != 2 && d != 4 && d != 8) {
		L_ERROR("invalid cmap depth: %d\n", procName, d);
		return 1;
	}
	nalloc = cmap->nalloc;
	if(nalloc != (1 << d)) {
		L_ERROR("invalid cmap nalloc = %d; d = %d\n", procName, nalloc, d);
		return 1;
	}
	if(cmap->n < 0 || cmap->n > nalloc) {
		L_ERROR("invalid cmap n: %d; nalloc = %d\n", procName, cmap->n, nalloc);
		return 1;
	}

	/* If a pix is given, it must have a depth no larger than 8 */
	if(pix) {
		depth = pixGetDepth(pix);
		if(depth > 8) {
			L_ERROR("pix depth %d > 8\n", procName, depth);
			return 1;
		}
		maxcolors = 1 << depth;
	}

	/* To prevent indexing overflow into the cmap, the pix depth
	 * must not exceed the cmap depth.  Do not require depth equality,
	 * because some functions such as median cut quantizers allow
	 * the cmap depth to be bigger than the pix depth. */
	if(pix && (depth > d)) {
		L_ERROR("(pix depth = %d) > (cmap depth = %d)\n", procName, depth, d);
		return 1;
	}
	if(pix && cmap->n < 1) {
		L_ERROR("cmap array is empty; invalid with any pix\n", procName);
		return 1;
	}

	/* Do not let the colormap have more colors than the pixels
	 * can address.  The png encoder considers this to be an
	 * "invalid palette length".  For example, for 1 bpp, the
	 * colormap may have a depth > 1, but it must not have more
	 * than 2 colors. */
	if(pix && (cmap->n > maxcolors)) {
		L_ERROR("cmap entries = %d > max colors for pix = %d\n", procName,
		    cmap->n, maxcolors);
		return 1;
	}

	/* Where the colormap or the pix may have been corrupted, and
	 * in particular when reading or writing image files, it should
	 * be verified that the image pixel values do not exceed the
	 * max indexing into the colormap array. */
	if(pix) {
		pixGetMaxColorIndex(pix, &maxindex);
		if(maxindex >= cmap->n) {
			L_ERROR("(max index = %d) >= (num colors = %d)\n", procName,
			    maxindex, cmap->n);
			return 1;
		}
	}

	*pvalid = 1;
	return 0;
}

/*!
 * \brief   pixcmapAddColor()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval   colormap entry to be added; each number
 *                                  is in range [0, ... 255]
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This always adds the color if there is room.
 *      (2) The alpha component is 255 (opaque)
 * </pre>
 */
l_ok pixcmapAddColor(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval)
{
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(cmap->n >= cmap->nalloc)
		return ERROR_INT("no free color entries", procName, 1);

	cta = (RGBA_QUAD*)cmap->array;
	cta[cmap->n].red = rval;
	cta[cmap->n].green = gval;
	cta[cmap->n].blue = bval;
	cta[cmap->n].alpha = 255;
	cmap->n++;
	return 0;
}

/*!
 * \brief   pixcmapAddRGBA()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval, aval   colormap entry to be added;
 *                                        each number is in range [0, ... 255]
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This always adds the color if there is room.
 * </pre>
 */
l_ok pixcmapAddRGBA(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 aval)
{
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(cmap->n >= cmap->nalloc)
		return ERROR_INT("no free color entries", procName, 1);

	cta = (RGBA_QUAD*)cmap->array;
	cta[cmap->n].red = rval;
	cta[cmap->n].green = gval;
	cta[cmap->n].blue = bval;
	cta[cmap->n].alpha = aval;
	cmap->n++;
	return 0;
}

/*!
 * \brief   pixcmapAddNewColor()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval    colormap entry to be added; each number
 *                                   is in range [0, ... 255]
 * \param[out]   pindex              index of color
 * \return  0 if OK, 1 on error; 2 if unable to add color
 *
 * <pre>
 * Notes:
 *      (1) This only adds color if not already there.
 *      (2) The alpha component is 255 (opaque)
 *      (3) This returns the index of the new (or existing) color.
 *      (4) Returns 2 with a warning if unable to add this color;
 *          the caller should check the return value.
 * </pre>
 */
l_ok pixcmapAddNewColor(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 * pindex)
{
	PROCNAME(__FUNCTION__);

	if(!pindex)
		return ERROR_INT("&index not defined", procName, 1);
	*pindex = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	/* Check if the color is already present. */
	if(!pixcmapGetIndex(cmap, rval, gval, bval, pindex)) /* found */
		return 0;

	/* We need to add the color.  Is there room? */
	if(cmap->n >= cmap->nalloc) {
		L_WARNING("no free color entries\n", procName);
		return 2;
	}

	/* There's room.  Add it. */
	pixcmapAddColor(cmap, rval, gval, bval);
	*pindex = pixcmapGetCount(cmap) - 1;
	return 0;
}

/*!
 * \brief   pixcmapAddNearestColor()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval    colormap entry to be added; each number
 *                                   is in range [0, ... 255]
 * \param[out]   pindex              index of color
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This only adds color if not already there.
 *      (2) The alpha component is 255 (opaque)
 *      (3) If it's not in the colormap and there is no room to add
 *          another color, this returns the index of the nearest color.
 * </pre>
 */
l_ok pixcmapAddNearestColor(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 * pindex)
{
	PROCNAME(__FUNCTION__);

	if(!pindex)
		return ERROR_INT("&index not defined", procName, 1);
	*pindex = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	/* Check if the color is already present. */
	if(!pixcmapGetIndex(cmap, rval, gval, bval, pindex)) /* found */
		return 0;

	/* We need to add the color.  Is there room? */
	if(cmap->n < cmap->nalloc) {
		pixcmapAddColor(cmap, rval, gval, bval);
		*pindex = pixcmapGetCount(cmap) - 1;
		return 0;
	}

	/* There's no room.  Return the index of the nearest color */
	pixcmapGetNearestIndex(cmap, rval, gval, bval, pindex);
	return 0;
}

/*!
 * \brief   pixcmapUsableColor()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval   colormap entry to be added; each number
 *                                  is in range [0, ... 255]
 * \param[out]   pusable            1 if usable; 0 if not
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This checks if the color already exists or if there is
 *          room to add it.  It makes no change in the colormap.
 * </pre>
 */
l_ok pixcmapUsableColor(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 * pusable)
{
	int32 index;

	PROCNAME(__FUNCTION__);

	if(!pusable)
		return ERROR_INT("&usable not defined", procName, 1);
	*pusable = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	/* Is there room to add it? */
	if(cmap->n < cmap->nalloc) {
		*pusable = 1;
		return 0;
	}

	/* No room; check if the color is already present. */
	if(!pixcmapGetIndex(cmap, rval, gval, bval, &index)) /* found */
		*pusable = 1;
	return 0;
}

/*!
 * \brief   pixcmapAddBlackOrWhite()
 *
 * \param[in]    cmap
 * \param[in]    color    0 for black, 1 for white
 * \param[out]   pindex   [optional] index of color; can be null
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This only adds color if not already there.
 *      (2) The alpha component is 255 (opaque)
 *      (3) This sets index to the requested color.
 *      (4) If there is no room in the colormap, returns the index
 *          of the closest color.
 * </pre>
 */
l_ok pixcmapAddBlackOrWhite(PIXCMAP  * cmap,
    int32 color,
    int32 * pindex)
{
	int32 index;

	PROCNAME(__FUNCTION__);

	if(pindex) *pindex = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	if(color == 0) { /* black */
		if(pixcmapGetFreeCount(cmap) > 0)
			pixcmapAddNewColor(cmap, 0, 0, 0, &index);
		else
			pixcmapGetRankIntensity(cmap, 0.0, &index);
	}
	else { /* white */
		if(pixcmapGetFreeCount(cmap) > 0)
			pixcmapAddNewColor(cmap, 255, 255, 255, &index);
		else
			pixcmapGetRankIntensity(cmap, 1.0, &index);
	}

	if(pindex)
		*pindex = index;
	return 0;
}

/*!
 * \brief   pixcmapSetBlackAndWhite()
 *
 * \param[in]    cmap
 * \param[in]    setblack   0 for no operation; 1 to set darkest color to black
 * \param[in]    setwhite   0 for no operation; 1 to set lightest color to white
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapSetBlackAndWhite(PIXCMAP  * cmap,
    int32 setblack,
    int32 setwhite)
{
	int32 index;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	if(setblack) {
		pixcmapGetRankIntensity(cmap, 0.0, &index);
		pixcmapResetColor(cmap, index, 0, 0, 0);
	}
	if(setwhite) {
		pixcmapGetRankIntensity(cmap, 1.0, &index);
		pixcmapResetColor(cmap, index, 255, 255, 255);
	}
	return 0;
}

/*!
 * \brief   pixcmapGetCount()
 *
 * \param[in]    cmap
 * \return  count, or 0 on error
 */
int32 pixcmapGetCount(const PIXCMAP  * cmap)
{
	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 0);
	return cmap->n;
}

/*!
 * \brief   pixcmapGetFreeCount()
 *
 * \param[in]    cmap
 * \return  free entries, or 0 on error
 */
int32 pixcmapGetFreeCount(PIXCMAP  * cmap)
{
	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 0);
	return (cmap->nalloc - cmap->n);
}

/*!
 * \brief   pixcmapGetDepth()
 *
 * \param[in]    cmap
 * \return  depth, or 0 on error
 */
int32 pixcmapGetDepth(PIXCMAP  * cmap)
{
	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 0);
	return cmap->depth;
}

/*!
 * \brief   pixcmapGetMinDepth()
 *
 * \param[in]    cmap
 * \param[out]   pmindepth    minimum depth to support the colormap
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) On error, &mindepth is returned as 0.
 * </pre>
 */
l_ok pixcmapGetMinDepth(PIXCMAP  * cmap,
    int32 * pmindepth)
{
	int32 ncolors;

	PROCNAME(__FUNCTION__);

	if(!pmindepth)
		return ERROR_INT("&mindepth not defined", procName, 1);
	*pmindepth = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	if(ncolors <= 4)
		*pmindepth = 2;
	else if(ncolors <= 16)
		*pmindepth = 4;
	else /* ncolors > 16 */
		*pmindepth = 8;
	return 0;
}

/*!
 * \brief   pixcmapClear()
 *
 * \param[in]    cmap
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This removes the colors by setting the count to 0.
 * </pre>
 */
l_ok pixcmapClear(PIXCMAP  * cmap)
{
	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	cmap->n = 0;
	return 0;
}

/*-------------------------------------------------------------*
*                      Colormap random access                 *
*-------------------------------------------------------------*/
/*!
 * \brief   pixcmapGetColor()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[out]   prval, pgval, pbval    each color value
 * \return  0 if OK, 1 if not accessible caller should check
 */
l_ok pixcmapGetColor(PIXCMAP  * cmap,
    int32 index,
    int32 * prval,
    int32 * pgval,
    int32 * pbval)
{
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!prval || !pgval || !pbval)
		return ERROR_INT("&rval, &gval, &bval not all defined", procName, 1);
	*prval = *pgval = *pbval = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(index < 0 || index >= cmap->n)
		return ERROR_INT("index out of bounds", procName, 1);

	cta = (RGBA_QUAD*)cmap->array;
	*prval = cta[index].red;
	*pgval = cta[index].green;
	*pbval = cta[index].blue;
	return 0;
}

/*!
 * \brief   pixcmapGetColor32()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[out]   pval32     32-bit rgb color value
 * \return  0 if OK, 1 if not accessible caller should check
 *
 * <pre>
 * Notes:
 *      (1) The returned alpha channel value is 255.
 * </pre>
 */
l_ok pixcmapGetColor32(PIXCMAP   * cmap,
    int32 index,
    uint32 * pval32)
{
	int32 rval, gval, bval;

	PROCNAME(__FUNCTION__);

	if(!pval32)
		return ERROR_INT("&val32 not defined", procName, 1);
	*pval32 = 0;

	if(pixcmapGetColor(cmap, index, &rval, &gval, &bval) != 0)
		return ERROR_INT("rgb values not found", procName, 1);
	composeRGBAPixel(rval, gval, bval, 255, pval32);
	return 0;
}

/*!
 * \brief   pixcmapGetRGBA()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[out]   prval, pgval, pbval, paval    each color value
 * \return  0 if OK, 1 if not accessible caller should check
 */
l_ok pixcmapGetRGBA(PIXCMAP  * cmap, int32 index, int32 * prval, int32 * pgval, int32 * pbval, int32 * paval)
{
	RGBA_QUAD  * cta;
	PROCNAME(__FUNCTION__);
	if(!prval || !pgval || !pbval || !paval)
		return ERROR_INT("&rval, &gval, &bval, &aval not all defined", procName, 1);
	*prval = *pgval = *pbval = *paval = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(index < 0 || index >= cmap->n)
		return ERROR_INT("index out of bounds", procName, 1);
	cta = (RGBA_QUAD*)cmap->array;
	*prval = cta[index].red;
	*pgval = cta[index].green;
	*pbval = cta[index].blue;
	*paval = cta[index].alpha;
	return 0;
}

/*!
 * \brief   pixcmapGetRGBA32()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[out]   pval32     32-bit rgba color value
 * \return  0 if OK, 1 if not accessible caller should check
 */
l_ok pixcmapGetRGBA32(PIXCMAP   * cmap,
    int32 index,
    uint32 * pval32)
{
	int32 rval, gval, bval, aval;

	PROCNAME(__FUNCTION__);

	if(!pval32)
		return ERROR_INT("&val32 not defined", procName, 1);
	*pval32 = 0;

	if(pixcmapGetRGBA(cmap, index, &rval, &gval, &bval, &aval) != 0)
		return ERROR_INT("rgba values not found", procName, 1);
	composeRGBAPixel(rval, gval, bval, aval, pval32);
	return 0;
}

/*!
 * \brief   pixcmapResetColor()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[in]    rval, gval, bval    colormap entry to be reset; each number
 *                                   is in range [0, ... 255]
 * \return  0 if OK, 1 if not accessible caller should check
 *
 * <pre>
 * Notes:
 *      (1) This resets sets the color of an entry that has already
 *          been set and included in the count of colors.
 *      (2) The alpha component is 255 (opaque)
 * </pre>
 */
l_ok pixcmapResetColor(PIXCMAP  * cmap,
    int32 index,
    int32 rval,
    int32 gval,
    int32 bval)
{
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(index < 0 || index >= cmap->n)
		return ERROR_INT("index out of bounds", procName, 1);

	cta = (RGBA_QUAD*)cmap->array;
	cta[index].red = rval;
	cta[index].green = gval;
	cta[index].blue = bval;
	cta[index].alpha = 255;
	return 0;
}

/*!
 * \brief   pixcmapSetAlpha()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[in]    aval     in range [0, ... 255]
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This modifies the transparency of one entry in a colormap.
 *          The alpha component by default is 255 (opaque).
 *          This is used when extracting the colormap from a PNG file
 *          without decoding the image.
 * </pre>
 */
l_ok pixcmapSetAlpha(PIXCMAP  * cmap,
    int32 index,
    int32 aval)
{
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(index < 0 || index >= cmap->n)
		return ERROR_INT("index out of bounds", procName, 1);

	cta = (RGBA_QUAD*)cmap->array;
	cta[index].alpha = aval;
	return 0;
}

/*!
 * \brief   pixcmapGetIndex()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval   colormap colors to search for; each number
 *                                  is in range [0, ... 255]
 * \param[out]   pindex             value of index found
 * \return  0 if found, 1 if not found caller must check
 */
int32 pixcmapGetIndex(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 * pindex)
{
	int32 n, i;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!pindex)
		return ERROR_INT("&index not defined", procName, 1);
	*pindex = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	n = pixcmapGetCount(cmap);

	cta = (RGBA_QUAD*)cmap->array;
	for(i = 0; i < n; i++) {
		if(rval == cta[i].red &&
		    gval == cta[i].green &&
		    bval == cta[i].blue) {
			*pindex = i;
			return 0;
		}
	}
	return 1;
}

/*!
 * \brief   pixcmapHasColor()
 *
 * \param[in]    cmap
 * \param[out]   pcolor    TRUE if cmap has color; FALSE otherwise
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapHasColor(PIXCMAP  * cmap,
    int32 * pcolor)
{
	int32 n, i;
	int32 * rmap, * gmap, * bmap;

	PROCNAME(__FUNCTION__);

	if(!pcolor)
		return ERROR_INT("&color not defined", procName, 1);
	*pcolor = FALSE;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	if(pixcmapToArrays(cmap, &rmap, &gmap, &bmap, NULL))
		return ERROR_INT("colormap arrays not made", procName, 1);
	n = pixcmapGetCount(cmap);
	for(i = 0; i < n; i++) {
		if((rmap[i] != gmap[i]) || (rmap[i] != bmap[i])) {
			*pcolor = TRUE;
			break;
		}
	}

	SAlloc::F(rmap);
	SAlloc::F(gmap);
	SAlloc::F(bmap);
	return 0;
}

/*!
 * \brief   pixcmapIsOpaque()
 *
 * \param[in]    cmap
 * \param[out]   popaque     TRUE if fully opaque: all entries are 255
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapIsOpaque(PIXCMAP  * cmap,
    int32 * popaque)
{
	int32 i, n;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!popaque)
		return ERROR_INT("&opaque not defined", procName, 1);
	*popaque = TRUE;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	n = pixcmapGetCount(cmap);
	cta = (RGBA_QUAD*)cmap->array;
	for(i = 0; i < n; i++) {
		if(cta[i].alpha != 255) {
			*popaque = FALSE;
			break;
		}
	}
	return 0;
}

/*!
 * \brief   pixcmapNonOpaqueColorsInfo()
 *
 * \param[in]    cmap
 * \param[out]   pntrans         [optional] number of transparent alpha
 *                                          entries; <= 256
 * \param[out]   pmax_trans      [optional] max index of transparent alpha
 * \param[out]   pmin_opaque     [optional] min index of opaque < 256
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is used, for clarity, when writing the png tRNS palette.
 *          According to the spec, http://www.w3.org/TR/PNG/#11tRNS,
 *          if the tRNS palette is of size ntrans, the palette uses the first
 *          ntrans alpha entries in the cmap, and the remaining alpha values
 *          are assumed to be 255 (opaque), regardless of cmap alpha value.
 *      (2) Ordinarily, the non-opaque colors come first in the cmap, so
 *               min_opaque > max_trans
 *          and
 *               ntrans = max_trans + 1 = min_opaque.
 *          But this does not happen in general.  In trans-2bpp-palette.png,
 *          for example, only the third of four entries is not opaque, so
 *               ntrans = 1
 *               max_trans = 2 (index is 0-based)
 *               min_opaque = 0
 *          The tRNS palette must extend to the third entry to cover the
 *          color with transparency: use 3 as the fourth arg to png_set_tRNS().
 *      (3) If all entries are opaque, max_trans = -1.
 *          If all entries are transparent, min_opaque = size of cmap.
 * </pre>
 */
l_ok pixcmapNonOpaqueColorsInfo(PIXCMAP  * cmap,
    int32 * pntrans,
    int32 * pmax_trans,
    int32 * pmin_opaque)
{
	int32 i, n, ntrans, max_trans, min_opaque, opaque_found;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(pntrans) *pntrans = 0;
	if(pmax_trans) *pmax_trans = -1;
	if(pmin_opaque) *pmin_opaque = 256;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	n = pixcmapGetCount(cmap);
	ntrans = 0;
	max_trans = -1;
	min_opaque = n;
	cta = (RGBA_QUAD*)cmap->array;
	opaque_found = FALSE;
	for(i = 0; i < n; i++) {
		if(cta[i].alpha != 255) {
			ntrans++;
			max_trans = i;
		}
		else if(opaque_found == FALSE) {
			opaque_found = TRUE;
			min_opaque = i;
		}
	}
	if(pntrans) *pntrans = ntrans;
	if(pmax_trans) *pmax_trans = max_trans;
	if(pmin_opaque) *pmin_opaque = min_opaque;
	return 0;
}

/*!
 * \brief   pixcmapIsBlackAndWhite()
 *
 * \param[in]    cmap
 * \param[out]   pblackwhite   TRUE if the cmap has only two colors:
 *                             black (0,0,0) and white (255,255,255)
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapIsBlackAndWhite(PIXCMAP  * cmap,
    int32 * pblackwhite)
{
	int32 val0, val1, hascolor;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!pblackwhite)
		return ERROR_INT("&blackwhite not defined", procName, 1);
	*pblackwhite = FALSE;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(pixcmapGetCount(cmap) != 2)
		return 0;

	pixcmapHasColor(cmap, &hascolor);
	if(hascolor) return 0;

	cta = (RGBA_QUAD*)cmap->array;
	val0 = cta[0].red;
	val1 = cta[1].red;
	if((val0 == 0 && val1 == 255) || (val0 == 255 && val1 == 0))
		*pblackwhite = TRUE;
	return 0;
}

/*!
 * \brief   pixcmapCountGrayColors()
 *
 * \param[in]    cmap
 * \param[out]   pngray     number of gray colors
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This counts the unique gray colors, including black and white.
 * </pre>
 */
l_ok pixcmapCountGrayColors(PIXCMAP  * cmap,
    int32 * pngray)
{
	int32 n, i, rval, gval, bval, count;
	int32 * array;

	PROCNAME(__FUNCTION__);

	if(!pngray)
		return ERROR_INT("&ngray not defined", procName, 1);
	*pngray = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	array = (int32*)SAlloc::C(256, sizeof(int32));
	n = pixcmapGetCount(cmap);
	count = 0;
	for(i = 0; i < n; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		if((rval == gval) && (rval == bval) && (array[rval] == 0)) {
			array[rval] = 1;
			count++;
		}
	}

	SAlloc::F(array);
	*pngray = count;
	return 0;
}

/*!
 * \brief   pixcmapGetRankIntensity()
 *
 * \param[in]    cmap
 * \param[in]    rankval   0.0 for darkest, 1.0 for lightest color
 * \param[out]   pindex    the index into the colormap that corresponds
 *                         to the rank intensity color
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapGetRankIntensity(PIXCMAP * cmap,
    float rankval,
    int32 * pindex)
{
	int32 n, i, rval, gval, bval, rankindex;
	NUMA * na, * nasort;

	PROCNAME(__FUNCTION__);

	if(!pindex)
		return ERROR_INT("&index not defined", procName, 1);
	*pindex = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(rankval < 0.0 || rankval > 1.0)
		return ERROR_INT("rankval not in [0.0 ... 1.0]", procName, 1);

	n = pixcmapGetCount(cmap);
	na = numaCreate(n);
	for(i = 0; i < n; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		numaAddNumber(na, rval + gval + bval);
	}
	nasort = numaGetSortIndex(na, L_SORT_INCREASING);
	rankindex = (int32)(rankval * (n - 1) + 0.5);
	numaGetIValue(nasort, rankindex, pindex);

	numaDestroy(&na);
	numaDestroy(&nasort);
	return 0;
}

/*!
 * \brief   pixcmapGetNearestIndex()
 *
 * \param[in]    cmap
 * \param[in]    rval, gval, bval   colormap colors to search for; each number
 *                                  is in range [0, ... 255]
 * \param[out]   pindex             the index of the nearest color
 * \return  0 if OK, 1 on error caller must check
 *
 * <pre>
 * Notes:
 *      (1) Returns the index of the exact color if possible, otherwise the
 *          index of the color closest to the target color.
 *      (2) Nearest color is that which is the least sum-of-squares distance
 *          from the target color.
 * </pre>
 */
l_ok pixcmapGetNearestIndex(PIXCMAP  * cmap,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 * pindex)
{
	int32 i, n, delta, dist, mindist;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!pindex)
		return ERROR_INT("&index not defined", procName, 1);
	*pindex = UNDEF;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	if((cta = (RGBA_QUAD*)cmap->array) == NULL)
		return ERROR_INT("cta not defined(!)", procName, 1);
	n = pixcmapGetCount(cmap);

	mindist = 3 * 255 * 255 + 1;
	for(i = 0; i < n; i++) {
		delta = cta[i].red - rval;
		dist = delta * delta;
		delta = cta[i].green - gval;
		dist += delta * delta;
		delta = cta[i].blue - bval;
		dist += delta * delta;
		if(dist < mindist) {
			*pindex = i;
			if(dist == 0)
				break;
			mindist = dist;
		}
	}

	return 0;
}

/*!
 * \brief   pixcmapGetNearestGrayIndex()
 *
 * \param[in]    cmap
 * \param[in]    val       gray value to search for; in range [0, ... 255]
 * \param[out]   pindex    the index of the nearest color
 * \return  0 if OK, 1 on error caller must check
 *
 * <pre>
 * Notes:
 *      (1) This should be used on gray colormaps.  It uses only the
 *          green value of the colormap.
 *      (2) Returns the index of the exact color if possible, otherwise the
 *          index of the color closest to the target color.
 * </pre>
 */
l_ok pixcmapGetNearestGrayIndex(PIXCMAP  * cmap,
    int32 val,
    int32 * pindex)
{
	int32 i, n, dist, mindist;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!pindex)
		return ERROR_INT("&index not defined", procName, 1);
	*pindex = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(val < 0 || val > 255)
		return ERROR_INT("val not in [0 ... 255]", procName, 1);

	if((cta = (RGBA_QUAD*)cmap->array) == NULL)
		return ERROR_INT("cta not defined(!)", procName, 1);
	n = pixcmapGetCount(cmap);

	mindist = 256;
	for(i = 0; i < n; i++) {
		dist = cta[i].green - val;
		dist = L_ABS(dist);
		if(dist < mindist) {
			*pindex = i;
			if(dist == 0)
				break;
			mindist = dist;
		}
	}

	return 0;
}

/*!
 * \brief   pixcmapGetDistanceToColor()
 *
 * \param[in]    cmap
 * \param[in]    index
 * \param[in]    rval,    gval, bval target color
 * \param[out]   pdist    the distance from the cmap entry to target
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Returns the L2 distance (squared) between the color at index i
 *          and the target color.
 * </pre>
 */
l_ok pixcmapGetDistanceToColor(PIXCMAP  * cmap,
    int32 index,
    int32 rval,
    int32 gval,
    int32 bval,
    int32 * pdist)
{
	int32 n, delta, dist;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!pdist)
		return ERROR_INT("&dist not defined", procName, 1);
	*pdist = UNDEF;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	n = pixcmapGetCount(cmap);
	if(index >= n)
		return ERROR_INT("invalid index", procName, 1);

	if((cta = (RGBA_QUAD*)cmap->array) == NULL)
		return ERROR_INT("cta not defined(!)", procName, 1);

	delta = cta[index].red - rval;
	dist = delta * delta;
	delta = cta[index].green - gval;
	dist += delta * delta;
	delta = cta[index].blue - bval;
	dist += delta * delta;
	*pdist = dist;

	return 0;
}

/*!
 * \brief   pixcmapGetRangeValues()
 *
 * \param[in]    cmap
 * \param[in]    select      L_SELECT_RED, L_SELECT_GREEN, L_SELECT_BLUE or
 *                           L_SELECT_AVERAGE
 * \param[out]   pminval     [optional] minimum value of component
 * \param[out]   pmaxval     [optional] maximum value of component
 * \param[out]   pminindex   [optional] index of minimum value
 * \param[out]   pmaxindex   [optional] index of maximum value
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Returns, for selected components (or the average), the
 *          the extreme values (min and/or max) and their indices
 *          that are found in the cmap.
 * </pre>
 */
l_ok pixcmapGetRangeValues(PIXCMAP  * cmap,
    int32 select,
    int32 * pminval,
    int32 * pmaxval,
    int32 * pminindex,
    int32 * pmaxindex)
{
	int32 i, n, imin, imax, minval, maxval, rval, gval, bval, aveval;

	PROCNAME(__FUNCTION__);

	if(pminval) *pminval = UNDEF;
	if(pmaxval) *pmaxval = UNDEF;
	if(pminindex) *pminindex = UNDEF;
	if(pmaxindex) *pmaxindex = UNDEF;
	if(!pminval && !pmaxval && !pminindex && !pmaxindex)
		return ERROR_INT("no result requested", procName, 1);
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	imin = UNDEF;
	imax = UNDEF;
	minval = 100000;
	maxval = -1;
	n = pixcmapGetCount(cmap);
	for(i = 0; i < n; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		if(select == L_SELECT_RED) {
			if(rval < minval) {
				minval = rval;
				imin = i;
			}
			if(rval > maxval) {
				maxval = rval;
				imax = i;
			}
		}
		else if(select == L_SELECT_GREEN) {
			if(gval < minval) {
				minval = gval;
				imin = i;
			}
			if(gval > maxval) {
				maxval = gval;
				imax = i;
			}
		}
		else if(select == L_SELECT_BLUE) {
			if(bval < minval) {
				minval = bval;
				imin = i;
			}
			if(bval > maxval) {
				maxval = bval;
				imax = i;
			}
		}
		else if(select == L_SELECT_AVERAGE) {
			aveval = (rval + gval + bval) / 3;
			if(aveval < minval) {
				minval = aveval;
				imin = i;
			}
			if(aveval > maxval) {
				maxval = aveval;
				imax = i;
			}
		}
		else {
			return ERROR_INT("invalid selection", procName, 1);
		}
	}

	if(pminval) *pminval = minval;
	if(pmaxval) *pmaxval = maxval;
	if(pminindex) *pminindex = imin;
	if(pmaxindex) *pmaxindex = imax;
	return 0;
}

/*-------------------------------------------------------------*
*                       Colormap conversion                   *
*-------------------------------------------------------------*/
/*!
 * \brief   pixcmapGrayToFalseColor()
 *
 * \param[in]    gamma   (factor) 0.0 or 1.0 for default; > 1.0 for brighter;
 *                       2.0 is quite nice
 * \return  cmap, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This creates a colormap that maps from gray to false colors.
 *          The colormap is modeled after the Matlap "jet" configuration.
 * </pre>
 */
PIXCMAP * pixcmapGrayToFalseColor(float gamma)
{
	int32 i, rval, gval, bval;
	int32 * curve;
	float invgamma, x;
	PIXCMAP   * cmap;
	if(gamma <= 0.0) gamma = 1.0;
	/* Generate curve for transition part of color map */
	curve = (int32*)SAlloc::C(64, sizeof(int32));
	invgamma = 1.0f / gamma;
	for(i = 0; i < 64; i++) {
		x = (float)i / 64.0f;
		curve[i] = (int32)(255.0 * powf(x, invgamma) + 0.5);
	}

	cmap = pixcmapCreate(8);
	for(i = 0; i < 256; i++) {
		if(i < 32) {
			rval = 0;
			gval = 0;
			bval = curve[i + 32];
		}
		else if(i < 96) { /* 32 - 95 */
			rval = 0;
			gval = curve[i - 32];
			bval = 255;
		}
		else if(i < 160) { /* 96 - 159 */
			rval = curve[i - 96];
			gval = 255;
			bval = curve[159 - i];
		}
		else if(i < 224) { /* 160 - 223 */
			rval = 255;
			gval = curve[223 - i];
			bval = 0;
		}
		else { /* 224 - 255 */
			rval = curve[287 - i];
			gval = 0;
			bval = 0;
		}
		pixcmapAddColor(cmap, rval, gval, bval);
	}

	SAlloc::F(curve);
	return cmap;
}

/*!
 * \brief   pixcmapGrayToColor()
 *
 * \param[in]    color
 * \return  cmap, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This creates a colormap that maps from gray to
 *          a specific color.  In the mapping, each component
 *          is faded to white, depending on the gray value.
 *      (2) In use, this is simply attached to a grayscale pix
 *          to give it the input color.
 * </pre>
 */
PIXCMAP * pixcmapGrayToColor(uint32 color)
{
	int32 i, rval, gval, bval;
	PIXCMAP  * cmap;

	extractRGBValues(color, &rval, &gval, &bval);
	cmap = pixcmapCreate(8);
	for(i = 0; i < 256; i++) {
		pixcmapAddColor(cmap, rval + (i * (255 - rval)) / 255,
		    gval + (i * (255 - gval)) / 255,
		    bval + (i * (255 - bval)) / 255);
	}

	return cmap;
}

/*!
 * \brief   pixcmapColorToGray()
 *
 * \param[in]    cmaps
 * \param[in]    rwt, gwt, bwt    non-negative; these should add to 1.0
 * \return  cmap gray, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This creates a gray colormap from an arbitrary colormap.
 *      (2) In use, attach the output gray colormap to the pix
 *          (or a copy of it) that provided the input colormap.
 * </pre>
 */
PIXCMAP * pixcmapColorToGray(PIXCMAP   * cmaps,
    float rwt,
    float gwt,
    float bwt)
{
	int32 i, n, rval, gval, bval, val;
	float sum;
	PIXCMAP   * cmapd;

	PROCNAME(__FUNCTION__);

	if(!cmaps)
		return (PIXCMAP*)ERROR_PTR("cmaps not defined", procName, NULL);
	if(rwt < 0.0 || gwt < 0.0 || bwt < 0.0)
		return (PIXCMAP*)ERROR_PTR("weights not all >= 0.0", procName, NULL);

	/* Make sure the sum of weights is 1.0; otherwise, you can get
	 * overflow in the gray value. */
	sum = rwt + gwt + bwt;
	if(sum == 0.0) {
		L_WARNING("all weights zero; setting equal to 1/3\n", procName);
		rwt = gwt = bwt = 0.33333;
		sum = 1.0;
	}
	if(L_ABS(sum - 1.0) > 0.0001) { /* maintain ratios with sum == 1.0 */
		L_WARNING("weights don't sum to 1; maintaining ratios\n", procName);
		rwt = rwt / sum;
		gwt = gwt / sum;
		bwt = bwt / sum;
	}

	if((cmapd = pixcmapCopy(cmaps)) == NULL)
		return (PIXCMAP*)ERROR_PTR("cmapd not made", procName, NULL);
	n = pixcmapGetCount(cmapd);
	for(i = 0; i < n; i++) {
		pixcmapGetColor(cmapd, i, &rval, &gval, &bval);
		val = (int32)(rwt * rval + gwt * gval + bwt * bval + 0.5);
		pixcmapResetColor(cmapd, i, val, val, val);
	}

	return cmapd;
}

/*!
 * \brief   pixcmapConvertTo4()
 *
 * \param[in]    cmaps   colormap for 2 bpp pix
 * \return  cmapd   (4 bpp)
 *
 * <pre>
 * Notes:
 *      (1) This converts a 2 bpp colormap to 4 bpp.  The colors
 *          are the same; the output colormap entry array has size 16.
 * </pre>
 */
PIXCMAP * pixcmapConvertTo4(PIXCMAP  * cmaps)
{
	int32 i, n, rval, gval, bval;
	PIXCMAP  * cmapd;

	PROCNAME(__FUNCTION__);

	if(!cmaps)
		return (PIXCMAP*)ERROR_PTR("cmaps not defined", procName, NULL);
	if(pixcmapGetDepth(cmaps) != 2)
		return (PIXCMAP*)ERROR_PTR("cmaps not for 2 bpp pix", procName, NULL);

	cmapd = pixcmapCreate(4);
	n = pixcmapGetCount(cmaps);
	for(i = 0; i < n; i++) {
		pixcmapGetColor(cmaps, i, &rval, &gval, &bval);
		pixcmapAddColor(cmapd, rval, gval, bval);
	}
	return cmapd;
}

/*!
 * \brief   pixcmapConvertTo8()
 *
 * \param[in]    cmaps   colormap for 2 bpp or 4 bpp pix
 * \return  cmapd   (8 bpp)
 *
 * <pre>
 * Notes:
 *      (1) This converts a 2 bpp or 4 bpp colormap to 8 bpp.  The colors
 *          are the same; the output colormap entry array has size 256.
 * </pre>
 */
PIXCMAP * pixcmapConvertTo8(PIXCMAP  * cmaps)
{
	int32 i, n, depth, rval, gval, bval;
	PIXCMAP  * cmapd;

	PROCNAME(__FUNCTION__);

	if(!cmaps)
		return (PIXCMAP*)ERROR_PTR("cmaps not defined", procName, NULL);
	depth = pixcmapGetDepth(cmaps);
	if(depth == 8) return pixcmapCopy(cmaps);
	if(depth != 2 && depth != 4)
		return (PIXCMAP*)ERROR_PTR("cmaps not 2 or 4 bpp", procName, NULL);

	cmapd = pixcmapCreate(8);
	n = pixcmapGetCount(cmaps);
	for(i = 0; i < n; i++) {
		pixcmapGetColor(cmaps, i, &rval, &gval, &bval);
		pixcmapAddColor(cmapd, rval, gval, bval);
	}
	return cmapd;
}

/*-------------------------------------------------------------*
*                         Colormap I/O                        *
*-------------------------------------------------------------*/
/*!
 * \brief   pixcmapRead()
 *
 * \param[in]    filename
 * \return  cmap, or NULL on error
 */
PIXCMAP * pixcmapRead(const char * filename)
{
	FILE * fp;
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return (PIXCMAP*)ERROR_PTR("filename not defined", procName, NULL);

	if((fp = fopenReadStream(filename)) == NULL)
		return (PIXCMAP*)ERROR_PTR("stream not opened", procName, NULL);
	cmap = pixcmapReadStream(fp);
	fclose(fp);
	if(!cmap)
		return (PIXCMAP*)ERROR_PTR("cmap not read", procName, NULL);
	return cmap;
}

/*!
 * \brief   pixcmapReadStream()
 *
 * \param[in]    fp     file stream
 * \return  cmap, or NULL on error
 */
PIXCMAP * pixcmapReadStream(FILE * fp)
{
	int32 rval, gval, bval, aval, ignore;
	int32 i, index, ret, depth, ncolors;
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return (PIXCMAP*)ERROR_PTR("stream not defined", procName, NULL);

	ret = fscanf(fp, "\nPixcmap: depth = %d bpp; %d colors\n",
		&depth, &ncolors);
	if(ret != 2 ||
	    (depth != 1 && depth != 2 && depth != 4 && depth != 8) ||
	    (ncolors < 2 || ncolors > 256))
		return (PIXCMAP*)ERROR_PTR("invalid cmap size", procName, NULL);
	ignore = fscanf(fp, "Color    R-val    G-val    B-val   Alpha\n");
	ignore = fscanf(fp, "----------------------------------------\n");

	if((cmap = pixcmapCreate(depth)) == NULL)
		return (PIXCMAP*)ERROR_PTR("cmap not made", procName, NULL);
	for(i = 0; i < ncolors; i++) {
		if(fscanf(fp, "%3d       %3d      %3d      %3d      %3d\n",
		    &index, &rval, &gval, &bval, &aval) != 5) {
			pixcmapDestroy(&cmap);
			return (PIXCMAP*)ERROR_PTR("invalid entry", procName, NULL);
		}
		pixcmapAddRGBA(cmap, rval, gval, bval, aval);
	}
	return cmap;
}

/*!
 * \brief   pixcmapReadMem()
 *
 * \param[in]    data     serialization of pixcmap; in ascii
 * \param[in]    size     of data in bytes; can use strlen to get it
 * \return  cmap, or NULL on error
 */
PIXCMAP * pixcmapReadMem(const uint8  * data,
    size_t size)
{
	FILE * fp;
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(!data)
		return (PIXCMAP*)ERROR_PTR("data not defined", procName, NULL);
	if((fp = fopenReadFromMemory(data, size)) == NULL)
		return (PIXCMAP*)ERROR_PTR("stream not opened", procName, NULL);

	cmap = pixcmapReadStream(fp);
	fclose(fp);
	if(!cmap) L_ERROR("cmap not read\n", procName);
	return cmap;
}

/*!
 * \brief   pixcmapWrite()
 *
 * \param[in]    filename
 * \param[in]    cmap
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapWrite(const char     * filename,
    const PIXCMAP  * cmap)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return ERROR_INT("filename not defined", procName, 1);
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	if((fp = fopenWriteStream(filename, "w")) == NULL)
		return ERROR_INT("stream not opened", procName, 1);
	ret = pixcmapWriteStream(fp, cmap);
	fclose(fp);
	if(ret)
		return ERROR_INT("cmap not written to stream", procName, 1);
	return 0;
}

/*!
 * \brief   pixcmapWriteStream()
 *
 * \param[in]    fp      file stream
   \param[in]    cmap
 * \return  0 if OK, 1 on error
 */
l_ok pixcmapWriteStream(FILE           * fp,
    const PIXCMAP  * cmap)
{
	int32 * rmap, * gmap, * bmap, * amap;
	int32 i;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	if(pixcmapToArrays(cmap, &rmap, &gmap, &bmap, &amap))
		return ERROR_INT("colormap arrays not made", procName, 1);

	fprintf(fp, "\nPixcmap: depth = %d bpp; %d colors\n", cmap->depth, cmap->n);
	fprintf(fp, "Color    R-val    G-val    B-val   Alpha\n");
	fprintf(fp, "----------------------------------------\n");
	for(i = 0; i < cmap->n; i++)
		fprintf(fp, "%3d       %3d      %3d      %3d      %3d\n",
		    i, rmap[i], gmap[i], bmap[i], amap[i]);
	fprintf(fp, "\n");

	SAlloc::F(rmap);
	SAlloc::F(gmap);
	SAlloc::F(bmap);
	SAlloc::F(amap);
	return 0;
}

/*!
 * \brief   pixcmapWriteMem()
 *
 * \param[out]   pdata     data of serialized pixcmap; ascii
 * \param[out]   psize     size of returned data
 * \param[in]    cmap
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes a pixcmap in memory and puts the result in a buffer.
 * </pre>
 */
l_ok pixcmapWriteMem(uint8        ** pdata,
    size_t         * psize,
    const PIXCMAP  * cmap)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(pdata) *pdata = NULL;
	if(psize) *psize = 0;
	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

#if HAVE_FMEMOPEN
	if((fp = open_memstream((char**)pdata, psize)) == NULL)
		return ERROR_INT("stream not opened", procName, 1);
	ret = pixcmapWriteStream(fp, cmap);
	fputc('\0', fp);
	fclose(fp);
	*psize = *psize - 1;
#else
	L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
	if((fp = fopenWriteWinTempfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
	if((fp = tmpfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
	ret = pixcmapWriteStream(fp, cmap);
	rewind(fp);
	*pdata = l_binaryReadStream(fp, psize);
	fclose(fp);
#endif  /* HAVE_FMEMOPEN */
	return ret;
}

/*----------------------------------------------------------------------*
*               Extract colormap arrays and serialization              *
*----------------------------------------------------------------------*/
/*!
 * \brief   pixcmapToArrays()
 *
 * \param[in]    cmap     colormap
 * \param[out]   prmap    array of red values
 * \param[out]   pgmap    array of green values
 * \param[out]   pbmap    array of blue values
 * \param[out]   pamap    [optional] array of alpha (transparency) values
 * \return  0 if OK; 1 on error
 */
l_ok pixcmapToArrays(const PIXCMAP  * cmap,
    int32 ** prmap,
    int32 ** pgmap,
    int32 ** pbmap,
    int32 ** pamap)
{
	int32 * rmap, * gmap, * bmap, * amap;
	int32 i, ncolors;
	RGBA_QUAD  * cta;

	PROCNAME(__FUNCTION__);

	if(!prmap || !pgmap || !pbmap)
		return ERROR_INT("&rmap, &gmap, &bmap not all defined", procName, 1);
	*prmap = *pgmap = *pbmap = NULL;
	if(pamap) *pamap = NULL;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	rmap = (int32*)SAlloc::C(ncolors, sizeof(int32));
	gmap = (int32*)SAlloc::C(ncolors, sizeof(int32));
	bmap = (int32*)SAlloc::C(ncolors, sizeof(int32));
	*prmap = rmap;
	*pgmap = gmap;
	*pbmap = bmap;
	if(pamap) {
		amap = (int32*)SAlloc::C(ncolors, sizeof(int32));
		*pamap = amap;
	}

	cta = (RGBA_QUAD*)cmap->array;
	for(i = 0; i < ncolors; i++) {
		rmap[i] = cta[i].red;
		gmap[i] = cta[i].green;
		bmap[i] = cta[i].blue;
		if(pamap)
			amap[i] = cta[i].alpha;
	}

	return 0;
}

/*!
 * \brief   pixcmapToRGBTable()
 *
 * \param[in]    cmap       colormap
 * \param[out]   ptab       table of rgba values for the colormap
 * \param[out]   pncolors   [optional] size of table
 * \return  0 if OK; 1 on error
 */
l_ok pixcmapToRGBTable(PIXCMAP * cmap,
    uint32  ** ptab,
    int32 * pncolors)
{
	int32 i, ncolors, rval, gval, bval, aval;
	uint32 * tab;

	PROCNAME(__FUNCTION__);

	if(!ptab)
		return ERROR_INT("&tab not defined", procName, 1);
	*ptab = NULL;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	if(pncolors) *pncolors = ncolors;
	tab = (uint32*)SAlloc::C(ncolors, sizeof(uint32));
	*ptab = tab;

	for(i = 0; i < ncolors; i++) {
		pixcmapGetRGBA(cmap, i, &rval, &gval, &bval, &aval);
		composeRGBAPixel(rval, gval, bval, aval, &tab[i]);
	}
	return 0;
}

/*!
 * \brief   pixcmapSerializeToMemory()
 *
 * \param[in]    cmap       colormap
 * \param[in]    cpc        components/color: 3 for rgb, 4 for rgba
 * \param[out]   pncolors   number of colors in table
 * \param[out]   pdata      binary string, cpc bytes per color
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) When serializing to store in a pdf, use %cpc = 3.
 * </pre>
 */
l_ok pixcmapSerializeToMemory(PIXCMAP   * cmap,
    int32 cpc,
    int32 * pncolors,
    uint8  ** pdata)
{
	int32 i, ncolors, rval, gval, bval, aval;
	uint8  * data;

	PROCNAME(__FUNCTION__);

	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	*pdata = NULL;
	if(!pncolors)
		return ERROR_INT("&ncolors not defined", procName, 1);
	*pncolors = 0;
	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(cpc != 3 && cpc != 4)
		return ERROR_INT("cpc not 3 or 4", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	*pncolors = ncolors;
	data = (uint8 *)SAlloc::C((size_t)cpc * ncolors, sizeof(uint8));
	*pdata = data;

	for(i = 0; i < ncolors; i++) {
		pixcmapGetRGBA(cmap, i, &rval, &gval, &bval, &aval);
		data[cpc * i] = rval;
		data[cpc * i + 1] = gval;
		data[cpc * i + 2] = bval;
		if(cpc == 4)
			data[cpc * i + 3] = aval;
	}
	return 0;
}

/*!
 * \brief   pixcmapDeserializeFromMemory()
 *
 * \param[in]    data      binary string, 3 or 4 bytes per color
 * \param[in]    cpc       components/color: 3 for rgb, 4 for rgba
 * \param[in]    ncolors   > 0
 * \return  cmap, or NULL on error
 */
PIXCMAP * pixcmapDeserializeFromMemory(uint8  * data,
    int32 cpc,
    int32 ncolors)
{
	int32 i, d, rval, gval, bval, aval;
	PIXCMAP  * cmap;

	PROCNAME(__FUNCTION__);

	if(!data)
		return (PIXCMAP*)ERROR_PTR("data not defined", procName, NULL);
	if(cpc != 3 && cpc != 4)
		return (PIXCMAP*)ERROR_PTR("cpc not 3 or 4", procName, NULL);
	if(ncolors <= 0)
		return (PIXCMAP*)ERROR_PTR("no entries", procName, NULL);
	if(ncolors > 256)
		return (PIXCMAP*)ERROR_PTR("ncolors > 256", procName, NULL);

	if(ncolors > 16)
		d = 8;
	else if(ncolors > 4)
		d = 4;
	else if(ncolors > 2)
		d = 2;
	else
		d = 1;
	cmap = pixcmapCreate(d);
	for(i = 0; i < ncolors; i++) {
		rval = data[cpc * i];
		gval = data[cpc * i + 1];
		bval = data[cpc * i + 2];
		if(cpc == 4)
			aval = data[cpc * i + 3];
		else
			aval = 255; /* opaque */
		pixcmapAddRGBA(cmap, rval, gval, bval, aval);
	}

	return cmap;
}

/*!
 * \brief   pixcmapConvertToHex()
 *
 * \param[in]    data       binary serialized data
 * \param[in]    ncolors    in colormap
 * \return  hexdata bracketed, space-separated ascii hex string,
 *                       or NULL on error.
 *
 * <pre>
 * Notes:
 *      (1) The number of bytes in %data is 3 * ncolors.
 *      (2) Output is in form:
 *             < r0g0b0 r1g1b1 ... rngnbn >
 *          where r0, g0, b0 ... are each 2 bytes of hex ascii
 *      (3) This is used in pdf files to express the colormap as an
 *          array in ascii (human-readable) format.
 * </pre>
 */
char * pixcmapConvertToHex(uint8 * data,
    int32 ncolors)
{
	int32 i, j, hexbytes;
	char * hexdata = NULL;
	char buf[4];

	PROCNAME(__FUNCTION__);

	if(!data)
		return (char *)ERROR_PTR("data not defined", procName, NULL);
	if(ncolors < 1)
		return (char *)ERROR_PTR("no colors", procName, NULL);

	hexbytes = 2 + (2 * 3 + 1) * ncolors + 2;
	hexdata = (char *)SAlloc::C(hexbytes, sizeof(char));
	hexdata[0] = '<';
	hexdata[1] = ' ';

	for(i = 0; i < ncolors; i++) {
		j = 2 + (2 * 3 + 1) * i;
		snprintf(buf, sizeof(buf), "%02x", data[3 * i]);
		hexdata[j] = buf[0];
		hexdata[j + 1] = buf[1];
		snprintf(buf, sizeof(buf), "%02x", data[3 * i + 1]);
		hexdata[j + 2] = buf[0];
		hexdata[j + 3] = buf[1];
		snprintf(buf, sizeof(buf), "%02x", data[3 * i + 2]);
		hexdata[j + 4] = buf[0];
		hexdata[j + 5] = buf[1];
		hexdata[j + 6] = ' ';
	}
	hexdata[j + 7] = '>';
	hexdata[j + 8] = '\0';
	return hexdata;
}

/*-------------------------------------------------------------*
*                     Colormap transforms                     *
*-------------------------------------------------------------*/
/*!
 * \brief   pixcmapGammaTRC()
 *
 * \param[in]    cmap      colormap
 * \param[in]    gamma     gamma correction; must be > 0.0
 * \param[in]    minval    input value that gives 0 for output; can be < 0
 * \param[in]    maxval    input value that gives 255 for output; can be > 255
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is an in-place transform
 *      (2) See pixGammaTRC() and numaGammaTRC() in enhance.c
 *          for description and use of transform
 * </pre>
 */
l_ok pixcmapGammaTRC(PIXCMAP   * cmap,
    float gamma,
    int32 minval,
    int32 maxval)
{
	int32 rval, gval, bval, trval, tgval, tbval, i, ncolors;
	NUMA * nag;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(gamma <= 0.0) {
		L_WARNING("gamma must be > 0.0; setting to 1.0\n", procName);
		gamma = 1.0;
	}
	if(minval >= maxval)
		return ERROR_INT("minval not < maxval", procName, 1);

	if(gamma == 1.0 && minval == 0 && maxval == 255) /* no-op */
		return 0;

	if((nag = numaGammaTRC(gamma, minval, maxval)) == NULL)
		return ERROR_INT("nag not made", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	for(i = 0; i < ncolors; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		numaGetIValue(nag, rval, &trval);
		numaGetIValue(nag, gval, &tgval);
		numaGetIValue(nag, bval, &tbval);
		pixcmapResetColor(cmap, i, trval, tgval, tbval);
	}

	numaDestroy(&nag);
	return 0;
}

/*!
 * \brief   pixcmapContrastTRC()
 *
 * \param[in]    cmap     colormap
 * \param[in]    factor   generally between 0.0 [no enhancement]
 *                        and 1.0, but can be larger than 1.0
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is an in-place transform
 *      (2) See pixContrastTRC() and numaContrastTRC() in enhance.c
 *          for description and use of transform
 * </pre>
 */
l_ok pixcmapContrastTRC(PIXCMAP   * cmap,
    float factor)
{
	int32 i, ncolors, rval, gval, bval, trval, tgval, tbval;
	NUMA * nac;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(factor < 0.0) {
		L_WARNING("factor must be >= 0.0; setting to 0.0\n", procName);
		factor = 0.0;
	}

	if((nac = numaContrastTRC(factor)) == NULL)
		return ERROR_INT("nac not made", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	for(i = 0; i < ncolors; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		numaGetIValue(nac, rval, &trval);
		numaGetIValue(nac, gval, &tgval);
		numaGetIValue(nac, bval, &tbval);
		pixcmapResetColor(cmap, i, trval, tgval, tbval);
	}

	numaDestroy(&nac);
	return 0;
}

/*!
 * \brief   pixcmapShiftIntensity()
 *
 * \param[in]    cmap       colormap
 * \param[in]    fraction   between -1.0 and +1.0
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is an in-place transform
 *      (2) It does a proportional shift of the intensity for each color.
 *      (3) If fraction < 0.0, it moves all colors towards (0,0,0).
 *          This darkens the image.
 *          If fraction > 0.0, it moves all colors towards (255,255,255)
 *          This fades the image.
 *      (4) The equivalent transform can be accomplished with pixcmapGammaTRC(),
 *          but it is considerably more difficult (see numaGammaTRC()).
 * </pre>
 */
l_ok pixcmapShiftIntensity(PIXCMAP   * cmap,
    float fraction)
{
	int32 i, ncolors, rval, gval, bval;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);
	if(fraction < -1.0 || fraction > 1.0)
		return ERROR_INT("fraction not in [-1.0, 1.0]", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	for(i = 0; i < ncolors; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		if(fraction < 0.0)
			pixcmapResetColor(cmap, i,
			    (int32)((1.0 + fraction) * rval),
			    (int32)((1.0 + fraction) * gval),
			    (int32)((1.0 + fraction) * bval));
		else
			pixcmapResetColor(cmap, i,
			    rval + (int32)(fraction * (255 - rval)),
			    gval + (int32)(fraction * (255 - gval)),
			    bval + (int32)(fraction * (255 - bval)));
	}

	return 0;
}

/*!
 * \brief   pixcmapShiftByComponent()
 *
 * \param[in]    cmap     colormap
 * \param[in]    srcval   source color: 0xrrggbb00
 * \param[in]    dstval   target color: 0xrrggbb00
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is an in-place transform
 *      (2) It implements pixelShiftByComponent() for each color.
 *          The mapping is specified by srcval and dstval.
 *      (3) If a component decreases, the component in the colormap
 *          decreases by the same ratio.  Likewise for increasing, except
 *          all ratios are taken with respect to the distance from 255.
 * </pre>
 */
l_ok pixcmapShiftByComponent(PIXCMAP  * cmap,
    uint32 srcval,
    uint32 dstval)
{
	int32 i, ncolors, rval, gval, bval;
	uint32 newval;

	PROCNAME(__FUNCTION__);

	if(!cmap)
		return ERROR_INT("cmap not defined", procName, 1);

	ncolors = pixcmapGetCount(cmap);
	for(i = 0; i < ncolors; i++) {
		pixcmapGetColor(cmap, i, &rval, &gval, &bval);
		pixelShiftByComponent(rval, gval, bval, srcval, dstval, &newval);
		extractRGBValues(newval, &rval, &gval, &bval);
		pixcmapResetColor(cmap, i, rval, gval, bval);
	}

	return 0;
}
