#ifndef EXTRACT_ALLOC_H
#define EXTRACT_ALLOC_H

#include <stdlib.h>

int extract_malloc(void** pptr, size_t size);
/* Sets *pptr to point to new buffer and returns 0. On error return -1 with
errno set and *pptr=NULL. */

int extract_realloc(void** pptr, size_t newsize);
/* Sets *pptr to point to reallocated buffer and returns 0. On error return -1
with errno set and *pptr unchanged (pointing to the existing buffer). */

void extract_free(void** pptr);
/* Frees block pointed to by *pptr and sets *pptr to NULL. */

#define extract_malloc(pptr, size) (extract_malloc)((void**) pptr, size)
#define extract_realloc(pptr, newsize) (extract_realloc)((void**) pptr, newsize)
#define extract_free(pptr) (extract_free)((void**) pptr)
/* These allow callers to use any pointer type, not just void*. */

typedef struct
{
    int num_malloc;
    int num_realloc;
    int num_free;
    int num_libc_realloc;
} extract_alloc_info_t;

extern extract_alloc_info_t extract_alloc_info;

int extract_realloc2(void** pptr, size_t oldsize, size_t newsize);
/* A realloc variant that takes the existing buffer size.

If <oldsize> is not zero and *pptr is not NULL, <oldsize> must be the size of
the existing buffer and may used internally to over-allocate in order to avoid
too many calls to realloc(). See extract_alloc_exp_min() for more information.
*/

#define extract_realloc2(pptr, oldsize, newsize) (extract_realloc2)((void**) pptr, oldsize, newsize)

void extract_alloc_exp_min(size_t size);
/* If size is non-zero, sets minimum actual allocation size, and we only
allocate in powers of two times this size. This is an attempt to improve speed
with memento squeeze. Default is 0 (every call to extract_realloc() calls
realloc(). */

#endif
