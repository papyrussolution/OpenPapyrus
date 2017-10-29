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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
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
// @sobolev #include <pixman.h>
#include "cairo-compiler-private.h"
//
//#include "cairo-list-private.h"
//
// Basic circular, doubly linked list implementation 
//
typedef struct _cairo_list {
    struct _cairo_list * next;
	struct _cairo_list * prev;
} cairo_list_t;
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
#include "cairo-atomic-private.h"
//
//#include "cairo-reference-count-private.h"
//
// Encapsulate operations on the object's reference count 
//
typedef struct {
	cairo_atomic_int_t ref_count;
} cairo_reference_count_t;

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
	typedef signed   __int8  int8_t; // @sobolev __int8-->signed __int8
	typedef unsigned __int8  uint8_t;
	typedef __int16          int16_t;
	typedef unsigned __int16 uint16_t;
	typedef __int32          int32_t;
	typedef unsigned __int32 uint32_t;
	typedef __int64          int64_t;
	typedef unsigned __int64 uint64_t;
	#ifndef HAVE_UINT64_T
		#define HAVE_UINT64_T 1
	#endif
#else
	#error Cannot find definitions for fixed-width integral types (uint8_t, uint32_t, etc.)
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
	#define bswap_16(p) (((((uint16_t)(p)) & 0x00ff) << 8) | (((uint16_t)(p)) >> 8));
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

/*typedef*/ struct /*_cairo_point*/ cairo_point_t {
    cairo_fixed_t x;
    cairo_fixed_t y;
} /*cairo_point_t*/;
//
#include "cairo-fixed-private.h"
#include "cairo-types-private.h"
#include "cairo-error-private.h"
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
//#include "cairo-private.h"
//
CAIRO_BEGIN_DECLS

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
	uint8_t coverage; /* The pixel coverage for the pixels to the right. */
	uint8_t inverse; /* between regular mask and clip */
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

struct _cairo_surface {
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
	unsigned _finishing : 1;
	unsigned finished : 1;
	unsigned is_clear : 1;
	unsigned has_font_options : 1;
	unsigned owns_device : 1;
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
	// current snapshots of this surface
	cairo_list_t snapshots;
	// place upon snapshot list 
	cairo_list_t snapshot;
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

	/* Get the extents of the current surface. For many surface types
	 * this will be as simple as { x=0, y=0, width=surface->width,
	 * height=surface->height}.
	 *
	 * If this function is not implemented, or if it returns
	 * FALSE the surface is considered to be
	 * boundless and infinite bounds are used for it.
	 */
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

CAIRO_END_DECLS
//
//#include "cairo-surface-inline.h"
//
static inline cairo_status_t __cairo_surface_flush(cairo_surface_t * surface, unsigned flags)
{
	cairo_status_t status = CAIRO_STATUS_SUCCESS;
	if(surface->backend->flush)
		status = surface->backend->flush(surface, flags);
	return status;
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
#include "cairo-image-surface-private.h"
#include "cairo-image-surface-inline.h"
#include "cairo-surface-offset-private.h"
//
//#include "cairo-damage-private.h"
//
CAIRO_BEGIN_DECLS

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

CAIRO_END_DECLS
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

CAIRO_BEGIN_DECLS

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
	static inline uint16_t cairo_const cpu_to_be16(uint16_t v)
	{
		return (v << 8) | (v >> 8);
	}
	static inline uint16_t cairo_const be16_to_cpu(uint16_t v)
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
static inline uint16_t get_unaligned_be16(const uchar * p)
{
	return p[0] << 8 | p[1];
}

static inline uint32_t get_unaligned_be32(const uchar * p)
{
	return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline void put_unaligned_be16(uint16_t v, uchar * p)
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

typedef struct {
	const cairo_user_data_key_t * key;
	void * user_data;
	cairo_destroy_func_t destroy;
} cairo_user_data_slot_t;

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

/* #cairo_toy_font_face_t - simple family/slant/weight font faces used for
 * the built-in font API
 */

typedef struct _cairo_toy_font_face {
	cairo_font_face_t base;
	const char * family;
	cairo_bool_t owns_family;
	cairo_font_slant_t slant;
	cairo_font_weight_t weight;
	cairo_font_face_t * impl_face; /* The non-toy font face this actually uses */
} cairo_toy_font_face_t;

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

	/* A backend only needs to implement this or ucs4_to_index(), not
	 * both. This allows the backend to do something more sophisticated
	 * then just converting characters one by one.
	 */
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

/* concrete font backends */
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

#if   CAIRO_HAS_WIN32_FONT
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

cairo_private uint16_t _cairo_half_from_float(float f) cairo_const;
cairo_private cairo_bool_t FASTCALL _cairo_operator_bounded_by_mask(cairo_operator_t op) cairo_const;
cairo_private cairo_bool_t FASTCALL _cairo_operator_bounded_by_source(cairo_operator_t op) cairo_const;

enum {
	CAIRO_OPERATOR_BOUND_BY_MASK = 1 << 1,
	CAIRO_OPERATOR_BOUND_BY_SOURCE = 1 << 2,
};

cairo_private uint32_t _cairo_operator_bounded_by_either(cairo_operator_t op) cairo_const;
/* cairo-color.c */
cairo_private const cairo_color_t * FASTCALL _cairo_stock_color(cairo_stock_t stock) cairo_pure;

#define CAIRO_COLOR_WHITE       _cairo_stock_color(CAIRO_STOCK_WHITE)
#define CAIRO_COLOR_BLACK       _cairo_stock_color(CAIRO_STOCK_BLACK)
#define CAIRO_COLOR_TRANSPARENT _cairo_stock_color(CAIRO_STOCK_TRANSPARENT)

cairo_private uint16_t _cairo_color_double_to_short(double d) cairo_const;

cairo_private void _cairo_color_init_rgba(cairo_color_t * color, double red, double green, double blue, double alpha);
cairo_private void _cairo_color_multiply_alpha(cairo_color_t * color, double alpha);
cairo_private void _cairo_color_get_rgba(cairo_color_t * color, double * red, double * green, double * blue, double * alpha);
cairo_private void _cairo_color_get_rgba_premultiplied(cairo_color_t * color, double * red, double * green, double * blue, double * alpha);
cairo_private cairo_bool_t _cairo_color_equal(const cairo_color_t * color_a, const cairo_color_t * color_b) cairo_pure;
cairo_private cairo_bool_t _cairo_color_stop_equal(const cairo_color_stop_t * color_a, const cairo_color_stop_t * color_b) cairo_pure;
cairo_private cairo_content_t _cairo_color_get_content(const cairo_color_t * color) cairo_pure;

/* cairo-font-face.c */

extern const cairo_private cairo_font_face_t _cairo_font_face_nil;
extern const cairo_private cairo_font_face_t _cairo_font_face_nil_file_not_found; // @v1.14.6

cairo_private void _cairo_font_face_init(cairo_font_face_t * font_face, const cairo_font_face_backend_t * backend);
cairo_private cairo_bool_t _cairo_font_face_destroy(void * abstract_face);
cairo_private cairo_status_t _cairo_font_face_set_error(cairo_font_face_t * font_face, cairo_status_t status);
cairo_private void _cairo_unscaled_font_init(cairo_unscaled_font_t * font, const cairo_unscaled_font_backend_t * backend);
cairo_private_no_warn cairo_unscaled_font_t * _cairo_unscaled_font_reference(cairo_unscaled_font_t * font);
cairo_private void _cairo_unscaled_font_destroy(cairo_unscaled_font_t * font);

/* cairo-font-face-twin.c */

cairo_private cairo_font_face_t * _cairo_font_face_twin_create_fallback(void);
cairo_private cairo_status_t _cairo_font_face_twin_create_for_toy(cairo_toy_font_face_t * toy_face, cairo_font_face_t ** font_face);

/* cairo-font-face-twin-data.c */

extern const cairo_private int8_t _cairo_twin_outlines[];
extern const cairo_private uint16_t _cairo_twin_charmap[128];

/* cairo-font-options.c */

cairo_private void _cairo_font_options_init_default(cairo_font_options_t * options);

cairo_private void _cairo_font_options_init_copy(cairo_font_options_t * options, const cairo_font_options_t       * other);
cairo_private void _cairo_font_options_set_lcd_filter(cairo_font_options_t   * options, cairo_lcd_filter_t lcd_filter);
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

/* cairo-unicode.c */

cairo_private int FASTCALL _cairo_utf8_get_char_validated(const char * p, uint32_t   * unicode);
cairo_private cairo_status_t _cairo_utf8_to_ucs4(const char * str, int len, uint32_t  ** result, int * items_written);
cairo_private int FASTCALL _cairo_ucs4_to_utf8(uint32_t unicode, char * utf8);

#if CAIRO_HAS_WIN32_FONT || CAIRO_HAS_QUARTZ_FONT || CAIRO_HAS_PDF_OPERATORS
	#define CAIRO_HAS_UTF8_TO_UTF16 1
#endif
#if CAIRO_HAS_UTF8_TO_UTF16
	cairo_private cairo_status_t _cairo_utf8_to_utf16(const char * str, int len, uint16_t  ** result, int * items_written);
#endif
cairo_private void _cairo_matrix_multiply(cairo_matrix_t * r, const cairo_matrix_t * a, const cairo_matrix_t * b);

/* cairo-observer.c */

cairo_private void FASTCALL _cairo_observers_notify(cairo_list_t * observers, void * arg);

/* Avoid unnecessary PLT entries.  */
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

#include "cairo-mutex-impl-private.h"
#include "cairo-mutex-type-private.h"
#include "cairo-mutex-private.h"
#include "cairo-scaled-font-private.h"
//
//#include "cairo-line-inline.h"
//#include "cairo-line-private.h"
//
CAIRO_BEGIN_DECLS
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
#define _cairo_realloc_ab(ptr, a, size) ((size) && (uint)(a) >= INT32_MAX / (uint)(size) ? NULL : realloc(ptr, (uint)(a) * (uint)(size)))
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
cairo_private cairo_surface_t * _cairo_analysis_surface_create(cairo_surface_t * target);
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
	return ((box->p1.x & CAIRO_FIXED_FRAC_MASK) << 24 |
	    (box->p1.y & CAIRO_FIXED_FRAC_MASK) << 16 |
	    (box->p2.x & CAIRO_FIXED_FRAC_MASK) << 8 |
	    (box->p2.y & CAIRO_FIXED_FRAC_MASK) << 0) == 0;
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

static inline cairo_bool_t _cairo_path_fixed_fill_is_empty(const cairo_path_fixed_t * path)
{
	return path->fill_is_empty;
}

static inline cairo_bool_t _cairo_path_fixed_fill_is_rectilinear(const cairo_path_fixed_t * path)
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

static inline cairo_bool_t _cairo_path_fixed_stroke_is_rectilinear(const cairo_path_fixed_t * path)
{
	return path->stroke_is_rectilinear;
}

static inline cairo_bool_t _cairo_path_fixed_fill_maybe_region(const cairo_path_fixed_t * path)
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
#include "cairo-clip-private.h"
#include "cairo-clip-inline.h"
#include "cairo-surface-clipper-private.h"
#include "cairo-freed-pool-private.h"
#include "cairo-gstate-private.h"
#include "cairo-pattern-private.h"
#include "cairo-pattern-inline.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-freelist-type-private.h"
#include "cairo-freelist-private.h"
#include "cairo-rtree-private.h"
#include "cairo-combsort-inline.h"
#include "cairo-traps-private.h"
#include "cairo-device-private.h"
#include "cairo-output-stream-private.h"
#include "cairo-compositor-private.h"
#include "cairo-spans-compositor-private.h"
#include "cairo-tristrip-private.h"
#include "cairo-paginated-private.h"
#include "cairo-paginated-surface-private.h"
#include "cairo-recording-surface-private.h"
#include "cairo-recording-surface-inline.h"
#include "cairo-surface-observer-private.h"
#include "cairo-surface-observer-inline.h"
#include "cairo-surface-snapshot-private.h"
#include "cairo-surface-snapshot-inline.h"
#include "cairo-surface-subsurface-private.h"
#include "cairo-surface-subsurface-inline.h"
#include "cairo-surface-wrapper-private.h"
#include "cairo-surface-fallback-private.h"
#include "cairo-image-info-private.h"
#include "cairo-type1-private.h"
#include "cairo-scaled-font-subsets-private.h"
#include "cairo-truetype-subset-private.h"
#include "cairo-pdf.h"
#include "cairo-pdf-operators-private.h"
#include "cairo-pdf-surface-private.h"
#include "cairo-pdf-shading-private.h"
#include "cairo-type3-glyph-surface-private.h"
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
cairo_private void _cairo_region_init(cairo_region_t * region);
cairo_private void _cairo_region_init_rectangle(cairo_region_t * region, const CairoIRect * rectangle);
cairo_private void _cairo_region_fini(cairo_region_t * region);
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
