/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
/* !!ONLY FOR USE INTERNALLY TO LIBARCHIVE!! */
/*
 * This header is the first thing included in any of the libarchive
 * source files.  As far as possible, platform-specific issues should
 * be dealt with here and not within individual source files.  I'm
 * actively trying to minimize #if blocks within the main source,
 * since they obfuscate the code.
 */

#ifndef ARCHIVE_PLATFORM_H_INCLUDED
#define	ARCHIVE_PLATFORM_H_INCLUDED

#include <slib.h> // @sobolev
// @v11.8.12 (defined at slconfig.h) #define __STDC_FORMAT_MACROS // @sobolev
#define HAVE_INTTYPES_H 1
/* archive.h and archive_entry.h require this. */
#define	__LIBARCHIVE_BUILD 1

#if defined(PLATFORM_CONFIG_H)
/* Use hand-built config.h in environments that need it. */
#include PLATFORM_CONFIG_H
#elif defined(HAVE_CONFIG_H)
/* Most POSIX platforms use the 'configure' script to build config.h */
#include "config.h"
#define __LIBARCHIVE_CONFIG_H_INCLUDED // @sobolev
#else
/* Warn if the library hasn't been (automatically or manually) configured. */
#error Oops: No config.h and no pre-built configuration in archive_platform.h.
#endif

/* On macOS check for some symbols based on the deployment target version.  */
#if defined(__APPLE__)
#undef HAVE_FUTIMENS
#undef HAVE_UTIMENSAT
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101300
#define HAVE_FUTIMENS 1
#define HAVE_UTIMENSAT 1
#endif
#endif

/* It should be possible to get rid of this by extending the feature-test
 * macros to cover Windows API functions, probably along with non-trivial
 * refactoring of code to find structures that sit more cleanly on top of
 * either Windows or Posix APIs. */
#if (defined(__WIN32__) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
#include "archive_windows.h"
#else
#define la_stat(path,stref)		stat(path,stref)
#endif
/*
 * The config files define a lot of feature macros.  The following
 * uses those macros to select/define replacements and include key
 * headers as required.
 */
/* Get a real definition for __FBSDID or __RCSID if we can */
#if HAVE_SYS_CDEFS_H
	#include <sys/cdefs.h>
#endif
/* If not, define them so as to avoid dangling semicolons. */
#ifndef __FBSDID
	#define	__FBSDID(a)     struct _undefined_hack
#endif
#ifndef __RCSID
	#define	__RCSID(a)     struct _undefined_hack
#endif
/* Try to get standard C99-style integer type definitions. */
#if HAVE_INTTYPES_H
	#include <inttypes.h>
#endif
#if HAVE_STDINT_H
	#include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
/* Borland warns about its own constants!  */
#if defined(__BORLANDC__)
	#if HAVE_DECL_UINT64_MAX
		#undef	UINT64_MAX
		#undef	HAVE_DECL_UINT64_MAX
	#endif
	#if HAVE_DECL_UINT64_MIN
		#undef	UINT64_MIN
		#undef	HAVE_DECL_UINT64_MIN
	#endif
	#if HAVE_DECL_INT64_MAX
		#undef	INT64_MAX
		#undef	HAVE_DECL_INT64_MAX
	#endif
	#if HAVE_DECL_INT64_MIN
		#undef	INT64_MIN
		#undef	HAVE_DECL_INT64_MIN
	#endif
#endif
//
// Some platforms lack the standard *_MAX definitions
//
#ifndef SIZE_MAX
	#define	SIZE_MAX (~(size_t)0)
#endif
#ifndef SSIZE_MAX
	#define	SSIZE_MAX ((ssize_t)(SIZE_MAX >> 1))
#endif
#ifndef SLIBINCLUDED // {
	#if !HAVE_DECL_UINT32_MAX
		#define	UINT32_MAX (~(uint32)0)
	#endif
	#if !HAVE_DECL_INT32_MAX
		#define	INT32_MAX ((int32_t)(UINT32_MAX >> 1))
	#endif
	#if !HAVE_DECL_INT32_MIN
		#define	INT32_MIN ((int32_t)(~INT32_MAX))
	#endif
	#if !HAVE_DECL_UINT64_MAX
		#define	UINT64_MAX (~(uint64)0)
	#endif
	#if !HAVE_DECL_INT64_MAX
		#define	INT64_MAX ((int64)(UINT64_MAX >> 1))
	#endif
	#if !HAVE_DECL_INT64_MIN
		#define	INT64_MIN ((int64)(~INT64_MAX))
	#endif
	#if !HAVE_DECL_UINTMAX_MAX
		#define	UINTMAX_MAX (~(uintmax_t)0)
	#endif
	#if !HAVE_DECL_INTMAX_MAX
		#define	INTMAX_MAX ((intmax_t)(UINTMAX_MAX >> 1))
	#endif
	#if !HAVE_DECL_INTMAX_MIN
		#define	INTMAX_MIN ((intmax_t)(~INTMAX_MAX))
	#endif
#endif // } SLIBINCLUDED
/*
 * If we can't restore metadata using a file descriptor, then
 * for compatibility's sake, close files before trying to restore metadata.
 */
#if defined(HAVE_FCHMOD) || defined(HAVE_FUTIMES) || defined(HAVE_ACL_SET_FD) || defined(HAVE_ACL_SET_FD_NP) || defined(HAVE_FCHOWN)
	#define	CAN_RESTORE_METADATA_FD
#endif
/*
 * glibc 2.24 deprecates readdir_r
 */
#if defined(HAVE_READDIR_R) && (!defined(__GLIBC__) || !defined(__GLIBC_MINOR__) || __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 24))
	#define	USE_READDIR_R	1
#else
	#undef	USE_READDIR_R
#endif
/* Set up defaults for internal error codes. */
#ifndef ARCHIVE_ERRNO_FILE_FORMAT
	#if HAVE_EFTYPE
		#define	ARCHIVE_ERRNO_FILE_FORMAT EFTYPE
	#else
		#if HAVE_EILSEQ
			#define	ARCHIVE_ERRNO_FILE_FORMAT EILSEQ
		#else
			#define	ARCHIVE_ERRNO_FILE_FORMAT EINVAL
		#endif
	#endif
#endif
#ifndef ARCHIVE_ERRNO_PROGRAMMER
	#define	ARCHIVE_ERRNO_PROGRAMMER EINVAL
#endif
#ifndef ARCHIVE_ERRNO_MISC
	#define	ARCHIVE_ERRNO_MISC (-1)
#endif
/* @sobolev (replaced with CXX_FALLTHROUGH) #if defined(__GNUC__) && (__GNUC__ >= 7)
	#define	__LA_FALLTHROUGH_Removed __attribute__((fallthrough))
#else
	#define	__LA_FALLTHROUGH_Removed
#endif*/
#if HAVE_LIBXML_XMLWRITER_H
	// @v11.7.9 #include <..\osf\libxml\libxml.h>
	#include <..\slib\libxml\libxml.h> // @v11.7.9
	#include <libxml\xmlwriter.h>
#endif
#ifdef HAVE_BZLIB_H
	#include <..\slib\bzip2\bzlib.h>
#endif
#ifdef HAVE_LZMA_H
	#include <..\osf\liblzma\api\lzma.h>
#endif
#ifdef HAVE_ZLIB_H
	#include <zlib.h>
#endif
#include "archive.h"
#include "archive_entry.h"
#include "archive_string.h"
#include "archive_private.h"
#include "archive_write_private.h"
#include "archive_endian.h"
#include "archive_getdate.h"
#include "archive_pathmatch.h"
#include "archive_acl_private.h"
#include "archive_entry_private.h"
#include "archive_entry_locale.h"
#include "archive_rb.h"
#include "archive_read_private.h"
#include "archive_options_private.h"
#include "archive_write_set_format_private.h"
#include "archive_digest_private.h"
#include "filter_fork.h"
#endif /* !ARCHIVE_PLATFORM_H_INCLUDED */
