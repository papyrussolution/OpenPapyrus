/* xalloc.h -- malloc with out-of-memory checking

   Copyright (C) 1990-2000, 2003-2004, 2006-2011 Free Software Foundation, Inc.

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

#ifndef XALLOC_H_
#define XALLOC_H_

//#include <stddef.h>
//#ifdef __cplusplus
	//extern "C" {
//#endif
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
	#define _GL_ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#else
	#define _GL_ATTRIBUTE_NORETURN /* empty */
#endif
#if __GNUC__ >= 3
	#define _GL_ATTRIBUTE_MALLOC __attribute__ ((__malloc__))
#else
	#define _GL_ATTRIBUTE_MALLOC
#endif
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
	#define _GL_ATTRIBUTE_ALLOC_SIZE(args) __attribute__ ((__alloc_size__ args))
#else
	#define _GL_ATTRIBUTE_ALLOC_SIZE(args)
#endif
// 
// This function is always triggered when memory is exhausted.
// It must be defined by the application, either explicitly
// or by using gnulib's xalloc-die module.  This is the
// function to call when one wants the program to die because of a
// memory allocation failure. 
// 
extern void xalloc_die(void) _GL_ATTRIBUTE_NORETURN;
void * xmalloc(size_t s) _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_ALLOC_SIZE((1));
void * xzalloc(size_t s) _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_ALLOC_SIZE((1));
void * xcalloc(size_t n, size_t s) _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_ALLOC_SIZE((1, 2));
void * xrealloc(void * p, size_t s) _GL_ATTRIBUTE_ALLOC_SIZE((2));
void * x2realloc(void * p, size_t * pn);
void * xmemdup(void const * p, size_t s) _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_ALLOC_SIZE((2));
char * xstrdup(char const * str) _GL_ATTRIBUTE_MALLOC;

/* Return 1 if an array of N objects, each of size S, cannot exist due
   to size arithmetic overflow.  S must be positive and N must be
   nonnegative.  This is a macro, not an inline function, so that it
   works correctly even when SIZE_MAX < N.

   By gnulib convention, SIZE_MAX represents overflow in size
   calculations, so the conservative dividend to use here is
   SIZE_MAX - 1, since SIZE_MAX might represent an overflowed value.
   However, malloc (SIZE_MAX) fails on all known hosts where
   sizeof(ptrdiff_t) <= sizeof(size_t), so do not bother to test for
   exactly-SIZE_MAX allocations on such hosts; this avoids a test and
   branch when S is known to be 1.  */
#define xalloc_oversized(n, s) ((size_t)(sizeof(ptrdiff_t) <= sizeof(size_t) ? -1 : -2) / (s) < (n))

/* In the following macros, T must be an elementary or structure/union or
   typedef'ed type, or a pointer to such a type.  To apply one of the
   following macros to a function pointer or array type, you need to typedef
   it first and use the typedef name.  */

/* Allocate an object of type T dynamically, with error checking.  */
/* extern t *XMALLOC (typename t); */
#define XMALLOC(t) ((t*)xmalloc(sizeof(t)))

/* Allocate memory for N elements of type T, with error checking.  */
/* extern t *XNMALLOC (size_t n, typename t); */
#define XNMALLOC(n, t) ((t*)(sizeof(t) == 1 ? xmalloc(n) : xnmalloc(n, sizeof(t))))

/* Allocate an object of type T dynamically, with error checking, and zero it.  */
/* extern t *XZALLOC (typename t); */
#define XZALLOC(t) ((t*)xzalloc(sizeof(t)))

/* Allocate memory for N elements of type T, with error checking, and zero it.  */
/* extern t *XCALLOC (size_t n, typename t); */
#define XCALLOC(n, t) ((t*)(sizeof(t) == 1 ? xzalloc(n) : xcalloc(n, sizeof(t))))

void * xnmalloc(size_t n, size_t s) _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_ALLOC_SIZE((1, 2));
void * xnrealloc(void * p, size_t n, size_t s) _GL_ATTRIBUTE_ALLOC_SIZE((2, 3));
void * x2nrealloc(void * p, size_t * pn, size_t s);
char * xcharalloc(size_t n) _GL_ATTRIBUTE_MALLOC _GL_ATTRIBUTE_ALLOC_SIZE((1));

#ifdef __cplusplus
//}
// 
// C++ does not allow conversions from void * to other pointer types
// without a cast.  Use templates to work around the problem when possible.
// 
//template <typename T> inline T * xrealloc(T * p, size_t s) { return (T*)xrealloc((void*)p, s); }
//template <typename T> inline T * xnrealloc(T * p, size_t n, size_t s) { return (T*)xnrealloc((void*)p, n, s); }
//template <typename T> inline T * x2realloc(T * p, size_t * pn) { return (T*)x2realloc((void*)p, pn); }
//template <typename T> inline T * x2nrealloc(T * p, size_t * pn, size_t s) { return (T*)x2nrealloc((void*)p, pn, s); }
//template <typename T> inline T * xmemdup(T const * p, size_t s) { return (T*)xmemdup((void const*)p, s); }

#endif

#endif /* !XALLOC_H_ */
