//          TESTMEM.CPP                                   Agner Fog 2016-11-12

// Test file for asmlib memcpy, memmove, memset, and memcmp functions
// Instructions: Compile on any platform and link with the appropriate
// version of the asmlib library.

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "asmlib.h"


#if 0    // thorough testing
const int step1 = 1;
const int step2 = 1;
const int step3 = 1;
#else    // fast testing
const int step1 = 11;
const int step2 = 3;
const int step3 = 5;
#endif

// Define which tests to do
const int memcpytest  = 2;   // 0: not test, 1: test cached, 2: test cached and uncached
const int memmovetest = 2;   // 0: not test, 1: test cached, 2: test cached and uncached
const int memsettest  = 2;   // 0: not test, 1: test cached, 2: test cached and uncached
const int memcmptest  = 1;   // 0: not test, 1: test

// define function types
typedef void * memcpyF(void * dest, const void * src, size_t count);
typedef void * memsetF(void * dest, int c, size_t count);
typedef int    memcmpF(const void * ptr1, const void * ptr2, size_t num);

// define reference functions to compare with
#define refmemcpy  memcpy      // memcpy
#define refmemmove memmove     // memmove
#define refmemset  memset      // memset
#define refmemcmp  memcmp      // memcmp

extern "C" {
    extern int IInstrSet;
    // function prototypes for CPU specific function versions
    memcpyF memcpy386, memcpySSE2, memcpySSSE3, memcpyU, memcpyU256, memcpyAVX512F, memcpyAVX512BW;
    memcpyF memmove386, memmoveSSE2, memmoveSSSE3, memmoveU, memmoveU256, memmoveAVX512F, memmoveAVX512BW;
    memsetF memset386, memsetSSE2, memsetAVX, memsetAVX512F, memsetAVX512BW;
    memcmpF memcmp386, memcmpSSE2, memcmpAVX2, memcmpAVX512F, memcmpAVX512BW;
}

// Tables of function pointers, names, and required instruction sets
#if defined(_WIN64) || defined(_M_X64) || defined(__amd64)  // 64 bit mode
const int NUMMEMCPYFUNC = 7;
memcpyF * memcpyTab[NUMMEMCPYFUNC] = { A_memcpy, memcpySSE2, memcpySSSE3, memcpyU, memcpyU256, memcpyAVX512F, memcpyAVX512BW };
const char * memcpyNames[NUMMEMCPYFUNC] = { "Dispatched", "SSE2", "SSSE3", "Unalign", "U256", "AVX512F", "AVX512BW" };
int isetmemcpy[NUMMEMCPYFUNC] = { 0, 4, 6, 4, 11, 15, 16 };  // instruction set required

const int NUMMEMMOVEFUNC = 7;
memcpyF * memmoveTab[NUMMEMMOVEFUNC] = { A_memmove, memmoveSSE2, memmoveSSSE3, memmoveU, memmoveU256, memmoveAVX512F, memmoveAVX512BW };
const char * memmoveNames[NUMMEMMOVEFUNC] = { "Dispatched", "SSE2", "SSSE3", "Unalign", "U256", "AVX512F", "AVX512BW" };
int isetmemmove[NUMMEMMOVEFUNC] = { 0, 4, 6, 4, 11, 15, 16 };  // instruction set required

const int NUMMEMSETFUNC = 5;
memsetF * memsetTab[NUMMEMSETFUNC] = { A_memset, memsetSSE2, memsetAVX, memsetAVX512F, memsetAVX512BW };
const char * memsetNames[NUMMEMSETFUNC] = { "Dispatched", "SSE2", "AVX", "AVX512F" , "AVX512BW" };
int isetmemset[NUMMEMSETFUNC] = { 0, 4, 11, 15, 16 };  // instruction set required

const int NUMMEMCMPFUNC = 5;
memcmpF * memcmpTab[NUMMEMCMPFUNC] = { A_memcmp, memcmpSSE2, memcmpAVX2, memcmpAVX512F, memcmpAVX512BW };
const char * memcmpNames[NUMMEMCMPFUNC] = { "Dispatched", "SSE2", "AVX2", "AVX512F" , "AVX512BW" };
int isetmemcmp[NUMMEMCMPFUNC] = { 0, 4, 12, 15, 16 };  // instruction set required

#else        // 32 bit mode
const int NUMMEMCPYFUNC = 8;
memcpyF * memcpyTab[NUMMEMCPYFUNC] = { A_memcpy, memcpy386, memcpySSE2, memcpySSSE3, memcpyU, memcpyU256, memcpyAVX512F, memcpyAVX512BW };
const char * memcpyNames[NUMMEMCPYFUNC] = { "Dispatched", "386", "SSE2", "SSSE3", "Unalign", "U256", "AVX512F", "AVX512BW" };
int isetmemcpy[NUMMEMCPYFUNC] = { 0, 0, 4, 6, 4, 11, 15, 16 };  // instruction set required

const int NUMMEMMOVEFUNC = 8;
memcpyF * memmoveTab[NUMMEMMOVEFUNC] = { A_memmove, memmove386, memmoveSSE2, memmoveSSSE3, memmoveU, memmoveU256, memmoveAVX512F, memmoveAVX512BW };
const char * memmoveNames[NUMMEMMOVEFUNC] = { "Dispatched", "386", "SSE2", "SSSE3", "Unalign", "U256", "AVX512F", "AVX512BW" };
int isetmemmove[NUMMEMMOVEFUNC] = { 0, 0, 4, 6, 4, 11, 15, 16 };  // instruction set required

const int NUMMEMSETFUNC = 6;
memsetF * memsetTab[NUMMEMSETFUNC] = { A_memset, memset386, memsetSSE2, memsetAVX, memsetAVX512F, memsetAVX512BW };
const char * memsetNames[NUMMEMSETFUNC] = { "Dispatched", "386", "SSE2", "AVX", "AVX512F", "AVX512BW" };
int isetmemset[NUMMEMSETFUNC] = { 0, 0, 4, 11, 15, 16 };  // instruction set required

const int NUMMEMCMPFUNC = 6;
memcmpF * memcmpTab[NUMMEMCMPFUNC] = { A_memcmp, memcmp386, memcmpSSE2, memcmpAVX2, memcmpAVX512F, memcmpAVX512BW };
const char * memcmpNames[NUMMEMCMPFUNC] = { "Dispatched", "386", "SSE2", "AVX2", "AVX512F" , "AVX512BW" };
int isetmemcmp[NUMMEMCMPFUNC] = { 0, 0, 4, 12, 15, 16 };  // instruction set required

#endif

// error reporting functions
void error(const char * s, int a, int b, int c) {
    printf("\nError %s: %i %i %i\n", s, a, b, c);
    exit (1);
}

void error(const char * s, int i, int a, int b, int c) {
    printf("\nError %s: %i %i %i %i\n", s, i, a, b, c);
    exit (1);
}

// main
int main () {
    // define variables and arrays
    int ao, bo, os, len;
    int version;
    int cached;
    const int pagesize = 0x1000;  // memory page size: 4 kbytes
    const int n = 16 * pagesize;
    char a[n], b[n], c[n];
    int instrset = InstructionSet();
    int i, x = 91;
    // initialize arrays
    for (i = 0; i<n; i++) {
        x += 23;   a[i] = (char)x;
    }
    refmemset(b, -1, n);

    // print calculated cache limit
    SetMemcpyCacheLimit(0);  // default
    printf("\nmemcpy cache limit = 0x%X, memset cache limit 0x%X\n",
        (int)GetMemcpyCacheLimit(), (int)GetMemsetCacheLimit());

    printf("\nTest memory functions\n%i bit mode", (int)(sizeof(void*))*8);


    // Test memcpy for correctness
    if (memcpytest) {
        printf("\n\nTest memcpy\n%i bit mode\n", (int)(sizeof(void*)) * 8);
    }

    for (cached = 0; cached < memcpytest; cached++) {
        SetMemcpyCacheLimit(cached);

        // Loop through versions
        for (version = 0; version < NUMMEMCPYFUNC; version++) {

            printf("\n%s, %s", memcpyNames[version], cached ? "non-temporal" : "cached");
            if (instrset < isetmemcpy[version]) {
                // instruction set not supported
                printf(" skipped"); continue;
            }
            for (len = 0; len < 514; len += step1) {
                for (ao = 0x80; ao <= 0xC5; ao += step2) {
                    refmemset(b, -1, len + ao + 0x100);
                    refmemset(c, -1, len + ao + 0x100);
                    refmemcpy(b + ao, a, len);
                    (*memcpyTab[version])(c + ao, a, len);
                    if (memcmp(b, c, len + ao + 0x80)) error("memcpy fail:", ao, bo, len);
                }
            }
        }
    }

    // Test memmove for correctness
    if (memmovetest) {
        printf("\n\nTest memmove\n%i bit mode\n", (int)(sizeof(void*)) * 8);
    }
    for (cached = 0; cached < memmovetest; cached++) {
        SetMemcpyCacheLimit(cached);

        // Loop through versions
        for (version = 0; version < NUMMEMMOVEFUNC; version++) {

            printf("\n%s, %s", memmoveNames[version], cached ? "non-temporal" : "cached");
            if (instrset < isetmemmove[version]) {
                // instruction set not supported
                printf(" skipped"); continue;
            }
            for (len = 0; len < 514; len += step1) {
                for (ao = 0x80; ao <= 0xC5; ao += step2) {
                    for (bo = -0x40; bo <= 0x41; bo += step3) {
                        refmemcpy(b, a, len + ao + 0x100);
                        refmemcpy(c, a, len + ao + 0x100);
                        refmemmove(b + ao, b + ao + bo, len);
                        (*memmoveTab[version])(c + ao, c + ao + bo, len);
                        if (memcmp(b, c, len + ao + 0x80)) error("memmove fail", ao, bo, len);
                    }
                }
            }
        }
    }

    // Test memset for correctness
    if (memsettest) {
        printf("\n\nTest memset\n%i bit mode\n", (int)(sizeof(void*)) * 8);
    }

    for (cached = 0; cached < memsettest; cached++) {
        SetMemsetCacheLimit(cached);

        // Loop through versions
        for (version = 0; version < NUMMEMSETFUNC; version++) {

            printf("\n%s, %s", memsetNames[version], cached ? "non-temporal" : "cached");
            if (instrset < isetmemset[version]) {
                // instruction set not supported
                printf(" skipped"); continue;
            }
            for (len = 0; len < 514; len += step1) {
                for (ao = 0x80; ao <= 0xC5; ao += step2) {
                    refmemset(b, -1, len + ao + 0x100);
                    refmemset(c, -1, len + ao + 0x100);
                    refmemset(b + ao, 85, len);
                    (*memsetTab[version])(c + ao, 85, len);
                    if (memcmp(b, c, len + ao + 0x80)) error("A", ao, bo, len);
                }
            }
        }
    }

    // Test memcmp for correctness
    if (memcmptest) {
        printf("\n\nTest memcmp\n%i bit mode\n", (int)(sizeof(void*)) * 8);

        // Loop through versions
        for (version = 0; version < NUMMEMCMPFUNC; version++) {

            printf("\n%s", memcmpNames[version]);
            if (instrset < isetmemcmp[version]) {
                // instruction set not supported
                printf(" skipped"); continue;
            }
            char dif = 0;
            int  pos = 0;
            int  res1, res2;
            for (len = 0; len < 514; len += step1) {
                for (ao = 0x80; ao <= 0xC5; ao += step2) {
                    refmemcpy(b, a, len + ao + 0x100);
                    refmemcpy(c, a, len + ao + 0x100);
                    dif += 64;  // will wrap around
                    pos += 5; if (pos > len) pos = 0;
                    if (len) c[ao + pos] += dif;
                    res1 = refmemcmp(b + ao, c + ao, len);
                    res2 = (*memcmpTab[version])(b + ao, c + ao, len);
                    if (res1 != res2) {
                        if ((res1 < 0 && res2 >= 0) || (res1 > 0 && res2 <= 0) || (res1 == 0 && res2 != 0)) {
                            error("memcmp", len, ao, res1, res2);
                        }
                    }
                }
            }
        }
    }

    printf("\n\nTest strlen");

    // test strlen
    for (len=0; len<400; len+= step1) {
        for (os = 0; os <= 32; os+= step2) {
            refmemset(b, 0, len+64);
            refmemset(b+os, 'a', len);
            x = (int)A_strlen(b+os);
            if (x != len) error("strlen fail:", 0, os, len);
            refmemset(b, 1, len+64);
            b[os+len] = 0;
            x = (int)A_strlen(b+os);
            if (x != len) error("strlen fail:", 0, os, len);
        }
    }

    printf("\n\nTest strcpy and strcat");

    // test strcpy and strcat
    for (i=0; i<n; i++) {
        x += 23;
        a[i] = char(x) | 1;
    }
    for (len=0; len<400; len+= step1) {
        for (os = 0; os <= 16; os+= step2) {
            for (i=0; i<33; i++) {
                refmemmove(b, a, len+64);
                b[os+len] = 0;
                A_strcpy(c+5, b+os);
                if (A_strlen(c+5) != len) error("strcpy fail:", 0, os, len);
                refmemmove(b+55, a, i+4);
                b[55+i] = 0;
                A_strcat(c+5, b+55);
                if (A_strlen(c+5) != len+i) error("strcat fail:", 0, os, len);
            }
        }
    }
    printf("\n\nSuccess\n");

    return 0;
}
