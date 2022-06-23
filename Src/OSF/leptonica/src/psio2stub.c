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

l_ok pixWritePSEmbed(const char * filein, const char * fileout)
{
	return ERROR_INT("function not present", "pixWritePSEmbed", 1);
}

l_ok pixWriteStreamPS(FILE * fp, PIX * pix, BOX * box, int32 res,
    float scale)
{
	return ERROR_INT("function not present", "pixWriteStreamPS", 1);
}

char * pixWriteStringPS(PIX * pixs, BOX * box, int32 res, float scale)
{
	return (char *)ERROR_PTR("function not present", "pixWriteStringPS", NULL);
}

char * generateUncompressedPS(char * hexdata, int32 w, int32 h, int32 d,
    int32 psbpl, int32 bps, float xpt,
    float ypt, float wpt, float hpt,
    int32 boxflag)
{
	return (char *)ERROR_PTR("function not present",
		   "generateUncompressedPS", NULL);
}

l_ok convertJpegToPSEmbed(const char * filein, const char * fileout)
{
	return ERROR_INT("function not present", "convertJpegToPSEmbed", 1);
}

l_ok convertJpegToPS(const char * filein, const char * fileout,
    const char * operation, int32 x, int32 y,
    int32 res, float scale, int32 pageno,
    int32 endpage)
{
	return ERROR_INT("function not present", "convertJpegToPS", 1);
}

l_ok convertG4ToPSEmbed(const char * filein, const char * fileout)
{
	return ERROR_INT("function not present", "convertG4ToPSEmbed", 1);
}

l_ok convertG4ToPS(const char * filein, const char * fileout,
    const char * operation, int32 x, int32 y,
    int32 res, float scale, int32 pageno,
    int32 maskflag, int32 endpage)
{
	return ERROR_INT("function not present", "convertG4ToPS", 1);
}

l_ok convertTiffMultipageToPS(const char * filein, const char * fileout,
    float fillfract)
{
	return ERROR_INT("function not present", "convertTiffMultipageToPS", 1);
}

l_ok convertFlateToPSEmbed(const char * filein, const char * fileout)
{
	return ERROR_INT("function not present", "convertFlateToPSEmbed", 1);
}

l_ok convertFlateToPS(const char * filein, const char * fileout,
    const char * operation, int32 x, int32 y,
    int32 res, float scale, int32 pageno,
    int32 endpage)
{
	return ERROR_INT("function not present", "convertFlateToPS", 1);
}

l_ok pixWriteMemPS(uint8 ** pdata, size_t * psize, PIX * pix, BOX * box,
    int32 res, float scale)
{
	return ERROR_INT("function not present", "pixWriteMemPS", 1);
}

int32 getResLetterPage(int32 w, int32 h, float fillfract)
{
	return ERROR_INT("function not present", "getResLetterPage", 1);
}

void l_psWriteBoundingBox(int32 flag)
{
	L_ERROR("function not present\n", "l_psWriteBoundingBox");
	return;
}

#endif  /* !USE_PSIO */
