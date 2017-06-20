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
 * \file pngiostub.c
 * <pre>
 *
 *     Stubs for pngio.c functions
 * </pre>
 */
//#ifdef HAVE_CONFIG_H
//#include "config_auto.h"
//#endif  /* HAVE_CONFIG_H */

#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !HAVE_LIBPNG   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamPng(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamPng", NULL);
}

/* ----------------------------------------------------------------------*/

int32 readHeaderPng(const char *filename, int32 *pwidth, int32 *pheight,
                      int32 *pbps, int32 *pspp, int32 *piscmap)
{
    return ERROR_INT("function not present", "readHeaderPng", 1);
}

/* ----------------------------------------------------------------------*/

int32 freadHeaderPng(FILE *fp, int32 *pwidth, int32 *pheight,
                       int32 *pbps, int32 *pspp, int32 *piscmap)
{
    return ERROR_INT("function not present", "freadHeaderPng", 1);
}

/* ----------------------------------------------------------------------*/

int32 readHeaderMemPng(const uint8 *data, size_t size, int32 *pwidth,
                         int32 *pheight, int32 *pbps, int32 *pspp,
                         int32 *piscmap)
{
    return ERROR_INT("function not present", "readHeaderMemPng", 1);
}

/* ----------------------------------------------------------------------*/

int32 fgetPngResolution(FILE *fp, int32 *pxres, int32 *pyres)
{
    return ERROR_INT("function not present", "fgetPngResolution", 1);
}

/* ----------------------------------------------------------------------*/

int32 isPngInterlaced(const char *filename, int32 *pinterlaced)
{
    return ERROR_INT("function not present", "isPngInterlaced", 1);
}

/* ----------------------------------------------------------------------*/

int32 fgetPngColormapInfo(FILE *fp, PIXCMAP **pcmap, int32 *ptransparency)
{
    return ERROR_INT("function not present", "fgetPngColormapInfo", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWritePng(const char *filename, PIX *pix, float gamma)
{
    return ERROR_INT("function not present", "pixWritePng", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteStreamPng(FILE *fp, PIX *pix, float gamma)
{
    return ERROR_INT("function not present", "pixWriteStreamPng", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixSetZlibCompression(PIX *pix, int32 compval)

{
    return ERROR_INT("function not present", "pixSetZlibCompression", 1);
}

/* ----------------------------------------------------------------------*/

void l_pngSetReadStrip16To8(int32 flag)
{
    L_ERROR("function not present\n", "l_pngSetReadStrip16To8");
    return;
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemPng(const uint8 *cdata, size_t size)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemPng", NULL);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteMemPng(uint8 **pdata, size_t *psize, PIX *pix,
                       float gamma)
{
    return ERROR_INT("function not present", "pixWriteMemPng", 1);
}

/* --------------------------------------------*/
#endif  /* !HAVE_LIBPNG */
/* --------------------------------------------*/
