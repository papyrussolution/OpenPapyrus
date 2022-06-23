// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file pngiostub.c
 * <pre>
 *
 *     Stubs for pngio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamPng(FILE * fp)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadStreamPng", NULL);
}

l_ok readHeaderPng(const char * filename, int32 * pwidth, int32 * pheight,
    int32 * pbps, int32 * pspp, int32 * piscmap)
{
	return ERROR_INT("function not present", "readHeaderPng", 1);
}

l_ok freadHeaderPng(FILE * fp, int32 * pwidth, int32 * pheight,
    int32 * pbps, int32 * pspp, int32 * piscmap)
{
	return ERROR_INT("function not present", "freadHeaderPng", 1);
}

l_ok readHeaderMemPng(const uint8 * data, size_t size, int32 * pwidth,
    int32 * pheight, int32 * pbps, int32 * pspp,
    int32 * piscmap)
{
	return ERROR_INT("function not present", "readHeaderMemPng", 1);
}

int32 fgetPngResolution(FILE * fp, int32 * pxres, int32 * pyres)
{
	return ERROR_INT("function not present", "fgetPngResolution", 1);
}

l_ok isPngInterlaced(const char * filename, int32 * pinterlaced)
{
	return ERROR_INT("function not present", "isPngInterlaced", 1);
}

l_ok fgetPngColormapInfo(FILE * fp, PIXCMAP ** pcmap, int32 * ptransparency)
{
	return ERROR_INT("function not present", "fgetPngColormapInfo", 1);
}

l_ok pixWritePng(const char * filename, PIX * pix, float gamma)
{
	return ERROR_INT("function not present", "pixWritePng", 1);
}

l_ok pixWriteStreamPng(FILE * fp, PIX * pix, float gamma)
{
	return ERROR_INT("function not present", "pixWriteStreamPng", 1);
}

l_ok pixSetZlibCompression(PIX * pix, int32 compval)

{
	return ERROR_INT("function not present", "pixSetZlibCompression", 1);
}

void l_pngSetReadStrip16To8(int32 flag)
{
	L_ERROR("function not present\n", "l_pngSetReadStrip16To8");
	return;
}

PIX * pixReadMemPng(const uint8 * filedata, size_t filesize)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadMemPng", NULL);
}

l_ok pixWriteMemPng(uint8 ** pfiledata, size_t * pfilesize, PIX * pix,
    float gamma)
{
	return ERROR_INT("function not present", "pixWriteMemPng", 1);
}

#endif  /* !HAVE_LIBPNG */
