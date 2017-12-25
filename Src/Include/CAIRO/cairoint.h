/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern California.
 *
 * Contributor(s): Carl D. Worth <cworth@cworth.org>
 */

/*
 * These definitions are solely for use by the implementation of cairo
 * and constitute no kind of standard.  If you need any of these
 * functions, please drop me a note.  Either the library needs new
 * functionality, or there's a way to do what you need using the
 * existing published interfaces. cworth@cworth.org
 */
#ifndef _CAIROINT_H_
#define _CAIROINT_H_

#if HAVE_CONFIG_H
	#include "config.h"
#endif
#ifdef _MSC_VER
	#define cairo_public __declspec(dllexport)
#endif

#include <slib.h> // @sobolev
#include <setjmp.h>
// @sobolev #include <assert.h>
// @sobolev #include <stdlib.h>
// @sobolev #include <string.h>
// @sobolev #include <stdarg.h>
// @sobolev #include <stddef.h>

#ifdef _MSC_VER
	#define _USE_MATH_DEFINES
#endif
// @sobolev #include <math.h>
// @sobolev #include <limits.h>
// @sobolev #include <stdio.h>

#include "cairo.h"
//
//#include "cairo-pixman-private.h" // @sobolev
//
#include "pixman-private.h"

#if PIXMAN_VERSION < PIXMAN_VERSION_ENCODE(0,22,0)
	#define pixman_image_composite32 pixman_image_composite
	#define pixman_image_get_component_alpha(i) 0
	#define pixman_image_set_component_alpha(i, x) do { } while (0)
#endif
//
// @sobolev #include <pixman.h>
//#include "cairo-compiler-private.h"
// 
// Size in bytes of buffer to use off the stack per functions.
// Mostly used by text functions.  For larger allocations, they'll SAlloc::M().
// 
#ifndef CAIRO_STACK_BUFFER_SIZE
	#define CAIRO_STACK_BUFFER_SIZE (512 * sizeof (int))
#endif
#define CAIRO_STACK_ARRAY_LENGTH(T) (CAIRO_STACK_BUFFER_SIZE / sizeof(T))
/*
 * The goal of this block is to define the following macros for
 * providing faster linkage to functions in the public API for calls from within cairo.
 *
 * slim_hidden_proto(f)
 * slim_hidden_proto_no_warn(f)
 *
 *   Declares `f' as a library internal function and hides the
 *   function from the global symbol table.  This macro must be
 *   expanded after `f' has been declared with a prototype but before
 *   any calls to the function are seen by the compiler.  The no_warn
 *   variant inhibits warnings about the return value being unused at
 *   call sites.  The macro works by renaming `f' to an internal name
 *   in the symbol table and hiding that.  As far as cairo internal
 *   calls are concerned they're calling a library internal function
 *   and thus don't need to bounce via the PLT.
 *
 * slim_hidden_def(f)
 *
 *   Exports `f' back to the global symbol table.  This macro must be
 *   expanded right after the function definition and only for symbols
 *   hidden previously with slim_hidden_proto().  The macro works by
 *   adding a global entry to the symbol table which points at the
 *   internal name of `f' created by slim_hidden_proto().
 *
 * Functions in the public API which aren't called by the library
 * don't need to be hidden and re-exported using the slim hidden macros.
 */
#if __GNUC__ >= 3 && defined(__ELF__) && !defined(__sun)
	#define slim_hidden_proto(name)		slim_hidden_proto1(name, slim_hidden_int_name(name)) cairo_private
	#define slim_hidden_proto_no_warn(name)	slim_hidden_proto1(name, slim_hidden_int_name(name)) cairo_private_no_warn
	#define slim_hidden_def(name)			slim_hidden_def1(name, slim_hidden_int_name(name))
	#define slim_hidden_int_name(name) INT_##name
	#define slim_hidden_proto1(name, internal) extern __typeof (name) name __asm__ (slim_hidden_asmname (internal))
	#define slim_hidden_def1(name, internal)   extern __typeof (name) EXT_##name __asm__(slim_hidden_asmname(name))	__attribute__((__alias__(slim_hidden_asmname(internal))))
	#define slim_hidden_ulp		slim_hidden_ulp1(__USER_LABEL_PREFIX__)
	#define slim_hidden_ulp1(x)		slim_hidden_ulp2(x)
	#define slim_hidden_ulp2(x)		#x
	#define slim_hidden_asmname(name)	slim_hidden_asmname1(name)
	#define slim_hidden_asmname1(name)	slim_hidden_ulp #name
#else
	#define slim_hidden_proto(name)		int _cairo_dummy_prototype(void)
	#define slim_hidden_proto_no_warn(name)	int _cairo_dummy_prototype(void)
	#define slim_hidden_def(name)			int _cairo_dummy_prototype(void)
#endif
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
	#define CAIRO_PRINTF_FORMAT(fmt_index, va_index) __attribute__((__format__(__printf__, fmt_index, va_index)))
#else
	#define CAIRO_PRINTF_FORMAT(fmt_index, va_index)
#endif
// slim_internal.h 
#define CAIRO_HAS_HIDDEN_SYMBOLS 1
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && (defined(__ELF__) || defined(__APPLE__)) && !defined(__sun)
	#define cairo_private_no_warn	__attribute__((__visibility__("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
	#define cairo_private_no_warn	__hidden
#else
	#define cairo_private_no_warn
	#undef CAIRO_HAS_HIDDEN_SYMBOLS
#endif
#ifndef WARN_UNUSED_RESULT
	#define WARN_UNUSED_RESULT
#endif
// Add attribute(warn_unused_result) if supported 
#define cairo_warn    WARN_UNUSED_RESULT
#define cairo_private cairo_private_no_warn cairo_warn
// 
// This macro allow us to deprecate a function by providing an alias
// for the old function name to the new function name. With this
// macro, binary compatibility is preserved. The macro only works on some platforms --- tough.
// 
// Meanwhile, new definitions in the public header file break the
// source code so that it will no longer link against the old
// symbols. Instead it will give a descriptive error message
// indicating that the old function has been deprecated by the new function.
// 
#if __GNUC__ >= 2 && defined(__ELF__)
	#define CAIRO_FUNCTION_ALIAS(old, new) extern __typeof (new) old __asm__ ("" #old) __attribute__((__alias__("" #new)))
#else
	#define CAIRO_FUNCTION_ALIAS(old, new)
#endif
// 
// Cairo uses the following function attributes in order to improve the
// generated code (effectively by manual inter-procedural analysis).
// 
//   'cairo_pure': The function is only allowed to read from its arguments
//      and global memory (i.e. following a pointer argument or accessing a shared variable). The return value should
//      only depend on its arguments, and for an identical set of arguments should return the same value.
// 
//   'cairo_const': The function is only allowed to read from its arguments.
//      It is not allowed to access global memory. The return value should only depend its arguments, and for an
//      identical set of arguments should return the same value. This is currently the most strict function attribute.
// 
// Both these function attributes allow gcc to perform CSE and
// constant-folding, with 'cairo_const 'also guaranteeing that pointer contents
// do not change across the function call.
// 
#if __GNUC__ >= 3
	#define cairo_pure __attribute__((pure))
	#define cairo_const __attribute__((const))
	#define cairo_always_inline inline __attribute__((always_inline))
#else
	#define cairo_pure
	#define cairo_const
	#define cairo_always_inline inline
#endif
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
	#define likely(expr) (__builtin_expect (!!(expr), 1))
	#define unlikely(expr) (__builtin_expect (!!(expr), 0))
#else
	#define likely(expr) (expr)
	#define unlikely(expr) (expr)
#endif
#ifndef __GNUC__
	#undef __attribute__
	#define __attribute__(x)
#endif
#if (defined(__WIN32__) && !defined(__WINE__)) || defined(_MSC_VER)
	#define access _access
	#define fdopen _fdopen
	#define hypot _hypot
	#define pclose _pclose
	#define popen _popen
	#define snprintf _snprintf
	//#define strdup _strdup
	#define unlink _unlink
	#define vsnprintf _vsnprintf
#endif
#ifdef _MSC_VER
	#ifndef __cplusplus
		#undef inline
		#define inline __inline
	#endif
#endif
#if defined(_MSC_VER) && defined(_M_IX86)
	// When compiling with /Gy and /OPT:ICF identical functions will be folded in together.
	// The CAIRO_ENSURE_UNIQUE macro ensures that a function is always unique and
	// will never be folded into another one. Something like this might eventually
	// be needed for GCC but it seems fine for now. */
	#define CAIRO_ENSURE_UNIQUE                       \
		do {                                          \
		char file[] = __FILE__;                   \
		__asm {                                   \
			__asm jmp __internal_skip_line_no     \
			__asm _emit (__COUNTER__ & 0xff)      \
			__asm _emit ((__COUNTER__>>8) & 0xff) \
			__asm _emit ((__COUNTER__>>16) & 0xff)\
			__asm _emit ((__COUNTER__>>24) & 0xff)\
			__asm lea eax, dword ptr file         \
			__asm __internal_skip_line_no:        \
		};                                        \
		} while (0)
#else
	#define CAIRO_ENSURE_UNIQUE    do { } while (0)
#endif
#ifdef __STRICT_ANSI__
	#undef inline
	#define inline __inline__
#endif
//
//#include "cairo-list-private.h"
//
// Basic circular, doubly linked list implementation 
//
struct cairo_list_t {
    cairo_list_t * next;
	cairo_list_t * prev;
};
//
//#include "cairo-list-inline.h"
//
#define cairo_list_entry(ptr, type, member)       cairo_container_of(ptr, type, member)
#define cairo_list_first_entry(ptr, type, member) cairo_list_entry((ptr)->next, type, member)
#define cairo_list_last_entry(ptr, type, member)  cairo_list_entry((ptr)->prev, type, member)
#define cairo_list_foreach(pos, head)             for(pos = (head)->next; pos != (head); pos = pos->next)
#define cairo_list_foreach_entry(pos, type, head, member)		\
	for(pos = cairo_list_entry((head)->next, type, member);	&pos->member != (head); pos = cairo_list_entry(pos->member.next, type, member))
#define cairo_list_foreach_entry_safe(pos, n, type, head, member)	\
	for(pos = cairo_list_entry((head)->next, type, member),	n = cairo_list_entry(pos->member.next, type, member); &pos->member != (head); pos = n, n = cairo_list_entry(n->member.next, type, member))
#define cairo_list_foreach_entry_reverse(pos, type, head, member)	\
	for(pos = cairo_list_entry((head)->prev, type, member);	&pos->member != (head); pos = cairo_list_entry(pos->member.prev, type, member))
#define cairo_list_foreach_entry_reverse_safe(pos, n, type, head, member)	\
	for(pos = cairo_list_entry((head)->prev, type, member),	n = cairo_list_entry(pos->member.prev, type, member); &pos->member != (head); pos = n, n = cairo_list_entry(n->member.prev, type, member))

#ifdef CAIRO_LIST_DEBUG
	static inline void _cairo_list_validate(const cairo_list_t * link)
	{
		assert(link->next->prev == link);
		assert(link->prev->next == link);
	}
	static inline void cairo_list_validate(const cairo_list_t * head)
	{
		cairo_list_t * link;
		cairo_list_foreach(link, head)
		_cairo_list_validate(link);
	}
	static inline cairo_bool_t cairo_list_is_empty(const cairo_list_t * head);
	static inline void cairo_list_validate_is_empty(const cairo_list_t * head)
	{
		assert(head->next == NULL || (cairo_list_is_empty(head) && head->next == head->prev));
	}
#else
	#define _cairo_list_validate(link)
	#define cairo_list_validate(head)
	#define cairo_list_validate_is_empty(head)
#endif

static inline void cairo_list_init(cairo_list_t * entry)
{
	entry->next = entry;
	entry->prev = entry;
}

static inline void __cairo_list_add(cairo_list_t * entry, cairo_list_t * prev, cairo_list_t * next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

static inline void cairo_list_add(cairo_list_t * entry, cairo_list_t * head)
{
	cairo_list_validate(head);
	cairo_list_validate_is_empty(entry);
	__cairo_list_add(entry, head, head->next);
	cairo_list_validate(head);
}

static inline void cairo_list_add_tail(cairo_list_t * entry, cairo_list_t * head)
{
	cairo_list_validate(head);
	cairo_list_validate_is_empty(entry);
	__cairo_list_add(entry, head->prev, head);
	cairo_list_validate(head);
}

static inline void __cairo_list_del(cairo_list_t * prev, cairo_list_t * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void _cairo_list_del(cairo_list_t * entry)
{
	__cairo_list_del(entry->prev, entry->next);
}

static inline void cairo_list_del(cairo_list_t * entry)
{
	_cairo_list_del(entry);
	cairo_list_init(entry);
}

static inline void cairo_list_move(cairo_list_t * entry, cairo_list_t * head)
{
	cairo_list_validate(head);
	__cairo_list_del(entry->prev, entry->next);
	__cairo_list_add(entry, head, head->next);
	cairo_list_validate(head);
}

static inline void cairo_list_move_tail(cairo_list_t * entry, cairo_list_t * head)
{
	cairo_list_validate(head);
	__cairo_list_del(entry->prev, entry->next);
	__cairo_list_add(entry, head->prev, head);
	cairo_list_validate(head);
}

static inline void cairo_list_swap(cairo_list_t * entry, cairo_list_t * other)
{
	__cairo_list_add(entry, other->prev, other->next);
	cairo_list_init(other);
}

static inline cairo_bool_t cairo_list_is_first(const cairo_list_t * entry, const cairo_list_t * head)
{
	cairo_list_validate(head);
	return entry->prev == head;
}

static inline cairo_bool_t cairo_list_is_last(const cairo_list_t * entry, const cairo_list_t * head)
{
	cairo_list_validate(head);
	return entry->next == head;
}

static inline cairo_bool_t cairo_list_is_empty(const cairo_list_t * head)
{
	cairo_list_validate(head);
	return head->next == head;
}

static inline cairo_bool_t cairo_list_is_singular(const cairo_list_t * head)
{
	cairo_list_validate(head);
	return head->next == head || head->next == head->prev;
}
//
//#include "cairo-atomic-private.h"
//
// The autoconf on OpenBSD 4.5 produces the malformed constant name
// SIZEOF_VOID__ rather than SIZEOF_VOID_P.  Work around that here. 
#if !defined(SIZEOF_VOID_P) && defined(SIZEOF_VOID__)
	#define SIZEOF_VOID_P SIZEOF_VOID__
#endif

CAIRO_BEGIN_DECLS

#if HAVE_INTEL_ATOMIC_PRIMITIVES
	#define HAS_ATOMIC_OPS 1

	typedef int cairo_atomic_int_t;

	#ifdef ATOMIC_OP_NEEDS_MEMORY_BARRIER
		static cairo_always_inline cairo_atomic_int_t _cairo_atomic_int_get(cairo_atomic_int_t * x)
		{
			__sync_synchronize();
			return *x;
		}
		static cairo_always_inline void * _cairo_atomic_ptr_get(void ** x)
		{
			__sync_synchronize();
			return *x;
		}
	#else
		#define _cairo_atomic_int_get(x) (*x)
		#define _cairo_atomic_ptr_get(x) (*x)
	#endif
	#define _cairo_atomic_int_inc(x) ((void)__sync_fetch_and_add(x, 1))
	#define _cairo_atomic_int_dec(x) ((void)__sync_fetch_and_add(x, -1))
	#define _cairo_atomic_int_dec_and_test(x) (__sync_fetch_and_add(x, -1) == 1)
	#define _cairo_atomic_int_cmpxchg(x, oldv, newv) __sync_bool_compare_and_swap(x, oldv, newv)
	#define _cairo_atomic_int_cmpxchg_return_old(x, oldv, newv) __sync_val_compare_and_swap(x, oldv, newv)
	#if SIZEOF_VOID_P==SIZEOF_INT
		typedef int cairo_atomic_intptr_t;
	#elif SIZEOF_VOID_P==SIZEOF_LONG
		typedef long cairo_atomic_intptr_t;
	#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
		typedef long long cairo_atomic_intptr_t;
	#else
		#error No matching integer pointer type
	#endif
	#define _cairo_atomic_ptr_cmpxchg(x, oldv, newv) __sync_bool_compare_and_swap((cairo_atomic_intptr_t*)x, (cairo_atomic_intptr_t)oldv, (cairo_atomic_intptr_t)newv)
	#define _cairo_atomic_ptr_cmpxchg_return_old(x, oldv, newv) \
		_cairo_atomic_intptr_to_voidptr(__sync_val_compare_and_swap((cairo_atomic_intptr_t*)x, (cairo_atomic_intptr_t)oldv, (cairo_atomic_intptr_t)newv))
#endif
#if HAVE_LIB_ATOMIC_OPS
	#include <atomic_ops.h>

	#define HAS_ATOMIC_OPS 1

	typedef  AO_t cairo_atomic_int_t;

	#define _cairo_atomic_int_get(x) (AO_load_full(x))
	#define _cairo_atomic_int_inc(x) ((void)AO_fetch_and_add1_full(x))
	#define _cairo_atomic_int_dec(x) ((void)AO_fetch_and_sub1_full(x))
	#define _cairo_atomic_int_dec_and_test(x) (AO_fetch_and_sub1_full(x) == 1)
	#define _cairo_atomic_int_cmpxchg(x, oldv, newv) AO_compare_and_swap_full(x, oldv, newv)
	#if SIZEOF_VOID_P==SIZEOF_INT
		typedef uint cairo_atomic_intptr_t;
	#elif SIZEOF_VOID_P==SIZEOF_LONG
		typedef ulong cairo_atomic_intptr_t;
	#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
		typedef ulong long cairo_atomic_intptr_t;
	#else
		#error No matching integer pointer type
	#endif
	#define _cairo_atomic_ptr_get(x) _cairo_atomic_intptr_to_voidptr(AO_load_full(x))
	#define _cairo_atomic_ptr_cmpxchg(x, oldv, newv) _cairo_atomic_int_cmpxchg((cairo_atomic_intptr_t*)(x), (cairo_atomic_intptr_t)oldv, (cairo_atomic_intptr_t)newv)
#endif
#if HAVE_OS_ATOMIC_OPS
	#include <libkern/OSAtomic.h>

	#define HAS_ATOMIC_OPS 1

	typedef int32_t cairo_atomic_int_t;

	#define _cairo_atomic_int_get(x) (OSMemoryBarrier(), *(x))
	#define _cairo_atomic_int_inc(x) ((void)OSAtomicIncrement32Barrier(x))
	#define _cairo_atomic_int_dec(x) ((void)OSAtomicDecrement32Barrier(x))
	#define _cairo_atomic_int_dec_and_test(x) (OSAtomicDecrement32Barrier(x) == 0)
	#define _cairo_atomic_int_cmpxchg(x, oldv, newv) OSAtomicCompareAndSwap32Barrier(oldv, newv, x)
	#if SIZEOF_VOID_P==4
		typedef int32_t cairo_atomic_intptr_t;
		#define _cairo_atomic_ptr_cmpxchg(x, oldv, newv) \
			OSAtomicCompareAndSwap32Barrier((cairo_atomic_intptr_t)oldv, (cairo_atomic_intptr_t)newv, (cairo_atomic_intptr_t*)x)
	#elif SIZEOF_VOID_P==8
		typedef int64_t cairo_atomic_intptr_t;
		#define _cairo_atomic_ptr_cmpxchg(x, oldv, newv) \
			OSAtomicCompareAndSwap64Barrier((cairo_atomic_intptr_t)oldv, (cairo_atomic_intptr_t)newv, (cairo_atomic_intptr_t*)x)
	#else
		#error No matching integer pointer type
	#endif
	#define _cairo_atomic_ptr_get(x) (OSMemoryBarrier(), *(x))
#endif
#ifndef HAS_ATOMIC_OPS
	#if SIZEOF_VOID_P==SIZEOF_INT
		typedef uint cairo_atomic_intptr_t;
	#elif SIZEOF_VOID_P==SIZEOF_LONG
		typedef ulong cairo_atomic_intptr_t;
	#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
		typedef ulong long cairo_atomic_intptr_t;
	#else
		#error No matching integer pointer type
	#endif

	typedef cairo_atomic_intptr_t cairo_atomic_int_t;

	cairo_private void FASTCALL _cairo_atomic_int_inc(cairo_atomic_int_t * x);

	#define _cairo_atomic_int_dec(x) _cairo_atomic_int_dec_and_test(x)

	cairo_private cairo_bool_t FASTCALL _cairo_atomic_int_dec_and_test(cairo_atomic_int_t * x);
	cairo_private cairo_atomic_int_t _cairo_atomic_int_cmpxchg_return_old_impl(cairo_atomic_int_t * x, cairo_atomic_int_t oldv, cairo_atomic_int_t newv);
	cairo_private void * _cairo_atomic_ptr_cmpxchg_return_old_impl(void ** x, void * oldv, void * newv);

	#define _cairo_atomic_int_cmpxchg_return_old(x, oldv, newv) _cairo_atomic_int_cmpxchg_return_old_impl(x, oldv, newv)
	#define _cairo_atomic_ptr_cmpxchg_return_old(x, oldv, newv) _cairo_atomic_ptr_cmpxchg_return_old_impl(x, oldv, newv)
	#ifdef ATOMIC_OP_NEEDS_MEMORY_BARRIER
		cairo_private cairo_atomic_int_t _cairo_atomic_int_get(cairo_atomic_int_t * x);
		#define _cairo_atomic_ptr_get(x) (void*)_cairo_atomic_int_get((cairo_atomic_int_t*)x)
	#else
		#define _cairo_atomic_int_get(x) (* x)
		#define _cairo_atomic_ptr_get(x) (* x)
	#endif
#else

// Workaround GCC complaining about casts 
static cairo_always_inline void * _cairo_atomic_intptr_to_voidptr(cairo_atomic_intptr_t x)
{
	return (void*)x;
}

static cairo_always_inline cairo_atomic_int_t _cairo_atomic_int_cmpxchg_return_old_fallback(cairo_atomic_int_t * x, cairo_atomic_int_t oldv, cairo_atomic_int_t newv)
{
	cairo_atomic_int_t curr;
	do {
		curr = _cairo_atomic_int_get(x);
	} while(curr == oldv && !_cairo_atomic_int_cmpxchg(x, oldv, newv));
	return curr;
}

static cairo_always_inline void * _cairo_atomic_ptr_cmpxchg_return_old_fallback(void ** x, void * oldv, void * newv)
{
	void * curr;
	do {
		curr = _cairo_atomic_ptr_get(x);
	} while(curr == oldv && !_cairo_atomic_ptr_cmpxchg(x, oldv, newv));
	return curr;
}

#endif

#ifndef _cairo_atomic_int_cmpxchg_return_old
	#define _cairo_atomic_int_cmpxchg_return_old(x, oldv, newv) _cairo_atomic_int_cmpxchg_return_old_fallback(x, oldv, newv)
#endif
#ifndef _cairo_atomic_ptr_cmpxchg_return_old
	#define _cairo_atomic_ptr_cmpxchg_return_old(x, oldv, newv) _cairo_atomic_ptr_cmpxchg_return_old_fallback(x, oldv, newv)
#endif
#ifndef _cairo_atomic_int_cmpxchg
	#define _cairo_atomic_int_cmpxchg(x, oldv, newv) (_cairo_atomic_int_cmpxchg_return_old(x, oldv, newv) == oldv)
#endif
#ifndef _cairo_atomic_ptr_cmpxchg
	#define _cairo_atomic_ptr_cmpxchg(x, oldv, newv) (_cairo_atomic_ptr_cmpxchg_return_old(x, oldv, newv) == oldv)
#endif
#define _cairo_atomic_uint_get(x) _cairo_atomic_int_get(x)
#define _cairo_atomic_uint_cmpxchg(x, oldv, newv) _cairo_atomic_int_cmpxchg((cairo_atomic_int_t*)x, oldv, newv)
#define _cairo_status_set_error(status, err) do { \
		int ret__; \
		assert(err < CAIRO_STATUS_LAST_STATUS);	\
		/* hide compiler warnings about cairo_status_t != int (gcc treats its as \
		 * an unsigned integer instead, and about ignoring the return value. */  \
		ret__ = _cairo_atomic_int_cmpxchg((cairo_atomic_int_t*)status, CAIRO_STATUS_SUCCESS, err); \
		(void)ret__; \
} while(0)

CAIRO_END_DECLS
//
//#include "cairo-reference-count-private.h"
//
// Encapsulate operations on the object's reference count 
//
struct cairo_reference_count_t {
	cairo_atomic_int_t ref_count;
};

#define _cairo_reference_count_inc(RC) _cairo_atomic_int_inc(&(RC)->ref_count)
#define _cairo_reference_count_dec(RC) _cairo_atomic_int_dec(&(RC)->ref_count)
#define _cairo_reference_count_dec_and_test(RC) _cairo_atomic_int_dec_and_test(&(RC)->ref_count)
#define CAIRO_REFERENCE_COUNT_INIT(RC, VALUE) ((RC)->ref_count = (VALUE))
#define CAIRO_REFERENCE_COUNT_GET_VALUE(RC) _cairo_atomic_int_get(&(RC)->ref_count)
#define CAIRO_REFERENCE_COUNT_INVALID_VALUE ((cairo_atomic_int_t)-1)
#define CAIRO_REFERENCE_COUNT_INVALID {CAIRO_REFERENCE_COUNT_INVALID_VALUE}
#define CAIRO_REFERENCE_COUNT_IS_INVALID(RC) (CAIRO_REFERENCE_COUNT_GET_VALUE(RC) == CAIRO_REFERENCE_COUNT_INVALID_VALUE)
#define CAIRO_REFERENCE_COUNT_HAS_REFERENCE(RC) (CAIRO_REFERENCE_COUNT_GET_VALUE(RC) > 0)
//
//#include "cairo-wideint-type-private.h"
//
#if HAVE_STDINT_H
	#include <stdint.h>
#elif HAVE_INTTYPES_H
	#include <inttypes.h>
#elif HAVE_SYS_INT_TYPES_H
	#include <sys/int_types.h>
#elif defined(_MSC_VER)
	typedef signed   __int8  int8; // @sobolev __int8-->signed __int8
	typedef unsigned __int8  uint8;
	typedef __int16          int16;
	typedef unsigned __int16 uint16;
	typedef __int32          int32_t;
	typedef unsigned __int32 uint32_t;
	typedef __int64          int64_t;
	typedef unsigned __int64 uint64_t;
	#ifndef HAVE_UINT64_T
		#define HAVE_UINT64_T 1
	#endif
#else
	#error Cannot find definitions for fixed-width integral types (uint8, uint32_t, etc.)
#endif
#ifndef INT16_MIN
	#define INT16_MIN	(-32767-1)
#endif
#ifndef INT16_MAX
	#define INT16_MAX	(32767)
#endif
#ifndef UINT16_MAX
	#define UINT16_MAX	(65535)
#endif
#ifndef INT32_MIN
	#define INT32_MIN	(-2147483647-1)
#endif
#ifndef INT32_MAX
	#define INT32_MAX	(2147483647)
#endif
#ifndef UINT32_MAX
	#define UINT32_MAX     (4294967295U)
#endif
#if HAVE_BYTESWAP_H
	#include <byteswap.h>
#endif
#ifndef bswap_16
	#define bswap_16(p) (((((uint16)(p)) & 0x00ff) << 8) | (((uint16)(p)) >> 8));
#endif
#ifndef bswap_32
	#define bswap_32(p) (((((uint32_t)(p)) & 0x000000ff) << 24) | ((((uint32_t)(p)) & 0x0000ff00) << 8) | ((((uint32_t)(p)) & 0x00ff0000) >> 8) | ((((uint32_t)(p))) >> 24));
#endif
#if !HAVE_UINT64_T
	typedef struct _cairo_uint64 {
		uint32_t	lo, hi;
	} cairo_uint64_t, cairo_int64_t;
#else
	typedef uint64_t    cairo_uint64_t;
	typedef int64_t	    cairo_int64_t;
#endif

typedef struct _cairo_uquorem64 {
    cairo_uint64_t	quo;
    cairo_uint64_t	rem;
} cairo_uquorem64_t;

typedef struct _cairo_quorem64 {
    cairo_int64_t	quo;
    cairo_int64_t	rem;
} cairo_quorem64_t;
//
// gcc has a non-standard name
//
#if HAVE___UINT128_T && !HAVE_UINT128_T
	typedef __uint128_t uint128_t;
	typedef __int128_t int128_t;
	#define HAVE_UINT128_T 1
#endif

#if !HAVE_UINT128_T
	typedef struct cairo_uint128 {
		cairo_uint64_t	lo, hi;
	} cairo_uint128_t, cairo_int128_t;
#else
	typedef uint128_t	cairo_uint128_t;
	typedef int128_t	cairo_int128_t;
#endif

typedef struct _cairo_uquorem128 {
    cairo_uint128_t	quo;
    cairo_uint128_t	rem;
} cairo_uquorem128_t;

typedef struct _cairo_quorem128 {
    cairo_int128_t	quo;
    cairo_int128_t	rem;
} cairo_quorem128_t;
//
//#include "cairo-wideint-private.h"
//
// 
// 64-bit datatypes.  Two separate implementations, one using
// built-in 64-bit signed/unsigned types another implemented as a pair of 32-bit ints
// 
#define I cairo_private cairo_const

#if !HAVE_UINT64_T
	cairo_uquorem64_t I _cairo_uint64_divrem(cairo_uint64_t num, cairo_uint64_t den);
	cairo_uint64_t I        _cairo_double_to_uint64(double i);
	double I        _cairo_uint64_to_double(cairo_uint64_t i);
	cairo_int64_t I        _cairo_double_to_int64(double i);
	double I        _cairo_int64_to_double(cairo_uint64_t i);
	cairo_uint64_t I        _cairo_uint32_to_uint64(uint32_t i);
	#define _cairo_uint64_to_uint32(a)  ((a).lo)
	cairo_uint64_t I        _cairo_uint64_add(cairo_uint64_t a, cairo_uint64_t b);
	cairo_uint64_t I        _cairo_uint64_sub(cairo_uint64_t a, cairo_uint64_t b);
	cairo_uint64_t I        _cairo_uint64_mul(cairo_uint64_t a, cairo_uint64_t b);
	cairo_uint64_t I        _cairo_uint32x32_64_mul(uint32_t a, uint32_t b);
	cairo_uint64_t I        _cairo_uint64_lsl(cairo_uint64_t a, int shift);
	cairo_uint64_t I        _cairo_uint64_rsl(cairo_uint64_t a, int shift);
	cairo_uint64_t I        _cairo_uint64_rsa(cairo_uint64_t a, int shift);
	int I  _cairo_uint64_lt(cairo_uint64_t a, cairo_uint64_t b);
	int I  _cairo_uint64_cmp(cairo_uint64_t a, cairo_uint64_t b);
	int I  _cairo_uint64_eq(cairo_uint64_t a, cairo_uint64_t b);
	cairo_uint64_t I        _cairo_uint64_negate(cairo_uint64_t a);
	#define _cairo_uint64_is_zero(a) ((a).hi == 0 && (a).lo == 0)
	#define _cairo_uint64_negative(a)   (((int32_t)((a).hi)) < 0)
	cairo_uint64_t I        _cairo_uint64_not(cairo_uint64_t a);

	#define _cairo_uint64_to_int64(i)   (i)
	#define _cairo_int64_to_uint64(i)   (i)

	cairo_int64_t I        _cairo_int32_to_int64(int32_t i);
	#define _cairo_int64_to_int32(a)    ((int32_t)_cairo_uint64_to_uint32(a))
	#define _cairo_int64_add(a, b)       _cairo_uint64_add(a, b)
	#define _cairo_int64_sub(a, b)       _cairo_uint64_sub(a, b)
	#define _cairo_int64_mul(a, b)       _cairo_uint64_mul(a, b)
	cairo_int64_t I        _cairo_int32x32_64_mul(int32_t a, int32_t b);
	int I  _cairo_int64_lt(cairo_int64_t a, cairo_int64_t b);
	int I  _cairo_int64_cmp(cairo_int64_t a, cairo_int64_t b);
	#define _cairo_int64_is_zero(a)     _cairo_uint64_is_zero(a)
	#define _cairo_int64_eq(a, b)        _cairo_uint64_eq(a, b)
	#define _cairo_int64_lsl(a, b)       _cairo_uint64_lsl(a, b)
	#define _cairo_int64_rsl(a, b)       _cairo_uint64_rsl(a, b)
	#define _cairo_int64_rsa(a, b)       _cairo_uint64_rsa(a, b)
	#define _cairo_int64_negate(a)      _cairo_uint64_negate(a)
	#define _cairo_int64_negative(a)    (((int32_t)((a).hi)) < 0)
	#define _cairo_int64_not(a)         _cairo_uint64_not(a)
#else
	static inline cairo_uquorem64_t _cairo_uint64_divrem(cairo_uint64_t num, cairo_uint64_t den)
	{
		cairo_uquorem64_t qr;
		qr.quo = num / den;
		qr.rem = num % den;
		return qr;
	}
	// 
	// These need to be functions or gcc will complain when used on the result of a function:
	// 
	// warning: cast from function call of type ‘#cairo_uint64_t’ to non-matching type ‘double’
	// 
	static cairo_always_inline cairo_const cairo_uint64_t _cairo_double_to_uint64(double i) 
	{
		return (cairo_uint64_t)i;
	}
	static cairo_always_inline cairo_const double _cairo_uint64_to_double(cairo_uint64_t i) 
	{
		return (double)i;
	}
	static cairo_always_inline cairo_int64_t I _cairo_double_to_int64(double i) 
	{
		return (cairo_uint64_t)i;
	}
	static cairo_always_inline double I _cairo_int64_to_double(cairo_int64_t i) 
	{
		return (double)i;
	}
	#define _cairo_uint32_to_uint64(i)  ((uint64_t)(i))
	#define _cairo_uint64_to_uint32(i)  ((uint32_t)(i))
	#define _cairo_uint64_add(a, b)      ((a) + (b))
	#define _cairo_uint64_sub(a, b)      ((a) - (b))
	#define _cairo_uint64_mul(a, b)      ((a) * (b))
	#define _cairo_uint32x32_64_mul(a, b)    ((uint64_t)(a) * (b))
	#define _cairo_uint64_lsl(a, b)      ((a) << (b))
	#define _cairo_uint64_rsl(a, b)      ((uint64_t)(a) >> (b))
	#define _cairo_uint64_rsa(a, b)      ((uint64_t)((int64_t)(a) >> (b)))
	#define _cairo_uint64_lt(a, b)       ((a) < (b))
	#define _cairo_uint64_cmp(a, b)       ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
	#define _cairo_uint64_is_zero(a)    ((a) == 0)
	#define _cairo_uint64_eq(a, b)       ((a) == (b))
	#define _cairo_uint64_negate(a)     ((uint64_t)-((int64_t)(a)))
	#define _cairo_uint64_negative(a)   ((int64_t)(a) < 0)
	#define _cairo_uint64_not(a)        (~(a))
	#define _cairo_uint64_to_int64(i)   ((int64_t)(i))
	#define _cairo_int64_to_uint64(i)   ((uint64_t)(i))
	#define _cairo_int32_to_int64(i)    ((int64_t)(i))
	#define _cairo_int64_to_int32(i)    ((int32_t)(i))
	#define _cairo_int64_add(a, b)       ((a) + (b))
	#define _cairo_int64_sub(a, b)       ((a) - (b))
	#define _cairo_int64_mul(a, b)       ((a) * (b))
	#define _cairo_int32x32_64_mul(a, b) ((int64_t)(a) * (b))
	#define _cairo_int64_lt(a, b)        ((a) < (b))
	#define _cairo_int64_cmp(a, b)       ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
	#define _cairo_int64_is_zero(a)     ((a) == 0)
	#define _cairo_int64_eq(a, b)        ((a) == (b))
	#define _cairo_int64_lsl(a, b)       ((a) << (b))
	#define _cairo_int64_rsl(a, b)       ((int64_t)((uint64_t)(a) >> (b)))
	#define _cairo_int64_rsa(a, b)       ((int64_t)(a) >> (b))
	#define _cairo_int64_negate(a)      (-(a))
	#define _cairo_int64_negative(a)    ((a) < 0)
	#define _cairo_int64_not(a)         (~(a))
#endif
// 
// 64-bit comparisons derived from lt or eq
// 
#define _cairo_uint64_le(a, b)       (!_cairo_uint64_gt(a, b))
#define _cairo_uint64_ne(a, b)       (!_cairo_uint64_eq(a, b))
#define _cairo_uint64_ge(a, b)       (!_cairo_uint64_lt(a, b))
#define _cairo_uint64_gt(a, b)       _cairo_uint64_lt(b, a)
#define _cairo_int64_le(a, b)        (!_cairo_int64_gt(a, b))
#define _cairo_int64_ne(a, b)        (!_cairo_int64_eq(a, b))
#define _cairo_int64_ge(a, b)        (!_cairo_int64_lt(a, b))
#define _cairo_int64_gt(a, b)        _cairo_int64_lt(b, a)
// 
// As the C implementation always computes both, create
// a function which returns both for the 'native' type as well
// 
static inline cairo_quorem64_t _cairo_int64_divrem(cairo_int64_t num, cairo_int64_t den)
{
	int num_neg = _cairo_int64_negative(num);
	int den_neg = _cairo_int64_negative(den);
	cairo_uquorem64_t uqr;
	cairo_quorem64_t qr;
	if(num_neg)
		num = _cairo_int64_negate(num);
	if(den_neg)
		den = _cairo_int64_negate(den);
	uqr = _cairo_uint64_divrem(num, den);
	if(num_neg)
		qr.rem = _cairo_int64_negate(uqr.rem);
	else
		qr.rem = uqr.rem;
	if(num_neg != den_neg)
		qr.quo = (cairo_int64_t)_cairo_int64_negate(uqr.quo);
	else
		qr.quo = (cairo_int64_t)uqr.quo;
	return qr;
}

static inline int32_t _cairo_int64_32_div(cairo_int64_t num, int32_t den)
{
#if !HAVE_UINT64_T
	return _cairo_int64_to_int32(_cairo_int64_divrem(num, _cairo_int32_to_int64(den)).quo);
#else
	return (int32_t)(num / den);
#endif
}
// 
// 128-bit datatypes.  Again, provide two implementations in
// case the machine has a native 128-bit datatype.  GCC supports int128_t on ia64
// 
#if !HAVE_UINT128_T
	cairo_uint128_t I _cairo_uint32_to_uint128(uint32_t i);
	cairo_uint128_t I _cairo_uint64_to_uint128(cairo_uint64_t i);
	#define _cairo_uint128_to_uint64(a)     ((a).lo)
	#define _cairo_uint128_to_uint32(a)     _cairo_uint64_to_uint32(_cairo_uint128_to_uint64(a))
	cairo_uint128_t I _cairo_uint128_add(cairo_uint128_t a, cairo_uint128_t b);
	cairo_uint128_t I _cairo_uint128_sub(cairo_uint128_t a, cairo_uint128_t b);
	cairo_uint128_t I _cairo_uint128_mul(cairo_uint128_t a, cairo_uint128_t b);
	cairo_uint128_t I _cairo_uint64x64_128_mul(cairo_uint64_t a, cairo_uint64_t b);
	cairo_uint128_t I _cairo_uint128_lsl(cairo_uint128_t a, int shift);
	cairo_uint128_t I _cairo_uint128_rsl(cairo_uint128_t a, int shift);
	cairo_uint128_t I _cairo_uint128_rsa(cairo_uint128_t a, int shift);
	int I _cairo_uint128_lt(cairo_uint128_t a, cairo_uint128_t b);
	int I _cairo_uint128_cmp(cairo_uint128_t a, cairo_uint128_t b);
	int I _cairo_uint128_eq(cairo_uint128_t a, cairo_uint128_t b);
	#define _cairo_uint128_is_zero(a) (_cairo_uint64_is_zero((a).hi) && _cairo_uint64_is_zero((a).lo))
	cairo_uint128_t I _cairo_uint128_negate(cairo_uint128_t a);
	#define _cairo_uint128_negative(a)  (_cairo_uint64_negative(a.hi))
	cairo_uint128_t I _cairo_uint128_not(cairo_uint128_t a);

	#define _cairo_uint128_to_int128(i)     (i)
	#define _cairo_int128_to_uint128(i)     (i)

	cairo_int128_t I _cairo_int32_to_int128(int32_t i);
	cairo_int128_t I _cairo_int64_to_int128(cairo_int64_t i);
	#define _cairo_int128_to_int64(a)   ((cairo_int64_t)(a).lo)
	#define _cairo_int128_to_int32(a)   _cairo_int64_to_int32(_cairo_int128_to_int64(a))
	#define _cairo_int128_add(a, b)      _cairo_uint128_add(a, b)
	#define _cairo_int128_sub(a, b)      _cairo_uint128_sub(a, b)
	#define _cairo_int128_mul(a, b)      _cairo_uint128_mul(a, b)
	cairo_int128_t I _cairo_int64x64_128_mul(cairo_int64_t a, cairo_int64_t b);
	#define _cairo_int64x32_128_mul(a, b) _cairo_int64x64_128_mul(a, _cairo_int32_to_int64(b))
	#define _cairo_int128_lsl(a, b)      _cairo_uint128_lsl(a, b)
	#define _cairo_int128_rsl(a, b)      _cairo_uint128_rsl(a, b)
	#define _cairo_int128_rsa(a, b)      _cairo_uint128_rsa(a, b)
	int I _cairo_int128_lt(cairo_int128_t a, cairo_int128_t b);
	int I _cairo_int128_cmp(cairo_int128_t a, cairo_int128_t b);
	#define _cairo_int128_is_zero(a)    _cairo_uint128_is_zero(a)
	#define _cairo_int128_eq(a, b)       _cairo_uint128_eq(a, b)
	#define _cairo_int128_negate(a)     _cairo_uint128_negate(a)
	#define _cairo_int128_negative(a)   (_cairo_uint128_negative(a))
	#define _cairo_int128_not(a)        _cairo_uint128_not(a)
#else // !HAVE_UINT128_T 
	#define _cairo_uint32_to_uint128(i) ((uint128_t)(i))
	#define _cairo_uint64_to_uint128(i) ((uint128_t)(i))
	#define _cairo_uint128_to_uint64(i) ((uint64_t)(i))
	#define _cairo_uint128_to_uint32(i) ((uint32_t)(i))
	#define _cairo_uint128_add(a, b)     ((a) + (b))
	#define _cairo_uint128_sub(a, b)     ((a) - (b))
	#define _cairo_uint128_mul(a, b)     ((a) * (b))
	#define _cairo_uint64x64_128_mul(a, b)   ((uint128_t)(a) * (b))
	#define _cairo_uint128_lsl(a, b)     ((a) << (b))
	#define _cairo_uint128_rsl(a, b)     ((uint128_t)(a) >> (b))
	#define _cairo_uint128_rsa(a, b)     ((uint128_t)((int128_t)(a) >> (b)))
	#define _cairo_uint128_lt(a, b)      ((a) < (b))
	#define _cairo_uint128_cmp(a, b)     ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
	#define _cairo_uint128_is_zero(a)   ((a) == 0)
	#define _cairo_uint128_eq(a, b)      ((a) == (b))
	#define _cairo_uint128_negate(a)    ((uint128_t)-((int128_t)(a)))
	#define _cairo_uint128_negative(a)  ((int128_t)(a) < 0)
	#define _cairo_uint128_not(a)       (~(a))
	#define _cairo_uint128_to_int128(i) ((int128_t)(i))
	#define _cairo_int128_to_uint128(i) ((uint128_t)(i))
	#define _cairo_int32_to_int128(i)   ((int128_t)(i))
	#define _cairo_int64_to_int128(i)   ((int128_t)(i))
	#define _cairo_int128_to_int64(i)   ((int64_t)(i))
	#define _cairo_int128_to_int32(i)   ((int32_t)(i))
	#define _cairo_int128_add(a, b)      ((a) + (b))
	#define _cairo_int128_sub(a, b)      ((a) - (b))
	#define _cairo_int128_mul(a, b)      ((a) * (b))
	#define _cairo_int64x64_128_mul(a, b) ((int128_t)(a) * (b))
	#define _cairo_int64x32_128_mul(a, b) _cairo_int64x64_128_mul(a, _cairo_int32_to_int64(b))
	#define _cairo_int128_lt(a, b)       ((a) < (b))
	#define _cairo_int128_cmp(a, b)      ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
	#define _cairo_int128_is_zero(a)    ((a) == 0)
	#define _cairo_int128_eq(a, b)       ((a) == (b))
	#define _cairo_int128_lsl(a, b)      ((a) << (b))
	#define _cairo_int128_rsl(a, b)      ((int128_t)((uint128_t)(a) >> (b)))
	#define _cairo_int128_rsa(a, b)      ((int128_t)(a) >> (b))
	#define _cairo_int128_negate(a)     (-(a))
	#define _cairo_int128_negative(a)   ((a) < 0)
	#define _cairo_int128_not(a)        (~(a))
#endif // HAVE_UINT128_T 

cairo_uquorem128_t I _cairo_uint128_divrem(cairo_uint128_t num, cairo_uint128_t den);
cairo_quorem128_t I _cairo_int128_divrem(cairo_int128_t num, cairo_int128_t den);
cairo_uquorem64_t I _cairo_uint_96by64_32x64_divrem(cairo_uint128_t num, cairo_uint64_t den);
cairo_quorem64_t I _cairo_int_96by64_32x64_divrem(cairo_int128_t num, cairo_int64_t den);

#define _cairo_uint128_le(a, b)      (!_cairo_uint128_gt(a, b))
#define _cairo_uint128_ne(a, b)      (!_cairo_uint128_eq(a, b))
#define _cairo_uint128_ge(a, b)      (!_cairo_uint128_lt(a, b))
#define _cairo_uint128_gt(a, b)      _cairo_uint128_lt(b, a)
#define _cairo_int128_le(a, b)       (!_cairo_int128_gt(a, b))
#define _cairo_int128_ne(a, b)       (!_cairo_int128_eq(a, b))
#define _cairo_int128_ge(a, b)       (!_cairo_int128_lt(a, b))
#define _cairo_int128_gt(a, b)       _cairo_int128_lt(b, a)

#undef I
//
//#include "cairo-fixed-type-private.h"
// 
// Fixed-point configuration
// 
typedef int32_t		cairo_fixed_16_16_t;
typedef cairo_int64_t	cairo_fixed_32_32_t;
typedef cairo_int64_t	cairo_fixed_48_16_t;
typedef cairo_int128_t	cairo_fixed_64_64_t;
typedef cairo_int128_t	cairo_fixed_96_32_t;
// 
// Eventually, we should allow changing this, but I think
// there are some assumptions in the tessellator about the
// size of a fixed type.  For now, it must be 32.
// 
#define CAIRO_FIXED_BITS	32
// 
// The number of fractional bits.  Changing this involves
// making sure that you compute a double-to-fixed magic number (see below).
// 
#define CAIRO_FIXED_FRAC_BITS	8

typedef int32_t  cairo_fixed_t;          // A signed type %CAIRO_FIXED_BITS in size; the main fixed point type 
typedef uint32_t cairo_fixed_unsigned_t; // An unsigned type of the same size as #cairo_fixed_t 

struct cairo_point_t {
    cairo_fixed_t x;
    cairo_fixed_t y;
};
//
#include "cairo-fixed-private.h"
//
//#include "cairo-types-private.h"
//
CAIRO_BEGIN_DECLS
// 
// SECTION:cairo-types
// @Title: Types
// @Short_Description: Generic data types
// This section lists generic data types used in the cairo API.
// 
typedef struct _cairo_array cairo_array_t;
typedef struct _cairo_backend cairo_backend_t;
typedef struct _cairo_boxes_t cairo_boxes_t;
typedef struct _cairo_cache cairo_cache_t;
typedef struct _cairo_composite_rectangles cairo_composite_rectangles_t;
typedef struct _cairo_clip cairo_clip_t;
typedef struct _cairo_clip_path cairo_clip_path_t;
typedef struct _cairo_color cairo_color_t;
typedef struct _cairo_color_stop cairo_color_stop_t;
typedef struct _cairo_contour cairo_contour_t;
typedef struct _cairo_contour_chain cairo_contour_chain_t;
typedef struct _cairo_contour_iter cairo_contour_iter_t;
typedef struct _cairo_damage cairo_damage_t;
typedef struct _cairo_device_backend cairo_device_backend_t;
typedef struct _cairo_font_face_backend     cairo_font_face_backend_t;
typedef struct _cairo_gstate cairo_gstate_t;
typedef struct _cairo_gstate_backend cairo_gstate_backend_t;
typedef struct _cairo_glyph_text_info cairo_glyph_text_info_t;
typedef struct _cairo_hash_entry cairo_hash_entry_t;
typedef struct _cairo_hash_table cairo_hash_table_t;
typedef struct _cairo_image_surface cairo_image_surface_t;
typedef struct _cairo_mime_data cairo_mime_data_t;
typedef struct _cairo_observer cairo_observer_t;
typedef struct _cairo_output_stream cairo_output_stream_t;
typedef struct _cairo_paginated_surface_backend cairo_paginated_surface_backend_t;
typedef struct _cairo_path_fixed cairo_path_fixed_t;
typedef struct _cairo_rectangle_int16 cairo_glyph_size_t;
typedef struct _cairo_scaled_font_subsets cairo_scaled_font_subsets_t;
typedef struct _cairo_solid_pattern cairo_solid_pattern_t;
typedef struct _cairo_surface_attributes cairo_surface_attributes_t;
typedef struct _cairo_surface_backend cairo_surface_backend_t;
typedef struct _cairo_surface_observer cairo_surface_observer_t;
typedef struct _cairo_surface_snapshot cairo_surface_snapshot_t;
typedef struct _cairo_surface_subsurface cairo_surface_subsurface_t;
typedef struct _cairo_surface_wrapper cairo_surface_wrapper_t;
typedef struct _cairo_traps cairo_traps_t;
typedef struct _cairo_tristrip cairo_tristrip_t;
typedef struct _cairo_unscaled_font_backend cairo_unscaled_font_backend_t;
typedef struct _cairo_xlib_screen_info cairo_xlib_screen_info_t;
typedef cairo_array_t cairo_user_data_array_t;
typedef struct _cairo_scaled_font_private cairo_scaled_font_private_t;
typedef struct _cairo_scaled_font_backend   cairo_scaled_font_backend_t;
typedef struct _cairo_scaled_glyph cairo_scaled_glyph_t;
typedef struct _cairo_scaled_glyph_private cairo_scaled_glyph_private_t;
typedef struct cairo_compositor cairo_compositor_t;
typedef struct cairo_fallback_compositor cairo_fallback_compositor_t;
typedef struct cairo_mask_compositor cairo_mask_compositor_t;
typedef struct cairo_traps_compositor cairo_traps_compositor_t;
typedef struct cairo_spans_compositor cairo_spans_compositor_t;

struct _cairo_observer {
    cairo_list_t link;
    void (*callback) (cairo_observer_t *self, void *arg);
};
/**
 * _cairo_hash_entry:
 *
 * A #cairo_hash_entry_t contains both a key and a value for
 * #cairo_hash_table_t. User-derived types for #cairo_hash_entry_t must
 * be type-compatible with this structure (eg. they must have an
 * ulong as the first parameter. The easiest way to get this
 * is to use:
 *
 * 	typedef _my_entry {
 *	    cairo_hash_entry_t base;
 *	    ... Remainder of key and value fields here ..
 *	} my_entry_t;
 *
 * which then allows a pointer to my_entry_t to be passed to any of
 * the #cairo_hash_table_t functions as follows without requiring a cast:
 *
 *	_cairo_hash_table_insert (hash_table, &my_entry->base);
 *
 * IMPORTANT: The caller is responsible for initializing
 * my_entry->base.hash with a hash code derived from the key. The
 * essential property of the hash code is that keys_equal must never
 * return %TRUE for two keys that have different hashes. The best hash
 * code will reduce the frequency of two keys with the same code for
 * which keys_equal returns %FALSE.
 *
 * Which parts of the entry make up the "key" and which part make up
 * the value are entirely up to the caller, (as determined by the
 * computation going into base.hash as well as the keys_equal
 * function). A few of the #cairo_hash_table_t functions accept an entry
 * which will be used exclusively as a "key", (indicated by a
 * parameter name of key). In these cases, the value-related fields of
 * the entry need not be initialized if so desired.
 **/
struct _cairo_hash_entry {
    ulong hash;
};

struct _cairo_array {
    uint size;
    uint num_elements;
    uint element_size;
    char *elements;
};
/**
 * _cairo_lcd_filter:
 * @CAIRO_LCD_FILTER_DEFAULT: Use the default LCD filter for
 *   font backend and target device
 * @CAIRO_LCD_FILTER_NONE: Do not perform LCD filtering
 * @CAIRO_LCD_FILTER_INTRA_PIXEL: Intra-pixel filter
 * @CAIRO_LCD_FILTER_FIR3: FIR filter with a 3x3 kernel
 * @CAIRO_LCD_FILTER_FIR5: FIR filter with a 5x5 kernel
 *
 * The LCD filter specifies the low-pass filter applied to LCD-optimized
 * bitmaps generated with an antialiasing mode of %CAIRO_ANTIALIAS_SUBPIXEL.
 *
 * Note: This API was temporarily made available in the public
 * interface during the 1.7.x development series, but was made private
 * before 1.8.
 **/
typedef enum _cairo_lcd_filter {
    CAIRO_LCD_FILTER_DEFAULT,
    CAIRO_LCD_FILTER_NONE,
    CAIRO_LCD_FILTER_INTRA_PIXEL,
    CAIRO_LCD_FILTER_FIR3,
    CAIRO_LCD_FILTER_FIR5
} cairo_lcd_filter_t;

typedef enum _cairo_round_glyph_positions {
    CAIRO_ROUND_GLYPH_POS_DEFAULT,
    CAIRO_ROUND_GLYPH_POS_ON,
    CAIRO_ROUND_GLYPH_POS_OFF
} cairo_round_glyph_positions_t;

struct _cairo_font_options {
    cairo_antialias_t antialias;
    cairo_subpixel_order_t subpixel_order;
    cairo_lcd_filter_t lcd_filter;
    cairo_hint_style_t hint_style;
    cairo_hint_metrics_t hint_metrics;
    cairo_round_glyph_positions_t round_glyph_positions;
};

struct _cairo_glyph_text_info {
    const char *utf8;
    int utf8_len;
    const cairo_text_cluster_t *clusters;
    int num_clusters;
    cairo_text_cluster_flags_t cluster_flags;
};
// 
// XXX: Right now, the _cairo_color structure puts unpremultiplied
// color in the doubles and premultiplied color in the shorts. Yes,
// this is crazy insane, (but at least we don't export this
// madness). I'm still working on a cleaner API, but in the meantime,
// at least this does prevent precision loss in color when changing alpha.
//
struct _cairo_color {
    double red;
    double green;
    double blue;
    double alpha;
    ushort red_short;
    ushort green_short;
    ushort blue_short;
    ushort alpha_short;
};

struct _cairo_color_stop {
    // unpremultiplied 
    double red;
    double green;
    double blue;
    double alpha;
    // unpremultipled, for convenience 
    uint16 red_short;
    uint16 green_short;
    uint16 blue_short;
    uint16 alpha_short;
};

typedef enum _cairo_paginated_mode {
    CAIRO_PAGINATED_MODE_ANALYZE,	/* analyze page regions */
    CAIRO_PAGINATED_MODE_RENDER,	/* render page contents */
    CAIRO_PAGINATED_MODE_FALLBACK	/* paint fallback images */
} cairo_paginated_mode_t;

#ifndef __SLCAIRO_H // @sobolev {
typedef enum _cairo_internal_surface_type {
    CAIRO_INTERNAL_SURFACE_TYPE_SNAPSHOT = 0x1000,
    CAIRO_INTERNAL_SURFACE_TYPE_PAGINATED,
    CAIRO_INTERNAL_SURFACE_TYPE_ANALYSIS,
    CAIRO_INTERNAL_SURFACE_TYPE_OBSERVER,
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_FALLBACK,
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_PAGINATED,
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_WRAPPING,
    CAIRO_INTERNAL_SURFACE_TYPE_NULL,
    CAIRO_INTERNAL_SURFACE_TYPE_TYPE3_GLYPH
} cairo_internal_surface_type_t;
#endif // } @sobolev __SLCAIRO_H

typedef enum _cairo_internal_device_type {
    CAIRO_INTERNAL_DEVICE_TYPE_OBSERVER = 0x1000,
} cairo_device_surface_type_t;

#define CAIRO_HAS_TEST_PAGINATED_SURFACE 1

typedef struct _cairo_slope {
    cairo_fixed_t dx;
    cairo_fixed_t dy;
} cairo_slope_t, cairo_distance_t;

/*typedef struct _cairo_point_double {
    double x;
    double y;
} cairo_point_double_t_ReplacedWith_RPoint;
*/

typedef struct _cairo_circle_double {
    RPoint center;
    double radius;
} cairo_circle_double_t;

typedef struct _cairo_distance_double {
    double dx;
    double dy;
} cairo_distance_double_t;

typedef struct _cairo_box_double {
    RPoint p1;
    RPoint p2;
} cairo_box_double_t;

typedef struct _cairo_line {
    cairo_point_t p1;
    cairo_point_t p2;
} cairo_line_t, cairo_box_t;

typedef struct _cairo_trapezoid {
    cairo_fixed_t top;
	cairo_fixed_t bottom;
    cairo_line_t left;
	cairo_line_t right;
} cairo_trapezoid_t;

/* @sobolev (unused) typedef struct _cairo_point_int {
    int    x;
	int    y;
} cairo_point_int_t;*/

#define CAIRO_RECT_INT_MIN (INT_MIN >> CAIRO_FIXED_FRAC_BITS)
#define CAIRO_RECT_INT_MAX (INT_MAX >> CAIRO_FIXED_FRAC_BITS)

/* (used only for _cairo_arc_in_direction: replaced with arg reverse[0,1]) typedef enum _cairo_direction { CAIRO_DIRECTION_FORWARD, CAIRO_DIRECTION_REVERSE } cairo_direction_t;*/

typedef struct _cairo_edge {
    cairo_line_t line;
    int top;
	int bottom;
    int dir;
} cairo_edge_t;

typedef struct _cairo_polygon {
    cairo_status_t status;
    cairo_box_t extents;
    cairo_box_t limit;
    const cairo_box_t *limits;
    int num_limits;
    int num_edges;
    int edges_size;
    cairo_edge_t *edges;
    cairo_edge_t  edges_embedded[32];
} cairo_polygon_t;

typedef cairo_warn cairo_status_t (*cairo_spline_add_point_func_t) (void *closure, const cairo_point_t *point, const cairo_slope_t *tangent);

typedef struct _cairo_spline_knots {
    cairo_point_t a, b, c, d;
} cairo_spline_knots_t;

typedef struct _cairo_spline {
    cairo_spline_add_point_func_t add_point_func;
    void *closure;
    cairo_spline_knots_t knots;
    cairo_slope_t initial_slope;
    cairo_slope_t final_slope;
    cairo_bool_t has_point;
    cairo_point_t last_point;
} cairo_spline_t;

typedef struct _cairo_pen_vertex {
    cairo_point_t point;
    cairo_slope_t slope_ccw;
    cairo_slope_t slope_cw;
} cairo_pen_vertex_t;

typedef struct _cairo_pen {
    double radius;
    double tolerance;
    int num_vertices;
    cairo_pen_vertex_t *vertices;
    cairo_pen_vertex_t  vertices_embedded[32];
} cairo_pen_t;

typedef struct _cairo_stroke_style {
    double		 line_width;
    cairo_line_cap_t	 line_cap;
    cairo_line_join_t	 line_join;
    double		 miter_limit;
    double		*dash;
    uint	 num_dashes;
    double		 dash_offset;
} cairo_stroke_style_t;

typedef struct _cairo_format_masks {
    int bpp;
    ulong alpha_mask;
    ulong red_mask;
    ulong green_mask;
    ulong blue_mask;
} cairo_format_masks_t;

typedef enum {
    CAIRO_STOCK_WHITE,
    CAIRO_STOCK_BLACK,
    CAIRO_STOCK_TRANSPARENT,
    CAIRO_STOCK_NUM_COLORS,
} cairo_stock_t;

typedef enum _cairo_image_transparency {
    CAIRO_IMAGE_IS_OPAQUE,
    CAIRO_IMAGE_HAS_BILEVEL_ALPHA,
    CAIRO_IMAGE_HAS_ALPHA,
    CAIRO_IMAGE_UNKNOWN
} cairo_image_transparency_t;

typedef enum _cairo_image_color {
    CAIRO_IMAGE_IS_COLOR,
    CAIRO_IMAGE_IS_GRAYSCALE,
    CAIRO_IMAGE_IS_MONOCHROME,
    CAIRO_IMAGE_UNKNOWN_COLOR
} cairo_image_color_t;

struct _cairo_mime_data {
    cairo_reference_count_t ref_count;
    uchar *data;
    ulong length;
    cairo_destroy_func_t destroy;
    void *closure;
};
// 
// A #cairo_unscaled_font_t is just an opaque handle we use in the glyph cache.
// 
struct cairo_unscaled_font_t {
    cairo_hash_entry_t hash_entry;
    cairo_reference_count_t ref_count;
    const cairo_unscaled_font_backend_t	* backend;
};
//
//#include "cairo-error-private.h"
//
// _cairo_int_status: internal status
// 
// Sure wish C had a real enum type so that this would be distinct
// from #cairo_status_t. Oh well, without that, I'll use this bogus 100
// offset.  We want to keep it fit in int8 as the compiler may choose that for #cairo_status_t
// 
#ifndef __SLCAIRO_H // @sobolev {
enum _cairo_int_status {
	CAIRO_INT_STATUS_SUCCESS = 0,
	CAIRO_INT_STATUS_NO_MEMORY,
	CAIRO_INT_STATUS_INVALID_RESTORE,
	CAIRO_INT_STATUS_INVALID_POP_GROUP,
	CAIRO_INT_STATUS_NO_CURRENT_POINT,
	CAIRO_INT_STATUS_INVALID_MATRIX,
	CAIRO_INT_STATUS_INVALID_STATUS,
	CAIRO_INT_STATUS_NULL_POINTER,
	CAIRO_INT_STATUS_INVALID_STRING,
	CAIRO_INT_STATUS_INVALID_PATH_DATA,
	CAIRO_INT_STATUS_READ_ERROR,
	CAIRO_INT_STATUS_WRITE_ERROR,
	CAIRO_INT_STATUS_SURFACE_FINISHED,
	CAIRO_INT_STATUS_SURFACE_TYPE_MISMATCH,
	CAIRO_INT_STATUS_PATTERN_TYPE_MISMATCH,
	CAIRO_INT_STATUS_INVALID_CONTENT,
	CAIRO_INT_STATUS_INVALID_FORMAT,
	CAIRO_INT_STATUS_INVALID_VISUAL,
	CAIRO_INT_STATUS_FILE_NOT_FOUND,
	CAIRO_INT_STATUS_INVALID_DASH,
	CAIRO_INT_STATUS_INVALID_DSC_COMMENT,
	CAIRO_INT_STATUS_INVALID_INDEX,
	CAIRO_INT_STATUS_CLIP_NOT_REPRESENTABLE,
	CAIRO_INT_STATUS_TEMP_FILE_ERROR,
	CAIRO_INT_STATUS_INVALID_STRIDE,
	CAIRO_INT_STATUS_FONT_TYPE_MISMATCH,
	CAIRO_INT_STATUS_USER_FONT_IMMUTABLE,
	CAIRO_INT_STATUS_USER_FONT_ERROR,
	CAIRO_INT_STATUS_NEGATIVE_COUNT,
	CAIRO_INT_STATUS_INVALID_CLUSTERS,
	CAIRO_INT_STATUS_INVALID_SLANT,
	CAIRO_INT_STATUS_INVALID_WEIGHT,
	CAIRO_INT_STATUS_INVALID_SIZE,
	CAIRO_INT_STATUS_USER_FONT_NOT_IMPLEMENTED,
	CAIRO_INT_STATUS_DEVICE_TYPE_MISMATCH,
	CAIRO_INT_STATUS_DEVICE_ERROR,
	CAIRO_INT_STATUS_INVALID_MESH_CONSTRUCTION,
	CAIRO_INT_STATUS_DEVICE_FINISHED,
	CAIRO_INT_STATUS_JBIG2_GLOBAL_MISSING,

	CAIRO_INT_STATUS_LAST_STATUS,

	CAIRO_INT_STATUS_UNSUPPORTED = 100,
	CAIRO_INT_STATUS_DEGENERATE,
	CAIRO_INT_STATUS_NOTHING_TO_DO,
	CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY,
	CAIRO_INT_STATUS_IMAGE_FALLBACK,
	CAIRO_INT_STATUS_ANALYZE_RECORDING_SURFACE_PATTERN,
};

typedef enum _cairo_int_status cairo_int_status_t;
#endif // } @sobolev __SLCAIRO_H

#define _cairo_status_is_error(status) (status != CAIRO_STATUS_SUCCESS && status < CAIRO_STATUS_LAST_STATUS)
#define _cairo_int_status_is_error(status) (status != CAIRO_INT_STATUS_SUCCESS && status < CAIRO_INT_STATUS_LAST_STATUS)

cairo_private cairo_status_t FASTCALL _cairo_error(cairo_status_t status);

// hide compiler warnings when discarding the return value 
#define _cairo_error_throw(status) do {	cairo_status_t status__ = _cairo_error(status);	(void)status__;	} while(0)
//
//#include "cairo-private.h"
//
struct _cairo {
    cairo_reference_count_t ref_count;
    cairo_status_t status;
    cairo_user_data_array_t user_data;
    const cairo_backend_t *backend;
};

cairo_private cairo_t * _cairo_create_in_error (cairo_status_t status);
cairo_private void _cairo_init(cairo_t *cr, const cairo_backend_t *backend);
cairo_private void _cairo_fini(cairo_t *cr);

CAIRO_END_DECLS
//
//#include "cairo-backend-private.h"
//
typedef enum _cairo_backend_type {
	CAIRO_TYPE_DEFAULT,
	CAIRO_TYPE_SKIA,
} cairo_backend_type_t;

struct _cairo_backend {
	cairo_backend_type_t type;
	void (* destroy)(void * cr);
	cairo_surface_t *(*get_original_target)(void* cr);
	cairo_surface_t *(*get_current_target)(void* cr);
	cairo_status_t (*save)(void * cr);
	cairo_status_t (*restore)(void * cr);
	cairo_status_t (*push_group)(void * cr, cairo_content_t content);
	cairo_pattern_t *(*pop_group)(void* cr);
	cairo_status_t (*set_source_rgba)(void * cr, double red, double green, double blue, double alpha);
	cairo_status_t (*set_source_surface)(void * cr, cairo_surface_t * surface, double x, double y);
	cairo_status_t (*set_source)(void * cr, cairo_pattern_t * source);
	cairo_pattern_t *(*get_source)(void* cr);
	cairo_status_t (*set_antialias)(void * cr, cairo_antialias_t antialias);
	cairo_status_t (*set_dash)(void * cr, const double * dashes, int num_dashes, double offset);
	cairo_status_t (*set_fill_rule)(void * cr, CairoFillRule fill_rule);
	cairo_status_t (*set_line_cap)(void * cr, cairo_line_cap_t line_cap);
	cairo_status_t (*set_line_join)(void * cr, cairo_line_join_t line_join);
	cairo_status_t (*set_line_width)(void * cr, double line_width);
	cairo_status_t (*set_miter_limit)(void * cr, double limit);
	cairo_status_t (*set_opacity)(void * cr, double opacity);
	cairo_status_t (*set_operator)(void * cr, cairo_operator_t op);
	cairo_status_t (*set_tolerance)(void * cr, double tolerance);
	cairo_antialias_t (* get_antialias)(void * cr);
	void (* get_dash)(void * cr, double * dashes, int * num_dashes, double * offset);
	CairoFillRule (* get_fill_rule)(void * cr);
	cairo_line_cap_t (* get_line_cap)(void * cr);
	cairo_line_join_t (* get_line_join)(void * cr);
	double (* get_line_width)(void * cr);
	double (* get_miter_limit)(void * cr);
	double (* get_opacity)(void * cr);
	cairo_operator_t (* get_operator)(void * cr);
	double (* get_tolerance)(void * cr);
	cairo_status_t (* translate)(void * cr, double tx, double ty);
	cairo_status_t (* scale)(void * cr, double sx, double sy);
	cairo_status_t (* rotate)(void * cr, double theta);
	cairo_status_t (* transform)(void * cr, const cairo_matrix_t * matrix);
	cairo_status_t (* set_matrix)(void * cr, const cairo_matrix_t * matrix);
	cairo_status_t (* set_identity_matrix)(void * cr);
	void (* get_matrix)(void * cr, cairo_matrix_t * matrix);
	void (* user_to_device)(void * cr, double * x, double * y);
	void (* user_to_device_distance)(void * cr, double * x, double * y);
	void (* device_to_user)(void * cr, double * x, double * y);
	void (* device_to_user_distance)(void * cr, double * x, double * y);
	void (* user_to_backend)(void * cr, double * x, double * y);
	void (* user_to_backend_distance)(void * cr, double * x, double * y);
	void (* backend_to_user)(void * cr, double * x, double * y);
	void (* backend_to_user_distance)(void * cr, double * x, double * y);
	cairo_status_t (* new_path)(void * cr);
	cairo_status_t (* new_sub_path)(void * cr);
	cairo_status_t (* move_to)(void * cr, double x, double y);
	cairo_status_t (* rel_move_to)(void * cr, double dx, double dy);
	cairo_status_t (* line_to)(void * cr, double x, double y);
	cairo_status_t (* rel_line_to)(void * cr, double dx, double dy);
	cairo_status_t (* curve_to)(void * cr, double x1, double y1, double x2, double y2, double x3, double y3);
	cairo_status_t (* rel_curve_to)(void * cr, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3);
	cairo_status_t (* arc_to)(void * cr, double x1, double y1, double x2, double y2, double radius);
	cairo_status_t (* rel_arc_to)(void * cr, double dx1, double dy1, double dx2, double dy2, double radius);
	cairo_status_t (* close_path)(void * cr);
	cairo_status_t (* arc)(void * cr, double xc, double yc, double radius, double angle1, double angle2, cairo_bool_t forward);
	cairo_status_t (* rectangle)(void * cr, double x, double y, double width, double height);
	void (* path_extents)(void * cr, double * x1, double * y1, double * x2, double * y2);
	cairo_bool_t (* has_current_point)(void * cr);
	cairo_bool_t (* get_current_point)(void * cr, double * x, double * y);
	cairo_path_t *(*copy_path)(void* cr);
	cairo_path_t *(*copy_path_flat)(void* cr);
	cairo_status_t (* append_path)(void * cr, const cairo_path_t * path);
	cairo_status_t (* stroke_to_path)(void * cr);
	cairo_status_t (* clip)(void * cr);
	cairo_status_t (* clip_preserve)(void * cr);
	cairo_status_t (* in_clip)(void * cr, double x, double y, cairo_bool_t * inside);
	cairo_status_t (* clip_extents)(void * cr, double * x1, double * y1, double * x2, double * y2);
	cairo_status_t (* reset_clip)(void * cr);
	cairo_rectangle_list_t *(*clip_copy_rectangle_list)(void* cr);
	cairo_status_t (* paint)(void * cr);
	cairo_status_t (* paint_with_alpha)(void * cr, double opacity);
	cairo_status_t (* mask)(void * cr, cairo_pattern_t * pattern);
	cairo_status_t (* stroke)(void * cr);
	cairo_status_t (* stroke_preserve)(void * cr);
	cairo_status_t (* in_stroke)(void * cr, double x, double y, cairo_bool_t * inside);
	cairo_status_t (* stroke_extents)(void * cr, double * x1, double * y1, double * x2, double * y2);
	cairo_status_t (* fill)(void * cr);
	cairo_status_t (* fill_preserve)(void * cr);
	cairo_status_t (* in_fill)(void * cr, double x, double y, cairo_bool_t * inside);
	cairo_status_t (* fill_extents)(void * cr, double * x1, double * y1, double * x2, double * y2);
	cairo_status_t (* set_font_face)(void * cr, cairo_font_face_t * font_face);
	cairo_font_face_t *(*get_font_face)(void* cr);
	cairo_status_t (* set_font_size)(void * cr, double size);
	cairo_status_t (* set_font_matrix)(void * cr, const cairo_matrix_t * matrix);
	void (* get_font_matrix)(void * cr, cairo_matrix_t * matrix);
	cairo_status_t (* set_font_options)(void * cr, const cairo_font_options_t * options);
	void (* get_font_options)(void * cr, cairo_font_options_t * options);
	cairo_status_t (* set_scaled_font)(void * cr, cairo_scaled_font_t * scaled_font);
	cairo_scaled_font_t *(*get_scaled_font)(void* cr);
	cairo_status_t (* font_extents)(void * cr, cairo_font_extents_t * extents);
	cairo_status_t (* glyphs)(void * cr, const cairo_glyph_t * glyphs, int num_glyphs, cairo_glyph_text_info_t * info);
	cairo_status_t (* glyph_path)(void * cr, const cairo_glyph_t * glyphs, int num_glyphs);
	cairo_status_t (* glyph_extents)(void * cr, const cairo_glyph_t * glyphs, int num_glyphs, cairo_text_extents_t * extents);
	cairo_status_t (* copy_page)(void * cr);
	cairo_status_t (* show_page)(void * cr);
};

static inline void _cairo_backend_to_user(cairo_t * cr, double * x, double * y)
{
	cr->backend->backend_to_user(cr, x, y);
}
static inline void _cairo_backend_to_user_distance(cairo_t * cr, double * x, double * y)
{
	cr->backend->backend_to_user_distance(cr, x, y);
}
static inline void _cairo_user_to_backend(cairo_t * cr, double * x, double * y)
{
	cr->backend->user_to_backend(cr, x, y);
}
static inline void _cairo_user_to_backend_distance(cairo_t * cr, double * x, double * y)
{
	cr->backend->user_to_backend_distance(cr, x, y);
}
//
//#include "cairo-cache-private.h"
//
/**
 * _cairo_cache_entry:
 *
 * A #cairo_cache_entry_t contains both a key and a value for
 * #cairo_cache_t. User-derived types for #cairo_cache_entry_t must
 * have a #cairo_cache_entry_t as their first field. For example:
 *
 *      typedef _my_entry {
 *	    cairo_cache_entry_t base;
 *	    ... Remainder of key and value fields here ..
 *	} my_entry_t;
 *
 * which then allows a pointer to my_entry_t to be passed to any of
 * the #cairo_cache_t functions as follows without requiring a cast:
 *
 *	_cairo_cache_insert (cache, &my_entry->base, size);
 *
 * IMPORTANT: The caller is responsible for initializing
 * my_entry->base.hash with a hash code derived from the key.  The
 * essential property of the hash code is that keys_equal must never
 * return %TRUE for two keys that have different hashes. The best hash
 * code will reduce the frequency of two keys with the same code for
 * which keys_equal returns %FALSE.
 *
 * The user must also initialize my_entry->base.size to indicate
 * the size of the current entry. What units to use for size is
 * entirely up to the caller, (though the same units must be used for
 * the max_size parameter passed to _cairo_cache_create()). If all
 * entries are close to the same size, the simplest thing to do is to
 * just use units of "entries", (eg. set size==1 in all entries and
 * set max_size to the number of entries which you want to be saved
 * in the cache).
 *
 * Which parts of the entry make up the "key" and which part make up
 * the value are entirely up to the caller, (as determined by the
 * computation going into base.hash as well as the keys_equal
 * function). A few of the #cairo_cache_t functions accept an entry which
 * will be used exclusively as a "key", (indicated by a parameter name
 * of key). In these cases, the value-related fields of the entry need
 * not be initialized if so desired.
 **/
typedef struct _cairo_cache_entry {
	ulong hash;
	ulong size;
} cairo_cache_entry_t;

typedef cairo_bool_t (*cairo_cache_predicate_func_t)(const void * entry);

struct _cairo_cache {
	cairo_hash_table_t * hash_table;
	cairo_cache_predicate_func_t predicate;
	cairo_destroy_func_t entry_destroy;
	ulong max_size;
	ulong size;
	int freeze_count;
};

typedef cairo_bool_t (*cairo_cache_keys_equal_func_t)(const void * key_a, const void * key_b);
typedef void (*cairo_cache_callback_func_t)(void * entry, void * closure);

cairo_private cairo_status_t _cairo_cache_init(cairo_cache_t * cache, cairo_cache_keys_equal_func_t keys_equal, cairo_cache_predicate_func_t predicate, cairo_destroy_func_t entry_destroy, ulong max_size);
cairo_private void _cairo_cache_fini(cairo_cache_t * cache);
cairo_private void _cairo_cache_freeze(cairo_cache_t * cache);
cairo_private void _cairo_cache_thaw(cairo_cache_t * cache);
cairo_private void * _cairo_cache_lookup(cairo_cache_t * cache, cairo_cache_entry_t  * key);
cairo_private cairo_status_t FASTCALL _cairo_cache_insert(cairo_cache_t * cache, cairo_cache_entry_t * entry);
cairo_private void FASTCALL _cairo_cache_remove(cairo_cache_t * cache, cairo_cache_entry_t * entry);
cairo_private void _cairo_cache_foreach(cairo_cache_t * cache, cairo_cache_callback_func_t cache_callback, void * closure);
//
//#include "cairo-spans-private.h"
//
// Number of bits of precision used for alpha
#define CAIRO_SPANS_UNIT_COVERAGE_BITS 8
#define CAIRO_SPANS_UNIT_COVERAGE ((1 << CAIRO_SPANS_UNIT_COVERAGE_BITS)-1)
//
// A structure representing an open-ended horizontal span of constant pixel coverage. 
//
typedef struct _cairo_half_open_span {
	int32_t x; /* The inclusive x-coordinate of the start of the span. */
	uint8 coverage; /* The pixel coverage for the pixels to the right. */
	uint8 inverse; /* between regular mask and clip */
} cairo_half_open_span_t;
//
// Span renderer interface. Instances of renderers are provided by
// surfaces if they want to composite spans instead of trapezoids. 
//
typedef struct _cairo_span_renderer cairo_span_renderer_t;
struct _cairo_span_renderer {
	cairo_status_t status; // Private status variable
	cairo_destroy_func_t destroy; // Called to destroy the renderer
	// Render the spans on row y of the destination by whatever compositing method is required. 
	cairo_status_t (* render_rows)(void * abstract_renderer, int y, int height, const cairo_half_open_span_t * coverages, unsigned num_coverages);
	// Called after all rows have been rendered to perform whatever
	// final rendering step is required.  This function is called just
	// once before the renderer is destroyed. 
	cairo_status_t (* finish)(void * abstract_renderer);
};
//
// Scan converter interface
//
typedef struct _cairo_scan_converter cairo_scan_converter_t;

struct _cairo_scan_converter {
	cairo_destroy_func_t destroy; // Destroy this scan converter
	// Generates coverage spans for rows for the added edges and calls
	// the renderer function for each row. After generating spans the
	// only valid thing to do with the converter is to destroy it. 
	cairo_status_t (* generate)(void * abstract_converter, cairo_span_renderer_t   * renderer);
	cairo_status_t status; // Private status. Read with _cairo_scan_converter_status(). 
};
// 
// Scan converter constructors
// 
cairo_private cairo_scan_converter_t * _cairo_tor_scan_converter_create(int xmin, int ymin, int xmax, int ymax, CairoFillRule fill_rule, cairo_antialias_t antialias);
cairo_private cairo_status_t _cairo_tor_scan_converter_add_polygon(void * converter, const cairo_polygon_t * polygon);
cairo_private cairo_scan_converter_t * _cairo_tor22_scan_converter_create(int xmin, int ymin, int xmax, int ymax, CairoFillRule fill_rule, cairo_antialias_t antialias);
cairo_private cairo_status_t _cairo_tor22_scan_converter_add_polygon(void * converter, const cairo_polygon_t * polygon);
cairo_private cairo_scan_converter_t * _cairo_mono_scan_converter_create(int xmin, int ymin, int xmax, int ymax, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_mono_scan_converter_add_polygon(void * converter, const cairo_polygon_t * polygon);
cairo_private cairo_scan_converter_t * _cairo_clip_tor_scan_converter_create(cairo_clip_t * clip, cairo_polygon_t * polygon, CairoFillRule fill_rule, cairo_antialias_t antialias);

typedef struct _cairo_rectangular_scan_converter {
	cairo_scan_converter_t base;
	cairo_box_t extents;
	struct _cairo_rectangular_scan_converter_chunk {
		struct _cairo_rectangular_scan_converter_chunk * next;
		void * base;
		int count;
		int size;
	} chunks, * tail;
	char buf[CAIRO_STACK_BUFFER_SIZE];
	int num_rectangles;
} cairo_rectangular_scan_converter_t;

cairo_private void _cairo_rectangular_scan_converter_init(cairo_rectangular_scan_converter_t * self, const CairoIRect * extents);
cairo_private cairo_status_t _cairo_rectangular_scan_converter_add_box(cairo_rectangular_scan_converter_t * self, const cairo_box_t * box, int dir);

typedef struct _cairo_botor_scan_converter {
	cairo_scan_converter_t base;
	cairo_box_t extents;
	CairoFillRule fill_rule;
	int xmin, xmax;
	struct _cairo_botor_scan_converter_chunk {
		_cairo_botor_scan_converter::_cairo_botor_scan_converter_chunk * next;
		void * base;
		int count;
		int size;
	} chunks, * tail;
	char buf[CAIRO_STACK_BUFFER_SIZE];
	int num_edges;
} cairo_botor_scan_converter_t;

cairo_private void _cairo_botor_scan_converter_init(cairo_botor_scan_converter_t * self, const cairo_box_t * extents, CairoFillRule fill_rule);
// 
// cairo-spans.c:
// 
cairo_private cairo_scan_converter_t * _cairo_scan_converter_create_in_error(cairo_status_t error);
cairo_private cairo_status_t _cairo_scan_converter_status(void * abstract_converter);
cairo_private cairo_status_t _cairo_scan_converter_set_error(void * abstract_converter, cairo_status_t error);
cairo_private cairo_span_renderer_t * _cairo_span_renderer_create_in_error(cairo_status_t error);
cairo_private cairo_status_t _cairo_span_renderer_status(void * abstract_renderer);
// 
// Set the renderer into an error state.  This sets all the method
// pointers except ->destroy() of the renderer to no-op
// implementations that just return the error status. 
// 
cairo_private cairo_status_t _cairo_span_renderer_set_error(void * abstract_renderer, cairo_status_t error);
cairo_private cairo_status_t _cairo_surface_composite_polygon(cairo_surface_t * surface, cairo_operator_t op,
    const cairo_pattern_t * pattern, CairoFillRule fill_rule, cairo_antialias_t antialias,
    const cairo_composite_rectangles_t * rects, cairo_polygon_t * polygon, cairo_region_t * clip_region);
//
//#include "cairo-surface-private.h"
//
typedef void (*cairo_surface_func_t)(cairo_surface_t *);

struct cairo_surface_t {
	const cairo_surface_backend_t * backend;
	cairo_device_t * device;
	// We allow surfaces to override the backend->type by shoving something
	// else into surface->type. This is for "wrapper" surfaces that want to
	// hide their internal type from the user-level API.
	cairo_surface_type_t type;
	cairo_content_t content;
	cairo_reference_count_t ref_count;
	cairo_status_t status;
	uint unique_id;
	uint serial;
	cairo_damage_t * damage;
	unsigned _finishing       : 1;
	unsigned finished         : 1;
	unsigned is_clear         : 1;
	unsigned has_font_options : 1;
	unsigned owns_device      : 1;
	cairo_user_data_array_t user_data;
	cairo_user_data_array_t mime_data;
	cairo_matrix_t device_transform;
	cairo_matrix_t device_transform_inverse;
	cairo_list_t device_transform_observers;
	// The actual resolution of the device, in dots per inch.
	double x_resolution;
	double y_resolution;
	// The resolution that should be used when generating image-based
	// fallback; generally only used by the analysis/paginated surfaces
	double x_fallback_resolution;
	double y_fallback_resolution;
	// A "snapshot" surface is immutable. See _cairo_surface_snapshot
	cairo_surface_t * snapshot_of;
	cairo_surface_func_t snapshot_detach;
	cairo_list_t snapshots; // current snapshots of this surface
	cairo_list_t snapshot; // place upon snapshot list 
	// 
	// Surface font options, falling back to backend's default options,
	// and set using _cairo_surface_set_font_options(), and propagated by
	// cairo_surface_create_similar().
	// 
	cairo_font_options_t font_options;
};

cairo_private cairo_surface_t * FASTCALL _cairo_surface_create_in_error(cairo_status_t status);
cairo_private cairo_surface_t * _cairo_int_surface_create_in_error(cairo_int_status_t status);
cairo_private cairo_surface_t * FASTCALL _cairo_surface_get_source(cairo_surface_t * surface, CairoIRect * extents);
cairo_private cairo_status_t FASTCALL _cairo_surface_flush(cairo_surface_t * surface, unsigned flags);
//
//#include "cairo-surface-backend-private.h"
//
CAIRO_BEGIN_DECLS

struct _cairo_surface_backend {
	cairo_surface_type_t type;
	cairo_warn cairo_status_t (* finish)(void * surface);
	cairo_t * (*create_context)(void* surface);
	cairo_surface_t * (*create_similar)(void * surface, cairo_content_t content, int width, int height);
	cairo_surface_t * (*create_similar_image)(void * surface, cairo_format_t format, int width, int height);
	cairo_image_surface_t * (*map_to_image)(void * surface, const CairoIRect  *extents);
	cairo_int_status_t (* unmap_image)(void * surface, cairo_image_surface_t  * image);
	cairo_surface_t * (*source)(void * abstract_surface, CairoIRect  *extents);
	cairo_warn cairo_status_t (* acquire_source_image)(void * abstract_surface, cairo_image_surface_t  ** image_out, void ** image_extra);
	cairo_warn void (* release_source_image)(void * abstract_surface, cairo_image_surface_t  * image_out, void * image_extra);
	cairo_surface_t * (*snapshot)(void* surface);
	cairo_warn cairo_int_status_t (* copy_page)(void * surface);
	cairo_warn cairo_int_status_t (* show_page)(void * surface);
	// 
	// Get the extents of the current surface. For many surface types
	// this will be as simple as { x=0, y=0, width=surface->width, height=surface->height}.
	// 
	// If this function is not implemented, or if it returns
	// FALSE the surface is considered to be boundless and infinite bounds are used for it.
	// 
	cairo_bool_t (* get_extents)(void * surface, CairoIRect   * extents);
	void (* get_font_options)(void * surface, cairo_font_options_t  * options);
	cairo_warn cairo_status_t (* flush)(void * surface, unsigned flags);
	cairo_warn cairo_status_t (* mark_dirty_rectangle)(void * surface, int x, int y, int width, int height);
	cairo_warn cairo_int_status_t (* paint)(void * surface, cairo_operator_t op, const cairo_pattern_t  * source, const cairo_clip_t * clip);
	cairo_warn cairo_int_status_t (* mask)(void * surface, cairo_operator_t op, const cairo_pattern_t  * source, const cairo_pattern_t  * mask, const cairo_clip_t * clip);
	cairo_warn cairo_int_status_t (* stroke)(void * surface, cairo_operator_t op, const cairo_pattern_t  * source,
	    const cairo_path_fixed_t * path, const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse,
	    double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
	cairo_warn cairo_int_status_t (* fill)(void * surface, cairo_operator_t op, const cairo_pattern_t  * source,
	    const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
	cairo_warn cairo_int_status_t (* fill_stroke)(void * surface, cairo_operator_t fill_op, const cairo_pattern_t * fill_source,
	    CairoFillRule fill_rule, double fill_tolerance, cairo_antialias_t fill_antialias, const cairo_path_fixed_t* path,
	    cairo_operator_t stroke_op, const cairo_pattern_t  * stroke_source, const cairo_stroke_style_t * stroke_style,
	    const cairo_matrix_t * stroke_ctm, const cairo_matrix_t * stroke_ctm_inverse, double stroke_tolerance, cairo_antialias_t stroke_antialias, const cairo_clip_t * clip);
	cairo_warn cairo_int_status_t (* show_glyphs)(void * surface, cairo_operator_t op,
	    const cairo_pattern_t  * source, cairo_glyph_t * glyphs, int num_glyphs, cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);
	cairo_bool_t (* has_show_text_glyphs)(void * surface);
	cairo_warn cairo_int_status_t (* show_text_glyphs)(void * surface, cairo_operator_t op,
	    const cairo_pattern_t * source, const char * utf8, int utf8_len, cairo_glyph_t * glyphs,
	    int num_glyphs, const cairo_text_cluster_t * clusters, int num_clusters, cairo_text_cluster_flags_t cluster_flags,
	    cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);
	const char ** (*get_supported_mime_types)(void* surface);
};

cairo_private cairo_status_t _cairo_surface_default_acquire_source_image(void * surface, cairo_image_surface_t  ** image_out, void ** image_extra);
cairo_private void _cairo_surface_default_release_source_image(void * surface, cairo_image_surface_t  * image, void * image_extra);
cairo_private cairo_surface_t * _cairo_surface_default_source(void * surface, CairoIRect * extents);
//
//#include "cairo-surface-inline.h"
//
static inline cairo_status_t __cairo_surface_flush(cairo_surface_t * surface, unsigned flags)
{
	return (surface->backend->flush) ? surface->backend->flush(surface, flags) : CAIRO_STATUS_SUCCESS;
}

static inline cairo_surface_t * _cairo_surface_reference(cairo_surface_t * surface)
{
	if(!CAIRO_REFERENCE_COUNT_IS_INVALID(&surface->ref_count))
		_cairo_reference_count_inc(&surface->ref_count);
	return surface;
}
//
//#include "cairo-path-private.h"
//
cairo_private cairo_path_t * _cairo_path_create(cairo_path_fixed_t * path, cairo_t * cr);
cairo_private cairo_path_t * _cairo_path_create_flat(cairo_path_fixed_t * path, cairo_t * cr);
cairo_private cairo_path_t * _cairo_path_create_in_error(cairo_status_t status);
cairo_private cairo_status_t _cairo_path_append_to_context(const cairo_path_t * path, cairo_t * cr);
//
//#include "cairo-image-surface-private.h"
//
// The canonical image backend 
//
struct _cairo_image_surface {
	cairo_surface_t base;
	pixman_image_t * pixman_image;
	const cairo_compositor_t * compositor;
	/* Parenting is tricky wrt lifetime tracking...
	 *
	 * One use for tracking the parent of an image surface is for
	 * create_similar_image() where we wish to create a device specific
	 * surface but return an image surface to the user. In such a case,
	 * the image may be owned by the device specific surface, its parent,
	 * but the user lifetime tracking is then performed on the image. So
	 * when the image is then finalized we call cairo_surface_destroy()
	 * on the parent. However, for normal usage where the lifetime tracking
	 * is done on the parent surface, we need to be careful to unhook
	 * the image->parent pointer before finalizing the image.
	 */
	cairo_surface_t * parent;
	pixman_format_code_t pixman_format;
	cairo_format_t format;
	uchar * data;
	int    width;
	int    height;
	int    stride;
	int    depth;
	unsigned owns_data    : 1;
	unsigned transparency : 2;
	unsigned color        : 2;
};

#define to_image_surface(S) ((cairo_image_surface_t*)(S))
//
// A wrapper for holding pixman images returned by create_for_pattern 
//
typedef struct _cairo_image_source {
	cairo_surface_t base;
	pixman_image_t * pixman_image;
	unsigned is_opaque_solid : 1;
} cairo_image_source_t;

cairo_private extern const cairo_surface_backend_t _cairo_image_surface_backend;
cairo_private extern const cairo_surface_backend_t _cairo_image_source_backend;

cairo_private const cairo_compositor_t * _cairo_image_mask_compositor_get(void);
cairo_private const cairo_compositor_t * _cairo_image_traps_compositor_get(void);
cairo_private const cairo_compositor_t * _cairo_image_spans_compositor_get(void);

#define _cairo_image_default_compositor_get _cairo_image_spans_compositor_get

cairo_private cairo_int_status_t _cairo_image_surface_paint(void * abstract_surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_image_surface_mask(void * abstract_surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_image_surface_stroke(void * abstract_surface, cairo_operator_t op, const cairo_pattern_t * source,
    const cairo_path_fixed_t * path, const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse,
    double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_image_surface_fill(void * abstract_surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_path_fixed_t * path,
    CairoFillRule fill_rule, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_image_surface_glyphs(void * abstract_surface,
    cairo_operator_t op, const cairo_pattern_t * source, cairo_glyph_t * glyphs,
    int num_glyphs, cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);
cairo_private void _cairo_image_surface_init(cairo_image_surface_t * surface, pixman_image_t * pixman_image, pixman_format_code_t pixman_format);
cairo_private cairo_surface_t * _cairo_image_surface_create_similar(void * abstract_other, cairo_content_t content, int width, int height);
cairo_private cairo_image_surface_t * _cairo_image_surface_map_to_image(void * abstract_other, const CairoIRect * extents);
cairo_private cairo_int_status_t _cairo_image_surface_unmap_image(void * abstract_surface, cairo_image_surface_t * image);
cairo_private cairo_surface_t * _cairo_image_surface_source(void * abstract_surface, CairoIRect * extents);
cairo_private cairo_status_t _cairo_image_surface_acquire_source_image(void * abstract_surface, cairo_image_surface_t ** image_out, void ** image_extra);
cairo_private void _cairo_image_surface_release_source_image(void * abstract_surface, cairo_image_surface_t * image, void * image_extra);
cairo_private cairo_surface_t * _cairo_image_surface_snapshot(void * abstract_surface);
cairo_private_no_warn cairo_bool_t _cairo_image_surface_get_extents(void * abstract_surface, CairoIRect * rectangle);
cairo_private void _cairo_image_surface_get_font_options(void * abstract_surface, cairo_font_options_t * options);
cairo_private cairo_surface_t * _cairo_image_source_create_for_pattern(cairo_surface_t * dst,
    const cairo_pattern_t * pattern, cairo_bool_t is_mask, const CairoIRect * extents, const CairoIRect * sample, int * src_x, int * src_y);
cairo_private cairo_status_t _cairo_image_surface_finish(void * abstract_surface);
cairo_private pixman_image_t * _pixman_image_for_color(const cairo_color_t * cairo_color);
cairo_private pixman_image_t * _pixman_image_for_pattern(cairo_image_surface_t * dst,
    const cairo_pattern_t * pattern, cairo_bool_t is_mask, const CairoIRect * extents, const CairoIRect * sample, int * tx, int * ty);
cairo_private void _pixman_image_add_traps(pixman_image_t * image, int dst_x, int dst_y, cairo_traps_t * traps);
cairo_private void _pixman_image_add_tristrip(pixman_image_t * image, int dst_x, int dst_y, cairo_tristrip_t * strip);
cairo_private cairo_image_surface_t * _cairo_image_surface_clone_subimage(cairo_surface_t * surface, const CairoIRect * extents);
//
// Similar to clone; but allow format conversion 
//
cairo_private cairo_image_surface_t * _cairo_image_surface_create_from_image(cairo_image_surface_t * other,
    pixman_format_code_t format, int x, int y, int width, int height, int stride);
//
//#include "cairo-image-surface-inline.h"
//
static inline cairo_image_surface_t * _cairo_image_surface_create_in_error(cairo_status_t status)
{
	return (cairo_image_surface_t*)_cairo_surface_create_in_error(status);
}

static inline void _cairo_image_surface_set_parent(cairo_image_surface_t * image, cairo_surface_t * parent)
{
	image->parent = parent;
}

static inline cairo_bool_t _cairo_image_surface_is_clone(cairo_image_surface_t * image)
{
	return image->parent != NULL;
}
// 
// _cairo_surface_is_image:
// @surface: a #cairo_surface_t
// Checks if a surface is an #cairo_image_surface_t
// Return value: %TRUE if the surface is an image surface
// 
static inline cairo_bool_t _cairo_surface_is_image(const cairo_surface_t * surface)
{
	// _cairo_surface_nil sets a NULL backend so be safe 
	return (surface->backend && surface->backend->type == CAIRO_SURFACE_TYPE_IMAGE);
}
// 
// _cairo_surface_is_image_source:
// @surface: a #cairo_surface_t
// Checks if a surface is an #cairo_image_source_t
// Return value: %TRUE if the surface is an image source
// 
static inline cairo_bool_t _cairo_surface_is_image_source(const cairo_surface_t * surface)
{
	return surface->backend == &_cairo_image_source_backend;
}
//
//#include "cairo-surface-offset-private.h"
//
cairo_private cairo_status_t _cairo_surface_offset_paint(cairo_surface_t * target,
	int x, int y, cairo_operator_t op, const cairo_pattern_t * source, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_offset_mask(cairo_surface_t * target,
	int x, int y, cairo_operator_t op, const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_offset_stroke(cairo_surface_t * surface,
	int x, int y, cairo_operator_t op, const cairo_pattern_t * source, const cairo_path_fixed_t * path,
	const cairo_stroke_style_t * stroke_style, const cairo_matrix_t * ctm,
	const cairo_matrix_t * ctm_inverse, double tolerance, cairo_antialias_t antialias,
	const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_offset_fill(cairo_surface_t * surface,
	int x, int y, cairo_operator_t op, const cairo_pattern_t* source, const cairo_path_fixed_t * path,
	CairoFillRule fill_rule, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_offset_glyphs(cairo_surface_t * surface,
	int x, int y, cairo_operator_t op, const cairo_pattern_t * source, cairo_scaled_font_t * scaled_font,
	cairo_glyph_t * glyphs, int num_glyphs, const cairo_clip_t * clip);
//
//#include "cairo-damage-private.h"
//
struct _cairo_damage {
	cairo_status_t status;
	cairo_region_t * region;
	int dirty, remain;
	struct _cairo_damage_chunk {
		struct _cairo_damage_chunk * next;
		cairo_box_t * base;
		int count;
		int size;
	} chunks, * tail;
	cairo_box_t boxes[32];
};

cairo_private cairo_damage_t * _cairo_damage_create(void);
cairo_private cairo_damage_t * _cairo_damage_create_in_error(cairo_status_t status);
cairo_private cairo_damage_t * _cairo_damage_add_box(cairo_damage_t * damage, const cairo_box_t * box);
cairo_private cairo_damage_t * _cairo_damage_add_rectangle(cairo_damage_t * damage, const CairoIRect * rect);
cairo_private cairo_damage_t * _cairo_damage_add_region(cairo_damage_t * damage, const cairo_region_t * region);
cairo_private cairo_damage_t * _cairo_damage_reduce(cairo_damage_t * damage);
cairo_private void _cairo_damage_destroy(cairo_damage_t * damage);
//
#if CAIRO_HAS_PDF_SURFACE || CAIRO_HAS_PS_SURFACE || CAIRO_HAS_SCRIPT_SURFACE || CAIRO_HAS_XML_SURFACE
	#define CAIRO_HAS_DEFLATE_STREAM 1
#endif
#if CAIRO_HAS_PS_SURFACE || CAIRO_HAS_PDF_SURFACE || CAIRO_HAS_SVG_SURFACE || CAIRO_HAS_WIN32_SURFACE
	#define CAIRO_HAS_FONT_SUBSET 1
#endif
#if CAIRO_HAS_PS_SURFACE || CAIRO_HAS_PDF_SURFACE || CAIRO_HAS_FONT_SUBSET
	#define CAIRO_HAS_PDF_OPERATORS 1
#endif
#if _WIN32 && !_WIN32_WCE /* Permissions on WinCE? No worries! */
	cairo_private FILE * _cairo_win32_tmpfile(void);
	#define tmpfile() _cairo_win32_tmpfile()
#endif
/* @sobolev
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE 1
#endif
*/
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif
/* @sobolev (replaced by SMathConst::Sqrt2) #ifndef M_SQRT2
	#define M_SQRT2 1.41421356237309504880
#endif*/
/* @sobolev (replaced by SMathConst::Sqrt1_2) #ifndef M_SQRT1_2
	#define M_SQRT1_2 0.707106781186547524400844362104849039
#endif */

// @sobolev #undef  ARRAY_LENGTH_Removed
// @sobolev #define ARRAY_LENGTH_Removed(__array) ((int)(sizeof(__array) / sizeof(__array[0])))

#undef STRINGIFY
#undef STRINGIFY_ARG
#define STRINGIFY(macro_or_string)    STRINGIFY_ARG(macro_or_string)
#define STRINGIFY_ARG(contents)       # contents

#if defined (__GNUC__)
	#define cairo_container_of(ptr, type, member) ({ const __typeof__(((type*)0)->member) *mptr__ = (ptr); (type*)((char*)mptr__ - offsetof(type, member)); })
#else
	#define cairo_container_of(ptr, type, member) ((type*)((char*)(ptr) - (char*)&((type*)0)->member))
#endif

#define ASSERT_NOT_REACHED do { assert(!"reached"); } while(0)
#define COMPILE_TIME_ASSERT1(condition, line)   typedef int compile_time_assertion_at_line_ ## line ## _failed [(condition) ? 1 : -1]
#define COMPILE_TIME_ASSERT0(condition, line)   COMPILE_TIME_ASSERT1 (condition, line)
#define COMPILE_TIME_ASSERT(condition)          COMPILE_TIME_ASSERT0 (condition, __LINE__)
#define CAIRO_ALPHA_IS_CLEAR(alpha) ((alpha) <= ((double)0x00ff / (double)0xffff))
#define CAIRO_ALPHA_SHORT_IS_CLEAR(alpha) ((alpha) <= 0x00ff)
#define CAIRO_ALPHA_IS_OPAQUE(alpha) ((alpha) >= ((double)0xff00 / (double)0xffff))
#define CAIRO_ALPHA_SHORT_IS_OPAQUE(alpha) ((alpha) >= 0xff00)
#define CAIRO_ALPHA_IS_ZERO(alpha) ((alpha) <= 0.0)
#define CAIRO_COLOR_IS_CLEAR(color) CAIRO_ALPHA_SHORT_IS_CLEAR((color)->alpha_short)
#define CAIRO_COLOR_IS_OPAQUE(color) CAIRO_ALPHA_SHORT_IS_OPAQUE((color)->alpha_short)
// 
// Reverse the bits in a byte with 7 operations (no 64-bit):
// Devised by Sean Anderson, July 13, 2001.
// Source: http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
// 
#define CAIRO_BITSWAP8(c) ((((c) * 0x0802LU & 0x22110LU) | ((c) * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16)
// 
// Return the number of 1 bits in mask.
// 
// GCC 3.4 supports a "population count" builtin, which on many targets is
// implemented with a single instruction. There is a fallback definition
// in libgcc in case a target does not have one, which should be just as
// good as the open-coded solution below, (which is "HACKMEM 169").
// 
static inline int cairo_const _cairo_popcount(uint32_t mask)
{
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
	return __builtin_popcount(mask);
#else
	register int y = (mask >> 1) &033333333333;
	y = mask - y - ((y >>1) & 033333333333);
	return (((y + (y >> 3)) & 030707070707) % 077);
#endif
}

static cairo_always_inline cairo_bool_t _cairo_is_little_endian(void)
{
	static const int i = 1;
	return *((char*)&i) == 0x01;
}

#ifdef WORDS_BIGENDIAN
	#define CAIRO_BITSWAP8_IF_LITTLE_ENDIAN(c) (c)
#else
	#define CAIRO_BITSWAP8_IF_LITTLE_ENDIAN(c) CAIRO_BITSWAP8(c)
#endif
#ifdef WORDS_BIGENDIAN
	#define cpu_to_be16(v) (v)
	#define be16_to_cpu(v) (v)
	#define cpu_to_be32(v) (v)
	#define be32_to_cpu(v) (v)
#else
	static inline uint16 cairo_const cpu_to_be16(uint16 v)
	{
		return (v << 8) | (v >> 8);
	}
	static inline uint16 cairo_const be16_to_cpu(uint16 v)
	{
		return cpu_to_be16(v);
	}
	static inline uint32_t cairo_const cpu_to_be32(uint32_t v)
	{
		return (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);
	}
	static inline uint32_t cairo_const be32_to_cpu(uint32_t v)
	{
		return cpu_to_be32(v);
	}
#endif
// 
// Unaligned big endian access
// 
static inline uint16 get_unaligned_be16(const uchar * p)
{
	return p[0] << 8 | p[1];
}

static inline uint32_t get_unaligned_be32(const uchar * p)
{
	return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline void put_unaligned_be16(uint16 v, uchar * p)
{
	p[0] = (v >> 8) & 0xff;
	p[1] = v & 0xff;
}

static inline void put_unaligned_be32(uint32_t v, uchar * p)
{
	p[0] = (v >> 24) & 0xff;
	p[1] = (v >> 16) & 0xff;
	p[2] = (v >> 8)  & 0xff;
	p[3] = v & 0xff;
}
//
// The glibc versions of ispace() and isdigit() are slow in UTF-8 locales.
//
static inline int cairo_const _cairo_isspace(int c)
{
	return (c == 0x20 || (c >= 0x09 && c <= 0x0d));
}

/* (replaced with isdec()) static inline int cairo_const _cairo_isdigit(int c)
{
	return (c >= '0' && c <= '9');
}*/

cairo_private void _cairo_box_from_doubles(cairo_box_t * box, double * x1, double * y1, double * x2, double * y2);
cairo_private void _cairo_box_to_doubles(const cairo_box_t * box, double * x1, double * y1, double * x2, double * y2);
cairo_private void FASTCALL _cairo_box_from_rectangle(cairo_box_t * box, const CairoIRect * rectangle);
cairo_private void FASTCALL _cairo_box_round_to_rectangle(const cairo_box_t * box, CairoIRect * rectangle);
cairo_private void _cairo_box_add_curve_to(cairo_box_t * extents, const cairo_point_t * a, const cairo_point_t * b, const cairo_point_t * c, const cairo_point_t * d);
cairo_private void _cairo_boxes_get_extents(const cairo_box_t * boxes, int num_boxes, cairo_box_t * extents);
cairo_private extern const CairoIRect _cairo_empty_rectangle;
cairo_private extern const CairoIRect _cairo_unbounded_rectangle;

static inline void _cairo_unbounded_rectangle_init(CairoIRect * rect)
{
	*rect = _cairo_unbounded_rectangle;
}

cairo_private_no_warn cairo_bool_t FASTCALL _cairo_rectangle_intersect(CairoIRect * dst, const CairoIRect * src);

static inline cairo_bool_t _cairo_rectangle_intersects(const CairoIRect * dst, const CairoIRect * src)
{
	return !(src->x >= dst->x + (int)dst->width || src->x + (int)src->width <= dst->x || src->y >= dst->y + (int)dst->height || src->y + (int)src->height <= dst->y);
}

static inline cairo_bool_t _cairo_rectangle_contains_rectangle(const CairoIRect * a, const CairoIRect * b)
{
	return (a->x <= b->x && a->x + (int)a->width >= b->x + (int)b->width && a->y <= b->y && a->y + (int)a->height >= b->y + (int)b->height);
}

cairo_private void _cairo_rectangle_int_from_double(CairoIRect * recti, const cairo_rectangle_t * rectf);
// 
// Extends the dst rectangle to also contain src.
// If one of the rectangles is empty, the result is undefined
// 
cairo_private void _cairo_rectangle_union(CairoIRect * dst, const CairoIRect * src);
cairo_private cairo_bool_t _cairo_box_intersects_line_segment(const cairo_box_t * box, cairo_line_t * line) cairo_pure;
cairo_private cairo_bool_t _cairo_spline_intersects(const cairo_point_t * a, const cairo_point_t * b, const cairo_point_t * c, const cairo_point_t * d, const cairo_box_t * box) cairo_pure;

struct cairo_user_data_slot_t {
	const cairo_user_data_key_t * key;
	void * user_data;
	cairo_destroy_func_t destroy;
};

cairo_private void FASTCALL _cairo_user_data_array_init(cairo_user_data_array_t * array);
cairo_private void FASTCALL _cairo_user_data_array_fini(cairo_user_data_array_t * array);
cairo_private void * FASTCALL _cairo_user_data_array_get_data(cairo_user_data_array_t * array, const cairo_user_data_key_t * key);
cairo_private cairo_status_t _cairo_user_data_array_set_data(cairo_user_data_array_t * array, const cairo_user_data_key_t * key,
    void * user_data, cairo_destroy_func_t destroy);
cairo_private cairo_status_t _cairo_user_data_array_copy(cairo_user_data_array_t * dst, const cairo_user_data_array_t * src);
cairo_private void _cairo_user_data_array_foreach(cairo_user_data_array_t * array,
    void (* func)(const void * key, void * elt, void * closure), void * closure);

#define _CAIRO_HASH_INIT_VALUE 5381

cairo_private ulong FASTCALL _cairo_hash_string(const char * c);
cairo_private ulong FASTCALL _cairo_hash_bytes(ulong hash, const void * bytes, uint length);

#define _cairo_scaled_glyph_index(g) ((g)->hash_entry.hash)
#define _cairo_scaled_glyph_set_index(g, i)  ((g)->hash_entry.hash = (i))

struct _cairo_font_face {
	// hash_entry must be first 
	cairo_hash_entry_t hash_entry;
	cairo_status_t status;
	cairo_reference_count_t ref_count;
	cairo_user_data_array_t user_data;
	const cairo_font_face_backend_t * backend;
};

cairo_private void _cairo_default_context_reset_static_data(void);
cairo_private void _cairo_toy_font_face_reset_static_data(void);
cairo_private void _cairo_ft_font_reset_static_data(void);
cairo_private void _cairo_win32_font_reset_static_data(void);

#if CAIRO_HAS_COGL_SURFACE
	void _cairo_cogl_context_reset_static_data(void);
#endif
//
// the font backend interface 
//
struct _cairo_unscaled_font_backend {
	cairo_bool_t (* destroy)(void * unscaled_font);
};
//
// #cairo_toy_font_face_t - simple family/slant/weight font faces used for the built-in font API
//
struct cairo_toy_font_face_t {
	cairo_font_face_t base;
	const char * family;
	cairo_bool_t owns_family;
	cairo_font_slant_t slant;
	cairo_font_weight_t weight;
	cairo_font_face_t * impl_face; /* The non-toy font face this actually uses */
};

#ifndef __SLCAIRO_H // @sobolev {
typedef enum _cairo_scaled_glyph_info {
	CAIRO_SCALED_GLYPH_INFO_METRICS      = (1 << 0),
	CAIRO_SCALED_GLYPH_INFO_SURFACE      = (1 << 1),
	CAIRO_SCALED_GLYPH_INFO_PATH         = (1 << 2),
	CAIRO_SCALED_GLYPH_INFO_RECORDING_SURFACE = (1 << 3)
} cairo_scaled_glyph_info_t;
#endif // } @sobolev __SLCAIRO_H

typedef struct _cairo_scaled_font_subset {
	cairo_scaled_font_t * scaled_font;
	uint   font_id;
	uint   subset_id;
	// Index of glyphs array is subset_glyph_index.
	// Value of glyphs array is scaled_font_glyph_index.
	ulong * glyphs;
	char ** utf8;
	char ** glyph_names;
	int   * to_latin_char;
	ulong * latin_to_subset_glyph_index;
	uint   num_glyphs;
	cairo_bool_t is_composite;
	cairo_bool_t is_scaled;
	cairo_bool_t is_latin;
} cairo_scaled_font_subset_t;

struct _cairo_scaled_font_backend {
	cairo_font_type_t type;
	void (* fini)(void * scaled_font);
	cairo_warn cairo_int_status_t (* scaled_glyph_init)(void * scaled_font, cairo_scaled_glyph_t * scaled_glyph, cairo_scaled_glyph_info_t info);
	// 
	// A backend only needs to implement this or ucs4_to_index(), not
	// both. This allows the backend to do something more sophisticated
	// then just converting characters one by one.
	// 
	cairo_warn cairo_int_status_t (* text_to_glyphs)(void * scaled_font,
	    double x, double y, const char * utf8, int utf8_len, cairo_glyph_t ** glyphs, int * num_glyphs,
	    cairo_text_cluster_t ** clusters, int * num_clusters, cairo_text_cluster_flags_t * cluster_flags);
	ulong (* ucs4_to_index)(void * scaled_font, uint32_t ucs4);

	/* Read data from a sfnt font table.
	 * @scaled_font: font
	 * @tag: 4 byte table name specifying the table to read.
	 * @offset: offset into the table
	 * @buffer: buffer to write data into. Caller must ensure there is sufficient space.
	 *          If NULL, return the size of the table in @length.
	 * @length: If @buffer is NULL, the size of the table will be returned in @length.
	 *          If @buffer is not null, @length specifies the number of bytes to read.
	 *
	 * If less than @length bytes are available to read this function
	 * returns CAIRO_INT_STATUS_UNSUPPORTED. Note that requesting more
	 * bytes than are available in the table may continue reading data
	 * from the following table and return success. If this is
	 * undesirable the caller should first query the table size. If an
	 * error occurs the output value of @length is undefined.
	 *
	 * Returns CAIRO_INT_STATUS_UNSUPPORTED if not a sfnt style font or table not found.
	 */
	cairo_warn cairo_int_status_t (* load_truetype_table)(void * scaled_font, ulong tag, long offset, uchar * buffer, ulong * length);

	/* ucs4 is set to -1 if the unicode character could not be found
	 * for the glyph */
	cairo_warn cairo_int_status_t (* index_to_ucs4)(void * scaled_font, ulong index, uint32_t * ucs4);
	cairo_warn cairo_bool_t (* is_synthetic)(void * scaled_font);

	/* For type 1 fonts, return the glyph name for a given glyph index.
	 * A glyph index and list of glyph names in the Type 1 fonts is provided.
	 * The function returns the index of the glyph in the list of glyph names.
	 * @scaled_font: font
	 * @glyph_names: the names of each glyph in the Type 1 font in the
	 *   order they appear in the CharStrings array
	 * @num_glyph_names: the number of names in the glyph_names array
	 * @glyph_index: the given glyph index
	 * @glyph_array_index: (index into glyph_names) the glyph name corresponding
	 *  to the glyph_index
	 */
	cairo_warn cairo_int_status_t (* index_to_glyph_name)(void * scaled_font, char ** glyph_names, int num_glyph_names, ulong glyph_index, ulong * glyph_array_index);

	/* Read data from a PostScript font.
	 * @scaled_font: font
	 * @offset: offset into the table
	 * @buffer: buffer to write data into. Caller must ensure there is sufficient space.
	 *          If NULL, return the size of the table in @length.
	 * @length: If @buffer is NULL, the size of the table will be returned in @length.
	 *          If @buffer is not null, @length specifies the number of bytes to read.
	 *
	 * If less than @length bytes are available to read this function
	 * returns CAIRO_INT_STATUS_UNSUPPORTED. If an error occurs the
	 * output value of @length is undefined.
	 *
	 * Returns CAIRO_INT_STATUS_UNSUPPORTED if not a Type 1 font.
	 */
	cairo_warn cairo_int_status_t (* load_type1_data)(void * scaled_font, long offset, uchar * buffer, ulong * length);
};

struct _cairo_font_face_backend {
	cairo_font_type_t type;
	cairo_warn cairo_status_t (* create_for_toy)(cairo_toy_font_face_t * toy_face, cairo_font_face_t ** font_face);
	//
	// The destroy() function is allowed to resurrect the font face
	// by re-referencing. This is needed for the FreeType backend.
	//
	cairo_bool_t (* destroy)(void * font_face);
	cairo_warn cairo_status_t (* scaled_font_create)(void * font_face, const cairo_matrix_t * font_matrix, const cairo_matrix_t  * ctm,
	    const cairo_font_options_t * options, cairo_scaled_font_t ** scaled_font);
	cairo_font_face_t * (*get_implementation)(void * font_face, const cairo_matrix_t * font_matrix, const cairo_matrix_t * ctm, const cairo_font_options_t * options);
};

extern const cairo_private struct _cairo_font_face_backend _cairo_user_font_face_backend;
//
// concrete font backends 
//
#if CAIRO_HAS_FT_FONT
	extern const cairo_private struct _cairo_font_face_backend _cairo_ft_font_face_backend;
#endif
#if CAIRO_HAS_WIN32_FONT
	extern const cairo_private struct _cairo_font_face_backend _cairo_win32_font_face_backend;
#endif
#if CAIRO_HAS_QUARTZ_FONT
	extern const cairo_private struct _cairo_font_face_backend _cairo_quartz_font_face_backend;
#endif

#define CAIRO_EXTEND_SURFACE_DEFAULT CAIRO_EXTEND_NONE
#define CAIRO_EXTEND_GRADIENT_DEFAULT CAIRO_EXTEND_PAD
#define CAIRO_FILTER_DEFAULT CAIRO_FILTER_GOOD

extern const cairo_private cairo_solid_pattern_t _cairo_pattern_clear;
extern const cairo_private cairo_solid_pattern_t _cairo_pattern_black;
extern const cairo_private cairo_solid_pattern_t _cairo_pattern_white;

struct _cairo_surface_attributes {
	cairo_matrix_t matrix;
	cairo_extend_t extend;
	cairo_filter_t filter;
	cairo_bool_t has_component_alpha;
	int x_offset;
	int y_offset;
	void * extra;
};

#define CAIRO_FONT_SLANT_DEFAULT   CAIRO_FONT_SLANT_NORMAL
#define CAIRO_FONT_WEIGHT_DEFAULT  CAIRO_FONT_WEIGHT_NORMAL

#define CAIRO_WIN32_FONT_FAMILY_DEFAULT "Arial"
#define CAIRO_QUARTZ_FONT_FAMILY_DEFAULT  "Helvetica"
#define CAIRO_FT_FONT_FAMILY_DEFAULT     ""
#define CAIRO_USER_FONT_FAMILY_DEFAULT     "@cairo:"

#if CAIRO_HAS_WIN32_FONT
	#define CAIRO_FONT_FAMILY_DEFAULT CAIRO_WIN32_FONT_FAMILY_DEFAULT
	#define CAIRO_FONT_FACE_BACKEND_DEFAULT &_cairo_win32_font_face_backend
#elif CAIRO_HAS_QUARTZ_FONT
	#define CAIRO_FONT_FAMILY_DEFAULT CAIRO_QUARTZ_FONT_FAMILY_DEFAULT
	#define CAIRO_FONT_FACE_BACKEND_DEFAULT &_cairo_quartz_font_face_backend
#elif CAIRO_HAS_FT_FONT
	#define CAIRO_FONT_FAMILY_DEFAULT CAIRO_FT_FONT_FAMILY_DEFAULT
	#define CAIRO_FONT_FACE_BACKEND_DEFAULT &_cairo_ft_font_face_backend
#else
	#define CAIRO_FONT_FAMILY_DEFAULT CAIRO_FT_FONT_FAMILY_DEFAULT
	#define CAIRO_FONT_FACE_BACKEND_DEFAULT &_cairo_user_font_face_backend
#endif

#define CAIRO_GSTATE_OPERATOR_DEFAULT   CAIRO_OPERATOR_OVER
#define CAIRO_GSTATE_TOLERANCE_DEFAULT  0.1
#define CAIRO_GSTATE_FILL_RULE_DEFAULT  CAIRO_FILL_RULE_WINDING
#define CAIRO_GSTATE_LINE_WIDTH_DEFAULT 2.0
#define CAIRO_GSTATE_LINE_CAP_DEFAULT   CAIRO_LINE_CAP_BUTT
#define CAIRO_GSTATE_LINE_JOIN_DEFAULT  CAIRO_LINE_JOIN_MITER
#define CAIRO_GSTATE_MITER_LIMIT_DEFAULT        10.0
#define CAIRO_GSTATE_DEFAULT_FONT_SIZE  10.0

#define CAIRO_SURFACE_RESOLUTION_DEFAULT 72.0
#define CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT 300.0

typedef struct _cairo_stroke_face {
	cairo_point_t ccw;
	cairo_point_t point;
	cairo_point_t cw;
	cairo_slope_t dev_vector;
	RPoint dev_slope;
	RPoint usr_vector;
	double length;
} cairo_stroke_face_t;

/* cairo.c */

/* replaced-with MINMAX() static inline double cairo_const _cairo_restrict_value(double value, double min, double max)
{
	return (value < min) ? min : ((value > max) ? max : value);
	//if(value < min)
	//	return min;
	//else if(value > max)
	//	return max;
	//else
	//	return value;
}*/
// 
// C99 round() rounds to the nearest integral value with halfway cases rounded
// away from 0. _cairo_round rounds halfway cases toward positive infinity.
// This matches the rounding behaviour of _cairo_lround. 
// 
static inline double cairo_const _cairo_round(double r)
{
	return floor(r + 0.5);
}

#if DISABLE_SOME_FLOATING_POINT
	cairo_private int _cairo_lround(double d) cairo_const;
#else
	static inline int cairo_const _cairo_lround(double r)
	{
		return (int)_cairo_round(r);
	}
#endif

cairo_private uint16 _cairo_half_from_float(float f) cairo_const;
cairo_private cairo_bool_t FASTCALL _cairo_operator_bounded_by_mask(cairo_operator_t op) cairo_const;
cairo_private cairo_bool_t FASTCALL _cairo_operator_bounded_by_source(cairo_operator_t op) cairo_const;

enum {
	CAIRO_OPERATOR_BOUND_BY_MASK = 1 << 1,
	CAIRO_OPERATOR_BOUND_BY_SOURCE = 1 << 2,
};

cairo_private uint32_t _cairo_operator_bounded_by_either(cairo_operator_t op) cairo_const;

// cairo-color.c 
cairo_private const cairo_color_t * FASTCALL _cairo_stock_color(cairo_stock_t stock) cairo_pure;

#define CAIRO_COLOR_WHITE       _cairo_stock_color(CAIRO_STOCK_WHITE)
#define CAIRO_COLOR_BLACK       _cairo_stock_color(CAIRO_STOCK_BLACK)
#define CAIRO_COLOR_TRANSPARENT _cairo_stock_color(CAIRO_STOCK_TRANSPARENT)

cairo_private uint16 _cairo_color_double_to_short(double d) cairo_const;

cairo_private void _cairo_color_init_rgba(cairo_color_t * color, double red, double green, double blue, double alpha);
cairo_private void _cairo_color_multiply_alpha(cairo_color_t * color, double alpha);
cairo_private void _cairo_color_get_rgba(cairo_color_t * color, double * red, double * green, double * blue, double * alpha);
cairo_private void _cairo_color_get_rgba_premultiplied(cairo_color_t * color, double * red, double * green, double * blue, double * alpha);
cairo_private cairo_bool_t _cairo_color_equal(const cairo_color_t * color_a, const cairo_color_t * color_b) cairo_pure;
cairo_private cairo_bool_t _cairo_color_stop_equal(const cairo_color_stop_t * color_a, const cairo_color_stop_t * color_b) cairo_pure;
cairo_private cairo_content_t _cairo_color_get_content(const cairo_color_t * color) cairo_pure;

// cairo-font-face.c 
extern const cairo_private cairo_font_face_t _cairo_font_face_nil;
extern const cairo_private cairo_font_face_t _cairo_font_face_nil_file_not_found; // @v1.14.6

cairo_private void _cairo_font_face_init(cairo_font_face_t * font_face, const cairo_font_face_backend_t * backend);
cairo_private cairo_bool_t _cairo_font_face_destroy(void * abstract_face);
cairo_private cairo_status_t _cairo_font_face_set_error(cairo_font_face_t * font_face, cairo_status_t status);
cairo_private void _cairo_unscaled_font_init(cairo_unscaled_font_t * font, const cairo_unscaled_font_backend_t * backend);
cairo_private_no_warn cairo_unscaled_font_t * _cairo_unscaled_font_reference(cairo_unscaled_font_t * font);
cairo_private void _cairo_unscaled_font_destroy(cairo_unscaled_font_t * font);
// cairo-font-face-twin.c 
cairo_private cairo_font_face_t * _cairo_font_face_twin_create_fallback(void);
cairo_private cairo_status_t _cairo_font_face_twin_create_for_toy(cairo_toy_font_face_t * toy_face, cairo_font_face_t ** font_face);
// cairo-font-face-twin-data.c 
extern const cairo_private int8 _cairo_twin_outlines[];
extern const cairo_private uint16 _cairo_twin_charmap[128];
// cairo-font-options.c 
cairo_private void FASTCALL _cairo_font_options_init_default(cairo_font_options_t * options);
cairo_private void FASTCALL _cairo_font_options_init_copy(cairo_font_options_t * options, const cairo_font_options_t * other);
cairo_private void _cairo_font_options_set_lcd_filter(cairo_font_options_t * options, cairo_lcd_filter_t lcd_filter);
cairo_private cairo_lcd_filter_t _cairo_font_options_get_lcd_filter(const cairo_font_options_t * options);
cairo_private void _cairo_font_options_set_round_glyph_positions(cairo_font_options_t   * options, cairo_round_glyph_positions_t round);
cairo_private cairo_round_glyph_positions_t _cairo_font_options_get_round_glyph_positions(const cairo_font_options_t * options);
// cairo-hull.c 
cairo_private cairo_status_t _cairo_hull_compute(cairo_pen_vertex_t * vertices, int * num_vertices);
// cairo-lzw.c 
cairo_private uchar * _cairo_lzw_compress(uchar * data, ulong * size_in_out);
// cairo-misc.c 
cairo_private cairo_status_t _cairo_validate_text_clusters(const char * utf8, int utf8_len,
    const cairo_glyph_t * glyphs, int num_glyphs, const cairo_text_cluster_t * clusters, int num_clusters, cairo_text_cluster_flags_t cluster_flags);
cairo_private cairo_status_t _cairo_intern_string(const char ** str_inout, int len);
cairo_private void _cairo_intern_string_reset_static_data(void);
cairo_private const char * cairo_get_locale_decimal_point(void);
// cairo-path-fixed.c 
cairo_private cairo_path_fixed_t * _cairo_path_fixed_create(void);
cairo_private void FASTCALL _cairo_path_fixed_init(cairo_path_fixed_t * path);
cairo_private cairo_status_t FASTCALL _cairo_path_fixed_init_copy(cairo_path_fixed_t * path, const cairo_path_fixed_t * other);
cairo_private void FASTCALL _cairo_path_fixed_fini(cairo_path_fixed_t * path);
cairo_private void FASTCALL _cairo_path_fixed_destroy(cairo_path_fixed_t * path);
cairo_private cairo_status_t FASTCALL _cairo_path_fixed_move_to(cairo_path_fixed_t  * path, cairo_fixed_t x, cairo_fixed_t y);
cairo_private void _cairo_path_fixed_new_sub_path(cairo_path_fixed_t * path);
cairo_private cairo_status_t _cairo_path_fixed_rel_move_to(cairo_path_fixed_t * path, cairo_fixed_t dx, cairo_fixed_t dy);
cairo_private cairo_status_t FASTCALL _cairo_path_fixed_line_to(cairo_path_fixed_t * path, cairo_fixed_t x, cairo_fixed_t y);
cairo_private cairo_status_t _cairo_path_fixed_rel_line_to(cairo_path_fixed_t * path, cairo_fixed_t dx, cairo_fixed_t dy);
cairo_private cairo_status_t _cairo_path_fixed_curve_to(cairo_path_fixed_t  * path,
    cairo_fixed_t x0, cairo_fixed_t y0, cairo_fixed_t x1, cairo_fixed_t y1, cairo_fixed_t x2, cairo_fixed_t y2);
cairo_private cairo_status_t _cairo_path_fixed_rel_curve_to(cairo_path_fixed_t * path,
    cairo_fixed_t dx0, cairo_fixed_t dy0, cairo_fixed_t dx1, cairo_fixed_t dy1, cairo_fixed_t dx2, cairo_fixed_t dy2);
cairo_private cairo_status_t FASTCALL _cairo_path_fixed_close_path(cairo_path_fixed_t * path);
cairo_private cairo_bool_t _cairo_path_fixed_get_current_point(cairo_path_fixed_t * path, cairo_fixed_t * x, cairo_fixed_t * y);
typedef cairo_status_t (cairo_path_fixed_move_to_func_t)(void * closure, const cairo_point_t * point);
typedef cairo_status_t (cairo_path_fixed_line_to_func_t)(void * closure, const cairo_point_t * point);
typedef cairo_status_t (cairo_path_fixed_curve_to_func_t)(void * closure, const cairo_point_t * p0,
    const cairo_point_t * p1, const cairo_point_t * p2);
typedef cairo_status_t (cairo_path_fixed_close_path_func_t)(void * closure);
cairo_private cairo_status_t _cairo_path_fixed_interpret(const cairo_path_fixed_t * path,
    cairo_path_fixed_move_to_func_t * move_to, cairo_path_fixed_line_to_func_t * line_to,
    cairo_path_fixed_curve_to_func_t * curve_to, cairo_path_fixed_close_path_func_t * close_path, void * closure);
cairo_private cairo_status_t _cairo_path_fixed_interpret_flat(const cairo_path_fixed_t * path,
    cairo_path_fixed_move_to_func_t * move_to, cairo_path_fixed_line_to_func_t * line_to,
    cairo_path_fixed_close_path_func_t * close_path, void * closure, double tolerance);
cairo_private cairo_bool_t _cairo_path_bounder_extents(const cairo_path_fixed_t * path, cairo_box_t * box);
cairo_private cairo_bool_t _cairo_path_fixed_extents(const cairo_path_fixed_t * path, cairo_box_t * box);
cairo_private void _cairo_path_fixed_approximate_clip_extents(const cairo_path_fixed_t * path, CairoIRect * extents);
cairo_private void _cairo_path_fixed_approximate_fill_extents(const cairo_path_fixed_t * path, CairoIRect * extents);
cairo_private void _cairo_path_fixed_fill_extents(const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, CairoIRect * extents);
cairo_private void _cairo_path_fixed_approximate_stroke_extents(const cairo_path_fixed_t * path,
    const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, CairoIRect * extents);
cairo_private cairo_status_t _cairo_path_fixed_stroke_extents(const cairo_path_fixed_t * path, const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse, double tolerance, CairoIRect * extents);
cairo_private void FASTCALL _cairo_path_fixed_transform(cairo_path_fixed_t * path, const cairo_matrix_t * matrix);
cairo_private cairo_bool_t FASTCALL _cairo_path_fixed_is_box(const cairo_path_fixed_t * path, cairo_box_t * box);
cairo_private cairo_bool_t _cairo_path_fixed_is_rectangle(const cairo_path_fixed_t * path, cairo_box_t * box);
// cairo-path-in-fill.c 
cairo_private cairo_bool_t _cairo_path_fixed_in_fill(const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, double x, double y);
// cairo-path-fill.c 
cairo_private cairo_status_t _cairo_path_fixed_fill_to_polygon(const cairo_path_fixed_t * path, double tolerance, cairo_polygon_t * polygon);
cairo_private cairo_status_t _cairo_path_fixed_fill_rectilinear_to_polygon(const cairo_path_fixed_t * path, cairo_antialias_t antialias, cairo_polygon_t * polygon);
cairo_private cairo_status_t _cairo_path_fixed_fill_rectilinear_to_boxes(const cairo_path_fixed_t * path, CairoFillRule fill_rule, cairo_antialias_t antialias, cairo_boxes_t * boxes);
cairo_private cairo_region_t * _cairo_path_fixed_fill_rectilinear_to_region(const cairo_path_fixed_t  * path, CairoFillRule fill_rule, const CairoIRect * extents);
cairo_private cairo_status_t _cairo_path_fixed_fill_to_traps(const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, cairo_traps_t * traps);
// cairo-path-stroke.c 
cairo_private cairo_status_t _cairo_path_fixed_stroke_to_polygon(const cairo_path_fixed_t * path,
    const cairo_stroke_style_t * stroke_style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse,
    double tolerance, cairo_polygon_t * polygon);
cairo_private cairo_int_status_t _cairo_path_fixed_stroke_to_tristrip(const cairo_path_fixed_t  * path,
    const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse,
    double tolerance, cairo_tristrip_t * strip);
cairo_private cairo_status_t _cairo_path_fixed_stroke_dashed_to_polygon(const cairo_path_fixed_t    * path,
    const cairo_stroke_style_t * stroke_style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse,
    double tolerance, cairo_polygon_t * polygon);
cairo_private cairo_int_status_t _cairo_path_fixed_stroke_rectilinear_to_boxes(const cairo_path_fixed_t * path,
    const cairo_stroke_style_t * stroke_style, const cairo_matrix_t * ctm,
    cairo_antialias_t antialias, cairo_boxes_t * boxes);
cairo_private cairo_int_status_t _cairo_path_fixed_stroke_to_traps(const cairo_path_fixed_t * path, const cairo_stroke_style_t * stroke_style,
    const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse, double tolerance, cairo_traps_t * traps);
cairo_private cairo_int_status_t _cairo_path_fixed_stroke_polygon_to_traps(const cairo_path_fixed_t * path,
    const cairo_stroke_style_t * stroke_style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse, double tolerance,
    cairo_traps_t * traps);
cairo_private cairo_status_t _cairo_path_fixed_stroke_to_shaper(cairo_path_fixed_t  *path, const cairo_stroke_style_t * stroke_style, 
	const cairo_matrix_t *ctm, const cairo_matrix_t *ctm_inverse, double tolerance,
    cairo_status_t (* add_triangle)(void * closure, const cairo_point_t triangle[3]),
    cairo_status_t (* add_triangle_fan)(void * closure, const cairo_point_t * midpt, const cairo_point_t * points, int npoints),
    cairo_status_t (* add_quad)(void * closure, const cairo_point_t quad[4]), void * closure);
// cairo-scaled-font.c 
cairo_private void FASTCALL _cairo_scaled_font_freeze_cache(cairo_scaled_font_t * scaled_font);
cairo_private void FASTCALL _cairo_scaled_font_thaw_cache(cairo_scaled_font_t * scaled_font);
cairo_private void FASTCALL _cairo_scaled_font_reset_cache(cairo_scaled_font_t * scaled_font);
cairo_private cairo_status_t FASTCALL _cairo_scaled_font_set_error(cairo_scaled_font_t * scaled_font, cairo_status_t status);
cairo_private cairo_scaled_font_t * _cairo_scaled_font_create_in_error(cairo_status_t status);
cairo_private void _cairo_scaled_font_reset_static_data(void);
cairo_private cairo_status_t _cairo_scaled_font_register_placeholder_and_unlock_font_map(cairo_scaled_font_t * scaled_font);
cairo_private void _cairo_scaled_font_unregister_placeholder_and_lock_font_map(cairo_scaled_font_t * scaled_font);
cairo_private cairo_status_t _cairo_scaled_font_init(cairo_scaled_font_t * scaled_font, cairo_font_face_t * font_face,
    const cairo_matrix_t * font_matrix, const cairo_matrix_t * ctm, const cairo_font_options_t * options, const cairo_scaled_font_backend_t * backend);
cairo_private cairo_status_t FASTCALL _cairo_scaled_font_set_metrics(cairo_scaled_font_t * scaled_font, cairo_font_extents_t * fs_metrics);
/* This should only be called on an error path by a scaled_font constructor */
cairo_private void _cairo_scaled_font_fini(cairo_scaled_font_t * scaled_font);
cairo_private cairo_status_t _cairo_scaled_font_font_extents(cairo_scaled_font_t  * scaled_font, cairo_font_extents_t * extents);
cairo_private cairo_status_t _cairo_scaled_font_glyph_device_extents(cairo_scaled_font_t * scaled_font,
    const cairo_glyph_t * glyphs, int num_glyphs, CairoIRect * extents, cairo_bool_t * overlap);
cairo_private cairo_bool_t _cairo_scaled_font_glyph_approximate_extents(cairo_scaled_font_t * scaled_font, const cairo_glyph_t * glyphs, int num_glyphs, CairoIRect * extents);
cairo_private cairo_status_t _cairo_scaled_font_show_glyphs(cairo_scaled_font_t * scaled_font,
    cairo_operator_t op, const cairo_pattern_t * source, cairo_surface_t     * surface,
    int source_x, int source_y, int dest_x, int dest_y,
    uint width, uint height, cairo_glyph_t * glyphs, int num_glyphs, cairo_region_t * clip_region);
cairo_private cairo_status_t _cairo_scaled_font_glyph_path(cairo_scaled_font_t * scaled_font,
    const cairo_glyph_t * glyphs, int num_glyphs, cairo_path_fixed_t  * path);
cairo_private void _cairo_scaled_glyph_set_metrics(cairo_scaled_glyph_t * scaled_glyph, const cairo_scaled_font_t * scaled_font, const cairo_text_extents_t * fs_metrics);
cairo_private void _cairo_scaled_glyph_set_surface(cairo_scaled_glyph_t * scaled_glyph, cairo_scaled_font_t * scaled_font, cairo_image_surface_t * surface);
cairo_private void _cairo_scaled_glyph_set_path(cairo_scaled_glyph_t * scaled_glyph, cairo_scaled_font_t * scaled_font, cairo_path_fixed_t * path);
cairo_private void _cairo_scaled_glyph_set_recording_surface(cairo_scaled_glyph_t * scaled_glyph, cairo_scaled_font_t * scaled_font, cairo_surface_t * recording_surface);
cairo_private cairo_int_status_t _cairo_scaled_glyph_lookup(cairo_scaled_font_t * scaled_font,
    ulong index, cairo_scaled_glyph_info_t info, cairo_scaled_glyph_t ** scaled_glyph_ret);
cairo_private double _cairo_scaled_font_get_max_scale(cairo_scaled_font_t * scaled_font);
cairo_private void _cairo_scaled_font_map_destroy(void);
// cairo-stroke-style.c 
cairo_private void _cairo_stroke_style_init(cairo_stroke_style_t * style);
cairo_private cairo_status_t _cairo_stroke_style_init_copy(cairo_stroke_style_t * style, const cairo_stroke_style_t * other);
cairo_private void _cairo_stroke_style_fini(cairo_stroke_style_t * style);
cairo_private void _cairo_stroke_style_max_distance_from_path(const cairo_stroke_style_t * style,
    const cairo_path_fixed_t * path, const cairo_matrix_t * ctm, double * dx, double * dy);
cairo_private void _cairo_stroke_style_max_line_distance_from_path(const cairo_stroke_style_t * style,
    const cairo_path_fixed_t * path, const cairo_matrix_t * ctm, double * dx, double * dy);
cairo_private void _cairo_stroke_style_max_join_distance_from_path(const cairo_stroke_style_t * style,
    const cairo_path_fixed_t * path, const cairo_matrix_t * ctm, double * dx, double * dy);
cairo_private double _cairo_stroke_style_dash_period(const cairo_stroke_style_t * style);
cairo_private double _cairo_stroke_style_dash_stroked(const cairo_stroke_style_t * style);
cairo_private cairo_bool_t _cairo_stroke_style_dash_can_approximate(const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm, double tolerance);
cairo_private void _cairo_stroke_style_dash_approximate(const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm, double tolerance, double * dash_offset, double * dashes, uint * num_dashes);
//
// cairo-surface.c 
//
cairo_private cairo_status_t _cairo_surface_copy_mime_data(cairo_surface_t * dst, cairo_surface_t * src);
cairo_private_no_warn cairo_int_status_t FASTCALL _cairo_surface_set_error(cairo_surface_t * surface, cairo_int_status_t status);
cairo_private void _cairo_surface_set_resolution(cairo_surface_t * surface, double x_res, double y_res);
cairo_private cairo_surface_t * _cairo_surface_create_for_rectangle_int(cairo_surface_t * target, const CairoIRect * extents);
cairo_private cairo_surface_t * _cairo_surface_create_scratch(cairo_surface_t * other, cairo_content_t content,
    int width, int height, const cairo_color_t  * color);
cairo_private void _cairo_surface_init(cairo_surface_t * surface,
    const cairo_surface_backend_t * backend, cairo_device_t * device, cairo_content_t content);
cairo_private void _cairo_surface_set_font_options(cairo_surface_t * surface, cairo_font_options_t  * options);
cairo_private cairo_status_t _cairo_surface_paint(cairo_surface_t   * surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_clip_t * clip);
cairo_private cairo_image_surface_t * _cairo_surface_map_to_image(cairo_surface_t  * surface, const CairoIRect * extents);
cairo_private_no_warn cairo_int_status_t _cairo_surface_unmap_image(cairo_surface_t * surface, cairo_image_surface_t * image);
cairo_private cairo_status_t _cairo_surface_mask(cairo_surface_t * surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_fill_stroke(cairo_surface_t * surface, cairo_operator_t fill_op,
    const cairo_pattern_t   * fill_source, CairoFillRule fill_rule, double fill_tolerance,
    cairo_antialias_t fill_antialias, cairo_path_fixed_t * path, cairo_operator_t stroke_op,
    const cairo_pattern_t   * stroke_source, const cairo_stroke_style_t * stroke_style,
    const cairo_matrix_t * stroke_ctm, const cairo_matrix_t * stroke_ctm_inverse,
    double stroke_tolerance, cairo_antialias_t stroke_antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_stroke(cairo_surface_t * surface, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_path_fixed_t * path, const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse, double tolerance,
    cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_fill(cairo_surface_t * surface, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance,
    cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_show_text_glyphs(cairo_surface_t * surface,
    cairo_operator_t op, const cairo_pattern_t * source, const char * utf8, int utf8_len,
    cairo_glyph_t * glyphs, int num_glyphs, const cairo_text_cluster_t * clusters, int num_clusters,
    cairo_text_cluster_flags_t cluster_flags, cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_acquire_source_image(cairo_surface_t * surface, cairo_image_surface_t  ** image_out, void ** image_extra);
cairo_private void _cairo_surface_release_source_image(cairo_surface_t * surface, cairo_image_surface_t  * image, void * image_extra);
cairo_private cairo_surface_t * _cairo_surface_snapshot(cairo_surface_t * surface);
cairo_private void FASTCALL _cairo_surface_attach_snapshot(cairo_surface_t * surface, cairo_surface_t * snapshot, cairo_surface_func_t detach_func);
cairo_private cairo_surface_t * _cairo_surface_has_snapshot(cairo_surface_t * surface, const cairo_surface_backend_t * backend);
cairo_private void FASTCALL _cairo_surface_detach_snapshot(cairo_surface_t * snapshot);
cairo_private cairo_status_t FASTCALL _cairo_surface_begin_modification(cairo_surface_t * surface);
cairo_private_no_warn cairo_bool_t FASTCALL _cairo_surface_get_extents(cairo_surface_t * surface, CairoIRect   * extents);
cairo_private cairo_bool_t FASTCALL _cairo_surface_has_device_transform(cairo_surface_t * surface) cairo_pure;
cairo_private void _cairo_surface_release_device_reference(cairo_surface_t * surface);

/* cairo-image-surface.c */

/* XXX: In cairo 1.2.0 we added a new %CAIRO_FORMAT_RGB16_565 but
 * neglected to adjust this macro. The net effect is that it's
 * impossible to externally create an image surface with this
 * format. This is perhaps a good thing since we also neglected to fix
 * up things like cairo_surface_write_to_png() for the new format
 * (-Wswitch-enum will tell you where). Is it obvious that format was
 * added in haste?
 *
 * The reason for the new format was to allow the xlib backend to be
 * used on X servers with a 565 visual. So the new format did its job
 * for that, even without being considered "valid" for the sake of
 * things like cairo_image_surface_create().
 *
 * Since 1.2.0 we ran into the same situtation with X servers with BGR
 * visuals. This time we invented #cairo_internal_format_t instead,
 * (see it for more discussion).
 *
 * The punchline is that %CAIRO_FORMAT_VALID must not conside any
 * internal format to be valid. Also we need to decide if the
 * RGB16_565 should be moved to instead be an internal format. If so,
 * this macro need not change for it. (We probably will need to leave
 * an RGB16_565 value in the header files for the sake of code that
 * might have that value in it.)
 *
 * If we do decide to start fully supporting RGB16_565 as an external
 * format, then %CAIRO_FORMAT_VALID needs to be adjusted to include
 * it. But that should not happen before all necessary code is fixed
 * to support it (at least cairo_surface_write_to_png() and a few spots
 * in cairo-xlib-surface.c--again see -Wswitch-enum).
 */
#define CAIRO_FORMAT_VALID(format) ((format) >= CAIRO_FORMAT_ARGB32 && (format) <= CAIRO_FORMAT_RGB30)

/* pixman-required stride alignment in bytes. */
#define CAIRO_STRIDE_ALIGNMENT (sizeof(uint32_t))
#define CAIRO_STRIDE_FOR_WIDTH_BPP(w, bpp) ((((bpp)*(w)+7)/8 + CAIRO_STRIDE_ALIGNMENT-1) & -CAIRO_STRIDE_ALIGNMENT)
#define CAIRO_CONTENT_VALID(content) ((content) && (((content) & ~(CAIRO_CONTENT_COLOR|CAIRO_CONTENT_ALPHA|CAIRO_CONTENT_COLOR_ALPHA)) == 0))

cairo_private int FASTCALL _cairo_format_bits_per_pixel(cairo_format_t format) cairo_const;
cairo_private cairo_format_t FASTCALL _cairo_format_from_content(cairo_content_t content) cairo_const;
cairo_private cairo_format_t FASTCALL _cairo_format_from_pixman_format(pixman_format_code_t pixman_format);
cairo_private cairo_content_t FASTCALL _cairo_content_from_format(cairo_format_t format) cairo_const;
cairo_private cairo_content_t _cairo_content_from_pixman_format(pixman_format_code_t pixman_format);
cairo_private cairo_surface_t * _cairo_image_surface_create_for_pixman_image(pixman_image_t * pixman_image, pixman_format_code_t pixman_format);
cairo_private pixman_format_code_t FASTCALL _cairo_format_to_pixman_format_code(cairo_format_t format);
cairo_private cairo_bool_t FASTCALL _pixman_format_from_masks(cairo_format_masks_t * masks, pixman_format_code_t * format_ret);
cairo_private cairo_bool_t FASTCALL _pixman_format_to_masks(pixman_format_code_t pixman_format, cairo_format_masks_t * masks);
cairo_private void _cairo_image_scaled_glyph_fini(cairo_scaled_font_t * scaled_font, cairo_scaled_glyph_t * scaled_glyph);
cairo_private void _cairo_image_reset_static_data(void);
cairo_private cairo_surface_t * _cairo_image_surface_create_with_pixman_format(uchar * data, pixman_format_code_t pixman_format, int width, int height, int stride);
cairo_private cairo_surface_t * _cairo_image_surface_create_with_content(cairo_content_t content, int width, int height);
cairo_private void _cairo_image_surface_assume_ownership_of_data(cairo_image_surface_t * surface);
cairo_private cairo_image_surface_t * _cairo_image_surface_coerce(cairo_image_surface_t * surface);
cairo_private cairo_image_surface_t * _cairo_image_surface_coerce_to_format(cairo_image_surface_t * surface, cairo_format_t format);
cairo_private cairo_image_transparency_t _cairo_image_analyze_transparency(cairo_image_surface_t  * image);
cairo_private cairo_image_color_t _cairo_image_analyze_color(cairo_image_surface_t * image);
/* cairo-pen.c */
cairo_private int _cairo_pen_vertices_needed(double tolerance, double radius, const cairo_matrix_t  * matrix);
cairo_private cairo_status_t _cairo_pen_init(cairo_pen_t * pen, double radius, double tolerance, const cairo_matrix_t   * ctm);

cairo_private void _cairo_pen_init_empty(cairo_pen_t * pen);
cairo_private cairo_status_t _cairo_pen_init_copy(cairo_pen_t * pen, const cairo_pen_t * other);
cairo_private void _cairo_pen_fini(cairo_pen_t * pen);
cairo_private cairo_status_t _cairo_pen_add_points(cairo_pen_t * pen, cairo_point_t * point, int num_points);
cairo_private int _cairo_pen_find_active_cw_vertex_index(const cairo_pen_t * pen, const cairo_slope_t * slope);
cairo_private int _cairo_pen_find_active_ccw_vertex_index(const cairo_pen_t * pen, const cairo_slope_t * slope);
cairo_private void _cairo_pen_find_active_cw_vertices(const cairo_pen_t * pen,
    const cairo_slope_t * in, const cairo_slope_t * out, int * start, int * stop);
cairo_private void _cairo_pen_find_active_ccw_vertices(const cairo_pen_t * pen,
    const cairo_slope_t * in, const cairo_slope_t * out, int * start, int * stop);
/* cairo-polygon.c */
cairo_private void _cairo_polygon_init(cairo_polygon_t   * polygon, const cairo_box_t * boxes, int num_boxes);
cairo_private void _cairo_polygon_init_with_clip(cairo_polygon_t * polygon, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_polygon_init_boxes(cairo_polygon_t * polygon, const cairo_boxes_t * boxes);
cairo_private cairo_status_t _cairo_polygon_init_box_array(cairo_polygon_t * polygon, cairo_box_t * boxes, int num_boxes);
cairo_private void _cairo_polygon_limit(cairo_polygon_t * polygon, const cairo_box_t * limits, int num_limits);
cairo_private void _cairo_polygon_limit_to_clip(cairo_polygon_t * polygon, const cairo_clip_t * clip);
cairo_private void _cairo_polygon_fini(cairo_polygon_t * polygon);
cairo_private_no_warn cairo_status_t _cairo_polygon_add_line(cairo_polygon_t * polygon,
    const cairo_line_t * line, int top, int bottom, int dir);
cairo_private_no_warn cairo_status_t _cairo_polygon_add_external_edge(void * polygon,
    const cairo_point_t * p1, const cairo_point_t * p2);
cairo_private_no_warn cairo_status_t _cairo_polygon_add_contour(cairo_polygon_t * polygon, const cairo_contour_t * contour);
cairo_private void _cairo_polygon_translate(cairo_polygon_t * polygon, int dx, int dy);
cairo_private cairo_status_t _cairo_polygon_reduce(cairo_polygon_t * polygon, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_polygon_intersect(cairo_polygon_t * a, int winding_a, cairo_polygon_t * b, int winding_b);
cairo_private cairo_status_t _cairo_polygon_intersect_with_boxes(cairo_polygon_t * polygon, CairoFillRule * winding, cairo_box_t * boxes, int num_boxes);

static inline cairo_bool_t _cairo_polygon_is_empty(const cairo_polygon_t * polygon)
{
	return (polygon->num_edges == 0 || polygon->extents.p2.x <= polygon->extents.p1.x);
}

#define _cairo_polygon_status(P) ((cairo_polygon_t*)(P))->status

/* cairo-spline.c */
cairo_private cairo_bool_t _cairo_spline_init(cairo_spline_t * spline,
    cairo_spline_add_point_func_t add_point_func, void * closure,
    const cairo_point_t * a, const cairo_point_t * b, const cairo_point_t * c, const cairo_point_t * d);
cairo_private cairo_status_t _cairo_spline_decompose(cairo_spline_t * spline, double tolerance);
cairo_private cairo_status_t _cairo_spline_bound(cairo_spline_add_point_func_t add_point_func,
    void * closure, const cairo_point_t * p0, const cairo_point_t * p1, const cairo_point_t * p2, const cairo_point_t * p3);
/* cairo-matrix.c */
cairo_private void _cairo_matrix_get_affine(const cairo_matrix_t * matrix, double * xx, double * yx, double * xy, double * yy, double * x0, double * y0);
cairo_private void _cairo_matrix_transform_bounding_box(const cairo_matrix_t * matrix, double * x1, double * y1, double * x2, double * y2, cairo_bool_t * is_tight);

cairo_private void FASTCALL _cairo_matrix_transform_bounding_box_fixed(const cairo_matrix_t * matrix, cairo_box_t * bbox, cairo_bool_t * is_tight);
cairo_private cairo_bool_t FASTCALL _cairo_matrix_is_invertible(const cairo_matrix_t * matrix) cairo_pure;
cairo_private cairo_bool_t FASTCALL _cairo_matrix_is_scale_0(const cairo_matrix_t * matrix) cairo_pure;
cairo_private double FASTCALL _cairo_matrix_compute_determinant(const cairo_matrix_t * matrix) cairo_pure;
cairo_private cairo_status_t _cairo_matrix_compute_basis_scale_factors(const cairo_matrix_t * matrix, double * sx, double * sy, int x_major);
/*
static inline cairo_bool_t _cairo_matrix_is_identity(const cairo_matrix_t * matrix)
{
	return (matrix->xx == 1.0 && matrix->yx == 0.0 && matrix->xy == 0.0 && matrix->yy == 1.0 && matrix->x0 == 0.0 && matrix->y0 == 0.0);
}

static inline cairo_bool_t _cairo_matrix_is_translation(const cairo_matrix_t * matrix)
{
	return (matrix->xx == 1.0 && matrix->yx == 0.0 && matrix->xy == 0.0 && matrix->yy == 1.0);
}

static inline cairo_bool_t _cairo_matrix_is_scale(const cairo_matrix_t * matrix)
{
	return matrix->yx == 0.0 && matrix->xy == 0.0;
}
*/
cairo_bool_t FASTCALL _cairo_matrix_is_identity(const cairo_matrix_t * matrix);
cairo_bool_t FASTCALL _cairo_matrix_is_translation(const cairo_matrix_t * matrix);
cairo_bool_t FASTCALL _cairo_matrix_is_scale(const cairo_matrix_t * matrix);

cairo_private cairo_bool_t FASTCALL _cairo_matrix_is_integer_translation(const cairo_matrix_t * matrix, int * itx, int * ity);
cairo_private cairo_bool_t FASTCALL _cairo_matrix_has_unity_scale(const cairo_matrix_t * matrix);
cairo_private cairo_bool_t FASTCALL _cairo_matrix_is_pixel_exact(const cairo_matrix_t * matrix) cairo_pure;
cairo_private double FASTCALL _cairo_matrix_transformed_circle_major_axis(const cairo_matrix_t * matrix, double radius) cairo_pure;
cairo_private cairo_bool_t _cairo_matrix_is_pixman_translation(const cairo_matrix_t * matrix, cairo_filter_t filter, int * out_x_offset, int * out_y_offset);
cairo_private cairo_status_t _cairo_matrix_to_pixman_matrix_offset(const cairo_matrix_t * matrix,
    cairo_filter_t filter, double xc, double yc, pixman_transform_t * out_transform, int * out_x_offset, int * out_y_offset);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_rectilinear_polygon(cairo_traps_t * traps, const cairo_polygon_t * polygon, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_polygon(cairo_traps_t * traps, const cairo_polygon_t * polygon, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_traps(cairo_traps_t * traps, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_rectangular_traps(cairo_traps_t * traps, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_boxes(const cairo_boxes_t * in, CairoFillRule fill_rule, cairo_boxes_t * out);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_rectilinear_traps(cairo_traps_t * traps, CairoFillRule fill_rule);
cairo_private cairo_status_t _cairo_bentley_ottmann_tessellate_rectilinear_polygon_to_boxes(const cairo_polygon_t * polygon,
    CairoFillRule fill_rule, cairo_boxes_t * boxes);
cairo_private void _cairo_trapezoid_array_translate_and_scale(cairo_trapezoid_t * offset_traps,
    cairo_trapezoid_t * src_traps, int num_traps, double tx, double ty, double sx, double sy);
#if CAIRO_HAS_DRM_SURFACE
	cairo_private void _cairo_drm_device_reset_static_data(void);
#endif
cairo_private void _cairo_clip_reset_static_data(void);
cairo_private void _cairo_pattern_reset_static_data(void);

// cairo-unicode.c 
cairo_private int FASTCALL _cairo_utf8_get_char_validated(const char * p, uint32_t   * unicode);
cairo_private cairo_status_t _cairo_utf8_to_ucs4(const char * str, int len, uint32_t  ** result, int * items_written);
cairo_private int FASTCALL _cairo_ucs4_to_utf8(uint32_t unicode, char * utf8);
#if CAIRO_HAS_WIN32_FONT || CAIRO_HAS_QUARTZ_FONT || CAIRO_HAS_PDF_OPERATORS
	#define CAIRO_HAS_UTF8_TO_UTF16 1
#endif
#if CAIRO_HAS_UTF8_TO_UTF16
	cairo_private cairo_status_t _cairo_utf8_to_utf16(const char * str, int len, uint16  ** result, int * items_written);
#endif
cairo_private void _cairo_matrix_multiply(cairo_matrix_t * r, const cairo_matrix_t * a, const cairo_matrix_t * b);

// cairo-observer.c 
cairo_private void FASTCALL _cairo_observers_notify(cairo_list_t * observers, void * arg);

// Avoid unnecessary PLT entries
slim_hidden_proto(cairo_clip_preserve);
slim_hidden_proto(cairo_close_path);
slim_hidden_proto(cairo_create);
slim_hidden_proto(cairo_curve_to);
slim_hidden_proto(cairo_destroy);
slim_hidden_proto(cairo_fill_preserve);
slim_hidden_proto(cairo_font_face_destroy);
slim_hidden_proto(cairo_font_face_get_user_data);
slim_hidden_proto_no_warn(cairo_font_face_reference);
slim_hidden_proto(cairo_font_face_set_user_data);
slim_hidden_proto(cairo_font_options_equal);
slim_hidden_proto(cairo_font_options_hash);
slim_hidden_proto(cairo_font_options_merge);
slim_hidden_proto(cairo_font_options_set_antialias);
slim_hidden_proto(cairo_font_options_set_hint_metrics);
slim_hidden_proto(cairo_font_options_set_hint_style);
slim_hidden_proto(cairo_font_options_set_subpixel_order);
slim_hidden_proto(cairo_font_options_status);
slim_hidden_proto(cairo_format_stride_for_width);
slim_hidden_proto(cairo_get_current_point);
slim_hidden_proto(cairo_get_line_width);
slim_hidden_proto(cairo_get_matrix);
slim_hidden_proto(cairo_get_scaled_font);
slim_hidden_proto(cairo_get_target);
slim_hidden_proto(cairo_get_tolerance);
slim_hidden_proto(cairo_glyph_allocate);
slim_hidden_proto(cairo_glyph_free);
slim_hidden_proto(cairo_image_surface_create);
slim_hidden_proto(cairo_image_surface_create_for_data);
slim_hidden_proto(cairo_image_surface_get_data);
slim_hidden_proto(cairo_image_surface_get_format);
slim_hidden_proto(cairo_image_surface_get_height);
slim_hidden_proto(cairo_image_surface_get_stride);
slim_hidden_proto(cairo_image_surface_get_width);
slim_hidden_proto(cairo_line_to);
slim_hidden_proto(cairo_mask);
slim_hidden_proto(cairo_matrix_init);
slim_hidden_proto(cairo_matrix_init_identity);
slim_hidden_proto(cairo_matrix_init_rotate);
slim_hidden_proto(cairo_matrix_init_scale);
slim_hidden_proto(cairo_matrix_init_translate);
slim_hidden_proto(cairo_matrix_invert);
slim_hidden_proto(cairo_matrix_multiply);
slim_hidden_proto(cairo_matrix_scale);
slim_hidden_proto(cairo_matrix_transform_distance);
slim_hidden_proto(cairo_matrix_transform_point);
slim_hidden_proto(cairo_matrix_translate);
slim_hidden_proto(cairo_move_to);
slim_hidden_proto(cairo_new_path);
slim_hidden_proto(cairo_paint);
slim_hidden_proto(cairo_pattern_add_color_stop_rgba);
slim_hidden_proto(cairo_pattern_create_for_surface);
slim_hidden_proto(cairo_pattern_create_rgb);
slim_hidden_proto(cairo_pattern_create_rgba);
slim_hidden_proto(cairo_pattern_destroy);
slim_hidden_proto(cairo_pattern_get_extend);
slim_hidden_proto(cairo_mesh_pattern_curve_to);
slim_hidden_proto(cairo_mesh_pattern_get_control_point);
slim_hidden_proto(cairo_mesh_pattern_get_corner_color_rgba);
slim_hidden_proto(cairo_mesh_pattern_get_patch_count);
slim_hidden_proto(cairo_mesh_pattern_get_path);
slim_hidden_proto(cairo_mesh_pattern_line_to);
slim_hidden_proto(cairo_mesh_pattern_move_to);
slim_hidden_proto(cairo_mesh_pattern_set_corner_color_rgba);
slim_hidden_proto_no_warn(cairo_pattern_reference);
slim_hidden_proto(cairo_pattern_set_matrix);
slim_hidden_proto(cairo_pop_group);
slim_hidden_proto(cairo_push_group_with_content);
slim_hidden_proto_no_warn(cairo_path_destroy);
slim_hidden_proto(cairo_recording_surface_create);
slim_hidden_proto(cairo_rel_line_to);
slim_hidden_proto(cairo_restore);
slim_hidden_proto(cairo_save);
slim_hidden_proto(cairo_scale);
slim_hidden_proto(cairo_scaled_font_create);
slim_hidden_proto(cairo_scaled_font_destroy);
slim_hidden_proto(cairo_scaled_font_extents);
slim_hidden_proto(cairo_scaled_font_get_ctm);
slim_hidden_proto(cairo_scaled_font_get_font_face);
slim_hidden_proto(cairo_scaled_font_get_font_matrix);
slim_hidden_proto(cairo_scaled_font_get_font_options);
slim_hidden_proto(cairo_scaled_font_glyph_extents);
slim_hidden_proto_no_warn(cairo_scaled_font_reference);
slim_hidden_proto(cairo_scaled_font_status);
slim_hidden_proto(cairo_scaled_font_get_user_data);
slim_hidden_proto(cairo_scaled_font_set_user_data);
slim_hidden_proto(cairo_scaled_font_text_to_glyphs);
slim_hidden_proto(cairo_set_font_matrix);
slim_hidden_proto(cairo_set_font_options);
slim_hidden_proto(cairo_set_font_size);
slim_hidden_proto(cairo_set_line_cap);
slim_hidden_proto(cairo_set_line_join);
slim_hidden_proto(cairo_set_line_width);
slim_hidden_proto(cairo_set_matrix);
slim_hidden_proto(cairo_set_operator);
slim_hidden_proto(cairo_set_source);
slim_hidden_proto(cairo_set_source_rgb);
slim_hidden_proto(cairo_set_source_surface);
slim_hidden_proto(cairo_set_tolerance);
slim_hidden_proto(cairo_status);
slim_hidden_proto(cairo_stroke);
slim_hidden_proto(cairo_stroke_preserve);
slim_hidden_proto(cairo_surface_copy_page);
slim_hidden_proto(cairo_surface_create_similar_image);
slim_hidden_proto(cairo_surface_destroy);
slim_hidden_proto(cairo_surface_finish);
slim_hidden_proto(cairo_surface_flush);
slim_hidden_proto(cairo_surface_get_device_offset);
slim_hidden_proto(cairo_surface_get_device_scale);
slim_hidden_proto(cairo_surface_get_font_options);
slim_hidden_proto(cairo_surface_get_mime_data);
slim_hidden_proto(cairo_surface_has_show_text_glyphs);
slim_hidden_proto(cairo_surface_mark_dirty);
slim_hidden_proto(cairo_surface_mark_dirty_rectangle);
slim_hidden_proto_no_warn(cairo_surface_reference);
slim_hidden_proto(cairo_surface_set_device_offset);
slim_hidden_proto(cairo_surface_set_device_scale);
slim_hidden_proto(cairo_surface_set_fallback_resolution);
slim_hidden_proto(cairo_surface_set_mime_data);
slim_hidden_proto(cairo_surface_show_page);
slim_hidden_proto(cairo_surface_status);
slim_hidden_proto(cairo_surface_supports_mime_type);
slim_hidden_proto(cairo_text_cluster_allocate);
slim_hidden_proto(cairo_text_cluster_free);
slim_hidden_proto(cairo_toy_font_face_create);
slim_hidden_proto(cairo_toy_font_face_get_slant);
slim_hidden_proto(cairo_toy_font_face_get_weight);
slim_hidden_proto(cairo_translate);
slim_hidden_proto(cairo_transform);
slim_hidden_proto(cairo_user_font_face_create);
slim_hidden_proto(cairo_user_font_face_set_init_func);
slim_hidden_proto(cairo_user_font_face_set_render_glyph_func);
slim_hidden_proto(cairo_user_font_face_set_unicode_to_glyph_func);
slim_hidden_proto(cairo_device_to_user);
slim_hidden_proto(cairo_user_to_device);
slim_hidden_proto(cairo_user_to_device_distance);
slim_hidden_proto(cairo_version_string);
slim_hidden_proto(cairo_region_create);
slim_hidden_proto(cairo_region_create_rectangle);
slim_hidden_proto(cairo_region_create_rectangles);
slim_hidden_proto(cairo_region_copy);
slim_hidden_proto(cairo_region_reference);
slim_hidden_proto(cairo_region_destroy);
slim_hidden_proto(cairo_region_equal);
slim_hidden_proto(cairo_region_status);
slim_hidden_proto(cairo_region_get_extents);
slim_hidden_proto(cairo_region_num_rectangles);
slim_hidden_proto(cairo_region_get_rectangle);
slim_hidden_proto(cairo_region_is_empty);
slim_hidden_proto(cairo_region_contains_rectangle);
slim_hidden_proto(cairo_region_contains_point);
slim_hidden_proto(cairo_region_translate);
slim_hidden_proto(cairo_region_subtract);
slim_hidden_proto(cairo_region_subtract_rectangle);
slim_hidden_proto(cairo_region_intersect);
slim_hidden_proto(cairo_region_intersect_rectangle);
slim_hidden_proto(cairo_region_union);
slim_hidden_proto(cairo_region_union_rectangle);
slim_hidden_proto(cairo_region_xor);
slim_hidden_proto(cairo_region_xor_rectangle);

#if CAIRO_HAS_PNG_FUNCTIONS
	slim_hidden_proto(cairo_surface_write_to_png_stream);
#endif

CAIRO_END_DECLS
//
//#include "cairo-mutex-impl-private.h"
//
#if HAVE_LOCKDEP
	#include <lockdep.h>
#endif
// A fully qualified no-operation statement
#define CAIRO_MUTEX_IMPL_NOOP	do {/*no-op*/} while (0)
// And one that evaluates its argument once
#define CAIRO_MUTEX_IMPL_NOOP1(expr)        do { (void)(expr); } while (0)
// Note: 'if (expr) {}' is an alternative to '(void)(expr);' that will 'use' the
// result of __attribute__((warn_used_result)) functions. 
//
/* Cairo mutex implementation:
 *
 * Any new mutex implementation needs to do the following:
 *
 * - Condition on the right header or feature.  Headers are
 *   preferred as eg. you still can use win32 mutex implementation
 *   on a win32 system even if you do not compile the win32
 *   surface/backend.
 *
 * - typedef #cairo_mutex_impl_t to the proper mutex type on your target
 *   system.  Note that you may or may not need to use a pointer,
 *   depending on what kinds of initialization your mutex
 *   implementation supports.  No trailing semicolon needed.
 *   You should be able to compile the following snippet (don't try
 *   running it):
 *
 *   <programlisting>
 *	cairo_mutex_impl_t _cairo_some_mutex;
 *   </programlisting>
 *
 * - #define %CAIRO_MUTEX_IMPL_<NAME> 1 with suitable name for your platform.  You
 *   can later use this symbol in cairo-system.c.
 *
 * - #define CAIRO_MUTEX_IMPL_LOCK(mutex) and CAIRO_MUTEX_IMPL_UNLOCK(mutex) to
 *   proper statement to lock/unlock the mutex object passed in.
 *   You can (and should) assume that the mutex is already
 *   initialized, and is-not-already-locked/is-locked,
 *   respectively.  Use the "do { ... } while (0)" idiom if necessary.
 *   No trailing semicolons are needed (in any macro you define here).
 *   You should be able to compile the following snippet:
 *
 *   <programlisting>
 *	cairo_mutex_impl_t _cairo_some_mutex;
 *
 *      if (1)
 *          CAIRO_MUTEX_IMPL_LOCK (_cairo_some_mutex);
 *      else
 *          CAIRO_MUTEX_IMPL_UNLOCK (_cairo_some_mutex);
 *   </programlisting>
 *
 * - #define %CAIRO_MUTEX_IMPL_NIL_INITIALIZER to something that can
 *   initialize the #cairo_mutex_impl_t type you defined.  Most of the
 *   time one of 0, %NULL, or {} works.  At this point
 *   you should be able to compile the following snippet:
 *
 *   <programlisting>
 *	cairo_mutex_impl_t _cairo_some_mutex = CAIRO_MUTEX_IMPL_NIL_INITIALIZER;
 *
 *      if (1)
 *          CAIRO_MUTEX_IMPL_LOCK (_cairo_some_mutex);
 *      else
 *          CAIRO_MUTEX_IMPL_UNLOCK (_cairo_some_mutex);
 *   </programlisting>
 *
 * - If the above code is not enough to initialize a mutex on
 *   your platform, #define CAIRO_MUTEX_IMPL_INIT(mutex) to statement
 *   to initialize the mutex (allocate resources, etc).  Such that
 *   you should be able to compile AND RUN the following snippet:
 *
 *   <programlisting>
 *	cairo_mutex_impl_t _cairo_some_mutex = CAIRO_MUTEX_IMPL_NIL_INITIALIZER;
 *
 *      CAIRO_MUTEX_IMPL_INIT (_cairo_some_mutex);
 *
 *      if (1)
 *          CAIRO_MUTEX_IMPL_LOCK (_cairo_some_mutex);
 *      else
 *          CAIRO_MUTEX_IMPL_UNLOCK (_cairo_some_mutex);
 *   </programlisting>
 *
 * - If you define CAIRO_MUTEX_IMPL_INIT(mutex), cairo will use it to
 *   initialize all static mutex'es.  If for any reason that should
 *   not happen (eg. %CAIRO_MUTEX_IMPL_INIT is just a faster way than
 *   what cairo does using %CAIRO_MUTEX_IMPL_NIL_INITIALIZER), then
 *   <programlisting>
 *      #define CAIRO_MUTEX_IMPL_INITIALIZE() CAIRO_MUTEX_IMPL_NOOP
 *   </programlisting>
 *
 * - If your system supports freeing a mutex object (deallocating
 *   resources, etc), then #define CAIRO_MUTEX_IMPL_FINI(mutex) to do
 *   that.
 *
 * - If you define CAIRO_MUTEX_IMPL_FINI(mutex), cairo will use it to
 *   define a finalizer function to finalize all static mutex'es.
 *   However, it's up to you to call CAIRO_MUTEX_IMPL_FINALIZE() at
 *   proper places, eg. when the system is unloading the cairo library.
 *   So, if for any reason finalizing static mutex'es is not needed
 *   (eg. you never call CAIRO_MUTEX_IMPL_FINALIZE()), then
 *   <programlisting>
 *      #define CAIRO_MUTEX_IMPL_FINALIZE() CAIRO_MUTEX_IMPL_NOOP
 *   </programlisting>
 *
 * - That is all.  If for any reason you think the above API is
 *   not enough to implement #cairo_mutex_impl_t on your system, please
 *   stop and write to the cairo mailing list about it.  DO NOT
 *   poke around cairo-mutex-private.h for possible solutions.
 */
#if CAIRO_NO_MUTEX
	//
	// No mutexes
	//
	typedef int cairo_mutex_impl_t;
	typedef int cairo_recursive_mutex_impl_t;

	#define CAIRO_MUTEX_IMPL_NO 1
	#define CAIRO_MUTEX_IMPL_INITIALIZE() CAIRO_MUTEX_IMPL_NOOP
	#define CAIRO_MUTEX_IMPL_LOCK(mutex) CAIRO_MUTEX_IMPL_NOOP1(mutex)
	#define CAIRO_MUTEX_IMPL_UNLOCK(mutex) CAIRO_MUTEX_IMPL_NOOP1(mutex)
	#define CAIRO_MUTEX_IMPL_NIL_INITIALIZER 0
	#define CAIRO_MUTEX_HAS_RECURSIVE_IMPL 1
	#define CAIRO_RECURSIVE_MUTEX_IMPL_INIT(mutex)
	#define CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER 0
#elif defined(_WIN32) /******************************************************/
	#define WIN32_LEAN_AND_MEAN
	/* We require Windows 2000 features such as ETO_PDY */
	#if !defined(WINVER) || (WINVER < 0x0600) // @sobolev 0x0500-->0x0600
		#define WINVER 0x0600 // @sobolev 0x0500-->0x0600
	#endif
	#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0600) // @sobolev 0x0500-->0x0600
		#define _WIN32_WINNT 0x0600 // @sobolev 0x0500-->0x0600
	#endif
	//#include <windows.h>

	typedef CRITICAL_SECTION cairo_mutex_impl_t;

	#define CAIRO_MUTEX_IMPL_WIN32 1
	#define CAIRO_MUTEX_IMPL_LOCK(mutex) EnterCriticalSection(&(mutex))
	#define CAIRO_MUTEX_IMPL_UNLOCK(mutex) LeaveCriticalSection(&(mutex))
	#define CAIRO_MUTEX_IMPL_INIT(mutex) InitializeCriticalSection(&(mutex))
	#define CAIRO_MUTEX_IMPL_FINI(mutex) DeleteCriticalSection(&(mutex))
	#define CAIRO_MUTEX_IMPL_NIL_INITIALIZER { NULL, 0, 0, NULL, NULL, 0 }
#elif defined __OS2__ /******************************************************/
	#define INCL_BASE
	#define INCL_PM
	#include <os2.h>

	typedef HMTX cairo_mutex_impl_t;

	#define CAIRO_MUTEX_IMPL_OS2 1
	#define CAIRO_MUTEX_IMPL_LOCK(mutex) DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT)
	#define CAIRO_MUTEX_IMPL_UNLOCK(mutex) DosReleaseMutexSem(mutex)
	#define CAIRO_MUTEX_IMPL_INIT(mutex) DosCreateMutexSem (NULL, &(mutex), 0L, FALSE)
	#define CAIRO_MUTEX_IMPL_FINI(mutex) DosCloseMutexSem (mutex)
	#define CAIRO_MUTEX_IMPL_NIL_INITIALIZER 0
#elif CAIRO_HAS_BEOS_SURFACE /***********************************************/
	typedef BLocker * cairo_mutex_impl_t;

	#define CAIRO_MUTEX_IMPL_BEOS 1
	#define CAIRO_MUTEX_IMPL_LOCK(mutex) (mutex)->Lock()
	#define CAIRO_MUTEX_IMPL_UNLOCK(mutex) (mutex)->Unlock()
	#define CAIRO_MUTEX_IMPL_INIT(mutex) (mutex) = new BLocker()
	#define CAIRO_MUTEX_IMPL_FINI(mutex) delete (mutex)
	#define CAIRO_MUTEX_IMPL_NIL_INITIALIZER NULL
#elif CAIRO_HAS_PTHREAD /* and finally if there are no native mutexes ********/

	#include <pthread.h>

	typedef pthread_mutex_t cairo_mutex_impl_t;
	typedef pthread_mutex_t cairo_recursive_mutex_impl_t;

	#define CAIRO_MUTEX_IMPL_PTHREAD 1
	#if HAVE_LOCKDEP
		// expose all mutexes to the validator 
		#define CAIRO_MUTEX_IMPL_INIT(mutex) pthread_mutex_init (&(mutex), NULL)
	#endif
		#define CAIRO_MUTEX_IMPL_LOCK(mutex) pthread_mutex_lock (&(mutex))
		#define CAIRO_MUTEX_IMPL_UNLOCK(mutex) pthread_mutex_unlock (&(mutex))
	#if HAVE_LOCKDEP
		#define CAIRO_MUTEX_IS_LOCKED(mutex) LOCKDEP_IS_LOCKED (&(mutex))
		#define CAIRO_MUTEX_IS_UNLOCKED(mutex) LOCKDEP_IS_UNLOCKED (&(mutex))
	#endif
		#define CAIRO_MUTEX_IMPL_FINI(mutex) pthread_mutex_destroy (&(mutex))
	#if ! HAVE_LOCKDEP
		#define CAIRO_MUTEX_IMPL_FINALIZE() CAIRO_MUTEX_IMPL_NOOP
	#endif
	#define CAIRO_MUTEX_IMPL_NIL_INITIALIZER PTHREAD_MUTEX_INITIALIZER
	#define CAIRO_MUTEX_HAS_RECURSIVE_IMPL 1
	#define CAIRO_RECURSIVE_MUTEX_IMPL_INIT(mutex) do { \
		pthread_mutexattr_t attr; \
		pthread_mutexattr_init (&attr); \
		pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE); \
		pthread_mutex_init (&(mutex), &attr); \
		pthread_mutexattr_destroy (&attr); \
	} while (0)
	#define CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#else
	#error "XXX: No mutex implementation found.  Cairo will not work with multiple threads.  Define CAIRO_NO_MUTEX to 1 to acknowledge and accept this limitation and compile cairo without thread-safety support."
#endif
//
// By default mutex implementations are assumed to be recursive
//
#if ! CAIRO_MUTEX_HAS_RECURSIVE_IMPL
	#define CAIRO_MUTEX_HAS_RECURSIVE_IMPL 1
	typedef cairo_mutex_impl_t cairo_recursive_mutex_impl_t;
	#define CAIRO_RECURSIVE_MUTEX_IMPL_INIT(mutex) CAIRO_MUTEX_IMPL_INIT(mutex)
	#define CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER CAIRO_MUTEX_IMPL_NIL_INITIALIZER
#endif
//
//#include "cairo-mutex-type-private.h"
//
// Only the following four are mandatory at this point 
#ifndef CAIRO_MUTEX_IMPL_LOCK
	#error "CAIRO_MUTEX_IMPL_LOCK not defined.  Check cairo-mutex-impl-private.h."
#endif
#ifndef CAIRO_MUTEX_IMPL_UNLOCK
	#error "CAIRO_MUTEX_IMPL_UNLOCK not defined.  Check cairo-mutex-impl-private.h."
#endif
#ifndef CAIRO_MUTEX_IMPL_NIL_INITIALIZER
	#error "CAIRO_MUTEX_IMPL_NIL_INITIALIZER not defined.  Check cairo-mutex-impl-private.h."
#endif
#ifndef CAIRO_RECURSIVE_MUTEX_IMPL_INIT
	#error "CAIRO_RECURSIVE_MUTEX_IMPL_INIT not defined.  Check cairo-mutex-impl-private.h."
#endif
//
// make sure implementations don't fool us: we decide these ourself 
//
#undef _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER
#undef _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
#ifdef CAIRO_MUTEX_IMPL_INIT
	// If %CAIRO_MUTEX_IMPL_INIT is defined, we may need to initialize all static mutex'es. 
	#ifndef CAIRO_MUTEX_IMPL_INITIALIZE
		#define CAIRO_MUTEX_IMPL_INITIALIZE() do { if(!_cairo_mutex_initialized) _cairo_mutex_initialize(); } while(0)
		/* and make sure we implement the above */
		#define _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER 1
	#endif /* CAIRO_MUTEX_IMPL_INITIALIZE */
#else /* no CAIRO_MUTEX_IMPL_INIT */
// Otherwise we probably don't need to initialize static mutex'es, 
	#ifndef CAIRO_MUTEX_IMPL_INITIALIZE
		#define CAIRO_MUTEX_IMPL_INITIALIZE() CAIRO_MUTEX_IMPL_NOOP
	#endif /* CAIRO_MUTEX_IMPL_INITIALIZE */
	// and dynamic ones can be initialized using the static initializer. 
	#define CAIRO_MUTEX_IMPL_INIT(mutex) do {				\
			cairo_mutex_t _tmp_mutex = CAIRO_MUTEX_IMPL_NIL_INITIALIZER;	\
			memcpy (&(mutex), &_tmp_mutex, sizeof (_tmp_mutex));	\
		} while (0)
#endif /* CAIRO_MUTEX_IMPL_INIT */
#ifdef CAIRO_MUTEX_IMPL_FINI
	// If %CAIRO_MUTEX_IMPL_FINI is defined, we may need to finalize all static mutex'es. 
	#ifndef CAIRO_MUTEX_IMPL_FINALIZE
		#define CAIRO_MUTEX_IMPL_FINALIZE() do { if(_cairo_mutex_initialized) _cairo_mutex_finalize (); } while(0)
		/* and make sure we implement the above */
		#define _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER 1
	#endif /* CAIRO_MUTEX_IMPL_FINALIZE */
#else /* no CAIRO_MUTEX_IMPL_FINI */
	/* Otherwise we probably don't need to finalize static mutex'es, */
	#ifndef CAIRO_MUTEX_IMPL_FINALIZE
		#define CAIRO_MUTEX_IMPL_FINALIZE() CAIRO_MUTEX_IMPL_NOOP
	#endif /* CAIRO_MUTEX_IMPL_FINALIZE */
	/* neither do the dynamic ones. */
	#define CAIRO_MUTEX_IMPL_FINI(mutex)	CAIRO_MUTEX_IMPL_NOOP1(mutex)
#endif /* CAIRO_MUTEX_IMPL_FINI */
#ifndef _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER
	#define _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER 0
#endif
#ifndef _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
	#define _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER 0
#endif
// Make sure everything we want is defined 
#ifndef CAIRO_MUTEX_IMPL_INITIALIZE
	#error "CAIRO_MUTEX_IMPL_INITIALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_FINALIZE
	#error "CAIRO_MUTEX_IMPL_FINALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_LOCK
	#error "CAIRO_MUTEX_IMPL_LOCK not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_UNLOCK
	#error "CAIRO_MUTEX_IMPL_UNLOCK not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_INIT
	#error "CAIRO_MUTEX_IMPL_INIT not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_FINI
	#error "CAIRO_MUTEX_IMPL_FINI not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_NIL_INITIALIZER
	#error "CAIRO_MUTEX_IMPL_NIL_INITIALIZER not defined"
#endif
// 
// Public interface
// 
// By default it simply uses the implementation provided.
// But we can provide for debugging features by overriding them 
// 
#ifndef CAIRO_MUTEX_DEBUG
	typedef cairo_mutex_impl_t cairo_mutex_t;
	typedef cairo_recursive_mutex_impl_t cairo_recursive_mutex_t;
#else
	#define cairo_mutex_t cairo_mutex_impl_t
#endif
#define CAIRO_MUTEX_INITIALIZE		CAIRO_MUTEX_IMPL_INITIALIZE
#define CAIRO_MUTEX_FINALIZE		CAIRO_MUTEX_IMPL_FINALIZE
#define CAIRO_MUTEX_LOCK		CAIRO_MUTEX_IMPL_LOCK
#define CAIRO_MUTEX_UNLOCK		CAIRO_MUTEX_IMPL_UNLOCK
#define CAIRO_MUTEX_INIT		CAIRO_MUTEX_IMPL_INIT
#define CAIRO_MUTEX_FINI		CAIRO_MUTEX_IMPL_FINI
#define CAIRO_MUTEX_NIL_INITIALIZER	CAIRO_MUTEX_IMPL_NIL_INITIALIZER
#define CAIRO_RECURSIVE_MUTEX_INIT		CAIRO_RECURSIVE_MUTEX_IMPL_INIT
#define CAIRO_RECURSIVE_MUTEX_NIL_INITIALIZER	CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER
#ifndef CAIRO_MUTEX_IS_LOCKED
	#define CAIRO_MUTEX_IS_LOCKED(name) 1
#endif
#ifndef CAIRO_MUTEX_IS_UNLOCKED
	#define CAIRO_MUTEX_IS_UNLOCKED(name) 1
#endif
//
// Debugging support 
//
#ifdef CAIRO_MUTEX_DEBUG

/* @todo add mutex debugging facilities here (eg deadlock detection) */

#endif /* CAIRO_MUTEX_DEBUG */
//
//#include "cairo-mutex-private.h"
//
CAIRO_BEGIN_DECLS

#if _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER
	cairo_private void _cairo_mutex_initialize(void);
#endif
#if _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
	cairo_private void _cairo_mutex_finalize(void);
#endif
// only if using static initializer and/or finalizer define the boolean 
#if _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER || _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
	cairo_private extern cairo_bool_t _cairo_mutex_initialized;
#endif
// Finally, extern the static mutexes and undef 
#define CAIRO_MUTEX_DECLARE(mutex) cairo_private extern cairo_mutex_t mutex;
#include "cairo-mutex-list-private.h"
#undef CAIRO_MUTEX_DECLARE
//
//#include "cairo-scaled-font-private.h"
//
typedef struct _cairo_scaled_glyph_page cairo_scaled_glyph_page_t;

struct _cairo_scaled_font {
	/* For most cairo objects, the rule for multiple threads is that
	 * the user is responsible for any locking if the same object is
	 * manipulated from multiple threads simultaneously.
	 *
	 * However, with the caching that cairo does for scaled fonts, a
	 * user can easily end up with the same cairo_scaled_font object
	 * being manipulated from multiple threads without the user ever
	 * being aware of this, (and in fact, unable to control it).
	 *
	 * So, as a special exception, the cairo implementation takes care
	 * of all locking needed for cairo_scaled_font_t. Most of what is
	 * in the scaled font is immutable, (which is what allows for the
	 * sharing in the first place). The things that are modified and
	 * the locks protecting them are as follows:
	 *
	 * 1. The reference count (scaled_font->ref_count)
	 *
	 *    Modifications to the reference count are protected by the
	 *    _cairo_scaled_font_map_mutex. This is because the reference
	 *    count of a scaled font is intimately related with the font
	 *    map itself, (and the magic holdovers array).
	 *
	 * 2. The cache of glyphs (scaled_font->glyphs)
	 * 3. The backend private data (scaled_font->surface_backend,
	 *				    scaled_font->surface_private)
	 *
	 *    Modifications to these fields are protected with locks on
	 *    scaled_font->mutex in the generic scaled_font code.
	 */

	cairo_hash_entry_t hash_entry;
	cairo_status_t status; // useful bits for _cairo_scaled_font_nil 
	cairo_reference_count_t ref_count;
	cairo_user_data_array_t user_data;
	cairo_font_face_t * original_font_face; /* may be NULL */
	//
	// hash key members 
	//
	cairo_font_face_t * font_face; /* may be NULL */
	cairo_matrix_t font_matrix; /* font space => user space */
	cairo_matrix_t ctm;       /* user space => device space */
	cairo_font_options_t options;
	uint placeholder : 1; /*  protected by fontmap mutex */
	uint holdover : 1;
	uint finished : 1;
	//
	// "live" scaled_font members 
	//
	cairo_matrix_t scale;        /* font space => device space */
	cairo_matrix_t scale_inverse; /* device space => font space */
	double max_scale;            /* maximum x/y expansion of scale */
	cairo_font_extents_t extents; /* user space */
	cairo_font_extents_t fs_extents; /* font space */
	cairo_mutex_t mutex; // The mutex protects modification to all subsequent fields
	cairo_hash_table_t * glyphs;
	cairo_list_t glyph_pages;
	cairo_bool_t cache_frozen;
	cairo_bool_t global_cache_frozen;
	cairo_list_t dev_privates;
	const cairo_scaled_font_backend_t * backend; // font backend managing this scaled font 
	cairo_list_t link;
};

struct _cairo_scaled_font_private {
	cairo_list_t link;
	const void * key;
	void (* destroy)(cairo_scaled_font_private_t *, cairo_scaled_font_t *);
};

struct _cairo_scaled_glyph {
	cairo_hash_entry_t hash_entry;
	cairo_text_extents_t metrics;           /* user-space metrics */
	cairo_text_extents_t fs_metrics;        /* font-space metrics */
	cairo_box_t bbox;                       /* device-space bounds */
	int16 x_advance;                      /* device-space rounded X advance */
	int16 y_advance;                      /* device-space rounded Y advance */
	uint has_info;
	cairo_image_surface_t   * surface;      /* device-space image */
	cairo_path_fixed_t      * path;         /* device-space outline */
	cairo_surface_t         * recording_surface; /* device-space recording-surface */
	const void             * dev_private_key;
	void                   * dev_private;
	cairo_list_t dev_privates;
};

struct _cairo_scaled_glyph_private {
	cairo_list_t link;
	const void * key;
	void (* destroy)(cairo_scaled_glyph_private_t *, cairo_scaled_glyph_t *, cairo_scaled_font_t *);
};

cairo_private cairo_scaled_font_private_t * _cairo_scaled_font_find_private(cairo_scaled_font_t * scaled_font, const void * key);
cairo_private void _cairo_scaled_font_attach_private(cairo_scaled_font_t * scaled_font,
    cairo_scaled_font_private_t * priv, const void * key, void (* destroy)(cairo_scaled_font_private_t *, cairo_scaled_font_t *));
cairo_private cairo_scaled_glyph_private_t * _cairo_scaled_glyph_find_private(cairo_scaled_glyph_t * scaled_glyph, const void * key);
cairo_private void _cairo_scaled_glyph_attach_private(cairo_scaled_glyph_t * scaled_glyph, cairo_scaled_glyph_private_t * priv, 
	const void * key, void (* destroy)(cairo_scaled_glyph_private_t *, cairo_scaled_glyph_t *, cairo_scaled_font_t *));
//
//#include "cairo-line-inline.h"
//#include "cairo-line-private.h"
//
cairo_private int cairo_lines_compare_at_y(const cairo_line_t *a, const cairo_line_t *b, int y);
CAIRO_END_DECLS
//
static inline int cairo_lines_equal(const cairo_line_t *a, const cairo_line_t *b)
{
	return (a->p1.x == b->p1.x && a->p1.y == b->p1.y && a->p2.x == b->p2.x && a->p2.y == b->p2.y);
}
//
//#include "cairo-malloc-private.h"
//
#if HAVE_MEMFAULT
	#include <memfault.h>
	#define CAIRO_INJECT_FAULT() MEMFAULT_INJECT_FAULT()
#else
	#define CAIRO_INJECT_FAULT() 0
#endif

#define _cairo_malloc(size) ((size) ? SAlloc::M((uint)(size)) : NULL)
/**
 * _cairo_malloc_ab:
 * @a: number of elements to allocate
 * @size: size of each element
 *
 * Allocates @a*@size memory using _cairo_malloc(), taking care to not
 * overflow when doing the multiplication.  Behaves much like
 * SAlloc::C(), except that the returned memory is not set to zero.
 * The memory should be freed using SAlloc::F().
 *
 * @size should be a constant so that the compiler can optimize
 * out a constant division.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of SAlloc::M() failure or overflow.
 **/
#define _cairo_malloc_ab(a, size) ((size) && (uint)(a) >= INT32_MAX / (uint)(size) ? NULL : _cairo_malloc((uint)(a) * (uint)(size)))
/**
 * _cairo_realloc_ab:
 * @ptr: original pointer to block of memory to be resized
 * @a: number of elements to allocate
 * @size: size of each element
 *
 * Reallocates @ptr a block of @a*@size memory using realloc(), taking
 * care to not overflow when doing the multiplication.  The memory
 * should be freed using SAlloc::F().
 *
 * @size should be a constant so that the compiler can optimize
 * out a constant division.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of realloc() failure or overflow (whereupon the original block
 * of memory * is left untouched).
 **/
#define _cairo_realloc_ab(ptr, a, size) ((size) && (uint)(a) >= INT32_MAX / (uint)(size) ? NULL : SAlloc::R(ptr, (uint)(a) * (uint)(size)))
/**
 * _cairo_malloc_abc:
 * @a: first factor of number of elements to allocate
 * @b: second factor of number of elements to allocate
 * @size: size of each element
 *
 * Allocates @a*@b*@size memory using _cairo_malloc(), taking care to not
 * overflow when doing the multiplication.  Behaves like
 * _cairo_malloc_ab().  The memory should be freed using SAlloc::F().
 *
 * @size should be a constant so that the compiler can optimize
 * out a constant division.
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of SAlloc::M() failure or overflow.
 **/
#define _cairo_malloc_abc(a, b, size) ((b) && (uint)(a) >= INT32_MAX / (uint)(b) ? NULL : (size) && (uint)((a)*(b)) >= INT32_MAX / (uint)(size) ? NULL : _cairo_malloc((uint)(a) * (uint)(b) * (uint)(size)))
/**
 * _cairo_malloc_ab_plus_c:
 * @a: number of elements to allocate
 * @size: size of each element
 * @c: additional size to allocate
 *
 * Allocates @a*@size+@c memory using _cairo_malloc(), taking care to not
 * overflow when doing the arithmetic.  Behaves similar to
 * _cairo_malloc_ab().  The memory should be freed using SAlloc::F().
 *
 * Return value: A pointer to the newly allocated memory, or %NULL in
 * case of SAlloc::M() failure or overflow.
 **/
#define _cairo_malloc_ab_plus_c(a, size, c) ((size) && (uint)(a) >= INT32_MAX / (uint)(size) ? NULL : (uint)(c) >= INT32_MAX - (uint)(a) * (uint)(size) ? NULL : _cairo_malloc((uint)(a) * (uint)(size) + (uint)(c)))
//
//#include "cairo-hash-private.h"
// 
// XXX: I'd like this file to be self-contained in terms of
// includeability, but that's not really possible with the current
// monolithic cairoint.h. So, for now, just include cairoint.h instead
// if you want to include this file. 
// 
typedef cairo_bool_t (*cairo_hash_keys_equal_func_t)(const void * key_a, const void * key_b);
typedef cairo_bool_t (*cairo_hash_predicate_func_t)(const void * entry);
typedef void (*cairo_hash_callback_func_t)(void * entry, void * closure);

cairo_private cairo_hash_table_t * FASTCALL _cairo_hash_table_create(cairo_hash_keys_equal_func_t keys_equal);
cairo_private void FASTCALL _cairo_hash_table_destroy(cairo_hash_table_t * hash_table);
cairo_private void * FASTCALL _cairo_hash_table_lookup(cairo_hash_table_t  * hash_table, cairo_hash_entry_t  * key);
cairo_private void * FASTCALL _cairo_hash_table_random_entry(cairo_hash_table_t * hash_table, cairo_hash_predicate_func_t predicate);
cairo_private cairo_status_t FASTCALL _cairo_hash_table_insert(cairo_hash_table_t * hash_table, cairo_hash_entry_t * entry);
cairo_private void FASTCALL _cairo_hash_table_remove(cairo_hash_table_t * hash_table, cairo_hash_entry_t * key);
cairo_private void _cairo_hash_table_foreach(cairo_hash_table_t * hash_table, cairo_hash_callback_func_t hash_callback, void * closure);
//
//#include "cairo-analysis-surface-private.h"
//
cairo_private cairo_surface_t * FASTCALL _cairo_analysis_surface_create(cairo_surface_t * target);
cairo_private void _cairo_analysis_surface_set_ctm(cairo_surface_t * surface, const cairo_matrix_t  * ctm);
cairo_private void _cairo_analysis_surface_get_ctm(cairo_surface_t * surface, cairo_matrix_t  * ctm);
cairo_private cairo_region_t * _cairo_analysis_surface_get_supported(cairo_surface_t * surface);
cairo_private cairo_region_t * _cairo_analysis_surface_get_unsupported(cairo_surface_t * surface);
cairo_private cairo_bool_t _cairo_analysis_surface_has_supported(cairo_surface_t * surface);
cairo_private cairo_bool_t _cairo_analysis_surface_has_unsupported(cairo_surface_t * surface);
cairo_private void _cairo_analysis_surface_get_bounding_box(cairo_surface_t * surface, cairo_box_t * bbox);
cairo_private cairo_int_status_t _cairo_analysis_surface_merge_status(cairo_int_status_t status_a, cairo_int_status_t status_b);
cairo_private cairo_surface_t * _cairo_null_surface_create(cairo_content_t content);
//
//#include "cairo-boxes-private.h"
//
struct _cairo_boxes_t {
	cairo_status_t status;
	cairo_box_t limit;
	const cairo_box_t * limits;
	int num_limits;
	int num_boxes;
	uint is_pixel_aligned;
	struct _cairo_boxes_chunk {
		_cairo_boxes_chunk * next;
		cairo_box_t * base;
		int count;
		int size;
	};
	_cairo_boxes_chunk chunks;
	_cairo_boxes_chunk * tail;
	cairo_box_t boxes_embedded[32];
};

cairo_private void FASTCALL _cairo_boxes_init(cairo_boxes_t * boxes);
cairo_private void FASTCALL _cairo_boxes_init_with_clip(cairo_boxes_t * boxes, cairo_clip_t * clip);
cairo_private void _cairo_boxes_init_for_array(cairo_boxes_t * boxes, cairo_box_t * array, int num_boxes);
cairo_private void _cairo_boxes_init_from_rectangle(cairo_boxes_t * boxes, int x, int y, int w, int h);
cairo_private void FASTCALL _cairo_boxes_limit(cairo_boxes_t * boxes, const cairo_box_t   * limits, int num_limits);
cairo_private cairo_status_t _cairo_boxes_add(cairo_boxes_t * boxes, cairo_antialias_t antialias, const cairo_box_t * box);
cairo_private void FASTCALL _cairo_boxes_extents(const cairo_boxes_t * boxes, cairo_box_t * box);
cairo_private cairo_box_t * _cairo_boxes_to_array(const cairo_boxes_t * boxes, int * num_boxes, cairo_bool_t force_allocation);
cairo_private cairo_status_t _cairo_boxes_intersect(const cairo_boxes_t * a, const cairo_boxes_t * b, cairo_boxes_t * out);
cairo_private void FASTCALL _cairo_boxes_clear(cairo_boxes_t * boxes);
cairo_private_no_warn cairo_bool_t _cairo_boxes_for_each_box(cairo_boxes_t *boxes, cairo_bool_t (* func)(cairo_box_t * box, void * data), void * data);
cairo_private cairo_status_t _cairo_rasterise_polygon_to_boxes(cairo_polygon_t * polygon, CairoFillRule fill_rule, cairo_boxes_t * boxes);
cairo_private void FASTCALL _cairo_boxes_fini(cairo_boxes_t * boxes);
cairo_private void _cairo_debug_print_boxes(FILE * stream, const cairo_boxes_t * boxes);
//
//#include "cairo-box-inline.h"
//
static inline void _cairo_box_set(cairo_box_t * box, const cairo_point_t * p1, const cairo_point_t * p2)
{
	box->p1 = *p1;
	box->p2 = *p2;
}

static inline void _cairo_box_from_integers(cairo_box_t * box, int x, int y, int w, int h)
{
	box->p1.x = _cairo_fixed_from_int(x);
	box->p1.y = _cairo_fixed_from_int(y);
	box->p2.x = _cairo_fixed_from_int(x + w);
	box->p2.y = _cairo_fixed_from_int(y + h);
}
//
// assumes box->p1 is top-left, p2 bottom-right 
//
static inline void _cairo_box_add_point(cairo_box_t * box, const cairo_point_t * point)
{
	if(point->x < box->p1.x)
		box->p1.x = point->x;
	else if(point->x > box->p2.x)
		box->p2.x = point->x;
	if(point->y < box->p1.y)
		box->p1.y = point->y;
	else if(point->y > box->p2.y)
		box->p2.y = point->y;
}

static inline void _cairo_box_add_box(cairo_box_t * box, const cairo_box_t * add)
{
	SETMIN(box->p1.x, add->p1.x);
	SETMAX(box->p2.x, add->p2.x);
	SETMIN(box->p1.y, add->p1.y);
	SETMAX(box->p2.y, add->p2.y);
}
//
// assumes box->p1 is top-left, p2 bottom-right 
//
static inline cairo_bool_t _cairo_box_contains_point(const cairo_box_t * box, const cairo_point_t * point)
{
	return box->p1.x <= point->x  && point->x <= box->p2.x && box->p1.y <= point->y  && point->y <= box->p2.y;
}

static inline cairo_bool_t _cairo_box_is_pixel_aligned(const cairo_box_t * box)
{
#if CAIRO_FIXED_FRAC_BITS <= 8 && 0
	return ((box->p1.x & CAIRO_FIXED_FRAC_MASK) << 24 | (box->p1.y & CAIRO_FIXED_FRAC_MASK) << 16 |
	    (box->p2.x & CAIRO_FIXED_FRAC_MASK) << 8 | (box->p2.y & CAIRO_FIXED_FRAC_MASK) << 0) == 0;
#else /* GCC on i7 prefers this variant (bizarrely according to the profiler) */
	cairo_fixed_t f = 0;
	f |= box->p1.x & CAIRO_FIXED_FRAC_MASK;
	f |= box->p1.y & CAIRO_FIXED_FRAC_MASK;
	f |= box->p2.x & CAIRO_FIXED_FRAC_MASK;
	f |= box->p2.y & CAIRO_FIXED_FRAC_MASK;
	return f == 0;
#endif
}

CAIRO_BEGIN_DECLS
//
//#include "cairo-contour-private.h"
// 
// Descr: A contour is simply a closed chain of points that divide the infinite plane
//   into inside and outside. Each contour is a simple polygon, that is it
//   contains no holes or self-intersections, but maybe either concave or convex.
// 
struct _cairo_contour_chain {
	cairo_point_t * points;
	int num_points, size_points;
	struct _cairo_contour_chain * next;
};

struct _cairo_contour_iter {
	cairo_point_t * point;
	cairo_contour_chain_t * chain;
};

struct _cairo_contour {
	cairo_list_t next;
	int direction;
	cairo_contour_chain_t chain, * tail;
	cairo_point_t embedded_points[64];
};
//
// Initial definition of a shape is a set of contours (some representing holes) 
//
struct _cairo_shape {
	cairo_list_t contours;
};

typedef struct _cairo_shape cairo_shape_t;

#if 0
cairo_private cairo_status_t _cairo_shape_init_from_polygon(cairo_shape_t * shape, const cairo_polygon_t * polygon);
cairo_private cairo_status_t _cairo_shape_reduce(cairo_shape_t * shape, double tolerance);
#endif
cairo_private void _cairo_contour_init(cairo_contour_t * contour, int direction);
cairo_private cairo_int_status_t __cairo_contour_add_point(cairo_contour_t * contour, const cairo_point_t * point);
cairo_private void _cairo_contour_simplify(cairo_contour_t * contour, double tolerance);
cairo_private void _cairo_contour_reverse(cairo_contour_t * contour);
cairo_private cairo_int_status_t _cairo_contour_add(cairo_contour_t * dst, const cairo_contour_t * src);
cairo_private cairo_int_status_t _cairo_contour_add_reversed(cairo_contour_t * dst, const cairo_contour_t * src);
cairo_private void __cairo_contour_remove_last_chain(cairo_contour_t * contour);
cairo_private void _cairo_contour_reset(cairo_contour_t * contour);
cairo_private void _cairo_contour_fini(cairo_contour_t * contour);
cairo_private void _cairo_debug_print_contour(FILE * file, cairo_contour_t * contour);
//
//#include "cairo-contour-inline.h"
//
static inline cairo_int_status_t _cairo_contour_add_point(cairo_contour_t * contour, const cairo_point_t * point)
{
	struct _cairo_contour_chain * tail = contour->tail;
	if(unlikely(tail->num_points == tail->size_points))
		return __cairo_contour_add_point(contour, point);
	tail->points[tail->num_points++] = *point;
	return CAIRO_INT_STATUS_SUCCESS;
}
static inline cairo_point_t * _cairo_contour_first_point(cairo_contour_t * c)
{
	return &c->chain.points[0];
}
static inline cairo_point_t * _cairo_contour_last_point(cairo_contour_t * c)
{
	return &c->tail->points[c->tail->num_points-1];
}
static inline void _cairo_contour_remove_last_point(cairo_contour_t * contour)
{
	if(contour->chain.num_points != 0) {
		if(--contour->tail->num_points == 0)
			__cairo_contour_remove_last_chain(contour);
	}
}
//
//#include "cairo-stroke-dash-private.h"
//
typedef struct _cairo_stroker_dash {
	cairo_bool_t dashed;
	uint dash_index;
	cairo_bool_t dash_on;
	cairo_bool_t dash_starts_on;
	double dash_remain;
	double dash_offset;
	const double * dashes;
	uint num_dashes;
} cairo_stroker_dash_t;

cairo_private void _cairo_stroker_dash_init(cairo_stroker_dash_t * dash, const cairo_stroke_style_t * style);
cairo_private void _cairo_stroker_dash_start(cairo_stroker_dash_t * dash);
cairo_private void _cairo_stroker_dash_step(cairo_stroker_dash_t * dash, double step);

CAIRO_END_DECLS
//
//#include "cairo-path-fixed-private.h"
//
#define WATCH_PATH 0
#if WATCH_PATH
	//#include <stdio.h>
#endif

enum cairo_path_op {
	CAIRO_PATH_OP_MOVE_TO = 0,
	CAIRO_PATH_OP_LINE_TO = 1,
	CAIRO_PATH_OP_CURVE_TO = 2,
	CAIRO_PATH_OP_CLOSE_PATH = 3
};

// we want to make sure a single byte is used for the enum 
typedef char cairo_path_op_t;

// make _cairo_path_fixed fit into ~512 bytes -- about 50 items 
#define CAIRO_PATH_BUF_SIZE ((512 - sizeof(cairo_path_buf_t)) / (2 * sizeof(cairo_point_t) + sizeof(cairo_path_op_t)))

#define cairo_path_head(path__) (&(path__)->buf.base)
#define cairo_path_tail(path__) cairo_path_buf_prev(cairo_path_head(path__))
#define cairo_path_buf_next(pos__) cairo_list_entry((pos__)->link.next, cairo_path_buf_t, link)
#define cairo_path_buf_prev(pos__) cairo_list_entry((pos__)->link.prev, cairo_path_buf_t, link)
#define cairo_path_foreach_buf_start(pos__, path__) pos__ = cairo_path_head(path__); do
#define cairo_path_foreach_buf_end(pos__, path__) while((pos__ = cairo_path_buf_next(pos__)) !=  cairo_path_head(path__))

typedef struct _cairo_path_buf {
	cairo_list_t link;
	uint num_ops;
	uint size_ops;
	uint num_points;
	uint size_points;
	cairo_path_op_t * op;
	cairo_point_t * points;
} cairo_path_buf_t;

typedef struct _cairo_path_buf_fixed {
	cairo_path_buf_t base;
	cairo_path_op_t op[CAIRO_PATH_BUF_SIZE];
	cairo_point_t points[2 * CAIRO_PATH_BUF_SIZE];
} cairo_path_buf_fixed_t;
// 
// NOTES:
// has_curve_to => !stroke_is_rectilinear
// fill_is_rectilinear => stroke_is_rectilinear
// fill_is_empty => fill_is_rectilinear
// fill_maybe_region => fill_is_rectilinear
// 
struct _cairo_path_fixed {
	cairo_point_t last_move_point;
	cairo_point_t current_point;
	uint has_current_point      : 1;
	uint needs_move_to          : 1;
	uint has_extents            : 1;
	uint has_curve_to           : 1;
	uint stroke_is_rectilinear  : 1;
	uint fill_is_rectilinear    : 1;
	uint fill_maybe_region      : 1;
	uint fill_is_empty          : 1;
	cairo_box_t extents;
	cairo_path_buf_fixed_t buf;
};

cairo_private void _cairo_path_fixed_translate(cairo_path_fixed_t * path, cairo_fixed_t offx, cairo_fixed_t offy);
cairo_private cairo_status_t _cairo_path_fixed_append(cairo_path_fixed_t * path, const cairo_path_fixed_t * other, cairo_fixed_t tx, cairo_fixed_t ty);
cairo_private ulong _cairo_path_fixed_hash(const cairo_path_fixed_t * path);
cairo_private ulong _cairo_path_fixed_size(const cairo_path_fixed_t * path);
cairo_private cairo_bool_t _cairo_path_fixed_equal(const cairo_path_fixed_t * a, const cairo_path_fixed_t * b);

typedef struct _cairo_path_fixed_iter {
	const cairo_path_buf_t * first;
	const cairo_path_buf_t * buf;
	uint n_op;
	uint n_point;
} cairo_path_fixed_iter_t;

cairo_private void _cairo_path_fixed_iter_init(cairo_path_fixed_iter_t * iter, const cairo_path_fixed_t * path);
cairo_private cairo_bool_t FASTCALL _cairo_path_fixed_iter_is_fill_box(cairo_path_fixed_iter_t * _iter, cairo_box_t * box);
cairo_private cairo_bool_t FASTCALL _cairo_path_fixed_iter_at_end(const cairo_path_fixed_iter_t * iter);

/*static*/inline cairo_bool_t _cairo_path_fixed_fill_is_empty(const cairo_path_fixed_t * path)
{
	return path->fill_is_empty;
}

/*static*/inline cairo_bool_t _cairo_path_fixed_fill_is_rectilinear(const cairo_path_fixed_t * path)
{
	if(!path->fill_is_rectilinear)
		return 0;
	else if(!path->has_current_point || path->needs_move_to)
		return 1;
	else {
		// check whether the implicit close preserves the rectilinear property 
		return path->current_point.x == path->last_move_point.x || path->current_point.y == path->last_move_point.y;
	}
}

/*static*/inline cairo_bool_t _cairo_path_fixed_stroke_is_rectilinear(const cairo_path_fixed_t * path)
{
	return path->stroke_is_rectilinear;
}

/*static*/inline cairo_bool_t _cairo_path_fixed_fill_maybe_region(const cairo_path_fixed_t * path)
{
	if(!path->fill_maybe_region)
		return 0;
	else if(!path->has_current_point || path->needs_move_to)
		return 1;
	else {
		// check whether the implicit close preserves the rectilinear property
		// (the integer point property is automatically preserved)
		return path->current_point.x == path->last_move_point.x || path->current_point.y == path->last_move_point.y;
	}
}

cairo_private cairo_bool_t _cairo_path_fixed_is_stroke_box(const cairo_path_fixed_t * path, cairo_box_t * box);
cairo_private cairo_bool_t _cairo_path_fixed_is_simple_quad(const cairo_path_fixed_t * path);
//
//#include "cairo-clip-private.h"
//
extern const cairo_private cairo_rectangle_list_t _cairo_rectangles_nil;

struct _cairo_clip_path {
	cairo_reference_count_t ref_count;
	cairo_path_fixed_t path;
	CairoFillRule fill_rule;
	double tolerance;
	cairo_antialias_t antialias;
	cairo_clip_path_t * prev;
};

struct _cairo_clip {
	CairoIRect extents;
	cairo_clip_path_t * path;
	cairo_box_t * boxes;
	int num_boxes;
	cairo_region_t * region;
	cairo_bool_t is_region;
	cairo_box_t embedded_box;
};

cairo_private cairo_clip_t * _cairo_clip_create(void);
cairo_private cairo_clip_path_t * FASTCALL _cairo_clip_path_reference(cairo_clip_path_t * clip_path);
cairo_private void FASTCALL _cairo_clip_path_destroy(cairo_clip_path_t * clip_path);
cairo_private void FASTCALL _cairo_clip_destroy(cairo_clip_t * clip);
cairo_private extern const cairo_clip_t __cairo_clip_all;
cairo_private cairo_clip_t * FASTCALL _cairo_clip_copy(const cairo_clip_t * clip);
cairo_private cairo_clip_t * FASTCALL _cairo_clip_copy_region(const cairo_clip_t * clip);
cairo_private cairo_clip_t * FASTCALL _cairo_clip_copy_path(const cairo_clip_t * clip);
cairo_private cairo_clip_t * _cairo_clip_translate(cairo_clip_t * clip, int tx, int ty);
cairo_private cairo_clip_t * FASTCALL _cairo_clip_transform(cairo_clip_t * clip, const cairo_matrix_t * m);
cairo_private cairo_clip_t * FASTCALL _cairo_clip_copy_with_translation(const cairo_clip_t * clip, int tx, int ty);
cairo_private cairo_bool_t FASTCALL _cairo_clip_equal(const cairo_clip_t * clip_a, const cairo_clip_t * clip_b);
cairo_private cairo_clip_t * _cairo_clip_intersect_rectangle(cairo_clip_t * clip, const CairoIRect * rectangle);
cairo_private cairo_clip_t * FASTCALL _cairo_clip_intersect_clip(cairo_clip_t * clip, const cairo_clip_t * other);
cairo_private cairo_clip_t * _cairo_clip_intersect_box(cairo_clip_t * clip, const cairo_box_t * box);
cairo_private cairo_clip_t * _cairo_clip_intersect_boxes(cairo_clip_t * clip, const cairo_boxes_t * boxes);
cairo_private cairo_clip_t * _cairo_clip_intersect_rectilinear_path(cairo_clip_t * clip,
    const cairo_path_fixed_t * path, CairoFillRule fill_rule, cairo_antialias_t antialias);
cairo_private cairo_clip_t * _cairo_clip_intersect_path(cairo_clip_t * clip,
    const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, cairo_antialias_t antialias);
cairo_private const CairoIRect * FASTCALL _cairo_clip_get_extents(const cairo_clip_t * clip);
cairo_private cairo_surface_t * _cairo_clip_get_surface(const cairo_clip_t * clip, cairo_surface_t * dst, int * tx, int * ty);
cairo_private cairo_surface_t * _cairo_clip_get_image(const cairo_clip_t * clip, cairo_surface_t * target, const CairoIRect * extents);
cairo_private cairo_status_t _cairo_clip_combine_with_surface(const cairo_clip_t * clip, cairo_surface_t * dst, int dst_x, int dst_y);
cairo_private cairo_clip_t * _cairo_clip_from_boxes(const cairo_boxes_t * boxes);
cairo_private cairo_region_t * FASTCALL _cairo_clip_get_region(const cairo_clip_t * clip);
cairo_private cairo_bool_t FASTCALL _cairo_clip_is_region(const cairo_clip_t * clip);
cairo_private cairo_clip_t * _cairo_clip_reduce_to_rectangle(const cairo_clip_t * clip, const CairoIRect * r);
cairo_private cairo_clip_t * _cairo_clip_reduce_for_composite(const cairo_clip_t * clip, cairo_composite_rectangles_t * extents);
cairo_private cairo_bool_t FASTCALL _cairo_clip_contains_rectangle(const cairo_clip_t * clip, const CairoIRect * rect);
cairo_private cairo_bool_t FASTCALL _cairo_clip_contains_box(const cairo_clip_t * clip, const cairo_box_t * box);
cairo_private cairo_bool_t _cairo_clip_contains_extents(const cairo_clip_t * clip, const cairo_composite_rectangles_t * extents);
cairo_private cairo_rectangle_list_t * _cairo_clip_copy_rectangle_list(cairo_clip_t * clip, cairo_gstate_t * gstate);
cairo_private cairo_rectangle_list_t * _cairo_rectangle_list_create_in_error(cairo_status_t status);
cairo_private cairo_bool_t _cairo_clip_is_polygon(const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_clip_get_polygon(const cairo_clip_t * clip, cairo_polygon_t * polygon, CairoFillRule * fill_rule, cairo_antialias_t * antialias);
//
//#include "cairo-clip-inline.h"
//
/*static*/inline cairo_bool_t _cairo_clip_is_all_clipped(const cairo_clip_t * clip)
{
	return clip == &__cairo_clip_all;
}

/*static*/inline cairo_clip_t * _cairo_clip_set_all_clipped(cairo_clip_t * clip)
{
	_cairo_clip_destroy(clip);
	return (cairo_clip_t*)&__cairo_clip_all;
}

/*static*/inline cairo_clip_t * _cairo_clip_copy_intersect_rectangle(const cairo_clip_t * clip, const CairoIRect * r)
{
	return _cairo_clip_intersect_rectangle(_cairo_clip_copy(clip), r);
}

/*static*/inline cairo_clip_t * _cairo_clip_copy_intersect_clip(const cairo_clip_t * clip, const cairo_clip_t * other)
{
	return _cairo_clip_intersect_clip(_cairo_clip_copy(clip), other);
}

/*static*/inline void _cairo_clip_steal_boxes(cairo_clip_t * clip, cairo_boxes_t * boxes)
{
	_cairo_boxes_init_for_array(boxes, clip->boxes, clip->num_boxes);
	clip->boxes = NULL;
	clip->num_boxes = 0;
}

/*static*/inline void _cairo_clip_unsteal_boxes(cairo_clip_t * clip, cairo_boxes_t * boxes)
{
	clip->boxes = boxes->chunks.base;
	clip->num_boxes = boxes->num_boxes;
}
//
//#include "cairo-surface-clipper-private.h"
//
CAIRO_BEGIN_DECLS

//typedef struct _cairo_surface_clipper cairo_surface_clipper_t;
struct cairo_surface_clipper_t;

typedef cairo_status_t (*cairo_surface_clipper_intersect_clip_path_func_t)(cairo_surface_clipper_t *, cairo_path_fixed_t *, CairoFillRule, double, cairo_antialias_t);

struct /*_cairo_surface_clipper*/cairo_surface_clipper_t {
	cairo_clip_t * clip;
	cairo_surface_clipper_intersect_clip_path_func_t intersect_clip_path;
};

cairo_private cairo_status_t FASTCALL _cairo_surface_clipper_set_clip(cairo_surface_clipper_t * clipper, const cairo_clip_t * clip);
cairo_private void FASTCALL _cairo_surface_clipper_init(cairo_surface_clipper_t * clipper, cairo_surface_clipper_intersect_clip_path_func_t intersect);
cairo_private void FASTCALL _cairo_surface_clipper_reset(cairo_surface_clipper_t * clipper);
//
//#include "cairo-freed-pool-private.h"
//
#define DISABLE_FREED_POOLS 0

#if HAS_ATOMIC_OPS && !DISABLE_FREED_POOLS
	#define MAX_FREED_POOL_SIZE 16 // Keep a stash of recently freed clip_paths, since we need to reallocate them frequently.
	typedef struct {
		void * pool[MAX_FREED_POOL_SIZE];
		int top;
	} freed_pool_t;

	static cairo_always_inline void * _atomic_fetch(void ** slot)
	{
		void * ptr;
		do {
			ptr = _cairo_atomic_ptr_get(slot);
		} while(!_cairo_atomic_ptr_cmpxchg(slot, ptr, NULL));
		return ptr;
	}
	static cairo_always_inline cairo_bool_t _atomic_store(void ** slot, void * ptr)
	{
		return _cairo_atomic_ptr_cmpxchg(slot, NULL, ptr);
	}
	cairo_private void * _freed_pool_get_search(freed_pool_t * pool);
	static inline void * _freed_pool_get(freed_pool_t * pool)
	{
		void * ptr;
		int i = pool->top - 1;
		if(i < 0)
			i = 0;
		ptr = _atomic_fetch(&pool->pool[i]);
		if(likely(ptr != NULL)) {
			pool->top = i;
			return ptr;
		}
		// either empty or contended 
		return _freed_pool_get_search(pool);
	}
	cairo_private void _freed_pool_put_search(freed_pool_t * pool, void * ptr);
	static inline void _freed_pool_put(freed_pool_t * pool, void * ptr)
	{
		int i = pool->top;
		if(likely(i < SIZEOFARRAY(pool->pool) && _atomic_store(&pool->pool[i], ptr))) {
			pool->top = i + 1;
			return;
		}
		// either full or contended 
		_freed_pool_put_search(pool, ptr);
	}
	cairo_private void _freed_pool_reset(freed_pool_t * pool);

	#define HAS_FREED_POOL 1
#else
	// 
	// A warning about an unused freed-pool in a build without atomics
	// enabled usually indicates a missing _freed_pool_reset() in the static reset function 
	// 
	typedef int freed_pool_t;

	#define _freed_pool_get(pool) NULL
	#define _freed_pool_put(pool, ptr) SAlloc::F(ptr)
	#define _freed_pool_reset(ptr)
#endif

CAIRO_END_DECLS
//
//#include "cairo-gstate-private.h"
//
struct _cairo_gstate {
	cairo_operator_t op;
	double opacity;
	double tolerance;
	cairo_antialias_t antialias;
	cairo_stroke_style_t stroke_style;
	CairoFillRule fill_rule;
	cairo_font_face_t * font_face;
	cairo_scaled_font_t * scaled_font; /* Specific to the current CTM */
	cairo_scaled_font_t * previous_scaled_font; /* holdover */
	cairo_matrix_t font_matrix;
	cairo_font_options_t font_options;
	cairo_clip_t * clip;
	cairo_surface_t * target;       /* The target to which all rendering is directed */
	cairo_surface_t * parent_target; /* The previous target which was receiving rendering */
	cairo_surface_t * original_target; /* The original target the initial gstate was created with */
	/* the user is allowed to update the device after we have cached the matrices... */
	cairo_observer_t device_transform_observer;
	cairo_matrix_t ctm;
	cairo_matrix_t ctm_inverse;
	cairo_matrix_t source_ctm_inverse; /* At the time ->source was set */
	cairo_bool_t is_identity;
	cairo_pattern_t * source;
	struct _cairo_gstate * next;
};

/* cairo-gstate.c */
cairo_private cairo_status_t _cairo_gstate_init(cairo_gstate_t  * gstate, cairo_surface_t * target);
cairo_private void _cairo_gstate_fini(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_save(cairo_gstate_t ** gstate, cairo_gstate_t ** freelist);
cairo_private cairo_status_t _cairo_gstate_restore(cairo_gstate_t ** gstate, cairo_gstate_t ** freelist);
cairo_private cairo_bool_t _cairo_gstate_is_group(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_redirect_target(cairo_gstate_t * gstate, cairo_surface_t * child);
cairo_private cairo_surface_t * _cairo_gstate_get_target(cairo_gstate_t * gstate);
cairo_private cairo_surface_t * _cairo_gstate_get_original_target(cairo_gstate_t * gstate);
cairo_private cairo_clip_t * _cairo_gstate_get_clip(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_source(cairo_gstate_t * gstate, cairo_pattern_t * source);
cairo_private cairo_pattern_t * _cairo_gstate_get_source(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_operator(cairo_gstate_t * gstate, cairo_operator_t op);
cairo_private cairo_operator_t _cairo_gstate_get_operator(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_opacity(cairo_gstate_t * gstate, double opacity);
cairo_private double _cairo_gstate_get_opacity(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_tolerance(cairo_gstate_t * gstate, double tolerance);
cairo_private double _cairo_gstate_get_tolerance(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_fill_rule(cairo_gstate_t * gstate, CairoFillRule fill_rule);
cairo_private CairoFillRule _cairo_gstate_get_fill_rule(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_line_width(cairo_gstate_t * gstate, double width);
cairo_private double _cairo_gstate_get_line_width(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_line_cap(cairo_gstate_t * gstate, cairo_line_cap_t line_cap);
cairo_private cairo_line_cap_t _cairo_gstate_get_line_cap(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_line_join(cairo_gstate_t * gstate, cairo_line_join_t line_join);
cairo_private cairo_line_join_t _cairo_gstate_get_line_join(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_set_dash(cairo_gstate_t * gstate, const double * dash, int num_dashes, double offset);
cairo_private void _cairo_gstate_get_dash(cairo_gstate_t * gstate, double * dash, int * num_dashes, double * offset);
cairo_private cairo_status_t _cairo_gstate_set_miter_limit(cairo_gstate_t * gstate, double limit);
cairo_private double _cairo_gstate_get_miter_limit(cairo_gstate_t * gstate);
cairo_private void _cairo_gstate_get_matrix(cairo_gstate_t * gstate, cairo_matrix_t * matrix);
cairo_private cairo_status_t _cairo_gstate_translate(cairo_gstate_t * gstate, double tx, double ty);
cairo_private cairo_status_t _cairo_gstate_scale(cairo_gstate_t * gstate, double sx, double sy);
cairo_private cairo_status_t _cairo_gstate_rotate(cairo_gstate_t * gstate, double angle);
cairo_private cairo_status_t _cairo_gstate_transform(cairo_gstate_t * gstate, const cairo_matrix_t * matrix);
cairo_private cairo_status_t _cairo_gstate_set_matrix(cairo_gstate_t * gstate, const cairo_matrix_t * matrix);
cairo_private void _cairo_gstate_identity_matrix(cairo_gstate_t * gstate);
cairo_private void _cairo_gstate_user_to_device(cairo_gstate_t * gstate, double * x, double * y);
cairo_private void _cairo_gstate_user_to_device_distance(cairo_gstate_t * gstate, double * dx, double * dy);
cairo_private void _cairo_gstate_device_to_user(cairo_gstate_t * gstate, double * x, double * y);
cairo_private void _cairo_gstate_device_to_user_distance(cairo_gstate_t * gstate, double * dx, double * dy);
cairo_private void _do_cairo_gstate_user_to_backend(cairo_gstate_t * gstate, double * x, double * y);
cairo_private void _do_cairo_gstate_user_to_backend_distance(cairo_gstate_t * gstate, double * x, double * y);
cairo_private void _do_cairo_gstate_backend_to_user(cairo_gstate_t * gstate, double * x, double * y);
cairo_private void _do_cairo_gstate_backend_to_user_distance(cairo_gstate_t * gstate, double * x, double * y);

/*static*/inline void _cairo_gstate_user_to_backend(cairo_gstate_t * gstate, double * x, double * y)
{
	if(!gstate->is_identity)
		_do_cairo_gstate_user_to_backend(gstate, x, y);
}

/*static*/inline void _cairo_gstate_user_to_backend_distance(cairo_gstate_t * gstate, double * x, double * y)
{
	if(!gstate->is_identity)
		_do_cairo_gstate_user_to_backend_distance(gstate, x, y);
}

/*static*/inline void _cairo_gstate_backend_to_user(cairo_gstate_t * gstate, double * x, double * y)
{
	if(!gstate->is_identity)
		_do_cairo_gstate_backend_to_user(gstate, x, y);
}

/*static*/inline void _cairo_gstate_backend_to_user_distance(cairo_gstate_t * gstate, double * x, double * y)
{
	if(!gstate->is_identity)
		_do_cairo_gstate_backend_to_user_distance(gstate, x, y);
}

cairo_private void _cairo_gstate_backend_to_user_rectangle(cairo_gstate_t * gstate, double * x1, double * y1, double * x2, double * y2, cairo_bool_t * is_tight);
cairo_private void _cairo_gstate_path_extents(cairo_gstate_t * gstate, cairo_path_fixed_t * path, double * x1, double * y1, double * x2, double * y2);
cairo_private cairo_status_t _cairo_gstate_paint(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_mask(cairo_gstate_t  * gstate, cairo_pattern_t * mask);
cairo_private cairo_status_t _cairo_gstate_stroke(cairo_gstate_t * gstate, cairo_path_fixed_t * path);
cairo_private cairo_status_t _cairo_gstate_fill(cairo_gstate_t * gstate, cairo_path_fixed_t * path);
cairo_private cairo_status_t _cairo_gstate_copy_page(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_show_page(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_stroke_extents(cairo_gstate_t * gstate, cairo_path_fixed_t * path, double * x1, double * y1, double * x2, double * y2);
cairo_private cairo_status_t _cairo_gstate_fill_extents(cairo_gstate_t * gstate, cairo_path_fixed_t * path, double * x1, double * y1, double * x2, double * y2);
cairo_private cairo_status_t _cairo_gstate_in_stroke(cairo_gstate_t * gstate, cairo_path_fixed_t * path, double x, double y, cairo_bool_t       * inside_ret);
cairo_private cairo_bool_t _cairo_gstate_in_fill(cairo_gstate_t * gstate, cairo_path_fixed_t * path, double x, double y);
cairo_private cairo_bool_t _cairo_gstate_in_clip(cairo_gstate_t * gstate, double x, double y);
cairo_private cairo_status_t _cairo_gstate_clip(cairo_gstate_t * gstate, cairo_path_fixed_t * path);
cairo_private cairo_status_t _cairo_gstate_reset_clip(cairo_gstate_t * gstate);
cairo_private cairo_bool_t _cairo_gstate_clip_extents(cairo_gstate_t * gstate, double * x1, double * y1, double * x2, double * y2);
cairo_private cairo_rectangle_list_t* _cairo_gstate_copy_clip_rectangle_list(cairo_gstate_t * gstate);
cairo_private cairo_status_t _cairo_gstate_show_surface(cairo_gstate_t * gstate, cairo_surface_t * surface, double x, double y, double width, double height);
cairo_private cairo_status_t _cairo_gstate_set_font_size(cairo_gstate_t * gstate, double size);
cairo_private void _cairo_gstate_get_font_matrix(cairo_gstate_t * gstate, cairo_matrix_t * matrix);
cairo_private cairo_status_t FASTCALL _cairo_gstate_set_font_matrix(cairo_gstate_t * gstate, const cairo_matrix_t * matrix);
cairo_private void _cairo_gstate_get_font_options(cairo_gstate_t * gstate, cairo_font_options_t * options);
cairo_private void FASTCALL _cairo_gstate_set_font_options(cairo_gstate_t * gstate, const cairo_font_options_t * options);
cairo_private cairo_status_t _cairo_gstate_get_font_face(cairo_gstate_t * gstate, cairo_font_face_t ** font_face);
cairo_private cairo_status_t _cairo_gstate_get_scaled_font(cairo_gstate_t * gstate, cairo_scaled_font_t ** scaled_font);
cairo_private cairo_status_t _cairo_gstate_get_font_extents(cairo_gstate_t * gstate, cairo_font_extents_t * extents);
cairo_private cairo_status_t _cairo_gstate_set_font_face(cairo_gstate_t * gstate, cairo_font_face_t * font_face);
cairo_private cairo_status_t _cairo_gstate_glyph_extents(cairo_gstate_t * gstate, const cairo_glyph_t * glyphs, int num_glyphs, cairo_text_extents_t * extents);
cairo_private cairo_status_t _cairo_gstate_show_text_glyphs(cairo_gstate_t * gstate, const cairo_glyph_t * glyphs, int num_glyphs, cairo_glyph_text_info_t * info);
cairo_private cairo_status_t _cairo_gstate_glyph_path(cairo_gstate_t * gstate, const cairo_glyph_t * glyphs, int num_glyphs, cairo_path_fixed_t  * path);
cairo_private cairo_status_t _cairo_gstate_set_antialias(cairo_gstate_t * gstate, cairo_antialias_t antialias);
cairo_private cairo_antialias_t _cairo_gstate_get_antialias(cairo_gstate_t * gstate);
//
//#include "cairo-pattern-private.h"
//
CAIRO_BEGIN_DECLS

typedef struct _cairo_pattern_observer cairo_pattern_observer_t;

enum {
    CAIRO_PATTERN_NOTIFY_MATRIX = 0x1,
    CAIRO_PATTERN_NOTIFY_FILTER = 0x2,
    CAIRO_PATTERN_NOTIFY_EXTEND = 0x4,
    CAIRO_PATTERN_NOTIFY_OPACITY = 0x9,
};

struct _cairo_pattern_observer {
    void (*notify) (cairo_pattern_observer_t *, cairo_pattern_t *pattern, uint flags);
    cairo_list_t link;
};

struct /*_cairo_pattern*/cairo_pattern_t {
    cairo_reference_count_t ref_count;
    cairo_status_t status;
    cairo_user_data_array_t user_data;
    cairo_list_t observers;
    cairo_pattern_type_t type;
    cairo_filter_t filter;
    cairo_extend_t extend;
    cairo_bool_t   has_component_alpha;
    cairo_matrix_t matrix;
    double opacity;
};

struct _cairo_solid_pattern {
    cairo_pattern_t base;
    cairo_color_t color;
};

typedef struct _cairo_surface_pattern {
    cairo_pattern_t base;
    cairo_surface_t *surface;
} cairo_surface_pattern_t;

typedef struct _cairo_gradient_stop {
    double offset;
    cairo_color_stop_t color;
} cairo_gradient_stop_t;

typedef struct _cairo_gradient_pattern {
    cairo_pattern_t base;
    uint	    n_stops;
    uint	    stops_size;
    cairo_gradient_stop_t  *stops;
    cairo_gradient_stop_t   stops_embedded[2];
} cairo_gradient_pattern_t;

typedef struct _cairo_linear_pattern {
    cairo_gradient_pattern_t base;
    RPoint pd1;
    RPoint pd2;
} cairo_linear_pattern_t;

typedef struct _cairo_radial_pattern {
    cairo_gradient_pattern_t base;
    cairo_circle_double_t cd1;
    cairo_circle_double_t cd2;
} cairo_radial_pattern_t;

typedef union {
    cairo_gradient_pattern_t base;
    cairo_linear_pattern_t linear;
    cairo_radial_pattern_t radial;
} cairo_gradient_pattern_union_t;
/*
 * A mesh patch is a tensor-product patch (bicubic Bezier surface
 * patch). It has 16 control points. Each set of 4 points along the
 * sides of the 4x4 grid of control points is a Bezier curve that
 * defines one side of the patch. A color is assigned to each
 * corner. The inner 4 points provide additional control over the
 * shape and the color mapping.
 *
 * Cairo uses the same convention as the PDF Reference for numbering
 * the points and side of the patch.
 *
 *
 *                      Side 1
 *
 *          p[0][3] p[1][3] p[2][3] p[3][3]
 * Side 0   p[0][2] p[1][2] p[2][2] p[3][2]  Side 2
 *          p[0][1] p[1][1] p[2][1] p[3][1]
 *          p[0][0] p[1][0] p[2][0] p[3][0]
 *
 *                      Side 3
 *
 *
 *   Point            Color
 *  -------------------------
 *  points[0][0]    colors[0]
 *  points[0][3]    colors[1]
 *  points[3][3]    colors[2]
 *  points[3][0]    colors[3]
 */
typedef struct _cairo_mesh_patch {
    RPoint points[4][4];
    cairo_color_t colors[4];
} cairo_mesh_patch_t;

typedef struct _cairo_mesh_pattern {
    cairo_pattern_t base;
    cairo_array_t patches;
    cairo_mesh_patch_t *current_patch;
    int current_side;
    cairo_bool_t has_control_point[4];
    cairo_bool_t has_color[4];
} cairo_mesh_pattern_t;

typedef struct _cairo_raster_source_pattern {
    cairo_pattern_t base;
    cairo_content_t content;
    CairoIRect extents;
    cairo_raster_source_acquire_func_t acquire;
    cairo_raster_source_release_func_t release;
    cairo_raster_source_snapshot_func_t snapshot;
    cairo_raster_source_copy_func_t copy;
    cairo_raster_source_finish_func_t finish;
    void * user_data; // an explicit pre-allocated member in preference to the general user-data 
} cairo_raster_source_pattern_t;

typedef union {
    cairo_pattern_t		    base;
    cairo_solid_pattern_t	    solid;
    cairo_surface_pattern_t	    surface;
    cairo_gradient_pattern_union_t  gradient;
    cairo_mesh_pattern_t	    mesh;
    cairo_raster_source_pattern_t   raster_source;
} cairo_pattern_union_t;
//
// cairo-pattern.c 
//
cairo_private cairo_pattern_t * _cairo_pattern_create_in_error (cairo_status_t status);
cairo_private cairo_status_t _cairo_pattern_create_copy(cairo_pattern_t **pattern, const cairo_pattern_t  *other);
cairo_private void _cairo_pattern_init (cairo_pattern_t *pattern, cairo_pattern_type_t type);
cairo_private cairo_status_t _cairo_pattern_init_copy (cairo_pattern_t * pattern, const cairo_pattern_t *other);
cairo_private void _cairo_pattern_init_static_copy (cairo_pattern_t	*pattern, const cairo_pattern_t *other);
cairo_private cairo_status_t _cairo_pattern_init_snapshot(cairo_pattern_t * pattern, const cairo_pattern_t *other);
cairo_private void _cairo_pattern_init_solid (cairo_solid_pattern_t	*pattern, const cairo_color_t		*color);
cairo_private void _cairo_pattern_init_for_surface (cairo_surface_pattern_t *pattern, cairo_surface_t *surface);
cairo_private void _cairo_pattern_fini (cairo_pattern_t *pattern);
cairo_private cairo_pattern_t * _cairo_pattern_create_solid (const cairo_color_t	*color);
cairo_private void _cairo_pattern_transform(cairo_pattern_t *pattern, const cairo_matrix_t *ctm_inverse);
cairo_private void _cairo_pattern_pretransform(cairo_pattern_t * pattern, const cairo_matrix_t *ctm);
cairo_private cairo_bool_t FASTCALL _cairo_pattern_is_opaque_solid(const cairo_pattern_t *pattern);
cairo_private cairo_bool_t FASTCALL _cairo_pattern_is_opaque(const cairo_pattern_t *pattern, const CairoIRect *extents);
cairo_private cairo_bool_t FASTCALL _cairo_pattern_is_clear(const cairo_pattern_t * pattern);
cairo_private cairo_bool_t _cairo_gradient_pattern_is_solid (const cairo_gradient_pattern_t *gradient, const CairoIRect *extents, cairo_color_t *color);
cairo_private void _cairo_gradient_pattern_fit_to_range (const cairo_gradient_pattern_t *gradient,
	double max_value, cairo_matrix_t * out_matrix, cairo_circle_double_t out_circle[2]);
cairo_private cairo_bool_t _cairo_radial_pattern_focus_is_inside (const cairo_radial_pattern_t *radial);
cairo_private void _cairo_gradient_pattern_box_to_parameter (const cairo_gradient_pattern_t *gradient,
	double x0, double y0, double x1, double y1, double tolerance, double out_range[2]);
cairo_private void _cairo_gradient_pattern_interpolate (const cairo_gradient_pattern_t *gradient, double t, cairo_circle_double_t * out_circle);
cairo_private void _cairo_pattern_alpha_range (const cairo_pattern_t *pattern, double *out_min, double *out_max);
cairo_private cairo_bool_t _cairo_mesh_pattern_coord_box (const cairo_mesh_pattern_t *mesh, double *out_xmin, double *out_ymin, double *out_xmax, double *out_ymax);
cairo_private void _cairo_pattern_sampled_area (const cairo_pattern_t *pattern, const CairoIRect *extents, CairoIRect *sample);
cairo_private void _cairo_pattern_get_extents (const cairo_pattern_t	    *pattern, CairoIRect           *extents);
cairo_private cairo_int_status_t _cairo_pattern_get_ink_extents (const cairo_pattern_t	    *pattern, CairoIRect       *extents);
cairo_private ulong _cairo_pattern_hash (const cairo_pattern_t *pattern);
cairo_private ulong _cairo_linear_pattern_hash (ulong hash, const cairo_linear_pattern_t *linear);
cairo_private ulong _cairo_radial_pattern_hash (ulong hash, const cairo_radial_pattern_t *radial);
cairo_private cairo_bool_t _cairo_linear_pattern_equal (const cairo_linear_pattern_t *a, const cairo_linear_pattern_t *b);
cairo_private ulong _cairo_pattern_size (const cairo_pattern_t *pattern);
cairo_private cairo_bool_t _cairo_radial_pattern_equal (const cairo_radial_pattern_t *a, const cairo_radial_pattern_t *b);
cairo_private cairo_bool_t _cairo_pattern_equal (const cairo_pattern_t *a, const cairo_pattern_t *b);
cairo_private cairo_filter_t _cairo_pattern_analyze_filter (const cairo_pattern_t *pattern);
//
// cairo-mesh-pattern-rasterizer.c 
//
cairo_private void _cairo_mesh_pattern_rasterize (const cairo_mesh_pattern_t *mesh, void * data, int width, int height, int stride, double x_offset, double y_offset);
cairo_private cairo_surface_t * _cairo_raster_source_pattern_acquire (const cairo_pattern_t *abstract_pattern, cairo_surface_t *target, const CairoIRect *extents);
cairo_private void _cairo_raster_source_pattern_release (const cairo_pattern_t *abstract_pattern, cairo_surface_t *surface);
cairo_private cairo_status_t _cairo_raster_source_pattern_snapshot (cairo_pattern_t *abstract_pattern);
cairo_private cairo_status_t _cairo_raster_source_pattern_init_copy (cairo_pattern_t *pattern, const cairo_pattern_t *other);
cairo_private void _cairo_raster_source_pattern_finish (cairo_pattern_t *abstract_pattern);
cairo_private void _cairo_debug_print_pattern (FILE *file, const cairo_pattern_t *pattern);
//
//#include "cairo-pattern-inline.h"
//
/*static*/inline void _cairo_pattern_add_observer(cairo_pattern_t * pattern, cairo_pattern_observer_t * observer, 
	void (* func)(cairo_pattern_observer_t *, cairo_pattern_t *, uint))
{
	observer->notify = func;
	cairo_list_add(&observer->link, &pattern->observers);
}

/*static*/inline cairo_surface_t * _cairo_pattern_get_source(const cairo_surface_pattern_t * pattern, CairoIRect * extents)
{
	return _cairo_surface_get_source(pattern->surface, extents);
}
//
//#include "cairo-composite-rectangles-private.h"
// 
// Rectangles that take part in a composite operation.
// 
// The source and mask track the extents of the respective patterns in device
// space. The unbounded rectangle is essentially the clip rectangle. And the
// intersection of all is the bounded rectangle, which is the minimum extents
// the operation may require. Whether or not the operation is actually bounded
// is tracked in the is_bounded boolean.
// 
struct _cairo_composite_rectangles {
	cairo_surface_t * surface;
	cairo_operator_t op;
	CairoIRect source;
	CairoIRect mask;
	CairoIRect destination;
	CairoIRect bounded; /* source? IN mask? IN unbounded */
	CairoIRect unbounded; /* destination IN clip */
	uint32_t is_bounded;
	CairoIRect source_sample_area;
	CairoIRect mask_sample_area;
	cairo_pattern_union_t source_pattern;
	cairo_pattern_union_t mask_pattern;
	const cairo_pattern_t * original_source_pattern;
	const cairo_pattern_t * original_mask_pattern;
	cairo_clip_t * clip; /* clip will be reduced to the minimal container */
};

cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_paint(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_mask(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_stroke(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source,
    const cairo_path_fixed_t * path, const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_fill(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_path_fixed_t * path, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_boxes(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_boxes_t * boxes, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_polygon(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source, const cairo_polygon_t * polygon, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_init_for_glyphs(cairo_composite_rectangles_t * extents,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source, cairo_scaled_font_t * scaled_font,
    cairo_glyph_t * glyphs, int num_glyphs, const cairo_clip_t * clip, cairo_bool_t * overlap);
cairo_private cairo_int_status_t _cairo_composite_rectangles_intersect_source_extents(cairo_composite_rectangles_t * extents, const cairo_box_t * box);
cairo_private cairo_int_status_t _cairo_composite_rectangles_intersect_mask_extents(cairo_composite_rectangles_t * extents, const cairo_box_t * box);
cairo_private cairo_bool_t _cairo_composite_rectangles_can_reduce_clip(cairo_composite_rectangles_t * composite, cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_composite_rectangles_add_to_damage(cairo_composite_rectangles_t * composite, cairo_boxes_t * damage);
cairo_private void _cairo_composite_rectangles_fini(cairo_composite_rectangles_t * extents);
//
//#include "cairo-compositor-private.h"
//
typedef struct {
	cairo_scaled_font_t * font;
	cairo_glyph_t * glyphs;
	int num_glyphs;
	cairo_bool_t use_mask;
	CairoIRect extents;
} cairo_composite_glyphs_info_t;

struct cairo_compositor {
	const cairo_compositor_t * delegate;
	cairo_warn cairo_int_status_t (* paint)(const cairo_compositor_t * compositor, cairo_composite_rectangles_t * extents);
	cairo_warn cairo_int_status_t (* mask)(const cairo_compositor_t  * compositor, cairo_composite_rectangles_t * extents);
	cairo_warn cairo_int_status_t (* stroke)(const cairo_compositor_t * compositor, cairo_composite_rectangles_t * extents,
	    const cairo_path_fixed_t * path, const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse,
	    double tolerance, cairo_antialias_t antialias);
	cairo_warn cairo_int_status_t (* fill)(const cairo_compositor_t * compositor, cairo_composite_rectangles_t * extents, 
		const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, cairo_antialias_t antialias);
	cairo_warn cairo_int_status_t (* glyphs)(const cairo_compositor_t * compositor, cairo_composite_rectangles_t * extents,
	    cairo_scaled_font_t      * scaled_font, cairo_glyph_t * glyphs, int num_glyphs, cairo_bool_t overlap);
};

struct cairo_mask_compositor {
	cairo_compositor_t base;
	cairo_int_status_t (* acquire)(void * surface);
	cairo_int_status_t (* release)(void * surface);
	cairo_int_status_t (* set_clip_region)(void * surface, cairo_region_t * clip_region);
	cairo_surface_t * (*pattern_to_surface)(cairo_surface_t *dst, const cairo_pattern_t *pattern,
	    cairo_bool_t is_mask, const CairoIRect *extents, const CairoIRect *sample, int * src_x, int * src_y);
	cairo_int_status_t (* draw_image_boxes)(void * surface, cairo_image_surface_t * image, cairo_boxes_t * boxes, int dx, int dy);
	cairo_int_status_t (* copy_boxes)(void * surface, cairo_surface_t * src, cairo_boxes_t * boxes, const CairoIRect * extents, int dx, int dy);
	cairo_int_status_t (* fill_rectangles)(void * surface, cairo_operator_t op, const cairo_color_t * color, CairoIRect * rectangles, int num_rects);
	cairo_int_status_t (* fill_boxes)(void * surface, cairo_operator_t op, const cairo_color_t * color, cairo_boxes_t * boxes);
	cairo_int_status_t (* check_composite)(const cairo_composite_rectangles_t * extents);
	cairo_int_status_t (* composite)(void * dst, cairo_operator_t op, cairo_surface_t * src, cairo_surface_t * mask,
	    int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, uint width, uint height);
	cairo_int_status_t (* composite_boxes)(void * surface, cairo_operator_t op, cairo_surface_t * source, cairo_surface_t * mask,
	    int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, cairo_boxes_t * boxes, const CairoIRect * extents);
	cairo_int_status_t (* check_composite_glyphs)(const cairo_composite_rectangles_t * extents, cairo_scaled_font_t * scaled_font, cairo_glyph_t * glyphs, int * num_glyphs);
	cairo_int_status_t (* composite_glyphs)(void * surface, cairo_operator_t op, cairo_surface_t * src,
	    int src_x, int src_y, int dst_x, int dst_y, cairo_composite_glyphs_info_t * info);
};

struct cairo_traps_compositor {
	cairo_compositor_t base;
	cairo_int_status_t (* acquire)(void * surface);
	cairo_int_status_t (* release)(void * surface);
	cairo_int_status_t (* set_clip_region)(void * surface, cairo_region_t * clip_region);
	cairo_surface_t * (*pattern_to_surface)(cairo_surface_t *dst, const cairo_pattern_t *pattern,
	    cairo_bool_t is_mask, const CairoIRect *extents, const CairoIRect *sample, int * src_x, int * src_y);
	cairo_int_status_t (* draw_image_boxes)(void * surface, cairo_image_surface_t * image, cairo_boxes_t * boxes, int dx, int dy);
	cairo_int_status_t (* copy_boxes)(void * surface, cairo_surface_t * src, cairo_boxes_t * boxes, const CairoIRect * extents, int dx, int dy);
	cairo_int_status_t (* fill_boxes)(void * surface, cairo_operator_t op, const cairo_color_t * color, cairo_boxes_t * boxes);
	cairo_int_status_t (* check_composite)(const cairo_composite_rectangles_t * extents);
	cairo_int_status_t (* composite)(void * dst, cairo_operator_t op, cairo_surface_t * src, cairo_surface_t * mask,
	    int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, uint width, uint height);
	cairo_int_status_t (* lerp)(void * _dst, cairo_surface_t * abstract_src, cairo_surface_t * abstract_mask,
	    int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, uint width, uint height);
	cairo_int_status_t (* composite_boxes)(void * surface, cairo_operator_t op, cairo_surface_t * source, cairo_surface_t * mask,
	    int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, cairo_boxes_t * boxes, const CairoIRect * extents);
	cairo_int_status_t (* composite_traps)(void * dst, cairo_operator_t op, cairo_surface_t * source,
	    int src_x, int src_y, int dst_x, int dst_y, const CairoIRect * extents, cairo_antialias_t antialias, cairo_traps_t * traps);
	cairo_int_status_t (* composite_tristrip)(void * dst, cairo_operator_t op, cairo_surface_t * source,
	    int src_x, int src_y, int dst_x, int dst_y, const CairoIRect * extents, cairo_antialias_t antialias, cairo_tristrip_t * tristrip);
	cairo_int_status_t (* check_composite_glyphs)(const cairo_composite_rectangles_t * extents,
	    cairo_scaled_font_t * scaled_font, cairo_glyph_t * glyphs, int * num_glyphs);
	cairo_int_status_t (* composite_glyphs)(void * surface, cairo_operator_t op, cairo_surface_t * src,
	    int src_x, int src_y, int dst_x, int dst_y, cairo_composite_glyphs_info_t * info);
};

cairo_private extern const cairo_compositor_t __cairo_no_compositor;
cairo_private extern const cairo_compositor_t _cairo_fallback_compositor;

cairo_private void _cairo_mask_compositor_init(cairo_mask_compositor_t * compositor, const cairo_compositor_t * delegate);
cairo_private void _cairo_shape_mask_compositor_init(cairo_compositor_t * compositor, const cairo_compositor_t * delegate);
cairo_private void _cairo_traps_compositor_init(cairo_traps_compositor_t * compositor, const cairo_compositor_t * delegate);
cairo_private cairo_int_status_t _cairo_compositor_paint(const cairo_compositor_t * compositor, cairo_surface_t * surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_compositor_mask(const cairo_compositor_t * compositor, cairo_surface_t * surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_compositor_stroke(const cairo_compositor_t  * compositor, cairo_surface_t * surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_path_fixed_t * path, const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_compositor_fill(const cairo_compositor_t * compositor,
    cairo_surface_t * surface, cairo_operator_t op, const cairo_pattern_t * source,
    const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_compositor_glyphs(const cairo_compositor_t * compositor, cairo_surface_t * surface,
    cairo_operator_t op, const cairo_pattern_t * source, cairo_glyph_t * glyphs, int num_glyphs,
    cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);
//
//#include "cairo-traps-private.h"
//
struct _cairo_traps {
	cairo_status_t status;
	cairo_box_t bounds;
	const cairo_box_t * limits;
	int    num_limits;
	int    num_traps;
	int    traps_size;
	cairo_trapezoid_t * traps;
	cairo_trapezoid_t traps_embedded[16];
	uint   maybe_region      : 1; // hint: 0 implies that it cannot be 
	uint   has_intersections : 1;
	uint   is_rectilinear    : 1;
	uint   is_rectangular    : 1;
};
//
// cairo-traps.c 
//
cairo_private void FASTCALL _cairo_traps_init(cairo_traps_t * traps);
cairo_private void FASTCALL _cairo_traps_init_with_clip(cairo_traps_t * traps, const cairo_clip_t * clip);
cairo_private void _cairo_traps_limit(cairo_traps_t * traps, const cairo_box_t * boxes, int num_boxes);
cairo_private cairo_status_t FASTCALL _cairo_traps_init_boxes(cairo_traps_t * traps, const cairo_boxes_t * boxes);
cairo_private void FASTCALL _cairo_traps_clear(cairo_traps_t * traps);
cairo_private void FASTCALL _cairo_traps_fini(cairo_traps_t * traps);

#define _cairo_traps_status(T) (T)->status

cairo_private void _cairo_traps_translate(cairo_traps_t * traps, int x, int y);
cairo_private void _cairo_traps_tessellate_triangle_with_edges(cairo_traps_t * traps, const cairo_point_t t[3], const cairo_point_t edges[4]);
cairo_private void _cairo_traps_tessellate_convex_quad(cairo_traps_t * traps, const cairo_point_t q[4]);
cairo_private cairo_status_t _cairo_traps_tessellate_rectangle(cairo_traps_t * traps, const cairo_point_t * top_left, const cairo_point_t * bottom_right);
cairo_private void _cairo_traps_add_trap(cairo_traps_t * traps, cairo_fixed_t top, cairo_fixed_t bottom, const cairo_line_t * left, const cairo_line_t * right);
cairo_private int _cairo_traps_contain(const cairo_traps_t * traps, double x, double y);
cairo_private void _cairo_traps_extents(const cairo_traps_t * traps, cairo_box_t * extents);
cairo_private cairo_int_status_t _cairo_traps_extract_region(cairo_traps_t  * traps, cairo_antialias_t antialias, cairo_region_t ** region);
cairo_private cairo_bool_t _cairo_traps_to_boxes(cairo_traps_t * traps, cairo_antialias_t antialias, cairo_boxes_t * boxes);
cairo_private cairo_status_t _cairo_traps_path(const cairo_traps_t * traps, cairo_path_fixed_t  * path);
cairo_private cairo_int_status_t _cairo_rasterise_polygon_to_traps(cairo_polygon_t * polygon, CairoFillRule fill_rule, cairo_antialias_t antialias, cairo_traps_t * traps);
//
//#include "cairo-tristrip-private.h"
//
struct _cairo_tristrip {
	cairo_status_t status;
	/* XXX clipping */
	const cairo_box_t *limits;
	int num_limits;
	int num_points;
	int size_points;
	cairo_point_t *points;
	cairo_point_t  points_embedded[64];
};

cairo_private void _cairo_tristrip_init(cairo_tristrip_t *strip);
cairo_private void _cairo_tristrip_limit(cairo_tristrip_t * strip, const cairo_box_t * limits, int num_limits);
cairo_private void _cairo_tristrip_init_with_clip(cairo_tristrip_t *strip, const cairo_clip_t * clip);
cairo_private void _cairo_tristrip_translate(cairo_tristrip_t *strip, int x, int y);
cairo_private void _cairo_tristrip_move_to(cairo_tristrip_t *strip, const cairo_point_t *point);
cairo_private void FASTCALL _cairo_tristrip_add_point(cairo_tristrip_t *strip, const cairo_point_t *point);
cairo_private void _cairo_tristrip_extents(const cairo_tristrip_t *strip, cairo_box_t *extents);
cairo_private void _cairo_tristrip_fini(cairo_tristrip_t *strip);

#define _cairo_tristrip_status(T) ((T)->status)

CAIRO_END_DECLS
//
//#include "cairo-freelist-type-private.h"
//
//typedef struct _cairo_freelist_node cairo_freelist_node_t;
struct /*_cairo_freelist_node*/cairo_freelist_node_t {
    cairo_freelist_node_t * next;
};

/*typedef*/ struct /*_cairo_freelist*/cairo_freelist_t {
    cairo_freelist_node_t * first_free_node;
    uint   nodesize;
};

//typedef struct _cairo_freelist_pool cairo_freelist_pool_t;
struct /*_cairo_freelist_pool*/cairo_freelist_pool_t {
    cairo_freelist_pool_t *next;
    uint   size;
	uint   rem;
    uint8 * data;
};

/*typedef*/ struct /*_cairo_freepool*/cairo_freepool_t {
    cairo_freelist_node_t * first_free_node;
    cairo_freelist_pool_t * pools;
    cairo_freelist_pool_t * freepools;
    uint   nodesize;
    cairo_freelist_pool_t embedded_pool;
    uint8 embedded_data[1000];
};
//
//#include "cairo-freelist-private.h"
//
// for stand-alone compilation
#ifndef VG
	#define VG(x)
#endif
#ifndef NULL
	#define NULL (void*)0
#endif
//
// Initialise a freelist that will be responsible for allocating nodes of size nodesize.
//
cairo_private void _cairo_freelist_init(cairo_freelist_t * freelist, unsigned nodesize);
//
// Deallocate any nodes in the freelist.
//
cairo_private void _cairo_freelist_fini(cairo_freelist_t * freelist);
// 
// Allocate a new node from the freelist.  If the freelist contains no
// nodes, a new one will be allocated using SAlloc::M().  The caller is
// responsible for calling _cairo_freelist_free() or SAlloc::F() on the
// returned node.  Returns %NULL on memory allocation error.
// 
cairo_private void * _cairo_freelist_alloc(cairo_freelist_t * freelist);
// 
// Allocate a new node from the freelist.  If the freelist contains no
// nodes, a new one will be allocated using SAlloc::C().  The caller is
// responsible for calling _cairo_freelist_free() or SAlloc::F() on the
// returned node.  Returns %NULL on memory allocation error.
// 
cairo_private void * _cairo_freelist_calloc(cairo_freelist_t * freelist);
// 
// Return a node to the freelist. This does not deallocate the memory,
// but makes it available for later reuse by _cairo_freelist_alloc().
// 
cairo_private void _cairo_freelist_free(cairo_freelist_t * freelist, void * node);
cairo_private void _cairo_freepool_init(cairo_freepool_t * freepool, unsigned nodesize);
cairo_private void _cairo_freepool_fini(cairo_freepool_t * freepool);
void FASTCALL _cairo_freepool_reset(cairo_freepool_t * freepool);
cairo_private void * _cairo_freepool_alloc_from_new_pool(cairo_freepool_t * freepool);
void * FASTCALL _cairo_freepool_alloc_from_pool(cairo_freepool_t * freepool);
void * FASTCALL _cairo_freepool_alloc(cairo_freepool_t * freepool);
cairo_private cairo_status_t _cairo_freepool_alloc_array(cairo_freepool_t * freepool, int count, void ** array);
void FASTCALL _cairo_freepool_free(cairo_freepool_t * freepool, void * ptr);
//
//#include "cairo-rtree-private.h"
//
enum {
	CAIRO_RTREE_NODE_AVAILABLE,
	CAIRO_RTREE_NODE_DIVIDED,
	CAIRO_RTREE_NODE_OCCUPIED,
};

typedef struct _cairo_rtree_node {
	struct _cairo_rtree_node * children[4];
	struct _cairo_rtree_node * parent;
	cairo_list_t link;
	uint16 pinned;
	uint16 state;
	uint16 x;
	uint16 y;
	uint16 width;
	uint16 height;
} cairo_rtree_node_t;

typedef struct _cairo_rtree {
	cairo_rtree_node_t root;
	int min_size;
	cairo_list_t pinned;
	cairo_list_t available;
	cairo_list_t evictable;
	void (* destroy)(cairo_rtree_node_t *);
	cairo_freepool_t node_freepool;
} cairo_rtree_t;

cairo_private cairo_rtree_node_t * _cairo_rtree_node_create(cairo_rtree_t * rtree, cairo_rtree_node_t * parent, int x, int y, int width, int height);
cairo_private cairo_status_t _cairo_rtree_node_insert(cairo_rtree_t * rtree, cairo_rtree_node_t * node, int width, int height, cairo_rtree_node_t ** out);
cairo_private void _cairo_rtree_node_collapse(cairo_rtree_t * rtree, cairo_rtree_node_t * node);
cairo_private void _cairo_rtree_node_remove(cairo_rtree_t * rtree, cairo_rtree_node_t * node);
cairo_private void _cairo_rtree_node_destroy(cairo_rtree_t * rtree, cairo_rtree_node_t * node);
cairo_private void _cairo_rtree_init(cairo_rtree_t * rtree, int width, int height, int min_size, int node_size, void (* destroy)(cairo_rtree_node_t *));
cairo_private cairo_int_status_t _cairo_rtree_insert(cairo_rtree_t * rtree, int width, int height, cairo_rtree_node_t ** out);
cairo_private cairo_int_status_t _cairo_rtree_evict_random(cairo_rtree_t * rtree, int width, int height, cairo_rtree_node_t   ** out);
cairo_private void _cairo_rtree_foreach(cairo_rtree_t * rtree, void (* func)(cairo_rtree_node_t *, void * data), void * data);

static inline void * _cairo_rtree_pin(cairo_rtree_t * rtree, cairo_rtree_node_t * node)
{
	assert(node->state == CAIRO_RTREE_NODE_OCCUPIED);
	if(!node->pinned) {
		cairo_list_move(&node->link, &rtree->pinned);
		node->pinned = 1;
	}
	return node;
}

cairo_private void _cairo_rtree_unpin(cairo_rtree_t * rtree);
cairo_private void _cairo_rtree_reset(cairo_rtree_t * rtree);
cairo_private void _cairo_rtree_fini(cairo_rtree_t * rtree);
//
//#include "cairo-device-private.h"
//
struct _cairo_device {
    cairo_reference_count_t ref_count;
    cairo_status_t status;
    cairo_user_data_array_t user_data;
    const cairo_device_backend_t *backend;
    cairo_recursive_mutex_t mutex;
    unsigned mutex_depth;
    cairo_bool_t finished;
};

struct _cairo_device_backend {
    cairo_device_type_t type;
    void (*lock) (void *device);
    void (*unlock) (void *device);
    cairo_warn cairo_status_t (*flush) (void *device);
    void (*finish) (void *device);
    void (*destroy) (void *device);
};

cairo_private cairo_device_t * _cairo_device_create_in_error (cairo_status_t status);
cairo_private void _cairo_device_init (cairo_device_t *device, const cairo_device_backend_t *backend);
cairo_private cairo_status_t _cairo_device_set_error (cairo_device_t *device, cairo_status_t error);

slim_hidden_proto_no_warn (cairo_device_reference);
slim_hidden_proto (cairo_device_acquire);
slim_hidden_proto (cairo_device_release);
slim_hidden_proto (cairo_device_flush);
slim_hidden_proto (cairo_device_finish);
slim_hidden_proto (cairo_device_destroy);
//
//#include "cairo-time-private.h"
//
// Make the base type signed for easier arithmetic
typedef cairo_int64_t cairo_time_t;

#define _cairo_time_add _cairo_int64_add
#define _cairo_time_sub _cairo_int64_sub
#define _cairo_time_gt  _cairo_int64_gt
#define _cairo_time_lt  _cairo_int64_lt

#define _cairo_time_to_double   _cairo_int64_to_double
#define _cairo_time_from_double _cairo_double_to_int64

cairo_private int _cairo_time_cmp(const void * a, const void * b);
cairo_private double _cairo_time_to_s(cairo_time_t t);
cairo_private cairo_time_t _cairo_time_from_s(double t);
cairo_private cairo_time_t _cairo_time_get(void);

static cairo_always_inline cairo_time_t _cairo_time_get_delta(cairo_time_t t)
{
	cairo_time_t now = _cairo_time_get();
	return _cairo_time_sub(now, t);
}
static cairo_always_inline double _cairo_time_to_ns(cairo_time_t t)
{
	return 1.e9 * _cairo_time_to_s(t);
}
static cairo_always_inline cairo_time_t _cairo_time_max(cairo_time_t a, cairo_time_t b)
{
	return _cairo_int64_gt(a, b) ? a : b;
}
static cairo_always_inline cairo_time_t _cairo_time_min(cairo_time_t a, cairo_time_t b)
{
	return _cairo_int64_lt(a, b) ? a : b;
}
//
//#include "cairo-image-info-private.h"
//
struct cairo_image_info_t {
	int    width;
	int    height;
	int    num_components;
	int    bits_per_component;
};

cairo_private cairo_int_status_t _cairo_image_info_get_jpeg_info(cairo_image_info_t * info, const uchar * data, long length);
cairo_private cairo_int_status_t _cairo_image_info_get_jpx_info(cairo_image_info_t * info, const uchar * data, ulong length);
cairo_private cairo_int_status_t _cairo_image_info_get_png_info(cairo_image_info_t * info, const uchar * data, ulong length);
cairo_private cairo_int_status_t _cairo_image_info_get_jbig2_info(cairo_image_info_t * info, const uchar * data, ulong length);
//
//#include "cairo-combsort-inline.h"
//
// This fragment implements a comb sort (specifically combsort11) 
#ifndef _HAVE_CAIRO_COMBSORT_NEWGAP
#define _HAVE_CAIRO_COMBSORT_NEWGAP
static inline uint _cairo_combsort_newgap(uint gap)
{
	gap = 10 * gap / 13;
	if(gap == 9 || gap == 10)
		gap = 11;
	if(gap < 1)
		gap = 1;
	return gap;
}
#endif

#define CAIRO_COMBSORT_DECLARE(NAME, TYPE, CMP)	static void NAME(TYPE *base, uint nmemb) \
	{ \
		uint gap = nmemb; \
		uint i, j; \
		int swapped; \
		do { \
			gap = _cairo_combsort_newgap(gap); \
			swapped = gap > 1; \
			for(i = 0; i < nmemb-gap; i++) { \
				j = i + gap; \
				if(CMP(base[i], base[j]) > 0) {	\
					TYPE tmp; \
					tmp = base[i]; \
					base[i] = base[j]; \
					base[j] = tmp; \
					swapped = 1; \
				} \
			} \
		} while(swapped); \
	}

#define CAIRO_COMBSORT_DECLARE_WITH_DATA(NAME, TYPE, CMP) static void NAME(TYPE *base, uint nmemb, void * data) \
	{ \
		uint gap = nmemb; \
		uint i, j; \
		int swapped; \
		do { \
			gap = _cairo_combsort_newgap(gap); \
			swapped = gap > 1; \
			for(i = 0; i < nmemb-gap; i++) { \
				j = i + gap; \
				if(CMP(base[i], base[j], data) > 0) { \
					TYPE tmp; \
					tmp = base[i]; \
					base[i] = base[j]; \
					base[j] = tmp; \
					swapped = 1; \
				} \
			} \
		} while(swapped); \
	}
//
//#include "cairo-output-stream-private.h"
//
typedef cairo_status_t (*cairo_output_stream_write_func_t)(cairo_output_stream_t * output_stream, const uchar * data, uint length);
typedef cairo_status_t (*cairo_output_stream_flush_func_t)(cairo_output_stream_t * output_stream);
typedef cairo_status_t (*cairo_output_stream_close_func_t)(cairo_output_stream_t * output_stream);

struct _cairo_output_stream {
	cairo_output_stream_write_func_t write_func;
	cairo_output_stream_flush_func_t flush_func;
	cairo_output_stream_close_func_t close_func;
	ulong position;
	cairo_status_t status;
	cairo_bool_t closed;
};

extern const cairo_private cairo_output_stream_t _cairo_output_stream_nil;

cairo_private void _cairo_output_stream_init(cairo_output_stream_t * stream,
    cairo_output_stream_write_func_t write_func, cairo_output_stream_flush_func_t flush_func, cairo_output_stream_close_func_t close_func);
cairo_private cairo_status_t _cairo_output_stream_fini(cairo_output_stream_t * stream);

/* We already have the following declared in cairo.h:

   typedef cairo_status_t (*cairo_write_func_t) (void *closure, const uchar *data, uint length);
 */
typedef cairo_status_t (*cairo_close_func_t)(void * closure);

/* This function never returns %NULL. If an error occurs (NO_MEMORY)
 * while trying to create the output stream this function returns a
 * valid pointer to a nil output stream.
 *
 * Note that even with a nil surface, the close_func callback will be
 * called by a call to _cairo_output_stream_close or
 * _cairo_output_stream_destroy.
 */
cairo_private cairo_output_stream_t * _cairo_output_stream_create(cairo_write_func_t write_func, cairo_close_func_t close_func, void * closure);
cairo_private cairo_output_stream_t * _cairo_output_stream_create_in_error(cairo_status_t status);
/* Tries to flush any buffer maintained by the stream or its delegates. */
cairo_private cairo_status_t _cairo_output_stream_flush(cairo_output_stream_t * stream);
/* Returns the final status value associated with this object, just
 * before its last gasp. This final status value will capture any
 * status failure returned by the stream's close_func as well. */
cairo_private cairo_status_t _cairo_output_stream_close(cairo_output_stream_t * stream);
/* Returns the final status value associated with this object, just
 * before its last gasp. This final status value will capture any
 * status failure returned by the stream's close_func as well. */
cairo_private cairo_status_t FASTCALL _cairo_output_stream_destroy(cairo_output_stream_t * stream);
cairo_private void FASTCALL _cairo_output_stream_write(cairo_output_stream_t * stream, const void * data, size_t length);
cairo_private void _cairo_output_stream_write_hex_string(cairo_output_stream_t * stream, const uchar * data, size_t length);
cairo_private void _cairo_output_stream_vprintf(cairo_output_stream_t * stream, const char * fmt, va_list ap) CAIRO_PRINTF_FORMAT(2, 0);
cairo_private void _cairo_output_stream_printf(cairo_output_stream_t * stream, const char * fmt, ...) CAIRO_PRINTF_FORMAT(2, 3);
/* Print matrix element values with rounding of insignificant digits. */
cairo_private void FASTCALL _cairo_output_stream_print_matrix(cairo_output_stream_t * stream, const cairo_matrix_t  * matrix);
cairo_private long FASTCALL _cairo_output_stream_get_position(cairo_output_stream_t * stream);
cairo_private cairo_status_t FASTCALL _cairo_output_stream_get_status(cairo_output_stream_t * stream);

/* This function never returns %NULL. If an error occurs (NO_MEMORY or
 * WRITE_ERROR) while trying to create the output stream this function
 * returns a valid pointer to a nil output stream.
 *
 * Note: Even if a nil surface is returned, the caller should still
 * call _cairo_output_stream_destroy (or _cairo_output_stream_close at
 * least) in order to ensure that everything is properly cleaned up.
 */
cairo_private cairo_output_stream_t * FASTCALL _cairo_output_stream_create_for_filename(const char * filename);

/* This function never returns %NULL. If an error occurs (NO_MEMORY or
 * WRITE_ERROR) while trying to create the output stream this function
 * returns a valid pointer to a nil output stream.
 *
 * The caller still "owns" file and is responsible for calling fclose
 * on it when finished. The stream will not do this itself.
 */
cairo_private cairo_output_stream_t * _cairo_output_stream_create_for_file(FILE * file);
cairo_private cairo_output_stream_t * _cairo_memory_stream_create(void);
cairo_private void FASTCALL _cairo_memory_stream_copy(cairo_output_stream_t * base, cairo_output_stream_t * dest);
cairo_private int FASTCALL _cairo_memory_stream_length(cairo_output_stream_t * stream);
cairo_private cairo_status_t _cairo_memory_stream_destroy(cairo_output_stream_t * abstract_stream, uchar ** data_out, ulong * length_out);
cairo_private cairo_output_stream_t * _cairo_null_stream_create(void);
/* cairo-base85-stream.c */
cairo_private cairo_output_stream_t * _cairo_base85_stream_create(cairo_output_stream_t * output);
/* cairo-base64-stream.c */
cairo_private cairo_output_stream_t * _cairo_base64_stream_create(cairo_output_stream_t * output);
/* cairo-deflate-stream.c */
cairo_private cairo_output_stream_t * _cairo_deflate_stream_create(cairo_output_stream_t * output);
//
//#include "cairo-spans-compositor-private.h"
//
CAIRO_BEGIN_DECLS

typedef struct _cairo_abstract_span_renderer {
	cairo_span_renderer_t base;
	char data[4096];
} cairo_abstract_span_renderer_t;

struct cairo_spans_compositor {
	cairo_compositor_t base;
	uint flags;
#define CAIRO_SPANS_COMPOSITOR_HAS_LERP 0x1
	/* pixel-aligned fast paths */
	cairo_int_status_t (* fill_boxes)(void * surface, cairo_operator_t op, const cairo_color_t * color, cairo_boxes_t * boxes);
	cairo_int_status_t (* draw_image_boxes)(void * surface, cairo_image_surface_t * image, cairo_boxes_t * boxes, int dx, int dy);
	cairo_int_status_t (* copy_boxes)(void * surface, cairo_surface_t * src, cairo_boxes_t * boxes, const CairoIRect * extents, int dx, int dy);
	cairo_surface_t * (*pattern_to_surface)(cairo_surface_t *dst, const cairo_pattern_t *pattern, cairo_bool_t is_mask,
	    const CairoIRect *extents, const CairoIRect *sample, int * src_x, int * src_y);
	cairo_int_status_t (* composite_boxes)(void * surface, cairo_operator_t op, cairo_surface_t * source, cairo_surface_t * mask,
	    int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, cairo_boxes_t * boxes, const CairoIRect  * extents);
	/* general shape masks using a span renderer */
	cairo_int_status_t (* renderer_init)(cairo_abstract_span_renderer_t * renderer, const cairo_composite_rectangles_t * extents,
	    cairo_antialias_t antialias, cairo_bool_t needs_clip);
	void (* renderer_fini)(cairo_abstract_span_renderer_t * renderer, cairo_int_status_t status);
};

cairo_private void _cairo_spans_compositor_init(cairo_spans_compositor_t * compositor, const cairo_compositor_t  * delegate);

CAIRO_END_DECLS
//
//#include "cairo-paginated-private.h"
//
struct _cairo_paginated_surface_backend {
	/* Optional. Will be called once for each page.
	 *
	 * Note: With respect to the order of drawing operations as seen
	 * by the target, this call will occur before any drawing
	 * operations for the relevant page. However, with respect to the
	 * function calls as made by the user, this call will be *after*
	 * any drawing operations for the page, (that is, it will occur
	 * during the user's call to cairo_show_page or cairo_copy_page).
	 */
	cairo_warn cairo_int_status_t (* start_page)(void * surface);
	/* Required. Will be called twice for each page, once with an
	 * argument of CAIRO_PAGINATED_MODE_ANALYZE and once with
	 * CAIRO_PAGINATED_MODE_RENDER. See more details in the
	 * documentation for _cairo_paginated_surface_create below.
	 */
	void (* set_paginated_mode)(void * surface, cairo_paginated_mode_t mode);
	/* Optional. Specifies the smallest box that encloses all objects
	 * on the page. Will be called at the end of the ANALYZE phase but
	 * before the mode is changed to RENDER.
	 */
	cairo_warn cairo_int_status_t (* set_bounding_box)(void * surface, cairo_box_t * bbox);
	/* Optional. Indicates whether the page requires fallback images.
	 * Will be called at the end of the ANALYZE phase but before the
	 * mode is changed to RENDER.
	 */
	cairo_warn cairo_int_status_t (* set_fallback_images_required)(void * surface, cairo_bool_t fallbacks_required);
	cairo_bool_t (* supports_fine_grained_fallbacks)(void * surface);
};
//
/* A #cairo_paginated_surface_t provides a very convenient wrapper that
 * is well-suited for doing the analysis common to most surfaces that
 * have paginated output, (that is, things directed at printers, or
 * for saving content in files such as PostScript or PDF files).
 *
 * To use the paginated surface, you'll first need to create your
 * 'real' surface using _cairo_surface_init() and the standard
 * #cairo_surface_backend_t. Then you also call
 * _cairo_paginated_surface_create which takes its own, much simpler,
 * #cairo_paginated_surface_backend_t. You are free to return the result
 * of _cairo_paginated_surface_create() from your public
 * cairo_<foo>_surface_create(). The paginated backend will be careful
 * to not let the user see that they really got a "wrapped"
 * surface. See test-paginated-surface.c for a fairly minimal example
 * of a paginated-using surface. That should be a reasonable example
 * to follow.
 *
 * What the paginated surface does is first save all drawing
 * operations for a page into a recording-surface. Then when the user calls
 * cairo_show_page(), the paginated surface performs the following
 * sequence of operations (using the backend functions passed to
 * cairo_paginated_surface_create()):
 *
 * 1. Calls start_page() (if not %NULL). At this point, it is appropriate
 *    for the target to emit any page-specific header information into
 *    its output.
 *
 * 2. Calls set_paginated_mode() with an argument of %CAIRO_PAGINATED_MODE_ANALYZE
 *
 * 3. Replays the recording-surface to the target surface, (with an
 *    analysis surface inserted between which watches the return value
 *    from each operation). This analysis stage is used to decide which
 *    operations will require fallbacks.
 *
 * 4. Calls set_bounding_box() to provide the target surface with the
 *    tight bounding box of the page.
 *
 * 5. Calls set_paginated_mode() with an argument of %CAIRO_PAGINATED_MODE_RENDER
 *
 * 6. Replays a subset of the recording-surface operations to the target surface
 *
 * 7. Calls set_paginated_mode() with an argument of %CAIRO_PAGINATED_MODE_FALLBACK
 *
 * 8. Replays the remaining operations to an image surface, sets an
 *    appropriate clip on the target, then paints the resulting image
 *    surface to the target.
 *
 * So, the target will see drawing operations during three separate
 * stages, (ANALYZE, RENDER and FALLBACK). During the ANALYZE phase
 * the target should not actually perform any rendering, (for example,
 * if performing output to a file, no output should be generated
 * during this stage). Instead the drawing functions simply need to
 * return %CAIRO_STATUS_SUCCESS or %CAIRO_INT_STATUS_UNSUPPORTED to
 * indicate whether rendering would be supported. And it should do
 * this as quickly as possible. The FALLBACK phase allows the surface
 * to distinguish fallback images from native rendering in case they
 * need to be handled as a special case.
 *
 * Note: The paginated surface layer assumes that the target surface
 * is "blank" by default at the beginning of each page, without any
 * need for an explicit erase operation, (as opposed to an image
 * surface, for example, which might have uninitialized content
 * originally). As such, it optimizes away CLEAR operations that
 * happen at the beginning of each page---the target surface will not
 * even see these operations.
 */
cairo_private cairo_surface_t * _cairo_paginated_surface_create(cairo_surface_t * target, cairo_content_t content, const cairo_paginated_surface_backend_t * backend);
cairo_private cairo_surface_t * _cairo_paginated_surface_get_target(cairo_surface_t * surface);
cairo_private cairo_surface_t * _cairo_paginated_surface_get_recording(cairo_surface_t * surface);
cairo_private cairo_bool_t _cairo_surface_is_paginated(cairo_surface_t * surface);
cairo_private cairo_status_t _cairo_paginated_surface_set_size(cairo_surface_t * surface, int width, int height);
//
//#include "cairo-paginated-surface-private.h"
//
struct cairo_paginated_surface_t {
	cairo_surface_t base;
	cairo_surface_t * target; // The target surface to hold the final result
	cairo_content_t content;
	const cairo_paginated_surface_backend_t * backend; // Paginated-surface specific functions for the target 
	// A cairo_recording_surface to record all operations. To be replayed
	// against target, and also against image surface as necessary for fallbacks. 
	// 
	cairo_surface_t * recording_surface;
	int page_num;
};
//
//#include "cairo-recording-surface-private.h"
//
typedef enum {
	// The 5 basic drawing operations
	CAIRO_COMMAND_PAINT,
	CAIRO_COMMAND_MASK,
	CAIRO_COMMAND_STROKE,
	CAIRO_COMMAND_FILL,
	CAIRO_COMMAND_SHOW_TEXT_GLYPHS,
} cairo_command_type_t;

typedef enum {
	CAIRO_RECORDING_REGION_ALL,
	CAIRO_RECORDING_REGION_NATIVE,
	CAIRO_RECORDING_REGION_IMAGE_FALLBACK
} cairo_recording_region_type_t;

typedef struct _cairo_command_header {
	cairo_command_type_t type;
	cairo_recording_region_type_t region;
	cairo_operator_t op;
	CairoIRect extents;
	cairo_clip_t * clip;
	int index;
	struct _cairo_command_header * chain;
} cairo_command_header_t;

typedef struct _cairo_command_paint {
	cairo_command_header_t header;
	cairo_pattern_union_t source;
} cairo_command_paint_t;

typedef struct _cairo_command_mask {
	cairo_command_header_t header;
	cairo_pattern_union_t source;
	cairo_pattern_union_t mask;
} cairo_command_mask_t;

typedef struct _cairo_command_stroke {
	cairo_command_header_t header;
	cairo_pattern_union_t source;
	cairo_path_fixed_t path;
	cairo_stroke_style_t style;
	cairo_matrix_t ctm;
	cairo_matrix_t ctm_inverse;
	double tolerance;
	cairo_antialias_t antialias;
} cairo_command_stroke_t;

typedef struct _cairo_command_fill {
	cairo_command_header_t header;
	cairo_pattern_union_t source;
	cairo_path_fixed_t path;
	CairoFillRule fill_rule;
	double tolerance;
	cairo_antialias_t antialias;
} cairo_command_fill_t;

typedef struct _cairo_command_show_text_glyphs {
	cairo_command_header_t header;
	cairo_pattern_union_t source;
	char * utf8;
	int utf8_len;
	cairo_glyph_t * glyphs;
	uint num_glyphs;
	cairo_text_cluster_t * clusters;
	int num_clusters;
	cairo_text_cluster_flags_t cluster_flags;
	cairo_scaled_font_t * scaled_font;
} cairo_command_show_text_glyphs_t;

typedef union _cairo_command {
	cairo_command_header_t header;
	cairo_command_paint_t paint;
	cairo_command_mask_t mask;
	cairo_command_stroke_t stroke;
	cairo_command_fill_t fill;
	cairo_command_show_text_glyphs_t show_text_glyphs;
} cairo_command_t;

typedef struct _cairo_recording_surface {
	cairo_surface_t base;
	/* A recording-surface is logically unbounded, but when used as a
	 * source we need to render it to an image, so we need a size at
	 * which to create that image. */
	cairo_rectangle_t extents_pixels;
	CairoIRect extents;
	cairo_bool_t unbounded;
	cairo_array_t commands;
	uint * indices;
	uint num_indices;
	cairo_bool_t optimize_clears;
	cairo_bool_t has_bilevel_alpha;
	cairo_bool_t has_only_op_over;

	struct BbTree {
		cairo_box_t extents;
		BbTree * left;
		BbTree * right;
		cairo_command_header_t * chain;
	};
	BbTree bbtree;
} cairo_recording_surface_t;

slim_hidden_proto(cairo_recording_surface_create);

cairo_private cairo_int_status_t _cairo_recording_surface_get_path(cairo_surface_t * surface, cairo_path_fixed_t * path);
cairo_private cairo_status_t _cairo_recording_surface_replay_one(cairo_recording_surface_t  * surface, long unsigned index, cairo_surface_t * target);
cairo_private cairo_status_t _cairo_recording_surface_replay(cairo_surface_t * surface, cairo_surface_t * target);
cairo_private cairo_status_t _cairo_recording_surface_replay_with_clip(cairo_surface_t * surface, const cairo_matrix_t * surface_transform, cairo_surface_t * target, const cairo_clip_t * target_clip);
cairo_private cairo_status_t _cairo_recording_surface_replay_and_create_regions(cairo_surface_t * surface, cairo_surface_t * target);
cairo_private cairo_status_t _cairo_recording_surface_replay_region(cairo_surface_t * surface, const CairoIRect * surface_extents, cairo_surface_t * target, cairo_recording_region_type_t region);
cairo_private cairo_status_t _cairo_recording_surface_get_bbox(cairo_recording_surface_t * recording, cairo_box_t * bbox, const cairo_matrix_t * transform);
cairo_private cairo_status_t _cairo_recording_surface_get_ink_bbox(cairo_recording_surface_t * surface, cairo_box_t * bbox, const cairo_matrix_t * transform);
cairo_private cairo_bool_t _cairo_recording_surface_has_only_bilevel_alpha(cairo_recording_surface_t * surface);
cairo_private cairo_bool_t _cairo_recording_surface_has_only_op_over(cairo_recording_surface_t * surface);
//
//#include "cairo-recording-surface-inline.h"
//
static inline cairo_bool_t _cairo_recording_surface_get_bounds(cairo_surface_t * surface, cairo_rectangle_t * extents)
{
	cairo_recording_surface_t * recording = (cairo_recording_surface_t*)surface;
	if(recording->unbounded)
		return FALSE;
	else {
		*extents = recording->extents_pixels;
		return TRUE;
	}
}
/**
 * _cairo_surface_is_recording:
 * @surface: a #cairo_surface_t
 *
 * Checks if a surface is a #cairo_recording_surface_t
 *
 * Return value: %TRUE if the surface is a recording surface
 **/
/*static*/inline cairo_bool_t _cairo_surface_is_recording(const cairo_surface_t * surface)
{
	return surface->backend->type == CAIRO_SURFACE_TYPE_RECORDING;
}
//
//#include "cairo-surface-observer-private.h"
//
struct SurfaceObserverStat { // @sobolev stat-->SurfaceObserverStat
	double min;
	double max;
	double sum;
	double sum_sq;
	uint   count;
};

#define NUM_OPERATORS (CAIRO_OPERATOR_HSL_LUMINOSITY+1)
#define NUM_CAPS (CAIRO_LINE_CAP_SQUARE+1)
#define NUM_JOINS (CAIRO_LINE_JOIN_BEVEL+1)
#define NUM_ANTIALIAS (CAIRO_ANTIALIAS_BEST+1)
#define NUM_FILL_RULE (CAIRO_FILL_RULE_EVEN_ODD+1)

struct extents {
	struct SurfaceObserverStat area;
	uint   bounded;
	uint   unbounded;
};

struct pattern {
	uint   type[8]; // native/record/other surface/gradients // @v1.14.6 [7]-->[8]
};

struct path {
	uint type[5]; /* empty/pixel/rectilinear/straight/curved */
};

struct clip {
	uint type[6]; /* none, region, boxes, single path, polygon, general */
};

typedef struct _cairo_observation cairo_observation_t;
typedef struct _cairo_observation_record cairo_observation_record_t;
typedef struct _cairo_device_observer cairo_device_observer_t;

struct _cairo_observation_record {
	cairo_content_t target_content;
	int target_width;
	int target_height;
	int index;
	cairo_operator_t op;
	int source;
	int mask;
	int num_glyphs;
	int path;
	int fill_rule;
	double tolerance;
	int antialias;
	int clip;
	cairo_time_t elapsed;
};

struct _cairo_observation {
	int num_surfaces;
	int num_contexts;
	int num_sources_acquired;
	/* XXX put interesting stats here! */
	struct Paint {
		cairo_time_t elapsed;
		uint   count;
		struct extents extents;
		uint   operators[NUM_OPERATORS];
		struct pattern source;
		struct clip clip;
		uint   noop;
		cairo_observation_record_t slowest;
	};
	struct Mask {
		cairo_time_t elapsed;
		uint count;
		struct extents extents;
		uint operators[NUM_OPERATORS];
		struct pattern source;
		struct pattern mask;
		struct clip clip;
		uint   noop;
		cairo_observation_record_t slowest;
	};
	struct Fill {
		cairo_time_t elapsed;
		uint   count;
		struct extents extents;
		uint   operators[NUM_OPERATORS];
		struct pattern source;
		struct path path;
		uint   antialias[NUM_ANTIALIAS];
		uint   fill_rule[NUM_FILL_RULE];
		struct clip clip;
		uint   noop;
		cairo_observation_record_t slowest;
	};
	struct Stroke {
		cairo_time_t elapsed;
		uint count;
		struct extents extents;
		uint operators[NUM_OPERATORS];
		uint caps[NUM_CAPS];
		uint joins[NUM_CAPS];
		uint antialias[NUM_ANTIALIAS];
		struct pattern source;
		struct path path;
		struct SurfaceObserverStat line_width;
		struct clip clip;
		uint noop;
		cairo_observation_record_t slowest;
	};
	struct Glyphs {
		cairo_time_t elapsed;
		uint   count;
		struct extents extents;
		uint   operators[NUM_OPERATORS];
		struct pattern source;
		struct clip clip;
		uint   noop;
		cairo_observation_record_t slowest;
	};
	Paint  paint;
	Mask   mask;
	Fill   fill;
	Stroke stroke;
	Glyphs glyphs;
	cairo_array_t timings;
	cairo_recording_surface_t * record;
};

struct _cairo_device_observer {
	cairo_device_t base;
	cairo_device_t * target;
	cairo_observation_t log;
};

struct callback_list {
	cairo_list_t link;
	cairo_surface_observer_callback_t func;
	void * data;
};

struct _cairo_surface_observer {
	cairo_surface_t base;
	cairo_surface_t * target;
	cairo_observation_t log;
	cairo_list_t paint_callbacks;
	cairo_list_t mask_callbacks;
	cairo_list_t fill_callbacks;
	cairo_list_t stroke_callbacks;
	cairo_list_t glyphs_callbacks;
	cairo_list_t flush_callbacks;
	cairo_list_t finish_callbacks;
};
//
//#include "cairo-surface-observer-inline.h"
//
/*static*/inline cairo_surface_t * _cairo_surface_observer_get_target(cairo_surface_t * surface)
{
	return ((cairo_surface_observer_t*)surface)->target;
}
/*static*/inline cairo_bool_t _cairo_surface_is_observer(cairo_surface_t * surface)
{
	return surface->backend->type == (cairo_surface_type_t)CAIRO_INTERNAL_SURFACE_TYPE_OBSERVER;
}
/*static*/inline cairo_bool_t _cairo_device_is_observer(cairo_device_t * device)
{
	return device->backend->type == (cairo_device_type_t)CAIRO_INTERNAL_DEVICE_TYPE_OBSERVER;
}
//
//#include "cairo-surface-snapshot-private.h"
//
struct _cairo_surface_snapshot {
	cairo_surface_t base;
	cairo_mutex_t mutex;
	cairo_surface_t * target;
	cairo_surface_t * clone;
};
//
//#include "cairo-surface-snapshot-inline.h"
//
/*static*/inline cairo_bool_t _cairo_surface_snapshot_is_reused(cairo_surface_t * surface)
{
	return CAIRO_REFERENCE_COUNT_GET_VALUE(&surface->ref_count) > 2;
}

/*static*/inline cairo_surface_t * _cairo_surface_snapshot_get_target(cairo_surface_t * surface)
{
	cairo_surface_snapshot_t * snapshot = (cairo_surface_snapshot_t*)surface;
	cairo_surface_t * target;
	CAIRO_MUTEX_LOCK(snapshot->mutex);
	target = _cairo_surface_reference(snapshot->target);
	CAIRO_MUTEX_UNLOCK(snapshot->mutex);
	return target;
}

/*static*/inline cairo_bool_t _cairo_surface_is_snapshot(cairo_surface_t * surface)
{
	return surface->backend->type == (cairo_surface_type_t)CAIRO_INTERNAL_SURFACE_TYPE_SNAPSHOT;
}
//
//#include "cairo-surface-subsurface-private.h"
//
struct _cairo_surface_subsurface {
    cairo_surface_t base;
    CairoIRect extents;
    cairo_surface_t * target;
    cairo_surface_t * snapshot;
};

cairo_private void _cairo_surface_subsurface_set_snapshot(cairo_surface_t * surface, cairo_surface_t * snapshot);
//
//#include "cairo-surface-subsurface-inline.h"
//
/*static*/inline cairo_surface_t * _cairo_surface_subsurface_get_target(cairo_surface_t * surface)
{
	return ((cairo_surface_subsurface_t*)surface)->target;
}
/*static*/inline void _cairo_surface_subsurface_offset(cairo_surface_t * surface, int * x, int * y)
{
	const cairo_surface_subsurface_t * ss = (const cairo_surface_subsurface_t*)surface;
	*x += ss->extents.x;
	*y += ss->extents.y;
}
/*static*/inline cairo_surface_t * _cairo_surface_subsurface_get_target_with_offset(cairo_surface_t * surface, int * x, int * y)
{
	const cairo_surface_subsurface_t * ss = (const cairo_surface_subsurface_t*)surface;
	*x += ss->extents.x;
	*y += ss->extents.y;
	return ss->target;
}
/*static*/inline cairo_bool_t _cairo_surface_is_subsurface(cairo_surface_t * surface)
{
	return surface->backend->type == CAIRO_SURFACE_TYPE_SUBSURFACE;
}
//
//#include "cairo-surface-wrapper-private.h"
//
CAIRO_BEGIN_DECLS

struct _cairo_surface_wrapper {
	cairo_surface_t * target;
	cairo_matrix_t transform;
	cairo_bool_t has_extents;
	CairoIRect extents;
	const cairo_clip_t * clip;
	cairo_bool_t needs_transform;
};

cairo_private void _cairo_surface_wrapper_init(cairo_surface_wrapper_t * wrapper, cairo_surface_t * target);
cairo_private void _cairo_surface_wrapper_intersect_extents(cairo_surface_wrapper_t * wrapper, const CairoIRect * extents);
cairo_private void _cairo_surface_wrapper_set_inverse_transform(cairo_surface_wrapper_t * wrapper, const cairo_matrix_t * transform);
cairo_private void _cairo_surface_wrapper_set_clip(cairo_surface_wrapper_t * wrapper, const cairo_clip_t * clip);
cairo_private void _cairo_surface_wrapper_fini(cairo_surface_wrapper_t * wrapper);

/*static*/inline cairo_bool_t _cairo_surface_wrapper_has_fill_stroke(cairo_surface_wrapper_t * wrapper)
{
	return (wrapper->target->backend->fill_stroke != NULL);
}

cairo_private cairo_status_t _cairo_surface_wrapper_acquire_source_image(cairo_surface_wrapper_t * wrapper, cairo_image_surface_t  ** image_out, void ** image_extra);
cairo_private void _cairo_surface_wrapper_release_source_image(cairo_surface_wrapper_t * wrapper, cairo_image_surface_t  * image, void * image_extra);
cairo_private cairo_status_t _cairo_surface_wrapper_paint(cairo_surface_wrapper_t * wrapper, cairo_operator_t op, const cairo_pattern_t * source, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_wrapper_mask(cairo_surface_wrapper_t * wrapper, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_wrapper_stroke(cairo_surface_wrapper_t * wrapper, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_path_fixed_t * path, const cairo_stroke_style_t * stroke_style,
    const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_wrapper_fill_stroke(cairo_surface_wrapper_t * wrapper, cairo_operator_t fill_op,
    const cairo_pattern_t * fill_source, CairoFillRule fill_rule, double fill_tolerance, cairo_antialias_t fill_antialias,
    const cairo_path_fixed_t * path, cairo_operator_t stroke_op, const cairo_pattern_t * stroke_source,
    const cairo_stroke_style_t * stroke_style, const cairo_matrix_t * stroke_ctm, const cairo_matrix_t * stroke_ctm_inverse,
    double stroke_tolerance, cairo_antialias_t stroke_antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_wrapper_fill(cairo_surface_wrapper_t * wrapper, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_path_fixed_t * path, CairoFillRule fill_rule, double tolerance,
    cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_status_t _cairo_surface_wrapper_show_text_glyphs(cairo_surface_wrapper_t * wrapper, cairo_operator_t op, 
	const cairo_pattern_t * source, const char * utf8, int utf8_len, const cairo_glyph_t * glyphs, 
	int num_glyphs, const cairo_text_cluster_t * clusters,
    int num_clusters, cairo_text_cluster_flags_t cluster_flags, cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);
cairo_private cairo_surface_t * _cairo_surface_wrapper_create_similar(cairo_surface_wrapper_t * wrapper, cairo_content_t content, int width, int height);
cairo_private cairo_bool_t _cairo_surface_wrapper_get_extents(cairo_surface_wrapper_t * wrapper, CairoIRect   * extents);
cairo_private void _cairo_surface_wrapper_get_font_options(cairo_surface_wrapper_t * wrapper, cairo_font_options_t * options);
cairo_private cairo_surface_t * _cairo_surface_wrapper_snapshot(cairo_surface_wrapper_t * wrapper);
cairo_private cairo_bool_t _cairo_surface_wrapper_has_show_text_glyphs(cairo_surface_wrapper_t * wrapper);

static inline cairo_bool_t _cairo_surface_wrapper_is_active(cairo_surface_wrapper_t * wrapper)
{
	return wrapper->target != (cairo_surface_t*)0;
}

cairo_private cairo_bool_t _cairo_surface_wrapper_get_target_extents(cairo_surface_wrapper_t * wrapper, CairoIRect * extents);
//
//#include "cairo-surface-fallback-private.h"
//
cairo_private cairo_int_status_t _cairo_surface_fallback_paint(void * abstract_surface, cairo_operator_t op, 
	const cairo_pattern_t * source, const cairo_clip_t  * clip);
cairo_private cairo_int_status_t _cairo_surface_fallback_mask(void * abstract_surface, cairo_operator_t op, 
	const cairo_pattern_t * source, const cairo_pattern_t * mask, const cairo_clip_t   * clip);
cairo_private cairo_int_status_t _cairo_surface_fallback_stroke(void * abstract_surface,
    cairo_operator_t op, const cairo_pattern_t * source, const cairo_path_fixed_t * path,
    const cairo_stroke_style_t * style, const cairo_matrix_t * ctm,
    const cairo_matrix_t * ctm_inverse, double tolerance, cairo_antialias_t antialias, const cairo_clip_t * clip);
cairo_private cairo_int_status_t _cairo_surface_fallback_fill(void * abstract_surface, cairo_operator_t op,
    const cairo_pattern_t * source, const cairo_path_fixed_t * path, CairoFillRule fill_rule,
    double tolerance, cairo_antialias_t antialias, const cairo_clip_t  * clip);
cairo_private cairo_int_status_t _cairo_surface_fallback_glyphs(void * abstract_surface, cairo_operator_t op,
    const cairo_pattern_t * source, cairo_glyph_t * glyphs, int num_glyphs, cairo_scaled_font_t * scaled_font, const cairo_clip_t * clip);

CAIRO_END_DECLS
//
//#include "cairo-pdf.h"
//
#if CAIRO_HAS_PDF_SURFACE
	CAIRO_BEGIN_DECLS
	/**
	 * cairo_pdf_version_t:
	 * @CAIRO_PDF_VERSION_1_4: The version 1.4 of the PDF specification. (Since 1.10)
	 * @CAIRO_PDF_VERSION_1_5: The version 1.5 of the PDF specification. (Since 1.10)
	 *
	 * #cairo_pdf_version_t is used to describe the version number of the PDF
	 * specification that a generated PDF file will conform to.
	 *
	 * Since: 1.10
	 **/
	typedef enum _cairo_pdf_version {
		CAIRO_PDF_VERSION_1_4,
		CAIRO_PDF_VERSION_1_5
	} cairo_pdf_version_t;

	cairo_public cairo_surface_t * cairo_pdf_surface_create (const char * filename, double width_in_points, double height_in_points);
	cairo_public cairo_surface_t * cairo_pdf_surface_create_for_stream (cairo_write_func_t	write_func,
		void * closure, double width_in_points, double height_in_points);
	cairo_public void cairo_pdf_surface_restrict_to_version (cairo_surface_t * surface, cairo_pdf_version_t version);
	cairo_public void cairo_pdf_get_versions (cairo_pdf_version_t const	** versions, int * num_versions);
	cairo_public const char * cairo_pdf_version_to_string(cairo_pdf_version_t version);
	cairo_public void cairo_pdf_surface_set_size(cairo_surface_t * surface, double width_in_points, double height_in_points);
	CAIRO_END_DECLS
#else
	#error Cairo was not compiled with support for the pdf backend
#endif // } CAIRO_HAS_PDF_SURFACE 
//
//#include "cairo-pdf-operators-private.h"
// 
// The glyph buffer size is based on the expected maximum glyphs in a
// line so that an entire line can be emitted in as one string. If the
// glyphs in a line exceeds this size the only downside is the slight overhead of emitting two strings.
// 
#define PDF_GLYPH_BUFFER_SIZE 200

typedef cairo_int_status_t (*cairo_pdf_operators_use_font_subset_t)(uint font_id, uint subset_id, void * closure);

typedef struct _cairo_pdf_glyph {
	uint   glyph_index;
	double x_position;
	double x_advance;
} cairo_pdf_glyph_t;

typedef struct _cairo_pdf_operators {
	cairo_output_stream_t * stream;
	cairo_matrix_t cairo_to_pdf;
	cairo_scaled_font_subsets_t * font_subsets;
	cairo_pdf_operators_use_font_subset_t use_font_subset;
	void * use_font_subset_closure;
	cairo_bool_t ps_output; /* output is for PostScript */
	cairo_bool_t use_actual_text;
	cairo_bool_t in_text_object; /* inside BT/ET pair */
	//
	// PDF text state 
	//
	cairo_bool_t is_new_text_object; /* text object started but matrix and font not yet selected */
	uint font_id;
	uint subset_id;
	cairo_matrix_t text_matrix; /* PDF text matrix (Tlm in the PDF reference) */
	cairo_matrix_t cairo_to_pdftext; /* translate cairo coords to PDF text space */
	cairo_matrix_t font_matrix_inverse;
	double cur_x; /* Current position in PDF text space (Tm in the PDF reference) */
	double cur_y;
	int hex_width;
	cairo_bool_t is_latin;
	int num_glyphs;
	double glyph_buf_x_pos;
	cairo_pdf_glyph_t glyphs[PDF_GLYPH_BUFFER_SIZE];
	/* PDF line style */
	cairo_bool_t has_line_style;
	double line_width;
	cairo_line_cap_t line_cap;
	cairo_line_join_t line_join;
	double miter_limit;
	cairo_bool_t has_dashes;
} cairo_pdf_operators_t;

cairo_private void _cairo_pdf_operators_init(cairo_pdf_operators_t * pdf_operators,
    cairo_output_stream_t * stream, cairo_matrix_t * cairo_to_pdf, cairo_scaled_font_subsets_t * font_subsets, cairo_bool_t ps);
cairo_private cairo_status_t _cairo_pdf_operators_fini(cairo_pdf_operators_t * pdf_operators);
cairo_private void _cairo_pdf_operators_set_font_subsets_callback(cairo_pdf_operators_t * pdf_operators,
    cairo_pdf_operators_use_font_subset_t use_font_subset, void * closure);
cairo_private void _cairo_pdf_operators_set_stream(cairo_pdf_operators_t * pdf_operators, cairo_output_stream_t * stream);
cairo_private void _cairo_pdf_operators_set_cairo_to_pdf_matrix(cairo_pdf_operators_t * pdf_operators, cairo_matrix_t * cairo_to_pdf);
cairo_private void _cairo_pdf_operators_enable_actual_text(cairo_pdf_operators_t * pdf_operators, cairo_bool_t enable);
cairo_private cairo_status_t _cairo_pdf_operators_flush(cairo_pdf_operators_t * pdf_operators);
cairo_private void _cairo_pdf_operators_reset(cairo_pdf_operators_t * pdf_operators);
cairo_private cairo_int_status_t _cairo_pdf_operators_clip(cairo_pdf_operators_t * pdf_operators,
    const cairo_path_fixed_t * path, CairoFillRule fill_rule);
cairo_private cairo_int_status_t _cairo_pdf_operators_emit_stroke_style(cairo_pdf_operators_t * pdf_operators,
    const cairo_stroke_style_t * style, double scale);
cairo_private cairo_int_status_t _cairo_pdf_operators_stroke(cairo_pdf_operators_t * pdf_operators,
    const cairo_path_fixed_t * path, const cairo_stroke_style_t * style, const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse);
cairo_private cairo_int_status_t _cairo_pdf_operators_fill(cairo_pdf_operators_t * pdf_operators,
    const cairo_path_fixed_t * path, CairoFillRule fill_rule);
cairo_private cairo_int_status_t _cairo_pdf_operators_fill_stroke(cairo_pdf_operators_t * pdf_operators,
    const cairo_path_fixed_t * path, CairoFillRule fill_rule, const cairo_stroke_style_t * style,
    const cairo_matrix_t * ctm, const cairo_matrix_t * ctm_inverse);
cairo_private cairo_int_status_t _cairo_pdf_operators_show_text_glyphs(cairo_pdf_operators_t * pdf_operators,
    const char * utf8, int utf8_len, cairo_glyph_t * glyphs, int num_glyphs, const cairo_text_cluster_t * clusters,
    int num_clusters, cairo_text_cluster_flags_t cluster_flags, cairo_scaled_font_t * scaled_font);
//
//#include "cairo-pdf-surface-private.h"
//
typedef struct _cairo_pdf_resource {
	uint id;
} cairo_pdf_resource_t;

#define CAIRO_NUM_OPERATORS (CAIRO_OPERATOR_HSL_LUMINOSITY + 1)

typedef struct _cairo_pdf_group_resources {
	cairo_bool_t operators[CAIRO_NUM_OPERATORS];
	cairo_array_t alphas;
	cairo_array_t smasks;
	cairo_array_t patterns;
	cairo_array_t shadings;
	cairo_array_t xobjects;
	cairo_array_t fonts;
} cairo_pdf_group_resources_t;

typedef struct _cairo_pdf_source_surface_entry {
	cairo_hash_entry_t base;
	uint id;
	uchar * unique_id;
	ulong unique_id_length;
	cairo_operator_t Op;
	cairo_bool_t interpolate;
	cairo_bool_t stencil_mask;
	cairo_bool_t smask;
	cairo_pdf_resource_t surface_res;
	cairo_pdf_resource_t smask_res;
	int width;
	int height;
	CairoIRect extents;
} cairo_pdf_source_surface_entry_t;

typedef struct _cairo_pdf_source_surface {
	cairo_pattern_type_t type;
	cairo_surface_t * surface;
	cairo_pattern_t * raster_pattern;
	cairo_pdf_source_surface_entry_t * hash_entry;
} cairo_pdf_source_surface_t;

typedef struct _cairo_pdf_pattern {
	double width;
	double height;
	CairoIRect extents;
	cairo_pattern_t * pattern;
	cairo_pdf_resource_t pattern_res;
	cairo_pdf_resource_t gstate_res;
	cairo_operator_t Op;
	cairo_bool_t is_shading;
} cairo_pdf_pattern_t;

typedef enum _cairo_pdf_operation {
	PDF_PAINT,
	PDF_MASK,
	PDF_FILL,
	PDF_STROKE,
	PDF_SHOW_GLYPHS
} cairo_pdf_operation_t;

typedef struct _cairo_pdf_smask_group {
	double width;
	double height;
	CairoIRect extents;
	cairo_pdf_resource_t group_res;
	cairo_pdf_operation_t operation;
	cairo_pattern_t      * source;
	cairo_pdf_resource_t source_res;
	cairo_pattern_t      * mask;
	cairo_path_fixed_t path;
	CairoFillRule fill_rule;
	cairo_stroke_style_t style;
	cairo_matrix_t ctm;
	cairo_matrix_t ctm_inverse;
	char                 * utf8;
	int utf8_len;
	cairo_glyph_t        * glyphs;
	int num_glyphs;
	cairo_text_cluster_t * clusters;
	int num_clusters;
	cairo_bool_t cluster_flags;
	cairo_scaled_font_t  * scaled_font;
} cairo_pdf_smask_group_t;

typedef struct _cairo_pdf_jbig2_global {
	uchar * id;
	ulong id_length;
	cairo_pdf_resource_t res;
	cairo_bool_t emitted;
} cairo_pdf_jbig2_global_t;

typedef struct _cairo_pdf_surface cairo_pdf_surface_t;

struct _cairo_pdf_surface {
	cairo_surface_t base;
	// Prefer the name "output" here to avoid confusion over the
	// structure within a PDF document known as a "stream".
	cairo_output_stream_t * output;
	double width;
	double height;
	cairo_matrix_t cairo_to_pdf;
	cairo_array_t objects;
	cairo_array_t pages;
	cairo_array_t rgb_linear_functions;
	cairo_array_t alpha_linear_functions;
	cairo_array_t page_patterns;
	cairo_array_t page_surfaces;
	cairo_hash_table_t * all_surfaces;
	cairo_array_t smask_groups;
	cairo_array_t knockout_group;
	cairo_array_t jbig2_global;
	cairo_scaled_font_subsets_t * font_subsets;
	cairo_array_t fonts;
	cairo_pdf_resource_t next_available_resource;
	cairo_pdf_resource_t pages_resource;
	cairo_pdf_version_t pdf_version;
	cairo_bool_t compress_content;
	cairo_pdf_resource_t content;
	cairo_pdf_resource_t content_resources;
	cairo_pdf_group_resources_t resources;
	cairo_bool_t has_fallback_images;
	cairo_bool_t header_emitted;

	struct {
		cairo_bool_t active;
		cairo_pdf_resource_t self;
		cairo_pdf_resource_t length;
		long start_offset;
		cairo_bool_t compressed;
		cairo_output_stream_t * old_output;
	} pdf_stream;

	struct {
		cairo_bool_t active;
		cairo_output_stream_t * stream;
		cairo_output_stream_t * mem_stream;
		cairo_output_stream_t * old_output;
		cairo_pdf_resource_t resource;
		cairo_box_double_t bbox;
		cairo_bool_t is_knockout;
	} group_stream;
	cairo_surface_clipper_t clipper;
	cairo_pdf_operators_t pdf_operators;
	cairo_paginated_mode_t paginated_mode;
	cairo_bool_t select_pattern_gstate_saved;
	cairo_bool_t force_fallbacks;
	cairo_operator_t current_operator;
	cairo_bool_t current_pattern_is_solid_color;
	cairo_bool_t current_color_is_stroke;
	double current_color_red;
	double current_color_green;
	double current_color_blue;
	double current_color_alpha;
	cairo_surface_t * paginated_surface;
};
//
//#include "cairo-pdf-shading-private.h"
//
typedef struct _cairo_pdf_shading {
	int    shading_type;
	int    bits_per_coordinate;
	int    bits_per_component;
	int    bits_per_flag;
	double * decode_array;
	int    decode_array_length;
	uchar * data;
	ulong  data_length;
} cairo_pdf_shading_t;
/**
 * _cairo_pdf_shading_init_color:
 * @shading: a #cairo_pdf_shading_t to initialize
 * @pattern: the #cairo_mesh_pattern_t to initialize from
 *
 * Generate the PDF shading dictionary data for the a PDF type 7
 * shading from RGB part of the specified mesh pattern.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful, possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_pdf_shading_init_color(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * pattern);
/**
 * _cairo_pdf_shading_init_alpha:
 * @shading: a #cairo_pdf_shading_t to initialize
 * @pattern: the #cairo_mesh_pattern_t to initialize from
 *
 * Generate the PDF shading dictionary data for a PDF type 7
 * shading from alpha part of the specified mesh pattern.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful, possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_pdf_shading_init_alpha(cairo_pdf_shading_t * shading, const cairo_mesh_pattern_t * pattern);
/**
 * _cairo_pdf_shading_fini:
 * @shading: a #cairo_pdf_shading_t
 *
 * Free all resources associated with @shading.  After this call,
 * @shading should not be used again without a subsequent call to
 * _cairo_pdf_shading_init() again first.
 **/
cairo_private void _cairo_pdf_shading_fini(cairo_pdf_shading_t * shading);
//
//#include "cairo-type1-private.h"
//
#if CAIRO_HAS_FONT_SUBSET // {
	//
	// Magic constants for the type1 eexec encryption 
	//
	#define CAIRO_TYPE1_ENCRYPT_C1        ((ushort)52845)
	#define CAIRO_TYPE1_ENCRYPT_C2        ((ushort)22719)
	#define CAIRO_TYPE1_PRIVATE_DICT_KEY  ((ushort)55665)
	#define CAIRO_TYPE1_CHARSTRING_KEY    ((ushort)4330)
//
//#include "cairo-type3-glyph-surface-private.h"
//
typedef cairo_int_status_t (*cairo_type3_glyph_surface_emit_image_t)(cairo_image_surface_t * image, cairo_output_stream_t * stream);

typedef struct cairo_type3_glyph_surface {
	cairo_surface_t base;
	cairo_scaled_font_t * scaled_font;
	cairo_output_stream_t * stream;
	cairo_pdf_operators_t pdf_operators;
	cairo_matrix_t cairo_to_pdf;
	cairo_type3_glyph_surface_emit_image_t emit_image;
	cairo_surface_clipper_t clipper;
} cairo_type3_glyph_surface_t;

cairo_private cairo_surface_t * _cairo_type3_glyph_surface_create(cairo_scaled_font_t   * scaled_font,
    cairo_output_stream_t * stream, cairo_type3_glyph_surface_emit_image_t emit_image,
    cairo_scaled_font_subsets_t * font_subsets, cairo_bool_t ps_output);
cairo_private void _cairo_type3_glyph_surface_set_font_subsets_callback(void * abstract_surface,
    cairo_pdf_operators_use_font_subset_t use_font_subset, void * closure);
cairo_private cairo_status_t _cairo_type3_glyph_surface_analyze_glyph(void * abstract_surface, ulong glyph_index);
cairo_private cairo_status_t _cairo_type3_glyph_surface_emit_glyph(void  * abstract_surface,
    cairo_output_stream_t * stream, ulong glyph_index, cairo_box_t * bbox, double  * width);
//
//#include "cairo-scaled-font-subsets-private.h"
//
typedef struct _cairo_scaled_font_subsets_glyph {
	uint   font_id;
	uint   subset_id;
	uint   subset_glyph_index;
	cairo_bool_t is_scaled;
	cairo_bool_t is_composite;
	cairo_bool_t is_latin;
	double x_advance;
	double y_advance;
	cairo_bool_t utf8_is_mapped;
	uint32_t unicode;
} cairo_scaled_font_subsets_glyph_t;
/**
 * _cairo_scaled_font_subsets_create_scaled:
 *
 * Create a new #cairo_scaled_font_subsets_t object which can be used
 * to create subsets of any number of #cairo_scaled_font_t
 * objects. This allows the (arbitrarily large and sparse) glyph
 * indices of a #cairo_scaled_font_t to be mapped to one or more font
 * subsets with glyph indices packed into the range
 * [0 .. max_glyphs_per_subset).
 *
 * Return value: a pointer to the newly creates font subsets. The
 * caller owns this object and should call
 * _cairo_scaled_font_subsets_destroy() when done with it.
 **/
cairo_private cairo_scaled_font_subsets_t * _cairo_scaled_font_subsets_create_scaled(void);
/**
 * _cairo_scaled_font_subsets_create_simple:
 *
 * Create a new #cairo_scaled_font_subsets_t object which can be used
 * to create font subsets suitable for embedding as Postscript or PDF
 * simple fonts.
 *
 * Glyphs with an outline path available will be mapped to one font
 * subset for each font face. Glyphs from bitmap fonts will mapped to
 * separate font subsets for each #cairo_scaled_font_t object.
 *
 * The maximum number of glyphs per subset is 256. Each subset
 * reserves the first glyph for the .notdef glyph.
 *
 * Return value: a pointer to the newly creates font subsets. The
 * caller owns this object and should call
 * _cairo_scaled_font_subsets_destroy() when done with it.
 **/
cairo_private cairo_scaled_font_subsets_t * _cairo_scaled_font_subsets_create_simple(void);
/**
 * _cairo_scaled_font_subsets_create_composite:
 *
 * Create a new #cairo_scaled_font_subsets_t object which can be used
 * to create font subsets suitable for embedding as Postscript or PDF
 * composite fonts.
 *
 * Glyphs with an outline path available will be mapped to one font
 * subset for each font face. Each unscaled subset has a maximum of
 * 65536 glyphs except for Type1 fonts which have a maximum of 256 glyphs.
 *
 * Glyphs from bitmap fonts will mapped to separate font subsets for
 * each #cairo_scaled_font_t object. Each unscaled subset has a maximum
 * of 256 glyphs.
 *
 * Each subset reserves the first glyph for the .notdef glyph.
 *
 * Return value: a pointer to the newly creates font subsets. The
 * caller owns this object and should call
 * _cairo_scaled_font_subsets_destroy() when done with it.
 **/
cairo_private cairo_scaled_font_subsets_t * _cairo_scaled_font_subsets_create_composite(void);

/**
 * _cairo_scaled_font_subsets_destroy:
 * @font_subsets: a #cairo_scaled_font_subsets_t object to be destroyed
 *
 * Destroys @font_subsets and all resources associated with it.
 **/
cairo_private void _cairo_scaled_font_subsets_destroy(cairo_scaled_font_subsets_t * font_subsets);
/**
 * _cairo_scaled_font_subsets_enable_latin_subset:
 * @font_subsets: a #cairo_scaled_font_subsets_t object to be destroyed
 * @use_latin: a #cairo_bool_t indicating if a latin subset is to be used
 *
 * If enabled, all CP1252 characters will be placed in a separate
 * 8-bit latin subset.
 **/
cairo_private void _cairo_scaled_font_subsets_enable_latin_subset(cairo_scaled_font_subsets_t * font_subsets, cairo_bool_t use_latin);
/**
 * _cairo_scaled_font_subsets_map_glyph:
 * @font_subsets: a #cairo_scaled_font_subsets_t
 * @scaled_font: the font of the glyph to be mapped
 * @scaled_font_glyph_index: the index of the glyph to be mapped
 * @utf8: a string of text encoded in UTF-8
 * @utf8_len: length of @utf8 in bytes
 * @subset_glyph_ret: return structure containing subset font and glyph id
 *
 * Map a glyph from a #cairo_scaled_font to a new index within a
 * subset of that font. The mapping performed is from the tuple:
 *
 *	(scaled_font, scaled_font_glyph_index)
 *
 * to the tuple:
 *
 *	(font_id, subset_id, subset_glyph_index)
 *
 * This mapping is 1:1. If the input tuple has previously mapped, the
 * the output tuple previously returned will be returned again.
 *
 * Otherwise, the return tuple will be constructed as follows:
 *
 * 1) There is a 1:1 correspondence between the input scaled_font
 *    value and the output font_id value. If no mapping has been
 *    previously performed with the scaled_font value then the
 *    smallest unused font_id value will be returned.
 *
 * 2) Within the set of output tuples of the same font_id value the
 *    smallest value of subset_id will be returned such that
 *    subset_glyph_index does not exceed max_glyphs_per_subset (as
 *    passed to _cairo_scaled_font_subsets_create()) and that the
 *    resulting tuple is unique.
 *
 * 3) The smallest value of subset_glyph_index is returned such that
 *    the resulting tuple is unique.
 *
 * The net result is that any #cairo_scaled_font_t will be represented
 * by one or more font subsets. Each subset is effectively a tuple of
 * (scaled_font, font_id, subset_id) and within each subset there
 * exists a mapping of scaled_glyph_font_index to subset_glyph_index.
 *
 * This final description of a font subset is the same representation
 * used by #cairo_scaled_font_subset_t as provided by
 * _cairo_scaled_font_subsets_foreach.
 *
 * @utf8 and @utf8_len specify a string of unicode characters that the
 * glyph @scaled_font_glyph_index maps to. If @utf8_is_mapped in
 * @subset_glyph_ret is %TRUE, the font subsetting will (where index to
 * unicode mapping is supported) ensure that @scaled_font_glyph_index
 * maps to @utf8. If @utf8_is_mapped is %FALSE,
 * @scaled_font_glyph_index has already been mapped to a different
 * unicode string.
 *
 * The returned values in the #cairo_scaled_font_subsets_glyph_t struct are:
 *
 * @font_id: The font ID of the mapped glyph
 * @subset_id : The subset ID of the mapped glyph within the @font_id
 * @subset_glyph_index: The index of the mapped glyph within the @subset_id subset
 * @is_scaled: If true, the mapped glyph is from a bitmap font, and separate font
 * subset is created for each font scale used. If false, the outline of the mapped glyph
 * is available. One font subset for each font face is created.
 * @x_advance, @y_advance: When @is_scaled is true, @x_advance and @y_advance contain
 * the x and y advance for the mapped glyph in device space.
 * When @is_scaled is false, @x_advance and @y_advance contain the x and y advance for
 * the the mapped glyph from an unhinted 1 point font.
 * @utf8_is_mapped: If true the utf8 string provided to _cairo_scaled_font_subsets_map_glyph()
 * is (or already was) the utf8 string mapped to this glyph. If false the glyph is already
 * mapped to a different utf8 string.
 * @unicode: the unicode character mapped to this glyph by the font backend.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful, or a non-zero
 * value indicating an error. Possible errors include
 * %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_scaled_font_subsets_map_glyph(cairo_scaled_font_subsets_t * font_subsets,
    cairo_scaled_font_t * scaled_font, ulong scaled_font_glyph_index, const char * utf8, int utf8_len,
    cairo_scaled_font_subsets_glyph_t * subset_glyph_ret);
typedef cairo_int_status_t (*cairo_scaled_font_subset_callback_func_t)(cairo_scaled_font_subset_t * font_subset, void * closure);
/**
 * _cairo_scaled_font_subsets_foreach_scaled:
 * @font_subsets: a #cairo_scaled_font_subsets_t
 * @font_subset_callback: a function to be called for each font subset
 * @closure: closure data for the callback function
 *
 * Iterate over each unique scaled font subset as created by calls to
 * _cairo_scaled_font_subsets_map_glyph(). A subset is determined by
 * unique pairs of (font_id, subset_id) as returned by
 * _cairo_scaled_font_subsets_map_glyph().
 *
 * For each subset, @font_subset_callback will be called and will be
 * provided with both a #cairo_scaled_font_subset_t object containing
 * all the glyphs in the subset as well as the value of @closure.
 *
 * The #cairo_scaled_font_subset_t object contains the scaled_font,
 * the font_id, and the subset_id corresponding to all glyphs
 * belonging to the subset. In addition, it contains an array providing
 * a mapping between subset glyph indices and the original scaled font
 * glyph indices.
 *
 * The index of the array corresponds to subset_glyph_index values
 * returned by _cairo_scaled_font_subsets_map_glyph() while the
 * values of the array correspond to the scaled_font_glyph_index
 * values passed as input to the same function.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful, or a non-zero
 * value indicating an error. Possible errors include
 * %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_scaled_font_subsets_foreach_scaled(cairo_scaled_font_subsets_t * font_subsets,
    cairo_scaled_font_subset_callback_func_t font_subset_callback, void * closure);
/**
 * _cairo_scaled_font_subsets_foreach_unscaled:
 * @font_subsets: a #cairo_scaled_font_subsets_t
 * @font_subset_callback: a function to be called for each font subset
 * @closure: closure data for the callback function
 *
 * Iterate over each unique unscaled font subset as created by calls to
 * _cairo_scaled_font_subsets_map_glyph(). A subset is determined by
 * unique pairs of (font_id, subset_id) as returned by
 * _cairo_scaled_font_subsets_map_glyph().
 *
 * For each subset, @font_subset_callback will be called and will be
 * provided with both a #cairo_scaled_font_subset_t object containing
 * all the glyphs in the subset as well as the value of @closure.
 *
 * The #cairo_scaled_font_subset_t object contains the scaled_font,
 * the font_id, and the subset_id corresponding to all glyphs
 * belonging to the subset. In addition, it contains an array providing
 * a mapping between subset glyph indices and the original scaled font
 * glyph indices.
 *
 * The index of the array corresponds to subset_glyph_index values
 * returned by _cairo_scaled_font_subsets_map_glyph() while the
 * values of the array correspond to the scaled_font_glyph_index
 * values passed as input to the same function.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful, or a non-zero
 * value indicating an error. Possible errors include
 * %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_scaled_font_subsets_foreach_unscaled(cairo_scaled_font_subsets_t * font_subsets,
    cairo_scaled_font_subset_callback_func_t font_subset_callback, void * closure);
/**
 * _cairo_scaled_font_subsets_foreach_user:
 * @font_subsets: a #cairo_scaled_font_subsets_t
 * @font_subset_callback: a function to be called for each font subset
 * @closure: closure data for the callback function
 *
 * Iterate over each unique scaled font subset as created by calls to
 * _cairo_scaled_font_subsets_map_glyph(). A subset is determined by
 * unique pairs of (font_id, subset_id) as returned by
 * _cairo_scaled_font_subsets_map_glyph().
 *
 * For each subset, @font_subset_callback will be called and will be
 * provided with both a #cairo_scaled_font_subset_t object containing
 * all the glyphs in the subset as well as the value of @closure.
 *
 * The #cairo_scaled_font_subset_t object contains the scaled_font,
 * the font_id, and the subset_id corresponding to all glyphs
 * belonging to the subset. In addition, it contains an array providing
 * a mapping between subset glyph indices and the original scaled font
 * glyph indices.
 *
 * The index of the array corresponds to subset_glyph_index values
 * returned by _cairo_scaled_font_subsets_map_glyph() while the
 * values of the array correspond to the scaled_font_glyph_index
 * values passed as input to the same function.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful, or a non-zero
 * value indicating an error. Possible errors include
 * %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_scaled_font_subsets_foreach_user(cairo_scaled_font_subsets_t * font_subsets,
    cairo_scaled_font_subset_callback_func_t font_subset_callback, void * closure);
/**
 * _cairo_scaled_font_subset_create_glyph_names:
 * @font_subsets: a #cairo_scaled_font_subsets_t
 *
 * Create an array of strings containing the glyph name for each glyph
 * in @font_subsets. The array as store in font_subsets->glyph_names.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font backend does not support
 * mapping the glyph indices to unicode characters. Possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_int_status_t _cairo_scaled_font_subset_create_glyph_names(cairo_scaled_font_subset_t * subset);

typedef struct _cairo_cff_subset {
	char * family_name_utf8;
	char * ps_name;
	double * widths;
	double x_min, y_min, x_max, y_max;
	double ascent, descent;
	char * data;
	ulong data_length;
} cairo_cff_subset_t;
/**
 * _cairo_cff_subset_init:
 * @cff_subset: a #cairo_cff_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a
 * cff file corresponding to @font_subset and initialize
 * @cff_subset with information about the subset and the cff
 * data.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a
 * cff file, or an non-zero value indicating an error.  Possible
 * errors include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_cff_subset_init(cairo_cff_subset_t * cff_subset, const char * name, cairo_scaled_font_subset_t * font_subset);
/**
 * _cairo_cff_subset_fini:
 * @cff_subset: a #cairo_cff_subset_t
 *
 * Free all resources associated with @cff_subset.  After this
 * call, @cff_subset should not be used again without a
 * subsequent call to _cairo_cff_subset_init() again first.
 **/
cairo_private void _cairo_cff_subset_fini(cairo_cff_subset_t * cff_subset);
/**
 * _cairo_cff_scaled_font_is_cid_cff:
 * @scaled_font: a #cairo_scaled_font_t
 *
 * Return %TRUE if @scaled_font is a CID CFF font, otherwise return %FALSE.
 **/
cairo_private cairo_bool_t _cairo_cff_scaled_font_is_cid_cff(cairo_scaled_font_t * scaled_font);
/**
 * _cairo_cff_fallback_init:
 * @cff_subset: a #cairo_cff_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a cff
 * file corresponding to @font_subset and initialize @cff_subset
 * with information about the subset and the cff data.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a
 * cff file, or an non-zero value indicating an error.  Possible
 * errors include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_cff_fallback_init(cairo_cff_subset_t * cff_subset, const char * name, cairo_scaled_font_subset_t  * font_subset);
/**
 * _cairo_cff_fallback_fini:
 * @cff_subset: a #cairo_cff_subset_t
 *
 * Free all resources associated with @cff_subset.  After this
 * call, @cff_subset should not be used again without a
 * subsequent call to _cairo_cff_subset_init() again first.
 **/
cairo_private void _cairo_cff_fallback_fini(cairo_cff_subset_t * cff_subset);

typedef struct _cairo_truetype_subset {
	char * family_name_utf8;
	char * ps_name;
	double * widths;
	double x_min;
	double y_min;
	double x_max;
	double y_max;
	double ascent;
	double descent;
	uchar * data;
	ulong  data_length;
	ulong * string_offsets;
	ulong  num_string_offsets;
} cairo_truetype_subset_t;
/**
 * _cairo_truetype_subset_init_ps:
 * @truetype_subset: a #cairo_truetype_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a
 * truetype file corresponding to @font_subset and initialize
 * @truetype_subset with information about the subset and the truetype
 * data. The generated font will be suitable for embedding in
 * PostScript.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a
 * truetype file, or an non-zero value indicating an error.  Possible
 * errors include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_truetype_subset_init_ps(cairo_truetype_subset_t * truetype_subset, cairo_scaled_font_subset_t * font_subset);
/**
 * _cairo_truetype_subset_init_pdf:
 * @truetype_subset: a #cairo_truetype_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a
 * truetype file corresponding to @font_subset and initialize
 * @truetype_subset with information about the subset and the truetype
 * data. The generated font will be suitable for embedding in
 * PDF.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a
 * truetype file, or an non-zero value indicating an error.  Possible
 * errors include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_truetype_subset_init_pdf(cairo_truetype_subset_t * truetype_subset, cairo_scaled_font_subset_t * font_subset);
/**
 * _cairo_truetype_subset_fini:
 * @truetype_subset: a #cairo_truetype_subset_t
 *
 * Free all resources associated with @truetype_subset.  After this
 * call, @truetype_subset should not be used again without a
 * subsequent call to _cairo_truetype_subset_init() again first.
 **/
cairo_private void _cairo_truetype_subset_fini(cairo_truetype_subset_t * truetype_subset);
cairo_private const char * _cairo_ps_standard_encoding_to_glyphname(int glyph);
cairo_private int _cairo_unicode_to_winansi(ulong unicode);
cairo_private const char * _cairo_winansi_to_glyphname(int glyph);

typedef struct _cairo_type1_subset {
	char * base_font;
	double * widths;
	double x_min;
	double y_min;
	double x_max;
	double y_max;
	double ascent;
	double descent;
	char * data;
	ulong  header_length;
	ulong  data_length;
	ulong  trailer_length;
} cairo_type1_subset_t;
/**
 * _cairo_type1_subset_init:
 * @type1_subset: a #cairo_type1_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 * @hex_encode: if true the encrypted portion of the font is hex encoded
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a type1
 * file corresponding to @font_subset and initialize @type1_subset
 * with information about the subset and the type1 data.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a type1
 * file, or an non-zero value indicating an error.  Possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_type1_subset_init(cairo_type1_subset_t * type_subset, const char * name, cairo_scaled_font_subset_t * font_subset, cairo_bool_t hex_encode);
/**
 * _cairo_type1_subset_fini:
 * @type1_subset: a #cairo_type1_subset_t
 *
 * Free all resources associated with @type1_subset.  After this call,
 * @type1_subset should not be used again without a subsequent call to
 * _cairo_truetype_type1_init() again first.
 **/
cairo_private void _cairo_type1_subset_fini(cairo_type1_subset_t * subset);
/**
 * _cairo_type1_scaled_font_is_type1:
 * @scaled_font: a #cairo_scaled_font_t
 *
 * Return %TRUE if @scaled_font is a Type 1 font, otherwise return %FALSE.
 **/
cairo_private cairo_bool_t _cairo_type1_scaled_font_is_type1(cairo_scaled_font_t  * scaled_font);
/**
 * _cairo_type1_fallback_init_binary:
 * @type1_subset: a #cairo_type1_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a type1
 * file corresponding to @font_subset and initialize @type1_subset
 * with information about the subset and the type1 data.  The encrypted
 * part of the font is binary encoded.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a type1
 * file, or an non-zero value indicating an error.  Possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_type1_fallback_init_binary(cairo_type1_subset_t * type_subset, const char * name, cairo_scaled_font_subset_t * font_subset);
/**
 * _cairo_type1_fallback_init_hex:
 * @type1_subset: a #cairo_type1_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate a type1
 * file corresponding to @font_subset and initialize @type1_subset
 * with information about the subset and the type1 data. The encrypted
 * part of the font is hex encoded.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a type1
 * file, or an non-zero value indicating an error.  Possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_type1_fallback_init_hex(cairo_type1_subset_t * type_subset, const char * name, cairo_scaled_font_subset_t * font_subset);
/**
 * _cairo_type1_fallback_fini:
 * @type1_subset: a #cairo_type1_subset_t
 *
 * Free all resources associated with @type1_subset.  After this call,
 * @type1_subset should not be used again without a subsequent call to
 * _cairo_truetype_type1_init() again first.
 **/
cairo_private void _cairo_type1_fallback_fini(cairo_type1_subset_t * subset);

typedef struct _cairo_type2_charstrings {
	int  * widths;
	long   x_min;
	long   y_min;
	long   x_max;
	long   y_max;
	long   ascent;
	long   descent;
	cairo_array_t charstrings;
} cairo_type2_charstrings_t;
/**
 * _cairo_type2_charstrings_init:
 * @type2_subset: a #cairo_type2_subset_t to initialize
 * @font_subset: the #cairo_scaled_font_subset_t to initialize from
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) generate type2
 * charstrings to @font_subset and initialize @type2_subset
 * with information about the subset.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font can't be subset as a type2
 * charstrings, or an non-zero value indicating an error.  Possible errors
 * include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_status_t _cairo_type2_charstrings_init(cairo_type2_charstrings_t   * charstrings, cairo_scaled_font_subset_t  * font_subset);
/**
 * _cairo_type2_charstrings_fini:
 * @subset: a #cairo_type2_charstrings_t
 *
 * Free all resources associated with @type2_charstring.  After this call,
 * @type2_charstring should not be used again without a subsequent call to
 * _cairo_type2_charstring_init() again first.
 **/
cairo_private void _cairo_type2_charstrings_fini(cairo_type2_charstrings_t * charstrings);
/**
 * _cairo_truetype_index_to_ucs4:
 * @scaled_font: the #cairo_scaled_font_t
 * @index: the glyph index
 * @ucs4: return value for the unicode value of the glyph
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) assign
 * the unicode character of the glyph to @ucs4.
 *
 * If mapping glyph indices to unicode is supported but the unicode
 * value of the specified glyph is not available, @ucs4 is set to -1.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if mapping glyph indices to unicode
 * is not supported.  Possible errors include %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_int_status_t _cairo_truetype_index_to_ucs4(cairo_scaled_font_t * scaled_font, ulong index, uint32_t * ucs4);
/**
 * _cairo_truetype_read_font_name:
 * @scaled_font: the #cairo_scaled_font_t
 * @ps_name: returns the PostScript name of the font
 *           or %NULL if the name could not be found.
 * @font_name: returns the font name or %NULL if the name could not be found.
 *
 * If possible (depending on the format of the underlying
 * #cairo_scaled_font_t and the font backend in use) read the
 * PostScript and Font names from a TrueType/OpenType font.
 *
 * The font name is the full name of the font eg "DejaVu Sans Bold".
 * The PostScript name is a shortened name with spaces removed
 * suitable for use as the font name in a PS or PDF file eg
 * "DejaVuSans-Bold".
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font is not TrueType/OpenType
 * or the name table is not present.  Possible errors include
 * %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_int_status_t _cairo_truetype_read_font_name(cairo_scaled_font_t * scaled_font, char ** ps_name, char ** font_name);
/**
 * _cairo_truetype_get_style:
 * @scaled_font: the #cairo_scaled_font_t
 * @weight: returns the font weight from the OS/2 table
 * @bold: returns true if font is bold
 * @italic: returns true if font is italic
 *
 * If the font is a truetype/opentype font with an OS/2 table, get the
 * weight, bold, and italic data from the OS/2 table.  The weight
 * values have the same meaning as the lfWeight field of the Windows
 * LOGFONT structure.  Refer to the TrueType Specification for
 * definition of the weight values.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful,
 * %CAIRO_INT_STATUS_UNSUPPORTED if the font is not TrueType/OpenType
 * or the OS/2 table is not present.
 **/
cairo_private cairo_int_status_t _cairo_truetype_get_style(cairo_scaled_font_t * scaled_font, int * weight, cairo_bool_t * bold, cairo_bool_t * italic);
/**
 * _cairo_escape_ps_name:
 * @ps_name: returns the PostScript name with all invalid characters escaped
 *
 * Ensure that PostSript name is a valid PDF/PostSript name object.
 * In PDF names are treated as UTF8 and non ASCII bytes, ' ',
 * and '#' are encoded as '#' followed by 2 hex digits that
 * encode the byte.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if successful. Possible errors include
 * %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_private cairo_int_status_t _cairo_escape_ps_name(char ** ps_name);
//
//#include "cairo-truetype-subset-private.h"
//
// The structs defined here should strictly follow the TrueType
// specification and not be padded.  We use only 16-bit integer
// in their definition to guarantee that.  The fields of type
// "FIXED" in the TT spec are broken into two *_1 and *_2 16-bit
// parts, and 64-bit members are broken into four.
// 
// The test truetype-tables in the test suite makes sure that
// these tables have the right size.  Please update that test
// if you add new tables/structs that should be packed.
// 
#define MAKE_TT_TAG(a, b, c, d)    (a<<24 | b<<16 | c<<8 | d)
#define TT_TAG_CFF    MAKE_TT_TAG('C','F','F',' ')
#define TT_TAG_cmap   MAKE_TT_TAG('c','m','a','p')
#define TT_TAG_cvt    MAKE_TT_TAG('c','v','t',' ')
#define TT_TAG_fpgm   MAKE_TT_TAG('f','p','g','m')
#define TT_TAG_glyf   MAKE_TT_TAG('g','l','y','f')
#define TT_TAG_head   MAKE_TT_TAG('h','e','a','d')
#define TT_TAG_hhea   MAKE_TT_TAG('h','h','e','a')
#define TT_TAG_hmtx   MAKE_TT_TAG('h','m','t','x')
#define TT_TAG_loca   MAKE_TT_TAG('l','o','c','a')
#define TT_TAG_maxp   MAKE_TT_TAG('m','a','x','p')
#define TT_TAG_name   MAKE_TT_TAG('n','a','m','e')
#define TT_TAG_OS2    MAKE_TT_TAG('O','S','/','2')
#define TT_TAG_post   MAKE_TT_TAG('p','o','s','t')
#define TT_TAG_prep   MAKE_TT_TAG('p','r','e','p')
//
// All tt_* structs are big-endian 
//
typedef struct _tt_cmap_index {
    uint16 platform;
    uint16 encoding;
    uint32_t offset;
} tt_cmap_index_t;

typedef struct _tt_cmap {
    uint16 version;
    uint16 num_tables;
    tt_cmap_index_t index[1];
} tt_cmap_t;

typedef struct _segment_map {
    uint16 format;
    uint16 length;
    uint16 version;
    uint16 segCountX2;
    uint16 searchRange;
    uint16 entrySelector;
    uint16 rangeShift;
    uint16 endCount[1];
} tt_segment_map_t;

typedef struct _tt_head {
    int16     version_1;
    int16     version_2;
    int16     revision_1;
    int16     revision_2;
    uint16    checksum_1;
    uint16    checksum_2;
    uint16    magic_1;
    uint16    magic_2;
    uint16    flags;
    uint16    units_per_em;
    int16     created_1;
    int16     created_2;
    int16     created_3;
    int16     created_4;
    int16     modified_1;
    int16     modified_2;
    int16     modified_3;
    int16     modified_4;
    int16     x_min;                  /* FWORD */
    int16     y_min;                  /* FWORD */
    int16     x_max;                  /* FWORD */
    int16     y_max;                  /* FWORD */
    uint16    mac_style;
    uint16    lowest_rec_pppem;
    int16     font_direction_hint;
    int16     index_to_loc_format;
    int16     glyph_data_format;
} tt_head_t;

typedef struct _tt_hhea {
    int16     version_1;
    int16     version_2;
    int16     ascender;               /* FWORD */
    int16     descender;              /* FWORD */
    int16     line_gap;               /* FWORD */
    uint16    advance_max_width;      /* UFWORD */
    int16     min_left_side_bearing;  /* FWORD */
    int16     min_right_side_bearing; /* FWORD */
    int16     x_max_extent;           /* FWORD */
    int16     caret_slope_rise;
    int16     caret_slope_run;
    int16     reserved[5];
    int16     metric_data_format;
    uint16    num_hmetrics;
} tt_hhea_t;

typedef struct _tt_maxp {
    int16     version_1;
    int16     version_2;
    uint16    num_glyphs;
    uint16    max_points;
    uint16    max_contours;
    uint16    max_composite_points;
    uint16    max_composite_contours;
    uint16    max_zones;
    uint16    max_twilight_points;
    uint16    max_storage;
    uint16    max_function_defs;
    uint16    max_instruction_defs;
    uint16    max_stack_elements;
    uint16    max_size_of_instructions;
    uint16    max_component_elements;
    uint16    max_component_depth;
} tt_maxp_t;

typedef struct _tt_name_record {
    uint16 platform;
    uint16 encoding;
    uint16 language;
    uint16 name;
    uint16 length;
    uint16 offset;
} tt_name_record_t;

typedef struct _tt_name {
    uint16   format;
    uint16   num_records;
    uint16   strings_offset;
    tt_name_record_t records[1];
} tt_name_t;
//
// bitmask for fsSelection field 
//
#define TT_FS_SELECTION_ITALIC   1
#define TT_FS_SELECTION_BOLD    32
//
// _unused fields are defined in TT spec but not used by cairo 
//
typedef struct _tt_os2 {
    uint16   _unused1[2];
    uint16   usWeightClass;
    uint16   _unused2[28];
    uint16   fsSelection;
    uint16   _unused3[11];
} tt_os2_t;
//
// composite_glyph_t flags 
//
#define TT_ARG_1_AND_2_ARE_WORDS     0x0001
#define TT_WE_HAVE_A_SCALE           0x0008
#define TT_MORE_COMPONENTS           0x0020
#define TT_WE_HAVE_AN_X_AND_Y_SCALE  0x0040
#define TT_WE_HAVE_A_TWO_BY_TWO      0x0080

typedef struct _tt_composite_glyph {
    uint16 flags;
    uint16 index;
    uint16 args[6]; /* 1 to 6 arguments depending on value of flags */
} tt_composite_glyph_t;

typedef struct _tt_glyph_data {
    int16  num_contours;
    int8   data[8];
    tt_composite_glyph_t glyph;
} tt_glyph_data_t;

#endif // } CAIRO_HAS_FONT_SUBSET 
//
//#include "cairo-tee-surface-private.h"
//
cairo_private cairo_surface_t * _cairo_tee_surface_find_match(void *abstract_surface, const cairo_surface_backend_t *backend, cairo_content_t content);
//
//#include "cairo-slope-private.h"
//
static inline void _cairo_slope_init(cairo_slope_t * slope, const cairo_point_t * a, const cairo_point_t * b)
{
	slope->dx = b->x - a->x;
	slope->dy = b->y - a->y;
}

static inline cairo_bool_t _cairo_slope_equal(const cairo_slope_t * a, const cairo_slope_t * b)
{
	return _cairo_int64_eq(_cairo_int32x32_64_mul(a->dy, b->dx), _cairo_int32x32_64_mul(b->dy, a->dx));
}

static inline cairo_bool_t _cairo_slope_backwards(const cairo_slope_t * a, const cairo_slope_t * b)
{
	return _cairo_int64_negative(_cairo_int64_add(_cairo_int32x32_64_mul(a->dx, b->dx), _cairo_int32x32_64_mul(a->dy, b->dy)));
}

cairo_private int _cairo_slope_compare(const cairo_slope_t * a, const cairo_slope_t * b) cairo_pure;

CAIRO_BEGIN_DECLS
//
//#include "cairo-array-private.h"
//
// cairo-array.c structures and functions 
//
cairo_private void FASTCALL _cairo_array_init(cairo_array_t * array, uint element_size);
cairo_private void FASTCALL _cairo_array_fini(cairo_array_t * array);
cairo_private cairo_status_t FASTCALL _cairo_array_grow_by(cairo_array_t * array, uint additional);
cairo_private void FASTCALL _cairo_array_truncate(cairo_array_t * array, uint num_elements);
cairo_private cairo_status_t FASTCALL _cairo_array_append(cairo_array_t * array, const void * element);
cairo_private cairo_status_t _cairo_array_append_multiple(cairo_array_t * array, const void * elements, uint num_elements);
cairo_private cairo_status_t _cairo_array_allocate(cairo_array_t * array, uint num_elements, void ** elements);
cairo_private void * FASTCALL _cairo_array_index(cairo_array_t * array, uint index);
cairo_private const void * FASTCALL _cairo_array_index_const(const cairo_array_t * array, uint index);
cairo_private void _cairo_array_copy_element(const cairo_array_t * array, uint index, void * dst);
cairo_private uint FASTCALL _cairo_array_num_elements(const cairo_array_t * array);
cairo_private uint _cairo_array_size(const cairo_array_t * array);
//
//#include "cairo-arc-private.h"
//
cairo_private void _cairo_arc_path(cairo_t * cr, double xc, double yc, double radius, double angle1, double angle2);
cairo_private void _cairo_arc_path_negative(cairo_t * cr, double xc, double yc, double radius, double angle1, double angle2);
//
//#include "cairo-default-context-private.h"
//
//typedef struct _cairo_default_context cairo_default_context_t;

struct /*_cairo_default_context*/cairo_default_context_t {
    cairo_t base;
    cairo_gstate_t *gstate;
    cairo_gstate_t  gstate_tail[2];
    cairo_gstate_t *gstate_freelist;
    cairo_path_fixed_t path[1];
};

cairo_private cairo_t * _cairo_default_context_create(void *target);
cairo_private cairo_status_t _cairo_default_context_init(cairo_default_context_t *cr, void *target);
cairo_private void _cairo_default_context_fini(cairo_default_context_t *cr);
//
//#include "cairo-region-private.h"
//
struct _cairo_region {
	cairo_reference_count_t ref_count;
	cairo_status_t status;
	pixman_region32_t rgn;
};

cairo_private cairo_region_t * _cairo_region_create_in_error(cairo_status_t status);
cairo_private void FASTCALL _cairo_region_init(cairo_region_t * region);
cairo_private void _cairo_region_init_rectangle(cairo_region_t * region, const CairoIRect * rectangle);
cairo_private void FASTCALL _cairo_region_fini(cairo_region_t * region);
cairo_private cairo_region_t * _cairo_region_create_from_boxes(const cairo_box_t * boxes, int count);
cairo_private cairo_box_t * _cairo_region_get_boxes(const cairo_region_t * region, int * nbox);

CAIRO_END_DECLS
//
//#include "cairo-user-font-private.h"
//
cairo_private cairo_bool_t _cairo_font_face_is_user(cairo_font_face_t * font_face);
//
//
#if HAVE_VALGRIND
	#include <memcheck.h>
	#define VG(x) x
	cairo_private void _cairo_debug_check_image_surface_is_defined(const cairo_surface_t * surface);
#else
	#define VG(x)
	#define _cairo_debug_check_image_surface_is_defined(X)
#endif

cairo_private void _cairo_debug_print_path(FILE * stream, cairo_path_fixed_t * path);
cairo_private void _cairo_debug_print_polygon(FILE * stream, cairo_polygon_t * polygon);
cairo_private void _cairo_debug_print_traps(FILE * file, const cairo_traps_t * traps);
cairo_private void _cairo_debug_print_clip(FILE * stream, const cairo_clip_t * clip);

#if 0
	#define TRACE(x) fprintf(stderr, "%s: ", __FILE__), fprintf x
	#define TRACE_(x) x
#else
	#define TRACE(x)
	#define TRACE_(x)
#endif

#endif
