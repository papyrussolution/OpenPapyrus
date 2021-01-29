// GNUPLOT - internal.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
/*
 * FIXME: Any platforms that still want support for matherr should
 * add appropriate definitions here.  Everyone else can now ignore it.
 *
 * Use of matherr is out of date on linux, since the matherr
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
/* Watcom compiler */
#ifdef __WATCOMC__
	int matherr(struct _exception * x) { return (undefined = TRUE); /* don't print error message */ }
#endif

static enum DATA_TYPES sprintf_specifier(const char * format);

#define BADINT_DEFAULT GPO.IntError(NO_CARET, "error: bit shift applied to non-INT");
#define BAD_TYPE(type) GPO.IntError(NO_CARET, (type==NOTDEFINED) ? "uninitialized user variable" : "internal error : type neither INT nor CMPLX");

static const char * nonstring_error = "internal error : STRING operator applied to undefined or non-STRING variable";

static int recursion_depth = 0;
void eval_reset_after_error()
{
	recursion_depth = 0;
	__IsUndefined = false;
}

void f_push(union argument * x)
{
	struct udvt_entry * udv = x->udv_arg;
	if(udv->udv_value.type == NOTDEFINED) {
		if(string_result_only)
			/* We're only here to check whether this is a string. It isn't. */
			udv = udv_NaN;
		else
			GPO.IntError(NO_CARET, "undefined variable: %s", udv->udv_name);
	}
	GPO.EvStk.Push(&(udv->udv_value));
}

void f_pushc(union argument * x)
{
	GPO.EvStk.Push(&(x->v_arg));
}

void f_pushd1(union argument * x)
{
	GPO.EvStk.Push(&(x->udf_arg->dummy_values[0]));
}

void f_pop(union argument * x)
{
	GpValue dummy;
	GPO.EvStk.Pop(&dummy);
	if(dummy.type == STRING)
		gpfree_string(&dummy);
}

void f_pushd2(union argument * x)
{
	GPO.EvStk.Push(&(x->udf_arg->dummy_values[1]));
}

void f_pushd(union argument * x)
{
	GpValue param;
	GPO.EvStk.Pop(&param);
	GPO.EvStk.Push(&(x->udf_arg->dummy_values[param.v.int_val]));
}
//
// execute a udf 
//
void f_call(union argument * x)
{
	GpValue save_dummy;
	udft_entry * udf = x->udf_arg;
	if(!udf->at) {
		if(string_result_only) {
			// We're only here to check whether this is a string. It isn't. 
			f_pop(x);
			GPO.EvStk.Push(&(udv_NaN->udv_value));
			return;
		}
		GPO.IntError(NO_CARET, "undefined function: %s", udf->udf_name);
	}
	save_dummy = udf->dummy_values[0];
	GPO.EvStk.Pop(&(udf->dummy_values[0]));
	if(udf->dummy_values[0].type == ARRAY)
		GPO.IntError(NO_CARET, "f_call: unsupported array operation");
	if(udf->dummy_num != 1)
		GPO.IntError(NO_CARET, "function %s requires %d variables", udf->udf_name, udf->dummy_num);
	if(recursion_depth++ > STACK_DEPTH)
		GPO.IntError(NO_CARET, "recursion depth limit exceeded");
	execute_at(udf->at);
	gpfree_string(&udf->dummy_values[0]);
	udf->dummy_values[0] = save_dummy;
	recursion_depth--;
}
//
// execute a udf of n variables 
//
void f_calln(union argument * x)
{
	GpValue save_dummy[MAX_NUM_VAR];
	int i;
	int num_pop;
	GpValue num_params;
	udft_entry * udf = x->udf_arg;
	if(!udf->at)            /* undefined */
		GPO.IntError(NO_CARET, "undefined function: %s", udf->udf_name);
	GPO.EvStk.Pop(&num_params);
	num_pop = num_params.v.int_val;
	if(num_pop != udf->dummy_num)
		GPO.IntError(NO_CARET, "function %s requires %d variable%c", udf->udf_name, udf->dummy_num, (udf->dummy_num == 1) ? '\0' : 's');
	// Jul 2020 CHANGE: we used to discard and ignore extra parameters 
	if(num_pop > MAX_NUM_VAR)
		GPO.IntError(NO_CARET, "too many parameters passed to function %s", udf->udf_name);
	for(i = 0; i < num_pop; i++)
		save_dummy[i] = udf->dummy_values[i];
	// pop parameters we can use 
	for(i = num_pop - 1; i >= 0; i--) {
		GPO.EvStk.Pop(&(udf->dummy_values[i]));
		if(udf->dummy_values[i].type == ARRAY)
			GPO.IntError(NO_CARET, "f_calln: unsupported array operation");
	}
	if(recursion_depth++ > STACK_DEPTH)
		GPO.IntError(NO_CARET, "recursion depth limit exceeded");
	execute_at(udf->at);
	recursion_depth--;
	for(i = 0; i < num_pop; i++) {
		gpfree_string(&udf->dummy_values[i]);
		udf->dummy_values[i] = save_dummy[i];
	}
}
//
// Evaluate expression   sum [i=beg:end] f(i)
// Return an integer if f(i) are all integral, complex otherwise.
// This is a change from versions 5.0 and 5.2 which always returned
// a complex value.
//
void f_sum(union argument * arg)
{
	GpValue beg, end, varname; /* [<var> = <start>:<end>] */
	udft_entry * udf;           /* function to evaluate */
	udvt_entry * udv;           /* iteration variable */
	GpValue result;        /* accummulated sum */
	GpValue f_i;
	int i;
	intgr_t llsum;              /* integer sum */
	bool integer_terms = TRUE;
	GPO.EvStk.Pop(&end);
	GPO.EvStk.Pop(&beg);
	GPO.EvStk.Pop(&varname);
	// Initialize sum to 0 
	Gcomplex(&result, 0, 0);
	llsum = 0;
	if(beg.type != INTGR || end.type != INTGR)
		GPO.IntError(NO_CARET, "range specifiers of sum must have integer values");
	if((varname.type != STRING) || !(udv = get_udv_by_name(varname.v.string_val)))
		GPO.IntError(NO_CARET, "internal error: lost iteration variable for summation");
	gpfree_string(&varname);
	udf = arg->udf_arg;
	if(!udf)
		GPO.IntError(NO_CARET, "internal error: lost expression to be evaluated during summation");
	for(i = beg.v.int_val; i<=end.v.int_val; ++i) {
		double x, y;
		// calculate f_i = f() with user defined variable i 
		Ginteger(&udv->udv_value, i);
		execute_at(udf->at);
		GPO.EvStk.Pop(&f_i);
		x = real(&result) + real(&f_i);
		y = imag(&result) + imag(&f_i);
		Gcomplex(&result, x, y);
		if(f_i.type != INTGR)
			integer_terms = FALSE;
		if(!integer_terms)
			continue;
		// So long as the individual terms are integral 
		// keep an integer sum as well.			
		llsum += f_i.v.int_val;
		// Check for integer overflow 
		if(overflow_handling == INT64_OVERFLOW_IGNORE)
			continue;
		if(sgn(result.v.cmplx_val.real) != sgn(llsum)) {
			integer_terms = FALSE;
			if(overflow_handling == INT64_OVERFLOW_TO_FLOAT)
				continue;
			if(overflow_handling == INT64_OVERFLOW_UNDEFINED)
				__IsUndefined = true;
			if(overflow_handling == INT64_OVERFLOW_NAN)
				Gcomplex(&result, not_a_number(), 0.0);
			break;
		}
	}
	if(integer_terms)
		GPO.EvStk.Push(Ginteger(&result, llsum));
	else
		GPO.EvStk.Push(&result);
}

void f_lnot(union argument * arg)
{
	GpValue a;
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, !a.v.int_val));
}

void f_bnot(union argument * arg)
{
	GpValue a;
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, ~a.v.int_val));
}

void f_lor(union argument * arg)
{
	GpValue a, b;
	GPO.EvStk.Pop(&b)->IntCheck();
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, a.v.int_val || b.v.int_val));
}

void f_land(union argument * arg)
{
	GpValue a, b;
	GPO.EvStk.Pop(&b)->IntCheck();
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, a.v.int_val && b.v.int_val));
}

void f_bor(union argument * arg)
{
	GpValue a, b;
	GPO.EvStk.Pop(&b)->IntCheck();
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, a.v.int_val | b.v.int_val));
}

void f_xor(union argument * arg)
{
	GpValue a, b;
	GPO.EvStk.Pop(&b)->IntCheck();
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, a.v.int_val ^ b.v.int_val));
}

void f_band(union argument * /*arg*/)
{
	GpValue a, b;
	GPO.EvStk.Pop(&b)->IntCheck();
	GPO.EvStk.Pop(&a)->IntCheck();
	GPO.EvStk.Push(Ginteger(&a, a.v.int_val & b.v.int_val));
}
// 
// Make all the following internal routines perform autoconversion
// from string to numeric value.
// 
#define __POP__(x) pop_or_convert_from_string(x)

void f_uminus(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
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
	GPO.EvStk.Push(&a);
}

void f_eq(union argument * /*arg*/)
{
	/* note: floating point equality is rare because of roundoff error! */
	GpValue a, b;
	int result = 0;
	__POP__(&b);
	__POP__(&a);
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
	GPO.EvStk.Push(Ginteger(&a, result));
}

void f_ne(union argument * /*arg*/)
{
	GpValue a, b;
	int result = 0;
	__POP__(&b);
	__POP__(&a);
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
	GPO.EvStk.Push(Ginteger(&a, result));
}

void f_gt(union argument * /*arg*/)
{
	GpValue a, b;
	int result = 0;
	__POP__(&b);
	__POP__(&a);
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
	GPO.EvStk.Push(Ginteger(&a, result));
}

void f_lt(union argument * arg)
{
	GpValue a, b;
	int result = 0;
	__POP__(&b);
	__POP__(&a);
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
				result = (a.v.cmplx_val.real < b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real < b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GPO.EvStk.Push(Ginteger(&a, result));
}

void f_ge(union argument * /*arg*/)
{
	GpValue a, b;
	int result = 0;
	__POP__(&b);
	__POP__(&a);
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
				result = (a.v.cmplx_val.real >= b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.cmplx_val.real >= b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GPO.EvStk.Push(Ginteger(&a, result));
}

void f_le(union argument * arg)
{
	GpValue a, b;
	int result = 0;
	__POP__(&b);
	__POP__(&a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				result = (a.v.int_val <= b.v.int_val);
				break;
			    case CMPLX:
				result = (a.v.int_val <= b.v.cmplx_val.real);
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
	GPO.EvStk.Push(Ginteger(&a, result));
}

void f_leftshift(union argument * /*arg*/)
{
	GpValue a, b, result;
	__POP__(&b);
	__POP__(&a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				Ginteger(&result, (uintgr_t)(a.v.int_val) << b.v.int_val);
				break;
			    default:
				BADINT_DEFAULT
		    }
		    break;
		default:
		    BADINT_DEFAULT
	}
	GPO.EvStk.Push(&result);
}

void f_rightshift(union argument * /*arg*/)
{
	GpValue a, b, result;
	__POP__(&b);
	__POP__(&a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				Ginteger(&result, (uintgr_t)(a.v.int_val) >> b.v.int_val);
				break;
			    default:
				BADINT_DEFAULT
		    }
		    break;
		default:
		    BADINT_DEFAULT
	}
	GPO.EvStk.Push(&result);
}

void f_plus(union argument * /*arg*/)
{
	GpValue a, b, result;
	double temp;
	__POP__(&b);
	__POP__(&a);
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				Ginteger(&result, a.v.int_val + b.v.int_val);
				/* Check for overflow */
				if(overflow_handling == INT64_OVERFLOW_IGNORE)
					break;
				temp = (double)(a.v.int_val) + (double)(b.v.int_val);
				if(sgn(temp) != sgn(result.v.int_val))
					switch(overflow_handling) {
						case INT64_OVERFLOW_TO_FLOAT:
						    Gcomplex(&result, temp, 0.0);
						    break;
						case INT64_OVERFLOW_UNDEFINED:
						    __IsUndefined = true;
						case INT64_OVERFLOW_NAN:
						    Gcomplex(&result, not_a_number(), 0.0);
						    break;
						default:
						    break;
					}
				break;
			    case CMPLX:
				Gcomplex(&result, a.v.int_val + b.v.cmplx_val.real, b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				Gcomplex(&result, b.v.int_val + a.v.cmplx_val.real, a.v.cmplx_val.imag);
				break;
			    case CMPLX:
				Gcomplex(&result, a.v.cmplx_val.real + b.v.cmplx_val.real, a.v.cmplx_val.imag + b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GPO.EvStk.Push(&result);
}

void f_minus(union argument * /*arg*/)
{
	GpValue a, b, result;
	double temp;
	__POP__(&b);
	__POP__(&a);          /* now do a - b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				Ginteger(&result, a.v.int_val - b.v.int_val);
				/* Check for overflow */
				if(overflow_handling == INT64_OVERFLOW_IGNORE)
					break;
				temp = (double)(a.v.int_val) - (double)(b.v.int_val);
				if(sgn(temp) != sgn(result.v.int_val))
					switch(overflow_handling) {
						case INT64_OVERFLOW_TO_FLOAT:
						    Gcomplex(&result, temp, 0.0);
						    break;
						case INT64_OVERFLOW_UNDEFINED:
						    __IsUndefined = true;
						case INT64_OVERFLOW_NAN:
						    Gcomplex(&result, not_a_number(), 0.0);
						    break;
						default:
						    break;
					}
				break;
			    case CMPLX:
				Gcomplex(&result, a.v.int_val - b.v.cmplx_val.real, -b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				Gcomplex(&result, a.v.cmplx_val.real - b.v.int_val, a.v.cmplx_val.imag);
				break;
			    case CMPLX:
				Gcomplex(&result, a.v.cmplx_val.real - b.v.cmplx_val.real, a.v.cmplx_val.imag - b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GPO.EvStk.Push(&result);
}

void f_mult(union argument * arg)
{
	GpValue a, b, result;
	double float_product;
	intgr_t int_product;
	__POP__(&b);
	__POP__(&a); // now do a*b 
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				/* FIXME: The test for overflow is complicated because (double)
				 * does not have enough precision to simply compare against
				 * 64-bit INTGR_MAX.
				 */
				int_product = a.v.int_val * b.v.int_val;
				float_product = (double)a.v.int_val * (double)b.v.int_val;
				if((fabs(float_product) > 2*LARGEST_GUARANTEED_NONOVERFLOW) || ((fabs(float_product) > LARGEST_GUARANTEED_NONOVERFLOW) && 
					(sgn(float_product) != sgn(int_product)))) {
					if(overflow_handling == INT64_OVERFLOW_UNDEFINED)
						__IsUndefined = true;
					if(overflow_handling == INT64_OVERFLOW_NAN)
						float_product = not_a_number();
					Gcomplex(&result, float_product, 0.0);
					break;
				}
				/* The simple case (no overflow) */
				Ginteger(&result, int_product);
				break;
			    case CMPLX:
				Gcomplex(&result, a.v.int_val * b.v.cmplx_val.real, a.v.int_val * b.v.cmplx_val.imag);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				Gcomplex(&result, b.v.int_val * a.v.cmplx_val.real, b.v.int_val * a.v.cmplx_val.imag);
				break;
			    case CMPLX:
				Gcomplex(&result, a.v.cmplx_val.real * b.v.cmplx_val.real - a.v.cmplx_val.imag * b.v.cmplx_val.imag,
				    a.v.cmplx_val.real * b.v.cmplx_val.imag + a.v.cmplx_val.imag * b.v.cmplx_val.real);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GPO.EvStk.Push(&result);
}

/*
 * Implements formula (5.4.5) from Numerical Recipes (2nd edition),
 * section "Complex Arithmetic".
 * The expression (a + i*b)/(c + i*d) is evaluated as
 *
 * [a + b(d/c)] + i[b − a(d/c)] / [c + d(d/c)] for |c| >= |d|
 *
 * and
 *
 * [a(c/d) + b] + i[b(c/d) − a] / [c(c/d) + d] for |c| <  |d|
 *
 */
void cmplx_divide(double a, double b, double c, double d, GpValue * result)
{
	double f1, f2, denom;
	/* The common case of pure real numbers has no spurious overflow */
	if(b == 0 && d == 0 && c != 0) {
		Gcomplex(result, a/c, 0.0);
	}
	else if(fabs(c) + fabs(d)) {
		if(fabs(c) >= fabs(d)) {
			f1 = 1;
			f2 = d / c;
		}
		else {
			f1 = c / d;
			f2 = 1;
		}
		denom = c * f1 + d * f2;
		Gcomplex(result, (a * f1 + b * f2) / denom, (b * f1 - a * f2) / denom);
	}
	else {
		Gcomplex(result, 0.0, 0.0);
		__IsUndefined = true;
	}
}

void f_div(union argument * arg)
{
	GpValue a, b, result;
	__POP__(&b);
	__POP__(&a);          /* now do a/b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				if(b.v.int_val)
					Ginteger(&result, a.v.int_val / b.v.int_val);
				else {
					Ginteger(&result, 0);
					__IsUndefined = true;
				}
				break;
			    case CMPLX:
				cmplx_divide((double)a.v.int_val, 0.0, b.v.cmplx_val.real, b.v.cmplx_val.imag, &result);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		case CMPLX:
		    switch(b.type) {
			    case INTGR:
				cmplx_divide(a.v.cmplx_val.real, a.v.cmplx_val.imag, (double)b.v.int_val, 0.0, &result);
				break;
			    case CMPLX:
				cmplx_divide(a.v.cmplx_val.real, a.v.cmplx_val.imag, b.v.cmplx_val.real, b.v.cmplx_val.imag, &result);
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	GPO.EvStk.Push(&result);
}

void f_mod(union argument * /*arg*/)
{
	GpValue a, b;
	__POP__(&b);
	__POP__(&a);          /* now do a%b */
	if(a.type != INTGR || b.type != INTGR)
		GPO.IntError(NO_CARET, "non-integer operand for %%");
	if(b.v.int_val)
		GPO.EvStk.Push(Ginteger(&a, a.v.int_val % b.v.int_val));
	else {
		GPO.EvStk.Push(Ginteger(&a, 0));
		__IsUndefined = true;
	}
}

void f_power(union argument * arg)
{
	GpValue a, b, result;
	int i;
	double mag, ang;
	__POP__(&b);
	__POP__(&a);          /* now find a**b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				if(a.v.int_val == 0) {
					if(b.v.int_val < 0)
						__IsUndefined = true;
					Ginteger(&result, b.v.int_val == 0 ? 1 : 0);
					break;
				}
				else if(b.v.int_val == 0) {
					Ginteger(&result, 1);
					break;
				}
				else if(b.v.int_val > 0) {
					/* DEBUG - deal with overflow by empirical check */
					intgr_t tprev, t;
					intgr_t tmag = labs(a.v.int_val);
					tprev = t = 1;
					for(i = 0; i < b.v.int_val; i++) {
						tprev = t;
						t *= tmag;
						if(t < tprev)
							goto integer_power_overflow;
					}
					if(a.v.int_val < 0) {
						if((0x1 & b.v.int_val) == 0x1)
							t = -t;
					}
					Ginteger(&result, t);
					break;
				}
integer_power_overflow:
				if(overflow_handling == INT64_OVERFLOW_NAN) {
					// result of integer overflow is NaN 
					Gcomplex(&result, not_a_number(), 0.0);
				}
				else if(overflow_handling == INT64_OVERFLOW_UNDEFINED) {
					// result of integer overflow is undefined 
					__IsUndefined = true;
				}
				else {
					// switch to double if overflow 
					mag = pow((double)a.v.int_val, (double)b.v.int_val);
					Gcomplex(&result, mag, 0.0);
				}
				break;
			    case CMPLX:
				if(a.v.int_val == 0) {
					if(b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
						__IsUndefined = true;
					}
					// return 1.0 for 0**0 
					Gcomplex(&result, b.v.cmplx_val.real == 0 ? 1.0 : 0.0, 0.0);
				}
				else {
					mag =
					    pow(magnitude(&a), fabs(b.v.cmplx_val.real));
					if(b.v.cmplx_val.real < 0.0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							__IsUndefined = true;
					}
					mag *= gp_exp(-b.v.cmplx_val.imag * angle(&a));
					ang = b.v.cmplx_val.real * angle(&a) + b.v.cmplx_val.imag * log(magnitude(&a));
					Gcomplex(&result, mag * cos(ang), mag * sin(ang));
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
					mag = pow(a.v.cmplx_val.real, fabs((double)b.v.int_val));
					if(b.v.int_val < 0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							__IsUndefined = true;
					}
					Gcomplex(&result, mag, 0.0);
				}
				else {
					/* not so good, but...! */
					mag = pow(magnitude(&a), fabs((double)b.v.int_val));
					if(b.v.int_val < 0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							__IsUndefined = true;
					}
					ang = angle(&a) * b.v.int_val;
					Gcomplex(&result, mag * cos(ang), mag * sin(ang));
				}
				break;
			    case CMPLX:
				if(a.v.cmplx_val.real == 0 && a.v.cmplx_val.imag == 0) {
					if(b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
						__IsUndefined = true;
					}
					// return 1.0 for 0**0 
					Gcomplex(&result, b.v.cmplx_val.real == 0 ? 1.0 : 0.0, 0.0);
				}
				else {
					mag = pow(magnitude(&a), fabs(b.v.cmplx_val.real));
					if(b.v.cmplx_val.real < 0.0) {
						if(mag != 0.0)
							mag = 1.0 / mag;
						else
							__IsUndefined = true;
					}
					mag *= gp_exp(-b.v.cmplx_val.imag * angle(&a));
					ang = b.v.cmplx_val.real * angle(&a) + b.v.cmplx_val.imag * log(magnitude(&a));
					Gcomplex(&result, mag * cos(ang), mag * sin(ang));
				}
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}

	/* Catch underflow and return 0 */
	/* Note: fpclassify() is an ISOC99 macro found also in other libc implementations */
#ifdef fpclassify
	if(errno == ERANGE && result.type == CMPLX) {
		int fperror = fpclassify(result.v.cmplx_val.real);
		if(fperror == FP_ZERO || fperror == FP_SUBNORMAL) {
			result.v.cmplx_val.real = 0.0;
			result.v.cmplx_val.imag = 0.0;
			errno = 0;
		}
	}
#endif
	GPO.EvStk.Push(&result);
}

void f_factorial(union argument * /*arg*/)
{
	GpValue a;
	intgr_t i;
	__POP__(&a);          /* find a! (factorial) */
	if(a.type != INTGR)
		GPO.IntError(NO_CARET, "factorial (!) argument must be an integer");

	if(((sizeof(int) == sizeof(intgr_t)) && a.v.int_val <= 12) || a.v.int_val <= 20) {
		intgr_t ival = 1;
		for(i = a.v.int_val; i > 1; i--)
			ival *= i;
		GPO.EvStk.Push(Ginteger(&a, ival));
	}
	else {
		double val = 1.0;
		for(i = a.v.int_val; i > 1; i--)
			val *= i;
		GPO.EvStk.Push(Gcomplex(&a, val, 0.0));
	}
}
#undef __POP__ // } Terminate the autoconversion from string to numeric values

void f_concatenate(union argument * /*arg*/)
{
	GpValue a, b, result;
	GPO.EvStk.Pop(&b);
	GPO.EvStk.Pop(&a);
	if(b.type == INTGR) {
		int i = b.v.int_val;
		b.type = STRING;
		b.v.string_val = (char*)gp_alloc(32, "str_const");
		snprintf(b.v.string_val, 32, "%d", i);
	}
	if(a.type != STRING || b.type != STRING)
		GPO.IntError(NO_CARET, nonstring_error);
	Gstring(&result, gp_stradd(a.v.string_val, b.v.string_val));
	gpfree_string(&a);
	gpfree_string(&b);
	GPO.EvStk.Push(&result);
	gpfree_string(&result); /* free string allocated within gp_stradd() */
}

void f_eqs(union argument * /*arg*/)
{
	GpValue a, b, result;
	GPO.EvStk.Pop(&b);
	GPO.EvStk.Pop(&a);
	if(a.type != STRING || b.type != STRING)
		GPO.IntError(NO_CARET, nonstring_error);
	Ginteger(&result, !strcmp(a.v.string_val, b.v.string_val));
	gpfree_string(&a);
	gpfree_string(&b);
	GPO.EvStk.Push(&result);
}

void f_nes(union argument * /*arg*/)
{
	GpValue a, b, result;
	GPO.EvStk.Pop(&b);
	GPO.EvStk.Pop(&a);
	if(a.type != STRING || b.type != STRING)
		GPO.IntError(NO_CARET, nonstring_error);
	Ginteger(&result, (int)(strcmp(a.v.string_val, b.v.string_val)!=0));
	gpfree_string(&a);
	gpfree_string(&b);
	GPO.EvStk.Push(&result);
}

void f_strlen(union argument * arg)
{
	GpValue a, result;
	GPO.EvStk.Pop(&a);
	if(a.type != STRING)
		GPO.IntError(NO_CARET, "internal error : strlen of non-STRING argument");
	Ginteger(&result, (int)gp_strlen(a.v.string_val));
	gpfree_string(&a);
	GPO.EvStk.Push(&result);
}

void f_strstrt(union argument * /*arg*/)
{
	GpValue needle, haystack, result;
	char * start;
	int hit = 0;
	GPO.EvStk.Pop(&needle);
	GPO.EvStk.Pop(&haystack);
	if(needle.type != STRING || haystack.type != STRING)
		GPO.IntError(NO_CARET, "internal error : non-STRING argument to strstrt");
	start = strstr(haystack.v.string_val, needle.v.string_val);
	if(start == 0) {
		hit = -1;
	}
	else if(encoding == S_ENC_UTF8) {
		char * utfstring = haystack.v.string_val;
		while(utfstring < start) {
			advance_one_utf8_char(utfstring);
			hit++;
		}
	}
	else {
		hit = (start - haystack.v.string_val);
	}
	Ginteger(&result, hit+1);
	gpfree_string(&needle);
	gpfree_string(&haystack);
	GPO.EvStk.Push(&result);
}
/*
 * f_range() handles both explicit calls to substr(string, beg, end)
 * and the short form string[beg:end].  The calls to gp_strlen() and
 * gp_strchrn() allow it to handle utf8 strings.
 */
void f_range(union argument * /*arg*/)
{
	GpValue beg, end, full;
	GpValue substr;
	int ibeg, iend;
	GPO.EvStk.Pop(&end);
	GPO.EvStk.Pop(&beg);
	GPO.EvStk.Pop(&full);
	if(beg.type == INTGR)
		ibeg = beg.v.int_val;
	else if(beg.type == CMPLX)
		ibeg = floor(beg.v.cmplx_val.real);
	else
		GPO.IntError(NO_CARET, "internal error: non-numeric substring range specifier");
	if(end.type == INTGR)
		iend = end.v.int_val;
	else if(end.type == CMPLX)
		iend = floor(end.v.cmplx_val.real);
	else
		GPO.IntError(NO_CARET, "internal error: non-numeric substring range specifier");

	if(full.type != STRING)
		GPO.IntError(NO_CARET, "internal error: substring range operator applied to non-STRING type");

	FPRINTF((stderr, "f_range( \"%s\", %d, %d)\n", full.v.string_val, beg.v.int_val, end.v.int_val));

	if(iend > gp_strlen(full.v.string_val))
		iend = gp_strlen(full.v.string_val);
	if(ibeg < 1)
		ibeg = 1;

	if(ibeg > iend) {
		GPO.EvStk.Push(Gstring(&substr, ""));
	}
	else {
		char * begp = gp_strchrn(full.v.string_val, ibeg-1);
		char * endp = gp_strchrn(full.v.string_val, iend);
		*endp = '\0';
		GPO.EvStk.Push(Gstring(&substr, begp));
	}
	gpfree_string(&full);
}

/*
 * f_index() extracts the value of a single element from an array.
 */
void f_index(union argument * arg)
{
	GpValue array, index;
	int i = -1;
	GPO.EvStk.Pop(&index);
	GPO.EvStk.Pop(&array);
	if(index.type == INTGR)
		i = index.v.int_val;
	else if(index.type == CMPLX)
		i = floor(index.v.cmplx_val.real);
	if(array.type == ARRAY) {
		if(i <= 0 || i > array.v.value_array[0].v.int_val)
			GPO.IntError(NO_CARET, "array index out of range");
		GPO.EvStk.Push(&array.v.value_array[i]);
	}
	else if(array.type == DATABLOCK) {
		i--; /* line numbers run from 1 to nlines */
		if(i < 0 || i >= datablock_size(&array))
			GPO.IntError(NO_CARET, "datablock index out of range");
		GPO.EvStk.Push(Gstring(&array, array.v.data_array[i]) );
	}
	else
		GPO.IntError(NO_CARET, "internal error: attempt to index a scalar variable");
}
/*
 * f_cardinality() extracts the number of elements in an array.
 */
void f_cardinality(union argument * arg)
{
	GpValue array;
	int size;
	GPO.EvStk.Pop(&array);
	if(array.type == ARRAY)
		size = array.v.value_array[0].v.int_val;
	else if(array.type == DATABLOCK)
		size = datablock_size(&array);
	else
		GPO.IntError(NO_CARET, "internal error: cardinality of a scalar variable");
	GPO.EvStk.Push(Ginteger(&array, size));
}

/* Magic number! */
#define RETURN_WORD_COUNT (-17*23*61)

void f_words(union argument * arg)
{
	GpValue a;
	/* "words(s)" is implemented as "word(s,RETURN_WORD_COUNT)" */
	GPO.EvStk.Push(Ginteger(&a, RETURN_WORD_COUNT));
	f_word(arg);
}

void f_word(union argument * /*arg*/)
{
	GpValue a, b, result;
	int nwords = 0;
	int in_string = 0;
	int ntarget;
	char q = '\0';
	char * s;
	if(GPO.EvStk.Pop(&b)->type != INTGR)
		GPO.IntError(NO_CARET, "internal error : non-INTGR argument");
	ntarget = b.v.int_val;
	if(GPO.EvStk.Pop(&a)->type != STRING)
		GPO.IntError(NO_CARET, "internal error : non-STRING argument");
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
		Ginteger(&result, nwords);
	GPO.EvStk.Push(&result);
	gpfree_string(&a);
}

#undef RETURN_WORD_COUNT

/* EAM July 2004
 * revised to handle 64-bit integers April 2018
 * There are probably an infinite number of things that can
 * go wrong if the user mis-matches arguments and format strings
 * in the call to sprintf, but I hope none will do worse than
 * result in a garbage output string.
 */
void f_sprintf(union argument * /*arg*/)
{
	GpValue a[10], * args;
	GpValue num_params;
	GpValue result;
	int bufsize;
	char * next_start, * outpos, tempchar;
	int next_length;
	char * prev_start;
	int prev_pos;
	int i, remaining;
	int nargs = 0;
	enum DATA_TYPES spec_type;
	char * buffer = NULL;
	char * error_return_message = NULL;
	/* Retrieve number of parameters from top of stack */
	GPO.EvStk.Pop(&num_params);
	nargs = num_params.v.int_val;
	if(nargs > 10) { /* Fall back to slow but sure allocation */
		args = (GpValue *)gp_alloc(sizeof(GpValue)*nargs, "sprintf args");
	}
	else
		args = a;
	for(i = 0; i<nargs; i++)
		GPO.EvStk.Pop(&args[i]); /* pop next argument */
	/* Make sure we got a format string of some sort */
	if(args[nargs-1].type != STRING) {
		error_return_message = "First parameter to sprintf must be a format string";
		goto f_sprintf_error_return;
	}

	/* Allocate space for the output string. If this isn't */
	/* long enough we can reallocate a larger space later. */
	bufsize = 80 + strlen(args[nargs-1].v.string_val);
	buffer = (char *)gp_alloc(bufsize, "f_sprintf");

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
	if(!evaluate_inside_using)
		set_numeric_locale();

	/* Each time we start this loop we are pointing to a % character */
	while(remaining-->0 && next_start[0] && next_start[1]) {
		GpValue * next_param = &args[remaining];
		/* Check for %%; print as literal and don't consume a parameter */
		if(!strncmp(next_start, "%%", 2)) {
			remaining++; /* Don't consume a parameter value */
			next_start++;
			next_length = strcspn(next_start+1, "%") + 1;
			prev_pos = outpos - buffer;
			while(prev_pos + next_length >= bufsize) {
				bufsize *= 2;
				buffer = (char *)gp_realloc(buffer, bufsize, "f_sprintf");
				outpos = buffer + prev_pos;
			}
			do {
				*outpos++ = *next_start++;
			} while(*next_start && *next_start != '%');
			continue;
		}

		next_length = strcspn(next_start+1, "%") + 1;
		tempchar = next_start[next_length];
		next_start[next_length] = '\0';

		spec_type = sprintf_specifier(next_start);

		/* string value <-> numerical value check */
		if(spec_type == STRING && next_param->type != STRING) {
			error_return_message = "f_sprintf: attempt to print numeric value with string format";
			goto f_sprintf_error_return;
		}
		if(spec_type != STRING && next_param->type == STRING) {
			error_return_message = "f_sprintf: attempt to print string value with numeric format";
			goto f_sprintf_error_return;
		}
		if(spec_type == INVALID_NAME) {
			error_return_message = "f_sprintf: unsupported or invalid format specifier";
			goto f_sprintf_error_return;
		}

		/* Use the format to print next arg */
		switch(spec_type) {
			case INTGR:
		    {
#if         (INTGR_MAX == INT_MAX)
			    /* This build deals only with 32-bit integers */
			    snprintf(outpos, bufsize-(outpos-buffer), next_start, (int)real(next_param));
			    break;
#else
			    intgr_t int64_val; /* The parameter value we are trying to print */
			    int int32_val; /* Copy of int64_val for sufficiently small values */

			    if(next_param->type == INTGR)
				    int64_val = next_param->v.int_val;
			    else /* FIXME: loses precision above 9007199254740992. */
				    int64_val = (intgr_t)real(next_param);

			    /* On some platforms (e.g. big-endian Solaris) trying to print a
			     * 64-bit int with %d or %x etc will fail due to using the wrong 32 bits.
			     * We try to accommodate this by converting to a 32-bit int if possible.
			     * If this overflows, replace the original format with the C99 64-bit
			     * equivalent as defined in <inttypes.h>.
			     */
			    int32_val = int64_val;
			    if((intgr_t)int32_val == int64_val)
				    snprintf(outpos, bufsize-(outpos-buffer), next_start, int32_val);
			    else {
				    /* Substitute an appropriate 64-bit format for the original one. */
				    /* INTGR return from sprintf_specifier() guarantees int_spec_post != NULL */
				    int int_spec_pos = strcspn(next_start, "diouxX");
				    char * newformat = gp_alloc(strlen(next_start) + strlen(PRId64) + 1, NULL);
				    char * new_int_spec;
				    strncpy(newformat, next_start, int_spec_pos);
				    switch(next_start[int_spec_pos]) {
					    default:
					    case 'd': new_int_spec = PRId64; break;
					    case 'i': new_int_spec = PRIi64; break;
					    case 'o': new_int_spec = PRIo64; break;
					    case 'u': new_int_spec = PRIu64; break;
					    case 'x': new_int_spec = PRIx64; break;
					    case 'X': new_int_spec = PRIX64; break;
				    }
				    strncpy(newformat+int_spec_pos, new_int_spec, strlen(new_int_spec)+1);
				    strcat(newformat, next_start+int_spec_pos+1);
				    snprintf(outpos, bufsize-(outpos-buffer), newformat, int64_val);
				    SAlloc::F(newformat);
			    }
			    break;
#endif
		    }
			case CMPLX:
			    snprintf(outpos, bufsize-(outpos-buffer),
				next_start, real(next_param));
			    break;
			case STRING:
			    snprintf(outpos, bufsize-(outpos-buffer),
				next_start, next_param->v.string_val);
			    break;
			default:
			    error_return_message = "internal error: invalid format specifier";
			    goto f_sprintf_error_return;
			    break;
		}

		next_start[next_length] = tempchar;
		next_start += next_length;
		outpos = &buffer[strlen(buffer)];

		/* Check whether previous parameter output hit the end of the buffer */
		/* If so, reallocate a larger buffer, go back and try it again.      */
		if(strlen(buffer) >= bufsize-2) {
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
	GPO.EvStk.Push(Gstring(&result, buffer));
f_sprintf_error_return:
	/* Free all locally allocated memory before returning */
	SAlloc::F(buffer);
	/* Free any strings from parameters we have now used */
	for(i = 0; i<nargs; i++)
		gpfree_string(&args[i]);
	if(args != a)
		SAlloc::F(args);
	/* Return to C locale for internal use */
	if(!evaluate_inside_using)
		reset_numeric_locale();
	if(error_return_message)
		GPO.IntError(NO_CARET, error_return_message);
}

/* EAM July 2004 - Gnuplot's own string formatting conventions.
 * Currently this routine assumes base 10 representation, because
 * it is not clear where it could be specified to be anything else.
 */
void f_gprintf(union argument * arg)
{
	GpValue fmt, val, result;
	char * buffer;
	int length;
	double base = 10.;
	/* Retrieve parameters from top of stack */
	GPO.EvStk.Pop(&val);
	GPO.EvStk.Pop(&fmt);
	/* Make sure parameters are of the correct type */
	if(fmt.type != STRING)
		GPO.IntError(NO_CARET, "First parameter to gprintf must be a format string");
	/* Make sure we have at least as much space in the output as the format itself */
	length = 80 + strlen(fmt.v.string_val);
	buffer = (char *)gp_alloc(length, "f_gprintf");
	/* Call the old internal routine */
	gprintf_value(buffer, length, fmt.v.string_val, base, &val);
	FPRINTF((stderr, " gprintf result = \"%s\"\n", buffer));
	GPO.EvStk.Push(Gstring(&result, buffer));
	gpfree_string(&fmt);
	SAlloc::F(buffer);
}

/* Output time given in seconds from year 2000 into string */
void f_strftime(union argument * arg)
{
	GpValue fmt, val;
	char * fmtstr, * buffer;
	int fmtlen, buflen, length;
	/* Retrieve parameters from top of stack */
	GPO.EvStk.Pop(&val);
	GPO.EvStk.Pop(&fmt);
	if(fmt.type != STRING)
		GPO.IntError(NO_CARET, "First parameter to strftime must be a format string");
	/* Prepare format string.
	 * Make sure the resulting string not empty by adding a space.
	 * Otherwise, the return value of gstrftime doesn't give enough
	 * information.
	 */
	fmtlen = strlen(fmt.v.string_val) + 1;
	fmtstr = (char *)gp_alloc(fmtlen + 1, "f_strftime: fmt");
	strncpy(fmtstr, fmt.v.string_val, fmtlen);
	strncat(fmtstr, " ", fmtlen);
	buflen = 80 + 2*fmtlen;
	buffer = (char *)gp_alloc(buflen, "f_strftime: buffer");

	/* Get time_str */
	length = gstrftime(buffer, buflen, fmtstr, real(&val));
	if(length == 0 || length >= buflen)
		GPO.IntError(NO_CARET, "String produced by time format is too long");

	/* Remove trailing space */
	assert(buffer[length-1] == ' ');
	buffer[length-1] = NUL;

	gpfree_string(&val);
	gpfree_string(&fmt);
	SAlloc::F(fmtstr);

	GPO.EvStk.Push(Gstring(&val, buffer));
	SAlloc::F(buffer);
}

/* Convert string into seconds from year 2000 */
void f_strptime(union argument * arg)
{
	GpValue fmt, val;
	struct tm time_tm;
	double usec = 0.0;
	double result;
	GPO.EvStk.Pop(&val);
	GPO.EvStk.Pop(&fmt);
	if(fmt.type != STRING || val.type != STRING)
		GPO.IntError(NO_CARET, "Both parameters to strptime must be strings");
	if(!fmt.v.string_val || !val.v.string_val)
		GPO.IntError(NO_CARET, "Internal error: string not allocated");
	/* string -> time_tm  plus extra fractional second */
	if(gstrptime(val.v.string_val, fmt.v.string_val, &time_tm, &usec, &result) == DT_TIMEDATE) {
		/* time_tm -> result */
		result = gtimegm(&time_tm);
		/* Add back any extra fractional second */
		result += usec;
	}
	FPRINTF((stderr, " strptime result = %g seconds \n", result));

	gpfree_string(&val);
	gpfree_string(&fmt);
	GPO.EvStk.Push(Gcomplex(&val, result, 0.0));
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
void f_time(union argument * arg)
{
	GpValue val, val2;
	double time_now;
#ifdef HAVE_SYS_TIME_H
	struct timeval tp;

	gettimeofday(&tp, NULL);
	tp.tv_sec -= SEC_OFFS_SYS;
	time_now = tp.tv_sec + (tp.tv_usec/1000000.0);
#elif defined(_WIN32)
	SYSTEMTIME systime;
	FILETIME filetime;
	ULARGE_INTEGER itime;

	/* get current system time (UTC) */
	GetSystemTime(&systime);
	/* convert to integer value in 100ns steps */
	SystemTimeToFileTime(&systime, &filetime);
	itime.HighPart = filetime.dwHighDateTime;
	itime.LowPart = filetime.dwLowDateTime;
	/* reference of this value is 1601-01-01 (no typo!) */
	/* strptime("%Y-%m-%d", "1601-01-01") = -11644473600.0 */
	time_now = itime.QuadPart * 100e-9 - 11644473600.0 - SEC_OFFS_SYS;
#else
	time_now = (double)time(NULL);
	time_now -= SEC_OFFS_SYS;
#endif
	GPO.EvStk.Pop(&val);
	switch(val.type) {
		case INTGR:
		    GPO.EvStk.Push(Ginteger(&val, (intgr_t)time_now));
		    break;
		case CMPLX:
		    GPO.EvStk.Push(Gcomplex(&val, time_now, 0.0));
		    break;
		case STRING:
		    GPO.EvStk.Push(&val); /* format string */
		    GPO.EvStk.Push(Gcomplex(&val2, time_now, 0.0));
		    f_strftime(arg);
		    break;
		default:
		    GPO.IntError(NO_CARET, "internal error: invalid argument type");
	}
}

/* Return which argument type sprintf will need for this format string:
 *   char*       STRING
 *   int         INTGR
 *   double      CMPLX
 * Should call int_err for any other type.
 * format is expected to start with '%'
 */
static enum DATA_TYPES sprintf_specifier(const char* format) {
	const char string_spec[]  = "s";
	const char real_spec[]    = "aAeEfFgG";
	const char int_spec[]     = "cdiouxX";
	/* The following characters are used to reject invalid formats */
	const char illegal_spec[] = "hlLqjzZtCSpn*";
	int string_pos, real_pos, int_pos, illegal_pos;
	/* check if really format specifier */
	if(format[0] != '%')
		GPO.IntError(NO_CARET, "internal error: sprintf_specifier called without '%'\n");
	string_pos  = strcspn(format, string_spec);
	real_pos    = strcspn(format, real_spec);
	int_pos     = strcspn(format, int_spec);
	illegal_pos = strcspn(format, illegal_spec);
	if(illegal_pos < int_pos && illegal_pos < real_pos && illegal_pos < string_pos)
		return INVALID_NAME;
	if(string_pos < real_pos && string_pos < int_pos)
		return STRING;
	if(real_pos < int_pos)
		return CMPLX;
	if(int_pos < strlen(format) )
		return INTGR;
	return INVALID_NAME;
}

/* execute a system call and return stream from STDOUT */
void f_system(union argument * arg)
{
	GpValue val, result;
	char * output;
	int output_len, ierr;
	/* Retrieve parameters from top of stack */
	GPO.EvStk.Pop(&val);
	/* Make sure parameters are of the correct type */
	if(val.type != STRING)
		GPO.IntError(NO_CARET, "non-string argument to system()");
	FPRINTF((stderr, " f_system input = \"%s\"\n", val.v.string_val));
	ierr = do_system_func(val.v.string_val, &output);
	fill_gpval_integer("GPVAL_ERRNO", ierr);
	/* chomp result */
	output_len = strlen(output);
	if(output_len > 0 && output[output_len-1] == '\n')
		output[output_len-1] = NUL;
	FPRINTF((stderr, " f_system result = \"%s\"\n", output));
	GPO.EvStk.Push(Gstring(&result, output));
	gpfree_string(&result); /* free output */
	gpfree_string(&val); /* free command string */
}

/* Variable assignment operator */
void f_assign(union argument * arg)
{
	struct udvt_entry * udv;
	GpValue a, b, index;
	GPO.EvStk.Pop(&b);  /* new value */
	GPO.EvStk.Pop(&index); /* index (only used if this is an array assignment) */
	GPO.EvStk.Pop(&a);  /* name of variable */
	if(a.type != STRING)
		GPO.IntError(NO_CARET, "attempt to assign to something other than a named variable");
	if(!strncmp(a.v.string_val, "GPVAL_", 6) || !strncmp(a.v.string_val, "MOUSE_", 6))
		GPO.IntError(NO_CARET, "attempt to assign to a read-only variable");
	if(b.type == ARRAY)
		GPO.IntError(NO_CARET, "unsupported array operation");
	udv = add_udv_by_name(a.v.string_val);
	gpfree_string(&a);
	if(udv->udv_value.type == ARRAY) {
		int i;
		if(index.type == INTGR)
			i = index.v.int_val;
		else if(index.type == CMPLX)
			i = floor(index.v.cmplx_val.real);
		else
			GPO.IntError(NO_CARET, "non-numeric array index");
		if(i <= 0 || i > udv->udv_value.v.value_array[0].v.int_val)
			GPO.IntError(NO_CARET, "array index out of range");
		gpfree_string(&udv->udv_value.v.value_array[i]);
		udv->udv_value.v.value_array[i] = b;
	}
	else {
		gpfree_string(&(udv->udv_value));
		udv->udv_value = b;
	}
	GPO.EvStk.Push(&b);
}
// 
// Retrieve the current value of a user-defined variable whose name is known.
// B = value("A") has the same result as B = A.
// 
void f_value(union argument * arg)
{
	udvt_entry * p = first_udv;
	GpValue a;
	GpValue result;
	GPO.EvStk.Pop(&a);
	if(a.type != STRING) {
		// GPO.IntWarn(NO_CARET,"non-string value passed to value()"); 
		GPO.EvStk.Push(&a);
		return;
	}
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
		/* GPO.IntWarn(NO_CARET,"undefined variable name passed to value()"); */
		result.type = CMPLX;
		result.v.cmplx_val.real = not_a_number();
		result.v.cmplx_val.imag = 0;
	}
	GPO.EvStk.Push(&result);
}
/*
 * remove leading and trailing whitespace from a string variable
 */
void f_trim(union argument * arg)
{
	GpValue a;
	char * s;
	char * trim;
	GPO.EvStk.Pop(&a);
	if(a.type != STRING)
		GPO.IntError(NO_CARET, nonstring_error);
	/* Trim from front */
	s = a.v.string_val;
	while(isspace(*s))
		s++;
	/* Trim from back */
	trim = sstrdup(s);
	s = &trim[strlen(trim)-1];
	while((s > trim) && isspace(*s))
		*(s--) = '\0';
	SAlloc::F(a.v.string_val);
	a.v.string_val = trim;
	GPO.EvStk.Push(&a);
}
