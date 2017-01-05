/*
 * $Id: standard.h,v 1.12 2008/03/30 03:03:48 sfeam Exp $
 */

/* GNUPLOT - standard.h */

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

#ifndef GNUPLOT_STANDARD_H
	#define GNUPLOT_STANDARD_H

double chisq_cdf(int dof, double chisqr);

// These are the more 'usual' functions built into the stack machine 
void f_real(GpArgument *x);
void f_imag(GpArgument *x);
void f_int(GpArgument *x);
void f_arg(GpArgument *x);
void f_conjg(GpArgument *x);
void f_sin(GpArgument *x);
void f_cos(GpArgument *x);
void f_tan(GpArgument *x);
void f_asin(GpArgument *x);
void f_acos(GpArgument *x);
void f_atan(GpArgument *x);
void f_atan2(GpArgument *x);
void f_sinh(GpArgument *x);
void f_cosh(GpArgument *x);
void f_tanh(GpArgument *x);
void f_asinh(GpArgument *x);
void f_acosh(GpArgument *x);
void f_atanh(GpArgument *x);
void f_ellip_first(GpArgument *x);
void f_ellip_second(GpArgument *x);
void f_ellip_third(GpArgument *x);
void f_void(GpArgument *x);
void f_abs(GpArgument *x);
void f_sgn(GpArgument *x);
void f_sqrt(GpArgument *x);
void f_exp(GpArgument *x);
void f_log10(GpArgument *x);
void f_log(GpArgument *x);
void f_floor(GpArgument *x);
void f_ceil(GpArgument *x);
void f_besj0(GpArgument *x);
void f_besj1(GpArgument *x);
void f_besy0(GpArgument *x);
void f_besy1(GpArgument *x);
void f_exists(GpArgument *x);   /* exists("foo") */
void f_tmsec(GpArgument *x);
void f_tmmin(GpArgument *x);
void f_tmhour(GpArgument *x);
void f_tmmday(GpArgument *x);
void f_tmmon(GpArgument *x);
void f_tmyear(GpArgument *x);
void f_tmwday(GpArgument *x);
void f_tmyday(GpArgument *x);
//
void f_erf(GpArgument *x);
void f_erfc(GpArgument *x);
void f_ibeta(GpArgument *x);
void f_igamma(GpArgument *x);
void f_gamma(GpArgument *x);
void f_lgamma(GpArgument *x);
void f_rand(GpArgument *x);
void f_normal(GpArgument *x);
void f_inverse_normal(GpArgument *x);
void f_inverse_erf(GpArgument *x);
void f_lambertw(GpArgument *x);
void f_airy(GpArgument *x);
void f_expint(GpArgument *x);
#ifndef HAVE_LIBCERF
	void f_voigt(GpArgument *x);
#endif

#endif // GNUPLOT_STANDARD_H 
