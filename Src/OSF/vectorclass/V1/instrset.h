/****************************  instrset.h   **********************************
* Author:        Agner Fog
* Date created:  2012-05-30
* Last modified: 2019-08-01
* Version:       1.40.00
* Project:       vector class library
* Description:
* Header file for various compiler-specific tasks as well as common
* macros and templates

* > Selects the supported instruction set
* > Defines integer types
* > Defines compiler version macros
* > Undefines certain macros that prevent function overloading
* > Defines template class to represent compile-time integer constant
* > Defines template for compile-time error messages
* > Helper functions that depend on instruction set, compiler, or platform
* > Common templates
*
* For detailed instructions, see vcl_manual.pdf
*
* (c) Copyright 2012-2019 Agner Fog.
* Apache License version 2.0 or later.
******************************************************************************/

#ifndef INSTRSET_H
#define INSTRSET_H 14000

// Detect 64 bit mode
#if (defined(_M_AMD64) || defined(_M_X64) || defined(__amd64) ) && ! defined(__x86_64__)
#define __x86_64__ 1  // There are many different macros for this, decide on only one
#endif

// Find instruction set from compiler macros if INSTRSET not defined
// Note: Most of these macros are not defined in Microsoft compilers
#ifndef INSTRSET
#if defined ( __AVX512VL__ ) && defined ( __AVX512BW__ ) && defined ( __AVX512DQ__ ) 
#define INSTRSET 10
#elif defined ( __AVX512F__ ) || defined ( __AVX512__ )
#define INSTRSET 9
#elif defined ( __AVX2__ )
#define INSTRSET 8
#elif defined ( __AVX__ )
#define INSTRSET 7
#elif defined ( __SSE4_2__ )
#define INSTRSET 6
#elif defined ( __SSE4_1__ )
#define INSTRSET 5
#elif defined ( __SSSE3__ )
#define INSTRSET 4
#elif defined ( __SSE3__ )
#define INSTRSET 3
#elif defined ( __SSE2__ ) || defined ( __x86_64__ )
#define INSTRSET 2
#elif defined ( __SSE__ )
#define INSTRSET 1
#elif defined ( _M_IX86_FP )           // Defined in MS compiler. 1: SSE, 2: SSE2
#define INSTRSET _M_IX86_FP
#else 
#define INSTRSET 0
#endif // instruction set defines
#endif // INSTRSET

// Include the appropriate header file for intrinsic functions
#if INSTRSET > 7                       // AVX2 and later
	#if defined (__GNUC__) && ! defined (__INTEL_COMPILER)
		#include <x86intrin.h> // x86intrin.h includes header files for whatever instruction sets are specified on the compiler command line, such as: xopintrin.h, fma4intrin.h
	#else
		#include <immintrin.h>                 // MS/Intel version of immintrin.h covers AVX and later
	#endif // __GNUC__
#elif INSTRSET == 7
	#include <immintrin.h>                 // AVX
#elif INSTRSET == 6
	#include <nmmintrin.h>                 // SSE4.2
#elif INSTRSET == 5
	#include <smmintrin.h>                 // SSE4.1
#elif INSTRSET == 4
	#include <tmmintrin.h>                 // SSSE3
#elif INSTRSET == 3
	#include <pmmintrin.h>                 // SSE3
#elif INSTRSET == 2
	#include <emmintrin.h>                 // SSE2
#elif INSTRSET == 1
	#include <xmmintrin.h>                 // SSE
#endif // INSTRSET
#if INSTRSET >= 8 && !defined(__FMA__)
	// Assume that all processors that have AVX2 also have FMA3
	#if defined (__GNUC__) && ! defined (__INTEL_COMPILER) 
		// Prevent error message in g++ and Clang when using FMA intrinsics with avx2:
		#pragma message "It is recommended to specify also option -mfma when using -mavx2 or higher"
	//#else
	#elif ! defined (__clang__)
		#define __FMA__  1
	#endif
#endif
// AMD  instruction sets
#if defined (__XOP__) || defined (__FMA4__)
	#ifdef __GNUC__
		#include <x86intrin.h>                 // AMD XOP (Gnu)
	#else
		#include <ammintrin.h>                 // AMD XOP (Microsoft)
	#endif //  __GNUC__
#elif defined (__SSE4A__)              // AMD SSE4A
	#include <ammintrin.h>
#endif // __XOP__ 
// FMA3 instruction set
#if defined (__FMA__) && (defined(__GNUC__) || defined(__clang__))  && ! defined (__INTEL_COMPILER)
	#include <fmaintrin.h> 
#endif // __FMA__ 
// FMA4 instruction set
#if defined (__FMA4__) && (defined(__GNUC__) || defined(__clang__))
	#include <fma4intrin.h> // must have both x86intrin.h and fma4intrin.h, don't know why
#endif // __FMA4__
// Define integer types with known size
#if defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1600)
  // Compilers supporting C99 or C++0x have stdint.h defining these integer types
  #include <stdint.h>
#elif defined(_MSC_VER)
	// Older Microsoft compilers have their own definitions
	typedef signed   __int8  int8_t;
	typedef unsigned __int8  uint8_t;
	typedef signed   __int16 int16_t;
	typedef unsigned __int16 uint16_t;
	typedef signed   __int32 int32_t;
	typedef unsigned __int32 uint32_t;
	typedef signed   __int64 int64_t;
	typedef unsigned __int64 uint64_t;
	#ifndef _INTPTR_T_DEFINED
		#define _INTPTR_T_DEFINED
		#ifdef  __x86_64__
			typedef int64_t intptr_t;
		#else
			typedef int32_t intptr_t;
		#endif
	#endif
#else
	// This works with most compilers
	typedef signed   char      int8_t;
	typedef unsigned char      uint8_t;
	typedef signed   short int int16_t;
	typedef unsigned short int uint16_t;
	typedef signed   int       int32_t;
	typedef unsigned int       uint32_t;
	typedef long long          int64_t;
	typedef unsigned long long uint64_t;
	#ifdef  __x86_64__
		typedef int64_t intptr_t;
	#else
		typedef int32_t intptr_t;
	#endif
#endif
#include <stdlib.h>                    // define abs(int)
#ifdef _MSC_VER                        // Microsoft compiler or compatible Intel compiler
	#include <intrin.h>                    // define _BitScanReverse(int), __cpuid(int[4],int), _xgetbv(int)
#endif // _MSC_VER
// functions in instrset_detect.cpp
#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif
	int  instrset_detect(void);        // tells which instruction sets are supported
	bool hasFMA3(void);                // true if FMA3 instructions supported
	bool hasFMA4(void);                // true if FMA4 instructions supported
	bool hasXOP(void);                 // true if XOP  instructions supported
	bool hasAVX512ER(void);            // true if AVX512ER instructions supported
	bool hasAVX512VBMI(void);          // true if AVX512VBMI instructions supported
	bool hasAVX512VBMI2(void);         // true if AVX512VBMI2 instructions supported
#ifdef VCL_NAMESPACE
}
#endif

// GCC version
#if defined(__GNUC__) && !defined (GCC_VERSION) && !defined (__clang__)
#define GCC_VERSION  ((__GNUC__) * 10000 + (__GNUC_MINOR__) * 100 + (__GNUC_PATCHLEVEL__))
#endif

// Clang version
#if defined (__clang__)
#define CLANG_VERSION  ((__clang_major__) * 10000 + (__clang_minor__) * 100 + (__clang_patchlevel__))
// Problem: The version number is not consistent across platforms
// http://llvm.org/bugs/show_bug.cgi?id=12643
// Apple bug 18746972
#endif

// Fix problem with non-overloadable macros named min and max in WinDef.h
#ifdef _MSC_VER
#if defined (_WINDEF_) && defined(min) && defined(max)
#undef min
#undef max
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// Fix problem with ambiguous overloading of pow function in Microsoft math.h:
// make sure math.h is included first rather than last
#if defined _MSC_VER && _MSC_VER >= 1800
#include <math.h>
#endif 


/* Clang problem:
The Clang compiler treats the intrinsic vector types __m128, __m128i, and __m128f as identical.
I have reported this problem in 2013 but it is still not fixed in april 2019!
See the bug report at https://bugs.llvm.org/show_bug.cgi?id=17164
Additional problem: The version number is not consistent across platforms. The Apple build has 
different version numbers. We have to rely on __apple_build_version__ on the Mac platform:
http://llvm.org/bugs/show_bug.cgi?id=12643
I have received reports that there was no aliasing of vector types on __apple_build_version__ = 6020053
but apparently the problem has come back. The aliasing of vector types has been reported on 
__apple_build_version__ = 8000042
We have to make switches here when - hopefully - the error some day has been fixed.
We need different version checks with and whithout __apple_build_version__
*/
#if (defined (__clang__) || defined(__apple_build_version__)) && !defined(__INTEL_COMPILER) 
#define FIX_CLANG_VECTOR_ALIAS_AMBIGUITY  
#endif

#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif

// Constant for don't care in permute functions
const int V_DC = -256;


/*****************************************************************************
*
*    Helper functions that depend on instruction set, compiler, or platform
*
*****************************************************************************/

// Define interface to cpuid instruction.
// input:  eax = functionnumber, ecx = 0
// output: eax = output[0], ebx = output[1], ecx = output[2], edx = output[3]
static inline void cpuid (int output[4], int functionnumber) {	
#if defined(__GNUC__) || defined(__clang__)           // use inline assembly, Gnu/AT&T syntax
    int a, b, c, d;
    __asm("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(functionnumber),"c"(0) : );
    output[0] = a;
    output[1] = b;
    output[2] = c;
    output[3] = d;

#elif defined (_MSC_VER)                              // Microsoft compiler, intrin.h included
    __cpuidex(output, functionnumber, 0);             // intrinsic function for CPUID

#else                                                 // unknown platform. try inline assembly with masm/intel syntax
    __asm {
        mov eax, functionnumber
        xor ecx, ecx
        cpuid;
        mov esi, output
            mov [esi],    eax
            mov [esi+4],  ebx
            mov [esi+8],  ecx
            mov [esi+12], edx
    }
#endif
}


// Define popcount function. Gives sum of bits
#if INSTRSET >= 6   // SSE4.2
// popcnt instruction is not officially part of the SSE4.2 instruction set,
// but available in all known processors with SSE4.2
//#if defined (__GNUC__) || defined(__clang__)
//    static inline uint32_t vml_popcnt (uint32_t a) __attribute__ ((pure));
//    static inline uint32_t vml_popcnt (uint32_t a) {	
//        uint32_t r;
//        __asm("popcnt %1, %0" : "=r"(r) : "r"(a) : );
//        return r;
//    }
//#else
    static inline uint32_t vml_popcnt (uint32_t a) {	
        return (uint32_t)_mm_popcnt_u32(a);  // Intel intrinsic. seems to be supported by gcc and clang
    }
#ifdef __x86_64__
    static inline int64_t vml_popcnt (uint64_t a) {	
        return _mm_popcnt_u64(a);  // Intel intrinsic.
    }
#else   // 32 bit mode
    static inline int64_t vml_popcnt (uint64_t a) {	
        return _mm_popcnt_u32(uint32_t(a >> 32)) + _mm_popcnt_u32(uint32_t(a));
    }
#endif
//#endif // platform
#else  // no SSE4.2
    static inline uint32_t vml_popcnt (uint32_t a) {	
        // popcnt instruction not available
        uint32_t b = a - ((a >> 1) & 0x55555555);
        uint32_t c = (b & 0x33333333) + ((b >> 2) & 0x33333333);
        uint32_t d = (c + (c >> 4)) & 0x0F0F0F0F;
        uint32_t e = d * 0x01010101;
        return   e >> 24;
    }
#endif


// Define bit-scan-forward function. Gives index to lowest set bit
#if defined (__GNUC__) || defined(__clang__)
    static inline uint32_t bit_scan_forward (uint32_t a) __attribute__ ((pure));
    static inline uint32_t bit_scan_forward (uint32_t a) {	
        uint32_t r;
        __asm("bsfl %1, %0" : "=r"(r) : "r"(a) : );
        return r;
    }
    // to do: make 64 bit version
#else
    static inline uint32_t bit_scan_forward (uint32_t a) {	
        unsigned long r;
        _BitScanForward(&r, a);        // defined in intrin.h for MS and Intel compilers
        return r;
    }
#ifdef __x86_64__
    static inline uint32_t bit_scan_forward (uint64_t a) {	
        unsigned long r;
        _BitScanForward64(&r, a);      // defined in intrin.h for MS and Intel compilers
        return (uint32_t)r;
    }
#endif
#endif
#ifndef __x86_64__
    static inline uint32_t bit_scan_forward (uint64_t a) {	
        uint32_t lo = uint32_t(a);
        if (lo) return bit_scan_forward(lo);            
        uint32_t hi = uint32_t(a >> 32);
        return bit_scan_forward(hi)+32;
    }
#endif


// Define bit-scan-reverse function. Gives index to highest set bit = floor(log2(a))
#if defined (__GNUC__) || defined(__clang__)
    static inline uint32_t bit_scan_reverse (uint32_t a) __attribute__ ((pure));
    static inline uint32_t bit_scan_reverse (uint32_t a) {	
        uint32_t r;
        __asm("bsrl %1, %0" : "=r"(r) : "r"(a) : );
        return r;
    }
#else
    static inline uint32_t bit_scan_reverse (uint32_t a) {	
        unsigned long r;
        _BitScanReverse(&r, a);        // defined in intrin.h for MS and Intel compilers
        return r;
    }
#endif

// Same function, for compile-time constants.
// We need template metaprogramming for calculating this function at compile time.
// This may take a long time to compile because of the template recursion.
// Todo: replace this with a constexpr function when C++14 becomes available
    template <uint32_t n> 
    struct BitScanR {
        enum {val = int(
            n >= 0x10u ? 4 + (BitScanR<(n>>4)>::val) :
            n  <    2u ? 0 :
            n  <    4u ? 1 :
            n  <    8u ? 2 : 3 )                       };
    };
    template <> struct BitScanR<0> {enum {val = 0};}; // Avoid infinite template recursion

#define bit_scan_reverse_const(n)  (BitScanR<n>::val) // n must be a valid compile-time constant


/*****************************************************************************
*
*    Common templates
*
*****************************************************************************/

// Template class to represent compile-time integer constant
template <int32_t  n> class Const_int_t {};       // represent compile-time signed integer constant
template <uint32_t n> class Const_uint_t {};      // represent compile-time unsigned integer constant
#define const_int(n)  (Const_int_t <n>())         // n must be compile-time integer constant
#define const_uint(n) (Const_uint_t<n>())         // n must be compile-time unsigned integer constant

// Template for compile-time error messages
// (may be replaced by static_assert in C++11)
template <bool> class Static_error_check {
    public:  Static_error_check() {};
};
template <> class Static_error_check<false> {     // generate compile-time error if false
    private: Static_error_check() {};
};

// template for producing quiet NAN
template <class VTYPE> static inline VTYPE nan_vec(uint32_t payload = 0x100) {
    /*
    if (VTYPE::elementtype() & 1) {  // double
        union {
            uint64_t q;
            double f;
        } ud;
        // n is left justified to avoid loss of NAN payload when converting to float
        ud.q = 0x7FF8000000000000 | uint64_t(payload) << 29;
        return VTYPE(ud.f);
    } */
    // float will be converted to double if necessary
    union {
        uint32_t i;
        float f;
    } uf;
    uf.i = 0x7FC00000 | (payload & 0x003FFFFF);
    return VTYPE(uf.f);
}


#ifdef VCL_NAMESPACE
}
#endif 


#endif // INSTRSET_H
