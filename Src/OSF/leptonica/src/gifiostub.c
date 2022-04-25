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
 * \file gifiostub.c
 * <pre>
 *
 *     Stubs for gifio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* -----------------------------------------------------------------*/
#if  (!HAVE_LIBGIF) && (!HAVE_LIBUNGIF)     /* defined in environ.h */
/* -----------------------------------------------------------------*/

PIX * pixReadStreamGif(FILE * fp)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadStreamGif", NULL);
}

PIX * pixReadMemGif(const uint8 * cdata, size_t size)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadMemGif", NULL);
}

l_ok pixWriteStreamGif(FILE * fp, PIX * pix)
{
	return ERROR_INT("function not present", "pixWriteStreamGif", 1);
}

l_ok pixWriteMemGif(uint8 ** pdata, size_t * psize, PIX * pix)
{
	return ERROR_INT("function not present", "pixWriteMemGif", 1);
}

/* -----------------------------------------------------------------*/
#endif      /* !HAVE_LIBGIF && !HAVE_LIBUNGIF */
