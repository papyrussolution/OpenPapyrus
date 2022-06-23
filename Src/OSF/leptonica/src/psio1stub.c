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
 * \file psio1stub.c
 * <pre>
 *
 *     Stubs for psio1.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_PSIO   /* defined in environ.h */
/* --------------------------------------------*/

l_ok convertFilesToPS(const char * dirin, const char * substr,
    int32 res, const char * fileout)
{
	return ERROR_INT("function not present", "convertFilesToPS", 1);
}

l_ok sarrayConvertFilesToPS(SARRAY * sa, int32 res, const char * fileout)
{
	return ERROR_INT("function not present", "sarrayConvertFilesToPS", 1);
}

l_ok convertFilesFittedToPS(const char * dirin, const char * substr,
    float xpts, float ypts,
    const char * fileout)
{
	return ERROR_INT("function not present", "convertFilesFittedToPS", 1);
}

l_ok sarrayConvertFilesFittedToPS(SARRAY * sa, float xpts,
    float ypts, const char * fileout)
{
	return ERROR_INT("function not present", "sarrayConvertFilesFittedToPS", 1);
}

l_ok writeImageCompressedToPSFile(const char * filein, const char * fileout,
    int32 res, int32 * pindex)
{
	return ERROR_INT("function not present", "writeImageCompressedToPSFile", 1);
}

l_ok convertSegmentedPagesToPS(const char * pagedir, const char * pagestr,
    int32 page_numpre, const char * maskdir,
    const char * maskstr, int32 mask_numpre,
    int32 numpost, int32 maxnum,
    float textscale, float imagescale,
    int32 threshold, const char * fileout)
{
	return ERROR_INT("function not present", "convertSegmentedPagesToPS", 1);
}

l_ok pixWriteSegmentedPageToPS(PIX * pixs, PIX * pixm, float textscale,
    float imagescale, int32 threshold,
    int32 pageno, const char * fileout)
{
	return ERROR_INT("function not present", "pixWriteSegmentedPagesToPS", 1);
}

l_ok pixWriteMixedToPS(PIX * pixb, PIX * pixc, float scale,
    int32 pageno, const char * fileout)
{
	return ERROR_INT("function not present", "pixWriteMixedToPS", 1);
}

l_ok convertToPSEmbed(const char * filein, const char * fileout, int32 level)
{
	return ERROR_INT("function not present", "convertToPSEmbed", 1);
}

l_ok pixaWriteCompressedToPS(PIXA * pixa, const char * fileout,
    int32 res, int32 level)
{
	return ERROR_INT("function not present", "pixaWriteCompressedtoPS", 1);
}

l_ok pixWriteCompressedToPS(PIX * pix, const char * fileout, int32 res,
    int32 level, int32 * pindex)
{
	return ERROR_INT("function not present", "pixWriteCompressedtoPS", 1);
}

#endif  /* !USE_PSIO */
