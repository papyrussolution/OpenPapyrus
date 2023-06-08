/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2004 Keith Packard
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
 * The Initial Developer of the Original Code is Keith Packard
 *
 * Contributor(s): Keith R. Packard <keithp@keithp.com>
 */
#include "cairoint.h"
#pragma hdrstop

#define uint64_lo32(i)  ((i) & 0xffffffff)
#define uint64_hi32(i)  ((i) >> 32)
#define uint64_lo(i)    ((i) & 0xffffffff)
#define uint64_hi(i)    ((i) >> 32)
#define uint64_shift32(i)   ((i) << 32)
#define uint64_carry32  (((uint64)1) << 32)

#define _cairo_uint32s_to_uint64(h, l) ((uint64)(h) << 32 | (l))

#if HAVE_UINT128_T
cairo_uquorem128_t _cairo_uint128_divrem(cairo_uint128_t num, cairo_uint128_t den)
{
	cairo_uquorem128_t qr;
	qr.quo = num / den;
	qr.rem = num % den;
	return qr;
}
#else

cairo_uint128_t _cairo_uint32_to_uint128(uint32 i)
{
	cairo_uint128_t q;
	q.lo = _cairo_uint32_to_uint64(i);
	q.hi = _cairo_uint32_to_uint64(0);
	return q;
}

cairo_int128_t _cairo_int32_to_int128(int32 i)
{
	cairo_int128_t q;
	q.lo = _cairo_int32_to_int64(i);
	q.hi = _cairo_int32_to_int64(i < 0 ? -1 : 0);
	return q;
}

cairo_uint128_t _cairo_uint64_to_uint128(uint64 i)
{
	cairo_uint128_t q;
	q.lo = i;
	q.hi = _cairo_uint32_to_uint64(0);
	return q;
}

cairo_int128_t _cairo_int64_to_int128(int64 i)
{
	cairo_int128_t q;
	q.lo = i;
	q.hi = _cairo_int32_to_int64(_cairo_int64_negative(i) ? -1 : 0);
	return q;
}

cairo_uint128_t _cairo_uint128_add(cairo_uint128_t a, cairo_uint128_t b)
{
	cairo_uint128_t s;
	s.hi = _cairo_uint64_add(a.hi, b.hi);
	s.lo = _cairo_uint64_add(a.lo, b.lo);
	if(_cairo_uint64_lt(s.lo, a.lo))
		s.hi = _cairo_uint64_add(s.hi, _cairo_uint32_to_uint64(1));
	return s;
}

cairo_uint128_t _cairo_uint128_sub(cairo_uint128_t a, cairo_uint128_t b)
{
	cairo_uint128_t s;
	s.hi = _cairo_uint64_sub(a.hi, b.hi);
	s.lo = _cairo_uint64_sub(a.lo, b.lo);
	if(_cairo_uint64_gt(s.lo, a.lo))
		s.hi = _cairo_uint64_sub(s.hi, _cairo_uint32_to_uint64(1));
	return s;
}

cairo_uint128_t _cairo_uint64x64_128_mul(uint64 a, uint64 b)
{
	cairo_uint128_t s;
	uint32 al = uint64_lo32(a);
	uint32 ah = uint64_hi32(a);
	uint32 bl = uint64_lo32(b);
	uint32 bh = uint64_hi32(b);
	uint64 r0 = _cairo_uint32x32_64_mul(al, bl);
	uint64 r1 = _cairo_uint32x32_64_mul(al, bh);
	uint64 r2 = _cairo_uint32x32_64_mul(ah, bl);
	uint64 r3 = _cairo_uint32x32_64_mul(ah, bh);
	r1 = _cairo_uint64_add(r1, uint64_hi(r0)); /* no carry possible */
	r1 = _cairo_uint64_add(r1, r2); /* but this can carry */
	if(_cairo_uint64_lt(r1, r2))                /* check */
		r3 = _cairo_uint64_add(r3, uint64_carry32);
	s.hi = _cairo_uint64_add(r3, uint64_hi(r1));
	s.lo = _cairo_uint64_add(uint64_shift32(r1), uint64_lo(r0));
	return s;
}

cairo_int128_t _cairo_int64x64_128_mul(int64 a, int64 b)
{
	cairo_int128_t s = _cairo_uint64x64_128_mul(_cairo_int64_to_uint64(a), _cairo_int64_to_uint64(b));
	if(_cairo_int64_negative(a))
		s.hi = _cairo_uint64_sub(s.hi, _cairo_int64_to_uint64(b));
	if(_cairo_int64_negative(b))
		s.hi = _cairo_uint64_sub(s.hi, _cairo_int64_to_uint64(a));
	return s;
}

cairo_uint128_t _cairo_uint128_mul(cairo_uint128_t a, cairo_uint128_t b)
{
	cairo_uint128_t s = _cairo_uint64x64_128_mul(a.lo, b.lo);
	s.hi = _cairo_uint64_add(s.hi, _cairo_uint64_mul(a.lo, b.hi));
	s.hi = _cairo_uint64_add(s.hi, _cairo_uint64_mul(a.hi, b.lo));
	return s;
}

cairo_uint128_t _cairo_uint128_lsl(cairo_uint128_t a, int shift)
{
	if(shift >= 64) {
		a.hi = a.lo;
		a.lo = _cairo_uint32_to_uint64(0);
		shift -= 64;
	}
	if(shift) {
		a.hi = _cairo_uint64_add(_cairo_uint64_lsl(a.hi, shift), _cairo_uint64_rsl(a.lo, (64 - shift)));
		a.lo = _cairo_uint64_lsl(a.lo, shift);
	}
	return a;
}

cairo_uint128_t _cairo_uint128_rsl(cairo_uint128_t a, int shift)
{
	if(shift >= 64) {
		a.lo = a.hi;
		a.hi = _cairo_uint32_to_uint64(0);
		shift -= 64;
	}
	if(shift) {
		a.lo = _cairo_uint64_add(_cairo_uint64_rsl(a.lo, shift), _cairo_uint64_lsl(a.hi, (64 - shift)));
		a.hi = _cairo_uint64_rsl(a.hi, shift);
	}
	return a;
}

cairo_uint128_t _cairo_uint128_rsa(cairo_int128_t a, int shift)
{
	if(shift >= 64) {
		a.lo = a.hi;
		a.hi = _cairo_uint64_rsa(a.hi, 64-1);
		shift -= 64;
	}
	if(shift) {
		a.lo = _cairo_uint64_add(_cairo_uint64_rsl(a.lo, shift), _cairo_uint64_lsl(a.hi, (64 - shift)));
		a.hi = _cairo_uint64_rsa(a.hi, shift);
	}
	return a;
}

int _cairo_uint128_lt(cairo_uint128_t a, cairo_uint128_t b)
{
	return (_cairo_uint64_lt(a.hi, b.hi) || (_cairo_uint64_eq(a.hi, b.hi) && _cairo_uint64_lt(a.lo, b.lo)));
}

int _cairo_int128_lt(cairo_int128_t a, cairo_int128_t b)
{
	if(_cairo_int128_negative(a) && !_cairo_int128_negative(b))
		return 1;
	if(!_cairo_int128_negative(a) && _cairo_int128_negative(b))
		return 0;
	return _cairo_uint128_lt(a, b);
}

int _cairo_uint128_cmp(cairo_uint128_t a, cairo_uint128_t b)
{
	const int cmp = _cairo_uint64_cmp(a.hi, b.hi);
	return cmp ? cmp : _cairo_uint64_cmp(a.lo, b.lo);
}

int _cairo_int128_cmp(cairo_int128_t a, cairo_int128_t b)
{
	if(_cairo_int128_negative(a) && !_cairo_int128_negative(b))
		return -1;
	if(!_cairo_int128_negative(a) && _cairo_int128_negative(b))
		return 1;
	return _cairo_uint128_cmp(a, b);
}

int _cairo_uint128_eq(cairo_uint128_t a, cairo_uint128_t b)
{
	return (_cairo_uint64_eq(a.hi, b.hi) && _cairo_uint64_eq(a.lo, b.lo));
}

#define _cairo_msbset64(q)  (q & ((uint64)1 << 63))

cairo_uquorem128_t _cairo_uint128_divrem(cairo_uint128_t num, cairo_uint128_t den)
{
	cairo_uquorem128_t qr;
	cairo_uint128_t quo;
	cairo_uint128_t bit = _cairo_uint32_to_uint128(1);
	/* normalize to make den >= num, but not overflow */
	while(_cairo_uint128_lt(den, num) && !_cairo_msbset64(den.hi)) {
		bit = _cairo_uint128_lsl(bit, 1);
		den = _cairo_uint128_lsl(den, 1);
	}
	quo = _cairo_uint32_to_uint128(0);
	/* generate quotient, one bit at a time */
	while(_cairo_uint128_ne(bit, _cairo_uint32_to_uint128(0))) {
		if(_cairo_uint128_le(den, num)) {
			num = _cairo_uint128_sub(num, den);
			quo = _cairo_uint128_add(quo, bit);
		}
		bit = _cairo_uint128_rsl(bit, 1);
		den = _cairo_uint128_rsl(den, 1);
	}
	qr.quo = quo;
	qr.rem = num;
	return qr;
}

cairo_uint128_t _cairo_uint128_negate(cairo_uint128_t a)
{
	a.lo = _cairo_uint64_not(a.lo);
	a.hi = _cairo_uint64_not(a.hi);
	return _cairo_uint128_add(a, _cairo_uint32_to_uint128(1));
}

cairo_uint128_t _cairo_uint128_not(cairo_uint128_t a)
{
	a.lo = _cairo_uint64_not(a.lo);
	a.hi = _cairo_uint64_not(a.hi);
	return a;
}

#endif /* !HAVE_UINT128_T */

cairo_quorem128_t _cairo_int128_divrem(cairo_int128_t num, cairo_int128_t den)
{
	int num_neg = _cairo_int128_negative(num);
	int den_neg = _cairo_int128_negative(den);
	cairo_uquorem128_t uqr;
	cairo_quorem128_t qr;
	if(num_neg)
		num = _cairo_int128_negate(num);
	if(den_neg)
		den = _cairo_int128_negate(den);
	uqr = _cairo_uint128_divrem(num, den);
	if(num_neg)
		qr.rem = _cairo_int128_negate(uqr.rem);
	else
		qr.rem = uqr.rem;
	if(num_neg != den_neg)
		qr.quo = _cairo_int128_negate(uqr.quo);
	else
		qr.quo = uqr.quo;
	return qr;
}
/**
 * _cairo_uint_96by64_32x64_divrem:
 *
 * Compute a 32 bit quotient and 64 bit remainder of a 96 bit unsigned
 * dividend and 64 bit divisor.  If the quotient doesn't fit into 32
 * bits then the returned remainder is equal to the divisor, and the
 * quotient is the largest representable 64 bit integer.  It is an
 * error to call this function with the high 32 bits of @num being
 * non-zero.
 **/
cairo_uquorem64_t _cairo_uint_96by64_32x64_divrem(cairo_uint128_t num, uint64 den)
{
	cairo_uquorem64_t result;
	uint64 B = _cairo_uint32s_to_uint64(1, 0);
	/* These are the high 64 bits of the *96* bit numerator.  We're
	 * going to represent the numerator as xB + y, where x is a 64,
	 * and y is a 32 bit number. */
	uint64 x = _cairo_uint128_to_uint64(_cairo_uint128_rsl(num, 32));

	/* Initialise the result to indicate overflow. */
	result.quo = _cairo_uint32s_to_uint64(-1U, -1U);
	result.rem = den;

	/* Don't bother if the quotient is going to overflow. */
	if(_cairo_uint64_ge(x, den)) {
		return /* overflow */ result;
	}
	if(_cairo_uint64_lt(x, B)) {
		/* When the final quotient is known to fit in 32 bits, then
		 * num < 2^64 if and only if den < 2^32. */
		return _cairo_uint64_divrem(_cairo_uint128_to_uint64(num), den);
	}
	else {
		/* Denominator is >= 2^32. the numerator is >= 2^64, and the
		 * division won't overflow: need two divrems.  Write the
		 * numerator and denominator as
		 *
		 *	num = xB + y		x : 64 bits, y : 32 bits
		 *	den = uB + v		u, v : 32 bits
		 */
		uint32 y = _cairo_uint128_to_uint32(num);
		uint32 u = uint64_hi32(den);
		uint32 v = _cairo_uint64_to_uint32(den);

		/* Compute a lower bound approximate quotient of num/den
		 * from x/(u+1).  Then we have
		 *
		 * x	= q(u+1) + r	; q : 32 bits, r <= u : 32 bits.
		 *
		 * xB + y	= q(u+1)B	+ (rB+y)
		 *		= q(uB + B + v - v) + (rB+y)
		 *		= q(uB + v)	+ qB - qv + (rB+y)
		 *		= q(uB + v)	+ q(B-v) + (rB+y)
		 *
		 * The true quotient of num/den then is q plus the
		 * contribution of q(B-v) + (rB+y).  The main contribution
		 * comes from the term q(B-v), with the term (rB+y) only
		 * contributing at most one part.
		 *
		 * The term q(B-v) must fit into 64 bits, since q fits into 32
		 * bits on account of being a lower bound to the true
		 * quotient, and as B-v <= 2^32, we may safely use a single
		 * 64/64 bit division to find its contribution. */

		cairo_uquorem64_t quorem;
		uint64 remainder; /* will contain final remainder */
		uint32 quotient; /* will contain final quotient. */
		uint32 q;
		uint32 r;
		// Approximate quotient by dividing the high 64 bits of num by u+1. Watch out for overflow of u+1.
		if(u+1) {
			quorem = _cairo_uint64_divrem(x, _cairo_uint32_to_uint64(u+1));
			q = _cairo_uint64_to_uint32(quorem.quo);
			r = _cairo_uint64_to_uint32(quorem.rem);
		}
		else {
			q = uint64_hi32(x);
			r = _cairo_uint64_to_uint32(x);
		}
		quotient = q;
		// Add the main term's contribution to quotient.  Note B-v = -v as an uint32 (unless v = 0) */
		if(v)
			quorem = _cairo_uint64_divrem(_cairo_uint32x32_64_mul(q, -v), den);
		else
			quorem = _cairo_uint64_divrem(_cairo_uint32s_to_uint64(q, 0), den);
		quotient += _cairo_uint64_to_uint32(quorem.quo);
		// Add the contribution of the subterm and start computing the true remainder.
		remainder = _cairo_uint32s_to_uint64(r, y);
		if(_cairo_uint64_ge(remainder, den)) {
			remainder = _cairo_uint64_sub(remainder, den);
			quotient++;
		}
		/* Add the contribution of the main term's remainder. The
		 * funky test here checks that remainder + main_rem >= den,
		 * taking into account overflow of the addition. */
		remainder = _cairo_uint64_add(remainder, quorem.rem);
		if(_cairo_uint64_ge(remainder, den) || _cairo_uint64_lt(remainder, quorem.rem)) {
			remainder = _cairo_uint64_sub(remainder, den);
			quotient++;
		}
		result.quo = _cairo_uint32_to_uint64(quotient);
		result.rem = remainder;
	}
	return result;
}

cairo_quorem64_t _cairo_int_96by64_32x64_divrem(cairo_int128_t num, int64 den)
{
	int num_neg = _cairo_int128_negative(num);
	int den_neg = _cairo_int64_negative(den);
	uint64 nonneg_den;
	cairo_uquorem64_t uqr;
	cairo_quorem64_t qr;
	if(num_neg)
		num = _cairo_int128_negate(num);
	if(den_neg)
		nonneg_den = _cairo_int64_negate(den);
	else
		nonneg_den = den;
	uqr = _cairo_uint_96by64_32x64_divrem(num, nonneg_den);
	if(_cairo_uint64_eq(uqr.rem, nonneg_den)) {
		/* bail on overflow. */
		qr.quo = _cairo_uint32s_to_uint64(0x7FFFFFFF, -1U);
		qr.rem = den;
		return qr;
	}
	if(num_neg)
		qr.rem = _cairo_int64_negate(uqr.rem);
	else
		qr.rem = uqr.rem;
	if(num_neg != den_neg)
		qr.quo = _cairo_int64_negate(uqr.quo);
	else
		qr.quo = uqr.quo;
	return qr;
}
