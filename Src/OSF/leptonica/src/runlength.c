// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file runlength.c
 * <pre>
 *
 *     Label pixels by membership in runs
 *           PIX         *pixStrokeWidthTransform()
 *           static PIX  *pixFindMinRunsOrthogonal()
 *           PIX         *pixRunlengthTransform()
 *
 *     Find runs along horizontal and vertical lines
 *           int32      pixFindHorizontalRuns()
 *           int32      pixFindVerticalRuns()
 *
 *     Find max runs along horizontal and vertical lines
 *           int32      pixFindMaxRuns()
 *           int32      pixFindMaxHorizontalRunOnLine()
 *           int32      pixFindMaxVerticalRunOnLine()
 *
 *     Compute runlength-to-membership transform on a line
 *           int32      runlengthMembershipOnLine()
 *
 *     Make byte position LUT
 *           int32      makeMSBitLocTab()
 *
 *  Here we're handling runs of either black or white pixels on 1 bpp
 *  images.  The directions of the runs in the stroke width transform
 *  are selectable from given sets of angles.  Most of the other runs
 *  are oriented either horizontally along the raster lines or
 *  vertically along pixel columns.
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

static PIX * pixFindMinRunsOrthogonal(PIX * pixs, float angle, int32 depth);

/*-----------------------------------------------------------------------*
*                   Label pixels by membership in runs                  *
*-----------------------------------------------------------------------*/
/*!
 * \brief   pixStrokeWidthTransform()
 *
 * \param[in]     pixs      1 bpp
 * \param[in]     color     0 for white runs, 1 for black runs
 * \param[in]     depth     of pixd: 8 or 16 bpp
 * \param[in]     nangles   2, 4, 6 or 8
 * \return   pixd   8 or 16 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The dest Pix is 8 or 16 bpp, with the pixel values
 *          equal to the stroke width in which it is a member.
 *          The values are clipped to the max pixel value if necessary.
 *      (2) %color determines if we're labelling white or black strokes.
 *      (3) A pixel that is not a member of the chosen color gets
 *          value 0; it belongs to a width of length 0 of the
 *          chosen color.
 *      (4) This chooses, for each dest pixel, the minimum of sets
 *          of runlengths through each pixel.  Here are the sets:
 *            nangles    increment          set
 *            -------    ---------    --------------------------------
 *               2          90       {0, 90}
 *               4          45       {0, 45, 90, 135}
 *               6          30       {0, 30, 60, 90, 120, 150}
 *               8          22.5     {0, 22.5, 45, 67.5, 90, 112.5, 135, 157.5}
 *      (5) Runtime scales linearly with (%nangles - 2).
 * </pre>
 */
PIX * pixStrokeWidthTransform(PIX * pixs, int32 color, int32 depth, int32 nangles)
{
	float angle, pi;
	PIX * pixh, * pixv, * pixt, * pixg1, * pixg2, * pixg3, * pixg4;
	PROCNAME(__FUNCTION__);
	if(!pixs || pixGetDepth(pixs) != 1)
		return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);
	if(depth != 8 && depth != 16)
		return (PIX *)ERROR_PTR("depth must be 8 or 16 bpp", procName, NULL);
	if(nangles != 2 && nangles != 4 && nangles != 6 && nangles != 8)
		return (PIX *)ERROR_PTR("nangles not in {2,4,6,8}", procName, NULL);
	/* Use fg runs for evaluation */
	if(color == 0)
		pixt = pixInvert(NULL, pixs);
	else
		pixt = pixClone(pixs);
	/* Find min length at 0 and 90 degrees */
	pixh = pixRunlengthTransform(pixt, 1, L_HORIZONTAL_RUNS, depth);
	pixv = pixRunlengthTransform(pixt, 1, L_VERTICAL_RUNS, depth);
	pixg1 = pixMinOrMax(NULL, pixh, pixv, L_CHOOSE_MIN);
	pixDestroy(&pixh);
	pixDestroy(&pixv);
	pixg2 = pixg3 = pixg4 = NULL;
	//pi = 3.1415926535;
	pi = SMathConst::Pi_f;
	if(nangles == 4 || nangles == 8) {
		/* Find min length at +45 and -45 degrees */
		angle = pi / 4.0f;
		pixg2 = pixFindMinRunsOrthogonal(pixt, angle, depth);
	}
	if(nangles == 6) {
		/* Find min length at +30 and -60 degrees */
		angle = pi / 6.0f;
		pixg2 = pixFindMinRunsOrthogonal(pixt, angle, depth);

		/* Find min length at +60 and -30 degrees */
		angle = pi / 3.0f;
		pixg3 = pixFindMinRunsOrthogonal(pixt, angle, depth);
	}
	if(nangles == 8) {
		/* Find min length at +22.5 and -67.5 degrees */
		angle = pi / 8.0f;
		pixg3 = pixFindMinRunsOrthogonal(pixt, angle, depth);
		/* Find min length at +67.5 and -22.5 degrees */
		angle = 3.0f * pi / 8.0f;
		pixg4 = pixFindMinRunsOrthogonal(pixt, angle, depth);
	}
	pixDestroy(&pixt);
	if(nangles > 2)
		pixMinOrMax(pixg1, pixg1, pixg2, L_CHOOSE_MIN);
	if(nangles > 4)
		pixMinOrMax(pixg1, pixg1, pixg3, L_CHOOSE_MIN);
	if(nangles > 6)
		pixMinOrMax(pixg1, pixg1, pixg4, L_CHOOSE_MIN);
	pixDestroy(&pixg2);
	pixDestroy(&pixg3);
	pixDestroy(&pixg4);
	return pixg1;
}
/*!
 * \brief   pixFindMinRunsOrthogonal()
 *
 * \param[in]     pixs     1 bpp
 * \param[in]     angle    in radians
 * \param[in]     depth    of pixd: 8 or 16 bpp
 * \return   pixd 8 or 16 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This computes, for each fg pixel in pixs, the minimum of
 *          the runlengths going through that pixel in two orthogonal
 *          directions: at %angle and at (90 + %angle).
 *      (2) We use rotation by shear because the forward and backward
 *          rotations by the same angle are exact inverse operations.
 *          As a result, the nonzero pixels in pixd correspond exactly
 *          to the fg pixels in pixs.  This is not the case with
 *          sampled rotation, due to spatial quantization.  Nevertheless,
 *          the result suffers from lack of exact correspondence
 *          between original and rotated pixels, also due to spatial
 *          quantization, causing some boundary pixels to be
 *          shifted from bg to fg or v.v.
 * </pre>
 */
static PIX * pixFindMinRunsOrthogonal(PIX * pixs,
    float angle,
    int32 depth)
{
	int32 w, h, diag, xoff, yoff;
	PIX * pixb, * pixr, * pixh, * pixv, * pixg1, * pixg2, * pixd;
	BOX * box;

	PROCNAME(__FUNCTION__);

	if(!pixs || pixGetDepth(pixs) != 1)
		return (PIX *)ERROR_PTR("pixs undefined or not 1 bpp", procName, NULL);

	/* Rasterop into the center of a sufficiently large image
	 * so we don't lose pixels for any rotation angle. */
	pixGetDimensions(pixs, &w, &h, NULL);
	diag = (int32)(sqrt((double)(w * w + h * h)) + 2.5);
	xoff = (diag - w) / 2;
	yoff = (diag - h) / 2;
	pixb = pixCreate(diag, diag, 1);
	pixRasterop(pixb, xoff, yoff, w, h, PIX_SRC, pixs, 0, 0);

	/* Rotate about the 'center', get the min of orthogonal transforms,
	 * rotate back, and crop the part corresponding to pixs.  */
	pixr = pixRotateShear(pixb, diag / 2, diag / 2, angle, L_BRING_IN_WHITE);
	pixh = pixRunlengthTransform(pixr, 1, L_HORIZONTAL_RUNS, depth);
	pixv = pixRunlengthTransform(pixr, 1, L_VERTICAL_RUNS, depth);
	pixg1 = pixMinOrMax(NULL, pixh, pixv, L_CHOOSE_MIN);
	pixg2 = pixRotateShear(pixg1, diag / 2, diag / 2, -angle, L_BRING_IN_WHITE);
	box = boxCreate(xoff, yoff, w, h);
	pixd = pixClipRectangle(pixg2, box, NULL);

	pixDestroy(&pixb);
	pixDestroy(&pixr);
	pixDestroy(&pixh);
	pixDestroy(&pixv);
	pixDestroy(&pixg1);
	pixDestroy(&pixg2);
	boxDestroy(&box);
	return pixd;
}

/*!
 * \brief   pixRunlengthTransform()
 *
 * \param[in]     pixs        1 bpp
 * \param[in]     color       0 for white runs, 1 for black runs
 * \param[in]     direction   L_HORIZONTAL_RUNS, L_VERTICAL_RUNS
 * \param[in]     depth       8 or 16 bpp
 * \return   pixd   8 or 16 bpp, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) The dest Pix is 8 or 16 bpp, with the pixel values
 *          equal to the runlength in which it is a member.
 *          The length is clipped to the max pixel value if necessary.
 *      (2) %color determines if we're labelling white or black runs.
 *      (3) A pixel that is not a member of the chosen color gets
 *          value 0; it belongs to a run of length 0 of the
 *          chosen color.
 *      (4) To convert for maximum dynamic range, either linear or
 *          log, use pixMaxDynamicRange().
 * </pre>
 */
PIX * pixRunlengthTransform(PIX * pixs,
    int32 color,
    int32 direction,
    int32 depth)
{
	int32 i, j, w, h, wpld, bufsize, maxsize, n;
	int32 * start, * end, * buffer;
	uint32  * datad, * lined;
	PIX * pixt, * pixd;

	PROCNAME(__FUNCTION__);

	if(!pixs)
		return (PIX *)ERROR_PTR("pixs not defined", procName, NULL);
	if(pixGetDepth(pixs) != 1)
		return (PIX *)ERROR_PTR("pixs not 1 bpp", procName, NULL);
	if(depth != 8 && depth != 16)
		return (PIX *)ERROR_PTR("depth must be 8 or 16 bpp", procName, NULL);

	pixGetDimensions(pixs, &w, &h, NULL);
	if(direction == L_HORIZONTAL_RUNS)
		maxsize = 1 + w / 2;
	else if(direction == L_VERTICAL_RUNS)
		maxsize = 1 + h / 2;
	else
		return (PIX *)ERROR_PTR("invalid direction", procName, NULL);
	bufsize = MAX(w, h);
	if(bufsize > 1000000) {
		L_ERROR("largest image dimension = %d; too big\n", procName, bufsize);
		return NULL;
	}

	if((pixd = pixCreate(w, h, depth)) == NULL)
		return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
	datad = pixGetData(pixd);
	wpld = pixGetWpl(pixd);

	start = (int32*)SAlloc::C(maxsize, sizeof(int32));
	end = (int32*)SAlloc::C(maxsize, sizeof(int32));
	buffer = (int32*)SAlloc::C(bufsize, sizeof(int32));

	/* Use fg runs for evaluation */
	if(color == 0)
		pixt = pixInvert(NULL, pixs);
	else
		pixt = pixClone(pixs);

	if(direction == L_HORIZONTAL_RUNS) {
		for(i = 0; i < h; i++) {
			pixFindHorizontalRuns(pixt, i, start, end, &n);
			runlengthMembershipOnLine(buffer, w, depth, start, end, n);
			lined = datad + i * wpld;
			if(depth == 8) {
				for(j = 0; j < w; j++)
					SET_DATA_BYTE(lined, j, buffer[j]);
			}
			else { /* depth == 16 */
				for(j = 0; j < w; j++)
					SET_DATA_TWO_BYTES(lined, j, buffer[j]);
			}
		}
	}
	else { /* L_VERTICAL_RUNS */
		for(j = 0; j < w; j++) {
			pixFindVerticalRuns(pixt, j, start, end, &n);
			runlengthMembershipOnLine(buffer, h, depth, start, end, n);
			if(depth == 8) {
				for(i = 0; i < h; i++) {
					lined = datad + i * wpld;
					SET_DATA_BYTE(lined, j, buffer[i]);
				}
			}
			else { /* depth == 16 */
				for(i = 0; i < h; i++) {
					lined = datad + i * wpld;
					SET_DATA_TWO_BYTES(lined, j, buffer[i]);
				}
			}
		}
	}

	pixDestroy(&pixt);
	SAlloc::F(start);
	SAlloc::F(end);
	SAlloc::F(buffer);
	return pixd;
}

/*-----------------------------------------------------------------------*
*               Find runs along horizontal and vertical lines           *
*-----------------------------------------------------------------------*/
/*!
 * \brief   pixFindHorizontalRuns()
 *
 * \param[in]    pix      1 bpp
 * \param[in]    y        line to traverse
 * \param[in]    xstart   returns array of start positions for fg runs
 * \param[in]    xend     returns array of end positions for fg runs
 * \param[out]   pn       the number of runs found
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This finds foreground horizontal runs on a single scanline.
 *      (2) To find background runs, use pixInvert() before applying
 *          this function.
 *      (3) %xstart and %xend arrays are input.  They should be
 *          of size w/2 + 1 to insure that they can hold
 *          the maximum number of runs in the raster line.
 * </pre>
 */
l_ok pixFindHorizontalRuns(PIX * pix,
    int32 y,
    int32 * xstart,
    int32 * xend,
    int32 * pn)
{
	int32 inrun; /* boolean */
	int32 index, w, h, d, j, wpl, val;
	uint32  * line;

	PROCNAME(__FUNCTION__);

	if(!pn)
		return ERROR_INT("&n not defined", procName, 1);
	*pn = 0;
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);
	pixGetDimensions(pix, &w, &h, &d);
	if(d != 1)
		return ERROR_INT("pix not 1 bpp", procName, 1);
	if(y < 0 || y >= h)
		return ERROR_INT("y not in [0 ... h - 1]", procName, 1);
	if(!xstart)
		return ERROR_INT("xstart not defined", procName, 1);
	if(!xend)
		return ERROR_INT("xend not defined", procName, 1);

	wpl = pixGetWpl(pix);
	line = pixGetData(pix) + y * wpl;

	inrun = FALSE;
	index = 0;
	for(j = 0; j < w; j++) {
		val = GET_DATA_BIT(line, j);
		if(!inrun) {
			if(val) {
				xstart[index] = j;
				inrun = TRUE;
			}
		}
		else {
			if(!val) {
				xend[index++] = j - 1;
				inrun = FALSE;
			}
		}
	}

	/* Finish last run if necessary */
	if(inrun)
		xend[index++] = w - 1;

	*pn = index;
	return 0;
}

/*!
 * \brief   pixFindVerticalRuns()
 *
 * \param[in]    pix      1 bpp
 * \param[in]    x        line to traverse
 * \param[in]    ystart   returns array of start positions for fg runs
 * \param[in]    yend     returns array of end positions for fg runs
 * \param[out]   pn       the number of runs found
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This finds foreground vertical runs on a single scanline.
 *      (2) To find background runs, use pixInvert() before applying
 *          this function.
 *      (3) %ystart and %yend arrays are input.  They should be
 *          of size h/2 + 1 to insure that they can hold
 *          the maximum number of runs in the raster line.
 * </pre>
 */
l_ok pixFindVerticalRuns(PIX * pix,
    int32 x,
    int32 * ystart,
    int32 * yend,
    int32 * pn)
{
	int32 inrun; /* boolean */
	int32 index, w, h, d, i, wpl, val;
	uint32  * data, * line;

	PROCNAME(__FUNCTION__);

	if(!pn)
		return ERROR_INT("&n not defined", procName, 1);
	*pn = 0;
	if(!pix)
		return ERROR_INT("pix not defined", procName, 1);
	pixGetDimensions(pix, &w, &h, &d);
	if(d != 1)
		return ERROR_INT("pix not 1 bpp", procName, 1);
	if(x < 0 || x >= w)
		return ERROR_INT("x not in [0 ... w - 1]", procName, 1);
	if(!ystart)
		return ERROR_INT("ystart not defined", procName, 1);
	if(!yend)
		return ERROR_INT("yend not defined", procName, 1);

	wpl = pixGetWpl(pix);
	data = pixGetData(pix);

	inrun = FALSE;
	index = 0;
	for(i = 0; i < h; i++) {
		line = data + i * wpl;
		val = GET_DATA_BIT(line, x);
		if(!inrun) {
			if(val) {
				ystart[index] = i;
				inrun = TRUE;
			}
		}
		else {
			if(!val) {
				yend[index++] = i - 1;
				inrun = FALSE;
			}
		}
	}

	/* Finish last run if necessary */
	if(inrun)
		yend[index++] = h - 1;

	*pn = index;
	return 0;
}

/*-----------------------------------------------------------------------*
*            Find max runs along horizontal and vertical lines          *
*-----------------------------------------------------------------------*/
/*!
 * \brief   pixFindMaxRuns()
 *
 * \param[in]    pix         1 bpp
 * \param[in]    direction   L_HORIZONTAL_RUNS or L_VERTICAL_RUNS
 * \param[out]   pnastart    [optional] start locations of longest runs
 * \return  na of lengths of runs, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) This finds the longest foreground runs by row or column
 *      (2) To find background runs, use pixInvert() before applying
 *          this function.
 * </pre>
 */
NUMA * pixFindMaxRuns(PIX * pix, int32 direction, NUMA ** pnastart)
{
	int32 w, h, i, start, size;
	NUMA * nasize;
	PROCNAME(__FUNCTION__);
	ASSIGN_PTR(pnastart, NULL);
	if(direction != L_HORIZONTAL_RUNS && direction != L_VERTICAL_RUNS)
		return (NUMA*)ERROR_PTR("direction invalid", procName, NULL);
	if(!pix || pixGetDepth(pix) != 1)
		return (NUMA*)ERROR_PTR("pix undefined or not 1 bpp", procName, NULL);
	pixGetDimensions(pix, &w, &h, NULL);
	nasize = numaCreate(w);
	if(pnastart) *pnastart = numaCreate(w);
	if(direction == L_HORIZONTAL_RUNS) {
		for(i = 0; i < h; i++) {
			pixFindMaxHorizontalRunOnLine(pix, i, &start, &size);
			numaAddNumber(nasize, size);
			if(pnastart) numaAddNumber(*pnastart, start);
		}
	}
	else { /* vertical scans */
		for(i = 0; i < w; i++) {
			pixFindMaxVerticalRunOnLine(pix, i, &start, &size);
			numaAddNumber(nasize, size);
			if(pnastart) numaAddNumber(*pnastart, start);
		}
	}
	return nasize;
}
/*!
 * \brief   pixFindMaxHorizontalRunOnLine()
 *
 * \param[in]    pix       1 bpp
 * \param[in]    y         line to traverse
 * \param[out]   pxstart   [optional] start position
 * \param[out]   psize     the size of the run
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This finds the longest foreground horizontal run on a scanline.
 *      (2) To find background runs, use pixInvert() before applying
 *          this function.
 * </pre>
 */
l_ok pixFindMaxHorizontalRunOnLine(PIX * pix, int32 y, int32 * pxstart, int32 * psize)
{
	int32 inrun; /* boolean */
	int32 w, h, j, wpl, val, maxstart, maxsize, length, start;
	uint32  * line;
	PROCNAME(__FUNCTION__);
	ASSIGN_PTR(pxstart, 0);
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	*psize = 0;
	if(!pix || pixGetDepth(pix) != 1)
		return ERROR_INT("pix not defined or not 1 bpp", procName, 1);
	pixGetDimensions(pix, &w, &h, NULL);
	if(y < 0 || y >= h)
		return ERROR_INT("y not in [0 ... h - 1]", procName, 1);
	wpl = pixGetWpl(pix);
	line = pixGetData(pix) + y * wpl;
	inrun = FALSE;
	start = 0;
	maxstart = 0;
	maxsize = 0;
	for(j = 0; j < w; j++) {
		val = GET_DATA_BIT(line, j);
		if(!inrun) {
			if(val) {
				start = j;
				inrun = TRUE;
			}
		}
		else if(!val) { /* run just ended */
			length = j - start;
			if(length > maxsize) {
				maxsize = length;
				maxstart = start;
			}
			inrun = FALSE;
		}
	}
	if(inrun) { /* a run has continued to the end of the row */
		length = j - start;
		if(length > maxsize) {
			maxsize = length;
			maxstart = start;
		}
	}
	ASSIGN_PTR(pxstart, maxstart);
	*psize = maxsize;
	return 0;
}

/*!
 * \brief   pixFindMaxVerticalRunOnLine()
 *
 * \param[in]    pix       1 bpp
 * \param[in]    x         column to traverse
 * \param[out]   pystart   [optional] start position
 * \param[out]   psize     the size of the run
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This finds the longest foreground vertical run on a scanline.
 *      (2) To find background runs, use pixInvert() before applying
 *          this function.
 * </pre>
 */
l_ok pixFindMaxVerticalRunOnLine(PIX * pix,
    int32 x,
    int32 * pystart,
    int32 * psize)
{
	int32 inrun; /* boolean */
	int32 w, h, i, wpl, val, maxstart, maxsize, length, start;
	uint32  * data, * line;

	PROCNAME(__FUNCTION__);

	if(pystart) *pystart = 0;
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	*psize = 0;
	if(!pix || pixGetDepth(pix) != 1)
		return ERROR_INT("pix not defined or not 1 bpp", procName, 1);
	pixGetDimensions(pix, &w, &h, NULL);
	if(x < 0 || x >= w)
		return ERROR_INT("x not in [0 ... w - 1]", procName, 1);

	wpl = pixGetWpl(pix);
	data = pixGetData(pix);
	inrun = FALSE;
	start = 0;
	maxstart = 0;
	maxsize = 0;
	for(i = 0; i < h; i++) {
		line = data + i * wpl;
		val = GET_DATA_BIT(line, x);
		if(!inrun) {
			if(val) {
				start = i;
				inrun = TRUE;
			}
		}
		else if(!val) { /* run just ended */
			length = i - start;
			if(length > maxsize) {
				maxsize = length;
				maxstart = start;
			}
			inrun = FALSE;
		}
	}

	if(inrun) { /* a run has continued to the end of the column */
		length = i - start;
		if(length > maxsize) {
			maxsize = length;
			maxstart = start;
		}
	}
	if(pystart) *pystart = maxstart;
	*psize = maxsize;
	return 0;
}

/*-----------------------------------------------------------------------*
*            Compute runlength-to-membership transform on a line        *
*-----------------------------------------------------------------------*/
/*!
 * \brief   runlengthMembershipOnLine()
 *
 * \param[in]     buffer   into which full line of data is placed
 * \param[in]     size     full size of line; w or h
 * \param[in]     depth    8 or 16 bpp
 * \param[in]     start    array of start positions for fg runs
 * \param[in]     end      array of end positions for fg runs
 * \param[in]     n        the number of runs
 * \return   0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Converts a set of runlengths into a buffer of
 *          runlength membership values.
 *      (2) Initialization of the array gives pixels that are
 *          not within a run the value 0.
 * </pre>
 */
l_ok runlengthMembershipOnLine(int32 * buffer, int32 size, int32 depth, int32 * start, int32 * end, int32 n)
{
	int32 i, j, first, last, diff, max;
	PROCNAME(__FUNCTION__);
	if(!buffer)
		return ERROR_INT("buffer not defined", procName, 1);
	if(!start)
		return ERROR_INT("start not defined", procName, 1);
	if(!end)
		return ERROR_INT("end not defined", procName, 1);
	if(depth == 8)
		max = 0xff;
	else /* depth == 16 */
		max = 0xffff;
	memzero(buffer, 4 * size);
	for(i = 0; i < n; i++) {
		first = start[i];
		last = end[i];
		diff = last - first + 1;
		diff = MIN(diff, max);
		for(j = first; j <= last; j++)
			buffer[j] = diff;
	}

	return 0;
}

/*-----------------------------------------------------------------------*
*                       Make byte position LUT                          *
*-----------------------------------------------------------------------*/
/*!
 * \brief   makeMSBitLocTab()
 *
 * \param[in]    bitval   either 0 or 1
 * \return  table:  for an input byte, the MS bit location, starting at 0
 *                  with the MSBit in the byte, or NULL on error.
 *
 * <pre>
 * Notes:
 *      (1) If %bitval == 1, it finds the leftmost ON pixel in a byte;
 *          otherwise if %bitval == 0, it finds the leftmost OFF pixel.
 *      (2) If there are no pixels of the indicated color in the byte,
 *          this returns 8.
 * </pre>
 */
int32 * makeMSBitLocTab(int32 bitval)
{
	int32 i, j;
	uint8 byte, mask;
	int32 * tab = (int32*)SAlloc::C(256, sizeof(int32));
	for(i = 0; i < 256; i++) {
		byte = (uint8)i;
		if(bitval == 0)
			byte = ~byte;
		tab[i] = 8;
		mask = 0x80;
		for(j = 0; j < 8; j++) {
			if(byte & mask) {
				tab[i] = j;
				break;
			}
			mask >>= 1;
		}
	}
	return tab;
}
