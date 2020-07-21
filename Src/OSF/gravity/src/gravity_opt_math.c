//  gravity_opt_math.c
//  gravity
//
//  Created by Marco Bambini on 14/08/2017.
//  Copyright © 2017 CreoLabs. All rights reserved.
//
//  Based on: https://www.w3schools.com/jsref/jsref_obj_math.asp
#include <gravity_.h>
#pragma hdrstop

#if GRAVITY_ENABLE_DOUBLE
#define SIN                         sin
#define COS                         cos
#define TAN                         tan
#define ASIN                        asin
#define ACOS                        acos
#define ATAN                        atan
#define ATAN2                       atan2
#define CEIL                        ceil
#define FLOOR                       floor
#define ROUND                       round
#define LOG                         log
#define POW                         pow
#define EXP                         exp
#define SQRT                        sqrt
#define CBRT                        cbrt
#else
#define SIN                         sinf
#define COS                         cosf
#define TAN                         tanf
#define ASIN                        asinf
#define ACOS                        acosf
#define ATAN                        atanf
#define ATAN2                       atan2
#define CEIL                        ceilf
#define FLOOR                       floorf
#define ROUND                       roundf
#define LOG                         logf
#define POW                         powf
#define EXP                         expf
#define SQRT                        sqrtf
#define CBRT                        cbrtf
#endif

// MARK: - Implementation -

// returns the arctangent of the quotient of its arguments
static bool math_atan2(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs != 3) return vm->ReturnUndefined(rindex);
	GravityValue value = args[1];
	GravityValue value2 = args[2];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	gravity_float_t n2;
	if(value2.IsInt()) 
		n2 = (gravity_float_t)value2.n;
	else if(value2.IsFloat()) 
		n2 = (gravity_float_t)value2.f;
	else 
		return vm->ReturnUndefined(rindex);
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)ATAN2((gravity_float_t)value.n, n2);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}

	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)ATAN2((gravity_float_t)value.f, n2);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}

	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the cosine of x (x is in radians)
static bool math_cos(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)COS((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)COS((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the value of Ex
static bool math_exp(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)EXP((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)EXP((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns x, rounded downwards to the nearest integer
static bool math_floor(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)FLOOR((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)FLOOR((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

static int gcf(int x, int y) 
{
	if(x == 0) {
		return y;
	}
	while(y != 0) {
		if(x > y) {
			x = x - y;
		}
		else {
			y = y - x;
		}
	}
	return x;
}

static bool math_gcf(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs < 3) 
		return vm->ReturnUndefined(rindex);
	for(int i = 1; i < nargs; ++i) {
		if(!args[i].IsInt()) 
			return vm->ReturnUndefined(rindex);
	}
	int gcf_value = (int)args[1].n;
	for(int i = 1; i < nargs-1; ++i) {
		gcf_value = gcf(gcf_value, (int)args[i+1].n);
	}
	return vm->ReturnValue(GravityValue::from_int(gcf_value), rindex);
}

static int lcm(int x, int y) 
{
	return x*y/gcf(x, y);
}

static bool math_lcm(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs < 3) 
		return vm->ReturnError(rindex, "2 or more arguments expected");
	for(int i = 1; i < nargs; ++i) {
		if(!args[i].IsInt()) 
			return vm->ReturnUndefined(rindex);
	}
	int lcm_value = (int)args[1].n;
	for(int i = 1; i < nargs-1; ++i) {
		lcm_value = lcm(lcm_value, (int)args[i+1].n);
	}
	return vm->ReturnValue(GravityValue::from_int(lcm_value), rindex);
}

// returns the linear interpolation from a to b of value t
static bool math_lerp(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	// three arguments are required
	if(nargs < 4) 
		return vm->ReturnUndefined(rindex);
	gravity_float_t a, b, t;
	GravityValue value = args[1];
	if(value.IsInt()) {
		a = (gravity_float_t)value.n;
	}
	else {
		if(value.IsFloat()) {
			a = value.f;
		}
		else {
			return vm->ReturnUndefined(rindex);
		}
	}
	value = args[2];
	if(value.IsInt()) {
		b = (gravity_float_t)value.n;
	}
	else {
		if(value.IsFloat()) {
			b = value.f;
		}
		else {
			return vm->ReturnUndefined(rindex);
		}
	}
	value = args[3];
	if(value.IsInt()) {
		t = (gravity_float_t)value.n;
	}
	else {
		if(value.IsFloat()) {
			t = value.f;
		}
		else {
			return vm->ReturnUndefined(rindex);
		}
	}
	gravity_float_t lerp = a+(b-a)*t;
	return vm->ReturnValue(GravityValue::from_float(lerp), rindex);
}

// returns the natural logarithm (base E) of x
static bool math_log(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the base 10 logarithm of x
static bool math_log10(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.n)/(gravity_float_t)LOG((gravity_float_t)10);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.f)/(gravity_float_t)LOG((gravity_float_t)10);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the logarithm (base x) of y
static bool math_logx(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue base = args[1];
	GravityValue value = args[2];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt() && base.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.n)/(gravity_float_t)LOG((gravity_float_t)base.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsInt() && base.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.n)/(gravity_float_t)LOG((gravity_float_t)base.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat() && base.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.f)/(gravity_float_t)LOG((gravity_float_t)base.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat() && base.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)LOG((gravity_float_t)value.f)/(gravity_float_t)LOG((gravity_float_t)base.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the number with the highest value
static bool math_max(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs == 1) 
		return vm->ReturnNull(rindex);
	gravity_float_t n = -GRAVITY_FLOAT_MAX;
	uint16 maxindex = 1;
	bool found = false;
	for(uint16 i = 1; i<nargs; ++i) {
		GravityValue value = args[i];
		if(value.IsInt()) {
			found = true;
			if((gravity_float_t)value.n > n) {
				n = (gravity_float_t)value.n;
				maxindex = i;
			}
		}
		else if(value.IsFloat()) {
			found = true;
			if(value.f > n) {
				n = value.f;
				maxindex = i;
			}
		}
		else continue;
	}
	if(!found) {
		return vm->ReturnNull(rindex);
	}
	return vm->ReturnValue(args[maxindex], rindex);
}

// returns the number with the lowest value
static bool math_min(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs == 1) 
		return vm->ReturnNull(rindex);
	gravity_float_t n = GRAVITY_FLOAT_MAX;
	uint16 minindex = 1;
	bool found = false;
	for(uint16 i = 1; i<nargs; ++i) {
		GravityValue value = args[i];
		if(value.IsInt()) {
			found = true;
			if((gravity_float_t)value.n < n) {
				n = (gravity_float_t)value.n;
				minindex = i;
			}
		}
		else if(value.IsFloat()) {
			found = true;
			if(value.f < n) {
				n = value.f;
				minindex = i;
			}
		}
		else continue;
	}
	if(!found) {
		return vm->ReturnNull(rindex);
	}
	return vm->ReturnValue(args[minindex], rindex);
}

// returns the value of x to the power of y
static bool math_pow(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	if(nargs != 3) 
		return vm->ReturnUndefined(rindex);
	GravityValue value = args[1];
	GravityValue value2 = args[2];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	gravity_float_t n2;
	if(value2.IsInt()) 
		n2 = (gravity_float_t)value2.n;
	else if(value2.IsFloat()) 
		n2 = (gravity_float_t)value2.f;
	else 
		return vm->ReturnUndefined(rindex);
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)POW((gravity_float_t)value.n, n2);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}

	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)POW((gravity_float_t)value.f, n2);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}

	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// rounds x to the nearest integer
static bool math_round(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)ROUND((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		// check for extra parameters
		gravity_int_t ndigits = 0;
		bool toString = false;
		if(nargs >= 3 && args[2].IsInt()) {
			GravityValue extra = args[2];
			if(extra.GetInt() > 0) 
				ndigits = extra.GetInt();
			if(ndigits > FLOAT_MAX_DECIMALS) 
				ndigits = FLOAT_MAX_DECIMALS;
		}
		if(nargs >= 4 && args[3].IsBool()) {
			toString = VALUE_AS_BOOL(args[3]);
		}
		if(ndigits) {
			double d = pow(10.0, (double)ndigits);
			gravity_float_t f = (gravity_float_t)(ROUND((gravity_float_t)value.f * (gravity_float_t)d)) / (gravity_float_t)d;
			// convert f to string
			char buffer[512];
			snprintf(buffer, sizeof(buffer), "%f", f);

			// trunc c string to the requested ndigits
			char * p = buffer;
			while(p) {
				if(p[0] == '.') {
					++p;
					gravity_int_t n = 0;
					while(p && n < ndigits) {
						++p;
						++n;
					}
					if(p) p[0] = 0;
					break;
				}
				++p;
			}

			if(toString) {
				// force string value
				return vm->ReturnValue(gravity_zstring_to_value(vm, buffer), rindex);
			}
			// default case is to re-convert string to float value
			return vm->ReturnValue(GravityValue::from_float(strtod(buffer, NULL)), rindex);
		}
		// simpler round case
		gravity_float_t computed_value = (gravity_float_t)ROUND((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the sine of x (x is in radians)
static bool math_sin(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)SIN((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)SIN((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the square root of x
static bool math_sqrt(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)SQRT((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)SQRT((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// returns the tangent of an angle
static bool math_tan(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	GravityValue value = args[1];
	if(value.IsNull()) {
		return vm->ReturnValue(GravityValue::from_int(0), rindex);
	}
	if(value.IsInt()) {
		gravity_float_t computed_value = (gravity_float_t)TAN((gravity_float_t)value.n);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	if(value.IsFloat()) {
		gravity_float_t computed_value = (gravity_float_t)TAN((gravity_float_t)value.f);
		return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
	}
	// should be NaN
	return vm->ReturnUndefined(rindex);
}

// CONSTANTS
static bool math_PI(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(/*3.141592653589793*/SMathConst::Pi), rindex); }
static bool math_E(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(/*2.718281828459045*/SMathConst::E), rindex); }
static bool math_LN2(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(0.6931471805599453), rindex); }
static bool math_LN10(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(2.302585092994046), rindex); }
static bool math_LOG2E(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(1.4426950408889634), rindex); }
static bool math_LOG10E(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(0.4342944819032518), rindex); }
static bool math_SQRT2(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(1.4142135623730951), rindex); }
static bool math_SQRT1_2(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{ return vm->ReturnValue(GravityValue::from_float(0.7071067811865476), rindex); }

// MARK: - Random -

#if GRAVITY_ENABLE_INT64
// https://en.wikipedia.org/wiki/Linear-feedback_shift_register

/*
   64-bits Random number generator U[0,1): lfsr258
   Author: Pierre L'Ecuyer,
   Source: http://www.iro.umontreal.ca/~lecuyer/myftp/papers/tausme2.ps
   ---------------------------------------------------------
 */

/**** VERY IMPORTANT **** :
   The initial seeds y1, y2, y3, y4, y5  MUST be larger than
   1, 511, 4095, 131071 and 8388607 respectively.
 ****/

#define LFSR_GERME 123456789123456789ULL

static uint64_t lfsr258_y1 = LFSR_GERME, lfsr258_y2 = LFSR_GERME, lfsr258_y3 = LFSR_GERME, lfsr258_y4 = LFSR_GERME, lfsr258_y5 = LFSR_GERME;

static void lfsr258_init(uint64_t n) 
{
	static int lfsr258_inited = 0;
	if(!lfsr258_inited) {
		lfsr258_inited = 1;
		if(n == 0) 
			n = LFSR_GERME;
		lfsr258_y1 = n; 
		lfsr258_y2 = n; 
		lfsr258_y3 = n; 
		lfsr258_y4 = n; 
		lfsr258_y5 = n;
	}
}

static double lfsr258() 
{
	uint64_t b;
	b = ((lfsr258_y1 << 1) ^ lfsr258_y1) >> 53;
	lfsr258_y1 = ((lfsr258_y1 & 18446744073709551614UL) << 10) ^ b;
	b = ((lfsr258_y2 << 24) ^ lfsr258_y2) >> 50;
	lfsr258_y2 = ((lfsr258_y2 & 18446744073709551104UL) << 5) ^ b;
	b = ((lfsr258_y3 << 3) ^ lfsr258_y3) >> 23;
	lfsr258_y3 = ((lfsr258_y3 & 18446744073709547520UL) << 29) ^ b;
	b = ((lfsr258_y4 << 5) ^ lfsr258_y4) >> 24;
	lfsr258_y4 = ((lfsr258_y4 & 18446744073709420544UL) << 23) ^ b;
	b = ((lfsr258_y5 << 3) ^ lfsr258_y5) >> 33;
	lfsr258_y5 = ((lfsr258_y5 & 18446744073701163008UL) << 8) ^ b;
	return (lfsr258_y1 ^ lfsr258_y2 ^ lfsr258_y3 ^ lfsr258_y4 ^ lfsr258_y5) * 5.421010862427522170037264e-20;
}

#else

#define LFSR_SEED 987654321

static uint32 lfsr113_z1 = LFSR_SEED, lfsr113_z2 = LFSR_SEED, lfsr113_z3 = LFSR_SEED, lfsr113_z4 = LFSR_SEED;

static void lfsr113_init(uint32 n) 
{
	static int lfsr113_inited = 0;
	if(lfsr113_inited) return;
	lfsr113_inited = 1;
	if(n == 0) n = LFSR_SEED;
	lfsr113_z1 = n; lfsr113_z2 = n; lfsr113_z3 = n; lfsr113_z4 = n;
}

static double lfsr113() 
{
	uint32 b;
	b  = ((lfsr113_z1 << 6) ^ lfsr113_z1) >> 13;
	lfsr113_z1 = ((lfsr113_z1 & 4294967294U) << 18) ^ b;
	b  = ((lfsr113_z2 << 2) ^ lfsr113_z2) >> 27;
	lfsr113_z2 = ((lfsr113_z2 & 4294967288U) << 2) ^ b;
	b  = ((lfsr113_z3 << 13) ^ lfsr113_z3) >> 21;
	lfsr113_z3 = ((lfsr113_z3 & 4294967280U) << 7) ^ b;
	b  = ((lfsr113_z4 << 3) ^ lfsr113_z4) >> 12;
	lfsr113_z4 = ((lfsr113_z4 & 4294967168U) << 13) ^ b;
	return (lfsr113_z1 ^ lfsr113_z2 ^ lfsr113_z3 ^ lfsr113_z4) * 2.3283064365386963e-10;
}
#endif

// returns a random number between 0 and 1
static bool math_random(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
{
	// generate a random number between 0.0 and 1.0
	// and automatically call seed (if not already called)
    #if GRAVITY_ENABLE_INT64
	lfsr258_init(nanotime());
	gravity_float_t rnd = lfsr258();
    #else
	lfsr113_init((uint32)time(NULL));
	gravity_float_t rnd = lfsr113();
    #endif
	// if at least one parameter is passed
	if(nargs > 1) {
		GravityValue value1 = GravityValue::from_undefined();
		GravityValue value2 = GravityValue::from_undefined();
		// if one parameter is passed it must be Int or Float and a number between 0 and the parameter will be
		// returned
		if(nargs == 2) {
			value2 = args[1];
			if(value2.IsInt()) 
				value1 = GravityValue::from_int(0);
			if(value2.IsFloat()) 
				value1 = GravityValue::from_float(0.0);
		}
		// if two parameters are passed they must be both Int or Float and a number between parameter1 and
		// parameter2 will be returned
		if(nargs == 3) {
			value1 = args[1];
			value2 = args[2];
		}
		// at this point I should have 2 values of the same type, if not continue with the default case (and
		// ignore any extra parameter)
		if((value1.isa == value2.isa) && value1.IsInt()) {
			gravity_int_t n1 = value1.GetInt(); // min
			gravity_int_t n2 = value2.GetInt(); // max
			if(n1 == n2) 
				return vm->ReturnValue(GravityValue::from_int(n1), rindex);
			gravity_int_t n0 = (gravity_int_t)(rnd * GRAVITY_INT_MAX);
			if(n1 > n2)
				memswap(&n1, &n2, sizeof(n1));
			gravity_int_t n = (gravity_int_t)(n0 % (n2 + 1 - n1) + n1);
			return vm->ReturnValue(GravityValue::from_int(n), rindex);
		}
		if(value1.isa == value2.isa && value1.IsFloat()) {
			gravity_float_t n1 = value1.GetFloat(); // min
			gravity_float_t n2 = value2.GetFloat(); // max
			if(n1 == n2) 
				return vm->ReturnValue(GravityValue::from_float(n1), rindex);
			if(n1 > n2) {
				gravity_float_t temp = n1; n1 = n2; n2 = temp;
			}                                                  // swap numbers if min > max
			gravity_float_t diff = n2 - n1;
			gravity_float_t r = rnd * diff;
			return vm->ReturnValue(GravityValue::from_float(r + n1), rindex);
		}
	}
	return vm->ReturnValue(GravityValue::from_float(rnd), rindex); // default case is to return a float number between 0.0 and 1.0
}

// MARK: - Internals -

class GravityClassImplementation_Math : public GravityClassImplementation {
private:
	// returns the absolute value of x
	static bool math_abs(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		GravityValue value = args[1];
		if(value.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt()) {
			gravity_int_t computed_value;
		#if GRAVITY_ENABLE_INT64
			computed_value = (gravity_int_t)llabs((long long)value.n);
		#else
			computed_value = (gravity_int_t)labs((long)value.n);
		#endif
			return vm->ReturnValue(GravityValue::from_int(computed_value), rindex);
		}
		if(value.IsFloat()) {
			gravity_float_t computed_value;
		#if GRAVITY_ENABLE_DOUBLE
			computed_value = (gravity_float_t)fabs((double)value.f);
		#else
			computed_value = (gravity_float_t)fabsf((float)value.f);
		#endif
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		// should be NaN
		return vm->ReturnUndefined(rindex);
	}
	// returns the arccosine of x, in radians
	static bool math_acos(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		GravityValue value = args[1];
		if(value.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)ACOS((gravity_float_t)value.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat()) {
			gravity_float_t computed_value = (gravity_float_t)ACOS((gravity_float_t)value.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		return vm->ReturnUndefined(rindex); // should be NaN
	}
	// returns the arcsine of x, in radians
	static bool math_asin(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		GravityValue value = args[1];
		if(value.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)ASIN((gravity_float_t)value.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat()) {
			gravity_float_t computed_value = (gravity_float_t)ASIN((gravity_float_t)value.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		return vm->ReturnUndefined(rindex); // should be NaN
	}
	// returns the arctangent of x as a numeric value between -PI/2 and PI/2 radians
	static bool math_atan(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		GravityValue value = args[1];
		if(value.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)ATAN((gravity_float_t)value.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat()) {
			gravity_float_t computed_value = (gravity_float_t)ATAN((gravity_float_t)value.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		return vm->ReturnUndefined(rindex); // should be NaN
	}
	static bool math_cbrt(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		GravityValue value = args[1];
		if(value.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)CBRT((gravity_float_t)value.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat()) {
			gravity_float_t computed_value = (gravity_float_t)CBRT((gravity_float_t)value.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		return vm->ReturnUndefined(rindex); // should be NaN
	}
	static bool math_xrt(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		if(nargs != 3) 
			return vm->ReturnUndefined(rindex);
		GravityValue base = args[1];
		GravityValue value = args[2];
		if(value.IsNull() || base.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt() && base.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)pow((gravity_float_t)value.n, 1.0/base.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsInt() && base.IsFloat()) {
			gravity_float_t computed_value = (gravity_float_t)pow((gravity_float_t)value.n, 1.0/base.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat() && base.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)pow((gravity_float_t)value.f, 1.0/base.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat() && base.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)pow((gravity_float_t)value.f, 1.0/base.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		return vm->ReturnUndefined(rindex); // should be NaN
	}
	// returns x, rounded upwards to the nearest integer
	static bool math_ceil(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		GravityValue value = args[1];
		if(value.IsNull()) {
			return vm->ReturnValue(GravityValue::from_int(0), rindex);
		}
		if(value.IsInt()) {
			gravity_float_t computed_value = (gravity_float_t)CEIL((gravity_float_t)value.n);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		if(value.IsFloat()) {
			gravity_float_t computed_value = (gravity_float_t)CEIL((gravity_float_t)value.f);
			return vm->ReturnValue(GravityValue::from_float(computed_value), rindex);
		}
		return vm->ReturnUndefined(rindex); // should be NaN
	}
public:
	GravityClassImplementation_Math() : GravityClassImplementation(GRAVITY_CLASS_MATH_NAME, fCore)
	{
	}
	virtual int Bind(gravity_class_t * pMeta)
	{
		int    ok = 1;
		//gravity_c_internal
		gravity_class_bind_outerproc(pMeta, "abs", math_abs);
		gravity_class_bind_outerproc(pMeta, "acos", math_acos);
		gravity_class_bind_outerproc(pMeta, "asin", math_asin);
		gravity_class_bind_outerproc(pMeta, "atan", math_atan);
		gravity_class_bind_outerproc(pMeta, "atan2", math_atan2);
		gravity_class_bind_outerproc(pMeta, "cbrt", math_cbrt);
		gravity_class_bind_outerproc(pMeta, "xrt", math_xrt);
		gravity_class_bind_outerproc(pMeta, "ceil", math_ceil);
		gravity_class_bind_outerproc(pMeta, "cos", math_cos);
		gravity_class_bind_outerproc(pMeta, "exp", math_exp);
		gravity_class_bind_outerproc(pMeta, "floor", math_floor);
		gravity_class_bind_outerproc(pMeta, "gcf", math_gcf);
		gravity_class_bind_outerproc(pMeta, "lcm", math_lcm);
		gravity_class_bind_outerproc(pMeta, "lerp", math_lerp);
		gravity_class_bind_outerproc(pMeta, "log", math_log);
		gravity_class_bind_outerproc(pMeta, "log10", math_log10);
		gravity_class_bind_outerproc(pMeta, "logx", math_logx);
		gravity_class_bind_outerproc(pMeta, "max", math_max);
		gravity_class_bind_outerproc(pMeta, "min", math_min);
		gravity_class_bind_outerproc(pMeta, "pow", math_pow);
		gravity_class_bind_outerproc(pMeta, "random", math_random);
		gravity_class_bind_outerproc(pMeta, "round", math_round);
		gravity_class_bind_outerproc(pMeta, "sin", math_sin);
		gravity_class_bind_outerproc(pMeta, "sqrt", math_sqrt);
		gravity_class_bind_outerproc(pMeta, "tan", math_tan);

		gravity_class_bind_property_outerproc(pMeta, "PI", math_PI, 0);
		gravity_class_bind_property_outerproc(pMeta, "E", math_E, 0);
		gravity_class_bind_property_outerproc(pMeta, "LN2", math_LN2, 0);
		gravity_class_bind_property_outerproc(pMeta, "LN10", math_LN10, 0);
		gravity_class_bind_property_outerproc(pMeta, "LOG2E", math_LOG2E, 0);
		gravity_class_bind_property_outerproc(pMeta, "LOG10E", math_LOG10E, 0);
		gravity_class_bind_property_outerproc(pMeta, "SQRT2", math_SQRT2, 0);
		gravity_class_bind_property_outerproc(pMeta, "SQRT1_2", math_SQRT1_2, 0);
		return ok;
	}
	virtual void DestroyMeta(gravity_class_t * pMeta)
	{
		computed_property_free(pMeta, "PI", true);
		computed_property_free(pMeta, "E", true);
		computed_property_free(pMeta, "LN2", true);
		computed_property_free(pMeta, "LN10", true);
		computed_property_free(pMeta, "LOG2E", true);
		computed_property_free(pMeta, "LOG10E", true);
		computed_property_free(pMeta, "SQRT2", true);
		computed_property_free(pMeta, "SQRT1_2", true);
	}
};

static GravityClassImplementation_Math gravity_clsimp_math;

bool gravity_ismath_class(const gravity_class_t * c) { return (c && c == gravity_clsimp_math.P_Cls); }
const char * gravity_math_name() { return gravity_clsimp_math.P_Name/*GRAVITY_CLASS_MATH_NAME*/; }
void gravity_math_register(gravity_vm * vm)  { gravity_clsimp_math.Register(vm); }
void gravity_math_free() { gravity_clsimp_math.UnRegister(); }

#if 0 // {
static gravity_class_t * gravity_class_math = NULL;
static uint32 refcount = 0;

bool gravity_ismath_class(const gravity_class_t * c) { return (c == gravity_class_math); }
const char * gravity_math_name() { return GRAVITY_CLASS_MATH_NAME; }

static void create_optional_class() 
{
	gravity_class_math = gravity_class_new_pair(NULL, GRAVITY_CLASS_MATH_NAME, NULL, 0, 0);
	gravity_class_t * meta = gravity_class_get_meta(gravity_class_math);
	gravity_class_bind(meta, "abs", NEW_CLOSURE_VALUE(math_abs));
	gravity_class_bind(meta, "acos", NEW_CLOSURE_VALUE(math_acos));
	gravity_class_bind(meta, "asin", NEW_CLOSURE_VALUE(math_asin));
	gravity_class_bind(meta, "atan", NEW_CLOSURE_VALUE(math_atan));
	gravity_class_bind(meta, "atan2", NEW_CLOSURE_VALUE(math_atan2));
	gravity_class_bind(meta, "cbrt", NEW_CLOSURE_VALUE(math_cbrt));
	gravity_class_bind(meta, "xrt", NEW_CLOSURE_VALUE(math_xrt));
	gravity_class_bind(meta, "ceil", NEW_CLOSURE_VALUE(math_ceil));
	gravity_class_bind(meta, "cos", NEW_CLOSURE_VALUE(math_cos));
	gravity_class_bind(meta, "exp", NEW_CLOSURE_VALUE(math_exp));
	gravity_class_bind(meta, "floor", NEW_CLOSURE_VALUE(math_floor));
	gravity_class_bind(meta, "gcf", NEW_CLOSURE_VALUE(math_gcf));
	gravity_class_bind(meta, "lcm", NEW_CLOSURE_VALUE(math_lcm));
	gravity_class_bind(meta, "lerp", NEW_CLOSURE_VALUE(math_lerp));
	gravity_class_bind(meta, "log", NEW_CLOSURE_VALUE(math_log));
	gravity_class_bind(meta, "log10", NEW_CLOSURE_VALUE(math_log10));
	gravity_class_bind(meta, "logx", NEW_CLOSURE_VALUE(math_logx));
	gravity_class_bind(meta, "max", NEW_CLOSURE_VALUE(math_max));
	gravity_class_bind(meta, "min", NEW_CLOSURE_VALUE(math_min));
	gravity_class_bind(meta, "pow", NEW_CLOSURE_VALUE(math_pow));
	gravity_class_bind(meta, "random", NEW_CLOSURE_VALUE(math_random));
	gravity_class_bind(meta, "round", NEW_CLOSURE_VALUE(math_round));
	gravity_class_bind(meta, "sin", NEW_CLOSURE_VALUE(math_sin));
	gravity_class_bind(meta, "sqrt", NEW_CLOSURE_VALUE(math_sqrt));
	gravity_class_bind(meta, "tan", NEW_CLOSURE_VALUE(math_tan));

	gravity_closure_t * closure = NULL;
	closure = computed_property_create(NULL, NEW_FUNCTION(math_PI), NULL);
	gravity_class_bind(meta, "PI", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_E), NULL);
	gravity_class_bind(meta, "E", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_LN2), NULL);
	gravity_class_bind(meta, "LN2", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_LN10), NULL);
	gravity_class_bind(meta, "LN10", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_LOG2E), NULL);
	gravity_class_bind(meta, "LOG2E", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_LOG10E), NULL);
	gravity_class_bind(meta, "LOG10E", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_SQRT2), NULL);
	gravity_class_bind(meta, "SQRT2", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	closure = computed_property_create(NULL, NEW_FUNCTION(math_SQRT1_2), NULL);
	gravity_class_bind(meta, "SQRT1_2", GravityValue::from_object(reinterpret_cast<gravity_class_t *>(closure)));
	SETMETA_INITED(gravity_class_math);
}

// MARK: - Commons -

void gravity_math_register(gravity_vm * vm) 
{
	if(!gravity_class_math) 
		create_optional_class();
	++refcount;
	if(!vm || gravity_vm_ismini(vm)) 
		return;
	gravity_vm_setvalue(vm, GRAVITY_CLASS_MATH_NAME, GravityValue::from_object(gravity_class_math));
}

void gravity_math_free() 
{
	if(!gravity_class_math) 
		return;
	if(--refcount) 
		return;
	gravity_class_t * meta = gravity_class_get_meta(gravity_class_math);
	computed_property_free(meta, "PI", true);
	computed_property_free(meta, "E", true);
	computed_property_free(meta, "LN2", true);
	computed_property_free(meta, "LN10", true);
	computed_property_free(meta, "LOG2E", true);
	computed_property_free(meta, "LOG10E", true);
	computed_property_free(meta, "SQRT2", true);
	computed_property_free(meta, "SQRT1_2", true);
	gravity_class_free_core(NULL, meta);
	gravity_class_free_core(NULL, gravity_class_math);
	gravity_class_math = NULL;
}
#endif // } 0