// SUCL.C
// Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer. All Rights Reserved.
//
#include "ucl_conf.h"
//
// ALLOC.C
//
//
// implementation
//
#if defined(__UCL_MMODEL_HUGE)
	#define acc_hsize_t             ucl_uint
	#define acc_hvoid_p             ucl_voidp
	#define ACCLIB_PUBLIC(r, f)      static r __UCL_CDECL f
	#define acc_halloc              ucl_malloc_internal
	#define acc_hfree               ucl_free_internal
	#include "acc/acclib/halloc.ch"
	#undef ACCLIB_PUBLIC
#else
	UCL_PRIVATE(ucl_voidp) ucl_malloc_internal(ucl_uint size)
	{
		ucl_voidp p = NULL;
		if(size < ~(size_t)0)
			p = (ucl_voidp)malloc((size_t)size);
		return p;
	}

	UCL_PRIVATE(void) ucl_free_internal(ucl_voidp p)
	{
		if(p)
			free(p);
	}
#endif
//
// public interface using the global hooks
//
/* global allocator hooks */
static ucl_malloc_hook_t ucl_malloc_hook = ucl_malloc_internal;
static ucl_free_hook_t ucl_free_hook = ucl_free_internal;

UCL_PUBLIC(void) ucl_set_malloc_hooks(ucl_malloc_hook_t a, ucl_free_hook_t f)
{
	ucl_malloc_hook = ucl_malloc_internal;
	ucl_free_hook = ucl_free_internal;
	if(a)
		ucl_malloc_hook = a;
	if(f)
		ucl_free_hook = f;
}

UCL_PUBLIC(void) ucl_get_malloc_hooks(ucl_malloc_hook_t* a, ucl_free_hook_t* f)
{
	if(a)
		*a = ucl_malloc_hook;
	if(f)
		*f = ucl_free_hook;
}

UCL_PUBLIC(ucl_voidp) ucl_malloc(ucl_uint size)
{
	return (size <= 0) ? NULL : ucl_malloc_hook(size);
}

UCL_PUBLIC(ucl_voidp) ucl_alloc(ucl_uint nelems, ucl_uint size)
{
	ucl_uint s = nelems * size;
	return (nelems <= 0 || s / nelems != size) ? NULL : ucl_malloc(s);
}

UCL_PUBLIC(void) ucl_free(ucl_voidp p)
{
	if(p)
		ucl_free_hook(p);
}
//
// UCL_PTR.C
//
UCL_PUBLIC(ucl_uintptr_t) __ucl_ptr_linear(const ucl_voidp ptr)
{
    ucl_uintptr_t p;
#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
    p = (((ucl_uintptr_t)(ACC_FP_SEG(ptr))) << (16 - ACC_MM_AHSHIFT)) + (ACC_FP_OFF(ptr));
#else
    p = PTR_LINEAR(ptr);
#endif
    return p;
}

UCL_PUBLIC(unsigned) __ucl_align_gap(const ucl_voidp ptr, ucl_uint size)
{
    ucl_uintptr_t p, s, n;
    assert(size > 0);
    p = __ucl_ptr_linear(ptr);
    s = (ucl_uintptr_t) (size - 1);
#if 0
    assert((size & (size - 1)) == 0);
    n = ((p + s) & ~s) - p;
#else
    n = (((p + s) / size) * size) - p;
#endif
    assert((long)n >= 0);
    assert(n <= s);
    return (unsigned)n;
}
//
// UCL_UTIL.C
//
UCL_PUBLIC(ucl_bool) ucl_assert(int expr) { return (expr) ? 1 : 0; }
// 
// If you use the UCL library in a product, you *must* keep this
// copyright string in the executable of your product.
// 
static const char __ucl_copyright[] =
        "\r\n\n"
        "UCL data compression library.\n"
        "$Copyright: UCL (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004 Markus Franz Xaver Johannes Oberhumer\n"
        "<markus@oberhumer.com>\n"
        "http://www.oberhumer.com $\n\n"
        "$Id: UCL version: v" UCL_VERSION_STRING ", " UCL_VERSION_DATE " $\n"
        "$Built: " __DATE__ " " __TIME__ " $\n"
        "$Info: " ACC_INFO_OS
#if defined(ACC_INFO_OS_POSIX)
        "/" ACC_INFO_OS_POSIX
#endif
        " " ACC_INFO_ARCH
#if defined(ACC_INFO_ENDIAN)
        "/" ACC_INFO_ENDIAN
#endif
        " " ACC_INFO_MM
        " " ACC_INFO_CC
#if defined(ACC_INFO_CCVER)
        " " ACC_INFO_CCVER
#endif
        " $\n";

UCL_PUBLIC(const ucl_bytep) ucl_copyright(void)
{
#if (ACC_OS_DOS16 && ACC_CC_TURBOC)
	return (ucl_voidp)__ucl_copyright;
#else
	return (const ucl_bytep)__ucl_copyright;
#endif
}

UCL_PUBLIC(ucl_uint32) ucl_version(void) { return UCL_VERSION; }
UCL_PUBLIC(const char *) ucl_version_string(void) { return UCL_VERSION_STRING; }
UCL_PUBLIC(const char *) ucl_version_date(void) { return UCL_VERSION_DATE; }
UCL_PUBLIC(const ucl_charp) _ucl_version_string(void) { return UCL_VERSION_STRING; }
UCL_PUBLIC(const ucl_charp) _ucl_version_date(void) { return UCL_VERSION_DATE; }
//
// adler32 checksum
// adapted from free code by Mark Adler <madler@alumni.caltech.edu>
// see http://www.cdrom.com/pub/infozip/zlib/
// 
#define UCL_BASE 65521u /* largest prime smaller than 65536 */
#define UCL_NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define UCL_DO1(buf, i)  {s1 += buf[i]; s2 += s1;}
#define UCL_DO2(buf, i)  UCL_DO1(buf, i); UCL_DO1(buf, i+1);
#define UCL_DO4(buf, i)  UCL_DO2(buf, i); UCL_DO2(buf, i+2);
#define UCL_DO8(buf, i)  UCL_DO4(buf, i); UCL_DO4(buf, i+4);
#define UCL_DO16(buf, i) UCL_DO8(buf, i); UCL_DO8(buf, i+8);

UCL_PUBLIC(ucl_uint32) ucl_adler32(ucl_uint32 adler, const ucl_bytep buf, ucl_uint len)
{
	ucl_uint32 s1 = adler&0xffff;
	ucl_uint32 s2 = (adler>>16)&0xffff;
	int k;
	if(!buf)
		return 1;
	while(len > 0) {
		k = len < UCL_NMAX ? (int)len : UCL_NMAX;
		len -= k;
		if(k >= 16) 
			do {
				UCL_DO16(buf, 0);
				buf += 16;
				k -= 16;
			} while(k >= 16);
		if(k != 0) 
			do {
				s1 += *buf++;
				s2 += s1;
			} while(--k > 0);
		s1 %= UCL_BASE;
		s2 %= UCL_BASE;
	}
	return (s2<<16)|s1;
}
//
// UCL_INIT.C
//
//
// Runtime check of the assumptions about the size of builtin types,
// memory model, byte order and other low-level constructs.
//
// We are really paranoid here - UCL should either fail
// at startup or not at all.
//
// Because of inlining much of this evaluates to nothing at compile time.
//
// And while many of the tests seem highly obvious and redundant they are
// here to catch compiler/optimizer bugs. Yes, these do exist.
//
static ucl_bool schedule_insns_bug(void);   /* avoid inlining */
static ucl_bool strength_reduce_bug(int *); /* avoid inlining */

#if defined(UCL_DEBUG)
#include <stdio.h>
static ucl_bool __ucl_assert_fail(const char * s, unsigned line)
{
 #if defined(__palmos__)
	printf("UCL assertion failed in line %u: '%s'\n", line, s);
 #else
	fprintf(stderr, "UCL assertion failed in line %u: '%s'\n", line, s);
 #endif
	return 0;
}

 #define __ucl_assert(x)   ((x) ? 1 : __ucl_assert_fail(# x, __LINE__))
#else
 #define __ucl_assert(x)   ((x) ? 1 : 0)
#endif
// 
// basic_check - compile time assertions
// 
#if 1

 #undef ACCCHK_ASSERT
 #define ACCCHK_ASSERT(expr)     ACC_COMPILE_TIME_ASSERT_HEADER(expr)

 #include "acc/acc_chk.ch"

ACCCHK_ASSERT_IS_SIGNED_T(ucl_int)
ACCCHK_ASSERT_IS_UNSIGNED_T(ucl_uint)

ACCCHK_ASSERT_IS_SIGNED_T(ucl_int32)
ACCCHK_ASSERT_IS_UNSIGNED_T(ucl_uint32)
ACCCHK_ASSERT((UCL_UINT32_C(1)<<(int)(8*sizeof(UCL_UINT32_C(1))-1)) > 0)

ACCCHK_ASSERT_IS_UNSIGNED_T(ucl_uintptr_t)
ACCCHK_ASSERT(sizeof(ucl_uintptr_t) >= sizeof(ucl_voidp))

#endif
#undef ACCCHK_ASSERT
//
//
//
static ucl_bool ptr_check(void)
{
	ucl_bool r = 1;
	int i;
	unsigned char _wrkmem[10*sizeof(ucl_bytep)+sizeof(ucl_align_t)];
	ucl_bytep wrkmem;
	ucl_bytepp dict;
	unsigned char x[4*sizeof(ucl_align_t)];
	long d;
	ucl_align_t a;
	for(i = 0; i < (int)sizeof(x); i++)
		x[i] = UCL_BYTE(i);
	wrkmem = UCL_PTR_ALIGN_UP((ucl_bytep)_wrkmem, sizeof(ucl_align_t));
	dict = (ucl_bytepp)(ucl_voidp)wrkmem;
	d = (long)((const ucl_bytep)dict-(const ucl_bytep)_wrkmem);
	r &= __ucl_assert(d >= 0);
	r &= __ucl_assert(d < (long)sizeof(ucl_align_t));

	/* this may seem obvious, but some compilers incorrectly inline memset */
	memset(&a, 0xff, sizeof(a));
	r &= __ucl_assert(a.a_ushort == USHRT_MAX);
	r &= __ucl_assert(a.a_uint == UINT_MAX);
	r &= __ucl_assert(a.a_ulong == ULONG_MAX);
	r &= __ucl_assert(a.a_ucl_uint == UCL_UINT_MAX);
	/* sanity check of the memory model */
	if(r == 1) {
		for(i = 0; i < 8; i++)
			r &= __ucl_assert((const ucl_voidp)(&dict[i]) == (const ucl_voidp)(&wrkmem[i*sizeof(ucl_bytep)]));
	}
	/* check that NULL == 0 */
	memset(&a, 0, sizeof(a));
	r &= __ucl_assert(a.a_char_p == NULL);
	r &= __ucl_assert(a.a_ucl_bytep == NULL);
	/* check that the pointer constructs work as expected */
	if(r == 1) {
		unsigned k = 1;
		const unsigned n = (unsigned)sizeof(ucl_uint32);
		ucl_bytep p0;
		ucl_bytep p1;

		k += __ucl_align_gap(&x[k], n);
		p0 = (ucl_bytep)&x[k];
#if defined(PTR_LINEAR)
		r &= __ucl_assert((PTR_LINEAR(p0)&(n-1)) == 0);
#else
		r &= __ucl_assert(n == 4);
		r &= __ucl_assert(PTR_ALIGNED_4(p0));
#endif

		r &= __ucl_assert(k >= 1);
		p1 = (ucl_bytep)&x[1];
		r &= __ucl_assert(PTR_GE(p0, p1));

		r &= __ucl_assert(k < 1u+n);
		p1 = (ucl_bytep)&x[1+n];
		r &= __ucl_assert(PTR_LT(p0, p1));
		/* now check that aligned memory access doesn't core dump */
		if(r == 1) {
			ucl_uint32 v0, v1;
			v0 = *(ucl_uint32p)(ucl_voidp)&x[k];
			v1 = *(ucl_uint32p)(ucl_voidp)&x[k+n];
			r &= __ucl_assert(v0 > 0);
			r &= __ucl_assert(v1 > 0);
		}
	}
	return r;
}
// 
// 
// 
UCL_PUBLIC(int) _ucl_config_check(void)
{
	ucl_bool r = 1;
	int i;
	union {
		ucl_uint32 a;
		unsigned short b;
		ucl_uint32 aa[4];
		unsigned char x[4*sizeof(ucl_align_t)];
	} u;
	u.a = 0; u.b = 0;
	for(i = 0; i < (int)sizeof(u.x); i++)
		u.x[i] = UCL_BYTE(i);
#if defined(ACC_ENDIAN_BIG_ENDIAN) || defined(ACC_ENDIAN_LITTLE_ENDIAN)
	if(r == 1) {
 #if defined(ACC_ENDIAN_BIG_ENDIAN)
		ucl_uint32 a = u.a>>(8*sizeof(u.a)-32);
		unsigned short b = u.b>>(8*sizeof(u.b)-16);
		r &= __ucl_assert(a == UCL_UINT32_C(0x00010203));
		r &= __ucl_assert(b == 0x0001);
 #endif
 #if defined(ACC_ENDIAN_LITTLE_ENDIAN)
		ucl_uint32 a = (ucl_uint32)(u.a&UCL_UINT32_C(0xffffffff));
		unsigned short b = (unsigned short)(u.b&0xffff);
		r &= __ucl_assert(a == UCL_UINT32_C(0x03020100));
		r &= __ucl_assert(b == 0x0100);
 #endif
	}
#endif
	/* check that unaligned memory access works as expected */
#if defined(UA_GET2) || defined(UA_SET2)
	if(r == 1) {
		unsigned short b[4];
		for(i = 0; i < 4; i++)
			b[i] = UA_GET2(&u.x[i]);
 #if defined(ACC_ENDIAN_LITTLE_ENDIAN)
		r &= __ucl_assert(b[0] == 0x0100);
		r &= __ucl_assert(b[1] == 0x0201);
		r &= __ucl_assert(b[2] == 0x0302);
		r &= __ucl_assert(b[3] == 0x0403);
 #endif
 #if defined(ACC_ENDIAN_BIG_ENDIAN)
		r &= __ucl_assert(b[0] == 0x0001);
		r &= __ucl_assert(b[1] == 0x0102);
		r &= __ucl_assert(b[2] == 0x0203);
		r &= __ucl_assert(b[3] == 0x0304);
 #endif
	}
#endif

#if defined(UA_GET4) || defined(UA_SET4)
	if(r == 1) {
		ucl_uint32 a[4];
		for(i = 0; i < 4; i++)
			a[i] = UA_GET4(&u.x[i]);
 #if defined(ACC_ENDIAN_LITTLE_ENDIAN)
		r &= __ucl_assert(a[0] == UCL_UINT32_C(0x03020100));
		r &= __ucl_assert(a[1] == UCL_UINT32_C(0x04030201));
		r &= __ucl_assert(a[2] == UCL_UINT32_C(0x05040302));
		r &= __ucl_assert(a[3] == UCL_UINT32_C(0x06050403));
 #endif
 #if defined(ACC_ENDIAN_BIG_ENDIAN)
		r &= __ucl_assert(a[0] == UCL_UINT32_C(0x00010203));
		r &= __ucl_assert(a[1] == UCL_UINT32_C(0x01020304));
		r &= __ucl_assert(a[2] == UCL_UINT32_C(0x02030405));
		r &= __ucl_assert(a[3] == UCL_UINT32_C(0x03040506));
 #endif
	}
#endif
	/* check the ucl_adler32() function */
	if(r == 1) {
		ucl_uint32 adler;
		adler = ucl_adler32(0, NULL, 0);
		adler = ucl_adler32(adler, ucl_copyright(), 195);
		r &= __ucl_assert(adler == UCL_UINT32_C(0x52ca3a75));
	}
	/* check for the gcc schedule-insns optimization bug */
	if(r == 1) {
		r &= __ucl_assert(!schedule_insns_bug());
	}
	/* check for the gcc strength-reduce optimization bug */
	if(r == 1) {
		static int x[3]; // @global
		static unsigned xn = 3; // @global
		register unsigned j;
		for(j = 0; j < xn; j++)
			x[j] = (int)j-3;
		r &= __ucl_assert(!strength_reduce_bug(x));
	}
	/* now for the low-level pointer checks */
	if(r == 1) {
		r &= ptr_check();
	}
	ACC_UNUSED(u);
	return r == 1 ? UCL_E_OK : UCL_E_ERROR;
}

static ucl_bool schedule_insns_bug(void)
{
#if defined(__UCL_CHECKER)
	return 0; // for some reason checker complains about uninitialized memory access 
#else
	const int clone[] = {1, 2, 0};
	const int * q;
	q = clone;
	return (*q) ? 0 : 1;
#endif
}

static ucl_bool strength_reduce_bug(int * x)
{
#if 1 && (ACC_CC_DMC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
	return 0;
#else
	return x[0] != -3 || x[1] != -2 || x[2] != -1;
#endif
}
// 
// 
// 
int __ucl_init_done = 0; // @global

UCL_PUBLIC(int) __ucl_init2(ucl_uint32 v, int s1, int s2, int s3, int s4, int s5, int s6, int s7, int s8, int s9)
{
	int r;
#if (ACC_CC_MSC && ((_MSC_VER) < 700))
#else
	#include "acc/acc_chk.ch"
	#undef ACCCHK_ASSERT
#endif
	__ucl_init_done = 1;
	if(v == 0)
		return UCL_E_ERROR;
	r = (s1 == -1 || s1 == (int)sizeof(short)) && (s2 == -1 || s2 == (int)sizeof(int)) &&
	    (s3 == -1 || s3 == (int)sizeof(long)) && (s4 == -1 || s4 == (int)sizeof(ucl_uint32)) &&
	    (s5 == -1 || s5 == (int)sizeof(ucl_uint)) && (s6 == -1 || s6 > 0) &&
	    (s7 == -1 || s7 == (int)sizeof(char *)) && (s8 == -1 || s8 == (int)sizeof(ucl_voidp)) &&
	    (s9 == -1 || s9 == (int)sizeof(ucl_compress_t));
	if(!r)
		return UCL_E_ERROR;
	r = _ucl_config_check();
	if(r != UCL_E_OK)
		return r;
	return r;
}

#include "ucl_dll.ch"
//
// N2B_99.C
//
//
// implementation of the NRV2B-99 compression algorithm
// This file is part of the UCL data compression library.
// 
#define NRV2B
#include "n2_99.ch"
#undef NRV2B