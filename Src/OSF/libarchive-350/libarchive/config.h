/* config.h.  Generated from build/cmake/config.h.in by cmake configure */
/*
 * Ensure we have C99-style int64, etc, all defined.
 */
/* First, we need to know if the system has already defined them. */
/* #undef HAVE_INT16_T */
/* #undef HAVE_INT32_T */
/* #undef HAVE_INT64_T */
/* #undef HAVE_INTMAX_T */
/* #undef HAVE_UINT8_T */
/* #undef HAVE_UINT16_T */
/* #undef HAVE_UINT32_T */
/* #undef HAVE_UINT64_T */
/* #undef HAVE_UINTMAX_T */

/* We might have the types we want under other spellings. */
#define HAVE___INT64
/* #undef HAVE_U_INT64_T */
#define HAVE_UNSIGNED___INT64

/* The sizes of various standard integer types. */
#define SIZE_OF_SHORT 2
#define SIZE_OF_INT 4
#define SIZE_OF_LONG 4
#define SIZE_OF_LONG_LONG 8
#define SIZE_OF_UNSIGNED_SHORT 2
#define SIZE_OF_UNSIGNED 4
#define SIZE_OF_UNSIGNED_LONG 4
#define SIZE_OF_UNSIGNED_LONG_LONG 8
/*
 * If we lack int64, define it to the first of __int64, int, long, and long long
 * that exists and is the right size.
 */
#if !defined(HAVE_INT64_T) && defined(HAVE___INT64)
	// @sobolev typedef __int64 int64;
	#define HAVE_INT64_T
#endif
#if !defined(HAVE_INT64_T) && SIZE_OF_INT == 8
	// @sobolev typedef int int64;
	#define HAVE_INT64_T
#endif
#if !defined(HAVE_INT64_T) && SIZE_OF_LONG == 8
	// @sobolev typedef long int64;
	#define HAVE_INT64_T
#endif
#if !defined(HAVE_INT64_T) && SIZE_OF_LONG_LONG == 8
	// @sobolev typedef long long int64;
	#define HAVE_INT64_T
#endif
#if !defined(HAVE_INT64_T)
	#error No 64-bit integer type was found.
#endif
/*
 * Similarly for int32_t
 */
#if !defined(HAVE_INT32_T) && SIZE_OF_INT == 4
	typedef int int32_t;
	#define HAVE_INT32_T
#endif
#if !defined(HAVE_INT32_T) && SIZE_OF_LONG == 4
	typedef long int32_t;
	#define HAVE_INT32_T
#endif
#if !defined(HAVE_INT32_T)
	#error No 32-bit integer type was found.
#endif
/*
 * Similarly for int16
 */
#if !defined(HAVE_INT16_T) && SIZE_OF_INT == 2
	// @sobolev typedef int int16_t__Removed;
	#define HAVE_INT16_T
#endif
#if !defined(HAVE_INT16_T) && SIZE_OF_SHORT == 2
	// @sobolev typedef short int16_t__Removed;
	#define HAVE_INT16_T
#endif
#if !defined(HAVE_INT16_T)
	#error No 16-bit integer type was found.
#endif
/*
 * Similarly for uint64
 */
#if !defined(HAVE_UINT64_T) && defined(HAVE_UNSIGNED___INT64)
	// @sobolev typedef unsigned __int64 uint64;
	#define HAVE_UINT64_T
#endif
#if !defined(HAVE_UINT64_T) && SIZE_OF_UNSIGNED == 8
	// @sobolev typedef unsigned uint64;
	#define HAVE_UINT64_T
#endif
#if !defined(HAVE_UINT64_T) && SIZE_OF_UNSIGNED_LONG == 8
	// @sobolev typedef unsigned long uint64;
	#define HAVE_UINT64_T
#endif
#if !defined(HAVE_UINT64_T) && SIZE_OF_UNSIGNED_LONG_LONG == 8
	// @sobolev typedef unsigned long long uint64;
	#define HAVE_UINT64_T
#endif
#if !defined(HAVE_UINT64_T)
	#error No 64-bit unsigned integer type was found.
#endif
/*
 * Similarly for uint32
 */
//#if !defined(HAVE_UINT32_T) && SIZE_OF_UNSIGNED == 4
	//typedef uint uint32_t;
	//#define HAVE_UINT32_T
//#endif
//#if !defined(HAVE_UINT32_T) && SIZE_OF_UNSIGNED_LONG == 4
	//typedef unsigned long uint32;
	//#define HAVE_UINT32_T
//#endif
//#if !defined(HAVE_UINT32_T)
	//#error No 32-bit unsigned integer type was found.
//#endif
/*
 * Similarly for uint16
 */
#if !defined(HAVE_UINT16_T) && SIZE_OF_UNSIGNED == 2
	// @sobolev typedef unsigned uint16_t__Removed;
	#define HAVE_UINT16_T
#endif
#if !defined(HAVE_UINT16_T) && SIZE_OF_UNSIGNED_SHORT == 2
	// @sobolev typedef unsigned short uint16_t__Removed;
	#define HAVE_UINT16_T
#endif
#if !defined(HAVE_UINT16_T)
	#error No 16-bit unsigned integer type was found.
#endif
/*
 * Similarly for uint8
 */
#if !defined(HAVE_UINT8_T)
	// @sobolev typedef uchar uint8;
	#define HAVE_UINT8_T
#endif
#if !defined(HAVE_UINT16_T)
	#error No 8-bit unsigned integer type was found.
#endif
/* Define intmax_t and uintmax_t if they are not already defined. */
#if !defined(HAVE_INTMAX_T)
	typedef int64 intmax_t;
#endif
#if !defined(HAVE_UINTMAX_T)
	typedef uint64 uintmax_t;
#endif

/* #undef ZLIB_WINAPI */ /* Define ZLIB_WINAPI if zlib was built on Visual Studio. */
/* #undef ARCHIVE_ACL_DARWIN */ /* Darwin ACL support */
/* #undef ARCHIVE_ACL_FREEBSD */ /* FreeBSD ACL support */
/* #undef ARCHIVE_ACL_FREEBSD_NFS4 */ /* FreeBSD NFSv4 ACL support */
/* #undef ARCHIVE_ACL_LIBACL */ /* Linux POSIX.1e ACL support via libacl */
/* #undef ARCHIVE_ACL_LIBRICHACL */ /* Linux NFSv4 ACL support via librichacl */
/* #undef ARCHIVE_ACL_SUNOS */ /* Solaris ACL support */
/* #undef ARCHIVE_ACL_SUNOS_NFS4 */ /* Solaris NFSv4 ACL support */
/* #undef ARCHIVE_CRYPTO_MD5_LIBC */ /* MD5 via ARCHIVE_CRYPTO_MD5_LIBC supported. */
/* #undef ARCHIVE_CRYPTO_MD5_LIBSYSTEM */ /* MD5 via ARCHIVE_CRYPTO_MD5_LIBSYSTEM supported. */
/* #undef ARCHIVE_CRYPTO_MD5_NETTLE */ /* MD5 via ARCHIVE_CRYPTO_MD5_NETTLE supported. */
#define ARCHIVE_CRYPTO_MD5_OPENSSL 1 /* #undef ARCHIVE_CRYPTO_MD5_OPENSSL */ /* MD5 via ARCHIVE_CRYPTO_MD5_OPENSSL supported. */
#define ARCHIVE_CRYPTO_MD5_WIN 1 /* MD5 via ARCHIVE_CRYPTO_MD5_WIN supported. */
/* #undef ARCHIVE_CRYPTO_RMD160_LIBC */ /* RMD160 via ARCHIVE_CRYPTO_RMD160_LIBC supported. */
/* #undef ARCHIVE_CRYPTO_RMD160_NETTLE */ /* RMD160 via ARCHIVE_CRYPTO_RMD160_NETTLE supported. */
/* #undef ARCHIVE_CRYPTO_RMD160_OPENSSL */ /* RMD160 via ARCHIVE_CRYPTO_RMD160_OPENSSL supported. */
/* #undef ARCHIVE_CRYPTO_SHA1_LIBC */ /* SHA1 via ARCHIVE_CRYPTO_SHA1_LIBC supported. */
/* #undef ARCHIVE_CRYPTO_SHA1_LIBSYSTEM */ /* SHA1 via ARCHIVE_CRYPTO_SHA1_LIBSYSTEM supported. */
/* #undef ARCHIVE_CRYPTO_SHA1_NETTLE */ /* SHA1 via ARCHIVE_CRYPTO_SHA1_NETTLE supported. */
#define ARCHIVE_CRYPTO_SHA1_OPENSSL 1 /* #undef ARCHIVE_CRYPTO_SHA1_OPENSSL */ /* SHA1 via ARCHIVE_CRYPTO_SHA1_OPENSSL supported. */
#define ARCHIVE_CRYPTO_SHA1_WIN 1 /* SHA1 via ARCHIVE_CRYPTO_SHA1_WIN supported. */
/* #undef ARCHIVE_CRYPTO_SHA256_LIBC */ /* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBC supported. */
/* #undef ARCHIVE_CRYPTO_SHA256_LIBC2 */ /* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBC2 supported. */
/* #undef ARCHIVE_CRYPTO_SHA256_LIBC3 */ /* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBC3 supported. */
/* #undef ARCHIVE_CRYPTO_SHA256_LIBSYSTEM */ /* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBSYSTEM supported. */
/* #undef ARCHIVE_CRYPTO_SHA256_NETTLE */ /* SHA256 via ARCHIVE_CRYPTO_SHA256_NETTLE supported. */
#define ARCHIVE_CRYPTO_SHA256_OPENSSL /* #undef ARCHIVE_CRYPTO_SHA256_OPENSSL */ /* SHA256 via ARCHIVE_CRYPTO_SHA256_OPENSSL supported. */
#define ARCHIVE_CRYPTO_SHA256_WIN 1 /* SHA256 via ARCHIVE_CRYPTO_SHA256_WIN supported. */
/* #undef ARCHIVE_CRYPTO_SHA384_LIBC */ /* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBC supported. */
/* #undef ARCHIVE_CRYPTO_SHA384_LIBC2 */ /* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBC2 supported. */
/* #undef ARCHIVE_CRYPTO_SHA384_LIBC3 */ /* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBC3 supported. */
/* #undef ARCHIVE_CRYPTO_SHA384_LIBSYSTEM */ /* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBSYSTEM supported. */
/* #undef ARCHIVE_CRYPTO_SHA384_NETTLE */ /* SHA384 via ARCHIVE_CRYPTO_SHA384_NETTLE supported. */
#define ARCHIVE_CRYPTO_SHA384_OPENSSL 1 /* #undef ARCHIVE_CRYPTO_SHA384_OPENSSL */ /* SHA384 via ARCHIVE_CRYPTO_SHA384_OPENSSL supported. */
#define ARCHIVE_CRYPTO_SHA384_WIN 1 /* SHA384 via ARCHIVE_CRYPTO_SHA384_WIN supported. */
/* #undef ARCHIVE_CRYPTO_SHA512_LIBC */ /* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBC supported. */
/* #undef ARCHIVE_CRYPTO_SHA512_LIBC2 */ /* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBC2 supported. */
/* #undef ARCHIVE_CRYPTO_SHA512_LIBC3 */ /* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBC3 supported. */
/* #undef ARCHIVE_CRYPTO_SHA512_LIBSYSTEM */ /* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBSYSTEM supported. */
/* #undef ARCHIVE_CRYPTO_SHA512_NETTLE */ /* SHA512 via ARCHIVE_CRYPTO_SHA512_NETTLE supported. */
#define ARCHIVE_CRYPTO_SHA512_OPENSSL 1 /* #undef ARCHIVE_CRYPTO_SHA512_OPENSSL */ /* SHA512 via ARCHIVE_CRYPTO_SHA512_OPENSSL supported. */
#define ARCHIVE_CRYPTO_SHA512_WIN     1 /* SHA512 via ARCHIVE_CRYPTO_SHA512_WIN supported. */
/* #undef ARCHIVE_XATTR_AIX */ /* AIX xattr support */
/* #undef ARCHIVE_XATTR_DARWIN */ /* Darwin xattr support */
/* #undef ARCHIVE_XATTR_FREEBSD */ /* FreeBSD xattr support */
/* #undef ARCHIVE_XATTR_LINUX */ /* Linux xattr support */
#define BSDCPIO_VERSION_STRING "3.3.4dev" /* Version number of bsdcpio */
#define BSDTAR_VERSION_STRING "3.3.4dev" /* Version number of bsdtar */
#define BSDCAT_VERSION_STRING "3.3.4dev" /* Version number of bsdcat */

#define ICONV_CONST  /* Define as const if the declaration of iconv() needs const. */
#define LIBARCHIVE_VERSION_NUMBER "3003004" /* Version number of libarchive as a single integer */
#define LIBARCHIVE_VERSION_STRING "3.3.4dev" /* Version number of libarchive */
/* #undef LSTAT_FOLLOWS_SLASHED_SYMLINK */ /* Define to 1 if `lstat' dereferences a symlink specified with a trailing slash. */
/* #undef MAJOR_IN_MKDEV */ /* Define to 1 if `major', `minor', and `makedev' are declared in <mkdev.h>. */
/* #undef MAJOR_IN_SYSMACROS */ /* Define to 1 if `major', `minor', and `makedev' are declared in <sysmacros.h>. */
/* #undef NO_MINUS_C_MINUS_O */ /* Define to 1 if your C compiler doesn't accept -c and -o together. */
#define SIZEOF_WCHAR_T 2 /* The size of `wchar_t', as computed by sizeof. */
/* #undef STRERROR_R_CHAR_P */ /* Define to 1 if strerror_r returns char *. */
/* #undef TIME_WITH_SYS_TIME */ /* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */

/*
 * Some platform requires a macro to use extension functions.
 */
#define SAFE_TO_DEFINE_EXTENSIONS 1
#ifdef SAFE_TO_DEFINE_EXTENSIONS
/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
	#define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
	#define _POSIX_PTHREAD_SEMANTICS 1
#endif
#ifndef _TANDEM_SOURCE
	#define _TANDEM_SOURCE 1 /* Enable extensions on HP NonStop.  */
#endif
#ifndef __EXTENSIONS__
	#define __EXTENSIONS__ 1 /* Enable general extensions on Solaris.  */
#endif
#endif /* SAFE_TO_DEFINE_EXTENSIONS */
#define VERSION "3.3.4dev" /* Version number of package */
/* #undef _FILE_OFFSET_BITS */ /* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _LARGEFILE_SOURCE */ /* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGE_FILES */ /* Define for large files, on AIX-style hosts. */
/* Define to control Windows SDK version */
#ifndef NTDDI_VERSION
	#define NTDDI_VERSION 0x06010000
#endif // NTDDI_VERSION
#ifndef WINVER
	#define WINVER 0x0601
#endif // WINVER
/* #undef const */ /* Define to empty if `const' does not conform to ANSI C. */
#define gid_t short /* Define to `int' if <sys/types.h> doesn't define. */
#define id_t short /* Define to `unsigned long' if <sys/types.h> does not define. */
#define mode_t unsigned short /* Define to `int' if <sys/types.h> does not define. */
/* #undef off_t */ /* Define to `long long' if <sys/types.h> does not define. */
#define pid_t int /* Define to `int' if <sys/types.h> doesn't define. */
/* #undef size_t */ /* Define to `uint' if <sys/types.h> does not define. */
#define uid_t short /* Define to `int' if <sys/types.h> doesn't define. */
/* #undef intptr_t */ /* Define to `int' if <sys/types.h> does not define. */
/* #undef uintptr_t */ /* Define to `uint' if <sys/types.h> does not define. */
