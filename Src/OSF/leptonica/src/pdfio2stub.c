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

l_ok pixConvertToPdfData(PIX * pix, int32 type, int32 quality,
    uint8 ** pdata, size_t * pnbytes,
    int32 x, int32 y, int32 res,
    const char * title,
    L_PDF_DATA ** plpd, int32 position)
{
	return ERROR_INT("function not present", "pixConvertToPdfData", 1);
}

l_ok ptraConcatenatePdfToData(L_PTRA * pa_data, SARRAY * sa,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "ptraConcatenatePdfToData", 1);
}

l_ok convertTiffMultipageToPdf(const char * filein, const char * fileout)
{
	return ERROR_INT("function not present", "convertTiffMultipageToPdf", 1);
}

l_ok l_generateCIDataForPdf(const char * fname, PIX * pix, int32 quality,
    L_COMP_DATA ** pcid)
{
	return ERROR_INT("function not present", "l_generateCIDataForPdf", 1);
}

L_COMP_DATA * l_generateFlateDataPdf(const char * fname, PIX * pix)
{
	return (L_COMP_DATA*)ERROR_PTR("function not present",
		   "l_generateFlateDataPdf", NULL);
}

L_COMP_DATA * l_generateJpegData(const char * fname, int32 ascii85flag)
{
	return (L_COMP_DATA*)ERROR_PTR("function not present",
		   "l_generateJpegData", NULL);
}

L_COMP_DATA * l_generateJpegDataMem(uint8 * data, size_t nbytes,
    int32 ascii85flag)
{
	return (L_COMP_DATA*)ERROR_PTR("function not present",
		   "l_generateJpegDataMem", NULL);
}

l_ok l_generateCIData(const char * fname, int32 type, int32 quality,
    int32 ascii85, L_COMP_DATA ** pcid)
{
	return ERROR_INT("function not present", "l_generateCIData", 1);
}

l_ok pixGenerateCIData(PIX * pixs, int32 type, int32 quality,
    int32 ascii85, L_COMP_DATA ** pcid)
{
	return ERROR_INT("function not present", "pixGenerateCIData", 1);
}

L_COMP_DATA * l_generateFlateData(const char * fname, int32 ascii85flag)
{
	return (L_COMP_DATA*)ERROR_PTR("function not present",
		   "l_generateFlateData", NULL);
}

L_COMP_DATA * l_generateG4Data(const char * fname, int32 ascii85flag)
{
	return (L_COMP_DATA*)ERROR_PTR("function not present",
		   "l_generateG4Data", NULL);
}

l_ok cidConvertToPdfData(L_COMP_DATA * cid, const char * title,
    uint8 ** pdata, size_t * pnbytes)
{
	return ERROR_INT("function not present", "cidConvertToPdfData", 1);
}

void l_CIDataDestroy(L_COMP_DATA  ** pcid)
{
	L_ERROR("function not present\n", "l_CIDataDestroy");
	return;
}

void l_pdfSetG4ImageMask(int32 flag)
{
	L_ERROR("function not present\n", "l_pdfSetG4ImageMask");
	return;
}

void l_pdfSetDateAndVersion(int32 flag)
{
	L_ERROR("function not present\n", "l_pdfSetDateAndVersion");
	return;
}

#endif  /* !USE_PDFIO */
