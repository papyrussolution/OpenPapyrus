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
#ifdef HAVE_CONFIG_H
	#include "config_types.h"
#elif _WIN32
	#include "config_win32.h"
#endif
#ifdef HAVE_STDINT_H
	#include <stdint.h>
#endif

#if 0 // {
int main(int argc, char * argv[])
{
	Jbig2Image * image;
	int code;
	/* we need a context for the allocators */
	Jbig2Ctx * ctx = jbig2_ctx_new(NULL, (Jbig2Options)0, NULL, NULL, NULL);
	if(argc != 3) {
		slfprintf_stderr("usage: %s <in.pbm> <out.png>\n\n", argv[0]);
		return 1;
	}
	image = jbig2_image_read_pbm_file(ctx, argv[1]);
	if(image == NULL) {
		slfprintf_stderr("error reading pbm file '%s'\n", argv[1]);
		return 1;
	}
	else {
		slfprintf_stderr("converting %dx%d image to png format\n", image->width, image->height);
	}
	code = jbig2_image_write_png_file(image, argv[2]);
	if(code) {
		slfprintf_stderr("error writing png file '%s' error %d\n", argv[2], code);
	}

	return (code);
}
#endif // } 0