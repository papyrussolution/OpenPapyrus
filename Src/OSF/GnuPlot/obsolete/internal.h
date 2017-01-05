/*
 * $Id: internal.h,v 1.26 2016/02/07 22:15:36 sfeam Exp $
 */

/* GNUPLOT - internal.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_INTERNAL_H
# define GNUPLOT_INTERNAL_H

/* #if... / #include / #define collection: */

//#include "syscfg.h"
//#include "gp_types.h"
//#include "eval.h"

/* Prototypes from file "internal.c" */
void eval_reset_after_error();

/* the basic operators of our stack machine for function evaluation: */
void f_push(GpArgument *x);
void f_pushc(GpArgument *x);
void f_pushd1(GpArgument *x);
void f_pushd2(GpArgument *x);
void f_pushd(GpArgument *x);
void f_pop(GpArgument *x);
void f_call(GpArgument *x);
void f_calln(GpArgument *x);
void f_sum(GpArgument *x);
void f_lnot(GpArgument *x);
void f_bnot(GpArgument *x);
void f_lor(GpArgument *x);
void f_land(GpArgument *x);
void f_bor(GpArgument *x);
void f_xor(GpArgument *x);
void f_band(GpArgument *x);
void f_uminus(GpArgument *x);
void f_eq(GpArgument *x);
void f_ne(GpArgument *x);
void f_gt(GpArgument *x);
void f_lt(GpArgument *x);
void f_ge(GpArgument *x);
void f_le(GpArgument *x);
void f_leftshift(GpArgument *x);
void f_rightshift(GpArgument *x);
void f_plus(GpArgument *x);
void f_minus(GpArgument *x);
void f_mult(GpArgument *x);
void f_div(GpArgument *x);
void f_mod(GpArgument *x);
void f_power(GpArgument *x);
void f_factorial(GpArgument *x);

void f_concatenate(GpArgument *x);
void f_eqs(GpArgument *x);
void f_nes(GpArgument *x);
void f_gprintf(GpArgument *x);
void f_range(GpArgument *x);
void f_index(GpArgument *x);
void f_sprintf(GpArgument *x);
void f_strlen(GpArgument *x);
void f_strstrt(GpArgument *x);
void f_system(GpArgument *x);
void f_word(GpArgument *x);
void f_words(GpArgument *x);
void f_strftime(GpArgument *x);
void f_strptime(GpArgument *x);
void f_time(GpArgument *x);
void f_assign(GpArgument *x);
void f_value(GpArgument *x);

#endif /* GNUPLOT_INTERNAL_H */
