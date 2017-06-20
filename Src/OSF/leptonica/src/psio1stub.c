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
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

int32 convertFilesToPS(const char *dirin, const char *substr,
                         int32 res, const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 sarrayConvertFilesToPS(SARRAY *sa, int32 res, const char *fileout)
{
    return ERROR_INT("function not present", "sarrayConvertFilesToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertFilesFittedToPS(const char *dirin, const char *substr,
                               float xpts, float ypts,
                               const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesFittedToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 sarrayConvertFilesFittedToPS(SARRAY *sa, float xpts,
                                     float ypts, const char *fileout)
{
    return ERROR_INT("function not present", "sarrayConvertFilesFittedToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 writeImageCompressedToPSFile(const char *filein, const char *fileout,
                                     int32 res, int32 *pfirstfile,
                                     int32 *pindex)
{
    return ERROR_INT("function not present", "writeImageCompressedToPSFile", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertSegmentedPagesToPS(const char *pagedir, const char *pagestr,
                                  int32 page_numpre, const char *maskdir,
                                  const char *maskstr, int32 mask_numpre,
                                  int32 numpost, int32 maxnum,
                                  float textscale, float imagescale,
                                  int32 threshold, const char *fileout)
{
    return ERROR_INT("function not present", "convertSegmentedPagesToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteSegmentedPageToPS(PIX *pixs, PIX *pixm, float textscale,
                                  float imagescale, int32 threshold,
                                  int32 pageno, const char *fileout)
{
    return ERROR_INT("function not present", "pixWriteSegmentedPagesToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteMixedToPS(PIX *pixb, PIX *pixc, float scale,
                          int32 pageno, const char *fileout)
{
    return ERROR_INT("function not present", "pixWriteMixedToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertToPSEmbed(const char *filein, const char *fileout, int32 level)
{
    return ERROR_INT("function not present", "convertToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixaWriteCompressedToPS(PIXA *pixa, const char *fileout,
                                int32 res, int32 level)
{
    return ERROR_INT("function not present", "pixaWriteCompressedtoPS", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_PSIO */
/* --------------------------------------------*/
