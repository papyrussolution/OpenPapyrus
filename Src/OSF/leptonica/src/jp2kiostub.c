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
 * \file jp2kiostub.c
 * <pre>
 *
 *     Stubs for jp2kio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !HAVE_LIBJP2K   /* defined in environ.h */
/* --------------------------------------------*/

/* ----------------------------------------------------------------------*/

PIX * pixReadJp2k(const char * filename, l_uint32 reduction, BOX * box,
    l_int32 hint, l_int32 debug)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadJp2k", NULL);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadStreamJp2k(FILE * fp, l_uint32 reduction, BOX * box,
    l_int32 hint, l_int32 debug)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadStreamJp2k", NULL);
}

/* ----------------------------------------------------------------------*/

l_ok pixWriteJp2k(const char * filename, PIX * pix, l_int32 quality,
    l_int32 nlevels, l_int32 hint, l_int32 debug)
{
	return ERROR_INT("function not present", "pixWriteJp2k", 1);
}

/* ----------------------------------------------------------------------*/

l_ok pixWriteStreamJp2k(FILE * fp, PIX * pix, l_int32 quality,
    l_int32 nlevels, l_int32 codec,
    l_int32 hint, l_int32 debug)
{
	return ERROR_INT("function not present", "pixWriteStreamJp2k", 1);
}

/* ----------------------------------------------------------------------*/

PIX * pixReadMemJp2k(const uint8 * data, size_t size, l_uint32 reduction,
    BOX * box, l_int32 hint, l_int32 debug)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadMemJp2k", NULL);
}

/* ----------------------------------------------------------------------*/

l_ok pixWriteMemJp2k(uint8 ** pdata, size_t * psize, PIX * pix,
    l_int32 quality, l_int32 nlevels, l_int32 hint,
    l_int32 debug)
{
	return ERROR_INT("function not present", "pixWriteMemJp2k", 1);
}

/* ----------------------------------------------------------------------*/

/* --------------------------------------------*/
#endif  /* !HAVE_LIBJP2K */
/* --------------------------------------------*/
