/*
 * $Id: libcerf.c,v 1.1 2013/07/13 05:52:43 sfeam Exp $
 */
/*
 * Complex error function (cerf) and related special functions from libcerf.
 * libcerf itself uses the C99 _Complex mechanism for decribing complex
 * numbers.  This set of wrapper routines converts back and forth from
 * gnuplot's own representation of complex numbers.
 * Ethan A Merritt - July 2013
 */
#include <gnuplot.h>
#pragma hdrstop
#ifdef HAVE_LIBCERF
#include <complex.h>    /* C99 _Complex */
#include <cerf.h>       /* libcerf library header */
#include "stdfn.h"      /* for not_a_number */
//#include "libcerf.h"    /* our own prototypes */

/* The libcerf complex error function
 *     cerf(z) = 2/sqrt(pi) * int[0,z] exp(-t^2) dt
 */
void f_cerf(GpArgument * pArg)
{
	t_value a;
	complex double z;
	pop(&a);
	z = a.Real() + I * imag(&a);    /* Convert gnuplot complex to C99 complex */
	z = cerf(z);                    /* libcerf complex -> complex function */
	push(a.SetComplex(creal(z), cimag(z)));
}

/* The libcerf cdawson function returns Dawson's integral
 *      cdawson(z) = exp(-z^2) int[0,z] exp(t^2) dt
 *                 = sqrt(pi)/2 * exp(-z^2) * erfi(z)
 * for complex z.
 */
void f_cdawson(GpArgument * pArg)
{
	t_value a;
	complex double z;
	pop(&a);
	z = a.Real() + I * imag(&a);    /* Convert gnuplot complex to C99 complex */
	z = cdawson(z);                 /* libcerf complex -> complex function */
	push(a.SetComplex(creal(z), cimag(z)));
}

/* The libcerf routine w_of_z returns the Faddeeva rescaled complex error function
 *     w(z) = exp(-z^2) * erfc(-i*z)
 * This corresponds to Abramowitz & Stegun Eqs. 7.1.3 and 7.1.4
 * This is also known as the plasma dispersion function.
 */
void f_faddeeva(GpArgument * pArg)
{
	t_value a;
	complex double z;
	pop(&a);
	z = a.Real() + I * imag(&a);    /* Convert gnuplot complex to C99 complex */
	z = w_of_z(z);                  /* libcerf complex -> complex function */
	push(a.SetComplex(creal(z), cimag(z)));
}

/* The libcerf voigt(z, sigma, gamma) function returns the Voigt profile
 * corresponding to the convolution
 *     voigt(x,sigma,gamma) = integral G(t,sigma) L(x-t,gamma) dt
 * of Gaussian
 *     G(x,sigma) = 1/sqrt(2*pi)/|sigma| * exp(-x^2/2/sigma^2)
 * with Lorentzian
 *     L(x,gamma) = |gamma| / pi / ( x^2 + gamma^2 )
 * over the integral from -infinity to +infinity.
 */
void f_voigtp(GpArgument * pArg)
{
	t_value a;
	double z, sigma, gamma;
	gamma = pop(&a)->Real();
	sigma = pop(&a)->Real();
	z = pop(&a)->Real();
	z = voigt(z, sigma, gamma);     /* libcerf double -> double function */
	push(a.SetComplex(z, 0.0));
}

/* The libcery routine re_w_of_z( double x, double y )
 * is equivalent to the previously implemented gnuplot routine voigt(x,y).
 * Use it in preference to the previous one.
 */
void f_voigt(GpArgument * pArg)
{
	t_value a;
	double y = pop(&a)->Real();
	double x = pop(&a)->Real();
	double w = re_w_of_z(x, y);
	push(a.SetComplex(w, 0.0));
}

/* erfi(z) = -i * erf(iz) */
void f_erfi(GpArgument * pArg)
{
	t_value a;
	double z = pop(&a)->Real();
	push(a.SetComplex(erfi(z), 0.0));
}

#endif
