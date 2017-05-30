// SZLIB.C
// Copyright (C) 2004-2017 Mark Adler
//
// Note: This is the merging of several small module of original zlib for compacting.
//
#define ZLIB_INTERNAL
#include "zlib.h"
#pragma hdrstop
//#include "gzguts.h"
//#include "zutil.h"
//#include "inftrees.h"

#ifdef _LARGEFILE64_SOURCE
	#ifndef _LARGEFILE_SOURCE
		#define _LARGEFILE_SOURCE 1
	#endif
	#ifdef _FILE_OFFSET_BITS
		#undef _FILE_OFFSET_BITS
	#endif
#endif
#ifdef HAVE_HIDDEN
	#define ZLIB_INTERNAL __attribute__((visibility("hidden")))
#else
	#define ZLIB_INTERNAL
#endif
//#include <stdio.h>
//#include "zlib.h"
#ifdef STDC
	//#include <string.h>
	//#include <stdlib.h>
	//#include <limits.h>
#endif
#ifndef _POSIX_SOURCE
	#define _POSIX_SOURCE
#endif
#include <fcntl.h>
#ifdef _WIN32
	//#include <stddef.h>
#endif
#if defined(__TURBOC__) || defined(_MSC_VER) || defined(_WIN32)
	//#include <io.h>
#endif
#if defined(_WIN32) || defined(__CYGWIN__)
	#if defined(UNICODE) || defined(_UNICODE)
		#define WIDECHAR
	#endif
#endif
#ifdef WINAPI_FAMILY
	#define open _open
	#define read _read
	#define write _write
	#define close _close
#endif
#ifdef NO_DEFLATE       /* for compatibility with old definition */
	#define NO_GZCOMPRESS
#endif
#if defined(STDC99) || (defined(__TURBOC__) && __TURBOC__ >= 0x550)
	#ifndef HAVE_VSNPRINTF
		#define HAVE_VSNPRINTF
	#endif
#endif
#if defined(__CYGWIN__)
	#ifndef HAVE_VSNPRINTF
		#define HAVE_VSNPRINTF
	#endif
#endif
#if defined(MSDOS) && defined(__BORLANDC__) && (BORLANDC > 0x410)
	#ifndef HAVE_VSNPRINTF
		#define HAVE_VSNPRINTF
	#endif
#endif
#ifndef HAVE_VSNPRINTF
	#ifdef MSDOS
		// vsnprintf may exist on some MS-DOS compilers (DJGPP?), but for now we just assume it doesn't. 
		#define NO_vsnprintf
	#endif
	#ifdef __TURBOC__
		#define NO_vsnprintf
	#endif
	#ifdef WIN32
		// In Win32, vsnprintf is available as the "non-ANSI" _vsnprintf. 
		#if !defined(vsnprintf) && !defined(NO_vsnprintf)
			#if !defined(_MSC_VER) || ( defined(_MSC_VER) && _MSC_VER < 1500 )
				#define vsnprintf _vsnprintf
			#endif
		#endif
	#endif
	#ifdef __SASC
		#define NO_vsnprintf
	#endif
	#ifdef VMS
		#define NO_vsnprintf
	#endif
	#ifdef __OS400__
		#define NO_vsnprintf
	#endif
	#ifdef __MVS__
		#define NO_vsnprintf
	#endif
#endif
// unlike snprintf (which is required in C99), _snprintf does not guarantee
// null termination of the result -- however this is only used in gzlib.c where
// the result is assured to fit in the space provided 
#if defined(_MSC_VER) && _MSC_VER < 1900
	#define snprintf _snprintf
#endif
/* @sobolev #ifndef local
	#define local static
#endif
/* since "static" is used to mean two completely different things in C, we
   define "local" for the non-static meaning of "static", for readability
   (compile with -Dlocal if your debugger can't find static symbols) */
//
// gz* functions always use library allocation functions 
//
#ifndef STDC
	extern voidp malloc(uInt size);
	extern void free(voidpf ptr);
#endif
//
// get errno and strerror definition 
//
#if defined UNDER_CE
	#include <windows.h>
	#define zstrerror() gz_strwinerror((DWORD)GetLastError())
#else
	#ifndef NO_STRERROR
		//#include <errno.h>
		#define zstrerror() strerror(errno)
	#else
		#define zstrerror() "stdio error (consult errno)"
	#endif
#endif
//
// provide prototypes for these when building zlib without LFS 
//
#if !defined(_LARGEFILE64_SOURCE) || _LFS64_LARGEFILE-0 == 0
	ZEXTERN gzFile ZEXPORT gzopen64(const char *, const char *);
	ZEXTERN z_off64_t ZEXPORT gzseek64(gzFile, z_off64_t, int);
	ZEXTERN z_off64_t ZEXPORT gztell64(gzFile);
	ZEXTERN z_off64_t ZEXPORT gzoffset64(gzFile);
#endif
//
// default memLevel 
//
#if MAX_MEM_LEVEL >= 8
	#define DEF_MEM_LEVEL 8
#else
	#define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
//
// default i/o buffer size -- double this for output when reading (this and
// twice this must be able to fit in an unsigned type) 
//
#define GZBUFSIZE 8192
//
// gzip modes, also provide a little integrity check on the passed structure 
//
#define GZ_NONE 0
#define GZ_READ 7247
#define GZ_WRITE 31153
#define GZ_APPEND 1     /* mode set to GZ_WRITE after the file is opened */

/* values for gz_state how */
#define LOOK 0      /* look for a gzip header */
#define COPY 1      /* copy input directly */
#define GZIP 2      /* decompress a gzip stream */

/* internal gzip file state data structure */
typedef struct {
	/* exposed contents for gzgetc() macro */
	struct gzFile_s x;  /* "x" for exposed */

	/* x.have: number of bytes available at x.next */
	/* x.next: next output data to deliver or write */
	/* x.pos: current position in uncompressed data */
	/* used for both reading and writing */
	int mode;           /* see gzip modes above */
	int fd;             /* file descriptor */
	char * path;        /* path or fd for error messages */
	unsigned size;      /* buffer size, zero if not allocated yet */
	unsigned want;      /* requested buffer size, default is GZBUFSIZE */
	unsigned char * in; /* input buffer (double-sized when writing) */
	unsigned char * out; /* output buffer (double-sized when reading) */
	int direct;         /* 0 if processing gzip, 1 if transparent */
	/* just for reading */
	int how;            /* 0: get header, 1: copy, 2: decompress */
	z_off64_t start;    /* where the gzip data started, for rewinding */
	int eof;            /* true if end of input file reached */
	int past;           /* true if read requested past end */
	/* just for writing */
	int level;          /* compression level */
	int strategy;       /* compression strategy */
	/* seek request */
	z_off64_t skip;     /* amount to skip (already rewound if backwards) */
	int seek;           /* true if seek request pending */
	/* error information */
	int err;            /* error code */
	char * msg;         /* error message */
	/* zlib inflate or deflate stream */
	z_stream strm;      /* stream structure in-place (not a pointer) */
} gz_state;

typedef gz_state * gz_statep;
//
// shared functions 
//
void ZLIB_INTERNAL gz_error(gz_statep, int, const char *);
#if defined UNDER_CE
	char ZLIB_INTERNAL * gz_strwinerror(DWORD error);
#endif

/* GT_OFF(x), where x is an unsigned value, is true if x > maximum z_off64_t
   value -- needed when comparing unsigned to z_off64_t, which is signed
   (possible z_off64_t types off_t, off64_t, and long are all signed) */
#ifdef INT_MAX
	#define GT_OFF(x) (sizeof(int) == sizeof(z_off64_t) && (x) > INT_MAX)
#else
	unsigned ZLIB_INTERNAL gz_intmax(void);
	#define GT_OFF(x) (sizeof(int) == sizeof(z_off64_t) && (x) > gz_intmax())
#endif
//
//
//
// Local functions for crc concatenation 
static unsigned long FASTCALL gf2_matrix_times(unsigned long * mat, unsigned long vec);
static void FASTCALL gf2_matrix_square(unsigned long * square, unsigned long * mat);
static uLong crc32_combine_(uLong crc1, uLong crc2, z_off64_t len2);
static uLong adler32_combine_(uLong adler1, uLong adler2, z_off64_t len2);
static int gz_init(gz_statep);
static int gz_comp(gz_statep, int);
static int gz_zero(gz_statep, z_off64_t);
static z_size_t gz_write(gz_statep, voidpc, z_size_t);
static int gz_load(gz_statep, unsigned char *, unsigned, unsigned *);
static int gz_avail(gz_statep);
static int gz_look(gz_statep);
static int gz_decomp(gz_statep);
static int gz_fetch(gz_statep);
static int gz_skip(gz_statep, z_off64_t);
static z_size_t gz_read(gz_statep, voidp, z_size_t);
static void gz_reset(gz_statep);
static gzFile gz_open(const void *, int, const char *);
//
// ADLER
//

#define BASE 65521U     /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf, i)  {adler += (buf)[i]; sum2 += adler; }
#define DO2(buf, i)  DO1(buf, i); DO1(buf, i+1);
#define DO4(buf, i)  DO2(buf, i); DO2(buf, i+2);
#define DO8(buf, i)  DO4(buf, i); DO4(buf, i+4);
#define DO16(buf)   DO8(buf, 0); DO8(buf, 8);

/* use NO_DIVIDE if your processor does not do division in hardware --
   try it both ways to see which is faster */
#ifdef NO_DIVIDE
	// note that this assumes BASE is 65521, where 65536 % 65521 == 15
   // (thank you to John Reiser for pointing this out) 
	#define CHOP(a) \
	do { \
		unsigned long tmp = a >> 16; \
		a &= 0xffffUL; \
		a += (tmp << 4) - tmp; \
	} while(0)
#define MOD28(a) do { CHOP(a); if(a >= BASE) a -= BASE; } while(0)
#define MOD(a) do { CHOP(a); MOD28(a); } while(0)
#define MOD63(a) \
	do { /* this assumes a is not negative */ \
		z_off64_t tmp = a >> 32; \
		a &= 0xffffffffL; \
		a += (tmp << 8) - (tmp << 5) + tmp; \
		tmp = a >> 16; \
		a &= 0xffffL; \
		a += (tmp << 4) - tmp; \
		tmp = a >> 16; \
		a &= 0xffffL; \
		a += (tmp << 4) - tmp; \
		if(a >= BASE) a -= BASE; \
	} while(0)
#else
	#define MOD(a) a %= BASE
	#define MOD28(a) a %= BASE
	#define MOD63(a) a %= BASE
#endif

/* ========================================================================= */
uLong ZEXPORT adler32_z(uLong adler, const Bytef * buf, z_size_t len)
{
	unsigned n;
	// split Adler-32 into component sums 
	unsigned long sum2 = (adler >> 16) & 0xffff;
	adler &= 0xffff;
	// in case user likes doing a byte at a time, keep it fast 
	if(len == 1) {
		adler += buf[0];
		if(adler >= BASE)
			adler -= BASE;
		sum2 += adler;
		if(sum2 >= BASE)
			sum2 -= BASE;
		return adler | (sum2 << 16);
	}
	/* initial Adler-32 value (deferred check for len == 1 speed) */
	if(buf == Z_NULL)
		return 1L;
	/* in case short lengths are provided, keep it somewhat fast */
	if(len < 16) {
		while(len--) {
			adler += *buf++;
			sum2 += adler;
		}
		if(adler >= BASE)
			adler -= BASE;
		MOD28(sum2);    /* only added so many BASE's */
		return adler | (sum2 << 16);
	}
	/* do length NMAX blocks -- requires just one modulo operation */
	while(len >= NMAX) {
		len -= NMAX;
		n = NMAX / 16;  /* NMAX is divisible by 16 */
		do {
			DO16(buf); /* 16 sums unrolled */
			buf += 16;
		} while(--n);
		MOD(adler);
		MOD(sum2);
	}
	/* do remaining bytes (less than NMAX, still just one modulo) */
	if(len) {               /* avoid modulos if none remaining */
		while(len >= 16) {
			len -= 16;
			DO16(buf);
			buf += 16;
		}
		while(len--) {
			adler += *buf++;
			sum2 += adler;
		}
		MOD(adler);
		MOD(sum2);
	}
	/* return recombined sums */
	return adler | (sum2 << 16);
}

uLong ZEXPORT adler32(uLong adler, const Bytef * buf, uInt len)
{
	return adler32_z(adler, buf, len);
}

static uLong adler32_combine_(uLong adler1, uLong adler2, z_off64_t len2)
{
	unsigned long sum1;
	unsigned long sum2;
	unsigned rem;
	/* for negative len, return invalid adler32 as a clue for debugging */
	if(len2 < 0)
		return 0xffffffffUL;

	/* the derivation of this formula is left as an exercise for the reader */
	MOD63(len2);            /* assumes len2 >= 0 */
	rem = (unsigned)len2;
	sum1 = adler1 & 0xffff;
	sum2 = rem * sum1;
	MOD(sum2);
	sum1 += (adler2 & 0xffff) + BASE - 1;
	sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
	if(sum1 >= BASE) sum1 -= BASE;
	if(sum1 >= BASE) sum1 -= BASE;
	if(sum2 >= ((unsigned long)BASE << 1)) sum2 -= ((unsigned long)BASE << 1);
	if(sum2 >= BASE) sum2 -= BASE;
	return sum1 | (sum2 << 16);
}

uLong ZEXPORT adler32_combine(uLong adler1, uLong adler2, z_off_t len2)
{
	return adler32_combine_(adler1, adler2, len2);
}

uLong ZEXPORT adler32_combine64(uLong adler1, uLong adler2, z_off64_t len2)
{
	return adler32_combine_(adler1, adler2, len2);
}

#undef DO1
#undef DO2
#undef DO4
#undef DO8
#undef DO16
//
// CRC32
//
#ifdef MAKECRCH
	//#include <stdio.h>
	#ifndef DYNAMIC_CRC_TABLE
		#define DYNAMIC_CRC_TABLE
	#endif /* !DYNAMIC_CRC_TABLE */
#endif

/* Definitions for doing the crc four data bytes at a time. */
#if !defined(NOBYFOUR) && defined(Z_U4)
	#define BYFOUR
#endif
#ifdef BYFOUR
	static unsigned long crc32_little(unsigned long, const unsigned char *, z_size_t);
	static unsigned long crc32_big(unsigned long, const unsigned char *, z_size_t);
	#define TBLS 8
#else
	#define TBLS 1
#endif /* BYFOUR */

#ifdef DYNAMIC_CRC_TABLE

static volatile int crc_table_empty = 1;
static z_crc_t crc_table[TBLS][256];
static void make_crc_table(void);
#ifdef MAKECRCH
	static void write_table(FILE *, const z_crc_t *);
#endif /* MAKECRCH */
/*
   Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
   x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

   Polynomials over GF(2) are represented in binary, one bit per coefficient,
   with the lowest powers in the most significant bit.  Then adding polynomials
   is just exclusive-or, and multiplying a polynomial by x is a right shift by
   one.  If we call the above polynomial p, and represent a byte as the
   polynomial q, also with the lowest power in the most significant bit (so the
   byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
   where a mod b means the remainder after dividing a by b.

   This calculation is done using the shift-register method of multiplying and
   taking the remainder.  The register is initialized to zero, and for each
   incoming bit, x^32 is added mod p to the register if the bit is a one (where
   x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
   x (which is shifting right by one and adding x^32 mod p if the bit shifted
   out is a one).  We start with the highest power (least significant bit) of
   q and repeat for all eight bits of q.

   The first table is simply the CRC of all possible eight bit values.  This is
   all the information needed to generate CRCs on data a byte at a time for all
   combinations of CRC register values and incoming bytes.  The remaining tables
   allow for word-at-a-time CRC calculation for both big-endian and little-
   endian machines, where a word is four bytes.
 */
static void make_crc_table()
{
	z_crc_t c;
	int n, k;
	z_crc_t poly;                   /* polynomial exclusive-or pattern */
	/* terms of polynomial defining this crc (except x^32): */
	static volatile int first = 1;  /* flag to limit concurrent making */
	static const unsigned char p[] = {0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26};

	/* See if another task is already doing this (not thread-safe, but better
	   than nothing -- significantly reduces duration of vulnerability in
	   case the advice about DYNAMIC_CRC_TABLE is ignored) */
	if(first) {
		first = 0;

		/* make exclusive-or pattern from polynomial (0xedb88320UL) */
		poly = 0;
		for(n = 0; n < (int)(sizeof(p)/sizeof(unsigned char)); n++)
			poly |= (z_crc_t)1 << (31 - p[n]);

		/* generate a crc for every 8-bit value */
		for(n = 0; n < 256; n++) {
			c = (z_crc_t)n;
			for(k = 0; k < 8; k++)
				c = c & 1 ? poly ^ (c >> 1) : c >> 1;
			crc_table[0][n] = c;
		}

#ifdef BYFOUR
		/* generate crc for each value followed by one, two, and three zeros,
		   and then the byte reversal of those as well as the first table */
		for(n = 0; n < 256; n++) {
			c = crc_table[0][n];
			crc_table[4][n] = ZSWAP32(c);
			for(k = 1; k < 4; k++) {
				c = crc_table[0][c & 0xff] ^ (c >> 8);
				crc_table[k][n] = c;
				crc_table[k + 4][n] = ZSWAP32(c);
			}
		}
#endif /* BYFOUR */

		crc_table_empty = 0;
	}
	else {  /* not first */
		/* wait for the other guy to finish (not efficient, but rare) */
		while(crc_table_empty)
			;
	}

#ifdef MAKECRCH
	/* write out CRC tables to crc32.h */
	{
		FILE * out = fopen("crc32.h", "w");
		if(out == NULL) return;
		fprintf(out, "/* crc32.h -- tables for rapid CRC calculation\n");
		fprintf(out, " * Generated automatically by crc32.c\n */\n\n");
		fprintf(out, "local const z_crc_t FAR ");
		fprintf(out, "crc_table[TBLS][256] =\n{\n  {\n");
		write_table(out, crc_table[0]);
#  ifdef BYFOUR
		fprintf(out, "#ifdef BYFOUR\n");
		for(k = 1; k < 8; k++) {
			fprintf(out, "  },\n  {\n");
			write_table(out, crc_table[k]);
		}
		fprintf(out, "#endif\n");
#  endif /* BYFOUR */
		fprintf(out, "  }\n};\n");
		fclose(out);
	}
#endif /* MAKECRCH */
}

#ifdef MAKECRCH
static void write_table(FILE * out, const z_crc_t  * table)
{
	for(int n = 0; n < 256; n++)
		fprintf(out, "%s0x%08lxUL%s", n % 5 ? "" : "    ", (unsigned long)(table[n]), n == 255 ? "\n" : (n % 5 == 4 ? ",\n" : ", "));
}

#endif /* MAKECRCH */

#else /* !DYNAMIC_CRC_TABLE */
	//
	// Tables of CRC-32s of all single-byte values, made by make_crc_table().
	//
	//#include "crc32.h"
	// crc32.h -- tables for rapid CRC calculation
	// Generated automatically by crc32.c
	//
	static const z_crc_t crc_table[TBLS][256] =
	{
		{
			0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
			0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
			0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
			0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
			0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
			0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
			0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
			0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
			0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
			0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
			0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
			0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
			0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
			0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
			0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
			0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
			0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
			0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
			0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
			0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
			0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
			0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
			0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
			0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
			0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
			0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
			0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
			0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
			0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
			0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
			0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
			0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
			0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
			0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
			0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
			0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
			0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
			0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
			0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
			0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
			0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
			0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
			0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
			0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
			0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
			0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
			0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
			0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
			0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
			0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
			0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
			0x2d02ef8dUL
	#ifdef BYFOUR
		},
		{
			0x00000000UL, 0x191b3141UL, 0x32366282UL, 0x2b2d53c3UL, 0x646cc504UL,
			0x7d77f445UL, 0x565aa786UL, 0x4f4196c7UL, 0xc8d98a08UL, 0xd1c2bb49UL,
			0xfaefe88aUL, 0xe3f4d9cbUL, 0xacb54f0cUL, 0xb5ae7e4dUL, 0x9e832d8eUL,
			0x87981ccfUL, 0x4ac21251UL, 0x53d92310UL, 0x78f470d3UL, 0x61ef4192UL,
			0x2eaed755UL, 0x37b5e614UL, 0x1c98b5d7UL, 0x05838496UL, 0x821b9859UL,
			0x9b00a918UL, 0xb02dfadbUL, 0xa936cb9aUL, 0xe6775d5dUL, 0xff6c6c1cUL,
			0xd4413fdfUL, 0xcd5a0e9eUL, 0x958424a2UL, 0x8c9f15e3UL, 0xa7b24620UL,
			0xbea97761UL, 0xf1e8e1a6UL, 0xe8f3d0e7UL, 0xc3de8324UL, 0xdac5b265UL,
			0x5d5daeaaUL, 0x44469febUL, 0x6f6bcc28UL, 0x7670fd69UL, 0x39316baeUL,
			0x202a5aefUL, 0x0b07092cUL, 0x121c386dUL, 0xdf4636f3UL, 0xc65d07b2UL,
			0xed705471UL, 0xf46b6530UL, 0xbb2af3f7UL, 0xa231c2b6UL, 0x891c9175UL,
			0x9007a034UL, 0x179fbcfbUL, 0x0e848dbaUL, 0x25a9de79UL, 0x3cb2ef38UL,
			0x73f379ffUL, 0x6ae848beUL, 0x41c51b7dUL, 0x58de2a3cUL, 0xf0794f05UL,
			0xe9627e44UL, 0xc24f2d87UL, 0xdb541cc6UL, 0x94158a01UL, 0x8d0ebb40UL,
			0xa623e883UL, 0xbf38d9c2UL, 0x38a0c50dUL, 0x21bbf44cUL, 0x0a96a78fUL,
			0x138d96ceUL, 0x5ccc0009UL, 0x45d73148UL, 0x6efa628bUL, 0x77e153caUL,
			0xbabb5d54UL, 0xa3a06c15UL, 0x888d3fd6UL, 0x91960e97UL, 0xded79850UL,
			0xc7cca911UL, 0xece1fad2UL, 0xf5facb93UL, 0x7262d75cUL, 0x6b79e61dUL,
			0x4054b5deUL, 0x594f849fUL, 0x160e1258UL, 0x0f152319UL, 0x243870daUL,
			0x3d23419bUL, 0x65fd6ba7UL, 0x7ce65ae6UL, 0x57cb0925UL, 0x4ed03864UL,
			0x0191aea3UL, 0x188a9fe2UL, 0x33a7cc21UL, 0x2abcfd60UL, 0xad24e1afUL,
			0xb43fd0eeUL, 0x9f12832dUL, 0x8609b26cUL, 0xc94824abUL, 0xd05315eaUL,
			0xfb7e4629UL, 0xe2657768UL, 0x2f3f79f6UL, 0x362448b7UL, 0x1d091b74UL,
			0x04122a35UL, 0x4b53bcf2UL, 0x52488db3UL, 0x7965de70UL, 0x607eef31UL,
			0xe7e6f3feUL, 0xfefdc2bfUL, 0xd5d0917cUL, 0xcccba03dUL, 0x838a36faUL,
			0x9a9107bbUL, 0xb1bc5478UL, 0xa8a76539UL, 0x3b83984bUL, 0x2298a90aUL,
			0x09b5fac9UL, 0x10aecb88UL, 0x5fef5d4fUL, 0x46f46c0eUL, 0x6dd93fcdUL,
			0x74c20e8cUL, 0xf35a1243UL, 0xea412302UL, 0xc16c70c1UL, 0xd8774180UL,
			0x9736d747UL, 0x8e2de606UL, 0xa500b5c5UL, 0xbc1b8484UL, 0x71418a1aUL,
			0x685abb5bUL, 0x4377e898UL, 0x5a6cd9d9UL, 0x152d4f1eUL, 0x0c367e5fUL,
			0x271b2d9cUL, 0x3e001cddUL, 0xb9980012UL, 0xa0833153UL, 0x8bae6290UL,
			0x92b553d1UL, 0xddf4c516UL, 0xc4eff457UL, 0xefc2a794UL, 0xf6d996d5UL,
			0xae07bce9UL, 0xb71c8da8UL, 0x9c31de6bUL, 0x852aef2aUL, 0xca6b79edUL,
			0xd37048acUL, 0xf85d1b6fUL, 0xe1462a2eUL, 0x66de36e1UL, 0x7fc507a0UL,
			0x54e85463UL, 0x4df36522UL, 0x02b2f3e5UL, 0x1ba9c2a4UL, 0x30849167UL,
			0x299fa026UL, 0xe4c5aeb8UL, 0xfdde9ff9UL, 0xd6f3cc3aUL, 0xcfe8fd7bUL,
			0x80a96bbcUL, 0x99b25afdUL, 0xb29f093eUL, 0xab84387fUL, 0x2c1c24b0UL,
			0x350715f1UL, 0x1e2a4632UL, 0x07317773UL, 0x4870e1b4UL, 0x516bd0f5UL,
			0x7a468336UL, 0x635db277UL, 0xcbfad74eUL, 0xd2e1e60fUL, 0xf9ccb5ccUL,
			0xe0d7848dUL, 0xaf96124aUL, 0xb68d230bUL, 0x9da070c8UL, 0x84bb4189UL,
			0x03235d46UL, 0x1a386c07UL, 0x31153fc4UL, 0x280e0e85UL, 0x674f9842UL,
			0x7e54a903UL, 0x5579fac0UL, 0x4c62cb81UL, 0x8138c51fUL, 0x9823f45eUL,
			0xb30ea79dUL, 0xaa1596dcUL, 0xe554001bUL, 0xfc4f315aUL, 0xd7626299UL,
			0xce7953d8UL, 0x49e14f17UL, 0x50fa7e56UL, 0x7bd72d95UL, 0x62cc1cd4UL,
			0x2d8d8a13UL, 0x3496bb52UL, 0x1fbbe891UL, 0x06a0d9d0UL, 0x5e7ef3ecUL,
			0x4765c2adUL, 0x6c48916eUL, 0x7553a02fUL, 0x3a1236e8UL, 0x230907a9UL,
			0x0824546aUL, 0x113f652bUL, 0x96a779e4UL, 0x8fbc48a5UL, 0xa4911b66UL,
			0xbd8a2a27UL, 0xf2cbbce0UL, 0xebd08da1UL, 0xc0fdde62UL, 0xd9e6ef23UL,
			0x14bce1bdUL, 0x0da7d0fcUL, 0x268a833fUL, 0x3f91b27eUL, 0x70d024b9UL,
			0x69cb15f8UL, 0x42e6463bUL, 0x5bfd777aUL, 0xdc656bb5UL, 0xc57e5af4UL,
			0xee530937UL, 0xf7483876UL, 0xb809aeb1UL, 0xa1129ff0UL, 0x8a3fcc33UL,
			0x9324fd72UL
		},
		{
			0x00000000UL, 0x01c26a37UL, 0x0384d46eUL, 0x0246be59UL, 0x0709a8dcUL,
			0x06cbc2ebUL, 0x048d7cb2UL, 0x054f1685UL, 0x0e1351b8UL, 0x0fd13b8fUL,
			0x0d9785d6UL, 0x0c55efe1UL, 0x091af964UL, 0x08d89353UL, 0x0a9e2d0aUL,
			0x0b5c473dUL, 0x1c26a370UL, 0x1de4c947UL, 0x1fa2771eUL, 0x1e601d29UL,
			0x1b2f0bacUL, 0x1aed619bUL, 0x18abdfc2UL, 0x1969b5f5UL, 0x1235f2c8UL,
			0x13f798ffUL, 0x11b126a6UL, 0x10734c91UL, 0x153c5a14UL, 0x14fe3023UL,
			0x16b88e7aUL, 0x177ae44dUL, 0x384d46e0UL, 0x398f2cd7UL, 0x3bc9928eUL,
			0x3a0bf8b9UL, 0x3f44ee3cUL, 0x3e86840bUL, 0x3cc03a52UL, 0x3d025065UL,
			0x365e1758UL, 0x379c7d6fUL, 0x35dac336UL, 0x3418a901UL, 0x3157bf84UL,
			0x3095d5b3UL, 0x32d36beaUL, 0x331101ddUL, 0x246be590UL, 0x25a98fa7UL,
			0x27ef31feUL, 0x262d5bc9UL, 0x23624d4cUL, 0x22a0277bUL, 0x20e69922UL,
			0x2124f315UL, 0x2a78b428UL, 0x2bbade1fUL, 0x29fc6046UL, 0x283e0a71UL,
			0x2d711cf4UL, 0x2cb376c3UL, 0x2ef5c89aUL, 0x2f37a2adUL, 0x709a8dc0UL,
			0x7158e7f7UL, 0x731e59aeUL, 0x72dc3399UL, 0x7793251cUL, 0x76514f2bUL,
			0x7417f172UL, 0x75d59b45UL, 0x7e89dc78UL, 0x7f4bb64fUL, 0x7d0d0816UL,
			0x7ccf6221UL, 0x798074a4UL, 0x78421e93UL, 0x7a04a0caUL, 0x7bc6cafdUL,
			0x6cbc2eb0UL, 0x6d7e4487UL, 0x6f38fadeUL, 0x6efa90e9UL, 0x6bb5866cUL,
			0x6a77ec5bUL, 0x68315202UL, 0x69f33835UL, 0x62af7f08UL, 0x636d153fUL,
			0x612bab66UL, 0x60e9c151UL, 0x65a6d7d4UL, 0x6464bde3UL, 0x662203baUL,
			0x67e0698dUL, 0x48d7cb20UL, 0x4915a117UL, 0x4b531f4eUL, 0x4a917579UL,
			0x4fde63fcUL, 0x4e1c09cbUL, 0x4c5ab792UL, 0x4d98dda5UL, 0x46c49a98UL,
			0x4706f0afUL, 0x45404ef6UL, 0x448224c1UL, 0x41cd3244UL, 0x400f5873UL,
			0x4249e62aUL, 0x438b8c1dUL, 0x54f16850UL, 0x55330267UL, 0x5775bc3eUL,
			0x56b7d609UL, 0x53f8c08cUL, 0x523aaabbUL, 0x507c14e2UL, 0x51be7ed5UL,
			0x5ae239e8UL, 0x5b2053dfUL, 0x5966ed86UL, 0x58a487b1UL, 0x5deb9134UL,
			0x5c29fb03UL, 0x5e6f455aUL, 0x5fad2f6dUL, 0xe1351b80UL, 0xe0f771b7UL,
			0xe2b1cfeeUL, 0xe373a5d9UL, 0xe63cb35cUL, 0xe7fed96bUL, 0xe5b86732UL,
			0xe47a0d05UL, 0xef264a38UL, 0xeee4200fUL, 0xeca29e56UL, 0xed60f461UL,
			0xe82fe2e4UL, 0xe9ed88d3UL, 0xebab368aUL, 0xea695cbdUL, 0xfd13b8f0UL,
			0xfcd1d2c7UL, 0xfe976c9eUL, 0xff5506a9UL, 0xfa1a102cUL, 0xfbd87a1bUL,
			0xf99ec442UL, 0xf85cae75UL, 0xf300e948UL, 0xf2c2837fUL, 0xf0843d26UL,
			0xf1465711UL, 0xf4094194UL, 0xf5cb2ba3UL, 0xf78d95faUL, 0xf64fffcdUL,
			0xd9785d60UL, 0xd8ba3757UL, 0xdafc890eUL, 0xdb3ee339UL, 0xde71f5bcUL,
			0xdfb39f8bUL, 0xddf521d2UL, 0xdc374be5UL, 0xd76b0cd8UL, 0xd6a966efUL,
			0xd4efd8b6UL, 0xd52db281UL, 0xd062a404UL, 0xd1a0ce33UL, 0xd3e6706aUL,
			0xd2241a5dUL, 0xc55efe10UL, 0xc49c9427UL, 0xc6da2a7eUL, 0xc7184049UL,
			0xc25756ccUL, 0xc3953cfbUL, 0xc1d382a2UL, 0xc011e895UL, 0xcb4dafa8UL,
			0xca8fc59fUL, 0xc8c97bc6UL, 0xc90b11f1UL, 0xcc440774UL, 0xcd866d43UL,
			0xcfc0d31aUL, 0xce02b92dUL, 0x91af9640UL, 0x906dfc77UL, 0x922b422eUL,
			0x93e92819UL, 0x96a63e9cUL, 0x976454abUL, 0x9522eaf2UL, 0x94e080c5UL,
			0x9fbcc7f8UL, 0x9e7eadcfUL, 0x9c381396UL, 0x9dfa79a1UL, 0x98b56f24UL,
			0x99770513UL, 0x9b31bb4aUL, 0x9af3d17dUL, 0x8d893530UL, 0x8c4b5f07UL,
			0x8e0de15eUL, 0x8fcf8b69UL, 0x8a809decUL, 0x8b42f7dbUL, 0x89044982UL,
			0x88c623b5UL, 0x839a6488UL, 0x82580ebfUL, 0x801eb0e6UL, 0x81dcdad1UL,
			0x8493cc54UL, 0x8551a663UL, 0x8717183aUL, 0x86d5720dUL, 0xa9e2d0a0UL,
			0xa820ba97UL, 0xaa6604ceUL, 0xaba46ef9UL, 0xaeeb787cUL, 0xaf29124bUL,
			0xad6fac12UL, 0xacadc625UL, 0xa7f18118UL, 0xa633eb2fUL, 0xa4755576UL,
			0xa5b73f41UL, 0xa0f829c4UL, 0xa13a43f3UL, 0xa37cfdaaUL, 0xa2be979dUL,
			0xb5c473d0UL, 0xb40619e7UL, 0xb640a7beUL, 0xb782cd89UL, 0xb2cddb0cUL,
			0xb30fb13bUL, 0xb1490f62UL, 0xb08b6555UL, 0xbbd72268UL, 0xba15485fUL,
			0xb853f606UL, 0xb9919c31UL, 0xbcde8ab4UL, 0xbd1ce083UL, 0xbf5a5edaUL,
			0xbe9834edUL
		},
		{
			0x00000000UL, 0xb8bc6765UL, 0xaa09c88bUL, 0x12b5afeeUL, 0x8f629757UL,
			0x37def032UL, 0x256b5fdcUL, 0x9dd738b9UL, 0xc5b428efUL, 0x7d084f8aUL,
			0x6fbde064UL, 0xd7018701UL, 0x4ad6bfb8UL, 0xf26ad8ddUL, 0xe0df7733UL,
			0x58631056UL, 0x5019579fUL, 0xe8a530faUL, 0xfa109f14UL, 0x42acf871UL,
			0xdf7bc0c8UL, 0x67c7a7adUL, 0x75720843UL, 0xcdce6f26UL, 0x95ad7f70UL,
			0x2d111815UL, 0x3fa4b7fbUL, 0x8718d09eUL, 0x1acfe827UL, 0xa2738f42UL,
			0xb0c620acUL, 0x087a47c9UL, 0xa032af3eUL, 0x188ec85bUL, 0x0a3b67b5UL,
			0xb28700d0UL, 0x2f503869UL, 0x97ec5f0cUL, 0x8559f0e2UL, 0x3de59787UL,
			0x658687d1UL, 0xdd3ae0b4UL, 0xcf8f4f5aUL, 0x7733283fUL, 0xeae41086UL,
			0x525877e3UL, 0x40edd80dUL, 0xf851bf68UL, 0xf02bf8a1UL, 0x48979fc4UL,
			0x5a22302aUL, 0xe29e574fUL, 0x7f496ff6UL, 0xc7f50893UL, 0xd540a77dUL,
			0x6dfcc018UL, 0x359fd04eUL, 0x8d23b72bUL, 0x9f9618c5UL, 0x272a7fa0UL,
			0xbafd4719UL, 0x0241207cUL, 0x10f48f92UL, 0xa848e8f7UL, 0x9b14583dUL,
			0x23a83f58UL, 0x311d90b6UL, 0x89a1f7d3UL, 0x1476cf6aUL, 0xaccaa80fUL,
			0xbe7f07e1UL, 0x06c36084UL, 0x5ea070d2UL, 0xe61c17b7UL, 0xf4a9b859UL,
			0x4c15df3cUL, 0xd1c2e785UL, 0x697e80e0UL, 0x7bcb2f0eUL, 0xc377486bUL,
			0xcb0d0fa2UL, 0x73b168c7UL, 0x6104c729UL, 0xd9b8a04cUL, 0x446f98f5UL,
			0xfcd3ff90UL, 0xee66507eUL, 0x56da371bUL, 0x0eb9274dUL, 0xb6054028UL,
			0xa4b0efc6UL, 0x1c0c88a3UL, 0x81dbb01aUL, 0x3967d77fUL, 0x2bd27891UL,
			0x936e1ff4UL, 0x3b26f703UL, 0x839a9066UL, 0x912f3f88UL, 0x299358edUL,
			0xb4446054UL, 0x0cf80731UL, 0x1e4da8dfUL, 0xa6f1cfbaUL, 0xfe92dfecUL,
			0x462eb889UL, 0x549b1767UL, 0xec277002UL, 0x71f048bbUL, 0xc94c2fdeUL,
			0xdbf98030UL, 0x6345e755UL, 0x6b3fa09cUL, 0xd383c7f9UL, 0xc1366817UL,
			0x798a0f72UL, 0xe45d37cbUL, 0x5ce150aeUL, 0x4e54ff40UL, 0xf6e89825UL,
			0xae8b8873UL, 0x1637ef16UL, 0x048240f8UL, 0xbc3e279dUL, 0x21e91f24UL,
			0x99557841UL, 0x8be0d7afUL, 0x335cb0caUL, 0xed59b63bUL, 0x55e5d15eUL,
			0x47507eb0UL, 0xffec19d5UL, 0x623b216cUL, 0xda874609UL, 0xc832e9e7UL,
			0x708e8e82UL, 0x28ed9ed4UL, 0x9051f9b1UL, 0x82e4565fUL, 0x3a58313aUL,
			0xa78f0983UL, 0x1f336ee6UL, 0x0d86c108UL, 0xb53aa66dUL, 0xbd40e1a4UL,
			0x05fc86c1UL, 0x1749292fUL, 0xaff54e4aUL, 0x322276f3UL, 0x8a9e1196UL,
			0x982bbe78UL, 0x2097d91dUL, 0x78f4c94bUL, 0xc048ae2eUL, 0xd2fd01c0UL,
			0x6a4166a5UL, 0xf7965e1cUL, 0x4f2a3979UL, 0x5d9f9697UL, 0xe523f1f2UL,
			0x4d6b1905UL, 0xf5d77e60UL, 0xe762d18eUL, 0x5fdeb6ebUL, 0xc2098e52UL,
			0x7ab5e937UL, 0x680046d9UL, 0xd0bc21bcUL, 0x88df31eaUL, 0x3063568fUL,
			0x22d6f961UL, 0x9a6a9e04UL, 0x07bda6bdUL, 0xbf01c1d8UL, 0xadb46e36UL,
			0x15080953UL, 0x1d724e9aUL, 0xa5ce29ffUL, 0xb77b8611UL, 0x0fc7e174UL,
			0x9210d9cdUL, 0x2aacbea8UL, 0x38191146UL, 0x80a57623UL, 0xd8c66675UL,
			0x607a0110UL, 0x72cfaefeUL, 0xca73c99bUL, 0x57a4f122UL, 0xef189647UL,
			0xfdad39a9UL, 0x45115eccUL, 0x764dee06UL, 0xcef18963UL, 0xdc44268dUL,
			0x64f841e8UL, 0xf92f7951UL, 0x41931e34UL, 0x5326b1daUL, 0xeb9ad6bfUL,
			0xb3f9c6e9UL, 0x0b45a18cUL, 0x19f00e62UL, 0xa14c6907UL, 0x3c9b51beUL,
			0x842736dbUL, 0x96929935UL, 0x2e2efe50UL, 0x2654b999UL, 0x9ee8defcUL,
			0x8c5d7112UL, 0x34e11677UL, 0xa9362eceUL, 0x118a49abUL, 0x033fe645UL,
			0xbb838120UL, 0xe3e09176UL, 0x5b5cf613UL, 0x49e959fdUL, 0xf1553e98UL,
			0x6c820621UL, 0xd43e6144UL, 0xc68bceaaUL, 0x7e37a9cfUL, 0xd67f4138UL,
			0x6ec3265dUL, 0x7c7689b3UL, 0xc4caeed6UL, 0x591dd66fUL, 0xe1a1b10aUL,
			0xf3141ee4UL, 0x4ba87981UL, 0x13cb69d7UL, 0xab770eb2UL, 0xb9c2a15cUL,
			0x017ec639UL, 0x9ca9fe80UL, 0x241599e5UL, 0x36a0360bUL, 0x8e1c516eUL,
			0x866616a7UL, 0x3eda71c2UL, 0x2c6fde2cUL, 0x94d3b949UL, 0x090481f0UL,
			0xb1b8e695UL, 0xa30d497bUL, 0x1bb12e1eUL, 0x43d23e48UL, 0xfb6e592dUL,
			0xe9dbf6c3UL, 0x516791a6UL, 0xccb0a91fUL, 0x740cce7aUL, 0x66b96194UL,
			0xde0506f1UL
		},
		{
			0x00000000UL, 0x96300777UL, 0x2c610eeeUL, 0xba510999UL, 0x19c46d07UL,
			0x8ff46a70UL, 0x35a563e9UL, 0xa395649eUL, 0x3288db0eUL, 0xa4b8dc79UL,
			0x1ee9d5e0UL, 0x88d9d297UL, 0x2b4cb609UL, 0xbd7cb17eUL, 0x072db8e7UL,
			0x911dbf90UL, 0x6410b71dUL, 0xf220b06aUL, 0x4871b9f3UL, 0xde41be84UL,
			0x7dd4da1aUL, 0xebe4dd6dUL, 0x51b5d4f4UL, 0xc785d383UL, 0x56986c13UL,
			0xc0a86b64UL, 0x7af962fdUL, 0xecc9658aUL, 0x4f5c0114UL, 0xd96c0663UL,
			0x633d0ffaUL, 0xf50d088dUL, 0xc8206e3bUL, 0x5e10694cUL, 0xe44160d5UL,
			0x727167a2UL, 0xd1e4033cUL, 0x47d4044bUL, 0xfd850dd2UL, 0x6bb50aa5UL,
			0xfaa8b535UL, 0x6c98b242UL, 0xd6c9bbdbUL, 0x40f9bcacUL, 0xe36cd832UL,
			0x755cdf45UL, 0xcf0dd6dcUL, 0x593dd1abUL, 0xac30d926UL, 0x3a00de51UL,
			0x8051d7c8UL, 0x1661d0bfUL, 0xb5f4b421UL, 0x23c4b356UL, 0x9995bacfUL,
			0x0fa5bdb8UL, 0x9eb80228UL, 0x0888055fUL, 0xb2d90cc6UL, 0x24e90bb1UL,
			0x877c6f2fUL, 0x114c6858UL, 0xab1d61c1UL, 0x3d2d66b6UL, 0x9041dc76UL,
			0x0671db01UL, 0xbc20d298UL, 0x2a10d5efUL, 0x8985b171UL, 0x1fb5b606UL,
			0xa5e4bf9fUL, 0x33d4b8e8UL, 0xa2c90778UL, 0x34f9000fUL, 0x8ea80996UL,
			0x18980ee1UL, 0xbb0d6a7fUL, 0x2d3d6d08UL, 0x976c6491UL, 0x015c63e6UL,
			0xf4516b6bUL, 0x62616c1cUL, 0xd8306585UL, 0x4e0062f2UL, 0xed95066cUL,
			0x7ba5011bUL, 0xc1f40882UL, 0x57c40ff5UL, 0xc6d9b065UL, 0x50e9b712UL,
			0xeab8be8bUL, 0x7c88b9fcUL, 0xdf1ddd62UL, 0x492dda15UL, 0xf37cd38cUL,
			0x654cd4fbUL, 0x5861b24dUL, 0xce51b53aUL, 0x7400bca3UL, 0xe230bbd4UL,
			0x41a5df4aUL, 0xd795d83dUL, 0x6dc4d1a4UL, 0xfbf4d6d3UL, 0x6ae96943UL,
			0xfcd96e34UL, 0x468867adUL, 0xd0b860daUL, 0x732d0444UL, 0xe51d0333UL,
			0x5f4c0aaaUL, 0xc97c0dddUL, 0x3c710550UL, 0xaa410227UL, 0x10100bbeUL,
			0x86200cc9UL, 0x25b56857UL, 0xb3856f20UL, 0x09d466b9UL, 0x9fe461ceUL,
			0x0ef9de5eUL, 0x98c9d929UL, 0x2298d0b0UL, 0xb4a8d7c7UL, 0x173db359UL,
			0x810db42eUL, 0x3b5cbdb7UL, 0xad6cbac0UL, 0x2083b8edUL, 0xb6b3bf9aUL,
			0x0ce2b603UL, 0x9ad2b174UL, 0x3947d5eaUL, 0xaf77d29dUL, 0x1526db04UL,
			0x8316dc73UL, 0x120b63e3UL, 0x843b6494UL, 0x3e6a6d0dUL, 0xa85a6a7aUL,
			0x0bcf0ee4UL, 0x9dff0993UL, 0x27ae000aUL, 0xb19e077dUL, 0x44930ff0UL,
			0xd2a30887UL, 0x68f2011eUL, 0xfec20669UL, 0x5d5762f7UL, 0xcb676580UL,
			0x71366c19UL, 0xe7066b6eUL, 0x761bd4feUL, 0xe02bd389UL, 0x5a7ada10UL,
			0xcc4add67UL, 0x6fdfb9f9UL, 0xf9efbe8eUL, 0x43beb717UL, 0xd58eb060UL,
			0xe8a3d6d6UL, 0x7e93d1a1UL, 0xc4c2d838UL, 0x52f2df4fUL, 0xf167bbd1UL,
			0x6757bca6UL, 0xdd06b53fUL, 0x4b36b248UL, 0xda2b0dd8UL, 0x4c1b0aafUL,
			0xf64a0336UL, 0x607a0441UL, 0xc3ef60dfUL, 0x55df67a8UL, 0xef8e6e31UL,
			0x79be6946UL, 0x8cb361cbUL, 0x1a8366bcUL, 0xa0d26f25UL, 0x36e26852UL,
			0x95770cccUL, 0x03470bbbUL, 0xb9160222UL, 0x2f260555UL, 0xbe3bbac5UL,
			0x280bbdb2UL, 0x925ab42bUL, 0x046ab35cUL, 0xa7ffd7c2UL, 0x31cfd0b5UL,
			0x8b9ed92cUL, 0x1daede5bUL, 0xb0c2649bUL, 0x26f263ecUL, 0x9ca36a75UL,
			0x0a936d02UL, 0xa906099cUL, 0x3f360eebUL, 0x85670772UL, 0x13570005UL,
			0x824abf95UL, 0x147ab8e2UL, 0xae2bb17bUL, 0x381bb60cUL, 0x9b8ed292UL,
			0x0dbed5e5UL, 0xb7efdc7cUL, 0x21dfdb0bUL, 0xd4d2d386UL, 0x42e2d4f1UL,
			0xf8b3dd68UL, 0x6e83da1fUL, 0xcd16be81UL, 0x5b26b9f6UL, 0xe177b06fUL,
			0x7747b718UL, 0xe65a0888UL, 0x706a0fffUL, 0xca3b0666UL, 0x5c0b0111UL,
			0xff9e658fUL, 0x69ae62f8UL, 0xd3ff6b61UL, 0x45cf6c16UL, 0x78e20aa0UL,
			0xeed20dd7UL, 0x5483044eUL, 0xc2b30339UL, 0x612667a7UL, 0xf71660d0UL,
			0x4d476949UL, 0xdb776e3eUL, 0x4a6ad1aeUL, 0xdc5ad6d9UL, 0x660bdf40UL,
			0xf03bd837UL, 0x53aebca9UL, 0xc59ebbdeUL, 0x7fcfb247UL, 0xe9ffb530UL,
			0x1cf2bdbdUL, 0x8ac2bacaUL, 0x3093b353UL, 0xa6a3b424UL, 0x0536d0baUL,
			0x9306d7cdUL, 0x2957de54UL, 0xbf67d923UL, 0x2e7a66b3UL, 0xb84a61c4UL,
			0x021b685dUL, 0x942b6f2aUL, 0x37be0bb4UL, 0xa18e0cc3UL, 0x1bdf055aUL,
			0x8def022dUL
		},
		{
			0x00000000UL, 0x41311b19UL, 0x82623632UL, 0xc3532d2bUL, 0x04c56c64UL,
			0x45f4777dUL, 0x86a75a56UL, 0xc796414fUL, 0x088ad9c8UL, 0x49bbc2d1UL,
			0x8ae8effaUL, 0xcbd9f4e3UL, 0x0c4fb5acUL, 0x4d7eaeb5UL, 0x8e2d839eUL,
			0xcf1c9887UL, 0x5112c24aUL, 0x1023d953UL, 0xd370f478UL, 0x9241ef61UL,
			0x55d7ae2eUL, 0x14e6b537UL, 0xd7b5981cUL, 0x96848305UL, 0x59981b82UL,
			0x18a9009bUL, 0xdbfa2db0UL, 0x9acb36a9UL, 0x5d5d77e6UL, 0x1c6c6cffUL,
			0xdf3f41d4UL, 0x9e0e5acdUL, 0xa2248495UL, 0xe3159f8cUL, 0x2046b2a7UL,
			0x6177a9beUL, 0xa6e1e8f1UL, 0xe7d0f3e8UL, 0x2483dec3UL, 0x65b2c5daUL,
			0xaaae5d5dUL, 0xeb9f4644UL, 0x28cc6b6fUL, 0x69fd7076UL, 0xae6b3139UL,
			0xef5a2a20UL, 0x2c09070bUL, 0x6d381c12UL, 0xf33646dfUL, 0xb2075dc6UL,
			0x715470edUL, 0x30656bf4UL, 0xf7f32abbUL, 0xb6c231a2UL, 0x75911c89UL,
			0x34a00790UL, 0xfbbc9f17UL, 0xba8d840eUL, 0x79dea925UL, 0x38efb23cUL,
			0xff79f373UL, 0xbe48e86aUL, 0x7d1bc541UL, 0x3c2ade58UL, 0x054f79f0UL,
			0x447e62e9UL, 0x872d4fc2UL, 0xc61c54dbUL, 0x018a1594UL, 0x40bb0e8dUL,
			0x83e823a6UL, 0xc2d938bfUL, 0x0dc5a038UL, 0x4cf4bb21UL, 0x8fa7960aUL,
			0xce968d13UL, 0x0900cc5cUL, 0x4831d745UL, 0x8b62fa6eUL, 0xca53e177UL,
			0x545dbbbaUL, 0x156ca0a3UL, 0xd63f8d88UL, 0x970e9691UL, 0x5098d7deUL,
			0x11a9ccc7UL, 0xd2fae1ecUL, 0x93cbfaf5UL, 0x5cd76272UL, 0x1de6796bUL,
			0xdeb55440UL, 0x9f844f59UL, 0x58120e16UL, 0x1923150fUL, 0xda703824UL,
			0x9b41233dUL, 0xa76bfd65UL, 0xe65ae67cUL, 0x2509cb57UL, 0x6438d04eUL,
			0xa3ae9101UL, 0xe29f8a18UL, 0x21cca733UL, 0x60fdbc2aUL, 0xafe124adUL,
			0xeed03fb4UL, 0x2d83129fUL, 0x6cb20986UL, 0xab2448c9UL, 0xea1553d0UL,
			0x29467efbUL, 0x687765e2UL, 0xf6793f2fUL, 0xb7482436UL, 0x741b091dUL,
			0x352a1204UL, 0xf2bc534bUL, 0xb38d4852UL, 0x70de6579UL, 0x31ef7e60UL,
			0xfef3e6e7UL, 0xbfc2fdfeUL, 0x7c91d0d5UL, 0x3da0cbccUL, 0xfa368a83UL,
			0xbb07919aUL, 0x7854bcb1UL, 0x3965a7a8UL, 0x4b98833bUL, 0x0aa99822UL,
			0xc9fab509UL, 0x88cbae10UL, 0x4f5def5fUL, 0x0e6cf446UL, 0xcd3fd96dUL,
			0x8c0ec274UL, 0x43125af3UL, 0x022341eaUL, 0xc1706cc1UL, 0x804177d8UL,
			0x47d73697UL, 0x06e62d8eUL, 0xc5b500a5UL, 0x84841bbcUL, 0x1a8a4171UL,
			0x5bbb5a68UL, 0x98e87743UL, 0xd9d96c5aUL, 0x1e4f2d15UL, 0x5f7e360cUL,
			0x9c2d1b27UL, 0xdd1c003eUL, 0x120098b9UL, 0x533183a0UL, 0x9062ae8bUL,
			0xd153b592UL, 0x16c5f4ddUL, 0x57f4efc4UL, 0x94a7c2efUL, 0xd596d9f6UL,
			0xe9bc07aeUL, 0xa88d1cb7UL, 0x6bde319cUL, 0x2aef2a85UL, 0xed796bcaUL,
			0xac4870d3UL, 0x6f1b5df8UL, 0x2e2a46e1UL, 0xe136de66UL, 0xa007c57fUL,
			0x6354e854UL, 0x2265f34dUL, 0xe5f3b202UL, 0xa4c2a91bUL, 0x67918430UL,
			0x26a09f29UL, 0xb8aec5e4UL, 0xf99fdefdUL, 0x3accf3d6UL, 0x7bfde8cfUL,
			0xbc6ba980UL, 0xfd5ab299UL, 0x3e099fb2UL, 0x7f3884abUL, 0xb0241c2cUL,
			0xf1150735UL, 0x32462a1eUL, 0x73773107UL, 0xb4e17048UL, 0xf5d06b51UL,
			0x3683467aUL, 0x77b25d63UL, 0x4ed7facbUL, 0x0fe6e1d2UL, 0xccb5ccf9UL,
			0x8d84d7e0UL, 0x4a1296afUL, 0x0b238db6UL, 0xc870a09dUL, 0x8941bb84UL,
			0x465d2303UL, 0x076c381aUL, 0xc43f1531UL, 0x850e0e28UL, 0x42984f67UL,
			0x03a9547eUL, 0xc0fa7955UL, 0x81cb624cUL, 0x1fc53881UL, 0x5ef42398UL,
			0x9da70eb3UL, 0xdc9615aaUL, 0x1b0054e5UL, 0x5a314ffcUL, 0x996262d7UL,
			0xd85379ceUL, 0x174fe149UL, 0x567efa50UL, 0x952dd77bUL, 0xd41ccc62UL,
			0x138a8d2dUL, 0x52bb9634UL, 0x91e8bb1fUL, 0xd0d9a006UL, 0xecf37e5eUL,
			0xadc26547UL, 0x6e91486cUL, 0x2fa05375UL, 0xe836123aUL, 0xa9070923UL,
			0x6a542408UL, 0x2b653f11UL, 0xe479a796UL, 0xa548bc8fUL, 0x661b91a4UL,
			0x272a8abdUL, 0xe0bccbf2UL, 0xa18dd0ebUL, 0x62defdc0UL, 0x23efe6d9UL,
			0xbde1bc14UL, 0xfcd0a70dUL, 0x3f838a26UL, 0x7eb2913fUL, 0xb924d070UL,
			0xf815cb69UL, 0x3b46e642UL, 0x7a77fd5bUL, 0xb56b65dcUL, 0xf45a7ec5UL,
			0x370953eeUL, 0x763848f7UL, 0xb1ae09b8UL, 0xf09f12a1UL, 0x33cc3f8aUL,
			0x72fd2493UL
		},
		{
			0x00000000UL, 0x376ac201UL, 0x6ed48403UL, 0x59be4602UL, 0xdca80907UL,
			0xebc2cb06UL, 0xb27c8d04UL, 0x85164f05UL, 0xb851130eUL, 0x8f3bd10fUL,
			0xd685970dUL, 0xe1ef550cUL, 0x64f91a09UL, 0x5393d808UL, 0x0a2d9e0aUL,
			0x3d475c0bUL, 0x70a3261cUL, 0x47c9e41dUL, 0x1e77a21fUL, 0x291d601eUL,
			0xac0b2f1bUL, 0x9b61ed1aUL, 0xc2dfab18UL, 0xf5b56919UL, 0xc8f23512UL,
			0xff98f713UL, 0xa626b111UL, 0x914c7310UL, 0x145a3c15UL, 0x2330fe14UL,
			0x7a8eb816UL, 0x4de47a17UL, 0xe0464d38UL, 0xd72c8f39UL, 0x8e92c93bUL,
			0xb9f80b3aUL, 0x3cee443fUL, 0x0b84863eUL, 0x523ac03cUL, 0x6550023dUL,
			0x58175e36UL, 0x6f7d9c37UL, 0x36c3da35UL, 0x01a91834UL, 0x84bf5731UL,
			0xb3d59530UL, 0xea6bd332UL, 0xdd011133UL, 0x90e56b24UL, 0xa78fa925UL,
			0xfe31ef27UL, 0xc95b2d26UL, 0x4c4d6223UL, 0x7b27a022UL, 0x2299e620UL,
			0x15f32421UL, 0x28b4782aUL, 0x1fdeba2bUL, 0x4660fc29UL, 0x710a3e28UL,
			0xf41c712dUL, 0xc376b32cUL, 0x9ac8f52eUL, 0xada2372fUL, 0xc08d9a70UL,
			0xf7e75871UL, 0xae591e73UL, 0x9933dc72UL, 0x1c259377UL, 0x2b4f5176UL,
			0x72f11774UL, 0x459bd575UL, 0x78dc897eUL, 0x4fb64b7fUL, 0x16080d7dUL,
			0x2162cf7cUL, 0xa4748079UL, 0x931e4278UL, 0xcaa0047aUL, 0xfdcac67bUL,
			0xb02ebc6cUL, 0x87447e6dUL, 0xdefa386fUL, 0xe990fa6eUL, 0x6c86b56bUL,
			0x5bec776aUL, 0x02523168UL, 0x3538f369UL, 0x087faf62UL, 0x3f156d63UL,
			0x66ab2b61UL, 0x51c1e960UL, 0xd4d7a665UL, 0xe3bd6464UL, 0xba032266UL,
			0x8d69e067UL, 0x20cbd748UL, 0x17a11549UL, 0x4e1f534bUL, 0x7975914aUL,
			0xfc63de4fUL, 0xcb091c4eUL, 0x92b75a4cUL, 0xa5dd984dUL, 0x989ac446UL,
			0xaff00647UL, 0xf64e4045UL, 0xc1248244UL, 0x4432cd41UL, 0x73580f40UL,
			0x2ae64942UL, 0x1d8c8b43UL, 0x5068f154UL, 0x67023355UL, 0x3ebc7557UL,
			0x09d6b756UL, 0x8cc0f853UL, 0xbbaa3a52UL, 0xe2147c50UL, 0xd57ebe51UL,
			0xe839e25aUL, 0xdf53205bUL, 0x86ed6659UL, 0xb187a458UL, 0x3491eb5dUL,
			0x03fb295cUL, 0x5a456f5eUL, 0x6d2fad5fUL, 0x801b35e1UL, 0xb771f7e0UL,
			0xeecfb1e2UL, 0xd9a573e3UL, 0x5cb33ce6UL, 0x6bd9fee7UL, 0x3267b8e5UL,
			0x050d7ae4UL, 0x384a26efUL, 0x0f20e4eeUL, 0x569ea2ecUL, 0x61f460edUL,
			0xe4e22fe8UL, 0xd388ede9UL, 0x8a36abebUL, 0xbd5c69eaUL, 0xf0b813fdUL,
			0xc7d2d1fcUL, 0x9e6c97feUL, 0xa90655ffUL, 0x2c101afaUL, 0x1b7ad8fbUL,
			0x42c49ef9UL, 0x75ae5cf8UL, 0x48e900f3UL, 0x7f83c2f2UL, 0x263d84f0UL,
			0x115746f1UL, 0x944109f4UL, 0xa32bcbf5UL, 0xfa958df7UL, 0xcdff4ff6UL,
			0x605d78d9UL, 0x5737bad8UL, 0x0e89fcdaUL, 0x39e33edbUL, 0xbcf571deUL,
			0x8b9fb3dfUL, 0xd221f5ddUL, 0xe54b37dcUL, 0xd80c6bd7UL, 0xef66a9d6UL,
			0xb6d8efd4UL, 0x81b22dd5UL, 0x04a462d0UL, 0x33cea0d1UL, 0x6a70e6d3UL,
			0x5d1a24d2UL, 0x10fe5ec5UL, 0x27949cc4UL, 0x7e2adac6UL, 0x494018c7UL,
			0xcc5657c2UL, 0xfb3c95c3UL, 0xa282d3c1UL, 0x95e811c0UL, 0xa8af4dcbUL,
			0x9fc58fcaUL, 0xc67bc9c8UL, 0xf1110bc9UL, 0x740744ccUL, 0x436d86cdUL,
			0x1ad3c0cfUL, 0x2db902ceUL, 0x4096af91UL, 0x77fc6d90UL, 0x2e422b92UL,
			0x1928e993UL, 0x9c3ea696UL, 0xab546497UL, 0xf2ea2295UL, 0xc580e094UL,
			0xf8c7bc9fUL, 0xcfad7e9eUL, 0x9613389cUL, 0xa179fa9dUL, 0x246fb598UL,
			0x13057799UL, 0x4abb319bUL, 0x7dd1f39aUL, 0x3035898dUL, 0x075f4b8cUL,
			0x5ee10d8eUL, 0x698bcf8fUL, 0xec9d808aUL, 0xdbf7428bUL, 0x82490489UL,
			0xb523c688UL, 0x88649a83UL, 0xbf0e5882UL, 0xe6b01e80UL, 0xd1dadc81UL,
			0x54cc9384UL, 0x63a65185UL, 0x3a181787UL, 0x0d72d586UL, 0xa0d0e2a9UL,
			0x97ba20a8UL, 0xce0466aaUL, 0xf96ea4abUL, 0x7c78ebaeUL, 0x4b1229afUL,
			0x12ac6fadUL, 0x25c6adacUL, 0x1881f1a7UL, 0x2feb33a6UL, 0x765575a4UL,
			0x413fb7a5UL, 0xc429f8a0UL, 0xf3433aa1UL, 0xaafd7ca3UL, 0x9d97bea2UL,
			0xd073c4b5UL, 0xe71906b4UL, 0xbea740b6UL, 0x89cd82b7UL, 0x0cdbcdb2UL,
			0x3bb10fb3UL, 0x620f49b1UL, 0x55658bb0UL, 0x6822d7bbUL, 0x5f4815baUL,
			0x06f653b8UL, 0x319c91b9UL, 0xb48adebcUL, 0x83e01cbdUL, 0xda5e5abfUL,
			0xed3498beUL
		},
		{
			0x00000000UL, 0x6567bcb8UL, 0x8bc809aaUL, 0xeeafb512UL, 0x5797628fUL,
			0x32f0de37UL, 0xdc5f6b25UL, 0xb938d79dUL, 0xef28b4c5UL, 0x8a4f087dUL,
			0x64e0bd6fUL, 0x018701d7UL, 0xb8bfd64aUL, 0xddd86af2UL, 0x3377dfe0UL,
			0x56106358UL, 0x9f571950UL, 0xfa30a5e8UL, 0x149f10faUL, 0x71f8ac42UL,
			0xc8c07bdfUL, 0xada7c767UL, 0x43087275UL, 0x266fcecdUL, 0x707fad95UL,
			0x1518112dUL, 0xfbb7a43fUL, 0x9ed01887UL, 0x27e8cf1aUL, 0x428f73a2UL,
			0xac20c6b0UL, 0xc9477a08UL, 0x3eaf32a0UL, 0x5bc88e18UL, 0xb5673b0aUL,
			0xd00087b2UL, 0x6938502fUL, 0x0c5fec97UL, 0xe2f05985UL, 0x8797e53dUL,
			0xd1878665UL, 0xb4e03addUL, 0x5a4f8fcfUL, 0x3f283377UL, 0x8610e4eaUL,
			0xe3775852UL, 0x0dd8ed40UL, 0x68bf51f8UL, 0xa1f82bf0UL, 0xc49f9748UL,
			0x2a30225aUL, 0x4f579ee2UL, 0xf66f497fUL, 0x9308f5c7UL, 0x7da740d5UL,
			0x18c0fc6dUL, 0x4ed09f35UL, 0x2bb7238dUL, 0xc518969fUL, 0xa07f2a27UL,
			0x1947fdbaUL, 0x7c204102UL, 0x928ff410UL, 0xf7e848a8UL, 0x3d58149bUL,
			0x583fa823UL, 0xb6901d31UL, 0xd3f7a189UL, 0x6acf7614UL, 0x0fa8caacUL,
			0xe1077fbeUL, 0x8460c306UL, 0xd270a05eUL, 0xb7171ce6UL, 0x59b8a9f4UL,
			0x3cdf154cUL, 0x85e7c2d1UL, 0xe0807e69UL, 0x0e2fcb7bUL, 0x6b4877c3UL,
			0xa20f0dcbUL, 0xc768b173UL, 0x29c70461UL, 0x4ca0b8d9UL, 0xf5986f44UL,
			0x90ffd3fcUL, 0x7e5066eeUL, 0x1b37da56UL, 0x4d27b90eUL, 0x284005b6UL,
			0xc6efb0a4UL, 0xa3880c1cUL, 0x1ab0db81UL, 0x7fd76739UL, 0x9178d22bUL,
			0xf41f6e93UL, 0x03f7263bUL, 0x66909a83UL, 0x883f2f91UL, 0xed589329UL,
			0x546044b4UL, 0x3107f80cUL, 0xdfa84d1eUL, 0xbacff1a6UL, 0xecdf92feUL,
			0x89b82e46UL, 0x67179b54UL, 0x027027ecUL, 0xbb48f071UL, 0xde2f4cc9UL,
			0x3080f9dbUL, 0x55e74563UL, 0x9ca03f6bUL, 0xf9c783d3UL, 0x176836c1UL,
			0x720f8a79UL, 0xcb375de4UL, 0xae50e15cUL, 0x40ff544eUL, 0x2598e8f6UL,
			0x73888baeUL, 0x16ef3716UL, 0xf8408204UL, 0x9d273ebcUL, 0x241fe921UL,
			0x41785599UL, 0xafd7e08bUL, 0xcab05c33UL, 0x3bb659edUL, 0x5ed1e555UL,
			0xb07e5047UL, 0xd519ecffUL, 0x6c213b62UL, 0x094687daUL, 0xe7e932c8UL,
			0x828e8e70UL, 0xd49eed28UL, 0xb1f95190UL, 0x5f56e482UL, 0x3a31583aUL,
			0x83098fa7UL, 0xe66e331fUL, 0x08c1860dUL, 0x6da63ab5UL, 0xa4e140bdUL,
			0xc186fc05UL, 0x2f294917UL, 0x4a4ef5afUL, 0xf3762232UL, 0x96119e8aUL,
			0x78be2b98UL, 0x1dd99720UL, 0x4bc9f478UL, 0x2eae48c0UL, 0xc001fdd2UL,
			0xa566416aUL, 0x1c5e96f7UL, 0x79392a4fUL, 0x97969f5dUL, 0xf2f123e5UL,
			0x05196b4dUL, 0x607ed7f5UL, 0x8ed162e7UL, 0xebb6de5fUL, 0x528e09c2UL,
			0x37e9b57aUL, 0xd9460068UL, 0xbc21bcd0UL, 0xea31df88UL, 0x8f566330UL,
			0x61f9d622UL, 0x049e6a9aUL, 0xbda6bd07UL, 0xd8c101bfUL, 0x366eb4adUL,
			0x53090815UL, 0x9a4e721dUL, 0xff29cea5UL, 0x11867bb7UL, 0x74e1c70fUL,
			0xcdd91092UL, 0xa8beac2aUL, 0x46111938UL, 0x2376a580UL, 0x7566c6d8UL,
			0x10017a60UL, 0xfeaecf72UL, 0x9bc973caUL, 0x22f1a457UL, 0x479618efUL,
			0xa939adfdUL, 0xcc5e1145UL, 0x06ee4d76UL, 0x6389f1ceUL, 0x8d2644dcUL,
			0xe841f864UL, 0x51792ff9UL, 0x341e9341UL, 0xdab12653UL, 0xbfd69aebUL,
			0xe9c6f9b3UL, 0x8ca1450bUL, 0x620ef019UL, 0x07694ca1UL, 0xbe519b3cUL,
			0xdb362784UL, 0x35999296UL, 0x50fe2e2eUL, 0x99b95426UL, 0xfcdee89eUL,
			0x12715d8cUL, 0x7716e134UL, 0xce2e36a9UL, 0xab498a11UL, 0x45e63f03UL,
			0x208183bbUL, 0x7691e0e3UL, 0x13f65c5bUL, 0xfd59e949UL, 0x983e55f1UL,
			0x2106826cUL, 0x44613ed4UL, 0xaace8bc6UL, 0xcfa9377eUL, 0x38417fd6UL,
			0x5d26c36eUL, 0xb389767cUL, 0xd6eecac4UL, 0x6fd61d59UL, 0x0ab1a1e1UL,
			0xe41e14f3UL, 0x8179a84bUL, 0xd769cb13UL, 0xb20e77abUL, 0x5ca1c2b9UL,
			0x39c67e01UL, 0x80fea99cUL, 0xe5991524UL, 0x0b36a036UL, 0x6e511c8eUL,
			0xa7166686UL, 0xc271da3eUL, 0x2cde6f2cUL, 0x49b9d394UL, 0xf0810409UL,
			0x95e6b8b1UL, 0x7b490da3UL, 0x1e2eb11bUL, 0x483ed243UL, 0x2d596efbUL,
			0xc3f6dbe9UL, 0xa6916751UL, 0x1fa9b0ccUL, 0x7ace0c74UL, 0x9461b966UL,
			0xf10605deUL
	#endif
		}
	};
#endif /* DYNAMIC_CRC_TABLE */
//
// This function can be used by asm versions of crc32()
//
const z_crc_t  * ZEXPORT get_crc_table()
{
#ifdef DYNAMIC_CRC_TABLE
	if(crc_table_empty)
		make_crc_table();
#endif /* DYNAMIC_CRC_TABLE */
	return (const z_crc_t *)crc_table;
}

#define DO1 crc = crc_table[0][((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
#define DO8 DO1; DO1; DO1; DO1; DO1; DO1; DO1; DO1

/* ========================================================================= */
unsigned long ZEXPORT crc32_z(unsigned long crc, const unsigned char  * buf, z_size_t len)
{
	if(buf == Z_NULL) 
		return 0UL;
#ifdef DYNAMIC_CRC_TABLE
	if(crc_table_empty)
		make_crc_table();
#endif /* DYNAMIC_CRC_TABLE */

#ifdef BYFOUR
	if(sizeof(void *) == sizeof(ptrdiff_t)) {
		z_crc_t endian = 1;
		if(*((unsigned char*)(&endian)))
			return crc32_little(crc, buf, len);
		else
			return crc32_big(crc, buf, len);
	}
#endif /* BYFOUR */
	crc = crc ^ 0xffffffffUL;
	while(len >= 8) {
		DO8;
		len -= 8;
	}
	if(len) do {
		DO1;
	} while(--len);
	return crc ^ 0xffffffffUL;
}

unsigned long ZEXPORT crc32(unsigned long crc, const unsigned char  * buf, uInt len)
{
	return crc32_z(crc, buf, len);
}

#ifdef BYFOUR

/*
   This BYFOUR code accesses the passed unsigned char * buffer with a 32-bit
   integer pointer type. This violates the strict aliasing rule, where a
   compiler can assume, for optimization purposes, that two pointers to
   fundamentally different types won't ever point to the same memory. This can
   manifest as a problem only if one of the pointers is written to. This code
   only reads from those pointers. So long as this code remains isolated in
   this compilation unit, there won't be a problem. For this reason, this code
   should not be copied and pasted into a compilation unit in which other code
   writes to the buffer that is passed to these routines.
 */

/* ========================================================================= */
#define DOLIT4 c ^= *buf4++; \
	c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4

/* ========================================================================= */
static unsigned long crc32_little(unsigned long crc, const unsigned char  * buf, z_size_t len)
{
	register z_crc_t c;
	register const z_crc_t  * buf4;
	c = (z_crc_t)crc;
	c = ~c;
	while(len && ((ptrdiff_t)buf & 3)) {
		c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
		len--;
	}
	buf4 = (const z_crc_t *)(const void *)buf;
	while(len >= 32) {
		DOLIT32;
		len -= 32;
	}
	while(len >= 4) {
		DOLIT4;
		len -= 4;
	}
	buf = (const unsigned char *)buf4;
	if(len) do {
		c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
	} while(--len);
	c = ~c;
	return (unsigned long)c;
}

/* ========================================================================= */
#define DOBIG4 c ^= *buf4++; \
	c = crc_table[4][c & 0xff] ^ crc_table[5][(c >> 8) & 0xff] ^ crc_table[6][(c >> 16) & 0xff] ^ crc_table[7][c >> 24]
#define DOBIG32 DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4

/* ========================================================================= */
static unsigned long crc32_big(unsigned long crc, const unsigned char  * buf, z_size_t len)
{
	register const z_crc_t  * buf4;
	register z_crc_t c = ZSWAP32((z_crc_t)crc);
	c = ~c;
	while(len && ((ptrdiff_t)buf & 3)) {
		c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
		len--;
	}
	buf4 = (const z_crc_t *)(const void *)buf;
	while(len >= 32) {
		DOBIG32;
		len -= 32;
	}
	while(len >= 4) {
		DOBIG4;
		len -= 4;
	}
	buf = (const unsigned char *)buf4;
	if(len) do {
		c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
	} while(--len);
	c = ~c;
	return (unsigned long)(ZSWAP32(c));
}

#endif /* BYFOUR */

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */

static unsigned long FASTCALL gf2_matrix_times(unsigned long * mat, unsigned long vec)
{
	unsigned long sum = 0;
	while(vec) {
		if(vec & 1)
			sum ^= *mat;
		vec >>= 1;
		mat++;
	}
	return sum;
}

static void FASTCALL gf2_matrix_square(unsigned long * square, unsigned long * mat)
{
	for(int n = 0; n < GF2_DIM; n++)
		square[n] = gf2_matrix_times(mat, mat[n]);
}

static uLong crc32_combine_(uLong crc1, uLong crc2, z_off64_t len2)
{
	int n;
	unsigned long row;
	unsigned long even[GF2_DIM]; /* even-power-of-two zeros operator */
	unsigned long odd[GF2_DIM]; /* odd-power-of-two zeros operator */
	// degenerate case (also disallow negative lengths) 
	if(len2 <= 0)
		return crc1;
	// put operator for one zero bit in odd 
	odd[0] = 0xedb88320UL;      /* CRC-32 polynomial */
	row = 1;
	for(n = 1; n < GF2_DIM; n++) {
		odd[n] = row;
		row <<= 1;
	}
	// put operator for two zero bits in even 
	gf2_matrix_square(even, odd);
	// put operator for four zero bits in odd 
	gf2_matrix_square(odd, even);
	// apply len2 zeros to crc1 (first square will put the operator for one
	// zero byte, eight zero bits, in even) 
	do {
		/* apply zeros operator for this bit of len2 */
		gf2_matrix_square(even, odd);
		if(len2 & 1)
			crc1 = gf2_matrix_times(even, crc1);
		len2 >>= 1;
		/* if no more bits set, then done */
		if(len2 == 0)
			break;
		/* another iteration of the loop with odd and even swapped */
		gf2_matrix_square(odd, even);
		if(len2 & 1)
			crc1 = gf2_matrix_times(odd, crc1);
		len2 >>= 1;
		/* if no more bits set, then done */
	} while(len2 != 0);
	/* return combined crc */
	crc1 ^= crc2;
	return crc1;
}

uLong ZEXPORT crc32_combine(uLong crc1, uLong crc2, z_off_t len2)
{
	return crc32_combine_(crc1, crc2, len2);
}

uLong ZEXPORT crc32_combine64(uLong crc1, uLong crc2, z_off64_t len2)
{
	return crc32_combine_(crc1, crc2, len2);
}
//
// ZUTIL
//
z_const char * const z_errmsg[10] = {
	(z_const char*)"need dictionary",  /* Z_NEED_DICT       2  */
	(z_const char*)"stream end",       /* Z_STREAM_END      1  */
	(z_const char*)"",                 /* Z_OK              0  */
	(z_const char*)"file error",       /* Z_ERRNO         (-1) */
	(z_const char*)"stream error",     /* Z_STREAM_ERROR  (-2) */
	(z_const char*)"data error",       /* Z_DATA_ERROR    (-3) */
	(z_const char*)"insufficient memory", /* Z_MEM_ERROR     (-4) */
	(z_const char*)"buffer error",     /* Z_BUF_ERROR     (-5) */
	(z_const char*)"incompatible version", /* Z_VERSION_ERROR (-6) */
	(z_const char*)""
};

const char * ZEXPORT zlibVersion()
{
	return ZLIB_VERSION;
}

uLong ZEXPORT zlibCompileFlags()
{
	uLong flags = 0;
	switch((int)(sizeof(uInt))) {
		case 2:     break;
		case 4:     flags += 1;     break;
		case 8:     flags += 2;     break;
		default:    flags += 3;
	}
	switch((int)(sizeof(uLong))) {
		case 2:     break;
		case 4:     flags += 1 << 2;        break;
		case 8:     flags += 2 << 2;        break;
		default:    flags += 3 << 2;
	}
	switch((int)(sizeof(voidpf))) {
		case 2:     break;
		case 4:     flags += 1 << 4;        break;
		case 8:     flags += 2 << 4;        break;
		default:    flags += 3 << 4;
	}
	switch((int)(sizeof(z_off_t))) {
		case 2:     break;
		case 4:     flags += 1 << 6;        break;
		case 8:     flags += 2 << 6;        break;
		default:    flags += 3 << 6;
	}
#ifdef ZLIB_DEBUG
	flags += 1 << 8;
#endif
#if defined(ASMV) || defined(ASMINF)
	flags += 1 << 9;
#endif
#ifdef ZLIB_WINAPI
	flags += 1 << 10;
#endif
#ifdef BUILDFIXED
	flags += 1 << 12;
#endif
#ifdef DYNAMIC_CRC_TABLE
	flags += 1 << 13;
#endif
#ifdef NO_GZCOMPRESS
	flags += 1L << 16;
#endif
#ifdef NO_GZIP
	flags += 1L << 17;
#endif
#ifdef PKZIP_BUG_WORKAROUND
	flags += 1L << 20;
#endif
#ifdef FASTEST
	flags += 1L << 21;
#endif
#if defined(STDC) || defined(Z_HAVE_STDARG_H)
#  ifdef NO_vsnprintf
	flags += 1L << 25;
#    ifdef HAS_vsprintf_void
	flags += 1L << 26;
#    endif
#  else
#    ifdef HAS_vsnprintf_void
	flags += 1L << 26;
#    endif
#  endif
#else
	flags += 1L << 24;
#  ifdef NO_snprintf
	flags += 1L << 25;
#    ifdef HAS_sprintf_void
	flags += 1L << 26;
#    endif
#  else
#    ifdef HAS_snprintf_void
	flags += 1L << 26;
#    endif
#  endif
#endif
	return flags;
}

#ifdef ZLIB_DEBUG
	#include <stdlib.h>
	#ifndef verbose
		#define verbose 0
	#endif
	
	int ZLIB_INTERNAL z_verbose = verbose;

	void ZLIB_INTERNAL z_error(char * m)
	{
		fprintf(stderr, "%s\n", m);
		exit(1);
	}
#endif

/* exported to allow conversion of error code to string for compress() and
 * uncompress()
 */
const char * ZEXPORT zError(int err)
{
	return ERR_MSG(err);
}

#if defined(_WIN32_WCE)
/* The Microsoft C Run-Time Library for Windows CE doesn't have
 * errno.  We define it as a global variable to simplify porting.
 * Its value is always 0 and should not be used.
 */
int errno = 0;
#endif

#ifndef HAVE_MEMCPY
	void ZLIB_INTERNAL zmemcpy(Bytef* dest, const Bytef* source, uInt len)
	{
		if(len) {
			do {
				*dest++ = *source++; /* ??? to be unrolled */
			} while(--len != 0);
		}
	}
	int ZLIB_INTERNAL zmemcmp(const Bytef* s1, const Bytef* s2, uInt len)
	{
		uInt j;
		for(j = 0; j < len; j++) {
			if(s1[j] != s2[j]) return 2*(s1[j] > s2[j])-1;
		}
		return 0;
	}
	void ZLIB_INTERNAL zmemzero(Bytef* dest, uInt len)
	{
		if(len) {
			do {
				*dest++ = 0; /* ??? to be unrolled */
			} while(--len != 0);
		}
	}
#endif

#ifndef Z_SOLO

#ifdef SYS16BIT

#ifdef __TURBOC__ // Turbo C in 16-bit mode 
	#define MY_ZCALLOC
	// Turbo C malloc() does not allow dynamic allocation of 64K bytes
	// and farmalloc(64K) returns a pointer with an offset of 8, so we
	// must fix the pointer. Warning: the pointer must be put back to its
	// original form in order to free it, use zcfree().
	// 
	#define MAX_PTR 10 // 10*64K = 640K 

	static int next_ptr = 0;

	typedef struct ptr_table_s {
		voidpf org_ptr;
		voidpf new_ptr;
	} ptr_table;

	static ptr_table table[MAX_PTR];
	/* This table is used to remember the original form of pointers
	* to large buffers (64K). Such pointers are normalized with a zero offset.
	* Since MSDOS is not a preemptive multitasking OS, this table is not
	* protected from concurrent access. This hack doesn't work anyway on
	* a protected system like OS/2. Use Microsoft C instead.
	*/
	voidpf ZLIB_INTERNAL zcalloc(voidpf opaque, unsigned items, unsigned size)
	{
		voidpf buf;
		ulg bsize = (ulg)items*size;
		(void)opaque;
		/* If we allocate less than 65520 bytes, we assume that farmalloc
		* will return a usable pointer which doesn't have to be normalized.
		*/
		if(bsize < 65520L) {
			buf = farmalloc(bsize);
			if(*(ush*)&buf != 0) return buf;
		}
		else {
			buf = farmalloc(bsize + 16L);
		}
		if(buf == NULL || next_ptr >= MAX_PTR) 
			return NULL;
		table[next_ptr].org_ptr = buf;
		/* Normalize the pointer to seg:0 */
		*((ush*)&buf+1) += ((ush)((uch*)buf-0) + 15) >> 4;
		*(ush*)&buf = 0;
		table[next_ptr++].new_ptr = buf;
		return buf;
	}
	void ZLIB_INTERNAL zcfree(voidpf opaque, voidpf ptr)
	{
		int n;
		(void)opaque;
		if(*(ush*)&ptr != 0) { /* object < 64K */
			farfree(ptr);
			return;
		}
		// Find the original pointer 
		for(n = 0; n < next_ptr; n++) {
			if(ptr != table[n].new_ptr) 
				continue;
			farfree(table[n].org_ptr);
			while(++n < next_ptr) {
				table[n-1] = table[n];
			}
			next_ptr--;
			return;
		}
		Assert(0, "zcfree: ptr not found");
	}
#endif /* __TURBOC__ */
#ifdef M_I86
	// Microsoft C in 16-bit mode 
	#define MY_ZCALLOC
	#if (!defined(_MSC_VER) || (_MSC_VER <= 600))
		#define _halloc  halloc
		#define _hfree   hfree
	#endif

	voidpf ZLIB_INTERNAL zcalloc(voidpf opaque, uInt items, uInt size)
	{
		(void)opaque;
		return _halloc((long)items, size);
	}

	void ZLIB_INTERNAL zcfree(voidpf opaque, voidpf ptr)
	{
		(void)opaque;
		_hfree(ptr);
	}
#endif /* M_I86 */

#endif /* SYS16BIT */

#ifndef MY_ZCALLOC /* Any system without a special alloc function */

#ifndef STDC
	extern voidp malloc(uInt size);
	extern voidp calloc(uInt items, uInt size);
	extern void free(voidpf ptr);
#endif

voidpf ZLIB_INTERNAL zcalloc(voidpf opaque, unsigned items, unsigned size)
{
	(void)opaque;
	return sizeof(uInt) > 2 ? (voidpf)SAlloc::M(items * size) : (voidpf)calloc(items, size);
}

void ZLIB_INTERNAL zcfree(voidpf opaque, voidpf ptr)
{
	(void)opaque;
	SAlloc::F(ptr);
}

#endif /* MY_ZCALLOC */

#endif /* !Z_SOLO */
//
// UNCOMPR
//
/* ===========================================================================
     Decompresses the source buffer into the destination buffer.  *sourceLen is
   the byte length of the source buffer. Upon entry, *destLen is the total size
   of the destination buffer, which must be large enough to hold the entire
   uncompressed data. (The size of the uncompressed data must have been saved
   previously by the compressor and transmitted to the decompressor by some
   mechanism outside the scope of this compression library.) Upon exit,
   *destLen is the size of the decompressed data and *sourceLen is the number
   of source bytes consumed. Upon return, source + *sourceLen points to the
   first unused input byte.

     uncompress returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer, or
   Z_DATA_ERROR if the input data was corrupted, including if the input data is
   an incomplete zlib stream.
 */
int ZEXPORT uncompress2(Bytef * dest, uLongf * destLen, const Bytef * source, uLong * sourceLen)
{
	z_stream stream;
	int err;
	const uInt max = (uInt)-1;
	uLong len, left;
	Byte buf[1]; /* for detection of incomplete stream when *destLen == 0 */
	len = *sourceLen;
	if(*destLen) {
		left = *destLen;
		*destLen = 0;
	}
	else {
		left = 1;
		dest = buf;
	}
	stream.next_in = (z_const Bytef*)source;
	stream.avail_in = 0;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	err = inflateInit(&stream);
	if(err != Z_OK) 
		return err;
	stream.next_out = dest;
	stream.avail_out = 0;
	do {
		if(stream.avail_out == 0) {
			stream.avail_out = left > (uLong)max ? max : (uInt)left;
			left -= stream.avail_out;
		}
		if(stream.avail_in == 0) {
			stream.avail_in = len > (uLong)max ? max : (uInt)len;
			len -= stream.avail_in;
		}
		err = inflate(&stream, Z_NO_FLUSH);
	} while(err == Z_OK);
	*sourceLen -= len + stream.avail_in;
	if(dest != buf)
		*destLen = stream.total_out;
	else if(stream.total_out && err == Z_BUF_ERROR)
		left = 1;
	inflateEnd(&stream);
	return (err == Z_STREAM_END) ? Z_OK : (err == Z_NEED_DICT) ? Z_DATA_ERROR : (err == Z_BUF_ERROR && (left + stream.avail_out)) ? Z_DATA_ERROR : err;
}

int ZEXPORT uncompress(Bytef * dest, uLongf * destLen, const Bytef * source, uLong sourceLen)
{
	return uncompress2(dest, destLen, source, &sourceLen);
}
//
// INFTREES
//
#define MAXBITS 15

const char inflate_copyright[] = " inflate 1.2.11 Copyright 1995-2017 Mark Adler ";
/*
   If you use the zlib library in a product, an acknowledgment is welcome
   in the documentation of your product. If for some reason you cannot
   include such an acknowledgment, I would appreciate that you keep this
   copyright string in the executable of your product.
 */
/*
   Build a set of tables to decode the provided canonical Huffman code.
   The code lengths are lens[0..codes-1].  The result starts at *table,
   whose indices are 0..2^bits-1.  work is a writable array of at least
   lens shorts, which is used as a work area.  type is the type of code
   to be generated, CODES, LENS, or DISTS.  On return, zero is success,
   -1 is an invalid code, and +1 means that ENOUGH isn't enough.  table
   on return points to the next available entry's address.  bits is the
   requested root table index bits, and on return it is the actual root
   table index bits.  It will differ if the request is greater than the
   longest code or if it is less than the shortest code.
 */
int ZLIB_INTERNAL inflate_table(codetype type, unsigned short  * lens, unsigned codes, ZInfTreesCode ** table, unsigned  * bits, unsigned short  * work)
{
	unsigned len;           /* a code's length in bits */
	unsigned sym;           /* index of code symbols */
	unsigned min, max;      /* minimum and maximum code lengths */
	unsigned root;          /* number of index bits for root table */
	unsigned curr;          /* number of index bits for current table */
	unsigned drop;          /* code bits to drop for sub-table */
	int left;               /* number of prefix codes available */
	unsigned used;          /* code entries in table used */
	unsigned huff;          /* Huffman code */
	unsigned incr;          /* for incrementing code, index */
	unsigned fill;          /* index for replicating entries */
	unsigned low;           /* low bits for current root entry */
	unsigned mask;          /* mask for low root bits */
	ZInfTreesCode here;              /* table entry for duplication */
	ZInfTreesCode * next;        /* next available space in table */
	const unsigned short  * base; /* base value table to use */
	const unsigned short  * extra; /* extra bits table to use */
	unsigned match;         /* use base and extra for symbol >= match */
	unsigned short count[MAXBITS+1]; /* number of codes of each length */
	unsigned short offs[MAXBITS+1]; /* offsets in table for each length */
	static const unsigned short lbase[31] = { /* Length codes 257..285 base */
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
	};
	static const unsigned short lext[31] = { /* Length codes 257..285 extra */
		16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
		19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 77, 202
	};
	static const unsigned short dbase[32] = { /* Distance codes 0..29 base */
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577, 0, 0
	};
	static const unsigned short dext[32] = { /* Distance codes 0..29 extra */
		16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
		23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
		28, 28, 29, 29, 64, 64
	};

	/*
	   Process a set of code lengths to create a canonical Huffman code.  The
	   code lengths are lens[0..codes-1].  Each length corresponds to the
	   symbols 0..codes-1.  The Huffman code is generated by first sorting the
	   symbols by length from short to long, and retaining the symbol order
	   for codes with equal lengths.  Then the code starts with all zero bits
	   for the first code of the shortest length, and the codes are integer
	   increments for the same length, and zeros are appended as the length
	   increases.  For the deflate format, these bits are stored backwards
	   from their more natural integer increment ordering, and so when the
	   decoding tables are built in the large loop below, the integer codes
	   are incremented backwards.

	   This routine assumes, but does not check, that all of the entries in
	   lens[] are in the range 0..MAXBITS.  The caller must assure this.
	   1..MAXBITS is interpreted as that code length.  zero means that that
	   symbol does not occur in this code.

	   The codes are sorted by computing a count of codes for each length,
	   creating from that a table of starting indices for each length in the
	   sorted table, and then entering the symbols in order in the sorted
	   table.  The sorted table is work[], with that space being provided by
	   the caller.

	   The length counts are used for other purposes as well, i.e. finding
	   the minimum and maximum length codes, determining if there are any
	   codes at all, checking for a valid set of lengths, and looking ahead
	   at length counts to determine sub-table sizes when building the
	   decoding tables.
	 */

	/* accumulate lengths for codes (assumes lens[] all in 0..MAXBITS) */
	for(len = 0; len <= MAXBITS; len++)
		count[len] = 0;
	for(sym = 0; sym < codes; sym++)
		count[lens[sym]]++;

	/* bound code lengths, force root to be within code lengths */
	root = *bits;
	for(max = MAXBITS; max >= 1; max--)
		if(count[max] != 0) break;
	if(root > max) root = max;
	if(max == 0) {                  /* no symbols to code at all */
		here.op = (unsigned char)64; /* invalid code marker */
		here.bits = (unsigned char)1;
		here.val = (unsigned short)0;
		*(*table)++ = here;     /* make a table to force an error */
		*(*table)++ = here;
		*bits = 1;
		return 0; /* no symbols, but wait for decoding to report error */
	}
	for(min = 1; min < max; min++)
		if(count[min] != 0) break;
	if(root < min) root = min;

	/* check for an over-subscribed or incomplete set of lengths */
	left = 1;
	for(len = 1; len <= MAXBITS; len++) {
		left <<= 1;
		left -= count[len];
		if(left < 0) return -1;  /* over-subscribed */
	}
	if(left > 0 && (type == CODES || max != 1))
		return -1;              /* incomplete set */

	/* generate offsets into symbol table for each length for sorting */
	offs[1] = 0;
	for(len = 1; len < MAXBITS; len++)
		offs[len + 1] = offs[len] + count[len];

	/* sort symbols by length, by symbol order within each length */
	for(sym = 0; sym < codes; sym++)
		if(lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;

	/*
	   Create and fill in decoding tables.  In this loop, the table being
	   filled is at next and has curr index bits.  The code being used is huff
	   with length len.  That code is converted to an index by dropping drop
	   bits off of the bottom.  For codes where len is less than drop + curr,
	   those top drop + curr - len bits are incremented through all values to
	   fill the table with replicated entries.

	   root is the number of index bits for the root table.  When len exceeds
	   root, sub-tables are created pointed to by the root entry with an index
	   of the low root bits of huff.  This is saved in low to check for when a
	   new sub-table should be started.  drop is zero when the root table is
	   being filled, and drop is root when sub-tables are being filled.

	   When a new sub-table is needed, it is necessary to look ahead in the
	   code lengths to determine what size sub-table is needed.  The length
	   counts are used for this, and so count[] is decremented as codes are
	   entered in the tables.

	   used keeps track of how many table entries have been allocated from the
	   provided *table space.  It is checked for LENS and DIST tables against
	   the constants ENOUGH_LENS and ENOUGH_DISTS to guard against changes in
	   the initial root table size constants.  See the comments in inftrees.h
	   for more information.

	   sym increments through all symbols, and the loop terminates when
	   all codes of length max, i.e. all codes, have been processed.  This
	   routine permits incomplete codes, so another loop after this one fills
	   in the rest of the decoding tables with invalid code markers.
	 */

	/* set up for code type */
	switch(type) {
		case CODES:
		    base = extra = work; /* dummy value--not used */
		    match = 20;
		    break;
		case LENS:
		    base = lbase;
		    extra = lext;
		    match = 257;
		    break;
		default: /* DISTS */
		    base = dbase;
		    extra = dext;
		    match = 0;
	}

	/* initialize state for loop */
	huff = 0;               /* starting code */
	sym = 0;                /* starting code symbol */
	len = min;              /* starting code length */
	next = *table;          /* current table to fill in */
	curr = root;            /* current table index bits */
	drop = 0;               /* current bits to drop from code for index */
	low = (unsigned)(-1);   /* trigger new sub-table when len > root */
	used = 1U << root;      /* use root table entries */
	mask = used - 1;        /* mask for comparing low */

	/* check available table space */
	if((type == LENS && used > ENOUGH_LENS) || (type == DISTS && used > ENOUGH_DISTS))
		return 1;

	/* process all codes and make table entries */
	for(;; ) {
		/* create table entry */
		here.bits = (unsigned char)(len - drop);
		if(work[sym] + 1U < match) {
			here.op = (unsigned char)0;
			here.val = work[sym];
		}
		else if(work[sym] >= match) {
			here.op = (unsigned char)(extra[work[sym] - match]);
			here.val = base[work[sym] - match];
		}
		else {
			here.op = (unsigned char)(32 + 64); /* end of block */
			here.val = 0;
		}
		// replicate for those indices with low len bits equal to huff 
		incr = 1U << (len - drop);
		fill = 1U << curr;
		min = fill;         /* save offset to next table */
		do {
			fill -= incr;
			next[(huff >> drop) + fill] = here;
		} while(fill != 0);
		// backwards increment the len-bit code huff 
		incr = 1U << (len - 1);
		while(huff & incr)
			incr >>= 1;
		if(incr != 0) {
			huff &= incr - 1;
			huff += incr;
		}
		else
			huff = 0;
		// go to next symbol, update count, len 
		sym++;
		if(--(count[len]) == 0) {
			if(len == max) 
				break;
			len = lens[work[sym]];
		}
		// create new sub-table if needed 
		if(len > root && (huff & mask) != low) {
			// if first time, transition to sub-tables 
			if(drop == 0)
				drop = root;
			// increment past last table 
			next += min; /* here min is 1 << curr */
			// determine length of next table 
			curr = len - drop;
			left = (int)(1 << curr);
			while(curr + drop < max) {
				left -= count[curr + drop];
				if(left <= 0) break;
				curr++;
				left <<= 1;
			}
			// check for enough space 
			used += 1U << curr;
			if((type == LENS && used > ENOUGH_LENS) || (type == DISTS && used > ENOUGH_DISTS))
				return 1;
			// point entry in root table to sub-table 
			low = huff & mask;
			(*table)[low].op = (unsigned char)curr;
			(*table)[low].bits = (unsigned char)root;
			(*table)[low].val = (unsigned short)(next - *table);
		}
	}

	/* fill in remaining table entry if code is incomplete (guaranteed to have
	   at most one remaining entry, since if the code is incomplete, the
	   maximum code length that was allowed to get this far is one bit) */
	if(huff != 0) {
		here.op = (unsigned char)64;    /* invalid code marker */
		here.bits = (unsigned char)(len - drop);
		here.val = (unsigned short)0;
		next[huff] = here;
	}
	// set return parameters 
	*table += used;
	*bits = root;
	return 0;
}
//
// COMPRESS
//
/* ===========================================================================
     Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

     compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
 */
int ZEXPORT compress2(Bytef * dest, uLongf * destLen, const Bytef * source, uLong sourceLen, int level)
{
	z_stream stream;
	int err;
	const uInt max = (uInt)-1;
	uLong left = *destLen;
	*destLen = 0;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	err = deflateInit(&stream, level);
	if(err != Z_OK) 
		return err;
	stream.next_out = dest;
	stream.avail_out = 0;
	stream.next_in = (z_const Bytef*)source;
	stream.avail_in = 0;
	do {
		if(stream.avail_out == 0) {
			stream.avail_out = left > (uLong)max ? max : (uInt)left;
			left -= stream.avail_out;
		}
		if(stream.avail_in == 0) {
			stream.avail_in = sourceLen > (uLong)max ? max : (uInt)sourceLen;
			sourceLen -= stream.avail_in;
		}
		err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
	} while(err == Z_OK);

	*destLen = stream.total_out;
	deflateEnd(&stream);
	return err == Z_STREAM_END ? Z_OK : err;
}

int ZEXPORT compress(Bytef * dest, uLongf * destLen, const Bytef * source, uLong sourceLen)
{
	return compress2(dest, destLen, source, sourceLen, Z_DEFAULT_COMPRESSION);
}
//
// If the default memLevel or windowBits for deflateInit() is changed, then this function needs to be updated.
//
uLong ZEXPORT compressBound(uLong sourceLen)
{
	return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + (sourceLen >> 25) + 13;
}
//
// GZWRITE
//
//
// Initialize state for writing a gzip file.  Mark initialization by setting
// state->size to non-zero.  Return -1 on a memory allocation failure, or 0 on success. 
//
static int gz_init(gz_statep state)
{
	int ret;
	z_streamp strm = &(state->strm);
	// allocate input buffer (double size for gzprintf) 
	state->in = (unsigned char*)SAlloc::M(state->want << 1);
	if(state->in == NULL) {
		gz_error(state, Z_MEM_ERROR, "out of memory");
		return -1;
	}
	// only need output buffer and deflate state if compressing 
	if(!state->direct) {
		// allocate output buffer 
		state->out = (unsigned char*)SAlloc::M(state->want);
		if(state->out == NULL) {
			SAlloc::F(state->in);
			gz_error(state, Z_MEM_ERROR, "out of memory");
			return -1;
		}
		// allocate deflate memory, set up for gzip compression 
		strm->zalloc = Z_NULL;
		strm->zfree = Z_NULL;
		strm->opaque = Z_NULL;
		ret = deflateInit2(strm, state->level, Z_DEFLATED, MAX_WBITS + 16, DEF_MEM_LEVEL, state->strategy);
		if(ret != Z_OK) {
			SAlloc::F(state->out);
			SAlloc::F(state->in);
			gz_error(state, Z_MEM_ERROR, "out of memory");
			return -1;
		}
		strm->next_in = NULL;
	}
	// mark state as initialized 
	state->size = state->want;
	// initialize write buffer if compressing 
	if(!state->direct) {
		strm->avail_out = state->size;
		strm->next_out = state->out;
		state->x.next = strm->next_out;
	}
	return 0;
}

/* Compress whatever is at avail_in and next_in and write to the output file.
   Return -1 if there is an error writing to the output file or if gz_init()
   fails to allocate memory, otherwise 0.  flush is assumed to be a valid
   deflate() flush value.  If flush is Z_FINISH, then the deflate() state is
   reset to start a new gzip stream.  If gz->direct is true, then simply write
   to the output file without compressing, and ignore flush. */
static int gz_comp(gz_statep state, int flush)
{
	int ret, writ;
	unsigned have, put, max = ((unsigned)-1 >> 2) + 1;
	z_streamp strm = &(state->strm);
	/* allocate memory if this is the first time through */
	if(state->size == 0 && gz_init(state) == -1)
		return -1;
	/* write directly if requested */
	if(state->direct) {
		while(strm->avail_in) {
			put = strm->avail_in > max ? max : strm->avail_in;
			writ = write(state->fd, strm->next_in, put);
			if(writ < 0) {
				gz_error(state, Z_ERRNO, zstrerror());
				return -1;
			}
			strm->avail_in -= (unsigned)writ;
			strm->next_in += writ;
		}
		return 0;
	}
	/* run deflate() on provided input until it produces no more output */
	ret = Z_OK;
	do {
		/* write out current buffer contents if full, or if flushing, but if
		   doing Z_FINISH then don't write until we get to Z_STREAM_END */
		if(strm->avail_out == 0 || (flush != Z_NO_FLUSH && (flush != Z_FINISH || ret == Z_STREAM_END))) {
			while(strm->next_out > state->x.next) {
				put = strm->next_out - state->x.next > (int)max ? max : (unsigned)(strm->next_out - state->x.next);
				writ = write(state->fd, state->x.next, put);
				if(writ < 0) {
					gz_error(state, Z_ERRNO, zstrerror());
					return -1;
				}
				state->x.next += writ;
			}
			if(strm->avail_out == 0) {
				strm->avail_out = state->size;
				strm->next_out = state->out;
				state->x.next = state->out;
			}
		}
		/* compress */
		have = strm->avail_out;
		ret = deflate(strm, flush);
		if(ret == Z_STREAM_ERROR) {
			gz_error(state, Z_STREAM_ERROR, "internal error: deflate stream corrupt");
			return -1;
		}
		have -= strm->avail_out;
	} while(have);
	/* if that completed a deflate stream, allow another to start */
	if(flush == Z_FINISH)
		deflateReset(strm);
	/* all done, no errors */
	return 0;
}
//
// Compress len zeros to output.  Return -1 on a write error or memory
// allocation failure by gz_comp(), or 0 on success. 
//
static int gz_zero(gz_statep state, z_off64_t len)
{
	int first;
	unsigned n;
	z_streamp strm = &(state->strm);
	/* consume whatever's left in the input buffer */
	if(strm->avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
		return -1;
	/* compress len zeros (len guaranteed > 0) */
	first = 1;
	while(len) {
		n = (GT_OFF(state->size) || (z_off64_t)state->size > len) ? (unsigned)len : state->size;
		if(first) {
			memzero(state->in, n);
			first = 0;
		}
		strm->avail_in = n;
		strm->next_in = state->in;
		state->x.pos += n;
		if(gz_comp(state, Z_NO_FLUSH) == -1)
			return -1;
		len -= n;
	}
	return 0;
}

/* Write len bytes from buf to file.  Return the number of bytes written.  If
   the returned value is less than len, then there was an error. */
static z_size_t gz_write(gz_statep state, voidpc buf, z_size_t len)
{
	z_size_t put = len;
	// if len is zero, avoid unnecessary operations 
	if(len == 0)
		return 0;
	// allocate memory if this is the first time through 
	if(state->size == 0 && gz_init(state) == -1)
		return 0;
	// check for seek request 
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			return 0;
	}
	// for small len, copy to input buffer, otherwise compress directly 
	if(len < state->size) {
		// copy to input buffer, compress when full 
		do {
			unsigned have, copy;
			if(state->strm.avail_in == 0)
				state->strm.next_in = state->in;
			have = (unsigned)((state->strm.next_in + state->strm.avail_in) - state->in);
			copy = state->size - have;
			if(copy > len)
				copy = len;
			memcpy(state->in + have, buf, copy);
			state->strm.avail_in += copy;
			state->x.pos += copy;
			buf = (const char*)buf + copy;
			len -= copy;
			if(len && gz_comp(state, Z_NO_FLUSH) == -1)
				return 0;
		} while(len);
	}
	else {
		// consume whatever's left in the input buffer 
		if(state->strm.avail_in && gz_comp(state, Z_NO_FLUSH) == -1)
			return 0;
		// directly compress user buffer to file 
		state->strm.next_in = (z_const Bytef*)buf;
		do {
			unsigned n = (unsigned)-1;
			if(n > len)
				n = len;
			state->strm.avail_in = n;
			state->x.pos += n;
			if(gz_comp(state, Z_NO_FLUSH) == -1)
				return 0;
			len -= n;
		} while(len);
	}
	// input was all buffered or compressed 
	return put;
}

int ZEXPORT gzwrite(gzFile file, voidpc buf, unsigned len)
{
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return 0;
	state = (gz_statep)file;
	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return 0;
	/* since an int is returned, make sure len fits in one, otherwise return
	   with an error (this avoids a flaw in the interface) */
	if((int)len < 0) {
		gz_error(state, Z_DATA_ERROR, "requested length does not fit in int");
		return 0;
	}
	/* write len bytes from buf (the return value will fit in an int) */
	return (int)gz_write(state, buf, len);
}

z_size_t ZEXPORT gzfwrite(voidpc buf, z_size_t size, z_size_t nitems, gzFile file)
{
	z_size_t len;
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return 0;
	state = (gz_statep)file;

	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return 0;

	/* compute bytes to read -- error on overflow */
	len = nitems * size;
	if(size && len / size != nitems) {
		gz_error(state, Z_STREAM_ERROR, "request does not fit in a size_t");
		return 0;
	}
	/* write len bytes to buf, return the number of full items written */
	return len ? gz_write(state, buf, len) / size : 0;
}

int ZEXPORT gzputc(gzFile file, int c)
{
	unsigned have;
	unsigned char buf[1];
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	strm = &(state->strm);

	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return -1;

	/* check for seek request */
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			return -1;
	}
	/* try writing to input buffer for speed (state->size == 0 if buffer not
	   initialized) */
	if(state->size) {
		if(strm->avail_in == 0)
			strm->next_in = state->in;
		have = (unsigned)((strm->next_in + strm->avail_in) - state->in);
		if(have < state->size) {
			state->in[have] = (unsigned char)c;
			strm->avail_in++;
			state->x.pos++;
			return c & 0xff;
		}
	}
	/* no room in buffer or not initialized, use gz_write() */
	buf[0] = (unsigned char)c;
	if(gz_write(state, buf, 1) != 1)
		return -1;
	return c & 0xff;
}

int ZEXPORT gzputs(gzFile file, const char * str)
{
	int ret;
	z_size_t len;
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return -1;
	/* write string */
	len = strlen(str);
	ret = gz_write(state, str, len);
	return ret == 0 && len != 0 ? -1 : ret;
}

#if defined(STDC) || defined(Z_HAVE_STDARG_H)
//#include <stdarg.h>

int ZEXPORTVA gzvprintf(gzFile file, const char * format, va_list va)
{
	int len;
	unsigned left;
	char * next;
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	strm = &(state->strm);

	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return Z_STREAM_ERROR;

	/* make sure we have some buffer space */
	if(state->size == 0 && gz_init(state) == -1)
		return state->err;

	/* check for seek request */
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			return state->err;
	}

	/* do the printf() into the input buffer, put length in len -- the input
	   buffer is double-sized just for this function, so there is guaranteed to
	   be state->size bytes available after the current contents */
	if(strm->avail_in == 0)
		strm->next_in = state->in;
	next = (char*)(state->in + (strm->next_in - state->in) + strm->avail_in);
	next[state->size - 1] = 0;
#ifdef NO_vsnprintf
#  ifdef HAS_vsprintf_void
	(void)vsprintf(next, format, va);
	for(len = 0; len < state->size; len++)
		if(next[len] == 0) break;
#  else
	len = vsprintf(next, format, va);
#  endif
#else
#  ifdef HAS_vsnprintf_void
	(void)vsnprintf(next, state->size, format, va);
	len = strlen(next);
#  else
	len = vsnprintf(next, state->size, format, va);
#  endif
#endif
	// check that printf() results fit in buffer
	if(len == 0 || (unsigned)len >= state->size || next[state->size - 1] != 0)
		return 0;
	// update buffer and position, compress first half if past that 
	strm->avail_in += (unsigned)len;
	state->x.pos += len;
	if(strm->avail_in >= state->size) {
		left = strm->avail_in - state->size;
		strm->avail_in = state->size;
		if(gz_comp(state, Z_NO_FLUSH) == -1)
			return state->err;
		memcpy(state->in, state->in + state->size, left);
		strm->next_in = state->in;
		strm->avail_in = left;
	}
	return len;
}

int ZEXPORTVA gzprintf(gzFile file, const char * format, ...)
{
	va_list va;
	int ret;
	va_start(va, format);
	ret = gzvprintf(file, format, va);
	va_end(va);
	return ret;
}

#else /* !STDC && !Z_HAVE_STDARG_H */

/* -- see zlib.h -- */
int ZEXPORTVA gzprintf(gzFile file,
    const char * format,
    int a1,
    int a2,
    int a3,
    int a4,
    int a5,
    int a6,
    int a7,
    int a8,
    int a9,
    int a10,
    int a11,
    int a12,
    int a13,
    int a14,
    int a15,
    int a16,
    int a17,
    int a18,
    int a19,
    int a20)
{
	unsigned len, left;
	char * next;
	gz_statep state;
	z_streamp strm;

	/* get internal structure */
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	strm = &(state->strm);

	/* check that can really pass pointer in ints */
	if(sizeof(int) != sizeof(void *))
		return Z_STREAM_ERROR;

	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return Z_STREAM_ERROR;

	/* make sure we have some buffer space */
	if(state->size == 0 && gz_init(state) == -1)
		return state->error;

	/* check for seek request */
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			return state->error;
	}

	/* do the printf() into the input buffer, put length in len -- the input
	   buffer is double-sized just for this function, so there is guaranteed to
	   be state->size bytes available after the current contents */
	if(strm->avail_in == 0)
		strm->next_in = state->in;
	next = (char*)(strm->next_in + strm->avail_in);
	next[state->size - 1] = 0;
#ifdef NO_snprintf
#  ifdef HAS_sprintf_void
	sprintf(next, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12,
	    a13, a14, a15, a16, a17, a18, a19, a20);
	for(len = 0; len < size; len++)
		if(next[len] == 0)
			break;
#  else
	len = sprintf(next, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
	    a12, a13, a14, a15, a16, a17, a18, a19, a20);
#  endif
#else
#  ifdef HAS_snprintf_void
	snprintf(next, state->size, format, a1, a2, a3, a4, a5, a6, a7, a8, a9,
	    a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
	len = strlen(next);
#  else
	len = snprintf(next, state->size, format, a1, a2, a3, a4, a5, a6, a7, a8,
	    a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
#  endif
#endif

	/* check that printf() results fit in buffer */
	if(len == 0 || len >= state->size || next[state->size - 1] != 0)
		return 0;

	/* update buffer and position, compress first half if past that */
	strm->avail_in += len;
	state->x.pos += len;
	if(strm->avail_in >= state->size) {
		left = strm->avail_in - state->size;
		strm->avail_in = state->size;
		if(gz_comp(state, Z_NO_FLUSH) == -1)
			return state->err;
		memcpy(state->in, state->in + state->size, left);
		strm->next_in = state->in;
		strm->avail_in = left;
	}
	return (int)len;
}

#endif

int ZEXPORT gzflush(gzFile file, int flush)
{
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return Z_STREAM_ERROR;
	/* check flush parameter */
	if(flush < 0 || flush > Z_FINISH)
		return Z_STREAM_ERROR;
	/* check for seek request */
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			return state->err;
	}
	/* compress remaining data with requested flush */
	(void)gz_comp(state, flush);
	return state->err;
}

int ZEXPORT gzsetparams(gzFile file, int level, int strategy)
{
	gz_statep state;
	z_streamp strm;
	/* get internal structure */
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	strm = &(state->strm);
	/* check that we're writing and that there's no error */
	if(state->mode != GZ_WRITE || state->err != Z_OK)
		return Z_STREAM_ERROR;
	/* if no change is requested, then do nothing */
	if(level == state->level && strategy == state->strategy)
		return Z_OK;
	/* check for seek request */
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			return state->err;
	}
	/* change compression parameters for subsequent input */
	if(state->size) {
		/* flush previous input with previous parameters before changing */
		if(strm->avail_in && gz_comp(state, Z_BLOCK) == -1)
			return state->err;
		deflateParams(strm, level, strategy);
	}
	state->level = level;
	state->strategy = strategy;
	return Z_OK;
}

int ZEXPORT gzclose_w(gzFile file)
{
	int ret = Z_OK;
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	/* check that we're writing */
	if(state->mode != GZ_WRITE)
		return Z_STREAM_ERROR;
	/* check for seek request */
	if(state->seek) {
		state->seek = 0;
		if(gz_zero(state, state->skip) == -1)
			ret = state->err;
	}
	/* flush, free memory, and close file */
	if(gz_comp(state, Z_FINISH) == -1)
		ret = state->err;
	if(state->size) {
		if(!state->direct) {
			(void)deflateEnd(&(state->strm));
			SAlloc::F(state->out);
		}
		SAlloc::F(state->in);
	}
	gz_error(state, Z_OK, NULL);
	SAlloc::F(state->path);
	if(close(state->fd) == -1)
		ret = Z_ERRNO;
	SAlloc::F(state);
	return ret;
}
//
// GZREAD
//
/* Use read() to load a buffer -- return -1 on error, otherwise 0.  Read from
   state->fd, and update state->eof, state->err, and state->msg as appropriate.
   This function needs to loop on read(), since read() is not guaranteed to
   read the number of bytes requested, depending on the type of descriptor. */
static int gz_load(gz_statep state, unsigned char * buf, unsigned len, unsigned * have)
{
	int ret;
	unsigned get;
	unsigned max = ((unsigned)-1 >> 2) + 1;
	*have = 0;
	do {
		get = len - *have;
		if(get > max)
			get = max;
		ret = read(state->fd, buf + *have, get);
		if(ret <= 0)
			break;
		*have += (unsigned)ret;
	} while(*have < len);
	if(ret < 0) {
		gz_error(state, Z_ERRNO, zstrerror());
		return -1;
	}
	if(ret == 0)
		state->eof = 1;
	return 0;
}

/* Load up input buffer and set eof flag if last data loaded -- return -1 on
   error, 0 otherwise.  Note that the eof flag is set when the end of the input
   file is reached, even though there may be unused data in the buffer.  Once
   that data has been used, no more attempts will be made to read the file.
   If strm->avail_in != 0, then the current data is moved to the beginning of
   the input buffer, and then the remainder of the buffer is loaded with the
   available data from the input file. */
static int gz_avail(gz_statep state)
{
	unsigned got;
	z_streamp strm = &(state->strm);
	if(state->err != Z_OK && state->err != Z_BUF_ERROR)
		return -1;
	if(state->eof == 0) {
		if(strm->avail_in) { /* copy what's there to the start */
			unsigned char * p = state->in;
			unsigned const char * q = strm->next_in;
			unsigned n = strm->avail_in;
			do {
				*p++ = *q++;
			} while(--n);
		}
		if(gz_load(state, state->in + strm->avail_in, state->size - strm->avail_in, &got) == -1)
			return -1;
		strm->avail_in += got;
		strm->next_in = state->in;
	}
	return 0;
}

/* Look for gzip header, set up for inflate or copy.  state->x.have must be 0.
   If this is the first time in, allocate required memory.  state->how will be
   left unchanged if there is no more input data available, will be set to COPY
   if there is no gzip header and direct copying will be performed, or it will
   be set to GZIP for decompression.  If direct copying, then leftover input
   data from the input buffer will be copied to the output buffer.  In that
   case, all further file reads will be directly to either the output buffer or
   a user buffer.  If decompressing, the inflate state will be initialized.
   gz_look() will return 0 on success or -1 on failure. */
static int gz_look(gz_statep state)
{
	z_streamp strm = &(state->strm);
	/* allocate read buffers and inflate memory */
	if(state->size == 0) {
		/* allocate buffers */
		state->in = (unsigned char*)SAlloc::M(state->want);
		state->out = (unsigned char*)SAlloc::M(state->want << 1);
		if(state->in == NULL || state->out == NULL) {
			SAlloc::F(state->out);
			SAlloc::F(state->in);
			gz_error(state, Z_MEM_ERROR, "out of memory");
			return -1;
		}
		state->size = state->want;

		/* allocate inflate memory */
		state->strm.zalloc = Z_NULL;
		state->strm.zfree = Z_NULL;
		state->strm.opaque = Z_NULL;
		state->strm.avail_in = 0;
		state->strm.next_in = Z_NULL;
		if(inflateInit2(&(state->strm), 15 + 16) != Z_OK) { /* gunzip */
			SAlloc::F(state->out);
			SAlloc::F(state->in);
			state->size = 0;
			gz_error(state, Z_MEM_ERROR, "out of memory");
			return -1;
		}
	}

	/* get at least the magic bytes in the input buffer */
	if(strm->avail_in < 2) {
		if(gz_avail(state) == -1)
			return -1;
		if(strm->avail_in == 0)
			return 0;
	}

	/* look for gzip magic bytes -- if there, do gzip decoding (note: there is
	   a logical dilemma here when considering the case of a partially written
	   gzip file, to wit, if a single 31 byte is written, then we cannot tell
	   whether this is a single-byte file, or just a partially written gzip
	   file -- for here we assume that if a gzip file is being written, then
	   the header will be written in a single operation, so that reading a
	   single byte is sufficient indication that it is not a gzip file) */
	if(strm->avail_in > 1 && strm->next_in[0] == 31 && strm->next_in[1] == 139) {
		inflateReset(strm);
		state->how = GZIP;
		state->direct = 0;
		return 0;
	}
	/* no gzip header -- if we were decoding gzip before, then this is trailing
	   garbage.  Ignore the trailing garbage and finish. */
	if(state->direct == 0) {
		strm->avail_in = 0;
		state->eof = 1;
		state->x.have = 0;
		return 0;
	}
	/* doing raw i/o, copy any leftover input to output -- this assumes that
	   the output buffer is larger than the input buffer, which also assures
	   space for gzungetc() */
	state->x.next = state->out;
	if(strm->avail_in) {
		memcpy(state->x.next, strm->next_in, strm->avail_in);
		state->x.have = strm->avail_in;
		strm->avail_in = 0;
	}
	state->how = COPY;
	state->direct = 1;
	return 0;
}

/* Decompress from input to the provided next_out and avail_out in the state.
   On return, state->x.have and state->x.next point to the just decompressed
   data.  If the gzip stream completes, state->how is reset to LOOK to look for
   the next gzip stream or raw data, once state->x.have is depleted.  Returns 0
   on success, -1 on failure. */
static int gz_decomp(gz_statep state)
{
	int ret = Z_OK;
	z_streamp strm = &(state->strm);
	/* fill output buffer up to end of deflate stream */
	unsigned had = strm->avail_out;
	do {
		/* get more input for inflate() */
		if(strm->avail_in == 0 && gz_avail(state) == -1)
			return -1;
		if(strm->avail_in == 0) {
			gz_error(state, Z_BUF_ERROR, "unexpected end of file");
			break;
		}
		/* decompress and handle errors */
		ret = inflate(strm, Z_NO_FLUSH);
		if(ret == Z_STREAM_ERROR || ret == Z_NEED_DICT) {
			gz_error(state, Z_STREAM_ERROR, "internal error: inflate stream corrupt");
			return -1;
		}
		if(ret == Z_MEM_ERROR) {
			gz_error(state, Z_MEM_ERROR, "out of memory");
			return -1;
		}
		if(ret == Z_DATA_ERROR) {       /* deflate stream invalid */
			gz_error(state, Z_DATA_ERROR, strm->msg == NULL ? "compressed data error" : strm->msg);
			return -1;
		}
	} while(strm->avail_out && ret != Z_STREAM_END);
	/* update available output */
	state->x.have = had - strm->avail_out;
	state->x.next = strm->next_out - state->x.have;
	/* if the gzip stream completed successfully, look for another */
	if(ret == Z_STREAM_END)
		state->how = LOOK;
	return 0; // good decompression 
}

/* Fetch data and put it in the output buffer.  Assumes state->x.have is 0.
   Data is either copied from the input file or decompressed from the input
   file depending on state->how.  If state->how is LOOK, then a gzip header is
   looked for to determine whether to copy or decompress.  Returns -1 on error,
   otherwise 0.  gz_fetch() will leave state->how as COPY or GZIP unless the
   end of the input file has been reached and all data has been processed.  */
static int gz_fetch(gz_statep state)
{
	z_streamp strm = &(state->strm);
	do {
		switch(state->how) {
			case LOOK: /* -> LOOK, COPY (only if never GZIP), or GZIP */
			    if(gz_look(state) == -1)
				    return -1;
			    if(state->how == LOOK)
				    return 0;
			    break;
			case COPY: /* -> COPY */
			    if(gz_load(state, state->out, state->size << 1, &(state->x.have))
			    == -1)
				    return -1;
			    state->x.next = state->out;
			    return 0;
			case GZIP: /* -> GZIP or LOOK (if end of gzip stream) */
			    strm->avail_out = state->size << 1;
			    strm->next_out = state->out;
			    if(gz_decomp(state) == -1)
				    return -1;
		}
	} while(state->x.have == 0 && (!state->eof || strm->avail_in));
	return 0;
}

/* Skip len uncompressed bytes of output.  Return -1 on error, 0 on success. */
static int gz_skip(gz_statep state, z_off64_t len)
{
	unsigned n;
	/* skip over len bytes or reach end-of-file, whichever comes first */
	while(len)
		/* skip over whatever is in output buffer */
		if(state->x.have) {
			n = GT_OFF(state->x.have) || (z_off64_t)state->x.have > len ? (unsigned)len : state->x.have;
			state->x.have -= n;
			state->x.next += n;
			state->x.pos += n;
			len -= n;
		}
		/* output buffer empty -- return if we're at the end of the input */
		else if(state->eof && state->strm.avail_in == 0)
			break;
		/* need more data to skip -- load up output buffer */
		else {
			/* get more output, looking for header if required */
			if(gz_fetch(state) == -1)
				return -1;
		}
	return 0;
}

/* Read len bytes into buf from file, or less than len up to the end of the
   input.  Return the number of bytes read.  If zero is returned, either the
   end of file was reached, or there was an error.  state->err must be
   consulted in that case to determine which. */
static z_size_t gz_read(gz_statep state, voidp buf, z_size_t len)
{
	z_size_t got;
	unsigned n;
	/* if len is zero, avoid unnecessary operations */
	if(len == 0)
		return 0;
	/* process a skip request */
	if(state->seek) {
		state->seek = 0;
		if(gz_skip(state, state->skip) == -1)
			return 0;
	}
	/* get len bytes to buf, or less than len if at the end */
	got = 0;
	do {
		// set n to the maximum amount of len that fits in an unsigned int 
		n = -1;
		if(n > len)
			n = len;
		// first just try copying data from the output buffer 
		if(state->x.have) {
			if(state->x.have < n)
				n = state->x.have;
			memcpy(buf, state->x.next, n);
			state->x.next += n;
			state->x.have -= n;
		}
		// output buffer empty -- return if we're at the end of the input 
		else if(state->eof && state->strm.avail_in == 0) {
			state->past = 1; /* tried to read past end */
			break;
		}
		// need output data -- for small len or new stream load up our output buffer 
		else if(state->how == LOOK || n < (state->size << 1)) {
			// get more output, looking for header if required 
			if(gz_fetch(state) == -1)
				return 0;
			continue; /* no progress yet -- go back to copy above */
			/* the copy above assures that we will leave with space in the
			   output buffer, allowing at least one gzungetc() to succeed */
		}
		/* large len -- read directly into user buffer */
		else if(state->how == COPY) { /* read directly */
			if(gz_load(state, (unsigned char*)buf, n, &n) == -1)
				return 0;
		}
		/* large len -- decompress directly into user buffer */
		else { /* state->how == GZIP */
			state->strm.avail_out = n;
			state->strm.next_out = (unsigned char*)buf;
			if(gz_decomp(state) == -1)
				return 0;
			n = state->x.have;
			state->x.have = 0;
		}
		/* update progress */
		len -= n;
		buf = (char*)buf + n;
		got += n;
		state->x.pos += n;
	} while(len);
	/* return number of bytes read into user buffer */
	return got;
}

int ZEXPORT gzread(gzFile file, voidp buf, unsigned len)
{
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	/* check that we're reading and that there's no (serious) error */
	if(state->mode != GZ_READ || (state->err != Z_OK && state->err != Z_BUF_ERROR))
		return -1;

	/* since an int is returned, make sure len fits in one, otherwise return
	   with an error (this avoids a flaw in the interface) */
	if((int)len < 0) {
		gz_error(state, Z_STREAM_ERROR, "request does not fit in an int");
		return -1;
	}
	/* read len or fewer bytes to buf */
	len = gz_read(state, buf, len);
	/* check for an error */
	if(len == 0 && state->err != Z_OK && state->err != Z_BUF_ERROR)
		return -1;
	/* return the number of bytes read (this is assured to fit in an int) */
	return (int)len;
}

z_size_t ZEXPORT gzfread(voidp buf, z_size_t size, z_size_t nitems, gzFile file)
{
	z_size_t len;
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return 0;
	state = (gz_statep)file;
	/* check that we're reading and that there's no (serious) error */
	if(state->mode != GZ_READ || (state->err != Z_OK && state->err != Z_BUF_ERROR))
		return 0;
	/* compute bytes to read -- error on overflow */
	len = nitems * size;
	if(size && len / size != nitems) {
		gz_error(state, Z_STREAM_ERROR, "request does not fit in a size_t");
		return 0;
	}
	/* read len or fewer bytes to buf, return the number of full items read */
	return len ? gz_read(state, buf, len) / size : 0;
}

#ifdef Z_PREFIX_SET
	#undef z_gzgetc
#else
	#undef gzgetc
#endif

int ZEXPORT gzgetc(gzFile file)
{
	int ret;
	unsigned char buf[1];
	// get internal structure 
	if(file == NULL)
		return -1;
	else {
		gz_statep state = (gz_statep)file;
		// check that we're reading and that there's no (serious) error 
		if(state->mode != GZ_READ || (state->err != Z_OK && state->err != Z_BUF_ERROR))
			return -1;
		else {
			// try output buffer (no need to check for skip request) 
			if(state->x.have) {
				state->x.have--;
				state->x.pos++;
				return *(state->x.next)++;
			}
			else {
				// nothing there -- try gz_read() 
				ret = gz_read(state, buf, 1);
				return (ret < 1) ? -1 : buf[0];
			}
		}
	}
}

int ZEXPORT gzgetc_(gzFile file)
{
	return gzgetc(file);
}

int ZEXPORT gzungetc(int c, gzFile file)
{
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	/* check that we're reading and that there's no (serious) error */
	if(state->mode != GZ_READ || (state->err != Z_OK && state->err != Z_BUF_ERROR))
		return -1;
	/* process a skip request */
	if(state->seek) {
		state->seek = 0;
		if(gz_skip(state, state->skip) == -1)
			return -1;
	}
	/* can't push EOF */
	if(c < 0)
		return -1;
	/* if output buffer empty, put byte at end (allows more pushing) */
	if(state->x.have == 0) {
		state->x.have = 1;
		state->x.next = state->out + (state->size << 1) - 1;
		state->x.next[0] = (unsigned char)c;
		state->x.pos--;
		state->past = 0;
		return c;
	}
	/* if no room, give up (must have already done a gzungetc()) */
	if(state->x.have == (state->size << 1)) {
		gz_error(state, Z_DATA_ERROR, "out of room to push characters");
		return -1;
	}
	// slide output data if needed and insert byte before existing data 
	if(state->x.next == state->out) {
		unsigned char * src = state->out + state->x.have;
		unsigned char * dest = state->out + (state->size << 1);
		while(src > state->out)
			*--dest = *--src;
		state->x.next = dest;
	}
	state->x.have++;
	state->x.next--;
	state->x.next[0] = (unsigned char)c;
	state->x.pos--;
	state->past = 0;
	return c;
}

/* -- see zlib.h -- */
char * ZEXPORT gzgets(gzFile file, char * buf, int len)
{
	char * str = 0;
	unsigned left, n;
	unsigned char * eol;
	gz_statep state;
	// check parameters and get internal structure 
	if(file == NULL || buf == NULL || len < 1)
		return NULL;
	state = (gz_statep)file;
	// check that we're reading and that there's no (serious) error 
	if(state->mode != GZ_READ || (state->err != Z_OK && state->err != Z_BUF_ERROR))
		return NULL;
	// process a skip request 
	if(state->seek) {
		state->seek = 0;
		if(gz_skip(state, state->skip) == -1)
			return NULL;
	}
	// copy output bytes up to new line or len - 1, whichever comes first --
	// append a terminating zero to the string (we don't check for a zero in
	// the contents, let the user worry about that) 
	str = buf;
	left = (unsigned)len - 1;
	if(left) do {
		// assure that something is in the output buffer 
		if(state->x.have == 0 && gz_fetch(state) == -1)
			return NULL;  /* error */
		if(state->x.have == 0) { /* end of file */
			state->past = 1; /* read past end */
			break;  /* return what we have */
		}
		/* look for end-of-line in current output buffer */
		n = state->x.have > left ? left : state->x.have;
		eol = (unsigned char*)memchr(state->x.next, '\n', n);
		if(eol != NULL)
			n = (unsigned)(eol - state->x.next) + 1;
		/* copy through end-of-line, or remainder if not found */
		memcpy(buf, state->x.next, n);
		state->x.have -= n;
		state->x.next += n;
		state->x.pos += n;
		left -= n;
		buf += n;
	} while(left && eol == NULL);
	// return terminated string, or if nothing, end of file 
	if(buf == str)
		return NULL;
	else {
		buf[0] = 0;
		return str;
	}
}

int ZEXPORT gzdirect(gzFile file)
{
	gz_statep state;
	// get internal structure 
	if(file == NULL)
		return 0;
	state = (gz_statep)file;
	// if the state is not known, but we can find out, then do so (this is mainly for right after a gzopen() or gzdopen()) 
	if(state->mode == GZ_READ && state->how == LOOK && state->x.have == 0)
		(void)gz_look(state);
	return state->direct; // return 1 if transparent, 0 if processing a gzip stream 
}

/* -- see zlib.h -- */
int ZEXPORT gzclose_r(gzFile file)
{
	int ret, err;
	gz_statep state;
	/* get internal structure */
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	/* check that we're reading */
	if(state->mode != GZ_READ)
		return Z_STREAM_ERROR;
	/* free memory and close file */
	if(state->size) {
		inflateEnd(&(state->strm));
		SAlloc::F(state->out);
		SAlloc::F(state->in);
	}
	err = state->err == Z_BUF_ERROR ? Z_BUF_ERROR : Z_OK;
	gz_error(state, Z_OK, NULL);
	SAlloc::F(state->path);
	ret = close(state->fd);
	SAlloc::F(state);
	return ret ? Z_ERRNO : err;
}
//
// GZCLOSE
//
int ZEXPORT gzclose(gzFile file)
{
#ifndef NO_GZCOMPRESS
	gz_statep state;
	if(file == NULL)
		return Z_STREAM_ERROR;
	state = (gz_statep)file;
	return state->mode == GZ_READ ? gzclose_r(file) : gzclose_w(file);
#else
	return gzclose_r(file);
#endif
}
//
// GZLIB
//
#if defined(_WIN32) && !defined(__BORLANDC__) && !defined(__MINGW32__)
	#define LSEEK _lseeki64
#else
#if defined(_LARGEFILE64_SOURCE) && _LFS64_LARGEFILE-0
	#define LSEEK lseek64
#else
	#define LSEEK lseek
#endif
#endif

#if defined UNDER_CE

/* Map the Windows error number in ERROR to a locale-dependent error message
   string and return a pointer to it.  Typically, the values for ERROR come
   from GetLastError.

   The string pointed to shall not be modified by the application, but may be
   overwritten by a subsequent call to gz_strwinerror

   The gz_strwinerror function does not change the current setting of
   GetLastError. */
char ZLIB_INTERNAL * gz_strwinerror(DWORD error)
{
	static char buf[1024];
	wchar_t * msgbuf;
	DWORD lasterr = GetLastError();
	DWORD chars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error, 0/* Default language */, (LPVOID)&msgbuf, 0, NULL);
	if(chars != 0) {
		/* If there is an \r\n appended, zap it.  */
		if(chars >= 2 && msgbuf[chars - 2] == '\r' && msgbuf[chars - 1] == '\n') {
			chars -= 2;
			msgbuf[chars] = 0;
		}
		if(chars > sizeof(buf) - 1) {
			chars = sizeof(buf) - 1;
			msgbuf[chars] = 0;
		}
		wcstombs(buf, msgbuf, chars + 1);
		LocalFree(msgbuf);
	}
	else {
		sprintf(buf, "unknown win32 error (%ld)", error);
	}
	SetLastError(lasterr);
	return buf;
}

#endif /* UNDER_CE */

/* Reset gzip file state */
static void gz_reset(gz_statep state)
{
	state->x.have = 0;          /* no output data available */
	if(state->mode == GZ_READ) { /* for reading ... */
		state->eof = 0;     /* not at end of file */
		state->past = 0;    /* have not read past end yet */
		state->how = LOOK;  /* look for gzip header */
	}
	state->seek = 0;            /* no seek request pending */
	gz_error(state, Z_OK, NULL); /* clear error */
	state->x.pos = 0;           /* no uncompressed data yet */
	state->strm.avail_in = 0;   /* no input data yet */
}

/* Open a gzip file either by name or file descriptor. */
static gzFile gz_open(const void * path, int fd, const char * mode)
{
	gz_statep state;
	z_size_t len;
	int oflag;
#ifdef O_CLOEXEC
	int cloexec = 0;
#endif
#ifdef O_EXCL
	int exclusive = 0;
#endif

	/* check input */
	if(path == NULL)
		return NULL;

	/* allocate gzFile structure to return */
	state = (gz_statep)SAlloc::M(sizeof(gz_state));
	if(state == NULL)
		return NULL;
	state->size = 0;        /* no buffers allocated yet */
	state->want = GZBUFSIZE; /* requested buffer size */
	state->msg = NULL;      /* no error message yet */

	/* interpret mode */
	state->mode = GZ_NONE;
	state->level = Z_DEFAULT_COMPRESSION;
	state->strategy = Z_DEFAULT_STRATEGY;
	state->direct = 0;
	while(*mode) {
		if(*mode >= '0' && *mode <= '9')
			state->level = *mode - '0';
		else
			switch(*mode) {
				case 'r': state->mode = GZ_READ; break;
#ifndef NO_GZCOMPRESS
				case 'w': state->mode = GZ_WRITE; break;
				case 'a': state->mode = GZ_APPEND; break;
#endif
				case '+': /* can't read and write at the same time */
				    SAlloc::F(state);
				    return NULL;
				case 'b': /* ignore -- will request binary anyway */
				    break;
#ifdef O_CLOEXEC
				case 'e':
				    cloexec = 1;
				    break;
#endif
#ifdef O_EXCL
				case 'x': exclusive = 1; break;
#endif
				case 'f': state->strategy = Z_FILTERED; break;
				case 'h': state->strategy = Z_HUFFMAN_ONLY; break;
				case 'R': state->strategy = Z_RLE; break;
				case 'F': state->strategy = Z_FIXED; break;
				case 'T': state->direct = 1; break;
				default: /* could consider as an error, but just ignore */
				    ;
			}
		mode++;
	}

	/* must provide an "r", "w", or "a" */
	if(state->mode == GZ_NONE) {
		SAlloc::F(state);
		return NULL;
	}
	/* can't force transparent read */
	if(state->mode == GZ_READ) {
		if(state->direct) {
			SAlloc::F(state);
			return NULL;
		}
		state->direct = 1; /* for empty file */
	}
	/* save the path name for error messages */
#ifdef WIDECHAR
	if(fd == -2) {
		len = wcstombs(NULL, path, 0);
		if(len == (z_size_t)-1)
			len = 0;
	}
	else
#endif
	len = strlen((const char*)path);
	state->path = (char*)SAlloc::M(len + 1);
	if(state->path == NULL) {
		SAlloc::F(state);
		return NULL;
	}
#ifdef WIDECHAR
	if(fd == -2)
		if(len)
			wcstombs(state->path, path, len + 1);
		else
			*(state->path) = 0;
	else
#endif
#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
	(void)snprintf(state->path, len + 1, "%s", (const char*)path);
#else
	strcpy(state->path, path);
#endif

	/* compute the flags for open() */
	oflag =
#ifdef O_LARGEFILE
	    O_LARGEFILE |
#endif
#ifdef O_BINARY
	    O_BINARY |
#endif
#ifdef O_CLOEXEC
	    (cloexec ? O_CLOEXEC : 0) |
#endif
	    (state->mode == GZ_READ ? O_RDONLY : (O_WRONLY | O_CREAT |
#ifdef O_EXCL
		    (exclusive ? O_EXCL : 0) |
#endif
		    (state->mode == GZ_WRITE ? O_TRUNC : O_APPEND)));

	/* open the file with the appropriate flags (or just use fd) */
	state->fd = fd > -1 ? fd : (
#ifdef WIDECHAR
	    fd == -2 ? _wopen(path, oflag, 0666) :
#endif
	    open((const char*)path, oflag, 0666));
	if(state->fd == -1) {
		SAlloc::F(state->path);
		SAlloc::F(state);
		return NULL;
	}
	if(state->mode == GZ_APPEND) {
		LSEEK(state->fd, 0, SEEK_END); /* so gzoffset() is correct */
		state->mode = GZ_WRITE; /* simplify later checks */
	}
	/* save the current position for rewinding (only if reading) */
	if(state->mode == GZ_READ) {
		state->start = LSEEK(state->fd, 0, SEEK_CUR);
		if(state->start == -1) state->start = 0;
	}
	gz_reset(state); /* initialize stream */
	return (gzFile)state; /* return stream */
}

gzFile ZEXPORT gzopen(const char * path, const char * mode)
{
	return gz_open(path, -1, mode);
}

gzFile ZEXPORT gzopen64(const char * path, const char * mode)
{
	return gz_open(path, -1, mode);
}

gzFile ZEXPORT gzdopen(int fd, const char * mode)
{
	char * path;    /* identifier for error messages */
	gzFile gz;
	if(fd == -1 || (path = (char*)SAlloc::M(7 + 3 * sizeof(int))) == NULL)
		return NULL;
#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
	(void)snprintf(path, 7 + 3 * sizeof(int), "<fd:%d>", fd);
#else
	sprintf(path, "<fd:%d>", fd); /* for debugging */
#endif
	gz = gz_open(path, fd, mode);
	SAlloc::F(path);
	return gz;
}

#ifdef WIDECHAR
gzFile ZEXPORT gzopen_w(const wchar_t * path, const char * mode)
{
	return gz_open(path, -2, mode);
}
#endif

int ZEXPORT gzbuffer(gzFile file, unsigned size)
{
	gz_statep state;
	/* get internal structure and check integrity */
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	if(state->mode != GZ_READ && state->mode != GZ_WRITE)
		return -1;
	/* make sure we haven't already allocated memory */
	if(state->size != 0)
		return -1;
	/* check and set requested size */
	if((size << 1) < size)
		return -1;      /* need to be able to double it */
	if(size < 2)
		size = 2;       /* need two bytes to check magic header */
	state->want = size;
	return 0;
}

int ZEXPORT gzrewind(gzFile file)
{
	gz_statep state;
	// get internal structure 
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	// check that we're reading and that there's no error 
	if(state->mode != GZ_READ || (state->err != Z_OK && state->err != Z_BUF_ERROR))
		return -1;
	// back up and start over 
	if(LSEEK(state->fd, state->start, SEEK_SET) == -1)
		return -1;
	gz_reset(state);
	return 0;
}

z_off64_t ZEXPORT gzseek64(gzFile file, z_off64_t offset, int whence)
{
	unsigned n;
	z_off64_t ret;
	gz_statep state;
	// get internal structure and check integrity 
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	if(state->mode != GZ_READ && state->mode != GZ_WRITE)
		return -1;
	if(state->err != Z_OK && state->err != Z_BUF_ERROR) // check that there's no error 
		return -1;
	if(whence != SEEK_SET && whence != SEEK_CUR) // can only seek from start or relative to current position 
		return -1;
	if(whence == SEEK_SET) // normalize offset to a SEEK_CUR specification 
		offset -= state->x.pos;
	else if(state->seek)
		offset += state->skip;
	state->seek = 0;
	// if within raw area while reading, just go there 
	if(state->mode == GZ_READ && state->how == COPY && state->x.pos + offset >= 0) {
		ret = LSEEK(state->fd, offset - state->x.have, SEEK_CUR);
		if(ret == -1)
			return -1;
		state->x.have = 0;
		state->eof = 0;
		state->past = 0;
		state->seek = 0;
		gz_error(state, Z_OK, NULL);
		state->strm.avail_in = 0;
		state->x.pos += offset;
		return state->x.pos;
	}
	// calculate skip amount, rewinding if needed for back seek when reading 
	if(offset < 0) {
		if(state->mode != GZ_READ)  /* writing -- can't go backwards */
			return -1;
		offset += state->x.pos;
		if(offset < 0)              /* before start of file! */
			return -1;
		if(gzrewind(file) == -1)    /* rewind, then skip to offset */
			return -1;
	}
	// if reading, skip what's in output buffer (one less gzgetc() check) 
	if(state->mode == GZ_READ) {
		n = GT_OFF(state->x.have) || (z_off64_t)state->x.have > offset ? (unsigned)offset : state->x.have;
		state->x.have -= n;
		state->x.next += n;
		state->x.pos += n;
		offset -= n;
	}
	// request skip (if not zero) 
	if(offset) {
		state->seek = 1;
		state->skip = offset;
	}
	return state->x.pos + offset;
}

z_off_t ZEXPORT gzseek(gzFile file, z_off_t offset, int whence)
{
	z_off64_t ret = gzseek64(file, (z_off64_t)offset, whence);
	return ret == (z_off_t)ret ? (z_off_t)ret : -1;
}

z_off64_t ZEXPORT gztell64(gzFile file)
{
	gz_statep state;
	// get internal structure and check integrity 
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	if(state->mode != GZ_READ && state->mode != GZ_WRITE)
		return -1;
	// return position 
	return state->x.pos + (state->seek ? state->skip : 0);
}

z_off_t ZEXPORT gztell(gzFile file)
{
	z_off64_t ret = gztell64(file);
	return (ret == (z_off_t)ret) ? (z_off_t)ret : -1;
}

z_off64_t ZEXPORT gzoffset64(gzFile file)
{
	z_off64_t offset;
	gz_statep state;
	// get internal structure and check integrity 
	if(file == NULL)
		return -1;
	state = (gz_statep)file;
	if(state->mode != GZ_READ && state->mode != GZ_WRITE)
		return -1;
	// compute and return effective offset in file 
	offset = LSEEK(state->fd, 0, SEEK_CUR);
	if(offset == -1)
		return -1;
	if(state->mode == GZ_READ)          /* reading */
		offset -= state->strm.avail_in;  /* don't count buffered input */
	return offset;
}

z_off_t ZEXPORT gzoffset(gzFile file)
{
	z_off64_t ret = gzoffset64(file);
	return ret == (z_off_t)ret ? (z_off_t)ret : -1;
}

int ZEXPORT gzeof(gzFile file)
{
	gz_statep state;
	// get internal structure and check integrity 
	if(file == NULL)
		return 0;
	state = (gz_statep)file;
	if(state->mode != GZ_READ && state->mode != GZ_WRITE)
		return 0;
	// return end-of-file state 
	return (state->mode == GZ_READ) ? state->past : 0;
}

const char * ZEXPORT gzerror(gzFile file, int * errnum)
{
	gz_statep state;
	// get internal structure and check integrity 
	if(file == NULL)
		return NULL;
	state = (gz_statep)file;
	if(state->mode != GZ_READ && state->mode != GZ_WRITE)
		return NULL;
	// return error information 
	ASSIGN_PTR(errnum, state->err);
	return (state->err == Z_MEM_ERROR) ? "out of memory" : (state->msg == NULL ? "" : state->msg);
}

void ZEXPORT gzclearerr(gzFile file)
{
	// get internal structure and check integrity 
	if(file) {
		gz_statep state = (gz_statep)file;
		if(state->mode != GZ_READ && state->mode != GZ_WRITE)
			return;
		// clear error and end-of-file 
		if(state->mode == GZ_READ) {
			state->eof = 0;
			state->past = 0;
		}
		gz_error(state, Z_OK, NULL);
	}
}

/* Create an error message in allocated memory and set state->err and
   state->msg accordingly.  Free any previous error message already there.  Do
   not try to free or allocate space if the error is Z_MEM_ERROR (out of
   memory).  Simply save the error message as a static string.  If there is an
   allocation failure constructing the error message, then convert the error to
   out of memory. */
void ZLIB_INTERNAL gz_error(gz_statep state, int err, const char * msg)
{
	// free previously allocated message and clear 
	if(state->msg != NULL) {
		if(state->err != Z_MEM_ERROR)
			SAlloc::F(state->msg);
		state->msg = NULL;
	}
	// if fatal, set state->x.have to 0 so that the gzgetc() macro fails 
	if(err != Z_OK && err != Z_BUF_ERROR)
		state->x.have = 0;
	// set error code, and if no message, then done 
	state->err = err;
	if(msg == NULL)
		return;
	// for an out of memory error, return literal string when requested 
	if(err == Z_MEM_ERROR)
		return;
	// construct error message with path 
	if((state->msg = (char*)SAlloc::M(strlen(state->path) + strlen(msg) + 3)) == NULL) {
		state->err = Z_MEM_ERROR;
		return;
	}
#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
	(void)snprintf(state->msg, strlen(state->path) + strlen(msg) + 3, "%s%s%s", state->path, ": ", msg);
#else
	strcpy(state->msg, state->path);
	strcat(state->msg, ": ");
	strcat(state->msg, msg);
#endif
}

#ifndef INT_MAX
//
// portably return maximum value for an int (when limits.h presumed not
// available) -- we need to do this to cover cases where 2's complement not
// used, since C standard permits 1's complement and sign-bit representations,
// otherwise we could just use ((unsigned)-1) >> 1 
//
unsigned ZLIB_INTERNAL gz_intmax()
{
	unsigned q;
	unsigned p = 1;
	do {
		q = p;
		p <<= 1;
		p++;
	} while(p > q);
	return q >> 1;
}

#endif
