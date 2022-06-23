// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file jp2kheaderstub.c
 * <pre>
 *
 *     Stubs for jp2kheader.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !USE_JP2KHEADER   /* defined in environ.h */
/* --------------------------------------------*/

l_ok readHeaderJp2k(const char * filename, int32 * pw, int32 * ph,
    int32 * pbps, int32 * pspp, int32 * pcodec)
{
	return ERROR_INT("function not present", "readHeaderJp2k", 1);
}

l_ok freadHeaderJp2k(FILE * fp, int32 * pw, int32 * ph,
    int32 * pbps, int32 * pspp, int32 * pcodec)
{
	return ERROR_INT("function not present", "freadHeaderJp2k", 1);
}

l_ok readHeaderMemJp2k(const uint8 * cdata, size_t size, int32 * pw,
    int32 * ph, int32 * pbps, int32 * pspp,
    int32 * pcodec)
{
	return ERROR_INT("function not present", "readHeaderMemJp2k", 1);
}

int32 fgetJp2kResolution(FILE * fp, int32 * pxres, int32 * pyres)
{
	return ERROR_INT("function not present", "fgetJp2kResolution", 1);
}

/* --------------------------------------------*/
#endif  /* !USE_JP2KHEADER */
