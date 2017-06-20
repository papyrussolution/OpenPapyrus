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
 * \file pnmiostub.c
 * <pre>
 *
 *     Stubs for pnmio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_PNMIO   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamPnm(FILE *fp)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadStreamPnm", NULL);
}

/* ----------------------------------------------------------------------*/

int32 readHeaderPnm(const char *filename, int32 *pw, int32 *ph,
                      int32 *pd, int32 *ptype, int32 *pbps,
                      int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderPnm", 1);
}

/* ----------------------------------------------------------------------*/

int32 freadHeaderPnm(FILE *fp, int32 *pw, int32 *ph, int32 *pd,
                       int32 *ptype, int32 *pbps, int32 *pspp)
{
    return ERROR_INT("function not present", "freadHeaderPnm", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteStreamPnm(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamPnm", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteStreamAsciiPnm(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamAsciiPnm", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteStreamPam(FILE *fp, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteStreamPam", 1);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemPnm(const uint8 *cdata, size_t size)
{
    return (PIX * )ERROR_PTR("function not present", "pixReadMemPnm", NULL);
}

/* ----------------------------------------------------------------------*/

int32 readHeaderMemPnm(const uint8 *cdata, size_t size, int32 *pw,
                         int32 *ph, int32 *pd, int32 *ptype,
                         int32 *pbps, int32 *pspp)
{
    return ERROR_INT("function not present", "readHeaderMemPnm", 1);
}

/* ----------------------------------------------------------------------*/

int32 pixWriteMemPnm(uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemPnm", 1);
}
/* ----------------------------------------------------------------------*/

int32 pixWriteMemPam(uint8 **pdata, size_t *psize, PIX *pix)
{
    return ERROR_INT("function not present", "pixWriteMemPam", 1);
}


/* --------------------------------------------*/
#endif  /* !USE_PNMIO */
/* --------------------------------------------*/
