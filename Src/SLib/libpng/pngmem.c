
/* pngmem.c - stub functions for memory allocation
 *
 * Last changed in libpng 1.6.15 [November 20, 2014]
 * Copyright (c) 1998-2002,2004,2006-2014 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * This file provides a location for all memory allocation.  Users who
 * need special memory handling are expected to supply replacement
 * functions for png_malloc() and png_free(), and to use
 * png_create_read_struct_2() and png_create_write_struct_2() to
 * identify the replacement functions.
 */
#include <slib-internal.h>
#pragma hdrstop
#include "libpng\pngpriv.h"

#if defined(PNG_READ_SUPPORTED) || defined(PNG_WRITE_SUPPORTED)
/* Free a png_struct */
void /*PRIVATE*/ png_destroy_png_struct(png_structrp png_ptr)
{
	if(png_ptr) {
		/* png_free might call png_error and may certainly call
		 * png_get_mem_ptr, so fake a temporary png_struct to support this.
		 */
		png_struct dummy_struct = *png_ptr;
		memzero(png_ptr, (sizeof *png_ptr));
		png_free(&dummy_struct, png_ptr);

#ifdef PNG_SETJMP_SUPPORTED
		png_free_jmpbuf(&dummy_struct); // We may have a jmp_buf left to deallocate.
#endif
	}
}
// 
// Allocate memory.  For reasonable files, size should never exceed 64K.  
// However, zlib may allocate more than 64K if you don't tell it not to.  
// See zconf.h and png.h for more information.  zlib does
// need to allocate exactly 64K, so whatever you call here must have the ability to do that.
// 
PNG_ALLOCATED void * PNGAPI png_calloc(png_const_structrp png_ptr, png_alloc_size_t size)
{
	void * ret = png_malloc(png_ptr, size);
	/*if(ret)
		memzero(ret, size);
	return ret;*/
	return memzero(ret, size);
}
/* png_malloc_base, an internal function added at libpng 1.6.0, does the work of
 * allocating memory, taking into account limits and PNG_USER_MEM_SUPPORTED.
 * Checking and error handling must happen outside this routine; it returns NULL
 * if the allocation cannot be done (for any reason.)
 */
PNG_ALLOCATED void * /*PRIVATE*/ png_malloc_base(png_const_structrp png_ptr, png_alloc_size_t size)
{
	/* Moved to png_malloc_base from png_malloc_default in 1.6.0; the DOS
	 * allocators have also been removed in 1.6.0, so any 16-bit system now has
	 * to implement a user memory handler.  This checks to be sure it isn't
	 * called with big numbers.
	 */
#ifndef PNG_USER_MEM_SUPPORTED
	CXX_UNUSED(png_ptr);
#endif
	/* Some compilers complain that this is always true.  However, it
	 * can be false when integer overflow happens.
	 */
	if(size > 0 && size <= PNG_SIZE_MAX
#ifdef PNG_MAX_MALLOC_64K
	 && size <= 65536U
#endif
	    ) {
#ifdef PNG_USER_MEM_SUPPORTED
		if(png_ptr && png_ptr->malloc_fn)
			return png_ptr->malloc_fn(png_constcast(png_structrp, png_ptr), size);
		else
#endif
		return SAlloc::M((size_t)size); /* checked for truncation above */
	}
	else
		return NULL;
}

#if defined(PNG_TEXT_SUPPORTED) || defined(PNG_sPLT_SUPPORTED) || defined(PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED)
/* This is really here only to work round a spurious warning in GCC 4.6 and 4.7
 * that arises because of the checks in png_realloc_array that are repeated in
 * png_malloc_array.
 */
static void * png_malloc_array_checked(png_const_structrp png_ptr, int nelements, size_t element_size)
{
	png_alloc_size_t req = (png_alloc_size_t)nelements; // known to be > 0 
	if(req <= PNG_SIZE_MAX/element_size)
		return png_malloc_base(png_ptr, req * element_size);
	return NULL; // The failure case when the request is too large 
}

PNG_ALLOCATED void * /*PRIVATE*/ png_malloc_array(png_const_structrp png_ptr, int nelements, size_t element_size)
{
	if(nelements <= 0 || element_size == 0)
		png_error(png_ptr, "internal error: array alloc");
	return png_malloc_array_checked(png_ptr, nelements, element_size);
}

PNG_ALLOCATED void * /*PRIVATE*/ png_realloc_array(png_const_structrp png_ptr, const void * old_array, int old_elements, int add_elements, size_t element_size)
{
	/* These are internal errors: */
	if(add_elements <= 0 || element_size == 0 || old_elements < 0 || (old_array == NULL && old_elements > 0))
		png_error(png_ptr, "internal error: array realloc");
	// Check for overflow on the elements count (so the caller does not have to check.)
	if(add_elements <= INT_MAX - old_elements) {
		void * new_array = png_malloc_array_checked(png_ptr, old_elements+add_elements, element_size);
		if(new_array) {
			// Because png_malloc_array worked the size calculations below cannot overflow.
			if(old_elements > 0)
				memcpy(new_array, old_array, element_size*(uint)old_elements);
			memzero((char *)new_array + element_size*(uint)old_elements, element_size*(uint)add_elements);
			return new_array;
		}
	}
	return NULL; /* error */
}
#endif /* TEXT || sPLT || STORE_UNKNOWN_CHUNKS */

/* Various functions that have different error handling are derived from this.
 * png_malloc always exists, but if PNG_USER_MEM_SUPPORTED is defined a separate
 * function png_malloc_default is also provided.
 */
PNG_ALLOCATED void * PNGAPI png_malloc(png_const_structrp png_ptr, png_alloc_size_t size)
{
	void * ret = 0;
	if(png_ptr) {
		ret = png_malloc_base(png_ptr, size);
		if(!ret)
			png_error(png_ptr, SlTxtOutOfMem); /* 'm' means png_malloc */
	}
	return ret;
}

#ifdef PNG_USER_MEM_SUPPORTED
PNG_ALLOCATED PNG_DEPRECATED void * PNGAPI png_malloc_default(png_const_structrp png_ptr, png_alloc_size_t size)
{
	void * ret = 0;
	if(png_ptr) {
		// Passing 'NULL' here bypasses the application provided memory handler. 
		ret = png_malloc_base(NULL /*use SAlloc::M*/, size);
		if(!ret)
			png_error(png_ptr, SlTxtOutOfMem); // 'M' means png_malloc_default 
	}
	return ret;
}
#endif /* USER_MEM */

/* This function was added at libpng version 1.2.3.  The png_malloc_warn()
 * function will issue a png_warning and return NULL instead of issuing a
 * png_error, if it fails to allocate the requested memory.
 */
PNG_ALLOCATED void * PNGAPI png_malloc_warn(png_const_structrp png_ptr, png_alloc_size_t size)
{
	if(png_ptr) {
		void * ret = png_malloc_base(png_ptr, size);
		if(ret)
			return ret;
		png_warning(png_ptr, SlTxtOutOfMem);
	}
	return NULL;
}

/* Free a pointer allocated by png_malloc().  If ptr is NULL, return
 * without taking any action.
 */
void PNGAPI png_free(png_const_structrp png_ptr, void * ptr)
{
	if(png_ptr == NULL || ptr == NULL)
		return;
#ifdef PNG_USER_MEM_SUPPORTED
	if(png_ptr->free_fn)
		png_ptr->free_fn(png_constcast(png_structrp, png_ptr), ptr);
	else
		png_free_default(png_ptr, ptr);
}

PNG_DEPRECATED void PNGAPI png_free_default(png_const_structrp png_ptr, void * ptr)
{
	if(png_ptr == NULL || ptr == NULL)
		return;
#endif /* USER_MEM */
	SAlloc::F(ptr);
}

#ifdef PNG_USER_MEM_SUPPORTED
/* This function is called when the application wants to use another method
 * of allocating and freeing memory.
 */
void PNGAPI png_set_mem_fn(png_structrp png_ptr, void * mem_ptr, png_malloc_ptr malloc_fn, png_free_ptr free_fn)
{
	if(png_ptr) {
		png_ptr->mem_ptr = mem_ptr;
		png_ptr->malloc_fn = malloc_fn;
		png_ptr->free_fn = free_fn;
	}
}

/* This function returns a pointer to the mem_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before png_write_destroy and png_read_destroy are called.
 */
void * PNGAPI png_get_mem_ptr(png_const_structrp png_ptr)
{
	return png_ptr ? png_ptr->mem_ptr : 0;
}

#endif /* USER_MEM */
#endif /* READ || WRITE */
