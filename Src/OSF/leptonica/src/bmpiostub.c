// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file  bmpiostub.c
 * <pre>
 *
 *      Stubs for bmpio.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_BMPIO   /* defined in environ.h */
/* --------------------------------------------*/

PIX * pixReadStreamBmp(FILE * fp)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadStreamBmp", NULL);
}

l_ok pixWriteStreamBmp(FILE * fp, PIX * pix)
{
	return ERROR_INT("function not present", "pixWriteStreamBmp", 1);
}

PIX * pixReadMemBmp(const uint8 * cdata, size_t size)
{
	return (PIX *)ERROR_PTR("function not present", "pixReadMemBmp", NULL);
}

l_ok pixWriteMemBmp(uint8 ** pdata, size_t * psize, PIX * pix)
{
	return ERROR_INT("function not present", "pixWriteMemBmp", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_BMPIO */
