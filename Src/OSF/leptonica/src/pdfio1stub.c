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
 * \file pdfio1stub.c
 * <pre>
 *
 *     Stubs for pdfio1.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_PDFIO   /* defined in environ.h */
/* --------------------------------------------*/

/* ----------------------------------------------------------------------*/

int32 convertFilesToPdf(const char *dirname, const char *substr,
                          int32 res, float scalefactor,
                          int32 type, int32 quality,
                          const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "convertFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 saConvertFilesToPdf(SARRAY *sa, int32 res, float scalefactor,
                            int32 type, int32 quality,
                            const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "saConvertFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 saConvertFilesToPdfData(SARRAY *sa, int32 res,
                                float scalefactor, int32 type,
                                int32 quality, const char *title,
                                uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "saConvertFilesToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 selectDefaultPdfEncoding(PIX *pix, int32 *ptype)
{
    return ERROR_INT("function not present", "selectDefaultPdfEncoding", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertUnscaledFilesToPdf(const char *dirname, const char *substr,
                                  const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "convertUnscaledFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 saConvertUnscaledFilesToPdf(SARRAY *sa, const char *title,
                                    const char *fileout)
{
    return ERROR_INT("function not present", "saConvertUnscaledFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 saConvertUnscaledFilesToPdfData(SARRAY *sa, const char *title,
                                        uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present",
                     "saConvertUnscaledFilesToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertUnscaledToPdfData(const char *fname, const char *title,
                                 uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "convertUnscaledToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixaConvertToPdf(PIXA *pixa, int32 res, float scalefactor,
                         int32 type, int32 quality,
                         const char *title, const char *fileout)
{
    return ERROR_INT("function not present", "pixaConvertToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixaConvertToPdfData(PIXA *pixa, int32 res, float scalefactor,
                             int32 type, int32 quality, const char *title,
                             uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "pixaConvertToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertToPdf(const char *filein,
                     int32 type, int32 quality,
                     const char *fileout,
                     int32 x, int32 y, int32 res,
                     const char *title,
                     L_PDF_DATA **plpd, int32 position)
{
    return ERROR_INT("function not present", "convertToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertImageDataToPdf(uint8 *imdata, size_t size,
                              int32 type, int32 quality,
                              const char *fileout,
                              int32 x, int32 y, int32 res,
                              const char *title,
                              L_PDF_DATA **plpd, int32 position)
{
    return ERROR_INT("function not present", "convertImageDataToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertToPdfData(const char *filein,
                         int32 type, int32 quality,
                         uint8 **pdata, size_t *pnbytes,
                         int32 x, int32 y, int32 res,
                         const char *title,
                         L_PDF_DATA **plpd, int32 position)
{
    return ERROR_INT("function not present", "convertToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertImageDataToPdfData(uint8 *imdata, size_t size,
                                  int32 type, int32 quality,
                                  uint8 **pdata, size_t *pnbytes,
                                  int32 x, int32 y, int32 res,
                                  const char *title,
                                  L_PDF_DATA **plpd, int32 position)
{
    return ERROR_INT("function not present", "convertImageDataToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixConvertToPdf(PIX *pix, int32 type, int32 quality,
                        const char *fileout,
                        int32 x, int32 y, int32 res,
                        const char *title,
                        L_PDF_DATA **plpd, int32 position)
{
    return ERROR_INT("function not present", "pixConvertToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteStreamPdf(FILE *fp, PIX *pix, int32 res, const char *title)
{
    return ERROR_INT("function not present", "pixWriteStreamPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteMemPdf(uint8 **pdata, size_t *pnbytes, PIX *pix,
                       int32 res, const char *title)
{
    return ERROR_INT("function not present", "pixWriteMemPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertSegmentedFilesToPdf(const char *dirname, const char *substr,
                                   int32 res, int32 type, int32 thresh,
                                   BOXAA *baa, int32 quality,
                                   float scalefactor, const char *title,
                                   const char *fileout)
{
    return ERROR_INT("function not present", "convertSegmentedFilesToPdf", 1);
}

/* ----------------------------------------------------------------------*/

BOXAA * convertNumberedMasksToBoxaa(const char *dirname, const char *substr,
                                    int32 numpre, int32 numpost)
{
    return (BOXAA *)ERROR_PTR("function not present",
                              "convertNumberedMasksToBoxaa", NULL);
}

/* ----------------------------------------------------------------------*/

int32 convertToPdfSegmented(const char *filein, int32 res, int32 type,
                              int32 thresh, BOXA *boxa, int32 quality,
                              float scalefactor, const char *title,
                              const char *fileout)
{
    return ERROR_INT("function not present", "convertToPdfSegmented", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixConvertToPdfSegmented(PIX *pixs, int32 res, int32 type,
                                 int32 thresh, BOXA *boxa, int32 quality,
                                 float scalefactor, const char *title,
                                 const char *fileout)
{
    return ERROR_INT("function not present", "pixConvertToPdfSegmented", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertToPdfDataSegmented(const char *filein, int32 res,
                                  int32 type, int32 thresh, BOXA *boxa,
                                  int32 quality, float scalefactor,
                                  const char *title,
                                  uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "convertToPdfDataSegmented", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixConvertToPdfDataSegmented(PIX *pixs, int32 res, int32 type,
                                     int32 thresh, BOXA *boxa,
                                     int32 quality, float scalefactor,
                                     const char *title,
                                     uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "pixConvertToPdfDataSegmented", 1);
}

/* ----------------------------------------------------------------------*/

int32 concatenatePdf(const char *dirname, const char *substr,
                       const char *fileout)
{
    return ERROR_INT("function not present", "concatenatePdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 saConcatenatePdf(SARRAY *sa, const char *fileout)
{
    return ERROR_INT("function not present", "saConcatenatePdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 ptraConcatenatePdf(L_PTRA *pa, const char *fileout)
{
    return ERROR_INT("function not present", "ptraConcatenatePdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 concatenatePdfToData(const char *dirname, const char *substr,
                             uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "concatenatePdfToData", 1);
}

/* ----------------------------------------------------------------------*/

int32 saConcatenatePdfToData(SARRAY *sa, uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "saConcatenatePdfToData", 1);
}

/* ----------------------------------------------------------------------*/

/* --------------------------------------------*/
#endif  /* !USE_PDFIO */
/* --------------------------------------------*/
