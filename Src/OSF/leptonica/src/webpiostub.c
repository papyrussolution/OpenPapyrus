// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file webpiostub.c
 * <pre>
 *
 *     Stubs for webpio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !HAVE_LIBWEBP   /* defined in environ.h   */
/* --------------------------------------------*/

PIX * pixReadStreamWebP(FILE * fp)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadStreamWebP", NULL);
}

PIX * pixReadMemWebP(const uint8 * filedata, size_t filesize)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadMemWebP", NULL);
}

l_ok readHeaderWebP(const char * filename, int32 * pw, int32 * ph,
    int32 * pspp)
{
	return ERROR_INT("function not present", "readHeaderWebP", 1);
}

l_ok readHeaderMemWebP(const uint8 * data, size_t size,
    int32 * pw, int32 * ph, int32 * pspp)
{
	return ERROR_INT("function not present", "readHeaderMemWebP", 1);
}

l_ok pixWriteWebP(const char * filename, PIX * pixs, int32 quality,
    int32 lossless)
{
	return ERROR_INT("function not present", "pixWriteWebP", 1);
}

l_ok pixWriteStreamWebP(FILE * fp, PIX * pixs, int32 quality,
    int32 lossless)
{
	return ERROR_INT("function not present", "pixWriteStreamWebP", 1);
}

l_ok pixWriteMemWebP(uint8 ** pencdata, size_t * pencsize, PIX * pixs,
    int32 quality, int32 lossless)
{
	return ERROR_INT("function not present", "pixWriteMemWebP", 1);
}

#endif  /* !HAVE_LIBWEBP */
