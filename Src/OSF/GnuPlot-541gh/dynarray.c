// DYNARRAY.C
// Copyright 1999, 2004   Hans-Bernhard Broeker
//
#include <gnuplot.h>
#pragma hdrstop
// 
// This module implements a dynamically growing array of arbitrary
// elements parametrized by their sizeof(). There is no 'access
// function', i.e. you'll have to access the elements of the
// dynarray->v memory block by hand. It's implemented in OO-style,
// even though this is C, not C++.  In particular, every function
// takes a pointer to a data structure type 'dynarray', which mimics
// the 'this' pointer of an object. 
// 
static char * init_failure_msg = "dynarray wasn't initialized";
// 
// The 'constructor' of a dynarray object: initializes all the
// variables to well-defined startup values 
// 
void init_dynarray(dynarray * pThis, size_t entry_size, long size, long increment)
{
	pThis->v = 0;            /* preset value, in case gp_alloc fails */
	if(size)
		pThis->v = gp_alloc(entry_size*size, "init dynarray");
	pThis->size = size;
	pThis->end = 0;
	pThis->increment = increment;
	pThis->entry_size = entry_size;
}
// 
// The 'destructor'; sets all crucial elements of the structure to
// well-defined values to avoid problems by use of bad pointers... 
// 
void free_dynarray(dynarray * pThis)
{
	ZFREE(pThis->v); // should work, even if gp_alloc failed 
	pThis->end = pThis->size = 0;
}
// 
// Set the size of the dynamical array to a new, fixed value 
// 
void resize_dynarray(dynarray * pThis, long newsize)
{
	if(!pThis->v)
		GPO.IntError(NO_CARET, init_failure_msg);
	if(newsize == 0)
		free_dynarray(pThis);
	else {
		pThis->v = gp_realloc(pThis->v, pThis->entry_size * newsize, "extend dynarray");
		pThis->size = newsize;
	}
}
// 
// Increase the size of the dynarray by a given amount 
// 
void extend_dynarray(dynarray * pThis, long increment)
{
	resize_dynarray(pThis, pThis->size + increment);
}
// 
// Get pointer to the element one past the current end of the dynamic
// array. Resize it if necessary. Returns a pointer-to-void to that element. 
// 
void * nextfrom_dynarray(dynarray * pThis)
{
	if(!pThis->v)
		GPO.IntError(NO_CARET, init_failure_msg);
	if(pThis->end >= pThis->size)
		extend_dynarray(pThis, pThis->increment);
	return ((char*)(pThis->v) + pThis->entry_size * (pThis->end++));
}
// 
// Release the element at the current end of the dynamic array, by
// moving the 'end' index one element backwards 
// 
void droplast_dynarray(dynarray * pThis)
{
	if(!pThis->v)
		GPO.IntError(NO_CARET, init_failure_msg);
	if(pThis->end)
		pThis->end--;
}
