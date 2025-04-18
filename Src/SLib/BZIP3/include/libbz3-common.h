/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _COMMON_H
#define _COMMON_H

//#define KiB(x) ((x)*1024)
//#define MiB(x) ((x)*1024 * 1024)
#define BWT_BOUND(x) ((x) + 128)

//#include <inttypes.h>
//#include <stdint.h>

static int32 read_neutral_s32(const uint8 * data) 
{
    return ((uint32)data[0]) | (((uint32)data[1]) << 8) | (((uint32)data[2]) << 16) | (((uint32)data[3]) << 24);
}

static void write_neutral_s32(uint8 * data, int32 value) 
{
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
}

//#if defined(__GNUC__) || defined(__clang__)
    //#define RESTRICT __restrict__
//#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
    //#define RESTRICT __restrict
//#else
    //#define RESTRICT restrict
    //#warning Your compiler, configuration or platform might not be supported.
//#endif

#if defined(__has_builtin)
    #if __has_builtin(__builtin_prefetch)
        #define HAS_BUILTIN_PREFETCH
    #endif
#elif defined(__GNUC__) && (((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2)) || (__GNUC__ >= 4))
    #define HAS_BUILTIN_PREFETCH
#endif

//#if defined(__has_builtin)
    //#if __has_builtin(__builtin_bswap16)
        //#define HAS_BUILTIN_BSWAP16
    //#endif
//#elif defined(__GNUC__) && (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)) || (__GNUC__ >= 5))
    //#define HAS_BUILTIN_BSWAP16
//#endif

#if defined(HAS_BUILTIN_PREFETCH)
    #define prefetch(address) __builtin_prefetch((const void *)(address), 0, 0)
    #define prefetchw(address) __builtin_prefetch((const void *)(address), 1, 0)
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__x86_64__) || defined(i386) || defined(__i386__) || \
    defined(__i386)
    #include <intrin.h>
    #define prefetch(address) _mm_prefetch((const char *)(address), _MM_HINT_NTA)
    #define prefetchw(address) _m_prefetchw((const void *)(address))
#elif defined(_M_ARM) || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || \
    defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    #include <intrin.h>
    #define prefetch(address) __prefetch((const void *)(address))
    #define prefetchw(address) __prefetchw((const void *)(address))
#elif defined(_M_ARM64) || defined(__aarch64__)
    #include <intrin.h>
    #define prefetch(address) __prefetch2((const void *)(address), 1)
    #define prefetchw(address) __prefetch2((const void *)(address), 17)
#else
    #error Your compiler, configuration or platform is not supported.
#endif
#endif
