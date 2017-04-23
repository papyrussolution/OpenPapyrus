/*[
 * Copyright 1999, 2004   Hans-Bernhard Broeker
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
   ]*/

/* This module implements a dynamically growing array of arbitrary
 * elements parametrized by their sizeof(). There is no 'access
 * function', i.e. you'll have to access the elements of the
 * dynarray->v memory block by hand. It's implemented in OO-style,
 * even though this is C, not C++.  In particular, every function
 * takes a pointer to a data structure type 'dynarray', which mimics
 * the 'this' pointer of an object. */
#include <gnuplot.h>
#pragma hdrstop

static const char * init_failure_msg = "dynarray wasn't initialized";

// The 'constructor' of a dynarray object: initializes all the
// variables to well-defined startup values
/*void init_dynarray(dynarray * pThis, size_t entry_size, long size, long increment)
{
	pThis->v = 0; //  preset value, in case malloc fails 
	if(size)
		pThis->v = malloc(entry_size*size, "init dynarray");
	pThis->size = size;
	pThis->end = 0;
	pThis->increment = increment;
	pThis->entry_size = entry_size;
}*/

void dynarray::Init(size_t entrySize, long sz, long incr)
//void init_dynarray(dynarray * pThis, size_t entrySize, long sz, long incr)
{
	v = 0; //  preset value, in case malloc fails 
	if(sz)
		v = malloc(entrySize*sz);
	size = sz;
	end = 0;
	increment = incr;
	entry_size = entrySize;
}

// The 'destructor'; sets all crucial elements of the structure to
// well-defined values to avoid problems by use of bad pointers...
/*void free_dynarray(dynarray * pThis)
{
	free(pThis->v);
	pThis->v = 0;
	pThis->end = pThis->size = 0;
}*/

// Set the size of the dynamical array to a new, fixed value 
void dynarray::Resize(long newsize)
{
	if(!v)
		GpGg.IntErrorNoCaret(init_failure_msg);
	if(newsize == 0)
		Destroy();
	else {
		v = gp_realloc(v, entry_size * newsize, "extend dynarray");
		size = newsize;
	}
}

// Increase the size of the dynarray by a given amount
/*void extend_dynarray(dynarray * pThis, long incr)
{
	pThis->Resize(pThis->size + incr);
}*/

// Get pointer to the element one past the current end of the dynamic
// array. Resize it if necessary. Returns a pointer-to-void to that element.
//void * nextfrom_dynarray(dynarray * pThis)
void * dynarray::GetNext()
{
	if(!v)
		GpGg.IntErrorNoCaret(init_failure_msg);
	if(end >= size) {
		Resize(size + increment);
	}
	return (void*)((char*)(v) + entry_size * (end++));
}

// Release the element at the current end of the dynamic array, by
// moving the 'end' index one element backwards 
void droplast_dynarray(dynarray * pThis)
{
	if(!pThis->v)
		GpGg.IntErrorNoCaret(init_failure_msg);
	if(pThis->end)
		pThis->end--;
}

