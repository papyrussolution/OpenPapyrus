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
 * \file pdfio2stub.c
 * <pre>
 *
 *     Stubs for pdfio2.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_PDFIO   /* defined in environ.h */
/* --------------------------------------------*/

/* ----------------------------------------------------------------------*/

int32 pixConvertToPdfData(PIX *pix, int32 type, int32 quality,
                            uint8 **pdata, size_t *pnbytes,
                            int32 x, int32 y, int32 res,
                            const char *title,
                            L_PDF_DATA **plpd, int32 position)
{
    return ERROR_INT("function not present", "pixConvertToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

int32 ptraConcatenatePdfToData(L_PTRA *pa_data, SARRAY *sa,
                                 uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "ptraConcatenatePdfToData", 1);
}

/* ----------------------------------------------------------------------*/

int32 convertTiffMultipageToPdf(const char *filein, const char *fileout)
{
    return ERROR_INT("function not present", "convertTiffMultipageToPdf", 1);
}

/* ----------------------------------------------------------------------*/

int32 l_generateCIDataForPdf(const char *fname, PIX *pix, int32 quality,
                               L_COMP_DATA **pcid)
{
    return ERROR_INT("function not present", "l_generateCIDataForPdf", 1);
}

/* ----------------------------------------------------------------------*/

L_COMP_DATA * l_generateFlateDataPdf(const char *fname, PIX *pix)
{
    return (L_COMP_DATA *)ERROR_PTR("function not present",
                                    "l_generateFlateDataPdf", NULL);
}

/* ----------------------------------------------------------------------*/

L_COMP_DATA * l_generateJpegData(const char *fname, int32 ascii85flag)
{
    return (L_COMP_DATA *)ERROR_PTR("function not present",
                                    "l_generateJpegData", NULL);
}

/* ----------------------------------------------------------------------*/

L_COMP_DATA * l_generateJp2kData(const char *fname)
{
    return (L_COMP_DATA *)ERROR_PTR("function not present",
                                    "l_generateJp2kData", NULL);
}

/* ----------------------------------------------------------------------*/

int32 l_generateCIData(const char *fname, int32 type, int32 quality,
                         int32 ascii85, L_COMP_DATA **pcid)
{
    return ERROR_INT("function not present", "l_generateCIData", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixGenerateCIData(PIX *pixs, int32 type, int32 quality,
                          int32 ascii85, L_COMP_DATA **pcid)
{
    return ERROR_INT("function not present", "pixGenerateCIData", 1);
}

/* ----------------------------------------------------------------------*/

L_COMP_DATA * l_generateFlateData(const char *fname, int32 ascii85flag)
{
    return (L_COMP_DATA *)ERROR_PTR("function not present",
                                    "l_generateFlateData", NULL);
}

/* ----------------------------------------------------------------------*/

L_COMP_DATA * l_generateG4Data(const char *fname, int32 ascii85flag)
{
    return (L_COMP_DATA *)ERROR_PTR("function not present",
                                    "l_generateG4Data", NULL);
}

/* ----------------------------------------------------------------------*/

int32 cidConvertToPdfData(L_COMP_DATA *cid, const char *title,
                            uint8 **pdata, size_t *pnbytes)
{
    return ERROR_INT("function not present", "cidConvertToPdfData", 1);
}

/* ----------------------------------------------------------------------*/

void l_CIDataDestroy(L_COMP_DATA  **pcid)
{
    L_ERROR("function not present\n", "l_CIDataDestroy");
    return;
}

/* ----------------------------------------------------------------------*/

void l_pdfSetG4ImageMask(int32 flag)
{
    L_ERROR("function not present\n", "l_pdfSetG4ImageMask");
    return;
}

/* ----------------------------------------------------------------------*/

void l_pdfSetDateAndVersion(int32 flag)
{
    L_ERROR("function not present\n", "l_pdfSetDateAndVersion");
    return;
}

/* ----------------------------------------------------------------------*/

/* --------------------------------------------*/
#endif  /* !USE_PDFIO */
/* --------------------------------------------*/
