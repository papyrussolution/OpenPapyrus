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

UCL_PUBLIC(unsigned int) __ucl_align_gap(const ucl_voidp ptr, ucl_uint size)
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
    assert(static_cast<long>(n) >= 0);
    assert(n <= s);
    return (unsigned int)(n);
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

//#include "acc/acc_chk.ch"
#if !defined(ACCCHK_ASSERT)
	#define ACCCHK_ASSERT(expr)   ACC_COMPILE_TIME_ASSERT(expr)
#endif
// compile-time sign 
#if !defined(ACCCHK_ASSERT_SIGN_T)
	#define ACCCHK_ASSERT_SIGN_T(type,relop) ACCCHK_ASSERT((type)(-1) relop (type)0) ACCCHK_ASSERT((type)(~(type)0) relop (type)0) ACCCHK_ASSERT((type)(~(type)0) == (type)(-1))
#endif
#if !defined(ACCCHK_IS_SIGNED_T)
	#define ACCCHK_ASSERT_IS_SIGNED_T(type)       ACCCHK_ASSERT_SIGN_T(type,<)
#endif
#if !defined(ACCCHK_IS_UNSIGNED_T)
	#if (ACC_BROKEN_INTEGRAL_PROMOTION)
		#define ACCCHK_ASSERT_IS_UNSIGNED_T(type) ACCCHK_ASSERT( (type) (-1) > (type) 0 )
	#else
		#define ACCCHK_ASSERT_IS_UNSIGNED_T(type)   ACCCHK_ASSERT_SIGN_T(type,>)
	#endif
#endif
//
// check preprocessor
//
#if (ACC_0xffffffffL - ACC_UINT32L_C(4294967294) != 1)
	#error "preprocessor error 1"
#endif
#if (ACC_0xffffffffL - ACC_UINT32L_C(0xfffffffd) != 2)
	#error "preprocessor error 2"
#endif
#define ACCCHK_VAL  1
#define ACCCHK_TMP1 ACCCHK_VAL
#undef ACCCHK_VAL
#define ACCCHK_VAL  2
#define ACCCHK_TMP2 ACCCHK_VAL
#if (ACCCHK_TMP1 != 2)
	#error "preprocessor error 3a"
#endif
#if (ACCCHK_TMP2 != 2)
	#error "preprocessor error 3b"
#endif
#undef ACCCHK_VAL
#if (ACCCHK_TMP2)
	#error "preprocessor error 3c"
#endif
#if (ACCCHK_TMP2 + 0 != 0)
	#error "preprocessor error 3d"
#endif
#undef ACCCHK_TMP1
#undef ACCCHK_TMP2
//
// check basic arithmetics
//
ACCCHK_ASSERT(1 == 1)
ACCCHK_ASSERT(__ACC_INT_MAX(2) == 1)
ACCCHK_ASSERT(__ACC_INT_MAX(8) == 127)
ACCCHK_ASSERT(__ACC_INT_MAX(16) == 32767)
ACCCHK_ASSERT(__ACC_UINT_MAX(2) == 3)
ACCCHK_ASSERT(__ACC_UINT_MAX(16) == 0xffffU)
ACCCHK_ASSERT(__ACC_UINT_MAX(32) == 0xffffffffUL)
#if !defined(ACC_BROKEN_INTEGRAL_PROMOTION)
    ACCCHK_ASSERT(__ACC_UINT_MAX(__ACC_INT_BIT) == ~(0u))
    ACCCHK_ASSERT(__ACC_UINT_MAX(__ACC_LONG_BIT) == ~(0ul))
#endif
//
// check basic types
//
ACCCHK_ASSERT_IS_SIGNED_T(signed char)
ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned char)
ACCCHK_ASSERT(sizeof(signed char) == sizeof(char))
ACCCHK_ASSERT(sizeof(unsigned char) == sizeof(char))
ACCCHK_ASSERT(sizeof(char) == 1)
#if (ACC_CC_CILLY)
    // CIL is broken 
#else
    ACCCHK_ASSERT(sizeof(char) == sizeof((char)0))
#endif
#if defined(__cplusplus)
	ACCCHK_ASSERT(sizeof('\0') == sizeof(char))
#else
	#if (ACC_CC_DMC)
		// Digital Mars C is broken 
	#else
		ACCCHK_ASSERT(sizeof('\0') == sizeof(int))
	#endif
#endif
#if defined(acc_alignof)
    ACCCHK_ASSERT(acc_alignof(char) == 1)
    ACCCHK_ASSERT(acc_alignof(signed char) == 1)
    ACCCHK_ASSERT(acc_alignof(unsigned char) == 1)
#endif
    ACCCHK_ASSERT_IS_SIGNED_T(short)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned short)
    ACCCHK_ASSERT(sizeof(short) == sizeof(unsigned short))
    ACCCHK_ASSERT(sizeof(short) >= 2)
    ACCCHK_ASSERT(sizeof(short) >= sizeof(char))
#if (ACC_CC_CILLY)
    // CIL is broken 
#else
    ACCCHK_ASSERT(sizeof(short) == sizeof((short)0))
#endif
#if (SIZEOF_SHORT > 0)
    ACCCHK_ASSERT(sizeof(short) == SIZEOF_SHORT)
#endif
    ACCCHK_ASSERT_IS_SIGNED_T(int)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned int)
    ACCCHK_ASSERT(sizeof(int) == sizeof(unsigned int))
    ACCCHK_ASSERT(sizeof(int) >= 2)
    ACCCHK_ASSERT(sizeof(int) >= sizeof(short))
    ACCCHK_ASSERT(sizeof(int) == sizeof(0))
    ACCCHK_ASSERT(sizeof(int) == sizeof((int)0))
#if (SIZEOF_INT > 0)
    ACCCHK_ASSERT(sizeof(int) == SIZEOF_INT)
#endif
    ACCCHK_ASSERT_IS_SIGNED_T(long)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned long)
    ACCCHK_ASSERT(sizeof(long) == sizeof(unsigned long))
    ACCCHK_ASSERT(sizeof(long) >= 4)
    ACCCHK_ASSERT(sizeof(long) >= sizeof(int))
    ACCCHK_ASSERT(sizeof(long) == sizeof(0L))
    ACCCHK_ASSERT(sizeof(long) == sizeof((long)0))
#if (SIZEOF_LONG > 0)
    ACCCHK_ASSERT(sizeof(long) == SIZEOF_LONG)
#endif
    ACCCHK_ASSERT_IS_UNSIGNED_T(size_t)
    ACCCHK_ASSERT(sizeof(size_t) >= sizeof(int))
    ACCCHK_ASSERT(sizeof(size_t) == sizeof(sizeof(0))) /* sizeof() returns size_t */
#if (SIZEOF_SIZE_T > 0)
    ACCCHK_ASSERT(sizeof(size_t) == SIZEOF_SIZE_T)
#endif
    ACCCHK_ASSERT_IS_SIGNED_T(ptrdiff_t)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) >= sizeof(int))
    ACCCHK_ASSERT(sizeof(ptrdiff_t) >= sizeof(size_t))
#if !defined(ACC_BROKEN_SIZEOF)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == sizeof((char*)0 - (char*)0))
# if (ACC_HAVE_MM_HUGE_PTR)
    ACCCHK_ASSERT(4 == sizeof((char __huge*)0 - (char __huge*)0))
# endif
#endif
#if (SIZEOF_PTRDIFF_T > 0)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == SIZEOF_PTRDIFF_T)
#endif
    ACCCHK_ASSERT(sizeof(void*) >= sizeof(char*))
#if (SIZEOF_VOID_P > 0)
    ACCCHK_ASSERT(sizeof(void*) == SIZEOF_VOID_P)
#endif
#if (SIZEOF_CHAR_P > 0)
    ACCCHK_ASSERT(sizeof(char*) == SIZEOF_CHAR_P)
#endif
#if (ACC_HAVE_MM_HUGE_PTR)
    ACCCHK_ASSERT(4 == sizeof(void __huge*))
    ACCCHK_ASSERT(4 == sizeof(char __huge*))
#endif
//
// check arithmetics
//
ACCCHK_ASSERT((((1u  << 15) + 1) >> 15) == 1)
ACCCHK_ASSERT((((1ul << 31) + 1) >> 31) == 1)
#if (ACC_CC_TURBOC && (__TURBOC__ < 0x0150))
    // TC 1.0 bug, probably due to ACC_BROKEN_INTEGRAL_PROMOTION ?? 
#else
    ACCCHK_ASSERT((1   << (8*SIZEOF_INT-1)) < 0)
#endif
    ACCCHK_ASSERT((1u  << (8*SIZEOF_INT-1)) > 0)
    ACCCHK_ASSERT((1l  << (8*SIZEOF_LONG-1)) < 0)
    ACCCHK_ASSERT((1ul << (8*SIZEOF_LONG-1)) > 0)
#if defined(acc_int32e_t)
    ACCCHK_ASSERT(sizeof(acc_int32e_t) == 4)
    ACCCHK_ASSERT(sizeof(acc_int32e_t) == SIZEOF_ACC_INT32E_T)
    ACCCHK_ASSERT(sizeof(acc_uint32e_t) == 4)
    ACCCHK_ASSERT(sizeof(acc_int32e_t) == sizeof(acc_uint32e_t))
    ACCCHK_ASSERT_IS_SIGNED_T(acc_int32e_t)
    ACCCHK_ASSERT(((( (acc_int32e_t)1 << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT(((( ACC_INT32E_C(1) << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint32e_t)
    ACCCHK_ASSERT(((( (acc_uint32e_t)1 << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT(((( ACC_UINT32E_C(1) << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT( (acc_int32e_t) (1 + ~(acc_int32e_t)0) == 0)
#if defined(ACCCHK_CONFIG_PEDANTIC)
    // compiler may warn about overflow 
    ACCCHK_ASSERT( (acc_uint32e_t)(1 + ~(acc_uint32e_t)0) == 0)
#endif // ACCCHK_CONFIG_PEDANTIC 
#if (SIZEOF_ACC_INT32E_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT32E_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT32E_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT32E_C(0)) == SIZEOF_ACC_INT32E_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT32E_C(0)) == SIZEOF_ACC_INT32E_T)
#endif
    ACCCHK_ASSERT((ACC_INT32E_C(1)  << (8*SIZEOF_ACC_INT32E_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32E_C(1) << (8*SIZEOF_ACC_INT32E_T-1)) > 0)
    ACCCHK_ASSERT((ACC_INT32E_C(1)  << (int)(8*sizeof(ACC_INT32E_C(1))-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32E_C(1) << (int)(8*sizeof(ACC_UINT32E_C(1))-1)) > 0)
    ACCCHK_ASSERT(ACC_INT32E_C(2147483647)      > 0)
    ACCCHK_ASSERT(ACC_INT32E_C(-2147483647) -1  < 0)
    ACCCHK_ASSERT(ACC_UINT32E_C(4294967295)     > 0)
    ACCCHK_ASSERT(ACC_UINT32E_C(4294967295) == ACC_0xffffffffL)
#endif
    ACCCHK_ASSERT(sizeof(acc_int32l_t) >= sizeof(int))
#if defined(acc_int32e_t)
    ACCCHK_ASSERT(sizeof(acc_int32l_t) >= sizeof(acc_int32e_t))
#endif
    ACCCHK_ASSERT(sizeof(acc_int32l_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32l_t) == SIZEOF_ACC_INT32L_T)
    ACCCHK_ASSERT(sizeof(acc_uint32l_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32l_t) == sizeof(acc_uint32l_t))
    ACCCHK_ASSERT_IS_SIGNED_T(acc_int32l_t)
    ACCCHK_ASSERT(((( (acc_int32l_t)1 << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT(((( ACC_INT32L_C(1) << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint32l_t)
    ACCCHK_ASSERT(((( (acc_uint32l_t)1 << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT(((( ACC_UINT32L_C(1) << 31) + 1) >> 31) == 1)
#if (SIZEOF_ACC_INT32L_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT32L_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT32L_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT32L_C(0)) == SIZEOF_ACC_INT32L_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT32L_C(0)) == SIZEOF_ACC_INT32L_T)
#endif
    ACCCHK_ASSERT((ACC_INT32L_C(1)  << (8*SIZEOF_ACC_INT32L_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32L_C(1) << (8*SIZEOF_ACC_INT32L_T-1)) > 0)
    ACCCHK_ASSERT((ACC_INT32L_C(1)  << (int)(8*sizeof(ACC_INT32L_C(1))-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32L_C(1) << (int)(8*sizeof(ACC_UINT32L_C(1))-1)) > 0)
    ACCCHK_ASSERT(ACC_INT32L_C(2147483647)      > 0)
    ACCCHK_ASSERT(ACC_INT32L_C(-2147483647) -1  < 0)
    ACCCHK_ASSERT(ACC_UINT32L_C(4294967295)     > 0)
    ACCCHK_ASSERT(ACC_UINT32L_C(4294967295) == ACC_0xffffffffL)
    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= sizeof(int))
#if defined(acc_int32e_t)
    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= sizeof(acc_int32e_t))
#endif
    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= sizeof(acc_int32l_t))
    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= sizeof(acc_int32l_t))
    ACCCHK_ASSERT(sizeof(acc_int32f_t) == SIZEOF_ACC_INT32F_T)
    ACCCHK_ASSERT(sizeof(acc_uint32f_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_uint32f_t) >= sizeof(acc_uint32l_t))
    ACCCHK_ASSERT(sizeof(acc_int32f_t) == sizeof(acc_uint32f_t))
    ACCCHK_ASSERT_IS_SIGNED_T(acc_int32f_t)
    ACCCHK_ASSERT(((( (acc_int32f_t)1 << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT(((( ACC_INT32F_C(1) << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint32f_t)
    ACCCHK_ASSERT(((( (acc_uint32f_t)1 << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT(((( ACC_UINT32F_C(1) << 31) + 1) >> 31) == 1)
#if (SIZEOF_ACC_INT32F_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT32F_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT32F_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT32F_C(0)) == SIZEOF_ACC_INT32F_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT32F_C(0)) == SIZEOF_ACC_INT32F_T)
#endif
    ACCCHK_ASSERT((ACC_INT32F_C(1)  << (8*SIZEOF_ACC_INT32F_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32F_C(1) << (8*SIZEOF_ACC_INT32F_T-1)) > 0)
    ACCCHK_ASSERT((ACC_INT32F_C(1)  << (int)(8*sizeof(ACC_INT32F_C(1))-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32F_C(1) << (int)(8*sizeof(ACC_UINT32F_C(1))-1)) > 0)
    ACCCHK_ASSERT(ACC_INT32F_C(2147483647)      > 0)
    ACCCHK_ASSERT(ACC_INT32F_C(-2147483647) -1  < 0)
    ACCCHK_ASSERT(ACC_UINT32F_C(4294967295)     > 0)
    ACCCHK_ASSERT(ACC_UINT32F_C(4294967295) == ACC_0xffffffffL)
#if defined(acc_int64l_t)
    ACCCHK_ASSERT(sizeof(acc_int64l_t) >= 8)
    ACCCHK_ASSERT(sizeof(acc_int64l_t) == SIZEOF_ACC_INT64L_T)
    ACCCHK_ASSERT(sizeof(acc_uint64l_t) >= 8)
    ACCCHK_ASSERT(sizeof(acc_int64l_t) == sizeof(acc_uint64l_t))
    ACCCHK_ASSERT_IS_SIGNED_T(acc_int64l_t)
    ACCCHK_ASSERT(((( (acc_int64l_t)1 << 62) + 1) >> 62) == 1)
    ACCCHK_ASSERT(((( ACC_INT64L_C(1) << 62) + 1) >> 62) == 1)
#if (ACC_CC_BORLANDC && (__BORLANDC__ < 0x0530))
    // Borland C is broken 
#else
    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint64l_t)
    ACCCHK_ASSERT(ACC_UINT64L_C(18446744073709551615)     > 0)
#endif
    ACCCHK_ASSERT(((( (acc_uint64l_t)1 << 63) + 1) >> 63) == 1)
    ACCCHK_ASSERT(((( ACC_UINT64L_C(1) << 63) + 1) >> 63) == 1)
#if defined(ACCCHK_CONFIG_PEDANTIC)
#if (ACC_CC_BORLANDC && (__BORLANDC__ < 0x0560))
    // Borland C is broken 
#elif (SIZEOF_ACC_INT64L_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT64L_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT64L_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT64L_C(0)) == SIZEOF_ACC_INT64L_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT64L_C(0)) == SIZEOF_ACC_INT64L_T)
#endif
#endif /* ACCCHK_CONFIG_PEDANTIC */
    ACCCHK_ASSERT((ACC_INT64L_C(1)  << (8*SIZEOF_ACC_INT64L_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT64L_C(1) << (8*SIZEOF_ACC_INT64L_T-1)) > 0)
#if (ACC_CC_GNUC && (ACC_CC_GNUC < 0x020600ul))
    /* avoid pedantic warning */
    ACCCHK_ASSERT(ACC_INT64L_C(9223372036854775807)       > ACC_INT64L_C(0))
#else
    ACCCHK_ASSERT(ACC_INT64L_C(9223372036854775807)       > 0)
#endif
    ACCCHK_ASSERT(ACC_INT64L_C(-9223372036854775807) - 1  < 0)
    ACCCHK_ASSERT( ACC_INT64L_C(9223372036854775807) % 2147483629l  == 721)
    ACCCHK_ASSERT( ACC_INT64L_C(9223372036854775807) % 2147483647l  == 1)
    ACCCHK_ASSERT(ACC_UINT64L_C(9223372036854775807) % 2147483629ul == 721)
    ACCCHK_ASSERT(ACC_UINT64L_C(9223372036854775807) % 2147483647ul == 1)
#endif /* acc_int64l_t */
    ACCCHK_ASSERT_IS_SIGNED_T(acc_intptr_t)
    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uintptr_t)
    ACCCHK_ASSERT(sizeof(acc_intptr_t) >= sizeof(void *))
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == SIZEOF_ACC_INTPTR_T)
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == sizeof(acc_uintptr_t))
//
// check memory model ACC_MM
//
#if (ACC_MM_FLAT)
#if 0
    /* this is not a valid assumption -- disabled */
    ACCCHK_ASSERT(sizeof(void*) == sizeof(void (*)(void)))
#endif
#endif
#if (ACC_MM_TINY || ACC_MM_SMALL || ACC_MM_MEDIUM)
    ACCCHK_ASSERT(sizeof(void*) == 2)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == 2)
#elif (ACC_MM_COMPACT || ACC_MM_LARGE || ACC_MM_HUGE)
    ACCCHK_ASSERT(sizeof(void*) == 4)
#endif
#if (ACC_MM_TINY || ACC_MM_SMALL || ACC_MM_COMPACT)
    ACCCHK_ASSERT(sizeof(void (*)(void)) == 2)
#elif (ACC_MM_MEDIUM || ACC_MM_LARGE || ACC_MM_HUGE)
    ACCCHK_ASSERT(sizeof(void (*)(void)) == 4)
#endif
//
// check ACC_ARCH and ACC_OS
//
#if (ACC_ARCH_IA16)
    ACCCHK_ASSERT(sizeof(size_t) == 2)
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == sizeof(void *))
#elif (ACC_ARCH_IA32 || ACC_ARCH_M68K)
    ACCCHK_ASSERT(sizeof(size_t) == 4)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == 4)
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == sizeof(void *))
#elif (ACC_ARCH_AMD64 || ACC_ARCH_IA64)
    ACCCHK_ASSERT(sizeof(size_t) == 8)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == 8)
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == sizeof(void *))
#endif
#if (ACC_OS_DOS32 || ACC_OS_OS2 || ACC_OS_WIN32)
    ACCCHK_ASSERT(sizeof(size_t) == 4)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == 4)
    ACCCHK_ASSERT(sizeof(void (*)(void)) == 4)
#elif (ACC_OS_WIN64)
    ACCCHK_ASSERT(sizeof(size_t) == 8)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == 8)
    ACCCHK_ASSERT(sizeof(void (*)(void)) == 8)
#endif
//
// check promotion rules
//
#if (ACC_CC_NDPC)
    // NDP C is broken 
#else
    // check that the compiler correctly casts signed to unsigned 
    ACCCHK_ASSERT( (int) ((unsigned char) ((signed char) -1)) == 255)
#endif
#if (ACC_CC_KEILC)
    // Keil C is broken 
#elif (ACC_CC_NDPC)
    // NDP C is broken 
#elif !defined(ACC_BROKEN_INTEGRAL_PROMOTION) && (SIZEOF_INT > 1)
    // check that the compiler correctly promotes integrals */
    ACCCHK_ASSERT( (((unsigned char)128) << (int)(8*sizeof(int)-8)) < 0)
#endif
//
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
		const unsigned n = (unsigned int)sizeof(ucl_uint32);
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
	// @v10.8.2 (inlined above) #include "acc/acc_chk.ch"
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