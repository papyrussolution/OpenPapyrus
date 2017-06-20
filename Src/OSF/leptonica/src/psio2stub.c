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
 * \file psio2stub.c
 * <pre>
 *
 *     Stubs for psio2.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_PSIO   /* defined in environ.h */
/* --------------------------------------------*/

int32 pixWritePSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "pixWritePSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteStreamPS(FILE *fp, PIX *pix, BOX *box, int32 res,
                         float scale)
{
    return ERROR_INT("function not present", "pixWriteStreamPS", 1);
}

/* ----------------------------------------------------------------------*/

char * pixWriteStringPS(PIX *pixs, BOX *box, int32 res, float scale)
{
    return (char *)ERROR_PTR("function not present", "pixWriteStringPS", NULL);
}

/* ----------------------------------------------------------------------*/

char * generateUncompressedPS(char *hexdata, int32 w, int32 h, int32 d,
                              int32 psbpl, int32 bps, float xpt,
                              float ypt, float wpt, float hpt,
                              int32 boxflag)
{
    return (char *)ERROR_PTR("function not present",
                             "generateUncompressedPS", NULL);
}

/* ----------------------------------------------------------------------*/

void getScaledParametersPS(BOX *box, int32 wpix, int32 hpix, int32 res,
                           float scale, float *pxpt, float *pypt,
                           float *pwpt, float *phpt)
{
    L_ERROR("function not present\n", "getScaledParametersPS");
    return;
}

/* ----------------------------------------------------------------------*/

void convertByteToHexAscii(uint8 byteval, char *pnib1, char *pnib2)
{
    L_ERROR("function not present\n", "convertByteToHexAscii");
    return;
}

/* ----------------------------------------------------------------------*/

int32 convertJpegToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertJpegToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertJpegToPS(const char *filein, const char *fileout,
                        const char *operation, int32 x, int32 y,
                        int32 res, float scale, int32 pageno,
                        int32 endpage)
{
    return ERROR_INT("function not present", "convertJpegToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertJpegToPSString(const char *filein, char **poutstr,
                              int32 *pnbytes, int32 x, int32 y,
                              int32 res, float scale, int32 pageno,
                              int32 endpage)
{
    return ERROR_INT("function not present", "convertJpegToPSString", 1);
}

/* ----------------------------------------------------------------------*/

char * generateJpegPS(const char *filein, L_COMP_DATA *cid,
                      float xpt, float ypt, float wpt,
                      float hpt, int32 pageno, int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateJpegPS", NULL);
}

/* ----------------------------------------------------------------------*/

int32 convertG4ToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertG4ToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertG4ToPS(const char *filein, const char *fileout,
                      const char *operation, int32 x, int32 y,
                      int32 res, float scale, int32 pageno,
                      int32 maskflag, int32 endpage)
{
    return ERROR_INT("function not present", "convertG4ToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertG4ToPSString(const char *filein, char **poutstr,
                            int32 *pnbytes, int32 x, int32 y,
                            int32 res, float scale, int32 pageno,
                            int32 maskflag, int32 endpage)
{
    return ERROR_INT("function not present", "convertG4ToPSString", 1);
}

/* ----------------------------------------------------------------------*/

char * generateG4PS(const char *filein, L_COMP_DATA *cid, float xpt,
                    float ypt, float wpt, float hpt,
                    int32 maskflag, int32 pageno, int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateG4PS", NULL);
}

/* ----------------------------------------------------------------------*/

int32 convertTiffMultipageToPS(const char *filein, const char *fileout,
                                 float fillfract)
{
    return ERROR_INT("function not present", "convertTiffMultipageToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertFlateToPSEmbed(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertFlateToPSEmbed", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertFlateToPS(const char *filein, const char *fileout,
                         const char *operation, int32 x, int32 y,
                         int32 res, float scale, int32 pageno,
                         int32 endpage)
{
    return ERROR_INT("function not present", "convertFlateToPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertFlateToPSString(const char *filein, char **poutstr,
                               int32 *pnbytes, int32 x, int32 y,
                               int32 res, float scale,
                               int32 pageno, int32 endpage)
{
    return ERROR_INT("function not present", "convertFlateToPSString", 1);
}

/* ----------------------------------------------------------------------*/

char * generateFlatePS(const char *filein, L_COMP_DATA *cid,
                       float xpt, float ypt, float wpt,
                       float hpt, int32 pageno, int32 endpage)
{
    return (char *)ERROR_PTR("function not present", "generateFlatePS", NULL);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteMemPS(uint8 **pdata, size_t *psize, PIX *pix, BOX *box,
                      int32 res, float scale)
{
    return ERROR_INT("function not present", "pixWriteMemPS", 1);
}

/* ----------------------------------------------------------------------*/

int32 getResLetterPage(int32 w, int32 h, float fillfract)
{
    return ERROR_INT("function not present", "getResLetterPage", 1);
}

/* ----------------------------------------------------------------------*/

int32 getResA4Page(int32 w, int32 h, float fillfract)
{
    return ERROR_INT("function not present", "getResA4Page", 1);
}

/* ----------------------------------------------------------------------*/

void l_psWriteBoundingBox(int32 flag)
{
    L_ERROR("function not present\n", "l_psWriteBoundingBox");
    return;
}

/* --------------------------------------------*/
#endif  /* !USE_PSIO */
/* --------------------------------------------*/
