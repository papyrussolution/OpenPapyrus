/* Copyright (C) 2001-2020 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  1305 Grant Avenue - Suite 200, Novato,
   CA 94945, U.S.A., +1(415)492-9861, for further information.
 */

/*
    jbig2dec
 */
#include "jbig2dec-internal.h"
#pragma hdrstop
#if 0 // {
/* replacement for broken memcmp() */

/*
 * compares two byte strings 'a' and 'b', both assumed to be 'len' bytes long
 * returns zero if the two strings are identical, otherwise returns -1 or 1
 * depending on the relative magnitude of the first differing elements,
 * considered as unsigned chars
 */
int memcmp(const void * b1, const void * b2, size_t len)
{
	size_t i;
	unsigned char * a = (unsigned char*)b1;
	unsigned char * b = (unsigned char*)b2;
	for(i = 0; i < len; i++) {
		if(*a != *b) {
			/* strings differ */
			return (*a < *b) ? -1 : 1;
		}
		a++;
		b++;
	}
	/* strings match */
	return 0;
}
#endif // } 0
