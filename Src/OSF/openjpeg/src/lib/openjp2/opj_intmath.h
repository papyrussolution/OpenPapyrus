/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef OPJ_INTMATH_H
#define OPJ_INTMATH_H
/**
   @file opj_intmath.h
   @brief Implementation of operations on integers (INT)

   The functions in OPJ_INTMATH.H have for goal to realize operations on integers.
 */

/** @defgroup OPJ_INTMATH OPJ_INTMATH - Implementation of operations on integers */
/*@{*/

/** @name Exported functions (see also openjpeg.h) */
/*@{*/
/**
   Get the minimum of two integers
   @return Returns a if a < b else b
 */
static INLINE int32_t opj_int_min(int32_t a, int32_t b) { return a < b ? a : b; }
/**
   Get the minimum of two integers
   @return Returns a if a < b else b
 */
static INLINE uint32_t opj_uint_min(uint32_t a, uint32_t b) { return a < b ? a : b; }
/**
   Get the maximum of two integers
   @return Returns a if a > b else b
 */
static INLINE int32_t opj_int_max(int32_t a, int32_t b) { return (a > b) ? a : b; }
/**
   Get the maximum of two integers
   @return Returns a if a > b else b
 */
static INLINE uint32_t opj_uint_max(uint32_t a, uint32_t b) { return (a > b) ? a : b; }
/**
   Get the saturated sum of two unsigned integers
   @return Returns saturated sum of a+b
 */
static INLINE uint32_t opj_uint_adds(uint32_t a, uint32_t b)
{
	uint64 sum = (uint64)a + (uint64)b;
	return (uint32_t)(-(int32_t)(sum >> 32)) | (uint32_t)sum;
}
/**
   Get the saturated difference of two unsigned integers
   @return Returns saturated sum of a-b
 */
static INLINE uint32_t opj_uint_subs(uint32_t a, uint32_t b) { return (a >= b) ? a - b : 0; }
/**
   Clamp an integer inside an interval
   @return
   <ul>
   <li>Returns a if (min < a < max)
   <li>Returns max if (a > max)
   <li>Returns min if (a < min)
   </ul>
 */
static INLINE int32_t opj_int_clamp(int32_t a, int32_t min, int32_t max)
{
	if(a < min) {
		return min;
	}
	if(a > max) {
		return max;
	}
	return a;
}
/**
   Clamp an integer inside an interval
   @return
   <ul>
   <li>Returns a if (min < a < max)
   <li>Returns max if (a > max)
   <li>Returns min if (a < min)
   </ul>
 */
static INLINE int64 opj_int64_clamp(int64 a, int64 min, int64 max)
{
	if(a < min) {
		return min;
	}
	if(a > max) {
		return max;
	}
	return a;
}
/**
   @return Get absolute value of integer
 */
static INLINE int32_t opj_int_abs(int32_t a)
{
	return a < 0 ? -a : a;
}
/**
   Divide an integer and round upwards
   @return Returns a divided by b
 */
static INLINE int32_t opj_int_ceildiv(int32_t a, int32_t b)
{
	assert(b);
	return (int32_t)(((int64)a + b - 1) / b);
}
/**
   Divide an integer and round upwards
   @return Returns a divided by b
 */
static INLINE uint32_t  opj_uint_ceildiv(uint32_t a, uint32_t b)
{
	assert(b);
	return (uint32_t)(((uint64)a + b - 1) / b);
}
/**
   Divide an integer by a power of 2 and round upwards
   @return Returns a divided by 2^b
 */
static INLINE int32_t opj_int_ceildivpow2(int32_t a, int32_t b)
{
	return (int32_t)((a + ((int64)1 << b) - 1) >> b);
}
/**
   Divide a 64bits integer by a power of 2 and round upwards
   @return Returns a divided by 2^b
 */
static INLINE int32_t opj_int64_ceildivpow2(int64 a, int32_t b)
{
	return (int32_t)((a + ((int64)1 << b) - 1) >> b);
}
/**
   Divide an integer by a power of 2 and round upwards
   @return Returns a divided by 2^b
 */
static INLINE uint32_t opj_uint_ceildivpow2(uint32_t a, uint32_t b)
{
	return (uint32_t)((a + ((uint64)1U << b) - 1U) >> b);
}
/**
   Divide an integer by a power of 2 and round downwards
   @return Returns a divided by 2^b
 */
static INLINE int32_t opj_int_floordivpow2(int32_t a, int32_t b)
{
	return a >> b;
}
/**
   Divide an integer by a power of 2 and round downwards
   @return Returns a divided by 2^b
 */
static INLINE uint32_t opj_uint_floordivpow2(uint32_t a, uint32_t b)
{
	return a >> b;
}
/**
   Get logarithm of an integer and round downwards
   @return Returns log2(a)
 */
static INLINE int32_t opj_int_floorlog2(int32_t a)
{
	int32_t l;
	for(l = 0; a > 1; l++) {
		a >>= 1;
	}
	return l;
}
/**
   Get logarithm of an integer and round downwards
   @return Returns log2(a)
 */
static INLINE uint32_t  opj_uint_floorlog2(uint32_t a)
{
	uint32_t l;
	for(l = 0; a > 1; ++l) {
		a >>= 1;
	}
	return l;
}

/**
   Multiply two fixed-precision rational numbers.
   @param a
   @param b
   @return Returns a * b
 */
static INLINE int32_t opj_int_fix_mul(int32_t a, int32_t b)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400) && !defined(__INTEL_COMPILER) && defined(_M_IX86)
	int64 temp = __emul(a, b);
#else
	int64 temp = (int64)a * (int64)b;
#endif
	temp += 4096;
	assert((temp >> 13) <= (int64)0x7FFFFFFF);
	assert((temp >> 13) >= (-(int64)0x7FFFFFFF - (int64)1));
	return (int32_t)(temp >> 13);
}

static INLINE int32_t opj_int_fix_mul_t1(int32_t a, int32_t b)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400) && !defined(__INTEL_COMPILER) && defined(_M_IX86)
	int64 temp = __emul(a, b);
#else
	int64 temp = (int64)a * (int64)b;
#endif
	temp += 4096;
	assert((temp >> (13 + 11 - T1_NMSEDEC_FRACBITS)) <= (int64)0x7FFFFFFF);
	assert((temp >> (13 + 11 - T1_NMSEDEC_FRACBITS)) >= (-(int64)0x7FFFFFFF -
	    (int64)1));
	return (int32_t)(temp >> (13 + 11 - T1_NMSEDEC_FRACBITS));
}

/**
   Addition two signed integers with a wrap-around behaviour.
   Assumes complement-to-two signed integers.
   @param a
   @param b
   @return Returns a + b
 */
static INLINE int32_t opj_int_add_no_overflow(int32_t a, int32_t b)
{
	void* pa = &a;
	void* pb = &b;
	uint32_t* upa = (uint32_t*)pa;
	uint32_t* upb = (uint32_t*)pb;
	uint32_t ures = *upa + *upb;
	void* pures = &ures;
	int32_t* ipres = (int32_t*)pures;
	return *ipres;
}

/**
   Subtract two signed integers with a wrap-around behaviour.
   Assumes complement-to-two signed integers.
   @param a
   @param b
   @return Returns a - b
 */
static INLINE int32_t opj_int_sub_no_overflow(int32_t a, int32_t b)
{
	void* pa = &a;
	void* pb = &b;
	uint32_t* upa = (uint32_t*)pa;
	uint32_t* upb = (uint32_t*)pb;
	uint32_t ures = *upa - *upb;
	void* pures = &ures;
	int32_t* ipres = (int32_t*)pures;
	return *ipres;
}
/*@}*/
/*@}*/
#endif /* OPJ_INTMATH_H */
