// XXHASH.C
// xxHash - Fast Hash algorithm
// Copyright (C) 2012-2016, Yann Collet
// BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
//
// You can contact the author at :
//  - xxHash homepage: http://www.xxhash.com
//  - xxHash source repository : https://github.com/Cyan4973/xxHash
//
#include <slib-internal.h>
#pragma hdrstop
// 
// Tuning parameters
//
/*!XXH_FORCE_MEMORY_ACCESS :
 * By default, access to unaligned memory is controlled by `memcpy()`, which is safe and portable.
 * Unfortunately, on some target/compiler combinations, the generated assembly is sub-optimal.
 * The below switch allow to select different access method for improved performance.
 * Method 0 (default) : use `memcpy()`. Safe and portable.
 * Method 1 : `__packed` statement. It depends on compiler extension (ie, not portable).
 *      This method is safe if your compiler supports it, and *generally* as fast or faster than `memcpy`.
 * Method 2 : direct access. This method doesn't depend on compiler but violate C standard.
 *      It can generate buggy code on targets which do not support unaligned memory accesses.
 *      But in some circumstances, it's the only known way to get the most performance (ie GCC + ARMv6)
 * See http://stackoverflow.com/a/32095106/646947 for details.
 * Prefer these methods in priority order (0 > 1 > 2)
 */
#ifndef XXH_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
	#if defined(__GNUC__) && defined(__ARM_FEATURE_UNALIGNED) && defined(__ARM_ARCH) && (__ARM_ARCH == 6)
		#define XXH_FORCE_MEMORY_ACCESS 2
	#elif (defined(__INTEL_COMPILER) && !defined(_WIN32)) || (defined(__GNUC__) && (defined(__ARM_ARCH) && __ARM_ARCH >= 7))
		#define XXH_FORCE_MEMORY_ACCESS 1
	#endif
#endif
/*!XXH_ACCEPT_NULL_INPUT_POINTER :
 * If input pointer is NULL, xxHash default behavior is to dereference it, triggering a segfault.
 * When this macro is enabled, xxHash actively checks input for null pointer.
 * It it is, result for null input pointers is the same as a null-length input.
 */
#ifndef XXH_ACCEPT_NULL_INPUT_POINTER   /* can be defined externally */
	#define XXH_ACCEPT_NULL_INPUT_POINTER 0
#endif
/*!XXH_FORCE_ALIGN_CHECK :
 * This is a minor performance trick, only useful with lots of very small keys.
 * It means : check for aligned/unaligned input.
 * The check costs one initial branch per hash;
 * set it to 0 when the input is guaranteed to be aligned,
 * or when alignment doesn't matter for performance.
 */
#ifndef XXH_FORCE_ALIGN_CHECK /* can be defined externally */
	#if defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
		#define XXH_FORCE_ALIGN_CHECK 0
	#else
		#define XXH_FORCE_ALIGN_CHECK 1
	#endif
#endif
/*!XXH_REROLL:
 * Whether to reroll XXH32_finalize, and XXH64_finalize,
 * instead of using an unrolled jump table/if statement loop.
 *
 * This is automatically defined on -Os/-Oz on GCC and Clang. */
#ifndef XXH_REROLL
	#if defined(__OPTIMIZE_SIZE__)
		#define XXH_REROLL 1
	#else
		#define XXH_REROLL 0
	#endif
#endif
// 
// Includes & Memory related functions
//
// Modify the local functions below should you wish to use some other memory routines for malloc(), SAlloc::F() 
//static void * XXH_malloc(size_t s) { return malloc(s); }
//static void  XXH_free(void * p) { free(p); }
//static void * XXH_memcpy(void * dest, const void * src, size_t size) { return memcpy(dest, src, size); }

#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"
// 
// Compiler Specific Options
// 
#ifdef _MSC_VER    /* Visual Studio */
	#pragma warning(disable : 4127)      /* disable: C4127: conditional expression is constant */
	//#define XXH_FORCE_INLINE static __forceinline
	#define XXH_NO_INLINE static __declspec(noinline)
#else
	#if defined (__cplusplus) || defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
		#ifdef __GNUC__
			//#define XXH_FORCE_INLINE static inline __attribute__((always_inline))
			#define XXH_NO_INLINE static __attribute__((noinline))
		#else
			//#define XXH_FORCE_INLINE static inline
			#define XXH_NO_INLINE static
		#endif
	#else
		//#define XXH_FORCE_INLINE static
		#define XXH_NO_INLINE static
	#endif /* __STDC_VERSION__ */
#endif
// 
// Debug
// 
// DEBUGLEVEL is expected to be defined externally,
// typically through compiler command line.
// Value must be a number.
#ifndef DEBUGLEVEL
	#define DEBUGLEVEL 0
#endif
//#if(DEBUGLEVEL>=1)
	//#include <assert.h>  // note : can still be disabled with NDEBUG 
	//#define XXH_ASSERT_Removed(c)   assert(c)
//#else
	//#define XXH_ASSERT_Removed(c)   ((void)0)
//#endif
//#define XXH_STATIC_ASSERT_Removed(c) { enum { XXH_sa = 1/(int)(!!(c)) }; } // note : use after variable declarations 
// 
// Basic Types
// 
//#ifndef MEM_MODULE
	//#if !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
		//#include <stdint.h>
		//typedef uint8_t BYTE;
		//typedef uint16_t U16_Removed;
		//typedef uint32_t U32_Removed;
	//#else
		//typedef uchar BYTE;
		//typedef unsigned short U16_Removed;
		//typedef unsigned int U32_Removed;
	//#endif
//#endif
/* ===   Memory access   === */

#if(defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))
	// Force direct memory access. Only works on CPU which support unaligned memory access in hardware 
	static uint32 XXH_read32(const void * memPtr) { return *(const uint32 *)memPtr; }
#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))
	// __pack instructions are safer, but compiler specific, hence potentially problematic for some compilers 
	// currently only defined for gcc and icc 
	typedef union { uint32 u32; } __attribute__((packed)) unalign;
	static uint32 XXH_read32(const void * ptr) { return ((const unalign*)ptr)->u32; }
#else
	// portable and safe solution. Generally efficient.
	// see : http://stackoverflow.com/a/32095106/646947
	static uint32 XXH_read32(const void * memPtr)
	{
		uint32 val;
		memcpy(&val, memPtr, sizeof(val));
		return val;
	}
#endif   /* XXH_FORCE_DIRECT_MEMORY_ACCESS */

/* ===   Endianess   === */
typedef enum { 
	XXH_bigEndian = 0, 
	XXH_littleEndian = 1 
} XXH_endianess;

/* XXH_CPU_LITTLE_ENDIAN can be defined externally, for example on the compiler command line */
#ifndef XXH_CPU_LITTLE_ENDIAN
	static int XXH_isLittleEndian(void)
	{
		const union { uint32 u; BYTE c[4]; } one = { 1 }; /* don't use static : performance detrimental  */
		return one.c[0];
	}
	#define XXH_CPU_LITTLE_ENDIAN   XXH_isLittleEndian()
#endif
// 
// Compiler-specific Functions and Macros
//
#define XXH_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

// @v11.4.0 #ifndef __has_builtin
// @v11.4.0 #define __has_builtin(x) 0
// @v11.4.0 #endif
/* @v11.4.0 (replaced with SBits::Rotl SBits::Rotl
#if !defined(NO_CLANG_BUILTIN) && __has_builtin(__builtin_rotateleft32) && __has_builtin(__builtin_rotateleft64)
	#define XXH_rotl32 __builtin_rotateleft32
	#define XXH_rotl64 __builtin_rotateleft64
	// Note : although _rotl exists for minGW (GCC under windows), performance seems poor
#elif defined(_MSC_VER)
	#define XXH_rotl32(x, r) _rotl(x, r)
	#define XXH_rotl64(x, r) _rotl64(x, r)
#else
	#define XXH_rotl32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
	#define XXH_rotl64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#endif*/
// @sobolev (XXH_swap32 replaced with sbswap32)
//#if defined(_MSC_VER) // Visual Studio 
	//#define XXH_swap32 _byteswap_ulong
//#elif XXH_GCC_VERSION >= 403
	//#define XXH_swap32 __builtin_bswap32
//#else
	//static uint32 XXH_swap32(uint32 x) { return ((x << 24) & 0xff000000) | ((x <<  8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff); }
//#endif
//
// Memory reads
//
typedef enum { 
	XXH_aligned, 
	XXH_unaligned 
} XXH_alignment;

static FORCEINLINE uint32 XXH_readLE32(const void * ptr) { return XXH_CPU_LITTLE_ENDIAN ? XXH_read32(ptr) : sbswap32(XXH_read32(ptr)); }
static uint32 XXH_readBE32(const void * ptr) { return XXH_CPU_LITTLE_ENDIAN ? sbswap32(XXH_read32(ptr)) : XXH_read32(ptr); }

static FORCEINLINE uint32 XXH_readLE32_align(const void * ptr, XXH_alignment align)
{
	if(align==XXH_unaligned)
		return XXH_readLE32(ptr);
	else
		return XXH_CPU_LITTLE_ENDIAN ? *(const uint32 *)ptr : sbswap32(*(const uint32 *)ptr);
}
// 
// Misc
// 
XXH_PUBLIC_API unsigned XXH_versionNumber(void) { return XXH_VERSION_NUMBER; }
// 
// 32-bit hash functions
// 
static const uint32 PRIME32_1 = 0x9E3779B1U; /* 0b10011110001101110111100110110001 */
static const uint32 PRIME32_2 = 0x85EBCA77U; /* 0b10000101111010111100101001110111 */
static const uint32 PRIME32_3 = 0xC2B2AE3DU; /* 0b11000010101100101010111000111101 */
static const uint32 PRIME32_4 = 0x27D4EB2FU; /* 0b00100111110101001110101100101111 */
static const uint32 PRIME32_5 = 0x165667B1U; /* 0b00010110010101100110011110110001 */

static uint32 XXH32_round(uint32 acc, uint32 input)
{
	acc += input * PRIME32_2;
	acc  = SBits::Rotl(acc, 13);
	acc *= PRIME32_1;
#if defined(__GNUC__) && defined(__SSE4_1__) && !defined(XXH_ENABLE_AUTOVECTORIZE)
	/* UGLY HACK:
	 * This inline assembly hack forces acc into a normal register. This is the
	 * only thing that prevents GCC and Clang from autovectorizing the XXH32 loop
	 * (pragmas and attributes don't work for some resason) without globally
	 * disabling SSE4.1.
	 *
	 * The reason we want to avoid vectorization is because despite working on
	 * 4 integers at a time, there are multiple factors slowing XXH32 down on
	 * SSE4:
	 * - There's a ridiculous amount of lag from pmulld (10 cycles of latency on newer chips!)
	 *   making it slightly slower to multiply four integers at once compared to four
	 *   integers independently. Even when pmulld was fastest, Sandy/Ivy Bridge, it is
	 *   still not worth it to go into SSE just to multiply unless doing a long operation.
	 *
	 * - Four instructions are required to rotate,
	 *      movqda tmp,  v // not required with VEX encoding
	 *      pslld  tmp, 13 // tmp <<= 13
	 *      psrld  v,   19 // x >>= 19
	 *      por    v,  tmp // x |= tmp
	 *   compared to one for scalar:
	 *      roll   v, 13    // reliably fast across the board
	 *      shldl  v, v, 13 // Sandy Bridge and later prefer this for some reason
	 *
	 * - Instruction level parallelism is actually more beneficial here because the
	 *   SIMD actually serializes this operation: While v1 is rotating, v2 can load data,
	 *   while v3 can multiply. SSE forces them to operate together.
	 *
	 * How this hack works:
	 * __asm__(""       // Declare an assembly block but don't declare any instructions
	 *    :       // However, as an Input/Output Operand,
	 *    "+r"    // constrain a read/write operand (+) as a general purpose register (r).
	 *    (acc)   // and set acc as the operand
	 * );
	 *
	 * Because of the 'r', the compiler has promised that seed will be in a
	 * general purpose register and the '+' says that it will be 'read/write',
	 * so it has to assume it has changed. It is like volatile without all the
	 * loads and stores.
	 *
	 * Since the argument has to be in a normal register (not an SSE register),
	 * each time XXH32_round is called, it is impossible to vectorize. */
	__asm__ ("" : "+r" (acc));
#endif
	return acc;
}
//
// mix all bits 
//
static uint32 FASTCALL XXH32_avalanche(uint32 h32)
{
	h32 ^= h32 >> 15;
	h32 *= PRIME32_2;
	h32 ^= h32 >> 13;
	h32 *= PRIME32_3;
	h32 ^= h32 >> 16;
	return h32;
}

#define XXH_get32bits(p) XXH_readLE32_align(p, align)

static uint32 XXH32_finalize(uint32 h32, const void * ptr, size_t len, XXH_alignment align)
{
	const BYTE * p = static_cast<const BYTE *>(ptr);
#define PROCESS1               \
	h32 += (*p++) * PRIME32_5; \
	h32 = SBits::Rotl(h32, 11) * PRIME32_1;

#define PROCESS4                         \
	h32 += XXH_get32bits(p) * PRIME32_3; \
	p += 4;                                \
	h32  = SBits::Rotl(h32, 17) * PRIME32_4;

	/* Compact rerolled version */
	if(XXH_REROLL) {
		len &= 15;
		while(len >= 4) {
			PROCESS4;
			len -= 4;
		}
		while(len > 0) {
			PROCESS1;
			--len;
		}
		return XXH32_avalanche(h32);
	}
	else {
		switch(len&15) /* or switch(bEnd - p) */ {
			case 12:      PROCESS4; // @fallthrough
			case 8:       PROCESS4; // @fallthrough
			case 4:       PROCESS4; return XXH32_avalanche(h32);
			case 13:      PROCESS4; // @fallthrough
			case 9:       PROCESS4; // @fallthrough
			case 5:       PROCESS4; PROCESS1; return XXH32_avalanche(h32);
			case 14:      PROCESS4; // @fallthrough
			case 10:      PROCESS4; // @fallthrough
			case 6:       PROCESS4; PROCESS1; PROCESS1; return XXH32_avalanche(h32);
			case 15:      PROCESS4; // @fallthrough
			case 11:      PROCESS4; // @fallthrough
			case 7:       PROCESS4; // @fallthrough
			case 3:       PROCESS1; // @fallthrough
			case 2:       PROCESS1; // @fallthrough
			case 1:       PROCESS1; // @fallthrough
			case 0: return XXH32_avalanche(h32);
		}
		assert(0);
		return h32; /* reaching this point is deemed impossible */
	}
}

static FORCEINLINE uint32 XXH32_endian_align(const void * input, size_t len, uint32 seed, XXH_alignment align)
{
	const BYTE * p = static_cast<const BYTE *>(input);
	const BYTE * bEnd = p + len;
	uint32 h32;
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
	if(!p) {
		len = 0;
		bEnd = p = (const BYTE *)(size_t)16;
	}
#endif
	if(len >= 16) {
		const BYTE * const limit = bEnd - 15;
		uint32 v1 = seed + PRIME32_1 + PRIME32_2;
		uint32 v2 = seed + PRIME32_2;
		uint32 v3 = seed + 0;
		uint32 v4 = seed - PRIME32_1;
		do {
			v1 = XXH32_round(v1, XXH_get32bits(p)); p += 4;
			v2 = XXH32_round(v2, XXH_get32bits(p)); p += 4;
			v3 = XXH32_round(v3, XXH_get32bits(p)); p += 4;
			v4 = XXH32_round(v4, XXH_get32bits(p)); p += 4;
		} while(p < limit);
		h32 = SBits::Rotl(v1, 1)  + SBits::Rotl(v2, 7) + SBits::Rotl(v3, 12) + SBits::Rotl(v4, 18);
	}
	else {
		h32  = seed + PRIME32_5;
	}
	h32 += (uint32)len;
	return XXH32_finalize(h32, p, len&15, align);
}

XXH_PUBLIC_API uint32 XXH32(const void * input, size_t len, uint seed)
{
#if 0
	/* Simple version, good for code maintenance, but unfortunately slow for small inputs */
	XXH32_state_t state;
	XXH32_reset(&state, seed);
	XXH32_update(&state, input, len);
	return XXH32_digest(&state);

#else
	if(XXH_FORCE_ALIGN_CHECK) {
		if((((size_t)input) & 3) == 0) { // Input is 4-bytes aligned, leverage the speed benefit 
			return XXH32_endian_align(input, len, seed, XXH_aligned);
		}
	}
	return XXH32_endian_align(input, len, seed, XXH_unaligned);
#endif
}
//
// Hash streaming
//
XXH_PUBLIC_API XXH32_state_t* XXH32_createState()
{
	return static_cast<XXH32_state_t *>(SAlloc::M(sizeof(XXH32_state_t)));
}

XXH_PUBLIC_API XXH_errorcode XXH32_freeState(XXH32_state_t* statePtr)
{
	SAlloc::F(statePtr);
	return XXH_OK;
}

XXH_PUBLIC_API void XXH32_copyState(XXH32_state_t* dstState, const XXH32_state_t* srcState)
{
	memcpy(dstState, srcState, sizeof(*dstState));
}

XXH_PUBLIC_API XXH_errorcode XXH32_reset(XXH32_state_t* statePtr, uint seed)
{
	XXH32_state_t state; /* using a local state to memcpy() in order to avoid strict-aliasing warnings */
	memzero(&state, sizeof(state));
	state.v1 = seed + PRIME32_1 + PRIME32_2;
	state.v2 = seed + PRIME32_2;
	state.v3 = seed + 0;
	state.v4 = seed - PRIME32_1;
	// do not write into reserved, planned to be removed in a future version 
	memcpy(statePtr, &state, sizeof(state) - sizeof(state.reserved));
	return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode XXH32_update(XXH32_state_t* state, const void * input, size_t len)
{
	if(input==NULL)
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
		return XXH_OK;
#else
		return XXH_ERROR;
#endif
	{   
		const BYTE * p = (const BYTE *)input;
	    const BYTE * const bEnd = p + len;
	    state->total_len_32 += (uint32)len;
	    state->large_len |= (uint32)((len>=16) | (state->total_len_32>=16));
	    if(state->memsize + len < 16) { /* fill in tmp buffer */
		    memcpy((BYTE *)(state->mem32) + state->memsize, input, len);
		    state->memsize += (uint32)len;
		    return XXH_OK;
	    }
	    if(state->memsize) { /* some data left from previous update */
		    memcpy((BYTE *)(state->mem32) + state->memsize, input, 16-state->memsize);
		    {   
				const uint32 * p32 = state->mem32;
				state->v1 = XXH32_round(state->v1, XXH_readLE32(p32)); p32++;
				state->v2 = XXH32_round(state->v2, XXH_readLE32(p32)); p32++;
				state->v3 = XXH32_round(state->v3, XXH_readLE32(p32)); p32++;
				state->v4 = XXH32_round(state->v4, XXH_readLE32(p32));
			}
		    p += 16-state->memsize;
		    state->memsize = 0;
	    }
	    if(p <= bEnd-16) {
		    const BYTE * const limit = bEnd - 16;
		    uint32 v1 = state->v1;
		    uint32 v2 = state->v2;
		    uint32 v3 = state->v3;
		    uint32 v4 = state->v4;
		    do {
			    v1 = XXH32_round(v1, XXH_readLE32(p)); p += 4;
			    v2 = XXH32_round(v2, XXH_readLE32(p)); p += 4;
			    v3 = XXH32_round(v3, XXH_readLE32(p)); p += 4;
			    v4 = XXH32_round(v4, XXH_readLE32(p)); p += 4;
		    } while(p<=limit);
		    state->v1 = v1;
		    state->v2 = v2;
		    state->v3 = v3;
		    state->v4 = v4;
	    }
	    if(p < bEnd) {
		    memcpy(state->mem32, p, (size_t)(bEnd-p));
		    state->memsize = (uint)(bEnd-p);
	    }
	}
	return XXH_OK;
}

XXH_PUBLIC_API uint32 XXH32_digest(const XXH32_state_t* state)
{
	uint32 h32;
	if(state->large_len) {
		h32 = SBits::Rotl(state->v1, 1) + SBits::Rotl(state->v2, 7) + SBits::Rotl(state->v3, 12) + SBits::Rotl(state->v4, 18);
	}
	else {
		h32 = state->v3 /* == seed */ + PRIME32_5;
	}
	h32 += state->total_len_32;
	return XXH32_finalize(h32, state->mem32, state->memsize, XXH_aligned);
}

/*======   Canonical representation   ======*/

/*! Default XXH result types are basic unsigned 32 and 64 bits.
 *   The canonical representation follows human-readable write convention, aka big-endian (large digits first).
 *   These functions allow transformation of hash result into and from its canonical format.
 *   This way, hash values can be written into a file or buffer, remaining comparable across different systems.
 */

XXH_PUBLIC_API void XXH32_canonicalFromHash(XXH32_canonical_t* dst, uint32 hash)
{
	STATIC_ASSERT(sizeof(XXH32_canonical_t) == sizeof(uint32));
	if(XXH_CPU_LITTLE_ENDIAN) 
		hash = sbswap32(hash);
	memcpy(dst, &hash, sizeof(*dst));
}

XXH_PUBLIC_API uint32 XXH32_hashFromCanonical(const XXH32_canonical_t* src)
{
	return XXH_readBE32(src);
}

#ifndef XXH_NO_LONG_LONG
//
// 64-bit hash functions
//
//
// Memory access
//
#ifndef MEM_MODULE
	#define MEM_MODULE
	#if !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
		//#include <stdint.h>
		typedef uint64_t U64_Removed;
	#else
		// if compiler doesn't support unsigned long long, replace by another 64-bit type 
		typedef unsigned long long U64_Removed;
	#endif
#endif

/*! XXH_REROLL_XXH64:
 * Whether to reroll the XXH64_finalize() loop.
 *
 * Just like XXH32, we can unroll the XXH64_finalize() loop. This can be a performance gain
 * on 64-bit hosts, as only one jump is required.
 *
 * However, on 32-bit hosts, because arithmetic needs to be done with two 32-bit registers,
 * and 64-bit arithmetic needs to be simulated, it isn't beneficial to unroll. The code becomes
 * ridiculously large (the largest function in the binary on i386!), and rerolling it saves
 * anywhere from 3kB to 20kB. It is also slightly faster because it fits into cache better
 * and is more likely to be inlined by the compiler.
 *
 * If XXH_REROLL is defined, this is ignored and the loop is always rerolled. */
#ifndef XXH_REROLL_XXH64
	#if(defined(__ILP32__) || defined(_ILP32)) /* ILP32 is often defined on 32-bit GCC family */ \
		|| !(defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) /* x86-64 */ \
		|| defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__) /* aarch64 */ \
		|| defined(__PPC64__) || defined(__PPC64LE__) || defined(__ppc64__) || defined(__powerpc64__) /* ppc64 */ \
		|| defined(__mips64__) || defined(__mips64)) /* mips64 */ \
		|| (!defined(SIZE_MAX) || SIZE_MAX < ULLONG_MAX) /* check limits */
		#define XXH_REROLL_XXH64 1
	#else
		#define XXH_REROLL_XXH64 0
	#endif
#endif /* !defined(XXH_REROLL_XXH64) */

#if(defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))
	// Force direct memory access. Only works on CPU which support unaligned memory access in hardware 
	static uint64 XXH_read64(const void * memPtr) { return *(const uint64*)memPtr; }
#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))
	// __pack instructions are safer, but compiler specific, hence potentially problematic for some compilers 
	// currently only defined for gcc and icc 
	typedef union { uint32 u32; uint64 u64; } __attribute__((packed)) unalign64;
	static uint64 XXH_read64(const void * ptr) 
	{
		return ((const unalign64*)ptr)->u64;
	}
#else
	// 
	// portable and safe solution. Generally efficient.
	// see : http://stackoverflow.com/a/32095106/646947
	// 
	static uint64 XXH_read64(const void * memPtr)
	{
		uint64 val;
		memcpy(&val, memPtr, sizeof(val));
		return val;
	}
#endif
#if defined(_MSC_VER)     /* Visual Studio */
	#define XXH_swap64 _byteswap_uint64
#elif XXH_GCC_VERSION >= 403
	#define XXH_swap64 __builtin_bswap64
#else
	static uint64 XXH_swap64(uint64 x)
	{
		return ((x << 56) & 0xff00000000000000ULL) | ((x << 40) & 0x00ff000000000000ULL) |
		   ((x << 24) & 0x0000ff0000000000ULL) | ((x << 8)  & 0x000000ff00000000ULL) |
		   ((x >> 8)  & 0x00000000ff000000ULL) | ((x >> 24) & 0x0000000000ff0000ULL) |
		   ((x >> 40) & 0x000000000000ff00ULL) | ((x >> 56) & 0x00000000000000ffULL);
	}
#endif

static FORCEINLINE uint64 XXH_readLE64(const void * ptr)
{
	return XXH_CPU_LITTLE_ENDIAN ? XXH_read64(ptr) : XXH_swap64(XXH_read64(ptr));
}

static uint64 XXH_readBE64(const void * ptr)
{
	return XXH_CPU_LITTLE_ENDIAN ? XXH_swap64(XXH_read64(ptr)) : XXH_read64(ptr);
}

static FORCEINLINE uint64 XXH_readLE64_align(const void * ptr, XXH_alignment align)
{
	if(align==XXH_unaligned)
		return XXH_readLE64(ptr);
	else
		return XXH_CPU_LITTLE_ENDIAN ? *(const uint64*)ptr : XXH_swap64(*(const uint64*)ptr);
}
//
// xxh64
//
static const uint64 PRIME64_1 = 0x9E3779B185EBCA87ULL; // 0b1001111000110111011110011011000110000101111010111100101010000111
static const uint64 PRIME64_2 = 0xC2B2AE3D27D4EB4FULL; // 0b1100001010110010101011100011110100100111110101001110101101001111
static const uint64 PRIME64_3 = 0x165667B19E3779F9ULL; // 0b0001011001010110011001111011000110011110001101110111100111111001
static const uint64 PRIME64_4 = 0x85EBCA77C2B2AE63ULL; // 0b1000010111101011110010100111011111000010101100101010111001100011
static const uint64 PRIME64_5 = 0x27D4EB2F165667C5ULL; // 0b0010011111010100111010110010111100010110010101100110011111000101 

static uint64 XXH64_round(uint64 acc, uint64 input)
{
	acc += input * PRIME64_2;
	acc  = SBits::Rotl(acc, 31);
	acc *= PRIME64_1;
	return acc;
}

static uint64 XXH64_mergeRound(uint64 acc, uint64 val)
{
	val  = XXH64_round(0, val);
	acc ^= val;
	acc  = acc * PRIME64_1 + PRIME64_4;
	return acc;
}

static uint64 XXH64_avalanche(uint64 h64)
{
	h64 ^= h64 >> 33;
	h64 *= PRIME64_2;
	h64 ^= h64 >> 29;
	h64 *= PRIME64_3;
	h64 ^= h64 >> 32;
	return h64;
}

#define XXH_get64bits(p) XXH_readLE64_align(p, align)

static uint64 XXH64_finalize(uint64 h64, const void * ptr, size_t len, XXH_alignment align)
{
	const BYTE * p = static_cast<const BYTE *>(ptr);

#define PROCESS1_64 h64 ^= (*p++) * PRIME64_5; h64 = SBits::Rotl(h64, 11) * PRIME64_1;
#define PROCESS4_64 h64 ^= (uint64)(XXH_get32bits(p)) * PRIME64_1; p += 4; h64 = SBits::Rotl(h64, 23) * PRIME64_2 + PRIME64_3;
#define PROCESS8_64 { uint64 const k1 = XXH64_round(0, XXH_get64bits(p)); p += 8; h64 ^= k1; h64  = SBits::Rotl(h64, 27) * PRIME64_1 + PRIME64_4; }

	/* Rerolled version for 32-bit targets is faster and much smaller. */
	if(XXH_REROLL || XXH_REROLL_XXH64) {
		len &= 31;
		while(len >= 8) {
			PROCESS8_64;
			len -= 8;
		}
		if(len >= 4) {
			PROCESS4_64;
			len -= 4;
		}
		while(len > 0) {
			PROCESS1_64;
			--len;
		}
		return XXH64_avalanche(h64);
	}
	else {
		switch(len & 31) {
			case 24: PROCESS8_64; // @fallthrough
			case 16: PROCESS8_64; // @fallthrough
			case  8: PROCESS8_64; return XXH64_avalanche(h64);
			case 28: PROCESS8_64; // @fallthrough
			case 20: PROCESS8_64; // @fallthrough
			case 12: PROCESS8_64; // @fallthrough
			case  4: PROCESS4_64; return XXH64_avalanche(h64);
			case 25: PROCESS8_64; // @fallthrough
			case 17: PROCESS8_64; // @fallthrough
			case  9: PROCESS8_64; PROCESS1_64; return XXH64_avalanche(h64);
			case 29: PROCESS8_64; // @fallthrough
			case 21: PROCESS8_64; // @fallthrough
			case 13: PROCESS8_64; // @fallthrough
			case  5: PROCESS4_64; PROCESS1_64; return XXH64_avalanche(h64);
			case 26: PROCESS8_64; // @fallthrough
			case 18: PROCESS8_64; // @fallthrough
			case 10: PROCESS8_64; PROCESS1_64; PROCESS1_64; return XXH64_avalanche(h64);
			case 30: PROCESS8_64; // @fallthrough
			case 22: PROCESS8_64; // @fallthrough
			case 14: PROCESS8_64; // @fallthrough
			case  6: PROCESS4_64; PROCESS1_64; PROCESS1_64; return XXH64_avalanche(h64);
			case 27: PROCESS8_64; // @fallthrough
			case 19: PROCESS8_64; // @fallthrough
			case 11: PROCESS8_64; PROCESS1_64; PROCESS1_64; PROCESS1_64; return XXH64_avalanche(h64);
			case 31: PROCESS8_64; // @fallthrough
			case 23: PROCESS8_64; // @fallthrough
			case 15: PROCESS8_64; // @fallthrough
			case  7: PROCESS4_64; // @fallthrough
			case  3: PROCESS1_64; // @fallthrough
			case  2: PROCESS1_64; // @fallthrough
			case  1: PROCESS1_64; // @fallthrough
			case  0: return XXH64_avalanche(h64);
		}
	}
	assert(0); // impossible to reach 
	return 0; // unreachable, but some compilers complain without it 
}

static FORCEINLINE uint64 XXH64_endian_align(const void * input, size_t len, uint64 seed, XXH_alignment align)
{
	const BYTE * p = (const BYTE *)input;
	const BYTE * bEnd = p + len;
	uint64 h64;
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
	if(!p) {
		len = 0;
		bEnd = p = (const BYTE *)(size_t)32;
	}
#endif
	if(len>=32) {
		const BYTE * const limit = bEnd - 32;
		uint64 v1 = seed + PRIME64_1 + PRIME64_2;
		uint64 v2 = seed + PRIME64_2;
		uint64 v3 = seed + 0;
		uint64 v4 = seed - PRIME64_1;
		do {
			v1 = XXH64_round(v1, XXH_get64bits(p)); p += 8;
			v2 = XXH64_round(v2, XXH_get64bits(p)); p += 8;
			v3 = XXH64_round(v3, XXH_get64bits(p)); p += 8;
			v4 = XXH64_round(v4, XXH_get64bits(p)); p += 8;
		} while(p<=limit);
		h64 = SBits::Rotl(v1, 1) + SBits::Rotl(v2, 7) + SBits::Rotl(v3, 12) + SBits::Rotl(v4, 18);
		h64 = XXH64_mergeRound(h64, v1);
		h64 = XXH64_mergeRound(h64, v2);
		h64 = XXH64_mergeRound(h64, v3);
		h64 = XXH64_mergeRound(h64, v4);
	}
	else {
		h64  = seed + PRIME64_5;
	}
	h64 += (uint64)len;
	return XXH64_finalize(h64, p, len, align);
}

XXH_PUBLIC_API uint64 XXH64(const void * input, size_t len, uint64 seed)
{
#if 0
	/* Simple version, good for code maintenance, but unfortunately slow for small inputs */
	XXH64_state_t state;
	XXH64_reset(&state, seed);
	XXH64_update(&state, input, len);
	return XXH64_digest(&state);
#else
	if(XXH_FORCE_ALIGN_CHECK) {
		if((((size_t)input) & 7)==0) { /* Input is aligned, let's leverage the speed advantage */
			return XXH64_endian_align(input, len, seed, XXH_aligned);
		}
	}
	return XXH64_endian_align(input, len, seed, XXH_unaligned);
#endif
}
//
// Hash Streaming
//
XXH_PUBLIC_API XXH64_state_t* XXH64_createState(void)
{
	return static_cast<XXH64_state_t *>(SAlloc::M(sizeof(XXH64_state_t)));
}

XXH_PUBLIC_API XXH_errorcode XXH64_freeState(XXH64_state_t* statePtr)
{
	SAlloc::F(statePtr);
	return XXH_OK;
}

XXH_PUBLIC_API void XXH64_copyState(XXH64_state_t* dstState, const XXH64_state_t* srcState)
{
	memcpy(dstState, srcState, sizeof(*dstState));
}

XXH_PUBLIC_API XXH_errorcode XXH64_reset(XXH64_state_t* statePtr, uint64 seed)
{
	XXH64_state_t state; /* using a local state to memcpy() in order to avoid strict-aliasing warnings */
	memzero(&state, sizeof(state));
	state.v1 = seed + PRIME64_1 + PRIME64_2;
	state.v2 = seed + PRIME64_2;
	state.v3 = seed + 0;
	state.v4 = seed - PRIME64_1;
	/* do not write into reserved64, might be removed in a future version */
	memcpy(statePtr, &state, sizeof(state) - sizeof(state.reserved64));
	return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode XXH64_update(XXH64_state_t* state, const void * input, size_t len)
{
	if(input==NULL)
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
		return XXH_OK;
#else
		return XXH_ERROR;
#endif
	{   
		const BYTE * p = (const BYTE *)input;
	    const BYTE * const bEnd = p + len;
	    state->total_len += len;
	    if(state->memsize + len < 32) { /* fill in tmp buffer */
		    memcpy(((BYTE *)state->mem64) + state->memsize, input, len);
		    state->memsize += (uint32)len;
		    return XXH_OK;
	    }
	    if(state->memsize) { /* tmp buffer is full */
		    memcpy(((BYTE *)state->mem64) + state->memsize, input, 32-state->memsize);
		    state->v1 = XXH64_round(state->v1, XXH_readLE64(state->mem64+0));
		    state->v2 = XXH64_round(state->v2, XXH_readLE64(state->mem64+1));
		    state->v3 = XXH64_round(state->v3, XXH_readLE64(state->mem64+2));
		    state->v4 = XXH64_round(state->v4, XXH_readLE64(state->mem64+3));
		    p += 32-state->memsize;
		    state->memsize = 0;
	    }
	    if(p+32 <= bEnd) {
		    const BYTE * const limit = bEnd - 32;
		    uint64 v1 = state->v1;
		    uint64 v2 = state->v2;
		    uint64 v3 = state->v3;
		    uint64 v4 = state->v4;

		    do {
			    v1 = XXH64_round(v1, XXH_readLE64(p)); p += 8;
			    v2 = XXH64_round(v2, XXH_readLE64(p)); p += 8;
			    v3 = XXH64_round(v3, XXH_readLE64(p)); p += 8;
			    v4 = XXH64_round(v4, XXH_readLE64(p)); p += 8;
		    } while(p<=limit);

		    state->v1 = v1;
		    state->v2 = v2;
		    state->v3 = v3;
		    state->v4 = v4;
	    }
	    if(p < bEnd) {
		    memcpy(state->mem64, p, (size_t)(bEnd-p));
		    state->memsize = (uint)(bEnd-p);
	    }
	}
	return XXH_OK;
}

XXH_PUBLIC_API uint64 XXH64_digest(const XXH64_state_t* state)
{
	uint64 h64;
	if(state->total_len >= 32) {
		uint64 const v1 = state->v1;
		uint64 const v2 = state->v2;
		uint64 const v3 = state->v3;
		uint64 const v4 = state->v4;
		h64 = SBits::Rotl(v1, 1) + SBits::Rotl(v2, 7) + SBits::Rotl(v3, 12) + SBits::Rotl(v4, 18);
		h64 = XXH64_mergeRound(h64, v1);
		h64 = XXH64_mergeRound(h64, v2);
		h64 = XXH64_mergeRound(h64, v3);
		h64 = XXH64_mergeRound(h64, v4);
	}
	else {
		h64  = state->v3 /*seed*/ + PRIME64_5;
	}
	h64 += (uint64)state->total_len;
	return XXH64_finalize(h64, state->mem64, (size_t)state->total_len, XXH_aligned);
}

/*====== Canonical representation   ======*/

XXH_PUBLIC_API void XXH64_canonicalFromHash(XXH64_canonical_t* dst, uint64 hash)
{
	STATIC_ASSERT(sizeof(XXH64_canonical_t) == sizeof(uint64));
	if(XXH_CPU_LITTLE_ENDIAN) 
		hash = XXH_swap64(hash);
	memcpy(dst, &hash, sizeof(*dst));
}

XXH_PUBLIC_API uint64 XXH64_hashFromCanonical(const XXH64_canonical_t* src)
{
	return XXH_readBE64(src);
}
// 
// XXH3
// New generation hash designed for speed on small keys and vectorization
// 
#include "xxh3.h"

#endif  /* XXH_NO_LONG_LONG */
