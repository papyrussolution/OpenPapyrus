// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
/*!
 * \file libversions.c
 * <pre>
 *
 *       Image library version number
 *           char      *getImagelibVersions()
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop
#if HAVE_LIBGIF
	#include "gif_lib.h"
#endif
#if HAVE_LIBJPEG
	/* jpeglib.h includes jconfig.h, which makes the error of setting
	 *   #define HAVE_STDLIB_H
	 * which conflicts with config_auto.h (where it is set to 1) and results
	 * for some gcc compiler versions in a warning.  The conflict is harmless
	 * but we suppress it by undefining the variable. */
	#undef HAVE_STDLIB_H
	#include "jpeglib.h"
	#include "jerror.h"
#endif
#if HAVE_LIBPNG
	#include "png.h"
#endif
#if HAVE_LIBTIFF
	#include "tiffio.h"
#endif
#if HAVE_LIBZ
	#include "zlib.h"
#endif
#if HAVE_LIBWEBP
	#include <../osf/libwebp/src/webp/encode.h>
#endif
#if HAVE_LIBJP2K
	#ifdef LIBJP2K_HEADER
		#include LIBJP2K_HEADER
	#else
		#include <openjpeg.h>
	#endif
#endif

/*---------------------------------------------------------------------*
*                    Image Library Version number                     *
*---------------------------------------------------------------------*/
/*!
 * \brief   getImagelibVersions()
 *
 * <pre>
 * Notes:
 *      (1) This returns a string of version numbers; e.g.,
 *            libgif 5.0.3
 *            libjpeg 8b (libjpeg-turbo 1.3.0)
 *            libpng 1.4.3
 *            libtiff 3.9.5
 *            zlib 1.2.5
 *            libwebp 0.3.0
 *            libopenjp2 2.1.0
 *      (2) The caller must free the memory.
 * </pre>
 */
char * getImagelibVersions(void)
{
	char buf[128];
	l_int32 first = TRUE;
	char * versionNumP;
	char * nextTokenP;
	char * versionStrP = NULL;
#if HAVE_LIBGIF
	first = FALSE;
	stringJoinIP(&versionStrP, "libgif ");
  #ifdef GIFLIB_MAJOR
	snprintf(buf, sizeof(buf), "%d.%d.%d", GIFLIB_MAJOR, GIFLIB_MINOR,
	    GIFLIB_RELEASE);
  #else
	stringCopy(buf, "4.1.6(?)", sizeof(buf));
  #endif
	stringJoinIP(&versionStrP, buf);
#endif  /* HAVE_LIBGIF */

#if HAVE_LIBJPEG
	{
		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr err;
		char buffer[JMSG_LENGTH_MAX];
		cinfo.err = jpeg_std_error(&err);
		err.msg_code = JMSG_VERSION;
		(*err.format_message)((j_common_ptr ) &cinfo, buffer);

		if(!first) stringJoinIP(&versionStrP, " : ");
		first = FALSE;
		stringJoinIP(&versionStrP, "libjpeg ");
		versionNumP = strtokSafe(buffer, " ", &nextTokenP);
		stringJoinIP(&versionStrP, versionNumP);
		SAlloc::F(versionNumP);

  #if defined(LIBJPEG_TURBO_VERSION)
		/* To stringify the result of expansion of a macro argument,
		 * you must use two levels of macros.  See:
		 *   https://gcc.gnu.org/onlinedocs/cpp/Stringification.html  */
  #define l_xstr(s) l_str(s)
  #define l_str(s) #s
		snprintf(buf, sizeof(buf), " (libjpeg-turbo %s)",
		    l_xstr(LIBJPEG_TURBO_VERSION));
		stringJoinIP(&versionStrP, buf);
  #endif  /* LIBJPEG_TURBO_VERSION */
	}
#endif  /* HAVE_LIBJPEG */

#if HAVE_LIBPNG
	if(!first) stringJoinIP(&versionStrP, " : ");
	first = FALSE;
	stringJoinIP(&versionStrP, "libpng ");
	stringJoinIP(&versionStrP, png_get_libpng_ver(NULL));
#endif  /* HAVE_LIBPNG */

#if HAVE_LIBTIFF
	if(!first) stringJoinIP(&versionStrP, " : ");
	first = FALSE;
	stringJoinIP(&versionStrP, "libtiff ");
	versionNumP = strtokSafe((char *)TIFFGetVersion(), " \n", &nextTokenP);
	SAlloc::F(versionNumP);
	versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
	SAlloc::F(versionNumP);
	versionNumP = strtokSafe(NULL, " \n", &nextTokenP);
	stringJoinIP(&versionStrP, versionNumP);
	SAlloc::F(versionNumP);
#endif  /* HAVE_LIBTIFF */

#if HAVE_LIBZ
	if(!first) stringJoinIP(&versionStrP, " : ");
	first = FALSE;
	stringJoinIP(&versionStrP, "zlib ");
	stringJoinIP(&versionStrP, ZLIB_VERSION);
#endif  /* HAVE_LIBZ */

#if HAVE_LIBWEBP
	{
		l_int32 val;
		char buf[32];
		if(!first) stringJoinIP(&versionStrP, " : ");
		first = FALSE;
		stringJoinIP(&versionStrP, "libwebp ");
		val = WebPGetEncoderVersion();
		snprintf(buf, sizeof(buf), "%d.%d.%d", val >> 16, (val >> 8) & 0xff,
		    val & 0xff);
		stringJoinIP(&versionStrP, buf);
	}
#endif  /* HAVE_LIBWEBP */

#if HAVE_LIBJP2K
	{
		const char * version;
		if(!first) stringJoinIP(&versionStrP, " : ");
		first = FALSE;
		stringJoinIP(&versionStrP, "libopenjp2 ");
		version = opj_version();
		stringJoinIP(&versionStrP, version);
	}
#endif  /* HAVE_LIBJP2K */

	return versionStrP;
}
