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
 * \file convertfiles.c
 * <pre>
 *
 *      Conversion to 1 bpp
 *          int32    convertFilesTo1bpp()
 *
 *  These are utility functions that will perform depth conversion
 *  on selected files, writing the results to a specified directory.
 *  We start with conversion to 1 bpp.
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/*------------------------------------------------------------------*
*                        Conversion to 1 bpp                       *
*------------------------------------------------------------------*/
/*!
 * \brief   convertFilesTo1bpp()
 *
 * \param[in]    dirin
 * \param[in]    substr       [optional] substring filter on filenames;
   8                            can be NULL
 * \param[in]    upscaling    1, 2 or 4; only for input color or grayscale
 * \param[in]    thresh       global threshold for binarization; 0 for default
 * \param[in]    firstpage
 * \param[in]    npages       use 0 to do all from %firstpage to the end
 * \param[in]    dirout
 * \param[in]    outformat    IFF_PNG, IFF_TIFF_G4
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Images are sorted lexicographically, and the names in the
 *          output directory are retained except for the extension.
 * </pre>
 */
l_ok convertFilesTo1bpp(const char * dirin,
    const char * substr,
    int32 upscaling,
    int32 thresh,
    int32 firstpage,
    int32 npages,
    const char * dirout,
    int32 outformat)
{
	int32 i, nfiles;
	char buf[512];
	char * fname, * tail, * basename;
	PIX * pixs, * pixg1, * pixg2, * pixb;
	SARRAY * safiles;

	PROCNAME(__FUNCTION__);

	if(!dirin)
		return ERROR_INT("dirin", procName, 1);
	if(!dirout)
		return ERROR_INT("dirout", procName, 1);
	if(upscaling != 1 && upscaling != 2 && upscaling != 4)
		return ERROR_INT("invalid upscaling factor", procName, 1);
	if(thresh <= 0) thresh = 180;
	if(firstpage < 0) firstpage = 0;
	if(npages < 0) npages = 0;
	if(outformat != IFF_TIFF_G4)
		outformat = IFF_PNG;

	safiles = getSortedPathnamesInDirectory(dirin, substr, firstpage, npages);
	if(!safiles)
		return ERROR_INT("safiles not made", procName, 1);
	if((nfiles = sarrayGetCount(safiles)) == 0) {
		sarrayDestroy(&safiles);
		return ERROR_INT("no matching files in the directory", procName, 1);
	}

	for(i = 0; i < nfiles; i++) {
		fname = sarrayGetString(safiles, i, L_NOCOPY);
		if((pixs = pixRead(fname)) == NULL) {
			L_WARNING("Couldn't read file %s\n", procName, fname);
			continue;
		}
		if(pixGetDepth(pixs) == 32)
			pixg1 = pixConvertRGBToLuminance(pixs);
		else
			pixg1 = pixClone(pixs);
		pixg2 = pixRemoveColormap(pixg1, REMOVE_CMAP_TO_GRAYSCALE);
		if(pixGetDepth(pixg2) == 1) {
			pixb = pixClone(pixg2);
		}
		else {
			if(upscaling == 1)
				pixb = pixThresholdToBinary(pixg2, thresh);
			else if(upscaling == 2)
				pixb = pixScaleGray2xLIThresh(pixg2, thresh);
			else /* upscaling == 4 */
				pixb = pixScaleGray4xLIThresh(pixg2, thresh);
		}
		pixDestroy(&pixs);
		pixDestroy(&pixg1);
		pixDestroy(&pixg2);

		splitPathAtDirectory(fname, NULL, &tail);
		splitPathAtExtension(tail, &basename, NULL);
		if(outformat == IFF_TIFF_G4) {
			snprintf(buf, sizeof(buf), "%s/%s.tif", dirout, basename);
			pixWrite(buf, pixb, IFF_TIFF_G4);
		}
		else {
			snprintf(buf, sizeof(buf), "%s/%s.png", dirout, basename);
			pixWrite(buf, pixb, IFF_PNG);
		}
		pixDestroy(&pixb);
		SAlloc::F(tail);
		SAlloc::F(basename);
	}

	sarrayDestroy(&safiles);
	return 0;
}
