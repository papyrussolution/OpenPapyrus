// GNUPLOT - complexfun.c 
// Copyright Ethan A Merritt 2019
//
#include <gnuplot.h>
#pragma hdrstop

#ifdef HAVE_COMPLEX_FUNCS
#include <complex.h>    /* C99 _Complex */
#ifdef HAVE_FENV_H
	#include <fenv.h>
#endif
#ifdef HAVE_FENV_H
	#define handle_underflow(who, var) \
		if(errno) { \
			if(fetestexcept(FE_UNDERFLOW)) { \
				var = 0.0; \
				errno = 0; \
			} else { \
				fprintf(stderr, "%s: errno = %d\n", who, errno); \
			} \
		}
#else
	#define handle_underflow(who, var) IntError(NO_CARET, "%s: errno = %d", who, errno);
#endif

// internal prototypes 
static _Dcomplex lnGamma(_Dcomplex z);
//static _Dcomplex Igamma(_Dcomplex a, _Dcomplex z);
static _Dcomplex Igamma_GL(_Dcomplex a, _Dcomplex z);
//static _Dcomplex Igamma_negative_z(double a, _Dcomplex z);

#undef IGAMMA_POINCARE
#ifdef IGAMMA_POINCARE
//static _Dcomplex Igamma_Poincare(double a, _Dcomplex z);
#endif
// 
// wrapper for Igamma so that when it replaces igamma
// there is still something for old callers who want to call
// it with real arguments rather than complex.
// 
double igamma(double a, double z)
{
	return creal(GPO.__Igamma((_Dcomplex)a, (_Dcomplex)z));
}
/*
 * Complex Sign function
 * Sign(z) = z/|z| for z non-zero
 */
//void f_Sign(union argument * arg)
void GnuPlot::F_Sign(union argument * arg)
{
	GpValue result;
	GpValue a;
	_Dcomplex z;
	Pop(&a); /* Complex argument z */
	if(a.type == INTGR) {
		Push(Gcomplex(&result, sgn(a.v.int_val), 0.0));
	}
	else if(a.type == CMPLX) {
		z = a.v.cmplx_val.real + I*a.v.cmplx_val.imag;
		if(z != 0.0)
			z = z/cabs(z);
		Push(Gcomplex(&result, creal(z), cimag(z)));
	}
	else
		IntError(NO_CARET, "z must be numeric");
}
/*
 * Lambert W function for complex numbers
 *
 * W(z) is a multi-valued function with the defining property
 *
 *     z = W(z) exp(W(z))   for complex z
 *
 * LambertW( z, k ) is the kth branch of W
 *
 * This implementation guided by C++ code by István Mező <istvanmezo81@gmail.com>
 * See also
 *   R. M. Corless, G. H. Gonnet, D. E. G. Hare, D. J. Jeffrey, and D. E. Knuth,
 *   On the Lambert W function, Adv. Comput. Math. 5 (1996), no. 4, 329–359.
 *   DOI 10.1007/BF02124750.
 */

/* Internal Prototypes */
_Dcomplex lambert_initial(_Dcomplex z, int k);
//_Dcomplex LambertW(_Dcomplex z, int k);

//void f_LambertW(union argument * arg)
void GnuPlot::F_LambertW(union argument * arg)
{
	GpValue result;
	GpValue a;
	struct cmplx z; /* gnuplot complex parameter z */
	int k; /* gnuplot integer parameter k */
	_Dcomplex w; /* C99 _Complex representation */
	Pop(&a); /* Integer argument k */
	if(a.type != INTGR)
		IntError(NO_CARET, "k must be integer");
	k = a.v.int_val;
	Pop(&a); /* Complex argument z */
	if(a.type != CMPLX)
		IntError(NO_CARET, "z must be real or complex");
	z = a.v.cmplx_val;
	w = z.real + I*z.imag;
	w = _LambertW(w, k);
	Push(Gcomplex(&result, creal(w), cimag(w)));
}
/*
 * First and second derivatives for z * e^z
 * dzexpz( z )  = first derivative of ze^z = e^z + ze^z
 * ddzexpz( z ) = second derivative of ze^z = e^z + e^z + ze^z
 */
#define dzexpz(z)  (cexp(z) + z * cexp(z))
#define ddzexpz(z) (2. * cexp(z) + z * cexp(z))
/*
 * The hard part is choosing a starting point
 * since Halley's method does not have a large radius of convergence
 * EAM: The domain windows in which special case starting points are used
 *      as found in the Mező code produced glitches in my tests.
 *      I adjusted them empirically but I have no justification for
 *      the specific window sizes or thresholds.
 */
_Dcomplex lambert_initial(_Dcomplex z, int k)
{
	_Dcomplex e = 2.71828182845904523536;
	_Dcomplex branch = SMathConst::Pi2 * I * k;
	_Dcomplex ip;
	double close;
	double case1_window = 1.2; // see note above, was 1.0 
	double case2_window = 0.9; // see note above, was 1.0 
	double case3_window = 0.5; // see note above, was 0.5 
	// Initial term of Eq (4.20) from Corless et al 
	ip = clog(z) + branch - clog(clog(z) + branch);
	// Close to a branch point use (4.22) from Corless et al 
	close = cabs(z - (-1/e));
	if(close <= case1_window) {
		_Dcomplex p = csqrt(2. * (e * z + 1.) );
		if(k == 0) {
			if(creal(z) > 0 || close < case2_window)
				ip = -1.0 + p - (1.0/3.0) * p*p + (11.0/72.0) * p*p*p;
		}
#if (0)
		// This treatment empirically causes more glitches than it removes 
		if(k == 1 && cimag(z) < 0.0) {
			if(creal(z) > 0 && close < case2_window) {
				ip = -1.0 - p - (1.0/3.0) * p*p - (11.0/72.0) * p*p*p;
				if(cimag(z) > -0.1)
					ip += (-43.0/540.0) * p*p*p*p;
			}
		}
#endif
		if(k == -1 && cimag(z) > 0.) {
			if(close < case2_window)
				ip = -1.0 - p - (1.0/3.0) * p*p - (11.0/72.0) * p*p*p;
		}
	}
	// Padé approximant for W(0,a) 
	if(k == 0 && cabs(z - 0.5) <= case3_window) {
		ip = (0.35173371 * (0.1237166 + 7.061302897 * z)) / (2.0 + 0.827184 * (1.0 + 2.0 * z));
	}
	// Padé approximant for W(-1,a) 
	if(k == -1 && cabs(z - 0.5) <= case3_window) {
		ip = -(((2.2591588985 + 4.22096*I) * ((-14.073271 - 33.767687754*I) * z - (12.7127 - 19.071643*I) * (1. + 2.*z))) / (2.0 - (17.23103 - 10.629721*I) * (1.0 + 2.0*z)));
	}
	return ip;
}

//_Dcomplex LambertW(_Dcomplex z, int k)
_Dcomplex GnuPlot::_LambertW(_Dcomplex z, int k)
{
#define LAMBERT_MAXITER 300
#define LAMBERT_CONVERGENCE 1.E-13
	int i; /* iteration variable */
	double residual; /* target for convergence */
	_Dcomplex w;
	// Special cases 
	if(z == 0) {
		return (k == 0) ? 0.0 : fgetnan();
	}
	if((k == 0 || k == -1) && (fabs(creal(z) + exp(-1.0)) < LAMBERT_CONVERGENCE) && cimag(z) == 0) {
		return -1.0;
	}
	if((k == 0) && (fabs(creal(z) - exp(1.0)) < LAMBERT_CONVERGENCE) && cimag(z) == 0) {
		return 1.0;
	}
	// Halley's method requires a good starting point 
	w = lambert_initial(z, k);
	for(i = 0; i < LAMBERT_MAXITER; i++) {
		_Dcomplex wprev = w;
		_Dcomplex delta = w * cexp(w) - z;
		w -= 2.0 * (delta * dzexpz(w)) / (2.0 * dzexpz(w) * dzexpz(w) - delta * ddzexpz(w));
		residual = cabs(w - wprev);
		if(residual < LAMBERT_CONVERGENCE)
			break;
	}
	if(i >= LAMBERT_MAXITER) {
		char message[1024];
		snprintf(message, 1023, "LambertW( {%g, %g}, %d) converged only to %g", creal(z), cimag(z), k, residual);
		IntWarn(NO_CARET, message);
	}
	return w;
}

#undef dzexpz
#undef ddzexpz

/*
 * lnGamma(z) computes the natural logarithm of the Gamma function
 * valid over the entire complex plane using a Lanczos approximation.
 * The imaginary component of the result is adjusted to describe a
 * continuous surface everywhere except the negative real axis.
 *
 * This implementation copyright 2019 Ethan A Merritt
 *
 * references
 * C. Lanczos, SIAM JNA  1, 1964. pp. 86-96.
 * J. Spouge,  SIAM JNA 31, 1994. pp. 931.
 * W. Press et al, "Numerical Recipes" Section 6.1.
 */
//void f_lnGamma(union argument * arg)
void GnuPlot::F_lnGamma(union argument * arg)
{
	GpValue result;
	GpValue a;
	struct cmplx z; /* gnuplot complex parameter z */
	_Dcomplex w; /* C99 _Complex representation of z */
	Pop(&a);
	if(a.type != CMPLX)
		IntError(NO_CARET, "z must be real or complex");
	z = a.v.cmplx_val;
	/* Negative integers are pole points */
	if(z.real < 0 && fabs(z.imag) < FLT_EPSILON && fabs(z.real - round(z.real)) < FLT_EPSILON) {
		Push(Gcomplex(&result, VERYLARGE, 0.0));
		return;
	}
	/* The Lancosz approximation is valid on the half-plane with Real(z) > 0.
	 * To deal with z for which Real(z) < 0 we use the equivalence
	 *     Gamma(1-z) = (pi*z) / ( Gamma(1+z) * sin(pi*z) )
	 * to reframe the request in terms of a "reflected" z.
	 */
	if(z.real < 0.5) {
		_Dcomplex lnpi  = 1.14472988584940017414342735;
		_Dcomplex t1;
		double treal, timag;
		w = (1.0 - z.real) + I*(-z.imag);
		w = lnGamma(w);
		t1 = clog(csin(SMathConst::Pi*z.real + I*SMathConst::Pi*z.imag));
		treal = lnpi - creal(w) - creal(t1);
		timag = -cimag(w) - cimag(t1);
		/* Shift result by 2pi to maintain a continuous surface
		 * other than the discontinuity at the negative real axis
		 */
		timag += sgn(z.imag) * SMathConst::Pi2 * floor((z.real+0.5)/2.);
		Push(Gcomplex(&result, treal, timag));
	}
	else {
		w = z.real + I*z.imag;
		w = lnGamma(w);
		Push(Gcomplex(&result, creal(w), cimag(w)));
	}
}

static _Dcomplex lnGamma(_Dcomplex z)
{
	static const double coef[15] = {
		0.99999999999999709182,
		57.156235665862923517,    -59.597960355475491248,
		14.136097974741747174,     -0.49191381609762019978,
		0.33994649984811888699e-4, 0.46523628927048575665e-4,
		-0.98374475304879564677e-4, 0.15808870322491248884e-3,
		-0.21026444172410488319e-3, 0.21743961811521264320e-3,
		-0.16431810653676389022e-3, 0.84418223983852743293e-4,
		-0.26190838401581408670e-4, 0.36899182659531622704e-5
	};
	_Dcomplex sqrt2pi = 2.5066282746310005;
	_Dcomplex g = 671.0/128.0;
	_Dcomplex sum, temp;
	_Dcomplex f;
	int k;
	// 15 term Lanczos approximation 
	sum = coef[0];
	for(k = 1; k<15; k++)
		sum = sum + coef[k] / (z + k);
	if(z == 1.0 || z == 2.0)
		return 0.0;
	// Numerical Recipes formulation 
	temp = z + g;
	temp = (z + 0.5) * clog(temp) - temp;
	f = temp + clog(sqrt2pi * sum/z);
	return f;
}

/*
 * TODO
 *	- extend to full complex plane e.g. Temme 1996 or Gil 2016
 */
#define IGAMMA_PRECISION 1.E-14
#define MAXLOG 708.396418532264106224   /* log(2**1022) */

//void f_Igamma(union argument * arg)
void GnuPlot::F_IGamma(union argument * arg)
{
	GpValue result;
	GpValue tmp;
	struct cmplx a; // gnuplot complex parameter a 
	struct cmplx z; // gnuplot complex parameter z 
	_Dcomplex w; // C99 _Complex representation 
	Pop(&tmp); /* Complex argument z */
	if(tmp.type == CMPLX)
		z = tmp.v.cmplx_val;
	else {
		z.real = Real(&tmp);
		z.imag = 0;
	}
	Pop(&tmp); /* Complex argument a */
	if(tmp.type == CMPLX)
		a = tmp.v.cmplx_val;
	else {
		a.real = Real(&tmp);
		a.imag = 0;
	}
	w = __Igamma(a.real + I * a.imag, z.real + I * z.imag);
	if(w == -1) {
		// Failed to converge or other error 
		Push(Gcomplex(&result, fgetnan(), 0));
		return;
	}
	Push(Gcomplex(&result, creal(w), cimag(w)));
}
/* Igamma(a, z)
 *   lower incomplete gamma function P(a, z).
 *
 *                  1      z  -t   (a-1)
 *       P(a, z) = ---- * ∫  e  * t     dt
 *                 Γ(a)    0
 *
 *   complex a,	real(a) > 0
 *   complex z
 *
 *   adapted from previous gnuplot real-valued function igamma(double a, double x)
 *
 *   REFERENCE ALGORITHM AS239  APPL. STATIST. (1988) VOL. 37, NO. 3
 *   B. L. Shea "Chi-Squared and Incomplete Gamma Integral"
 *
 *   See also:
 *   N. M. Temme (1994)
 *   Probability in the Engineering and Informational Sciences 8: 291-307.
 *
 *   Coefficients for Gauss-Legendre quadrature:
 *   Press et al, Numerical Recipes (3rd Ed.) Section 6.2
 *
 */
//static _Dcomplex Igamma(_Dcomplex a, _Dcomplex z)
_Dcomplex GnuPlot::__Igamma(_Dcomplex a, _Dcomplex z)
{
	// 
	// Various complex functions like cexp may set errno on underflow
	// We would prefer to return 0.0 rather than NaN
	// 
	#ifdef HAVE_FENV_H
		#define initialize_underflow(who) \
			if(errno) \
				IntError(NO_CARET, "%s: error present on entry (errno %d %s)", who, errno, strerror(errno)); \
			else feclearexcept(FE_ALL_EXCEPT);
	#else
		#define initialize_underflow(who) \
			if(errno) \
				IntError(NO_CARET, "%s: error present on entry (errno %d %s)", who, errno, strerror(errno));
	#endif
	_Dcomplex arg, ga1;
	_Dcomplex aa;
	_Dcomplex an;
	_Dcomplex b;
	int i;
	// Check that we have valid values for a and z 
	if(creal(a) <= 0.0)
		return -1.0;
	// Deal with special cases 
	if(z == 0.0)
		return 0.0;
	errno = 0;
	initialize_underflow("Igamma");
	// For real(z) < 0 convergence of the standard series is poor.
	// We catch these cases ahead of time and use a different algorithm.
	// See Gil et al (2016) ACM TOMS 43:3 Article 26
	// Note this alternative only accepts real a.
	if(creal(z) < 0 && cimag(a) == 0) {
#ifdef IGAMMA_POINCARE
		// Case 5:
		// Gil (2016) suggests using a Poincaré-like expansion when |z| > 50.
		// However in my tests this seemed worse than cases 1-3.
		if(debug == 5)
			if(creal(z) < -50.)
				return __Igamma_Poincare(creal(a), -z);
#endif
		// Case 3:
		// Abramowitz & Stegum (6.5.29)
		if(debug != 3)
			if(creal(a) < 75.0)
				return __Igamma_negative_z(creal(a), z);
	} // End special cases for real(z) < 0 
	// Case 1:
	// EAM 2020: For large values of a convergence fails.
	// Use Gauss-Legendre quadrature instead.
	if((cabs(a) > 100.) && (cabs(z-a)/cabs(a) < 0.2)) {
		if(debug == 1) return NAN;
		return Igamma_GL(a, z);
	}
	// Check value of factor arg 
	ga1 = lnGamma(a + 1.0);
	arg = a * clog(z) - z - ga1;
	arg = cexp(arg);
	// Underflow of arg is common for large z or a 
	handle_underflow("Igamma", arg);
	// Choose infinite series or continued fraction. 
	if((cabs(z) > 1.0) && (cabs(z) >= cabs(a) + 2.0)) {
		// Case 2:
		// Use a continued fraction expansion
		_Dcomplex pn1, pn2, pn3, pn4, pn5, pn6;
		_Dcomplex rn;
		_Dcomplex rnold;
		if(debug == 2) 
			return NAN;
		aa = 1.0 - a;
		b = aa + z + 1.0;
		pn1 = 1.0;
		pn2 = z;
		pn3 = z + 1.0;
		pn4 = z * b;
		rnold = pn3 / pn4;
		for(i = 1; i <= 2000; i++) {
			aa += 1.0;
			b += 2.0;
			an = aa * (double)i;
			pn5 = b * pn3 - an * pn1;
			pn6 = b * pn4 - an * pn2;
			// Serious overflow 
			if(isnan(cabs(pn5)) || isnan(cabs(pn6))) {
				IntWarn(NO_CARET, "Igamma: overflow");
				return -1.0;
			}
			if(pn6 != 0.0) {
				rn = pn5 / pn6;
				if(cabs(rnold - rn) <= MIN(IGAMMA_PRECISION, IGAMMA_PRECISION * cabs(rn))) {
					return 1.0 - arg * rn * a;
				}
				rnold = rn;
			}
			pn1 = pn3;
			pn2 = pn4;
			pn3 = pn5;
			pn4 = pn6;
			/* Re-scale terms in continued fraction if they are large */
#define IGAMMA_OVERFLOW  FLT_MAX
			if(cabs(pn5) >= IGAMMA_OVERFLOW) {
				pn1 /= IGAMMA_OVERFLOW;
				pn2 /= IGAMMA_OVERFLOW;
				pn3 /= IGAMMA_OVERFLOW;
				pn4 /= IGAMMA_OVERFLOW;
			}
		}
	}
	else {
		/* Case 4:
		 * Use Pearson's series expansion.
		 */
		_Dcomplex retval;
		if(debug == 4) 
			return NAN;
		for(i = 0, aa = a, an = b = 1.0; i <= 1000; i++) {
			aa += 1.0;
			an *= z / aa;
			handle_underflow("Igamma", an);
			b += an;
			retval = arg * b;
			if(cabs(an) < cabs(b) * IGAMMA_PRECISION)
				return retval;
		}
	}
	// Convergence failed 
	if(!errno)
		IntWarn(NO_CARET, "Igamma: no convergence after %d iterations residual %g", i, cabs(an));
	return -1.0;
}

/* icomplete gamma function evaluated by Gauss-Legendre quadrature
 * as recommended for large values of a by Numerical Recipes (Sec 6.2).
 */
static _Dcomplex Igamma_GL(_Dcomplex a, _Dcomplex z)
{
	static const double y[18] = {
		0.0021695375159141994,
		0.011413521097787704, 0.027972308950302116, 0.051727015600492421,
		0.082502225484340941, 0.12007019910960293, 0.16415283300752470,
		0.21442376986779355, 0.27051082840644336, 0.33199876341447887,
		0.39843234186401943, 0.46931971407375483, 0.54413605556657973,
		0.62232745288031077, 0.70331500465597174, 0.78649910768313447,
		0.87126389619061517, 0.95698180152629142
	};
	static const double w[18] = {
		0.0055657196642445571,
		0.012915947284065419, 0.020181515297735382, 0.027298621498568734,
		0.034213810770299537, 0.040875750923643261, 0.047235083490265582,
		0.053244713977759692, 0.058860144245324798, 0.064039797355015485,
		0.068745323835736408, 0.072941885005653087, 0.076598410645870640,
		0.079687828912071670, 0.082187266704339706, 0.084078218979661945,
		0.085346685739338721, 0.085983275670394821
	};
	_Dcomplex xu, t, ans;
	_Dcomplex a1 = a - 1.0;
	_Dcomplex lna1 = clog(a1);
	_Dcomplex sqrta1 = csqrt(a1);
	_Dcomplex sum = 0.0;
	int j;
	if(cabs(z) > cabs(a1))
		xu = MAX(cabs(a1 + 11.5 * sqrta1),  cabs(z + 6.0 * sqrta1) );
	else
		xu = MIN(cabs(a1 - 7.5 * sqrta1),  cabs(z - 5.0 * sqrta1) );

	/* FIXME:  I don't know what the complex equivalent is for this test */
	/* if (xu < 0) xu = 0.0; */

	for(j = 0; j<18; j++) {
		t = z + (xu - z) * y[j];
		sum += w[j] * cexp(-(t-a1) + a1 * (clog(t) - lna1));
	}

	ans = sum * (xu-z) * cexp(a1 * (lna1-1.) - lnGamma(a) );

	if(cabs(z) > cabs(a1))
		return 1.0 - ans;
	else
		return -ans;
}

/* Incomplete gamma function for negative z
 * computed using a series expansion for gamma*
 *
 *            1    inf   (-z)^k
 *   gamma*(a,z) = ---  Sum    ------
 *           Γ(a)  k=0   k!(a+k)
 *
 * Abramowitz & Stegun (6.5.29) = Paris (8.7.1)
 */
//static _Dcomplex Igamma_negative_z(double a, _Dcomplex z)
_Dcomplex GnuPlot::__Igamma_negative_z(double a, _Dcomplex z)
{
	_Dcomplex t = 1/a;
	_Dcomplex v = t;
	double p;
	int k;
	for(k = 0; k<1000; k++) {
		p = (a + k) / (a + k + 1);
		t *= -z * p / (k+1);
		v += t;
		if(!(cabs(t) < VERYLARGE)) {
			IntWarn(NO_CARET, "Igamma: overflow");
			return -1.0;
		}
		if(cabs(t/v) < IGAMMA_PRECISION)
			break;
	}
	if(k >= 1000)
		IntWarn(NO_CARET, "Igamma: no convergence after %d iterations residual %g", k, cabs(t/v));
	// At this point v is gamma* 
	// FIXME: Do we have to handle underflow/overflow? 
	// NB: a is real, so cexp(lnGamma(a)) could be exp(LGAMMA(a)) 
	t = v * cpow(z, a) / cexp(lnGamma(a));
	return t;
}

#ifdef IGAMMA_POINCARE
/* Incomplete gamma function computed using a Poincaré expansion
 *
 *            e^z    inf   (1-a)_k
 *   gamma*(a,z) ~ -----  Sum    ------
 *           zΓ(a)   k=0     z^k
 *
 * Suggested by Gil (2016) for a>0 and |z| > 50.
 * ACM TOMS 43, 3, Article 26  DOI: http://dx.doi.org/10.1145/2972951
 * Eq (29)
 */
//static _Dcomplex Igamma_Poincare(double a, _Dcomplex z)
_Dcomplex GnuPlot::__Igamma_Poincare(double a, _Dcomplex z)
{
	_Dcomplex t = 1.0;
	_Dcomplex v = t;
	double p;
	int k;
	for(k = 0; k<1000; k++) {
		p = (1. - a) + k;
		t *= p/z;
		v += t;
		if(!(cabs(t) < VERYLARGE)) {
			IntWarn(NO_CARET, "Igamma: overflow");
			return -1.0;
		}
		if(cabs(t/v) < IGAMMA_PRECISION)
			break;
	}
	if(k >= 1000)
		IntWarn(NO_CARET, "Igamma: no convergence after %d iterations residual %g", k, cabs(t/v));
	t = v * cexp(z) / (z * cexp(lnGamma(a))); /* NB: a is real, so cexp(lnGamma(a)) could be exp(LGAMMA(a)) */
	t *= cpow(z, a); /* convert from gamma* to igamma */
	return t;
}

#endif

#endif /* HAVE_COMPLEX_FUNCS */
