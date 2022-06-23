// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file jpegiostub.c
 * <pre>
 *
 *     Stubs for jpegio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !HAVE_LIBJPEG   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadJpeg(const char * filename, int32 cmflag, int32 reduction,
    int32 * pnwarn, int32 hint)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadJpeg", NULL);
}

PIX * pixReadStreamJpeg(FILE * fp, int32 cmflag, int32 reduction,
    int32 * pnwarn, int32 hint)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadStreamJpeg", NULL);
}

l_ok readHeaderJpeg(const char * filename, int32 * pw, int32 * ph,
    int32 * pspp, int32 * pycck, int32 * pcmyk)
{
	return ERROR_INT("function not present", "readHeaderJpeg", 1);
}

l_ok freadHeaderJpeg(FILE * fp, int32 * pw, int32 * ph,
    int32 * pspp, int32 * pycck, int32 * pcmyk)
{
	return ERROR_INT("function not present", "freadHeaderJpeg", 1);
}

int32 fgetJpegResolution(FILE * fp, int32 * pxres, int32 * pyres)
{
	return ERROR_INT("function not present", "fgetJpegResolution", 1);
}

int32 fgetJpegComment(FILE * fp, uint8 ** pcomment)
{
	return ERROR_INT("function not present", "fgetJpegComment", 1);
}

l_ok pixWriteJpeg(const char * filename, PIX * pix, int32 quality,
    int32 progressive)
{
	return ERROR_INT("function not present", "pixWriteJpeg", 1);
}

l_ok pixWriteStreamJpeg(FILE * fp, PIX * pix, int32 quality,
    int32 progressive)
{
	return ERROR_INT("function not present", "pixWriteStreamJpeg", 1);
}

PIX * pixReadMemJpeg(const uint8 * cdata, size_t size, int32 cmflag,
    int32 reduction, int32 * pnwarn, int32 hint)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadMemJpeg", NULL);
}

l_ok readHeaderMemJpeg(const uint8 * cdata, size_t size,
    int32 * pw, int32 * ph, int32 * pspp,
    int32 * pycck, int32 * pcmyk)
{
	return ERROR_INT("function not present", "readHeaderMemJpeg", 1);
}

l_ok readResolutionMemJpeg(const uint8 * data, size_t size,
    int32 * pxres, int32 * pyres)
{
	return ERROR_INT("function not present", "readResolutionMemJpeg", 1);
}

l_ok pixWriteMemJpeg(uint8 ** pdata, size_t * psize, PIX * pix,
    int32 quality, int32 progressive)
{
	return ERROR_INT("function not present", "pixWriteMemJpeg", 1);
}

l_ok pixSetChromaSampling(PIX * pix, int32 sampling)
{
	return ERROR_INT("function not present", "pixSetChromaSampling", 1);
}

#endif  /* !HAVE_LIBJPEG */
