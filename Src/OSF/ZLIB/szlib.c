// SZLIB.C
// Copyright (C) 2004-2017 Mark Adler
//
// Note: This is the merging of several small module of original zlib for compacting.
//
#define ZLIB_INTERNAL
#include "zlib.h"
#pragma hdrstop

#include "gzguts.h"
#include "zutil.h"
#include "inftrees.h"
//
// ADLER
//
static uLong adler32_combine_(uLong adler1, uLong adler2, z_off64_t len2);

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
/* note that this assumes BASE is 65521, where 65536 % 65521 == 15
   (thank you to John Reiser for pointing this out) */
#  define CHOP(a) \
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
	#include <stdio.h>
	#ifndef DYNAMIC_CRC_TABLE
		#define DYNAMIC_CRC_TABLE
	#endif /* !DYNAMIC_CRC_TABLE */
#endif

/* Definitions for doing the crc four data bytes at a time. */
#if !defined(NOBYFOUR) && defined(Z_U4)
	#define BYFOUR
#endif
#ifdef BYFOUR
	static unsigned long crc32_little OF((unsigned long, const unsigned char *, z_size_t));
	static unsigned long crc32_big OF((unsigned long, const unsigned char *, z_size_t));
	#define TBLS 8
#else
	#define TBLS 1
#endif /* BYFOUR */

/* Local functions for crc concatenation */
static unsigned long gf2_matrix_times OF((unsigned long * mat, unsigned long vec));
static void gf2_matrix_square OF((unsigned long * square, unsigned long * mat));
static uLong crc32_combine_ OF((uLong crc1, uLong crc2, z_off64_t len2));

#ifdef DYNAMIC_CRC_TABLE

static volatile int crc_table_empty = 1;
static z_crc_t crc_table[TBLS][256];
static void make_crc_table OF((void));
#ifdef MAKECRCH
	static void write_table OF((FILE *, const z_crc_t *));
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
/* ========================================================================
 * Tables of CRC-32s of all single-byte values, made by make_crc_table().
 */
#include "crc32.h"
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

static unsigned long gf2_matrix_times(unsigned long * mat, unsigned long vec)
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

static void gf2_matrix_square(unsigned long * square, unsigned long * mat)
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

#ifdef __TURBOC__
/* Turbo C in 16-bit mode */

#  define MY_ZCALLOC

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use zcfree().
 */

#define MAX_PTR 10
/* 10*64K = 640K */

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
	if(buf == NULL || next_ptr >= MAX_PTR) return NULL;
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
	/* Find the original pointer */
	for(n = 0; n < next_ptr; n++) {
		if(ptr != table[n].new_ptr) continue;

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
/* Microsoft C in 16-bit mode */
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
	extern voidp malloc OF((uInt size));
	extern voidp calloc OF((uInt items, uInt size));
	extern void free   OF((voidpf ptr));
#endif

voidpf ZLIB_INTERNAL zcalloc(voidpf opaque, unsigned items, unsigned size)
{
	(void)opaque;
	return sizeof(uInt) > 2 ? (voidpf)malloc(items * size) : (voidpf)calloc(items, size);
}

void ZLIB_INTERNAL zcfree(voidpf opaque, voidpf ptr)
{
	(void)opaque;
	free(ptr);
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
int ZLIB_INTERNAL inflate_table(codetype type, unsigned short  * lens, unsigned codes, code  ** table, unsigned  * bits, unsigned short  * work)
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
	code here;              /* table entry for duplication */
	code  * next;        /* next available space in table */
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

