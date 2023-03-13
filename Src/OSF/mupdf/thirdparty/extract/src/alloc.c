//
//
#include <slib.h>
#include "alloc.h"
#include "memento.h"
#include <assert.h>
#include <stdlib.h>

extract_alloc_info_t extract_alloc_info = {0};

static size_t s_exp_min_alloc_size = 0;

static size_t round_up(size_t n)
{
	if(s_exp_min_alloc_size) {
		/* Round up to power of two. */
		size_t ret;
		if(n==0) return 0;
		ret = s_exp_min_alloc_size;
		for(;;) {
			size_t ret_old;
			if(ret >= n) return ret;
			ret_old = ret;
			ret *= 2;
			assert(ret > ret_old);
			(void)ret_old;
		}
	}
	else {
		return n;
	}
}

int(extract_malloc)(void** pptr, size_t size)
{
	void* p;
	size = round_up(size);
	extract_alloc_info.num_malloc += 1;
	p = malloc(size);
	*pptr = p;
	if(!p) return -1;
	return 0;
}

int(extract_realloc)(void** pptr, size_t newsize)
{
	void* p = realloc(*pptr, newsize);
	if(!p) return -1;
	*pptr = p;
	return 0;
}

int(extract_realloc2)(void** pptr, size_t oldsize, size_t newsize)
{
	/* We ignore <oldsize> if <ptr> is NULL - allows callers to not worry about
	   edge cases e.g. with strlen+1. */
	oldsize = (*pptr) ? round_up(oldsize) : 0;
	newsize = round_up(newsize);
	extract_alloc_info.num_realloc += 1;
	if(newsize == oldsize) return 0;
	return (extract_realloc)(pptr, newsize);
}

void(extract_free)(void** pptr)
{
	extract_alloc_info.num_free += 1;
	free(*pptr);
	*pptr = NULL;
}

void extract_alloc_exp_min(size_t size)
{
	s_exp_min_alloc_size = size;
}
