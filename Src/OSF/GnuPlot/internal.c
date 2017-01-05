/* GNUPLOT - internal.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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
#include <gnuplot.h>
#pragma hdrstop

#ifndef _WIN64
/*
 * FIXME: This is almost certainly out of date on linux, since the matherr
 * mechanism has been replaced by math_error() and supposedly is only
 * enabled via an explicit declaration #define _SVID_SOURCE.
 */
/*
 * Excerpt from the Solaris man page for matherr():
 *
 *   The The System V Interface Definition, Third Edition (SVID3)
 *   specifies  that  certain  libm functions call matherr() when
 *   exceptions are detected. Users may define their own  mechan-
 *   isms  for handling exceptions, by including a function named
 *   matherr() in their programs.
 */

int GP_MATHERR(STRUCT_EXCEPTION_P_X)
{
	return (GpE.undefined = true); /* don't print error message */
}

#endif

static enum DATA_TYPES sprintf_specifier __PROTO((const char* format));

#define BADINT_DEFAULT int_error(NO_CARET, "error: bit shift applied to non-INT");
#define BAD_TYPE(type) int_error(NO_CARET, (type==NOTDEFINED) ? "uninitialized user variable" : "internal error : type neither INT nor CMPLX");

static int recursion_depth = 0;
void eval_reset_after_error()
{
	recursion_depth = 0;
}

void f_push(GpArgument * x)
{
	UdvtEntry * udv = x->udv_arg;
	if(udv->udv_value.type == NOTDEFINED) {
		if(GpC.P.IsStringResultOnly)
			udv = GpE.udv_NaN; // We're only here to check whether this is a string. It isn't
		else
			int_error(NO_CARET, "undefined variable: %s", udv->udv_name);
	}
	GpE.Push(&(udv->udv_value));
}

void f_pushc(GpArgument * x)
{
	GpE.Push(&(x->v_arg));
}

void f_pushd1(GpArgument * x)
{
	GpE.Push(&(x->udf_arg->dummy_values[0]));
}

void f_pop(GpArgument * x)
{
	t_value dummy;
	GpE.Pop(dummy);
	if(dummy.type == STRING)
		gpfree_string(&dummy);
#ifdef ARRAY_COPY_ON_REFERENCE
	if(dummy.type == ARRAY)
		gpfree_string(&dummy);
#endif
}

void f_pushd2(GpArgument * x)
{
	GpE.Push(&(x->udf_arg->dummy_values[1]));
}

void f_pushd(GpArgument * x)
{
	t_value param;

	GpE.Pop(param);
	GpE.Push(&(x->udf_arg->dummy_values[param.v.int_val]));
}

/* execute a udf */
void f_call(GpArgument * x)
{
	t_value save_dummy;
	UdftEntry * udf = x->udf_arg;
	if(!udf->at) {
		if(GpC.P.IsStringResultOnly) {
			/* We're only here to check whether this is a string. It isn't. */
			f_pop(x);
			GpE.Push(&GpE.udv_NaN->udv_value);
			return;
		}
		int_error(NO_CARET, "undefined function: %s", udf->udf_name);
	}
	save_dummy = udf->dummy_values[0];
	GpE.Pop(udf->dummy_values[0]);
	if(udf->dummy_values[0].type == ARRAY)
		int_error(NO_CARET, "f_call: unsupported array operation");
	if(udf->dummy_num != 1)
		int_error(NO_CARET, "function %s requires %d variables", udf->udf_name, udf->dummy_num);
	if(recursion_depth++ > STACK_DEPTH)
		int_error(NO_CARET, "recursion depth limit exceeded");
	GpE.ExecuteAt(udf->at);
	gpfree_string(&udf->dummy_values[0]);
	udf->dummy_values[0] = save_dummy;
	recursion_depth--;
}
//
// execute a udf of n variables
//
void f_calln(GpArgument * x)
{
	t_value save_dummy[MAX_NUM_VAR];
	int i;
	int num_pop;
	t_value num_params;
	UdftEntry * udf = x->udf_arg;
	if(!udf->at)            /* undefined */
		int_error(NO_CARET, "undefined function: %s", udf->udf_name);
	for(i = 0; i < MAX_NUM_VAR; i++)
		save_dummy[i] = udf->dummy_values[i];
	GpE.Pop(num_params);
	if(num_params.v.int_val != udf->dummy_num)
		int_error(NO_CARET, "function %s requires %d variable%c", udf->udf_name, udf->dummy_num, (udf->dummy_num == 1) ? '\0' : 's');
	/* if there are more parameters than the function is expecting */
	/* simply ignore the excess */
	if(num_params.v.int_val > MAX_NUM_VAR) {
		/* pop and discard the dummies that there is no room for */
		num_pop = num_params.v.int_val - MAX_NUM_VAR;
		for(i = 0; i < num_pop; i++)
			GpE.Pop((udf->dummy_values[0]));
		num_pop = MAX_NUM_VAR;
	}
	else {
		num_pop = num_params.v.int_val;
	}
	/* pop parameters we can use */
	for(i = num_pop - 1; i >= 0; i--) {
		GpE.Pop((udf->dummy_values[i]));
		if(udf->dummy_values[i].type == ARRAY)
			int_error(NO_CARET, "f_calln: unsupported array operation");
	}
	if(recursion_depth++ > STACK_DEPTH)
		int_error(NO_CARET, "recursion depth limit exceeded");
	GpE.ExecuteAt(udf->at);
	recursion_depth--;
	for(i = 0; i < MAX_NUM_VAR; i++) {
		gpfree_string(&udf->dummy_values[i]);
		udf->dummy_values[i] = save_dummy[i];
	}
}

void f_sum(GpArgument * arg)
{
	t_value beg, end, varname; /* [<var> = <start>:<end>] */
	UdftEntry * udf;           /* function to evaluate */
	UdvtEntry * udv;           /* iteration variable */
	t_value ret;           /* result */
	t_value z;
	int i;
	GpE.Pop(end);
	GpE.Pop(beg);
	GpE.Pop(varname);
	if(beg.type != INTGR || end.type != INTGR)
		int_error(NO_CARET, "range specifiers of sum must have integer values");
	if(varname.type != STRING)
		int_error(NO_CARET, "internal error: f_sum expects argument (varname) of type string.");
	udv = GpE.GetUdvByName(varname.v.string_val);
	if(!udv)
		int_error(NO_CARET, "internal error: f_sum could not access iteration variable.");
	udf = arg->udf_arg;
	if(!udf)
		int_error(NO_CARET, "internal error: f_sum could not access summation coefficient function");
	ret.SetComplex(0, 0);
	for(i = beg.v.int_val; i<=end.v.int_val; ++i) {
		// calculate f_i = f() with user defined variable i 
		udv->udv_value.SetInt(i);
		GpE.ExecuteAt(udf->at);
		GpE.Pop(z);
		ret.SetComplex(ret.Real() + z.Real(), imag(&ret) + imag(&z));
	}
	gpfree_string(&varname);
	GpE.Push(z.SetComplex(ret.Real(), imag(&ret)));
}

void f_lnot(GpArgument * arg)
{
	t_value a;
	(void)arg;              
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(!a.v.int_val));
}

void f_bnot(GpArgument * arg)
{
	t_value a;
	(void)arg;              
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(~a.v.int_val));
}

void f_lor(GpArgument * arg)
{
	t_value a, b;
	(void)arg;              
	int_check(GpE.Pop(b));
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(a.v.int_val || b.v.int_val));
}

void f_land(GpArgument * arg)
{
	t_value a, b;
	(void)arg;              
	int_check(GpE.Pop(b));
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(a.v.int_val && b.v.int_val));
}

void f_bor(GpArgument * arg)
{
	t_value a, b;
	(void)arg;              
	int_check(GpE.Pop(b));
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(a.v.int_val | b.v.int_val));
}

void f_xor(GpArgument * arg)
{
	t_value a, b;
	(void)arg;              
	int_check(GpE.Pop(b));
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(a.v.int_val ^ b.v.int_val));
}

void f_band(GpArgument * arg)
{
	t_value a, b;
	(void)arg;              
	int_check(GpE.Pop(b));
	int_check(GpE.Pop(a));
	GpE.Push(a.SetInt(a.v.int_val & b.v.int_val));
}
//
// Make all the following internal routines perform autoconversion
// from string to numeric value.
//
//#define pop__(x) pop_or_convert_from_string(x)

void f_uminus(GpArgument * arg)
{
	t_value a;
	(void)arg;              
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    a.v.int_val = -a.v.int_val;
		    break;
		case CMPLX:
		    a.v.cmplx_val.real = -a.v.cmplx_val.real;
		    a.v.cmplx_val.imag = -a.v.cmplx_val.imag;
		    break;
		default:
		    BAD_TYPE(a.type)
		    break;
	}
	GpE.Push(&a);
}

void f_eq(GpArgument * arg)
{
	/* note: floating point equality is rare because of roundoff error! */
	t_value a, b;
	int result = 0;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val == b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val == b.v.cmplx_val.real && b.v.cmplx_val.imag == 0.0);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result = (b.v.int_val == a.v.cmplx_val.real && a.v.cmplx_val.imag == 0.0);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real == b.v.cmplx_val.real && a.v.cmplx_val.imag == b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(a.SetInt(result));
}

void f_ne(GpArgument * arg)
{
	t_value a, b;
	int result = 0;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val != b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val != b.v.cmplx_val.real || b.v.cmplx_val.imag != 0.0);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result = (b.v.int_val != a.v.cmplx_val.real || a.v.cmplx_val.imag != 0.0);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real != b.v.cmplx_val.real || a.v.cmplx_val.imag != b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(a.SetInt(result));
}

void f_gt(GpArgument * arg)
{
	t_value a, b;
	int result = 0;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val > b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val > b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.cmplx_val.real > b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real > b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(a.SetInt(result));
}

void f_lt(GpArgument * arg)
{
	t_value a, b;
	int result = 0;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val < b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val < b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.cmplx_val.real <
			    b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real <
			    b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(a.SetInt(result));
}

void f_ge(GpArgument * arg)
{
	t_value a, b;
	int result = 0;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val >= b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val >= b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.cmplx_val.real >=
			    b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real >=
			    b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(a.SetInt(result));
}

void f_le(GpArgument * arg)
{
	t_value a, b;
	int result = 0;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val <=
			    b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val <=
			    b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.cmplx_val.real <= b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real <= b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(a.SetInt(result));
}

void f_leftshift(GpArgument * arg)
{
	t_value a, b, result;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result.SetInt((unsigned)(a.v.int_val) << b.v.int_val);
				break;
			    default:
				BADINT_DEFAULT
		    }
		    break;
		default:
		    BADINT_DEFAULT
	}
	GpE.Push(&result);
}

void f_rightshift(GpArgument * arg)
{
	t_value a, b, result;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result.SetInt((unsigned)(a.v.int_val) >> b.v.int_val);
				break;
			    default:
				BADINT_DEFAULT
		    }
		    break;
		default:
		    BADINT_DEFAULT
	}
	GpE.Push(&result);
}

void f_plus(GpArgument * arg)
{
	t_value a, b, result;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result.SetInt(a.v.int_val + b.v.int_val);
				break;
			    case CMPLX:
				result.SetComplex(a.v.int_val + b.v.cmplx_val.real, b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result.SetComplex(b.v.int_val + a.v.cmplx_val.real, a.v.cmplx_val.imag);
				break;
			    case CMPLX:
				result.SetComplex(a.v.cmplx_val.real + b.v.cmplx_val.real, a.v.cmplx_val.imag + b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(&result);
}

void f_minus(GpArgument * arg)
{
	t_value a, b, result;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);          /* now do a - b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result.SetInt(a.v.int_val - b.v.int_val);
				break;
			    case CMPLX:
				result.SetComplex(a.v.int_val - b.v.cmplx_val.real, -b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result.SetComplex(a.v.cmplx_val.real - b.v.int_val, a.v.cmplx_val.imag);
				break;
			    case CMPLX:
				result.SetComplex(a.v.cmplx_val.real - b.v.cmplx_val.real, a.v.cmplx_val.imag - b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(&result);
}

void f_mult(GpArgument * arg)
{
	t_value a, b, result;
	double product;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);          /* now do a*b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				product = (double)a.v.int_val * (double)b.v.int_val;
				if(fabs(product) >= (double)INT_MAX)
					result.SetComplex(product, 0.0);
				else
					result.SetInt(a.v.int_val * b.v.int_val);
				break;
			    case CMPLX:
				result.SetComplex(a.v.int_val * b.v.cmplx_val.real, a.v.int_val * b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				result.SetComplex(b.v.int_val * a.v.cmplx_val.real, b.v.int_val * a.v.cmplx_val.imag);
				break;
			    case CMPLX:
				result.SetComplex(a.v.cmplx_val.real * b.v.cmplx_val.real - a.v.cmplx_val.imag * b.v.cmplx_val.imag,
					a.v.cmplx_val.real * b.v.cmplx_val.imag + a.v.cmplx_val.imag * b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(&result);
}

void f_div(GpArgument * arg)
{
	t_value a, b, result;
	double square;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);          /* now do a/b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				if(b.v.int_val)
					result.SetInt(a.v.int_val / b.v.int_val);
				else {
					result.SetInt(0);
					GpE.undefined = true;
				}
				break;
			    case CMPLX:
				square = b.v.cmplx_val.real * b.v.cmplx_val.real + b.v.cmplx_val.imag * b.v.cmplx_val.imag;
				if(square)
					result.SetComplex(a.v.int_val * b.v.cmplx_val.real / square, -a.v.int_val * b.v.cmplx_val.imag / square);
				else {
					result.SetComplex(0.0, 0.0);
					GpE.undefined = true;
				}
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				if(b.v.int_val)
					result.SetComplex(a.v.cmplx_val.real / b.v.int_val, a.v.cmplx_val.imag / b.v.int_val);
				else {
					result.SetComplex(0.0, 0.0);
					GpE.undefined = true;
				}
				break;
			    case CMPLX:
				square = b.v.cmplx_val.real *
			    b.v.cmplx_val.real +
			    b.v.cmplx_val.imag *
			    b.v.cmplx_val.imag;
				if(square)
					result.SetComplex((a.v.cmplx_val.real * b.v.cmplx_val.real + a.v.cmplx_val.imag * b.v.cmplx_val.imag) / square,
						(a.v.cmplx_val.imag * b.v.cmplx_val.real - a.v.cmplx_val.real * b.v.cmplx_val.imag) / square);
				else {
					result.SetComplex(0.0, 0.0);
					GpE.undefined = true;
				}
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(&result);
}

void f_mod(GpArgument * arg)
{
	t_value a, b;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);          /* now do a%b */
	if(a.type != INTGR || b.type != INTGR)
		int_error(NO_CARET, "non-integer operand for %%");
	if(b.v.int_val)
		GpE.Push(a.SetInt(a.v.int_val % b.v.int_val));
	else {
		GpE.Push(a.SetInt(0));
		GpE.undefined = true;
	}
}

void f_power(GpArgument * arg)
{
	t_value a, b, result;
	int i, t;
	double mag, ang;
	(void)arg;              
	GpE.PopOrConvertFromString(b);
	GpE.PopOrConvertFromString(a);          /* now find a**b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				if(a.v.int_val == 0) {
					if(b.v.int_val < 0)
						GpE.undefined = true;
					result.SetInt(b.v.int_val == 0 ? 1 : 0);
					break;
				}
				/* EAM Oct 2009 - avoid integer overflow by switching to double */
				mag = pow((double)a.v.int_val, (double)b.v.int_val);
				if(mag > (double)INT_MAX  ||  b.v.int_val < 0) {
					result.SetComplex(mag, 0.0);
					break;
				}
				t = 1;
				/* this ought to use bit-masks and squares, etc */
				for(i = 0; i < b.v.int_val; i++)
					t *= a.v.int_val;
				result.SetInt(t);
				break;
			    case CMPLX:
				if(a.v.int_val == 0) {
					if(b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
						GpE.undefined = true;
					}
					// return 1.0 for 0**0 
					result.SetComplex(b.v.cmplx_val.real == 0 ? 1.0 : 0.0, 0.0);
				}
				else {
					mag =
				    pow(magnitude(&a), fabs(b.v.cmplx_val.real));
					if(b.v.cmplx_val.real < 0.0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							GpE.undefined = true;
					}
					mag *= gp_exp(-b.v.cmplx_val.imag * angle(&a));
					ang = b.v.cmplx_val.real * angle(&a) +
				    b.v.cmplx_val.imag * log(magnitude(&a));
					result.SetComplex(mag * cos(ang), mag * sin(ang));
				}
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				if(a.v.cmplx_val.imag == 0.0) {
					mag = pow(a.v.cmplx_val.real, (double)abs(b.v.int_val));
					if(b.v.int_val < 0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							GpE.undefined = true;
					}
					result.SetComplex(mag, 0.0);
				}
				else {
					// not so good, but...!
					mag = pow(magnitude(&a), (double)abs(b.v.int_val));
					if(b.v.int_val < 0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							GpE.undefined = true;
					}
					ang = angle(&a) * b.v.int_val;
					result.SetComplex(mag * cos(ang),
				    mag * sin(ang));
				}
				break;
			    case CMPLX:
				if(a.v.cmplx_val.real == 0 && a.v.cmplx_val.imag == 0) {
					if(b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
						GpE.undefined = true;
					}
					// return 1.0 for 0**0 
					result.SetComplex(b.v.cmplx_val.real == 0 ? 1.0 : 0.0, 0.0);
				}
				else {
					mag = pow(magnitude(&a), fabs(b.v.cmplx_val.real));
					if(b.v.cmplx_val.real < 0.0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							GpE.undefined = true;
					}
					mag *= gp_exp(-b.v.cmplx_val.imag * angle(&a));
					ang = b.v.cmplx_val.real * angle(&a) + b.v.cmplx_val.imag * log(magnitude(&a));
					result.SetComplex(mag * cos(ang), mag * sin(ang));
				}
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GpE.Push(&result);
}

void f_factorial(GpArgument * arg)
{
	t_value a;
	int i;
	double val = 0.0;
	(void)arg;              
	GpE.PopOrConvertFromString(a);          /* find a! (factorial) */
	switch(a.type) {
		case INTGR:
		    val = 1.0;
		    for(i = a.v.int_val; i > 1; i--) /*fpe's should catch overflows */
			    val *= i;
		    break;
		default:
		    int_error(NO_CARET, "factorial (!) argument must be an integer");
		    return;     /* avoid gcc -Wall warning about val */
	}
	GpE.Push(a.SetComplex(val, 0.0));
}
/*
 * Terminate the autoconversion from string to numeric values
 */
#undef pop__

void f_concatenate(GpArgument * arg)
{
	t_value a, b, result;

	(void)arg;              
	GpE.Pop(b);
	GpE.Pop(a);

	if(b.type == INTGR) {
		int i = b.v.int_val;
		b.type = STRING;
		b.v.string_val = (char*)malloc(32);
#ifdef HAVE_SNPRINTF
		snprintf(b.v.string_val, 32, "%d", i);
#else
		sprintf(b.v.string_val, "%d", i);
#endif
	}

	if(a.type != STRING || b.type != STRING)
		int_error(NO_CARET, "internal error : STRING operator applied to non-STRING type");

	(void)Gstring(&result, gp_stradd(a.v.string_val, b.v.string_val));
	gpfree_string(&a);
	gpfree_string(&b);
	GpE.Push(&result);
	gpfree_string(&result); /* free string allocated within gp_stradd() */
}

void f_eqs(GpArgument * arg)
{
	t_value a, b, result;
	(void)arg;              
	GpE.Pop(b);
	GpE.Pop(a);
	if(a.type != STRING || b.type != STRING)
		int_error(NO_CARET, "internal error : STRING operator applied to non-STRING type");
	result.SetInt(!strcmp(a.v.string_val, b.v.string_val));
	gpfree_string(&a);
	gpfree_string(&b);
	GpE.Push(&result);
}

void f_nes(GpArgument * arg)
{
	t_value a, b, result;
	(void)arg;              
	GpE.Pop(b);
	GpE.Pop(a);
	if(a.type != STRING || b.type != STRING)
		int_error(NO_CARET, "internal error : STRING operator applied to non-STRING type");
	result.SetInt((int)(strcmp(a.v.string_val, b.v.string_val)!=0));
	gpfree_string(&a);
	gpfree_string(&b);
	GpE.Push(&result);
}

void f_strlen(GpArgument * arg)
{
	t_value a, result;
	(void)arg;
	GpE.Pop(a);
	if(a.type != STRING)
		int_error(NO_CARET, "internal error : strlen of non-STRING argument");
	result.SetInt((int)gp_strlen(a.v.string_val));
	gpfree_string(&a);
	GpE.Push(&result);
}

void f_strstrt(GpArgument * arg)
{
	t_value needle, haystack, result;
	char * start;
	(void)arg;
	GpE.Pop(needle);
	GpE.Pop(haystack);
	if(needle.type != STRING || haystack.type != STRING)
		int_error(NO_CARET, "internal error : non-STRING argument to strstrt");
	start = strstr(haystack.v.string_val, needle.v.string_val);
	result.SetInt((int)(start ? (start-haystack.v.string_val)+1 : 0));
	gpfree_string(&needle);
	gpfree_string(&haystack);
	GpE.Push(&result);
}
/*
 * f_range() handles both explicit calls to substr(string, beg, end)
 * and the short form string[beg:end].  The calls to gp_strlen() and
 * gp_strchrn() allow it to handle utf8 strings.
 */
void f_range(GpArgument * arg)
{
	t_value beg, end, full;

	t_value substr;

	int ibeg, iend;

	(void)arg;              
	GpE.Pop(end);
	GpE.Pop(beg);
	GpE.Pop(full);

	if(beg.type == INTGR)
		ibeg = beg.v.int_val;
	else if(beg.type == CMPLX)
		ibeg = (int)floor(beg.v.cmplx_val.real);
	else
		int_error(NO_CARET, "internal error: non-numeric substring range specifier");
	if(end.type == INTGR)
		iend = end.v.int_val;
	else if(end.type == CMPLX)
		iend = (int)floor(end.v.cmplx_val.real);
	else
		int_error(NO_CARET, "internal error: non-numeric substring range specifier");

	if(full.type != STRING)
		int_error(NO_CARET, "internal error: substring range operator applied to non-STRING type");

	FPRINTF((stderr, "f_range( \"%s\", %d, %d)\n", full.v.string_val, beg.v.int_val, end.v.int_val));

	if(iend > (int)gp_strlen(full.v.string_val))
		iend = gp_strlen(full.v.string_val);
	if(ibeg < 1)
		ibeg = 1;

	if(ibeg > iend) {
		GpE.Push(Gstring(&substr, ""));
	}
	else {
		char * begp = gp_strchrn(full.v.string_val, ibeg-1);
		char * endp = gp_strchrn(full.v.string_val, iend);
		*endp = '\0';
		GpE.Push(Gstring(&substr, begp));
	}
	gpfree_string(&full);
}

/*
 * f_index() extracts the value of a single element from an array.
 */
void f_index(GpArgument * arg)
{
	t_value array, index;

	int i = -1;

	(void)arg;              
	GpE.Pop(index);
	GpE.Pop(array);

	if(array.type != ARRAY)
		int_error(NO_CARET, "internal error: attempt to index non-array variable");
	if(index.type == INTGR)
		i = index.v.int_val;
	else if(index.type == CMPLX)
		i = (int)floor(index.v.cmplx_val.real);
	if(i <= 0 || i > array.v.value_array[0].v.int_val)
		int_error(NO_CARET, "array index out of range");
	GpE.Push(&array.v.value_array[i]);
}

/* Magic number! */
#define RETURN_WORD_COUNT (-17*23*61)

void f_words(GpArgument * arg)
{
	t_value a;

	/* "words(s)" is implemented as "word(s,RETURN_WORD_COUNT)" */
	GpE.Push(a.SetInt(RETURN_WORD_COUNT));
	f_word(arg);
}

void f_word(GpArgument * arg)
{
	t_value a, b, result;

	int nwords = 0;
	int in_string = 0;
	int ntarget;
	char q;
	char * s;

	(void)arg;
	if(GpE.Pop(b).type != INTGR)
		int_error(NO_CARET, "internal error : non-INTGR argument");
	ntarget = b.v.int_val;

	if(GpE.Pop(a).type != STRING)
		int_error(NO_CARET, "internal error : non-STRING argument");
	s = a.v.string_val;

	Gstring(&result, "");
	while(*s) {
		while(isspace((uchar)*s)) s++;
		if(!*s)
			break;
		nwords++;
		if(*s == '"' || *s == '\'') {
			q = *s;
			s++;
			in_string = 1;
		}
		if(nwords == ntarget) { /* Found the one we wanted */
			Gstring(&result, s);
			s = result.v.string_val;
		}
		while(*s && ((!isspace((uchar)*s) && !in_string) || (in_string && *s != q))) s++;
		if(nwords == ntarget) { /* Terminate this word cleanly */
			*s = '\0';
			break;
		}
		if(in_string && (*s == q)) {
			in_string = 0;
			s++;
		}
	}

	/* words(s) = word(s,magic_number) = # of words in string */
	if(ntarget == RETURN_WORD_COUNT)
		result.SetInt(nwords);

	GpE.Push(&result);
	gpfree_string(&a);
}

#undef RETURN_WORD_COUNT

/* EAM July 2004  (revised to dynamic buffer July 2005)
 * There are probably an infinite number of things that can
 * go wrong if the user mis-matches arguments and format strings
 * in the call to sprintf, but I hope none will do worse than
 * result in a garbage output string.
 */
void f_sprintf(GpArgument * arg)
{
	t_value a[10], * args;

	t_value num_params;

	t_value result;

	char * buffer;
	int bufsize;
	char * next_start, * outpos, tempchar;
	int next_length;
	char * prev_start;
	int prev_pos;
	int i, remaining;
	int nargs = 0;
	enum DATA_TYPES spec_type;

	/* Retrieve number of parameters from top of stack */
	(void)arg;
	GpE.Pop(num_params);
	nargs = num_params.v.int_val;
	if(nargs > 10) { /* Fall back to slow but sure allocation */
		args = (t_value *)malloc(sizeof(t_value)*nargs);
	}
	else
		args = a;
	for(i = 0; i<nargs; i++)
		GpE.Pop(args[i]);  /* pop next argument */
	// Make sure we got a format string of some sort
	if(args[nargs-1].type != STRING)
		int_error(NO_CARET, "First parameter to sprintf must be a format string");

	/* Allocate space for the output string. If this isn't */
	/* long enough we can reallocate a larger space later. */
	bufsize = 80 + strlen(args[nargs-1].v.string_val);
	buffer = (char *)malloc(bufsize);
	/* Copy leading fragment of format into output buffer */
	outpos = buffer;
	next_start  = args[nargs-1].v.string_val;
	next_length = strcspn(next_start, "%");
	strncpy(outpos, next_start, next_length);

	next_start += next_length;
	outpos += next_length;

	/* Format the remaining sprintf() parameters one by one */
	prev_start = next_start;
	prev_pos = next_length;
	remaining = nargs - 1;

	/* If the user has set an explicit LC_NUMERIC locale, apply it */
	/* to sprintf calls during expression evaluation.              */
	set_numeric_locale();

	/* Each time we start this loop we are pointing to a % character */
	while(remaining-->0 && next_start[0] && next_start[1]) {
		t_value * next_param = &args[remaining];

		/* Check for %%; print as literal and don't consume a parameter */
		if(!strncmp(next_start, "%%", 2)) {
			next_start++;
			do {
				*outpos++ = *next_start++;
			} while(*next_start && *next_start != '%');
			remaining++;
			continue;
		}

		next_length = strcspn(next_start+1, "%") + 1;
		tempchar = next_start[next_length];
		next_start[next_length] = '\0';

		spec_type = sprintf_specifier(next_start);

		/* string value <-> numerical value check */
		if(spec_type == STRING && next_param->type != STRING)
			int_error(NO_CARET, "f_sprintf: attempt to print numeric value with string format");
		if(spec_type != STRING && next_param->type == STRING)
			int_error(NO_CARET, "f_sprintf: attempt to print string value with numeric format");

#ifdef HAVE_SNPRINTF
		/* Use the format to print next arg */
		switch(spec_type) {
			case INTGR:
			    snprintf(outpos, bufsize-(outpos-buffer),
			    next_start, (int)real(next_param));
			    break;
			case CMPLX:
			    snprintf(outpos, bufsize-(outpos-buffer),
			    next_start, real(next_param));
			    break;
			case STRING:
			    snprintf(outpos, bufsize-(outpos-buffer),
			    next_start, next_param->v.string_val);
			    break;
			default:
			    int_error(NO_CARET, "internal error: invalid spec_type");
		}
#else
		/* FIXME - this is bad; we should dummy up an snprintf equivalent */
		switch(spec_type) {
			case INTGR:
				sprintf(outpos, next_start, (int)next_param->Real());
			    break;
			case CMPLX:
				sprintf(outpos, next_start, next_param->Real());
			    break;
			case STRING:
			    sprintf(outpos, next_start, next_param->v.string_val);
			    break;
			default:
			    int_error(NO_CARET, "internal error: invalid spec_type");
		}
#endif

		next_start[next_length] = tempchar;
		next_start += next_length;
		outpos = &buffer[strlen(buffer)];

		/* Check whether previous parameter output hit the end of the buffer */
		/* If so, reallocate a larger buffer, go back and try it again.      */
		if((int)strlen(buffer) >= bufsize-2) {
			bufsize *= 2;
			buffer = (char *)gp_realloc(buffer, bufsize, "f_sprintf");
			next_start = prev_start;
			outpos = buffer + prev_pos;
			remaining++;
			continue;
		}
		else {
			prev_start = next_start;
			prev_pos = outpos - buffer;
		}
	}

	/* Copy the trailing portion of the format, if any */
	/* We could just call snprintf(), but it doesn't check for */
	/* whether there really are more variables to handle.      */
	i = bufsize - (outpos-buffer);
	while(*next_start && --i > 0) {
		if(*next_start == '%' && *(next_start+1) == '%')
			next_start++;
		*outpos++ = *next_start++;
	}
	*outpos = '\0';

	FPRINTF((stderr, " snprintf result = \"%s\"\n", buffer));
	GpE.Push(Gstring(&result, buffer));
	free(buffer);

	/* Free any strings from parameters we have now used */
	for(i = 0; i<nargs; i++)
		gpfree_string(&args[i]);

	if(args != a)
		free(args);

	/* Return to C locale for internal use */
	reset_numeric_locale();
}

/* EAM July 2004 - Gnuplot's own string formatting conventions.
 * Currently this routine assumes base 10 representation, because
 * it is not clear where it could be specified to be anything else.
 */
void f_gprintf(GpArgument * arg)
{
	t_value fmt, val, result;

	char * buffer;
	int length;
	double base = 10.;

	/* Retrieve parameters from top of stack */
	(void)arg;
	GpE.Pop(val);
	GpE.Pop(fmt);
	// Make sure parameters are of the correct type 
	if(fmt.type != STRING)
		int_error(NO_CARET, "First parameter to gprintf must be a format string");
	// Make sure we have at least as much space in the output as the format itself 
	length = 80 + strlen(fmt.v.string_val);
	buffer = (char *)malloc(length);
	// Call the old internal routine 
	gprintf(buffer, length, fmt.v.string_val, base, val.Real());
	FPRINTF((stderr, " gprintf result = \"%s\"\n", buffer));
	GpE.Push(Gstring(&result, buffer));
	gpfree_string(&fmt);
	free(buffer);
}

/* Output time given in seconds from year 2000 into string */
void f_strftime(GpArgument * arg)
{
	t_value fmt, val;
	char * fmtstr, * buffer;
	int fmtlen, buflen, length;
	(void)arg;
	/* Retrieve parameters from top of stack */
	GpE.Pop(val);
	GpE.Pop(fmt);
	if(fmt.type != STRING)
		int_error(NO_CARET,
		    "First parameter to strftime must be a format string");

	/* Prepare format string.
	 * Make sure the resulting string not empty by adding a space.
	 * Otherwise, the return value of gstrftime doesn't give enough
	 * information.
	 */
	fmtlen = strlen(fmt.v.string_val) + 1;
	fmtstr = (char *)malloc(fmtlen + 1);
	strncpy(fmtstr, fmt.v.string_val, fmtlen);
	strncat(fmtstr, " ", fmtlen);
	buflen = 80 + 2*fmtlen;
	buffer = (char *)malloc(buflen);

	/* Get time_str */
	length = gstrftime(buffer, buflen, fmtstr, val.Real());
	if(length == 0 || length >= buflen)
		int_error(NO_CARET, "Resulting string is too long");

	/* Remove trailing space */
	assert(buffer[length-1] == ' ');
	buffer[length-1] = NUL;

	gpfree_string(&val);
	gpfree_string(&fmt);
	free(fmtstr);

	GpE.Push(Gstring(&val, buffer));
	free(buffer);
}

/* Convert string into seconds from year 2000 */
void f_strptime(GpArgument * arg)
{
	t_value fmt, val;

	struct tm time_tm;

	double usec = 0.0;
	double result;
	(void)arg;
	GpE.Pop(val);
	GpE.Pop(fmt);
	if(fmt.type != STRING || val.type != STRING)
		int_error(NO_CARET, "Both parameters to strptime must be strings");
	if(!fmt.v.string_val || !val.v.string_val)
		int_error(NO_CARET, "Internal error: string not allocated");
	/* string -> time_tm  plus extra fractional second */
	gstrptime(val.v.string_val, fmt.v.string_val, &time_tm, &usec);
	/* time_tm -> result */
	result = gtimegm(&time_tm);
	FPRINTF((stderr, " strptime result = %g seconds \n", result));
	/* Add back any extra fractional second */
	result += usec;
	gpfree_string(&val);
	gpfree_string(&fmt);
	GpE.Push(val.SetComplex(result, 0.0));
}

/* Get current system time in seconds since 2000
 * The type of the value popped from the stack
 * determines what is returned.
 * If integer, the result is also an integer.
 * If real (complex), the result is also real,
 * with microsecond precision (if available).
 * If string, it is assumed to be a format string,
 * and it is passed to strftime to get a formatted time string.
 */
void f_time(GpArgument * arg)
{
	t_value val, val2;
	double time_now;
#ifdef HAVE_SYS_TIME_H
	struct timeval tp;
	gettimeofday(&tp, NULL);
	tp.tv_sec -= SEC_OFFS_SYS;
	time_now = tp.tv_sec + (tp.tv_usec/1000000.0);
#else
	time_now = (double)time(NULL);
	time_now -= SEC_OFFS_SYS;
#endif
	(void)arg;
	GpE.Pop(val);
	switch(val.type) {
		case INTGR:
		    GpE.Push(val.SetInt((int)time_now));
		    break;
		case CMPLX:
		    GpE.Push(val.SetComplex(time_now, 0.0));
		    break;
		case STRING:
		    GpE.Push(&val); /* format string */
		    GpE.Push(val2.SetComplex(time_now, 0.0));
		    f_strftime(arg);
		    break;
		default:
		    int_error(NO_CARET, "internal error: invalid argument type");
	}
}

/* Return which argument type sprintf will need for this format string:
 *   char*       STRING
 *   int         INTGR
 *   double      CMPLX
 * Should call int_err for any other type.
 * format is expected to start with '%'
 */
static enum DATA_TYPES sprintf_specifier(const char* format)
{
	const char string_spec[]  = "s";
	const char real_spec[]    = "aAeEfFgG";
	const char int_spec[]     = "cdiouxX";
	/* The following characters are used for use of invalid types */
	const char illegal_spec[] = "hlLqjzZtCSpn";
	int string_pos, real_pos, int_pos, illegal_pos;
	/* check if really format specifier */
	if(format[0] != '%')
		int_error(NO_CARET, "internal error: sprintf_specifier called without '%'\n");
	string_pos  = strcspn(format, string_spec);
	real_pos    = strcspn(format, real_spec);
	int_pos     = strcspn(format, int_spec);
	illegal_pos = strcspn(format, illegal_spec);
	if(illegal_pos < int_pos && illegal_pos < real_pos && illegal_pos < string_pos)
		int_error(NO_CARET, "sprintf_specifier: used with invalid format specifier\n");
	else if(string_pos < real_pos && string_pos < int_pos)
		return STRING;
	else if(real_pos < int_pos)
		return CMPLX;
	else if(int_pos < (int)strlen(format) )
		return INTGR;
	else
		int_error(NO_CARET, "sprintf_specifier: no format specifier\n");
	return INTGR; /* Can't happen, but the compiler doesn't realize that */
}

/* execute a system call and return stream from STDOUT */
void f_system(GpArgument * arg)
{
	t_value val, result;

	char * output;
	int output_len, ierr;

	/* Retrieve parameters from top of stack */
	(void)arg;
	GpE.Pop(val);
	/* Make sure parameters are of the correct type */
	if(val.type != STRING)
		int_error(NO_CARET, "non-string argument to system()");
	FPRINTF((stderr, " f_system input = \"%s\"\n", val.v.string_val));
	ierr = do_system_func(val.v.string_val, &output);
	GpE.FillGpValInteger("GPVAL_ERRNO", ierr);
	output_len = strlen(output);
	/* chomp result */
	if(output_len > 0 && output[output_len-1] == '\n')
		output[output_len-1] = NUL;
	FPRINTF((stderr, " f_system result = \"%s\"\n", output));
	GpE.Push(Gstring(&result, output));
	gpfree_string(&result); /* free output */
	gpfree_string(&val); /* free command string */
}

/* Variable assignment operator */
void f_assign(GpArgument * arg)
{
	UdvtEntry * udv;

	t_value a, b, index;

	(void)arg;
	GpE.Pop(b);  /* new value */
	GpE.Pop(index); /* index (only used if this is an array assignment) */
	GpE.Pop(a);  /* name of variable */

	if(a.type != STRING)
		int_error(NO_CARET, "attempt to assign to something other than a named variable");
	if(!strncmp(a.v.string_val, "GPVAL_", 6) || !strncmp(a.v.string_val, "MOUSE_", 6))
		int_error(NO_CARET, "attempt to assign to a read-only variable");
	if(b.type == ARRAY)
		int_error(NO_CARET, "unsupported array operation");

	udv = GpE.AddUdvByName(a.v.string_val);
	gpfree_string(&a);

	if(udv->udv_value.type == ARRAY) {
		int i;
		if(index.type == INTGR)
			i = index.v.int_val;
		else if(index.type == CMPLX)
			i = (int)floor(index.v.cmplx_val.real);
		else
			int_error(NO_CARET, "non-numeric array index");
		if(i <= 0 || i > udv->udv_value.v.value_array[0].v.int_val)
			int_error(NO_CARET, "array index out of range");
		gpfree_string(&udv->udv_value.v.value_array[i]);
		udv->udv_value.v.value_array[i] = b;
	}
	else {
		gpfree_string(&(udv->udv_value));
		udv->udv_value = b;
	}

	GpE.Push(&b);
}

/*
 * Retrieve the current value of a user-defined variable whose name is known.
 * B = value("A") has the same result as B = A.
 */

void f_value(GpArgument * arg)
{
	UdvtEntry * p = GpE.first_udv;
	t_value a;
	t_value result;
	(void)arg;
	GpE.Pop(a);
	if(a.type != STRING) {
		// int_warn(NO_CARET,"non-string value passed to value()");
		GpE.Push(&a);
	}
	else {
		while(p) {
			if(!strcmp(p->udv_name, a.v.string_val)) {
				result = p->udv_value;
				if(p->udv_value.type == NOTDEFINED)
					p = NULL;
				else if(result.type == STRING)
					result.v.string_val = gp_strdup(result.v.string_val);
				break;
			}
			p = p->next_udv;
		}
		gpfree_string(&a);
		if(!p) {
			// int_warn(NO_CARET,"undefined variable name passed to value()");
			result.type = CMPLX;
			result.v.cmplx_val.real = not_a_number();
		}
		GpE.Push(&result);
	}
}

