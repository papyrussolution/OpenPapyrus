/*
  Configuration defines for installed libtiff.
  This file maintained for backward compatibility. Do not use definitions
  from this file in your programs.
*/

#ifndef _TIFFCONF_
#define _TIFFCONF_

#define TIFF_INT16_T signed short /* Signed 16-bit type */
#define TIFF_INT32_T signed int /* Signed 32-bit type */
#define TIFF_INT64_T signed long long /* Signed 64-bit type */
#define TIFF_INT8_T signed char /* Signed 8-bit type */
#define TIFF_UINT16_T ushort /* Unsigned 16-bit type */
#define TIFF_UINT32_T unsigned int /* Unsigned 32-bit type */
#define TIFF_UINT64_T uint64 /* Unsigned 64-bit type */
#define TIFF_UINT8_T uchar /* Unsigned 8-bit type */
#define TIFF_SIZE_T unsigned int /* Unsigned size type */
#define TIFF_SSIZE_T signed int /* Signed size type */
#define TIFF_PTRDIFF_T ptrdiff_t /* Pointer difference type */
/* #undef HAVE_INT16 */ /* Define to 1 if the system has the type `int16'. */
/* #undef HAVE_INT32 */ /* Define to 1 if the system has the type `int32'. */
/* #undef HAVE_INT8 */ /* Define to 1 if the system has the type `int8'. */

/* Compatibility stuff. */

#define HAVE_IEEEFP 1 /* Define as 0 or 1 according to the floating point format suported by the machine */
#define HOST_FILLORDER FILLORDER_MSB2LSB /* Set the native cpu bit order (FILLORDER_LSB2MSB or FILLORDER_MSB2LSB) */
#define HOST_BIGENDIAN 0 /* Native cpu byte order: 1 if big-endian (Motorola) or 0 if little-endian (Intel) */
#define CCITT_SUPPORT 1 /* Support CCITT Group 3 & 4 algorithms */
/* #undef JPEG_SUPPORT */ /* Support JPEG compression (requires IJG JPEG library) */
#define JPEG_SUPPORT 1
/* #undef JBIG_SUPPORT */ /* Support JBIG compression (requires JBIG-KIT library) */
#define LOGLUV_SUPPORT 1 /* Support LogLuv high dynamic range encoding */
#define LZW_SUPPORT 1 /* Support LZW algorithm */
#define NEXT_SUPPORT 1 /* Support NeXT 2-bit RLE algorithm */
/* #undef OJPEG_SUPPORT */ /* Support Old JPEG compresson (read contrib/ojpeg/README first! Compilation fails with unpatched IJG JPEG library) */
#define PACKBITS_SUPPORT 1 /* Support Macintosh PackBits algorithm */
/* #undef PIXARLOG_SUPPORT */ /* Support Pixar log-format algorithm (requires Zlib) */
#define THUNDER_SUPPORT 1 /* Support ThunderScan 4-bit RLE algorithm */
/* #undef ZIP_SUPPORT */ /* Support Deflate compression */
#define ZIP_SUPPORT 1
#define STRIPCHOP_DEFAULT 1 /* Support strip chopping (whether or not to convert single-strip uncompressed images to mutiple strips of ~8Kb to reduce memory usage) */
#define SUBIFD_SUPPORT 1 /* Enable SubIFD tag (330) support */
/* Treat extra sample as alpha (default enabled). The RGBA interface will
   treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many
   packages produce RGBA files but don't mark the alpha properly. */
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1 /* Pick up YCbCr subsampling info from the JPEG data stream to support files lacking the tag (default enabled). */
#define MDI_SUPPORT 1 /* Support MS MDI magic number files as TIFF */
/*
 * Feature support definitions.
 * XXX: These macros are obsoleted. Don't use them in your apps!
 * Macros stays here for backward compatibility and should be always defined.
 */
#define COLORIMETRY_SUPPORT
#define YCBCR_SUPPORT
#define CMYK_SUPPORT
#define ICC_SUPPORT
#define PHOTOSHOP_SUPPORT
#define IPTC_SUPPORT

#endif /* _TIFFCONF_ */
