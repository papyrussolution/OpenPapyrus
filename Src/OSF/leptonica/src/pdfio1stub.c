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

l_ok convertFilesToPdf(const char * dirname, const char * substr,
    int32 res, float scalefactor,
    int32 type, int32 quality,
    const char * title, const char * fileout)
{
	return ERROR_INT("function not present", "convertFilesToPdf", 1);
}

l_ok saConvertFilesToPdf(SARRAY * sa, int32 res, float scalefactor,
    int32 type, int32 quality,
    const char * title, const char * fileout)
{
	return ERROR_INT("function not present", "saConvertFilesToPdf", 1);
}

l_ok saConvertFilesToPdfData(SARRAY * sa, int32 res,
    float scalefactor, int32 type,
    int32 quality, const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "saConvertFilesToPdfData", 1);
}

l_ok selectDefaultPdfEncoding(PIX * pix, int32 * ptype)
{
	return ERROR_INT("function not present", "selectDefaultPdfEncoding", 1);
}

l_ok convertUnscaledFilesToPdf(const char * dirname, const char * substr,
    const char * title, const char * fileout)
{
	return ERROR_INT("function not present", "convertUnscaledFilesToPdf", 1);
}

l_ok saConvertUnscaledFilesToPdf(SARRAY * sa, const char * title,
    const char * fileout)
{
	return ERROR_INT("function not present", "saConvertUnscaledFilesToPdf", 1);
}

l_ok saConvertUnscaledFilesToPdfData(SARRAY * sa, const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present",
		   "saConvertUnscaledFilesToPdfData", 1);
}

l_ok convertUnscaledToPdfData(const char * fname, const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "convertUnscaledToPdfData", 1);
}

l_ok pixaConvertToPdf(PIXA * pixa, int32 res, float scalefactor,
    int32 type, int32 quality,
    const char * title, const char * fileout)
{
	return ERROR_INT("function not present", "pixaConvertToPdf", 1);
}

l_ok pixaConvertToPdfData(PIXA * pixa, int32 res, float scalefactor,
    int32 type, int32 quality, const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "pixaConvertToPdfData", 1);
}

l_ok convertToPdf(const char * filein,
    int32 type, int32 quality,
    const char * fileout,
    int32 x, int32 y, int32 res,
    const char * title,
    L_PDF_DATA ** plpd, int32 position)
{
	return ERROR_INT("function not present", "convertToPdf", 1);
}

l_ok convertImageDataToPdf(uint8 * imdata, size_t size,
    int32 type, int32 quality,
    const char * fileout,
    int32 x, int32 y, int32 res,
    const char * title,
    L_PDF_DATA ** plpd, int32 position)
{
	return ERROR_INT("function not present", "convertImageDataToPdf", 1);
}

l_ok convertToPdfData(const char * filein,
    int32 type, int32 quality,
    uint8 ** pdata, size_t * pnbytes,
    int32 x, int32 y, int32 res,
    const char * title,
    L_PDF_DATA ** plpd, int32 position)
{
	return ERROR_INT("function not present", "convertToPdfData", 1);
}

l_ok convertImageDataToPdfData(uint8 * imdata, size_t size,
    int32 type, int32 quality,
    uint8 ** pdata, size_t * pnbytes,
    int32 x, int32 y, int32 res,
    const char * title,
    L_PDF_DATA ** plpd, int32 position)
{
	return ERROR_INT("function not present", "convertImageDataToPdfData", 1);
}

l_ok pixConvertToPdf(PIX * pix, int32 type, int32 quality,
    const char * fileout,
    int32 x, int32 y, int32 res,
    const char * title,
    L_PDF_DATA ** plpd, int32 position)
{
	return ERROR_INT("function not present", "pixConvertToPdf", 1);
}

l_ok pixWriteStreamPdf(FILE * fp, PIX * pix, int32 res, const char * title)
{
	return ERROR_INT("function not present", "pixWriteStreamPdf", 1);
}

l_ok pixWriteMemPdf(uint8 ** pdata, size_t * pnbytes, PIX * pix,
    int32 res, const char * title)
{
	return ERROR_INT("function not present", "pixWriteMemPdf", 1);
}

l_ok convertSegmentedFilesToPdf(const char * dirname, const char * substr,
    int32 res, int32 type, int32 thresh,
    BOXAA * baa, int32 quality,
    float scalefactor, const char * title,
    const char * fileout)
{
	return ERROR_INT("function not present", "convertSegmentedFilesToPdf", 1);
}

BOXAA * convertNumberedMasksToBoxaa(const char * dirname, const char * substr,
    int32 numpre, int32 numpost)
{
	return (BOXAA*)ERROR_PTR("function not present",
		   "convertNumberedMasksToBoxaa", NULL);
}

l_ok convertToPdfSegmented(const char * filein, int32 res, int32 type,
    int32 thresh, BOXA * boxa, int32 quality,
    float scalefactor, const char * title,
    const char * fileout)
{
	return ERROR_INT("function not present", "convertToPdfSegmented", 1);
}

l_ok pixConvertToPdfSegmented(PIX * pixs, int32 res, int32 type,
    int32 thresh, BOXA * boxa, int32 quality,
    float scalefactor, const char * title,
    const char * fileout)
{
	return ERROR_INT("function not present", "pixConvertToPdfSegmented", 1);
}

l_ok convertToPdfDataSegmented(const char * filein, int32 res,
    int32 type, int32 thresh, BOXA * boxa,
    int32 quality, float scalefactor,
    const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "convertToPdfDataSegmented", 1);
}

l_ok pixConvertToPdfDataSegmented(PIX * pixs, int32 res, int32 type,
    int32 thresh, BOXA * boxa,
    int32 quality, float scalefactor,
    const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "pixConvertToPdfDataSegmented", 1);
}

l_ok concatenatePdf(const char * dirname, const char * substr,
    const char * fileout)
{
	return ERROR_INT("function not present", "concatenatePdf", 1);
}

l_ok saConcatenatePdf(SARRAY * sa, const char * fileout)
{
	return ERROR_INT("function not present", "saConcatenatePdf", 1);
}

l_ok ptraConcatenatePdf(L_PTRA * pa, const char * fileout)
{
	return ERROR_INT("function not present", "ptraConcatenatePdf", 1);
}

l_ok concatenatePdfToData(const char * dirname, const char * substr,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "concatenatePdfToData", 1);
}

l_ok saConcatenatePdfToData(SARRAY * sa, uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "saConcatenatePdfToData", 1);
}

#endif  /* !USE_PDFIO */
