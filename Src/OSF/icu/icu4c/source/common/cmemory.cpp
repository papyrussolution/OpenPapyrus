// cmemory.c
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2002-2015, International Business Machines Corporation and others.  All Rights Reserved.
// ICU Heap allocation.
// All ICU heap allocation, both for C and C++ new of ICU
// class types, comes through these functions.
// 
// If you have a need to replace ICU allocation, this is the place to do it.
// 
// Note that uprv_malloc(0) returns a non-NULL pointer, and that a subsequent free of that pointer value is a NOP.
// 
#include <icu-internal.h>
#pragma hdrstop

static const int32_t zeroMem[] = {0, 0, 0, 0, 0, 0}; /* uprv_malloc(0) returns a pointer to this read-only data. */
/* Function Pointers for user-supplied heap functions  */
static const void   * pContext;
static UMemAllocFn    * pAlloc;
static UMemReallocFn  * pRealloc;
static UMemFreeFn     * pFree;

#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
static int n = 0;
static long b = 0;
#endif

U_CAPI void * U_EXPORT2 uprv_malloc(size_t s) 
{
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
#if 1
	putchar('>');
	fflush(stdout);
#else
	slfprintf_stderr("MALLOC\t#%d\t%ul bytes\t%ul total\n", ++n, s, (b += s)); fflush(stderr);
#endif
#endif
	if(s > 0) {
		return pAlloc ? (*pAlloc)(pContext, s) : SAlloc::M(s);
	}
	else {
		return (void *)zeroMem;
	}
}

U_CAPI void * U_EXPORT2 uprv_realloc(void * buffer, size_t size) 
{
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
	putchar('~');
	fflush(stdout);
#endif
	if(buffer == zeroMem) {
		return uprv_malloc(size);
	}
	else if(!size) {
		if(pFree) {
			(*pFree)(pContext, buffer);
		}
		else {
			SAlloc::F(buffer);
		}
		return (void *)zeroMem;
	}
	else {
		return pRealloc ? (*pRealloc)(pContext, buffer, size) : SAlloc::R(buffer, size);
	}
}

U_CAPI void U_EXPORT2 uprv_free(void * buffer) 
{
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
	putchar('<');
	fflush(stdout);
#endif
	if(buffer != zeroMem) {
		if(pFree) {
			(*pFree)(pContext, buffer);
		}
		else {
			SAlloc::F(buffer);
		}
	}
}

U_CAPI void * U_EXPORT2 uprv_calloc(size_t num, size_t size) 
{
	size *= num;
	void * mem = uprv_malloc(size);
	memzero(mem, size);
	return mem;
}

U_CAPI void U_EXPORT2 u_setMemoryFunctions(const void * context, UMemAllocFn * a, UMemReallocFn * r, UMemFreeFn * f,  UErrorCode * status)
{
	if(U_SUCCESS(*status)) {
		if(!a || !r || !f) {
			*status = U_ILLEGAL_ARGUMENT_ERROR;
		}
		else {
			pContext  = context;
			pAlloc    = a;
			pRealloc  = r;
			pFree     = f;
		}
	}
}

U_CFUNC bool cmemory_cleanup() 
{
	pContext   = NULL;
	pAlloc     = NULL;
	pRealloc   = NULL;
	pFree      = NULL;
	return TRUE;
}
