// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file zlibmemstub.c
 * <pre>
 *
 *     Stubs for zlibmem.c functions
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

/* --------------------------------------------*/
#if  !HAVE_LIBZ   /* defined in environ.h */
/* --------------------------------------------*/
uint8 * zlibCompress(const uint8 * datain, size_t nin, size_t * pnout)
{
	return (uint8 *)ERROR_PTR("function not present", "zlibCompress", NULL);
}

uint8 * zlibUncompress(const uint8 * datain, size_t nin, size_t * pnout)
{
	return (uint8 *)ERROR_PTR("function not present", "zlibUncompress", NULL);
}
/* --------------------------------------------*/
#endif  /* !HAVE_LIBZ */
/* --------------------------------------------*/
