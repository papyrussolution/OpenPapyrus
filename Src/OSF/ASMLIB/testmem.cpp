//          TESTMEM.CPP                                   Agner Fog 2011-07-04

// Test file for asmlib memcpy and memmove functions
// Instructions: Compile on any platform and link with the appropriate
// version of the asmlib library.

#include <stdio.h>
//#include <process.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "asmlib.h"

// define function type
typedef void * memcpyF (void * dest, const void * src, size_t count); 
typedef void * memsetF (void * dest, int c, size_t count);


extern "C" {
    extern int IInstrSet;
    // function prototypes for CPU specific function versions
    memcpyF memcpy386, memcpySSE2, memcpySSSE3, memcpyU, memcpyU256;
    memcpyF memmove386, memmoveSSE2, memmoveSSSE3, memmoveU, memmoveU256;
    memsetF memset386, memsetSSE2, memsetAVX;
}

// Tables of function pointers
#if defined(_WIN64) || defined(_M_X64) || defined(__amd64)
const int NUMFUNC = 5;
memcpyF * memcpyTab[NUMFUNC] = {A_memcpy, memcpySSE2, memcpySSSE3, memcpyU, memcpyU256};
memcpyF * memmoveTab[NUMFUNC] = {A_memmove, memmoveSSE2, memmoveSSSE3, memmoveU, memmoveU256};
const char * DispatchNames[NUMFUNC] = {"Dispatched", "SSE2", "SSSE3", "Unalign", "U256"};
int isetreq [NUMFUNC] = {0, 4, 6, 4, 11};  // instruction set required
const int MEMSETFUNCS = 3;
memsetF * memsetTab[MEMSETFUNCS] = {A_memset, memsetSSE2, memsetAVX};
const char * memsetNames[MEMSETFUNCS] = {"Dispatched", "SSE2", "AVX"};
int memsetreq [NUMFUNC] = {0, 4, 11};  // instruction set required
#else
const int NUMFUNC = 6;
memcpyF * memcpyTab[NUMFUNC] = {A_memcpy, memcpy386, memcpySSE2, memcpySSSE3, memcpyU, memcpyU256};
memcpyF * memmoveTab[NUMFUNC] = {A_memmove, memmove386, memmoveSSE2, memmoveSSSE3, memmoveU, memmoveU256};
const char * DispatchNames[NUMFUNC] = {"Dispatched", "386", "SSE2", "SSSE3", "Unalign", "U256"};
int isetreq [NUMFUNC] = {0, 0, 4, 6, 4, 11};  // instruction set required
const int MEMSETFUNCS = 4;
memsetF * memsetTab[MEMSETFUNCS] = {A_memset, memset386, memsetSSE2, memsetAVX};
const char * memsetNames[MEMSETFUNCS] = {"Dispatched", "386", "SSE2", "AVX"};
int memsetreq [NUMFUNC] = {0, 0, 4, 11};  // instruction set required
#endif



void error(const char * s, int a, int b, int c) {
    printf("\nError %s: %i %i %i\n", s, a, b, c);
    exit (1);
}

void error(const char * s, int i, int a, int b, int c) {
    printf("\nError %s: %i %i %i %i\n", s, i, a, b, c);
    exit (1);
}

int main () {

    int ao, bo, os, len;
    int version;
    const int pagesize = 0x1000;  // 4 kbytes
    const int n = 16*pagesize;
    char a[n], b[n], c[n];
    int instrset = InstructionSet();

    // CacheBypassLimit = 5;
    printf("\nmemcpy cache limit = 0x%X, memset cache limit 0x%X\n", 
        (int)GetMemcpyCacheLimit(), (int)GetMemsetCacheLimit());

    printf("\nTest memcpy");

    int i, x = 91;
    for (i=0; i<n; i++) {
        x += 23;
        a[i] = (char)x;
    }

    A_memset(b, -1, n);

    SetMemcpyCacheLimit(0);  // default

#if 1 
    // Test memcpy for correctness
    // Loop through versions
    for (version = 0; version < NUMFUNC; version++) {

        printf("\n%s", DispatchNames[version]);
        if (instrset < isetreq[version]) {
            // instruction set not supported
            printf(" skipped"); continue;
        }

        for (len=0; len<514; len++) {
            for (ao = 0; ao <=20; ao++) {
                for (bo = 0; bo <=32; bo++) {
                    A_memset(b, -1, len+96);
                    (*memcpyTab[version])(b+bo, a+ao, len);
                    if (bo && b[bo-1] != -1) error("A", ao, bo, len);
                    if (b[bo+len] != -1) error("B", ao, bo, len);
                    if (len==0) continue;
                    if (b[bo] != a[ao]) error("C", ao, bo, len);
                    if (b[bo+len-1] != a[ao+len-1]) error("D", ao, bo, len);
                    if (memcmp(b+bo, a+ao, len)) error("E", ao, bo, len);
                }
            }
        }
        // check false memory dependence branches
        len = 300;
        A_memcpy(b, a, 3*pagesize);
        for (ao = pagesize-300; ao < pagesize+200; ao++) {
            for (bo = 3*pagesize; bo <=3*pagesize+33; bo++) {
                A_memset(b+bo-64, -1, len+128);
                (*memcpyTab[version])(b+bo, b+ao, len);
                if (b[bo-1] != -1) error("A1", ao, bo, len);
                if (b[bo+len] != -1) error("B1", ao, bo, len);
                if (memcmp(b+bo, b+ao, len)) error("E1", ao, bo, len);
            }
        }
        // check false memory dependence branches with overlap
        // src > dest and overlap: must copy forwards
        len = pagesize+1000;
        for (ao = 2*pagesize; ao <=2*pagesize+33; ao++) {
            for (bo = pagesize-200; bo < pagesize+300; bo++) {
                A_memcpy(b, a, 4*pagesize);
                A_memcpy(c, a, 4*pagesize);
                (*memcpyTab[version])(b+bo, b+ao, len);
                //memcpy(c+bo, c+ao, len);  // Most library versions of memcpy are actually memmove
                memcpySSE2(c+bo, c+ao, len);            
                if (memcmp(b, c, 4*pagesize)) {
                    error("E2", ao-pagesize, bo-2*pagesize, len);
                }
            }
        }
        // check false memory dependence branches with overlap
        // dest > src and overlap: undefined behavior
#if 1
        len = pagesize+1000;
        for (ao = pagesize-200; ao < pagesize+200; ao++) {
            for (bo = 2*pagesize; bo <=2*pagesize+33; bo++) {
                A_memcpy(b, a, 4*pagesize);
                A_memcpy(c, a, 4*pagesize);
                (*memcpyTab[version])(b+bo, b+ao, len);
                //memcpy(c+bo, c+ao, len);  // MS Most library versions of memcpy are actually memmove
                memcpySSE2(c+bo, c+ao, len);
                if (memcmp(b, c, 4*pagesize)) {
                    error("E3", ao-pagesize, bo-2*pagesize, len);
                }
            }
        }
#endif
    }
    printf("\n\nTest memmove");

    // Test memmove for correctness
    for (i=0; i<n; i++) {
        x += 23;
        a[i] = char(x);
    }

    // Loop through versions
    for (version = 0; version < NUMFUNC; version++) {
        printf("\n%s", DispatchNames[version]);
        if (instrset < isetreq[version]) {
            // instruction set not supported
            printf(" skipped"); continue;
        }

        // move forward
        for (len=0; len<400; len++) {
            for (bo = 0; bo <=33; bo++) {
                for (os = 0; os <= 33; os++) {
                    A_memcpy(b, a, len+100);
                    (*memmoveTab[version])(b+bo+os, b+bo, len);
                    for (i=0; i<bo+os; i++) if (b[i]!=a[i]) error("E", i, bo, os, len);
                    for (i=bo+os; i<bo+os+len; i++) if (b[i] != a[i-os]) error("F", i, bo, os, len);
                    for (;i < bo+os+len+20; i++) if (b[i]!=a[i]) error("G", i, bo, os, len);
                }
            }
        }
        // move backwards
        for (len=0; len<400; len++) {
            for (bo = 0; bo <=33; bo++) {
                for (os = 0; os < 33; os++) {
                    A_memcpy(b, a, len+96);
                    (*memmoveTab[version])(b+bo, b+bo+os, len);
                    for (i=0; i<bo; i++) if (b[i]!=a[i]) error("H", i, bo, os, len);
                    for (i=bo; i<bo+len; i++) if (b[i] != a[i+os]) error("I", i, bo, os, len);
                    for (;i < bo+len+20; i++) if (b[i]!=a[i]) error("J", i, bo, os, len);
                }
            }
        }
    }

    printf("\n\nSame, with non-temporal moves");
    SetMemcpyCacheLimit(1); // bypass cache

    // Loop through versions
    for (version = 0; version < NUMFUNC; version++) {

        printf("\n%s", DispatchNames[version]);
        if (instrset < isetreq[version]) {
            // instruction set not supported
            printf(" skipped"); continue;
        }

        for (len=0; len<514; len++) {
            for (ao = 0; ao <=20; ao++) {
                for (bo = 0; bo <=32; bo++) {
                    A_memset(b, -1, len+96);
                    (*memcpyTab[version])(b+bo, a+ao, len);
                    if (bo && b[bo-1] != -1) error("A", ao, bo, len);
                    if (b[bo+len] != -1) error("B", ao, bo, len);
                    if (len==0) continue;
                    if (b[bo] != a[ao]) error("C", ao, bo, len);
                    if (b[bo+len-1] != a[ao+len-1]) error("D", ao, bo, len);
                    if (memcmp(b+bo, a+ao, len)) error("E", ao, bo, len);
                }
            }
        }
        // check false memory dependence branches
        len = 300;
        A_memcpy(b, a, 3*pagesize);
        for (ao = pagesize-200; ao < pagesize+200; ao++) {
            for (bo = 3*pagesize; bo <=3*pagesize+33; bo++) {
                A_memset(b+bo-64, -1, len+128);
                (*memcpyTab[version])(b+bo, b+ao, len);
                if (b[bo-1] != -1) error("A1", ao, bo, len);
                if (b[bo+len] != -1) error("B1", ao, bo, len);
                if (memcmp(b+bo, b+ao, len)) error("E1", ao, bo, len);
            }
        }
        // check false memory dependence branches with overlap
        // src > dest and overlap: must copy forwards
        len = pagesize+1000;
        for (ao = 2*pagesize; ao <=2*pagesize+33; ao++) {
            for (bo = pagesize-200; bo < pagesize+200; bo++) {
                A_memcpy(b, a, 4*pagesize);
                A_memcpy(c, a, 4*pagesize);
                (*memcpyTab[version])(b+bo, b+ao, len);
                //memcpy(c+bo, c+ao, len);  // Most library versions of memcpy are actually memmove
                memcpySSE2(c+bo, c+ao, len);            
                if (memcmp(b, c, 4*pagesize)) {
                    error("E2", ao-pagesize, bo-2*pagesize, len);
                }
            }
        }
        // (check false memory dependence branches with overlap. skipped)
    }
    printf("\n\nTest memmove");

    // Test memmove for correctness
    for (i=0; i<n; i++) {
        x += 23;
        a[i] = char(x);
    }

    // Loop through versions
    for (version = 0; version < NUMFUNC; version++) {
        printf("\n%s", DispatchNames[version]);
        if (instrset < isetreq[version]) {
            // instruction set not supported
            printf(" skipped"); continue;
        }

        // move forward
        for (len=0; len<400; len++) {
            for (bo = 0; bo <=33; bo++) {
                for (os = 0; os <= 33; os++) {
                    A_memcpy(b, a, len+100);
                    (*memmoveTab[version])(b+bo+os, b+bo, len);
                    for (i=0; i<bo+os; i++) if (b[i]!=a[i]) error("E", i, bo, os, len);
                    for (i=bo+os; i<bo+os+len; i++) if (b[i] != a[i-os]) error("F", i, bo, os, len);
                    for (;i < bo+os+len+20; i++) if (b[i]!=a[i]) error("G", i, bo, os, len);
                }
            }
        }
        // move backwards
        for (len=0; len<400; len++) {
            for (bo = 0; bo <=33; bo++) {
                for (os = 0; os < 33; os++) {
                    A_memcpy(b, a, len+96);
                    (*memmoveTab[version])(b+bo, b+bo+os, len);
                    for (i=0; i<bo; i++) if (b[i]!=a[i]) error("H", i, bo, os, len);
                    for (i=bo; i<bo+len; i++) if (b[i] != a[i+os]) error("I", i, bo, os, len);
                    for (;i < bo+len+20; i++) if (b[i]!=a[i]) error("J", i, bo, os, len);
                }
            }
        }
    }
#endif
    SetMemcpyCacheLimit(0);  // back to default
    SetMemsetCacheLimit(0);

    printf("\n\nTest memset");

    // test memset
    const int val1 = 0x4C, val2 = 0xA2, len2 = 1024;
    for (version = 0; version < MEMSETFUNCS; version++) {
        memsetF * func = memsetTab[version];
        printf("\n%s", memsetNames[version]);
        if (instrset < memsetreq[version]) {
            // instruction set not supported
            printf(" skipped"); continue;
        }
        for (os = 0; os < 34; os++) {
            for (len = 0; len < 500; len++) {
                memset(a, val1, len2);
                memset(a+os, val2, len);
                (*func)(b, val1, len2);
                (*func)(b+os, val2, len);
                if (memcmp(a, b, len2)) {
                    error("MS", version, os, len);
                }
            }
        }
        for (len=0; len<200; len++) {
            for (os = 0; os <= 33; os++) {
                A_memcpy(b, a, len+64);
                A_memset(b+os, 55, len);
                for (i=0; i<os; i++) if (b[i] != a[i]) error("K", i, os, len);
                for (; i<os+len; i++) if (b[i] != 55) error("L", i, os, len);
                for (; i<os+len+17; i++) if (b[i] != a[i]) error("M", i, os, len);
            }
        }
    }

    printf("\n\nSame, with non-temporal moves");
    SetMemsetCacheLimit(1);   // bypass cache

    for (version = 0; version < MEMSETFUNCS; version++) {
        memsetF * func = memsetTab[version];
        printf("\n%s", memsetNames[version]);
        if (instrset < memsetreq[version]) {
            // instruction set not supported
            printf(" skipped"); continue;
        }
        for (os = 0; os < 34; os++) {
            for (len = 0; len < 500; len++) {
                memset(a, val1, len2);
                memset(a+os, val2, len);
                (*func)(b, val1, len2);
                (*func)(b+os, val2, len);
                if (memcmp(a, b, len2)) {
                    error("MS", version, os, len);
                }
            }
        }
    }
    SetMemsetCacheLimit(0);   // back to default

    printf("\n\nTest strlen");

    // test strlen
    for (len=0; len<400; len++) {
        for (os = 0; os <= 32; os++) {
            A_memset(b, 0, len+64);
            A_memset(b+os, 'a', len);
            x = A_strlen(b+os);
            if (x != len) error("N", 0, os, len);
            A_memset(b, 1, len+64);
            b[os+len] = 0;
            x = A_strlen(b+os);
            if (x != len) error("O", 0, os, len);
        }
    }

    printf("\n\nTest strcpy and strcat");

    // test strcpy and strcat
    for (i=0; i<n; i++) {
        x += 23;
        a[i] = char(x) | 1;
    }
    for (len=0; len<400; len++) {
        for (os = 0; os <= 16; os++) {
            for (i=0; i<33; i++) {
                A_memmove(b, a, len+64);
                b[os+len] = 0;
                A_strcpy(c+5, b+os);
                if (A_strlen(c+5) != len) error("P", 0, os, len);
                A_memmove(b+55, a, i+4);
                b[55+i] = 0;
                A_strcat(c+5, b+55);
                if (A_strlen(c+5) != len+i) error("R", 0, os, len);
            }
        }
    }
    printf("\n\nSuccess\n");

    return 0;
}
