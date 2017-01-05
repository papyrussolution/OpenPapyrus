/* GNUPLOT - eval.c */

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

/* HBB 20010724: I moved several variables and functions from parse.c
 * to here, because they're involved with *evaluating* functions, not
 * with parsing them: evaluate_at(), fpe(), and fpe_env */
#include <gnuplot.h>
#pragma hdrstop
//#include "libcerf.h"
//#include "specfun.h"
//#include "standard.h"
#include "version.h"
#include <signal.h>
#include <setjmp.h>

//GpEval GpGg.Ev; // @global

GpCallFuncBlock::GpCallFuncBlock(const GpGadgets & rGg, GpArgument * pArg)
{
	AngToRad = rGg.Ang2Rad;
	P_Arg = pArg;
}
//
// Internal prototypes
//
//static RETSIGTYPE fpe(int an_int);

// The stack this operates on
//static t_value stack[STACK_DEPTH];
//static int GpGg.Ev.Sp = -1; // stack pointer 
//static int GpGg.Ev.JumpOffset; // to be modified by 'jump' operators 

//#define _top_of_stack GpGg.Ev.Stack[GpGg.Ev.Sp]

// The table of built-in functions
// These must strictly parallel enum operators in eval.h
#if 0 // {
const struct ft_entry GpEval::ft[] = {
	// internal functions: 
    {"push",         f_push},
    {"pushc",        f_pushc},
    {"pushd1",       f_pushd1},
    {"pushd2",       f_pushd2},
    {"pushd",        f_pushd},
    {"pop",          f_pop},
    {"call",         f_call},
    {"calln",        f_calln},
    {"sum",          f_sum},
    {"lnot",         f_lnot},
    {"bnot",         f_bnot},
    {"uminus",       f_uminus},
    {"lor",          f_lor},
    {"land",         f_land},
    {"bor",          f_bor},
    {"xor",          f_xor},
    {"band",         f_band},
    {"eq",           f_eq},
    {"ne",           f_ne},
    {"gt",           f_gt},
    {"lt",           f_lt},
    {"ge",           f_ge},
    {"le",           f_le},
    {"leftshift",    f_leftshift},
    {"rightshift",   f_rightshift},
    {"plus",         f_plus},
    {"minus",        f_minus},
    {"mult",         f_mult},
    {"div",          f_div},
    {"mod",          f_mod},
    {"power",        f_power},
    {"factorial",    f_factorial},
    {"bool",         f_bool},
    {"dollars",      f_dollars}, // for usespec 
    {"concatenate",  f_concatenate}, // for string variables only 
    {"eqs",          f_eqs},                // for string variables only 
    {"nes",          f_nes},                // for string variables only 
    {"[]",           f_range},               // for string variables only 
    {"[]",           f_index},               // for array variables only 
    {"assign",       f_assign},           // assignment operator '=' 
    {"jump",         f_jump},
    {"jumpz",        f_jumpz},
    {"jumpnz",       f_jumpnz},
    {"jtern",        f_jtern},
    // Placeholder for SF_START
    {"",             NULL},
#ifdef HAVE_EXTERNAL_FUNCTIONS
    {"",             f_calle},
#endif
    // legal in using spec only
    {"column",        f_column},
    {"stringcolumn",  GpDatafile::F_StringColumn }, // for using specs
    {"strcol",        GpDatafile::F_StringColumn }, // shorthand form
    {"columnhead",    f_columnhead},
    {"valid",         f_valid},
    {"timecolumn",    f_timecolumn},
    // standard functions:
    {"real",         f_real},
    {"imag",         f_imag},
    {"arg",          f_arg},
    {"conjg",        f_conjg},
    {"sin",          f_sin},
    {"cos",          f_cos},
    {"tan",          f_tan},
    {"asin",         f_asin},
    {"acos",         f_acos},
    {"atan",         f_atan},
    {"atan2",        f_atan2},
    {"sinh",         f_sinh},
    {"cosh",         f_cosh},
    {"tanh",         f_tanh},
    {"EllipticK",    f_ellip_first},
    {"EllipticE",    f_ellip_second},
    {"EllipticPi",   f_ellip_third},
    {"int",          f_int},
    {"abs",          f_abs},
    {"sgn",          f_sgn},
    {"sqrt",         f_sqrt},
    {"exp",          f_exp},
    {"log10",        f_log10},
    {"log",          f_log},
    {"besj0",        f_besj0},
    {"besj1",        f_besj1},
    {"besy0",        f_besy0},
    {"besy1",        f_besy1},
    {"erf",          f_erf},
    {"erfc",         f_erfc},
    {"gamma",        f_gamma},
    {"lgamma",       f_lgamma},
    {"ibeta",        f_ibeta},
    {"voigt",        f_voigt},
    {"igamma",       f_igamma},
    {"rand",         f_rand},
    {"floor",        f_floor},
    {"ceil",         f_ceil},
    {"defined",      f_exists},  // DEPRECATED syntax defined(foo) 

    {"norm",         f_normal},    // XXX-JG 
    {"inverf",       f_inverse_erf}, // XXX-JG 
    {"invnorm",      f_inverse_normal}, // XXX-JG 
    {"asinh",        f_asinh},
    {"acosh",        f_acosh},
    {"atanh",        f_atanh},
    {"lambertw",     f_lambertw}, // HBB, from G.Kuhnle 20001107 
    {"airy",         f_airy},     // janert, 20090905 
    {"expint",       f_expint}, // Jim Van Zandt, 20101010 
#ifdef HAVE_LIBCERF
    {"cerf",         f_cerf},       // complex error function 
    {"cdawson",      f_cdawson}, // complex Dawson's integral 
    {"erfi",         f_erfi},       // imaginary error function 
    {"VP",           f_voigtp},       // Voigt profile 
    {"faddeeva",     f_faddeeva}, // Faddeeva rescaled complex error function "w_of_z" 
#endif
    {"tm_sec",       f_tmsec},   // for timeseries 
    {"tm_min",       f_tmmin},   // for timeseries 
    {"tm_hour",      f_tmhour}, // for timeseries 
    {"tm_mday",      f_tmmday}, // for timeseries 
    {"tm_mon",       f_tmmon},   // for timeseries 
    {"tm_year",      f_tmyear}, // for timeseries 
    {"tm_wday",      f_tmwday}, // for timeseries 
    {"tm_yday",      f_tmyday}, // for timeseries 
    {"sprintf",      f_sprintf}, // for string variables only 
    {"gprintf",      f_gprintf}, // for string variables only 
    {"strlen",       f_strlen},  // for string variables only 
    {"strstrt",      f_strstrt}, // for string variables only 
    {"substr",       f_range},   // for string variables only 
    {"word",         f_word},      // for string variables only 
    {"words",        f_words},     // implemented as word(s,-1) 
    {"strftime",     f_strftime}, // time to string 
    {"strptime",     f_strptime}, // string to time 
    {"time",         f_time},       // get current time 
    {"system",       f_system},   // "dynamic backtics" 
    {"exist",        f_exists},    // exists("foo") replaces defined(foo) 
    {"exists",       f_exists},   // exists("foo") replaces defined(foo) 
    {"value",        f_value},     // retrieve value of variable known by name 
    {"hsv2rgb",      f_hsv2rgb}, // color conversion 
    {NULL,           NULL}
};
#endif // } 0

const struct ft_entry GpEval::ft[] = {
    {"push",         gpfunc_push},	
    {"pushc",        gpfunc_pushc},
    {"pushd1",       gpfunc_pushd1},
    {"pushd2",       gpfunc_pushd2},
    {"pushd",        gpfunc_pushd},
    {"pop",          gpfunc_pop},
    {"call",         gpfunc_call},
    {"calln",        gpfunc_calln},
    {"sum",          gpfunc_sum},
    {"lnot",         gpfunc_lnot},
    {"bnot",         gpfunc_bnot},
    {"uminus",       gpfunc_uminus},
    {"lor",          gpfunc_lor},
    {"land",         gpfunc_land},
    {"bor",          gpfunc_bor},
    {"xor",          gpfunc_xor},
    {"band",         gpfunc_band},
    {"eq",           gpfunc_eq},
    {"ne",           gpfunc_ne},
    {"gt",           gpfunc_gt},
    {"lt",           gpfunc_lt},
    {"ge",           gpfunc_ge},
    {"le",           gpfunc_le},
    {"leftshift",    gpfunc_leftshift},
    {"rightshift",   gpfunc_rightshift},
    {"plus",         gpfunc_plus},
    {"minus",        gpfunc_minus},
    {"mult",         gpfunc_mult},
    {"div",          gpfunc_div},
    {"mod",          gpfunc_mod},
    {"power",        gpfunc_power},
    {"factorial",    gpfunc_factorial},
    {"bool",         gpfunc_bool},
    {"dollars",      gpfunc_dollars},
    {"concatenate",  gpfunc_concatenate},
    {"eqs",          gpfunc_eqs},
    {"nes",          gpfunc_nes},
    {"[]",           gpfunc_range},
    {"[]",           gpfunc_index},
    {"assign",       gpfunc_assign},
    {"jump",         gpfunc_jump},
    {"jumpz",        gpfunc_jumpz},
    {"jumpnz",       gpfunc_jumpnz},
    {"jtern",        gpfunc_jtern},
    {"",             gpfuncNull},
    {"column",       gpfunc_column},
    {"stringcolumn", gpfunc_StringColumn },
    {"strcol",       gpfunc_StringColumn },
    {"columnhead",   gpfunc_columnhead},
    {"valid",        gpfunc_valid},
    {"timecolumn",   gpfunc_timecolumn},
    {"real",         gpfunc_real},
    {"imag",         gpfunc_imag},
    {"arg",          gpfunc_arg},
    {"conjg",        gpfunc_conjg},
    {"sin",          gpfunc_sin},
    {"cos",          gpfunc_cos},
    {"tan",          gpfunc_tan},
    {"asin",         gpfunc_asin},
    {"acos",         gpfunc_acos},
    {"atan",         gpfunc_atan},
    {"atan2",        gpfunc_atan2},
    {"sinh",         gpfunc_sinh},
    {"cosh",         gpfunc_cosh},
    {"tanh",         gpfunc_tanh},
    {"EllipticK",    gpfunc_ellip_first},
    {"EllipticE",    gpfunc_ellip_second},
    {"EllipticPi",   gpfunc_ellip_third},
    {"int",          gpfunc_int},
    {"abs",          gpfunc_abs},
    {"sgn",          gpfunc_sgn},
    {"sqrt",         gpfunc_sqrt},
    {"exp",          gpfunc_exp},
    {"log10",        gpfunc_log10},
    {"log",          gpfunc_log},
    {"besj0",        gpfunc_besj0},
    {"besj1",        gpfunc_besj1},
    {"besy0",        gpfunc_besy0},
    {"besy1",        gpfunc_besy1},
    {"erf",          gpfunc_erf},
    {"erfc",         gpfunc_erfc},
    {"gamma",        gpfunc_gamma},
    {"lgamma",       gpfunc_lgamma},
    {"ibeta",        gpfunc_ibeta},
    {"voigt",        gpfunc_voigt},
    {"igamma",       gpfunc_igamma},
    {"rand",         gpfunc_rand},
    {"floor",        gpfunc_floor},
    {"ceil",         gpfunc_ceil},
    {"defined",      gpfunc_exists},
    {"norm",         gpfunc_normal},
    {"inverf",       gpfunc_inverse_erf}, 
    {"invnorm",      gpfunc_inverse_normal},
    {"asinh",        gpfunc_asinh},
    {"acosh",        gpfunc_acosh},
    {"atanh",        gpfunc_atanh},
    {"lambertw",     gpfunc_lambertw},
    {"airy",         gpfunc_airy},   
    {"expint",       gpfunc_expint}, 
    {"tm_sec",       gpfunc_tmsec}, 
    {"tm_min",       gpfunc_tmmin}, 
    {"tm_hour",      gpfunc_tmhour},
    {"tm_mday",      gpfunc_tmmday},
    {"tm_mon",       gpfunc_tmmon}, 
    {"tm_year",      gpfunc_tmyear},
    {"tm_wday",      gpfunc_tmwday},
    {"tm_yday",      gpfunc_tmyday},
    {"sprintf",      gpfunc_sprintf},
    {"gprintf",      gpfunc_gprintf}, 
    {"strlen",       gpfunc_strlen}, 
    {"strstrt",      gpfunc_strstrt},
    {"substr",       gpfunc_range},   
    {"word",         gpfunc_word},    
    {"words",        gpfunc_words},   
    {"strftime",     gpfunc_strftime},
    {"strptime",     gpfunc_strptime},
    {"time",         gpfunc_time},
    {"system",       gpfunc_system},
    {"exist",        gpfunc_exists},
    {"exists",       gpfunc_exists},
    {"value",        gpfunc_value},
    {"hsv2rgb",      gpfunc_hsv2rgb},
#ifdef HAVE_LIBCERF  
    {"cerf",         gpfunc_cerf},   
    {"cdawson",      gpfunc_cdawson},
    {"erfi",         gpfunc_erfi},   
    {"VP",           gpfunc_voigtp}, 
    {"faddeeva",     gpfunc_faddeeva},
#endif
#ifdef HAVE_EXTERNAL_FUNCTIONS
    {"",             gpfunc_calle},
#endif
	{NULL,           gpfuncNull }
};
// Module-local variables:

static JMP_BUF fpe_env;

//static RETSIGTYPE fpe(int an_int)
RETSIGTYPE GpEval::Fpe(int an_int)
{
	(void)an_int;           
	signal(SIGFPE, (sigfunc)GpEval::Fpe);
	GpGg.Ev.undefined = true;
	LONGJMP(fpe_env, true);
}

/* Exported functions */

/* First, some functions that help other modules use 't_value' ---
 * these might justify a separate module, but I'll stick with this,
 * for now */

// returns the real part of val
//double real(const t_value * val)
double t_value::Real() const
{
	switch(type) {
		case INTGR: return ((double)v.int_val);
		case CMPLX: return (v.cmplx_val.real);
		case STRING:  /* is this ever used? */ return (atof(v.string_val));
		case NOTDEFINED: return not_a_number();
		default: GpGg.IntError(GpC, NO_CARET, "unknown type in real()");
	}
	return ((double)0.0); // NOTREACHED
}
//
// returns the real part of val, converted to int if necessary
//
int real_int(const t_value * val)
{
	switch(val->type) {
		case INTGR: return val->v.int_val;
		case CMPLX: return (int)val->v.cmplx_val.real;
		case STRING: return atoi(val->v.string_val);
		default: GpGg.IntError(GpC, NO_CARET, "unknown type in real_int()");
	}
	return 0; // NOTREACHED
}
//
// returns the imag part of val
//
double imag(const t_value * val)
{
	switch(val->type) {
		case INTGR: return (0.0);
		case CMPLX: return (val->v.cmplx_val.imag);
		case STRING:
		    // This is where we end up if the user tries:  x = 2;  plot sprintf(format,x)
		    int_warn(NO_CARET, "encountered a string when expecting a number");
		    GpGg.IntError(GpC, NO_CARET, "Did you try to generate a file name using dummy variable x or y?");
		default:
		    GpGg.IntError(GpC, NO_CARET, "unknown type in imag()");
	}
	return ((double)0.0); // NOTREACHED
}
//
// returns the magnitude of val
//
double magnitude(const t_value * val)
{
	switch(val->type) {
		case INTGR: return ((double)abs(val->v.int_val));
		case CMPLX:
	    {
		    /* The straightforward implementation sqrt(r*r+i*i)
		     * over-/underflows if either r or i is very large or very
		     * small. This implementation avoids over-/underflows from
		     * squaring large/small numbers whenever possible.  It
		     * only over-/underflows if the correct result would, too.
		     * CAVEAT: sqrt(1+x*x) can still have accuracy
		     * problems. */
		    double abs_r = fabs(val->v.cmplx_val.real);
		    double abs_i = fabs(val->v.cmplx_val.imag);
		    double quotient;
		    if(abs_i == 0)
			    return abs_r;
		    else if(abs_r > abs_i) {
			    quotient = abs_i / abs_r;
			    return abs_r * sqrt(1 + quotient*quotient);
		    }
		    else {
			    quotient = abs_r / abs_i;
			    return abs_i * sqrt(1 + quotient*quotient);
		    }
	    }
		default:
		    GpGg.IntError(GpC, NO_CARET, "unknown type in magnitude()");
	}
	return ((double)0.0); // NOTREACHED
}
//
// returns the angle of val
//
double angle(const t_value * val)
{
	switch(val->type) {
		case INTGR:
		    return ((val->v.int_val >= 0) ? 0.0 : M_PI);
		case CMPLX:
		    if(val->v.cmplx_val.imag == 0.0)
				return (val->v.cmplx_val.real >= 0.0) ? (0.0) : (M_PI);
			else
				return (atan2(val->v.cmplx_val.imag, val->v.cmplx_val.real));
		default:
		    GpGg.IntError(GpC, NO_CARET, "unknown type in angle()");
	}
	return ((double)0.0); // NOTREACHED
}

//t_value * Gcomplex(t_value * a, double realpart, double imagpart)
t_value * t_value::SetComplex(double realpart, double imagpart)
{
	type = CMPLX;
	v.cmplx_val.real = realpart;
	v.cmplx_val.imag = imagpart;
	return this;
}

//t_value * Ginteger(t_value * a, int i)
t_value * t_value::SetInt(int i)
{
	type = INTGR;
	v.int_val = i;
	return this;
}

t_value * Gstring(t_value * a, char * s)
{
	a->type = STRING;
	a->v.string_val = s;
	return (a);
}
//
// It is always safe to call gpfree_string with a->type is INTGR or CMPLX.
// However it would be fatal to call it with a->type = STRING if a->string_val
// was not obtained by a previous call to malloc(), or has already been freed.
// Thus 'a->type' is set to NOTDEFINED afterwards to make subsequent calls safe.
//
t_value * gpfree_string(t_value * a)
{
	if(a->type == STRING) {
		free(a->v.string_val);
		a->type = NOTDEFINED;
	}
	else if(a->type == ARRAY) {
		// gpfree_array() is now a separate routine. This is to help find
		// any remaining callers who expect gpfree_string to handle it.
		FPRINTF((stderr, "eval.c:%d hit array in gpfree_string()", __LINE__));
		a->type = NOTDEFINED;
	}
	return a;
}

void gpfree_array(t_value * a)
{
	if(a->type == ARRAY) {
		int size = a->v.value_array[0].v.int_val;
		for(int i = 1; i<=size; i++)
			gpfree_string(&(a->v.value_array[i]));
		free(a->v.value_array);
		a->type = NOTDEFINED;
	}
}

/* some machines have trouble with exp(-x) for large x
 * if E_MINEXP is defined at compile time, use gp_exp(x) instead,
 * which returns 0 for exp(x) with x < E_MINEXP
 * exp(x) will already have been defined as gp_exp(x) in plot.h
 */

double gp_exp(double x)
{
#ifdef E_MINEXP
	return (x < (E_MINEXP)) ? 0.0 : exp(x);
#else  /* E_MINEXP */
	int old_errno = errno;
	double result = exp(x);
	// exp(-large) quite uselessly raises ERANGE --- stop that 
	if(result == 0.0)
		errno = old_errno;
	return result;
#endif /* E_MINEXP */
}

//void reset_stack()
void GpEval::ResetStack()
{
	Sp = -1;
}

//void check_stack()
void GpEval::CheckStack()
{                               
	// make sure stack's empty 
	if(Sp != -1) {
		fprintf(stderr, "\nwarning:  internal error--stack not empty!\n(function called with too many parameters?)\n");
	}
}

bool more_on_stack()
{
	return (GpGg.Ev.Sp >= 0);
}

//t_value * pop(t_value * x)
t_value & FASTCALL GpEval::Pop(t_value & rValue)
{
	if(Sp < 0)
		GpGg.IntError(GpC, NO_CARET, "stack underflow (function call with missing parameters?)");
	rValue = Stack[Sp--];
	return rValue;
}
//
// Allow autoconversion of string variables to floats if they
// are dereferenced in a numeric context.
//
//t_value * pop_or_convert_from_string(t_value * v)
t_value & FASTCALL GpEval::PopOrConvertFromString(t_value & rValue)
{
	Pop(rValue);
	// DEBUG Dec 2014 - Consolidate sanity check for variable type 
	// FIXME: Test for INVALID_VALUE? Other corner cases? 
	if(rValue.type == INVALID_NAME)
		GpGg.IntError(GpC, NO_CARET, "invalid dummy variable name");
	if(rValue.type == STRING) {
		char * eov;
		if(*(rValue.v.string_val) && strspn(rValue.v.string_val, "0123456789 ") == strlen(rValue.v.string_val)) {
			int i = atoi(rValue.v.string_val);
			gpfree_string(&rValue);
			rValue.SetInt(i);
		}
		else {
			double d = strtod(rValue.v.string_val, &eov);
			if(rValue.v.string_val == eov) {
				gpfree_string(&rValue);
				GpGg.IntError(GpC, NO_CARET, "Non-numeric string found where a numeric expression was expected");
				/* Note: This also catches syntax errors like "set term ''*0 " */
			}
			gpfree_string(&rValue);
			rValue.SetComplex(d, 0.0);
			FPRINTF((stderr, "converted string to CMPLX value %g\n", real(pValue)));
		}
	}
	return rValue;
}

//void push(t_value * x)
void FASTCALL GpEval::Push(t_value * x)
{
	if(Sp == STACK_DEPTH - 1)
		GpGg.IntError(GpC, NO_CARET, "stack overflow");
	Stack[++Sp] = *x;
	// WARNING - This is a memory leak if the string is not later freed 
	if(x->type == STRING && x->v.string_val)
		Stack[Sp].v.string_val = gp_strdup(x->v.string_val);
#ifdef ARRAY_COPY_ON_REFERENCE
	// NOTE: Without this code, any operation during expression evaluation that 
	// alters the content of an existing array would potentially corrupt the	
	// original copy.  E.g. "Array A[3];  B=A" would result in a new variable B	
	// that points to the same content as the original array A.  This problem	
	// can be avoided by making a copy of the original array when pushing it on	
	// the evaluation stack.  Any change or persistance of the copy does not	
	// corrupt the original.  However there are two penalties from this.    
	// (1) Every reference, including retrieval of a single array element,  
	// triggers a sequence of copy/evaluate/free so it is very wasteful.    
	// (2) The lifetime of the copy is problematic.  Enabling this code in its	
	// current state will almost certainly reveal memory leaks or double-free	
	// failures.  Some compromise (detect and allow a simple copy but nothing	
	// else?) might be possible so this code is left as a starting point.   
	if(x->type == ARRAY) {
		const int array_size = x->v.value_array[0].v.int_val + 1;
		Stack[Sp].v.value_array = malloc(array_size * sizeof(t_value));
		memcpy(Stack[Sp].v.value_array, x->v.value_array, array_size*sizeof(t_value));
		for(int i = 1; i < array_size; i++)
			if(Stack[Sp].v.value_array[i].type == STRING) {
				Stack[Sp].v.value_array[i].v.string_val = _strdup(Stack[Sp].v.value_array[i].v.string_val);
			}
	}
#endif
}

void int_check(t_value & rValue)
{
	if(rValue.type != INTGR)
		GpGg.IntError(GpC, NO_CARET, "non-integer passed to boolean operator");
}
//
// This is the heart of the expression evaluation module: the stack
// program execution loop.
// 
// 'ft' is a table containing C functions within this program.
// 
// An 'action_table' contains pointers to these functions and
// arguments to be passed to them.
// 
// at_ptr is a pointer to the action table which must be executed (evaluated).
// 
// so the iterated line executes the function indexed by the at_ptr
// and passes the address of the argument which is pointed to by the arg_ptr
// 
void GpEval::ExecuteAt(AtType * pAt)
{
	const int saved_jump_offset = JumpOffset;
	const int count = pAt->a_count;
	for(int instruction_index = 0; instruction_index < count; ) {
		const int op = (int)pAt->actions[instruction_index].Index;
		JumpOffset = 1; // jump operators can modify this 
		GpArgument * p_arg = &(pAt->actions[instruction_index].arg);
		AngToRad = GpGg.Ang2Rad;
#if 0 // {
		GpEval::ft[op].Proc_(p_arg);
#else
		switch(GpEval::ft[op].FuncId) {
			case gpfuncNull: 
				break;	
			case gpfunc_StringColumn: 
				GpDatafile::F_StringColumn(p_arg);
				break;
			case gpfunc_abs: 
				f_abs(p_arg);
				break;
			case gpfunc_acos: 
				f_acos(p_arg);
				break;
			case gpfunc_acosh: 
				f_acosh(p_arg);
				break;
			case gpfunc_airy: 
				f_airy(p_arg);
				break;   
			case gpfunc_arg: 
				f_arg(p_arg);
				break;
			case gpfunc_asin: 
				f_asin(p_arg);
				break;
			case gpfunc_asinh: 
				f_asinh(p_arg);
				break;
			case gpfunc_assign: 
				f_assign(p_arg);
				break;
			case gpfunc_atan: 
				f_atan(p_arg);
				break;
			case gpfunc_atan2:
				f_atan2(p_arg);
				break;
			case gpfunc_atanh:
				f_atanh(p_arg);
				break;
			case gpfunc_band:
				f_band(p_arg);
				break;
			case gpfunc_besj0: 
				f_besj0(p_arg);
				break;
			case gpfunc_besj1: 
				f_besj1(p_arg);
				break;
			case gpfunc_besy0: 
				f_besy0(p_arg);
				break;
			case gpfunc_besy1: 
				f_besy1(p_arg);
				break;
			case gpfunc_bnot: 
				f_bnot(p_arg);
				break;
			case gpfunc_bool: 
				f_bool(p_arg);
				break;
			case gpfunc_bor: 
				f_bor(p_arg);
				break;
			case gpfunc_call: 
				f_call(p_arg);
				break;
			case gpfunc_calle: 
				f_calle(p_arg);
				break;
			case gpfunc_calln: 
				f_calln(p_arg);
				break;
#ifdef HAVE_LIBCERF
			case gpfunc_cdawson: 
				f_cdawson(p_arg);
				break;
			case gpfunc_cerf: 
				f_cerf(p_arg);
				break;   
			case gpfunc_erfi: 
				f_erfi(p_arg);
				break;   
			case gpfunc_faddeeva: 
				f_faddeeva(p_arg);
				break;
			case gpfunc_voigtp: 
				f_voigtp(p_arg);
				break; 
#endif
			case gpfunc_ceil: 
				f_ceil(p_arg);
				break;
			case gpfunc_column: 
				f_column(p_arg);
				break;
			case gpfunc_columnhead: 
				f_columnhead(p_arg);
				break;
			case gpfunc_concatenate: 
				f_concatenate(p_arg);
				break;
			case gpfunc_conjg: 
				f_conjg(p_arg);
				break;
			case gpfunc_cos: 
				f_cos(p_arg);
				break;
			case gpfunc_cosh: 
				f_cosh(p_arg);
				break;
			case gpfunc_div: 
				f_div(p_arg);
				break;
			case gpfunc_dollars: 
				f_dollars(p_arg);
				break;
			case gpfunc_ellip_first: 
				f_ellip_first(p_arg);
				break;
			case gpfunc_ellip_second: 
				f_ellip_second(p_arg);
				break;
			case gpfunc_ellip_third: 
				f_ellip_third(p_arg);
				break;
			case gpfunc_eq: 
				f_eq(p_arg);
				break;
			case gpfunc_eqs: 
				f_eqs(p_arg);
				break;
			case gpfunc_erf: 
				f_erf(p_arg);
				break;
			case gpfunc_erfc: 
				f_erfc(p_arg);
				break;
			case gpfunc_exists: 
				f_exists(p_arg);
				break;
			case gpfunc_exp: 
				f_exp(p_arg);
				break;
			case gpfunc_expint: 
				f_expint(p_arg);
				break; 
			case gpfunc_factorial: 
				f_factorial(p_arg);
				break;
			case gpfunc_floor: 
				f_floor(p_arg);
				break;
			case gpfunc_gamma: 
				f_gamma(p_arg);
				break;
			case gpfunc_ge: 
				f_ge(p_arg);
				break;
			case gpfunc_gprintf: 
				f_gprintf(p_arg);
				break; 
			case gpfunc_gt: 
				f_gt(p_arg);
				break;
			case gpfunc_hsv2rgb: 
				f_hsv2rgb(p_arg);
				break;
			case gpfunc_ibeta: 
				f_ibeta(p_arg);
				break;
			case gpfunc_igamma: 
				f_igamma(p_arg);
				break;
			case gpfunc_imag: 
				f_imag(p_arg);
				break;
			case gpfunc_index: 
				f_index(p_arg);
				break;
			case gpfunc_int: 
				f_int(p_arg);
				break;
			case gpfunc_inverse_erf: 
				f_inverse_erf(p_arg);
				break; 
			case gpfunc_inverse_normal: 
				f_inverse_normal(p_arg);
				break;
			case gpfunc_jtern: 
				f_jtern(p_arg);
				break;
			case gpfunc_jump: 
				f_jump(p_arg);
				break;
			case gpfunc_jumpnz: 
				f_jumpnz(p_arg);
				break;
			case gpfunc_jumpz: 
				f_jumpz(p_arg);
				break;
			case gpfunc_lambertw: 
				f_lambertw(p_arg);
				break;
			case gpfunc_land: 
				f_land(p_arg);
				break;
			case gpfunc_le: 
				f_le(p_arg);
				break;
			case gpfunc_leftshift: 
				f_leftshift(p_arg);
				break;
			case gpfunc_lgamma: 
				f_lgamma(p_arg);
				break;
			case gpfunc_lnot: 
				f_lnot(p_arg);
				break;
			case gpfunc_log: 
				f_log(p_arg);
				break;
			case gpfunc_log10: 
				f_log10(p_arg);
				break;
			case gpfunc_lor: 
				f_lor(p_arg);
				break;
			case gpfunc_lt: 
				f_lt(p_arg);
				break;
			case gpfunc_minus: 
				f_minus(p_arg);
				break;
			case gpfunc_mod: 
				f_mod(p_arg);
				break;
			case gpfunc_mult: 
				f_mult(p_arg);
				break;
			case gpfunc_ne: 
				f_ne(p_arg);
				break;
			case gpfunc_nes: 
				f_nes(p_arg);
				break;
			case gpfunc_normal: 
				f_normal(p_arg);
				break;
			case gpfunc_plus: 
				f_plus(p_arg);
				break;
			case gpfunc_pop: 
				f_pop(p_arg);
				break;
			case gpfunc_power: 
				f_power(p_arg);
				break;
			case gpfunc_push: 
				f_push(p_arg);
				break;	
			case gpfunc_pushc: 
				f_pushc(p_arg);
				break;
			case gpfunc_pushd: 
				f_pushd(p_arg);
				break;
			case gpfunc_pushd1: 
				f_pushd1(p_arg);
				break;
			case gpfunc_pushd2: 
				f_pushd2(p_arg);
				break;
			case gpfunc_rand: 
				f_rand(p_arg);
				break;
			case gpfunc_range: 
				f_range(p_arg);
				break;
			case gpfunc_real: 
				f_real(p_arg);
				break;
			case gpfunc_rightshift: 
				f_rightshift(p_arg);
				break;
			case gpfunc_sgn: 
				f_sgn(p_arg);
				break;
			case gpfunc_sin: 
				f_sin(p_arg);
				break;
			case gpfunc_sinh: 
				f_sinh(p_arg);
				break;
			case gpfunc_sprintf: 
				f_sprintf(p_arg);
				break;
			case gpfunc_sqrt: 
				f_sqrt(p_arg);
				break;
			case gpfunc_strftime: 
				f_strftime(p_arg);
				break;
			case gpfunc_strlen: 
				f_strlen(p_arg);
				break; 
			case gpfunc_strptime: 
				f_strptime(p_arg);
				break;
			case gpfunc_strstrt: 
				f_strstrt(p_arg);
				break;
			case gpfunc_sum: 
				f_sum(p_arg);
				break;
			case gpfunc_system: 
				f_system(p_arg);
				break;
			case gpfunc_tan: 
				f_tan(p_arg);
				break;
			case gpfunc_tanh: 
				f_tanh(p_arg);
				break;
			case gpfunc_time: 
				f_time(p_arg);
				break;
			case gpfunc_timecolumn: 
				f_timecolumn(p_arg);
				break;
			case gpfunc_tmhour: 
				f_tmhour(p_arg);
				break;
			case gpfunc_tmmday: 
				f_tmmday(p_arg);
				break;
			case gpfunc_tmmin: 
				f_tmmin(p_arg);
				break; 
			case gpfunc_tmmon: 
				f_tmmon(p_arg);
				break; 
			case gpfunc_tmsec: 
				f_tmsec(p_arg);
				break; 
			case gpfunc_tmwday: 
				f_tmwday(p_arg);
				break;
			case gpfunc_tmyday: 
				f_tmyday(p_arg);
				break;
			case gpfunc_tmyear: 
				f_tmyear(p_arg);
				break;
			case gpfunc_uminus: 
				f_uminus(p_arg);
				break;
			case gpfunc_valid: 
				f_valid(p_arg);
				break;
			case gpfunc_value: 
				f_value(p_arg);
				break;
			case gpfunc_voigt: 
				f_voigt(p_arg);
				break;
			case gpfunc_word: 
				f_word(p_arg);
				break;    
			case gpfunc_words: 
				f_words(p_arg);
				break;   
			case gpfunc_xor: 
				f_xor(p_arg);
				break;
		}
#endif // } 0 @construction 
		assert(is_jump(op) || (JumpOffset == 1));
		instruction_index += JumpOffset;
	}
	JumpOffset = saved_jump_offset;
}
//
// As of May 2013 input of Inf/NaN values through evaluation is treated
// equivalently to direct input of a formated value.  See imageNaN.dem.
//
void GpEval::EvaluateAt(AtType * at_ptr, t_value * val_ptr)
{
	undefined = false;
	errno = 0;
	ResetStack();
	if(!GpDf.evaluate_inside_using || !GpDf.df_nofpe_trap) {
		if(SETJMP(fpe_env, 1))
			return;
		signal(SIGFPE, (sigfunc)GpEval::Fpe);
	}
	ExecuteAt(at_ptr);
	if(!GpDf.evaluate_inside_using || !GpDf.df_nofpe_trap)
		signal(SIGFPE, SIG_DFL);
	if(errno == EDOM || errno == ERANGE)
		undefined = true;
	else if(!undefined) {
		Pop(*val_ptr);
		CheckStack();
	}
	if(!undefined && val_ptr->type == ARRAY) {
		int_warn(NO_CARET, "evaluate_at: unsupported array operation");
		val_ptr->type = NOTDEFINED;
	}
}

//void free_at(AtType * at_ptr)
//static
void AtType::Destroy(AtType * pAt)
{
	// All string constants belonging to this action table have to be
	// freed before destruction
	if(pAt) {
		for(int i = 0; i < pAt->a_count; i++) {
			AtEntry * p_entry = pAt->actions+i;
			// if union a->arg is used as a->arg.v_arg free potential string 
			if(oneof2(p_entry->Index, PUSHC, DOLLARS))
				gpfree_string(&p_entry->arg.v_arg);
			// a summation contains its own action table wrapped in a private udf 
			if(p_entry->Index == SUM) {
				AtType::Destroy(p_entry->arg.udf_arg->at); // @recursion
				free(p_entry->arg.udf_arg);
			}
#ifdef HAVE_EXTERNAL_FUNCTIONS
			// external function calls contain a parameter list 
			if(p_entry->Index == CALLE)
				free(p_entry->arg.exf_arg);
#endif
		}
		free(pAt);
	}
}

/* EAM July 2003 - Return pointer to udv with this name; if the key does not
 * match any existing udv names, create a new one and return a pointer to it.
 */
//UdvtEntry * GpGg.Ev.AddUdvByName(const char * key)
UdvtEntry * GpEval::AddUdvByName(const char * key)
{
	UdvtEntry ** udv_ptr = &first_udv;
	// check if it's already in the table... 
	while(*udv_ptr) {
		if(!strcmp(key, (*udv_ptr)->udv_name))
			return (*udv_ptr);
		udv_ptr = &((*udv_ptr)->next_udv);
	}
	*udv_ptr = (UdvtEntry*)malloc(sizeof(UdvtEntry));
	(*udv_ptr)->next_udv = NULL;
	(*udv_ptr)->udv_name = gp_strdup(key);
	(*udv_ptr)->udv_value.type = NOTDEFINED;
	return (*udv_ptr);
}

//UdvtEntry * get_udv_by_name(const char * key)
UdvtEntry * GpEval::GetUdvByName(const char * key) const
{
	for(UdvtEntry * udv = first_udv; udv; udv = udv->next_udv) {
		if(!strcmp(key, udv->udv_name))
			return udv;
	}
	return NULL;
}
//
// This doesn't really delete, it just marks the udv as undefined
//
//void del_udv_by_name(const char * key, bool wildcard)
void GpEval::DelUdvByName(const char * key, bool wildcard)
{
	UdvtEntry * udv_ptr = *udv_user_head;
	while(udv_ptr) {
		/* Forbidden to delete GPVAL_* */
		if(!strncmp(udv_ptr->udv_name, "GPVAL", 5))
			;
		else if(!strncmp(udv_ptr->udv_name, "GNUTERM", 7))
			;
		/* exact match */
		else if(!wildcard && !strcmp(key, udv_ptr->udv_name)) {
			gpfree_array(&(udv_ptr->udv_value));
			gpfree_string(&(udv_ptr->udv_value));
			gpfree_datablock(&(udv_ptr->udv_value));
			udv_ptr->udv_value.type = NOTDEFINED;
			break;
		}
		/* wildcard match: prefix matches */
		else if(wildcard && !strncmp(key, udv_ptr->udv_name, strlen(key)) ) {
			gpfree_array(&(udv_ptr->udv_value));
			gpfree_string(&(udv_ptr->udv_value));
			gpfree_datablock(&(udv_ptr->udv_value));
			udv_ptr->udv_value.type = NOTDEFINED;
			/* no break - keep looking! */
		}
		udv_ptr = udv_ptr->next_udv;
	}
}

// Clear (delete) all user defined functions
//void clear_udf_list()
void GpEval::ClearUdfList()
{
	UdftEntry * udf_ptr = first_udf;
	UdftEntry * udf_next;
	while(udf_ptr) {
		free(udf_ptr->udf_name);
		free(udf_ptr->definition);
		AtType::Destroy(udf_ptr->at);
		udf_next = udf_ptr->next_udf;
		free(udf_ptr);
		udf_ptr = udf_next;
	}
	first_udf = NULL;
}

//static void update_plot_bounds();
//static void fill_gpval_axis(AXIS_INDEX axis);
//static void fill_gpval_sysinfo();
static void set_gpval_axis_sth_double(const char * prefix, AXIS_INDEX axis, const char * suffix, double value, int is_int);

static void set_gpval_axis_sth_double(const char * prefix, AXIS_INDEX axis, const char * suffix, double value, int is_int)
{
	char * cc, s[24];
	sprintf(s, "%s_%s_%s", prefix, GpGg.GetAxisName(axis), suffix);
	for(cc = s; *cc; cc++)
		*cc = toupper((uchar)*cc);  /* make the name uppercase */
	UdvtEntry * v = GpGg.Ev.AddUdvByName(s);
	if(v) {
		if(is_int)
			v->udv_value.SetInt((int)(value+0.5));
		else
			v->udv_value.SetComplex(value, 0);
	}
}

//static void fill_gpval_axis(AXIS_INDEX axis)
void GpGadgets::FillGpValAxis(AXIS_INDEX axis)
{
	const char * prefix = "GPVAL";
	GpAxis * ap = &AxA[axis];
	double a = DelogValue(axis, ap->Range.low);
	double b = DelogValue(axis, ap->Range.upp);
	set_gpval_axis_sth_double(prefix, axis, "MIN", a, 0);
	set_gpval_axis_sth_double(prefix, axis, "MAX", b, 0);
	set_gpval_axis_sth_double(prefix, axis, "LOG", ap->base, 0);
	if(axis < POLAR_AXIS) {
		set_gpval_axis_sth_double("GPVAL_DATA", axis, "MIN", DelogValue(axis, ap->DataRange.low), 0);
		set_gpval_axis_sth_double("GPVAL_DATA", axis, "MAX", DelogValue(axis, ap->DataRange.upp), 0);
	}
}
//
// Fill variable "var" visible by "show var" or "show var all" ("GPVAL_*")
// by the given value (string, integer, float, complex).
//
//void fill_gpval_string(char * var, const char * stringvalue)
void GpEval::FillGpValString(char * var, const char * stringvalue)
{
	UdvtEntry * v = AddUdvByName(var);
	if(v && (v->udv_value.type != STRING || strcmp(v->udv_value.v.string_val, stringvalue) != 0)) {
		gpfree_string(&v->udv_value);
		Gstring(&v->udv_value, gp_strdup(stringvalue));
	}
}

//void fill_gpval_integer(char * var, int value)
void GpEval::FillGpValInteger(char * var, int value)
{
	UdvtEntry * v = AddUdvByName(var);
	if(v)
		v->udv_value.SetInt(value);
}

//void fill_gpval_float(char * var, double value)
void GpEval::FillGpValFloat(char * var, double value)
{
	UdvtEntry * v = AddUdvByName(var);
	if(v)
		v->udv_value.SetComplex(value, 0);
}

//void fill_gpval_complex(char * var, double areal, double aimag)
void GpEval::FillGpValComplex(char * var, double areal, double aimag)
{
	UdvtEntry * v = AddUdvByName(var);
	if(v)
		v->udv_value.SetComplex(areal, aimag);
}
//
// Export axis bounds in terminal coordinates from previous plot.
// This allows offline mapping of pixel coordinates onto plot coordinates.
//
//static void update_plot_bounds()
void GpEval::UpdatePlotBounds()
{
	FillGpValInteger("GPVAL_TERM_XMIN", (int)(GpGg[FIRST_X_AXIS].TermBounds.low / term->tscale));
	FillGpValInteger("GPVAL_TERM_XMAX", (int)(GpGg[FIRST_X_AXIS].TermBounds.upp / term->tscale));
	FillGpValInteger("GPVAL_TERM_YMIN", (int)(GpGg[FIRST_Y_AXIS].TermBounds.low / term->tscale));
	FillGpValInteger("GPVAL_TERM_YMAX", (int)(GpGg[FIRST_Y_AXIS].TermBounds.upp / term->tscale));
	FillGpValInteger("GPVAL_TERM_XSIZE", GpGg.Canvas.xright+1);
	FillGpValInteger("GPVAL_TERM_YSIZE", GpGg.Canvas.ytop+1);
	FillGpValInteger("GPVAL_TERM_SCALE", (int)term->tscale);
}

/*
 * Put all the handling for GPVAL_* variables in this one routine.
 * We call it from one of several contexts:
 * 0: following a successful set/unset command
 * 1: following a successful plot/splot
 * 2: following an unsuccessful command (int_error)
 * 3: program entry
 * 4: explicit reset of error status
 * 5: directory changed
 * 6: X11 Window ID changed
 */
//void update_gpval_variables(int context)
void GpEval::UpdateGpValVariables(int context)
{
	// These values may change during a plot command due to auto range
	if(context == 1) {
		GpGg.FillGpValAxis(FIRST_X_AXIS);
		GpGg.FillGpValAxis(FIRST_Y_AXIS);
		GpGg.FillGpValAxis(SECOND_X_AXIS);
		GpGg.FillGpValAxis(SECOND_Y_AXIS);
		GpGg.FillGpValAxis(FIRST_Z_AXIS);
		GpGg.FillGpValAxis(COLOR_AXIS);
		GpGg.FillGpValAxis(T_AXIS);
		GpGg.FillGpValAxis(U_AXIS);
		GpGg.FillGpValAxis(V_AXIS);
		FillGpValFloat("GPVAL_R_MIN", GpGg.GetR().Range.low);
		FillGpValFloat("GPVAL_R_MAX", GpGg.GetR().Range.upp);
		FillGpValFloat("GPVAL_R_LOG", GpGg.GetR().base);
		UpdatePlotBounds();
		FillGpValInteger("GPVAL_PLOT", GpGg.Is3DPlot ? 0 : 1);
		FillGpValInteger("GPVAL_SPLOT", GpGg.Is3DPlot ? 1 : 0);
		FillGpValInteger("GPVAL_VIEW_MAP", GpGg.splot_map ? 1 : 0);
		FillGpValFloat("GPVAL_VIEW_ROT_X", GpGg.surface_rot_x);
		FillGpValFloat("GPVAL_VIEW_ROT_Z", GpGg.surface_rot_z);
		FillGpValFloat("GPVAL_VIEW_SCALE", GpGg.surface_scale);
		FillGpValFloat("GPVAL_VIEW_ZSCALE", GpGg.surface_zscale);
		return;
	}
	// These are set after every "set" command, which is kind of silly
	// because they only change after 'set term' 'set output' ...
	if(context == 0 || context == 2 || context == 3) {
		// This prevents a segfault if term==NULL, which can
		// happen if set_terminal() exits via int_error().
		FillGpValString("GPVAL_TERM", term ? (char*)(term->name) : "unknown");
		FillGpValString("GPVAL_TERMOPTIONS", term_options);
		FillGpValString("GPVAL_OUTPUT", NZOR(outstr, ""));
		FillGpValString("GPVAL_ENCODING", encoding_names[encoding]);
	}
	// If we are called from int_error() then set the error state
	if(context == 2)
		FillGpValInteger("GPVAL_ERRNO", 1);
	// These initializations need only be done once, on program entry
	if(context == 3) {
		UdvtEntry * v = AddUdvByName("GPVAL_VERSION");
		char * tmp;
		if(v && v->udv_value.type == NOTDEFINED)
			v->udv_value.SetComplex(atof(gnuplot_version), 0);
		v = AddUdvByName("GPVAL_PATCHLEVEL");
		if(v && v->udv_value.type == NOTDEFINED)
			FillGpValString("GPVAL_PATCHLEVEL", gnuplot_patchlevel);
		v = AddUdvByName("GPVAL_COMPILE_OPTIONS");
		if(v && v->udv_value.type == NOTDEFINED)
			FillGpValString("GPVAL_COMPILE_OPTIONS", compile_options);
		// Start-up values 
		FillGpValInteger("GPVAL_MULTIPLOT", 0);
		FillGpValInteger("GPVAL_PLOT", 0);
		FillGpValInteger("GPVAL_SPLOT", 0);
		tmp = get_terminals_names();
		FillGpValString("GPVAL_TERMINALS", tmp);
		free(tmp);
		FillGpValString("GPVAL_ENCODING", encoding_names[encoding]);
		// Permanent copy of user-clobberable variables pi and NaN 
		FillGpValFloat("GPVAL_pi", M_PI);
		FillGpValFloat("GPVAL_NaN", not_a_number());
		// System information 
		FillGpValSysInfo();
	}
	if(context == 3 || context == 4) {
		FillGpValInteger("GPVAL_ERRNO", 0);
		FillGpValString("GPVAL_ERRMSG", "");
	}
	if(context == 3 || context == 5) {
		char * save_file = (char*)malloc(PATH_MAX);
		if(save_file) {
			GP_GETCWD(save_file, PATH_MAX);
			FillGpValString("GPVAL_PWD", save_file);
			free(save_file);
		}
	}
	if(context == 6) {
		FillGpValInteger("GPVAL_TERM_WINDOWID", GpGg.CurrentX11WindowId);
	}
}

/* System information is stored in GPVAL_BITS GPVAL_MACHINE GPVAL_SYSNAME */
#ifdef HAVE_UNAME
	#include <sys/utsname.h>
	struct utsname uts;
#elif defined(_Windows)
	#include <windows.h>
	// external header file findverion.h to find windows version from windows 2000 to 8.1
	#ifdef HAVE_FINDVERSION_H
		#include <findversion.h>
	#endif
#endif

//void fill_gpval_sysinfo()
void GpEval::FillGpValSysInfo()
{
#ifdef HAVE_UNAME // For linux/posix systems with uname
	struct utsname uts;
	if(uname(&uts) < 0)
		return;
	FillGpValString("GPVAL_SYSNAME", uts.sysname);
	FillGpValString("GPVAL_MACHINE", uts.machine);
#elif defined(_Windows) // For Windows systems
	SYSTEM_INFO stInfo;
#ifdef HAVE_FINDVERSION_H
	OSVERSIONINFOEX ret;
	int exitCode = GetVersionExEx(&ret);
	char s[30];
	snprintf(s, 30, "Windows_NT-%d.%d", ret.dwMajorVersion, ret.dwMinorVersion);
	FillGpValString("GPVAL_SYSNAME", s);
#else
	FillGpValString("GPVAL_SYSNAME", "Windows");
#endif
	GetSystemInfo(&stInfo);
	switch(stInfo.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_INTEL: FillGpValString("GPVAL_MACHINE", "x86"); break;
		case PROCESSOR_ARCHITECTURE_IA64: FillGpValString("GPVAL_MACHINE", "ia64"); break;
		case PROCESSOR_ARCHITECTURE_AMD64: FillGpValString("GPVAL_MACHINE", "x86_64"); break;
		default: FillGpValString("GPVAL_MACHINE", "unknown");
	}
#endif
	/* For all systems */
	FillGpValInteger("GPVAL_BITS", 8*sizeof(void *));
}

// Callable wrapper for the words() internal function 
//int gp_words(char * string)
int GpEval::GpWords(char * string)
{
	t_value a;
	Push(Gstring(&a, string));
	f_words((GpArgument*)NULL);
	Pop(a);
	return a.v.int_val;
}

// Callable wrapper for the word() internal function 
char * gp_word(char * string, int i)
{
	t_value a;
	GpGg.Ev.Push(Gstring(&a, string));
	GpGg.Ev.Push(a.SetInt(i));
	GpGg.Ev.f_word((GpArgument*)NULL);
	GpGg.Ev.Pop(a);
	return a.v.string_val;
}

