/* xmalloc.c -- malloc with out of memory checking

   Copyright (C) 1990-2000, 2002-2006, 2008-2011 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <flexbison_common.h>
#pragma hdrstop
#if !HAVE_INLINE
	#define static_inline
#endif
#undef static_inline

/* 1 if calloc is known to be compatible with GNU calloc.  This
   matters if we are not also using the calloc module, which defines
   HAVE_CALLOC_GNU and supports the GNU API even on non-GNU platforms.  */
#if defined HAVE_CALLOC_GNU || (defined __GLIBC__ && !defined __UCLIBC__)
	enum { HAVE_GNU_CALLOC = 1 };
#else
	enum { HAVE_GNU_CALLOC = 0 };
#endif
//
// Allocate N bytes of memory dynamically, with error checking.
//
void * xmalloc(size_t n)
{
	void * p = SAlloc::M(n);
	if(!p && n != 0)
		xalloc_die();
	memzero(p, n);
	return p;
}
// 
// Change the size of an allocated block of memory P to N bytes, with error checking.
// 
void * xrealloc(void * p, size_t n)
{
	if(!n && p) {
		// The GNU and C99 realloc behaviors disagree here.  Act like GNU, even if the underlying realloc is C99.
		SAlloc::F(p);
		return NULL;
	}
	else {
		p = SAlloc::R(p, n);
		if(!p && n)
			xalloc_die();
		return p;
	}
}

/* If P is null, allocate a block of at least *PN bytes; otherwise,
   reallocate P so that it contains more than *PN bytes.  *PN must be
   nonzero unless P is null.  Set *PN to the new block's size, and
   return the pointer to the new block.  *PN is never set to zero, and
   the returned pointer is never null.  */

void * x2realloc(void * p, size_t * pn)
{
	return x2nrealloc(p, pn, 1);
}
// 
// Allocate S bytes of zeroed memory dynamically, with error checking.
// There's no need for xnzalloc (N, S), since it would be equivalent to xcalloc (N, S).
// 
void * xzalloc(size_t s)
{
	return memzero(xmalloc(s), s);
}
// 
// Allocate zeroed memory for N elements of S bytes, with error checking.  S must be nonzero.
// 
void * xcalloc(size_t n, size_t s)
{
	void * p;
	/* Test for overflow, since some calloc implementations don't have
	   proper overflow checks.  But omit overflow and size-zero tests if
	   HAVE_GNU_CALLOC, since GNU calloc catches overflow and never
	   returns NULL if successful.  */
	if((!HAVE_GNU_CALLOC && xalloc_oversized(n, s)) || (!(p = SAlloc::C(n, s)) && (HAVE_GNU_CALLOC || n != 0)))
		xalloc_die();
	return p;
}
//
// Allocate an array of N objects, each with S bytes of memory, dynamically, with error checking.  S must be nonzero.
//
void * xnmalloc(size_t n, size_t s)
{
	if(xalloc_oversized(n, s))
		xalloc_die();
	return xmalloc(n * s);
}
//
// Change the size of an allocated block of memory P to an array of N
// objects each of S bytes, with error checking.  S must be nonzero.
//
void * xnrealloc(void * p, size_t n, size_t s)
{
	if(xalloc_oversized(n, s))
		xalloc_die();
	return xrealloc(p, n * s);
}
// 
// If P is null, allocate a block of at least *PN such objects;
// otherwise, reallocate P so that it contains more than *PN objects
// each of S bytes.  *PN must be nonzero unless P is null, and S must
// be nonzero.  Set *PN to the new number of objects, and return the
// pointer to the new block.  *PN is never set to zero, and the
// returned pointer is never null.
// 
// Repeated reallocations are guaranteed to make progress, either by
// allocating an initial block with a nonzero size, or by allocating a larger block.
// 
// In the following implementation, nonzero sizes are increased by a
// factor of approximately 1.5 so that repeated reallocations have
// O(N) overall cost rather than O(N**2) cost, but the
// specification for this function does not guarantee that rate.
// 
// Here is an example of use:
// 
// int *p = NULL;
// size_t used = 0;
// size_t allocated = 0;
// void append_int (int value)
// {
//   if(used == allocated)
//     p = x2nrealloc (p, &allocated, sizeof *p);
//     p[used++] = value;
// }
// 
// This causes x2nrealloc to allocate a block of some nonzero size the first time it is called.
// 
// To have finer-grained control over the initial size, set *PN to a
// nonzero value before calling this function with P == NULL.  For example:
// 
// int *p = NULL;
// size_t used = 0;
// size_t allocated = 0;
// size_t allocated1 = 1000;
// void append_int (int value)
// {
//   if(used == allocated) {
//     p = x2nrealloc (p, &allocated1, sizeof *p);
//     allocated = allocated1;
//   }
//   p[used++] = value;
// }
// 
void * x2nrealloc(void * p, size_t * pn, size_t s)
{
	size_t n = *pn;
	if(!p) {
		if(!n) {
			/* The approximate size to use for initial small allocation
			   requests, when the invoking code specifies an old size of
			   zero.  64 bytes is the largest "small" request for the
			   GNU C library malloc.  */
			enum { DEFAULT_MXFAST = 64 };
			n = DEFAULT_MXFAST / s;
			n += !n;
		}
	}
	else {
		// Set N = ceil (1.5 * N) so that progress is made if N == 1.
		// Check for overflow, so that N * S stays in size_t range.
		// The check is slightly conservative, but an exact check isn't worth the trouble.
		if((size_t)-1 / 3 * 2 / s <= n)
			xalloc_die();
		n += (n + 1) / 2;
	}
	*pn = n;
	return xrealloc(p, n * s);
}
//
// Return a pointer to a new buffer of N bytes.  This is like xmalloc, except it returns char *.
//
char * xcharalloc(size_t n) { return XNMALLOC(n, char); }
//
// Clone an object P of size S, with error checking.  There's no need
// for xnmemdup (P, N, S), since xmemdup (P, N * S) works without any
// need for an arithmetic overflow check. 
//
void * xmemdup(void const * p, size_t s) { return memcpy(xmalloc(s), p, s); }
// Clone STRING. 
char * xstrdup(char const * string) { return (char *)xmemdup(string, strlen(string) + 1); }
