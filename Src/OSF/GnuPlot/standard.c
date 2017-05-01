/* GNUPLOT - standard.c */

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

static double carlson_elliptic_rc(double x, double y);
static double carlson_elliptic_rf(double x, double y, double z);
static double carlson_elliptic_rd(double x, double y, double z);
static double carlson_elliptic_rj(double x, double y, double z, double p);
static double jzero(double x);
static double pzero(double x);
static double qzero(double x);
static double yzero(double x);
static double rj0(double x);
static double ry0(double x);
static double jone(double x);
static double pone(double x);
static double qone(double x);
static double yone(double x);
static double rj1(double x);
static double ry1(double x);
//
// The bessel function approximations here are from
// "Computer Approximations" by Hart, Cheney et al. John Wiley & Sons, 1968
//
//
/* There appears to be a mistake in Hart, Cheney et al. on page 149.
 * Where it list Qn(x)/x ~ P(z*z)/Q(z*z), z = 8/x, it should read
 *               Qn(x)/z ~ P(z*z)/Q(z*z), z = 8/x
 * In the functions below, Qn(x) is implementated using the later
 * equation.
 * These bessel functions are accurate to about 1e-13
 */

#define THREE_PI_ON_FOUR 2.35619449019234492884698253745962716
#define TWO_ON_PI        0.63661977236758134307553505349005744

static const double dzero = 0.0;

/* jzero for x in [0,8]
 * Index 5849, 19.22 digits precision
 */
static const double   pjzero[] = {
	0.4933787251794133561816813446e+21,
	-0.11791576291076105360384408e+21,
	0.6382059341072356562289432465e+19,
	-0.1367620353088171386865416609e+18,
	0.1434354939140346111664316553e+16,
	-0.8085222034853793871199468171e+13,
	0.2507158285536881945555156435e+11,
	-0.4050412371833132706360663322e+8,
	0.2685786856980014981415848441e+5
};

static const double   qjzero[] = {
	0.4933787251794133562113278438e+21,
	0.5428918384092285160200195092e+19,
	0.3024635616709462698627330784e+17,
	0.1127756739679798507056031594e+15,
	0.3123043114941213172572469442e+12,
	0.669998767298223967181402866e+9,
	0.1114636098462985378182402543e+7,
	0.1363063652328970604442810507e+4,
	0.1e+1
};

/* pzero for x in [8,inf]
 * Index 6548, 18.16 digits precision
 */
static const double ppzero[] = {
	0.2277909019730468430227002627e+5,
	0.4134538663958076579678016384e+5,
	0.2117052338086494432193395727e+5,
	0.348064864432492703474453111e+4,
	0.15376201909008354295771715e+3,
	0.889615484242104552360748e+0
};

static const double qpzero[] = {
	0.2277909019730468431768423768e+5,
	0.4137041249551041663989198384e+5,
	0.2121535056188011573042256764e+5,
	0.350287351382356082073561423e+4,
	0.15711159858080893649068482e+3,
	0.1e+1
};

/* qzero for x in [8,inf]
 * Index 6948, 18.33 digits precision
 */
static const double   pqzero[] = {
	-0.8922660020080009409846916e+2,
	-0.18591953644342993800252169e+3,
	-0.11183429920482737611262123e+3,
	-0.2230026166621419847169915e+2,
	-0.124410267458356384591379e+1,
	-0.8803330304868075181663e-2,
};

static const double   qqzero[] = {
	0.571050241285120619052476459e+4,
	0.1195113154343461364695265329e+5,
	0.726427801692110188369134506e+4,
	0.148872312322837565816134698e+4,
	0.9059376959499312585881878e+2,
	0.1e+1
};

/* yzero for x in [0,8]
 * Index 6245, 18.78 digits precision
 */
static const double   pyzero[] = {
	-0.2750286678629109583701933175e+20,
	0.6587473275719554925999402049e+20,
	-0.5247065581112764941297350814e+19,
	0.1375624316399344078571335453e+18,
	-0.1648605817185729473122082537e+16,
	0.1025520859686394284509167421e+14,
	-0.3436371222979040378171030138e+11,
	0.5915213465686889654273830069e+8,
	-0.4137035497933148554125235152e+5
};

static const double   qyzero[] = {
	0.3726458838986165881989980739e+21,
	0.4192417043410839973904769661e+19,
	0.2392883043499781857439356652e+17,
	0.9162038034075185262489147968e+14,
	0.2613065755041081249568482092e+12,
	0.5795122640700729537380087915e+9,
	0.1001702641288906265666651753e+7,
	0.1282452772478993804176329391e+4,
	0.1e+1
};
//
// jone for x in [0,8]
// Index 6050, 20.98 digits precision
//
static const double   pjone[] = {
	0.581199354001606143928050809e+21,
	-0.6672106568924916298020941484e+20,
	0.2316433580634002297931815435e+19,
	-0.3588817569910106050743641413e+17,
	0.2908795263834775409737601689e+15,
	-0.1322983480332126453125473247e+13,
	0.3413234182301700539091292655e+10,
	-0.4695753530642995859767162166e+7,
	0.270112271089232341485679099e+4
};

static const double   qjone[] = {
	0.11623987080032122878585294e+22,
	0.1185770712190320999837113348e+20,
	0.6092061398917521746105196863e+17,
	0.2081661221307607351240184229e+15,
	0.5243710262167649715406728642e+12,
	0.1013863514358673989967045588e+10,
	0.1501793594998585505921097578e+7,
	0.1606931573481487801970916749e+4,
	0.1e+1
};

/* pone for x in [8,inf]
 * Index 6749, 18.11 digits precision
 */
static const double   ppone[] = {
	0.352246649133679798341724373e+5,
	0.62758845247161281269005675e+5,
	0.313539631109159574238669888e+5,
	0.49854832060594338434500455e+4,
	0.2111529182853962382105718e+3,
	0.12571716929145341558495e+1
};

static const double   qpone[] = {
	0.352246649133679798068390431e+5,
	0.626943469593560511888833731e+5,
	0.312404063819041039923015703e+5,
	0.4930396490181088979386097e+4,
	0.2030775189134759322293574e+3,
	0.1e+1
};

/* qone for x in [8,inf]
 * Index 7149, 18.28 digits precision
 */
static const double   pqone[] = {
	0.3511751914303552822533318e+3,
	0.7210391804904475039280863e+3,
	0.4259873011654442389886993e+3,
	0.831898957673850827325226e+2,
	0.45681716295512267064405e+1,
	0.3532840052740123642735e-1
};

static const double   qqone[] = {
	0.74917374171809127714519505e+4,
	0.154141773392650970499848051e+5,
	0.91522317015169922705904727e+4,
	0.18111867005523513506724158e+4,
	0.1038187585462133728776636e+3,
	0.1e+1
};

/* yone for x in [0,8]
 * Index 6444, 18.24 digits precision
 */
static const double   pyone[] = {
	-0.2923821961532962543101048748e+20,
	0.7748520682186839645088094202e+19,
	-0.3441048063084114446185461344e+18,
	0.5915160760490070618496315281e+16,
	-0.4863316942567175074828129117e+14,
	0.2049696673745662182619800495e+12,
	-0.4289471968855248801821819588e+9,
	0.3556924009830526056691325215e+6
};

static const double   qyone[] = {
	0.1491311511302920350174081355e+21,
	0.1818662841706134986885065935e+19,
	0.113163938269888452690508283e+17,
	0.4755173588888137713092774006e+14,
	0.1500221699156708987166369115e+12,
	0.3716660798621930285596927703e+9,
	0.726914730719888456980191315e+6,
	0.10726961437789255233221267e+4,
	0.1e+1
};

void GpEval::f_real(GpArgument * pArg)
{
	t_value a;
	Push(a.SetComplex(PopOrConvertFromString(a).Real(), 0.0));
}

void GpEval::f_imag(GpArgument * pArg)
{
	t_value a;
	Push(a.SetComplex(imag(&PopOrConvertFromString(a)), 0.0));
}

// AngToRad is 1 if we are in radians, or pi/180 if we are in degrees 
void GpEval::f_arg(GpArgument * pArg)
{
	t_value a;
	Push(a.SetComplex(angle(&PopOrConvertFromString(a)) / AngToRad, 0.0));
}

void GpEval::f_conjg(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	Push(a.SetComplex(a.Real(), -imag(&a)));
}

/* AngToRad is 1 if we are in radians, or pi/180 if we are in degrees */

void GpEval::f_sin(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	Push(a.SetComplex(sin(AngToRad * a.Real()) * cosh(AngToRad * imag(&a)), cos(AngToRad * a.Real()) * sinh(AngToRad * imag(&a))));
}

void GpEval::f_cos(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	Push(a.SetComplex(cos(AngToRad * a.Real()) * cosh(AngToRad * imag(&a)), -sin(AngToRad * a.Real()) * sinh(AngToRad * imag(&a))));
}

void GpEval::f_tan(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	if(imag(&a) == 0.0)
		Push(a.SetComplex(tan(AngToRad * a.Real()), 0.0));
	else {
		const double den = cos(2 * AngToRad * a.Real()) + cosh(2 * AngToRad * imag(&a));
		if(den == 0.0) {
			undefined = true;
			Push(&a);
		}
		else
			Push(a.SetComplex(sin(2 * AngToRad * a.Real()) / den, sinh(2 * AngToRad * imag(&a)) / den));
	}
}

void GpEval::f_asin(GpArgument * pArg)
{
	t_value a;
	double alpha, beta;
	int ysign;
	PopOrConvertFromString(a);
	const double x = a.Real();
	const double y = imag(&a);
	if(y == 0.0 && fabs(x) <= 1.0) {
		Push(a.SetComplex(asin(x) / AngToRad, 0.0));
	}
	else if(x == 0.0) {
		Push(a.SetComplex(0.0, -log(-y + sqrt(y * y + 1)) / AngToRad));
	}
	else {
		beta = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		if(beta > 1)
			beta = 1; // Avoid rounding error problems 
		alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		ysign = (y >= 0) ? 1 : -1;
		Push(a.SetComplex(asin(beta) / AngToRad, ysign * log(alpha + sqrt(alpha * alpha - 1)) / AngToRad));
	}
}

void GpEval::f_acos(GpArgument * pArg)
{
	t_value a;
	double ysign;
	PopOrConvertFromString(a);
	const double x = a.Real();
	const double y = imag(&a);
	if(y == 0.0 && fabs(x) <= 1.0) {
		// real result 
		Push(a.SetComplex(acos(x) / AngToRad, 0.0));
	}
	else {
		double alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		double beta  = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		if(beta > 1)
			beta = 1; // Avoid rounding error problems
		else if(beta < -1)
			beta = -1;
		ysign = (y >= 0) ? 1 : -1;
		Push(a.SetComplex(acos(beta) / AngToRad, -ysign * log(alpha + sqrt(alpha * alpha - 1)) / AngToRad));
	}
}

void GpEval::f_atan(GpArgument * pArg)
{
	t_value a;
	double u, v, w, z;
	PopOrConvertFromString(a);
	const double x = a.Real();
	const double y = imag(&a);
	if(y == 0.0)
		Push(a.SetComplex(atan(x) / AngToRad, 0.0));
	else if(x == 0.0 && fabs(y) >= 1.0) {
		undefined = true;
		Push(a.SetComplex(0.0, 0.0));
	}
	else {
		if(x >= 0) {
			u = x;
			v = y;
		}
		else {
			u = -x;
			v = -y;
		}
		z = atan(2 * u / (1 - u * u - v * v));
		w = log((u * u + (v + 1) * (v + 1)) / (u * u + (v - 1) * (v - 1))) / 4;
		if(z < 0)
			z = z + 2 * SMathConst::PiDiv2;
		if(x < 0) {
			z = -z;
			w = -w;
		}
		Push(a.SetComplex(0.5 * z / AngToRad, w));
	}
}
//
// real parts only 
//
void GpEval::f_atan2(GpArgument * pArg)
{
	t_value a;
	const double x = PopOrConvertFromString(a).Real();
	const double y = PopOrConvertFromString(a).Real();
	if(x == 0.0 && y == 0.0) {
		undefined = true;
		Push(a.SetInt(0));
	}
	Push(a.SetComplex(atan2(y, x) / AngToRad, 0.0));
}

void GpEval::f_sinh(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	Push(a.SetComplex(sinh(a.Real()) * cos(imag(&a)), cosh(a.Real()) * sin(imag(&a))));
}

void GpEval::f_cosh(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	Push(a.SetComplex(cosh(a.Real()) * cos(imag(&a)), sinh(a.Real()) * sin(imag(&a))));
}

void GpEval::f_tanh(GpArgument * pArg)
{
	t_value a;
	double den;
	PopOrConvertFromString(a);
	const double real_2arg = 2.0 * a.Real();
	const double imag_2arg = 2.0 * imag(&a);
#ifdef E_MINEXP
	if(-fabs(real_2arg) < E_MINEXP) {
		Push(a.SetComplex(real_2arg < 0 ? -1.0 : 1.0, 0.0));
		return;
	}
#else
	{
		int old_errno = errno;
		if(exp(-fabs(real_2arg)) == 0.0) {
			/* some libm's will raise a silly ERANGE in cosh() and sin() */
			errno = old_errno;
			Push(a.SetComplex(real_2arg < 0 ? -1.0 : 1.0, 0.0));
			return;
		}
	}
#endif
	den = cosh(real_2arg) + cos(imag_2arg);
	Push(a.SetComplex(sinh(real_2arg) / den, sin(imag_2arg) / den));
}

void GpEval::f_asinh(GpArgument * pArg)
{
	t_value a;         /* asinh(z) = -I*asin(I*z) */
	double alpha, beta;
	int ysign;
	PopOrConvertFromString(a);
	const double x = -imag(&a);
	const double y = a.Real();
	if(y == 0.0 && fabs(x) <= 1.0) {
		Push(a.SetComplex(0.0, -asin(x) / AngToRad));
	}
	else if(y == 0.0) {
		Push(a.SetComplex(0.0, 0.0));
		undefined = true;
	}
	else if(x == 0.0) {
		Push(a.SetComplex(log(y + sqrt(y * y + 1)) / AngToRad, 0.0));
	}
	else {
		beta  = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		ysign = (y >= 0) ? 1 : -1;
		Push(a.SetComplex(ysign * log(alpha + sqrt(alpha * alpha - 1)) / AngToRad, -asin(beta) / AngToRad));
	}
}

void GpEval::f_acosh(GpArgument * pArg)
{
	t_value a;
	double alpha, beta; /* acosh(z) = I*acos(z) */
	PopOrConvertFromString(a);
	const double x = a.Real();
	const double y = imag(&a);
	if(y == 0.0 && fabs(x) <= 1.0) {
		Push(a.SetComplex(0.0, acos(x) / AngToRad));
	}
	else if(y == 0) {
		Push(a.SetComplex(log(x + sqrt(x * x - 1)) / AngToRad, 0.0));
	}
	else {
		alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		beta  = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		Push(a.SetComplex(log(alpha + sqrt(alpha * alpha - 1)) / AngToRad, (y<0 ? -1 : 1) * acos(beta) / AngToRad));
	}
}

void GpEval::f_atanh(GpArgument * pArg)
{
	t_value a;
	double u, v, w, z; /* atanh(z) = -I*atan(I*z) */
	PopOrConvertFromString(a);
	const double x = -imag(&a);
	const double y = a.Real();
	if(y == 0.0)
		Push(a.SetComplex(0.0, -atan(x) / AngToRad));
	else if(x == 0.0 && fabs(y) >= 1.0) {
		undefined = true;
		Push(a.SetComplex(0.0, 0.0));
	}
	else {
		if(x >= 0) {
			u = x;
			v = y;
		}
		else {
			u = -x;
			v = -y;
		}
		z = atan(2 * u / (1 - u * u - v * v));
		w = log((u * u + (v + 1) * (v + 1)) / (u * u + (v - 1) * (v - 1))) / 4;
		if(z < 0)
			z = z + 2 * SMathConst::PiDiv2;
		if(x < 0) {
			z = -z;
			w = -w;
		}
		Push(a.SetComplex(w, -0.5 * z / AngToRad));
	}
}

void GpEval::f_ellip_first(GpArgument * pArg)
{
	t_value a;
	double ak, q;
	PopOrConvertFromString(a);
	if(!R_Gg.IsZero(imag(&a)))
		R_Gg.IntErrorNoCaret("can only do elliptic integrals of reals");
	ak = a.Real();
	q = (1.0-ak)*(1.0+ak);
	if(q > 0.0)
		Push(a.SetComplex(carlson_elliptic_rf(0.0, q, 1.0), 0.0));
	else {
		Push(&a);
		undefined = true;
	}
}

void GpEval::f_ellip_second(GpArgument * pArg)
{
	t_value a;
	double ak, q, e;
	PopOrConvertFromString(a);
	if(!R_Gg.IsZero(imag(&a)))
		R_Gg.IntErrorNoCaret("can only do elliptic integrals of reals");
	ak = a.Real();
	q = (1.0-ak)*(1.0+ak);
	if(q > 0.0) {
		e = carlson_elliptic_rf(0.0, q, 1.0)-(ak*ak)*carlson_elliptic_rd(0.0, q, 1.0)/3.0;
		Push(a.SetComplex(e, 0.0));
	}
	else if(q < 0.0) {
		undefined = true;
		Push(&a);
	}
	else {
		e = 1.0;
		Push(a.SetComplex(e, 0.0));
	}
}

void GpEval::f_ellip_third(GpArgument * pArg)
{
	t_value a1, a2;
	double ak, en, q;
	PopOrConvertFromString(a1);
	PopOrConvertFromString(a2);
	if(!R_Gg.IsZero(imag(&a1)) || !R_Gg.IsZero(imag(&a2)))
		R_Gg.IntErrorNoCaret("can only do elliptic integrals of reals");
	ak = a1.Real();
	en = a2.Real();
	q = (1.0-ak)*(1.0+ak);
	if(q > 0.0 && en < 1.0)
		Push(a2.SetComplex(carlson_elliptic_rf(0.0, q, 1.0)+en*carlson_elliptic_rj(0.0, q, 1.0, 1.0-en)/3.0, 0.0));
	else {
		undefined = true;
		Push(&a1);
	}
}

void GpEval::f_int(GpArgument * pArg)
{
	t_value a;
	double foo = PopOrConvertFromString(a).Real();
	if(a.type == NOTDEFINED || fisnan(foo)) {
		Push(a.SetComplex(not_a_number(), 0.0));
		undefined = true;
	}
	else
		Push(a.SetInt((int)foo));
}

#define BAD_DEFAULT default: GpGg.IntErrorNoCaret("internal error : argument neither INT or CMPLX")

void GpEval::f_abs(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    Push(a.SetInt(abs(a.v.int_val)));
		    break;
		case CMPLX:
		    Push(a.SetComplex(magnitude(&a), 0.0));
		    break;
		    BAD_DEFAULT;
	}
}

void GpEval::f_sgn(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    Push(a.SetInt((a.v.int_val > 0) ? 1 : (a.v.int_val < 0) ? -1 : 0));
		    break;
		case CMPLX:
		    Push(a.SetInt((a.v.cmplx_val.real > 0.0) ? 1 : (a.v.cmplx_val.real < 0.0) ? -1 : 0));
		    break;
		    BAD_DEFAULT;
	}
}

void GpEval::f_sqrt(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	const double mag = sqrt(magnitude(&a));
	if(imag(&a) == 0.0) {
		if(a.Real() < 0.0)
			Push(a.SetComplex(0.0, mag));
		else
			Push(a.SetComplex(mag, 0.0));
	}
	else {
		// -pi < ang < pi, so real(sqrt(z)) >= 0 
		double ang = angle(&a) / 2.0;
		Push(a.SetComplex(mag * cos(ang), mag * sin(ang)));
	}
}

void GpEval::f_exp(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	const double mag = gp_exp(a.Real());
	const double ang = imag(&a);
	Push(a.SetComplex(mag * cos(ang), mag * sin(ang)));
}

void GpEval::f_log10(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	if(magnitude(&a) == 0.0) {
		undefined = true;
		Push(&a);
	}
	else
		Push(a.SetComplex(log(magnitude(&a)) / M_LN10, angle(&a) / M_LN10));
}

void GpEval::f_log(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	if(magnitude(&a) == 0.0) {
		undefined = true;
		Push(&a);
	}
	else
		Push(a.SetComplex(log(magnitude(&a)), angle(&a)));
}

void GpEval::f_floor(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    Push(a.SetInt((int)floor((double)a.v.int_val)));
		    break;
		case CMPLX:
		    Push(a.SetInt((int)floor(a.v.cmplx_val.real)));
		    break;
		    BAD_DEFAULT;
	}
}

void GpEval::f_ceil(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
	switch(a.type) {
		case INTGR:
		    Push(a.SetInt((int)ceil((double)a.v.int_val)));
		    break;
		case CMPLX:
		    Push(a.SetInt((int)ceil(a.v.cmplx_val.real)));
		    break;
		    BAD_DEFAULT;
	}
}
//
// EAM - replacement for defined(foo) + f_pushv + f_isvar
//      implements      exists("foo") instead
//
void GpEval::f_exists(GpArgument * pArg)
{
	t_value a;
	Pop(a);
	if(a.type == STRING) {
		UdvtEntry * udv = AddUdvByName(a.v.string_val);
		gpfree_string(&a);
		Push(a.SetInt(udv->udv_value.type == NOTDEFINED ? 0 : 1));
	}
	else {
		Push(a.SetInt(0));
	}
}

/* bessel function approximations */
static double jzero(double x)
{
	const double x2 = x * x;
	double p = pjzero[8];
	double q = qjzero[8];
	for(int n = 7; n >= 0; n--) {
		p = p * x2 + pjzero[n];
		q = q * x2 + qjzero[n];
	}
	return (p / q);
}

static double pzero(double x)
{
	double p, q, z, z2;
	z = 8.0 / x;
	z2 = z * z;
	p = ppzero[5];
	q = qpzero[5];
	for(int n = 4; n >= 0; n--) {
		p = p * z2 + ppzero[n];
		q = q * z2 + qpzero[n];
	}
	return (p / q);
}

static double qzero(double x)
{
	double z = 8.0 / x;
	double z2 = z * z;
	double p = pqzero[5];
	double q = qqzero[5];
	for(int n = 4; n >= 0; n--) {
		p = p * z2 + pqzero[n];
		q = q * z2 + qqzero[n];
	}
	return (p / q);
}

static double yzero(double x)
{
	double x2 = x * x;
	double p = pyzero[8];
	double q = qyzero[8];
	for(int n = 7; n >= 0; n--) {
		p = p * x2 + pyzero[n];
		q = q * x2 + qyzero[n];
	}
	return (p / q);
}

static double rj0(double x)
{
	if(x <= 0.0)
		x = -x;
	if(x < 8.0)
		return (jzero(x));
	else
		return (sqrt(TWO_ON_PI / x) * (pzero(x) * cos(x - SMathConst::PiDiv4) - 8.0 / x * qzero(x) * sin(x - SMathConst::PiDiv4)));
}

static double ry0(double x)
{
	if(x < 0.0)
		return 0.0/*(dzero / dzero)*/; // error
	if(x < 8.0)
		return (yzero(x) + TWO_ON_PI * rj0(x) * log(x));
	else
		return (sqrt(TWO_ON_PI / x) * (pzero(x) * sin(x - SMathConst::PiDiv4) + (8.0 / x) * qzero(x) * cos(x - SMathConst::PiDiv4)));
}

static double jone(double x)
{
	double p, q, x2;
	int n;
	x2 = x * x;
	p = pjone[8];
	q = qjone[8];
	for(n = 7; n >= 0; n--) {
		p = p * x2 + pjone[n];
		q = q * x2 + qjone[n];
	}
	return (p / q);
}

static double pone(double x)
{
	double p, q, z, z2;
	int n;
	z = 8.0 / x;
	z2 = z * z;
	p = ppone[5];
	q = qpone[5];
	for(n = 4; n >= 0; n--) {
		p = p * z2 + ppone[n];
		q = q * z2 + qpone[n];
	}
	return (p / q);
}

static double qone(double x)
{
	const double z = 8.0 / x;
	const double z2 = z * z;
	double p = pqone[5];
	double q = qqone[5];
	for(int n = 4; n >= 0; n--) {
		p = p * z2 + pqone[n];
		q = q * z2 + qqone[n];
	}
	return (p / q);
}

static double yone(double x)
{
	const double x2 = x * x;
	double p = 0.0;
	double q = qyone[8];
	for(int n = 7; n >= 0; n--) {
		p = p * x2 + pyone[n];
		q = q * x2 + qyone[n];
	}
	return (p / q);
}

static double rj1(double x)
{
	double w;
	double v = x;
	if(x < 0.0)
		x = -x;
	if(x < 8.0)
		return (v * jone(x));
	else {
		w = sqrt(TWO_ON_PI / x) * (pone(x) * cos(x - THREE_PI_ON_FOUR) - 8.0 / x * qone(x) * sin(x - THREE_PI_ON_FOUR));
		if(v < 0.0)
			w = -w;
		return (w);
	}
}

static double ry1(double x)
{
	if(x <= 0.0)
		return 0.0/*(dzero / dzero)*/;  /* error */
	if(x < 8.0)
		return (x * yone(x) + TWO_ON_PI * (rj1(x) * log(x) - 1.0 / x));
	else
		return (sqrt(TWO_ON_PI / x) * (pone(x) * sin(x - THREE_PI_ON_FOUR) + (8.0 / x) * qone(x) * cos(x - THREE_PI_ON_FOUR)));
}
//
// FIXME HBB 20010726: should bessel functions really call int_error,
// right in the middle of evaluating some mathematical expression?
// Couldn't they just flag 'undefined', or ignore the real part of the complex number? 
//
void GpEval::f_besj0(GpArgument * pArg)
{
	t_value a;
	Pop(a);
	if(!R_Gg.IsZero(imag(&a)))
		R_Gg.IntErrorNoCaret("can only do bessel functions of reals");
	Push(a.SetComplex(rj0(a.Real()), 0.0));
}

void GpEval::f_besj1(GpArgument * pArg)
{
	t_value a;
	Pop(a);
	if(!R_Gg.IsZero(imag(&a)))
		R_Gg.IntErrorNoCaret("can only do bessel functions of reals");
	Push(a.SetComplex(rj1(a.Real()), 0.0));
}

void GpEval::f_besy0(GpArgument * pArg)
{
	t_value a;
	Pop(a);
	if(!R_Gg.IsZero(imag(&a)))
		R_Gg.IntErrorNoCaret("can only do bessel functions of reals");
	if(a.Real() > 0.0)
		Push(a.SetComplex(ry0(a.Real()), 0.0));
	else {
		Push(a.SetComplex(0.0, 0.0));
		undefined = true;
	}
}

void GpEval::f_besy1(GpArgument * pArg)
{
	t_value a;
	Pop(a);
	if(!R_Gg.IsZero(imag(&a)))
		R_Gg.IntErrorNoCaret("can only do bessel functions of reals");
	if(a.Real() > 0.0)
		Push(a.SetComplex(ry1(a.Real()), 0.0));
	else {
		Push(a.SetComplex(0.0, 0.0));
		undefined = true;
	}
}
//
// functions for accessing fields from tm structure, for time series
// they are all the same, so define a macro
//
#define TIMEFUNC(name, field) void GpEval::name(GpArgument * pArg) \
	{								\
		t_value a;						    \
		struct tm tm;						    \
		Pop(a);						   \
		ggmtime(&tm, a.Real());					    \
		Push(a.SetComplex((double)tm.field, 0.0));		    \
	}

TIMEFUNC(f_tmsec, tm_sec)
TIMEFUNC(f_tmmin, tm_min)
TIMEFUNC(f_tmhour, tm_hour)
TIMEFUNC(f_tmmday, tm_mday)
TIMEFUNC(f_tmmon, tm_mon)
TIMEFUNC(f_tmyear, tm_year)
TIMEFUNC(f_tmwday, tm_wday)
TIMEFUNC(f_tmyday, tm_yday)

/*****************************************************************************/

#define         SQR(a)          ((a)*(a))

#define C1 0.3
#define C2 (1.0/7.0)
#define C3 0.375
#define C4 (9.0/22.0)

static double carlson_elliptic_rc(double x, double y)
{
	double alamb, ave, s, w, xt, yt, ans;
	if(y > 0.0) {
		xt = x;
		yt = y;
		w = 1.0;
	}
	else {
		xt = x-y;
		yt = -y;
		w = sqrt(x)/sqrt(xt);
	}
	do {
		alamb = 2.0*sqrt(xt)*sqrt(yt)+yt;
		xt = 0.25*(xt+alamb);
		yt = 0.25*(yt+alamb);
		ave = (1.0/3.0)*(xt+yt+yt);
		s = (yt-ave)/ave;
	} while(fabs(s) > 0.0012);
	ans = w*(1.0+s*s*(C1+s*(C2+s*(C3+s*C4))))/sqrt(ave);
	return(ans);
}

#undef  C4
#undef  C3
#undef  C2
#undef  C1

static double carlson_elliptic_rf(double x, double y, double z)
{
	double alamb, ave, delx, dely, delz, e2, e3, sqrtx, sqrty, sqrtz;
	double xt = x;
	double yt = y;
	double zt = z;
	do {
		sqrtx = sqrt(xt);
		sqrty = sqrt(yt);
		sqrtz = sqrt(zt);
		alamb = sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
		xt = 0.25*(xt+alamb);
		yt = 0.25*(yt+alamb);
		zt = 0.25*(zt+alamb);
		ave = (1.0/3.0)*(xt+yt+zt);
		delx = (ave-xt)/ave;
		dely = (ave-yt)/ave;
		delz = (ave-zt)/ave;
	} while(fabs(delx) > 0.0025 || fabs(dely) > 0.0025 || fabs(delz) > 0.0025);
	e2 = delx*dely-delz*delz;
	e3 = delx*dely*delz;
	return ((1.0+((1.0/24.0)*e2-(0.1)-(3.0/44.0)*e3)*e2+(1.0/14.0)*e3)/sqrt(ave));
}

#define C1 (3.0/14.0)
#define C2 (1.0/6.0)
#define C3 (9.0/22.0)
#define C4 (3.0/26.0)
#define C5 (0.25*C3)
#define C6 (1.5*C4)

static double carlson_elliptic_rd(double x, double y, double z)
{
	double alamb, ave, delx, dely, delz, ea, eb, ec, ed, ee, sqrtx, sqrty, sqrtz, ans;
	double xt = x;
	double yt = y;
	double zt = z;
	double sum = 0.0;
	double fac = 1.0;
	do {
		sqrtx = sqrt(xt);
		sqrty = sqrt(yt);
		sqrtz = sqrt(zt);
		alamb = sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
		sum += fac/(sqrtz*(zt+alamb));
		fac = 0.25*fac;
		xt = 0.25*(xt+alamb);
		yt = 0.25*(yt+alamb);
		zt = 0.25*(zt+alamb);
		ave = 0.2*(xt+yt+3.0*zt);
		delx = (ave-xt)/ave;
		dely = (ave-yt)/ave;
		delz = (ave-zt)/ave;
	} while(fabs(delx) > 0.0015 || fabs(dely) > 0.0015 || fabs(delz) > 0.0015);
	ea = delx*dely;
	eb = delz*delz;
	ec = ea-eb;
	ed = ea-6.0*eb;
	ee = ed+ec+ec;
	ans = 3.0*sum+fac*(1.0+ed*(-C1+C5*ed-C6*delz*ee)+delz*(C2*ee+delz*(-C3*ec+delz*C4*ea)))/(ave*sqrt(ave));
	return(ans);
}

#undef  C6
#undef  C5
#undef  C4
#undef  C3
#undef  C2
#undef  C1

#define C1 (3.0/14.0)
#define C2 (1.0/3.0)
#define C3 (3.0/22.0)
#define C4 (3.0/26.0)
#define C5 (0.75*C3)
#define C6 (1.5*C4)
#define C7 (0.5*C2)
#define C8 (C3+C3)

static double carlson_elliptic_rj(double x, double y, double z, double p)
{
	double a, alamb, alpha, ans, ave, b, beta, delp, delx, dely, delz, ea, eb, ec,
	    ed, ee, pt, rcx, rho, sqrtx, sqrty, sqrtz, tau, xt, yt, zt;
	double sum = 0.0;
	double fac = 1.0;
	if(p > 0.0) {
		xt = x;
		yt = y;
		zt = z;
		pt = p;
		a = b = rcx = 0.0;
	}
	else {
		xt = MIN(MIN(x, y), z);
		zt = MAX(MAX(x, y), z);
		yt = x+y+z-xt-zt;
		a = 1.0/(yt-p);
		b = a*(zt-yt)*(yt-xt);
		pt = yt+b;
		rho = xt*zt/yt;
		tau = p*pt/yt;
		rcx = carlson_elliptic_rc(rho, tau);
	}
	do {
		sqrtx = sqrt(xt);
		sqrty = sqrt(yt);
		sqrtz = sqrt(zt);
		alamb = sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
		alpha = SQR(pt*(sqrtx+sqrty+sqrtz)+sqrtx*sqrty*sqrtz);
		beta = pt*SQR(pt+alamb);
		sum += fac*carlson_elliptic_rc(alpha, beta);
		fac = 0.25*fac;
		xt = 0.25*(xt+alamb);
		yt = 0.25*(yt+alamb);
		zt = 0.25*(zt+alamb);
		pt = 0.25*(pt+alamb);
		ave = 0.2*(xt+yt+zt+pt+pt);
		delx = (ave-xt)/ave;
		dely = (ave-yt)/ave;
		delz = (ave-zt)/ave;
		delp = (ave-pt)/ave;
	} while(fabs(delx)>0.0015 || fabs(dely)>0.0015 || fabs(delz)>0.0015 || fabs(delp)>0.0015);
	ea = delx*(dely+delz)+dely*delz;
	eb = delx*dely*delz;
	ec = delp*delp;
	ed = ea-3.0*ec;
	ee = eb+2.0*delp*(ea-ec);
	ans = 3.0*sum+fac*(1.0+ed*(-C1+C5*ed-C6*ee)+eb*(C7+delp*(-C8+delp*C4))+delp*ea*(C2-delp*C3)-C2*delp*ec)/(ave*sqrt(ave));
	if(p <= 0.0)
		ans = a*(b*ans+3.0*(rcx-carlson_elliptic_rf(xt, yt, zt)));
	return(ans);
}

#undef  C6
#undef  C5
#undef  C4
#undef  C3
#undef  C2
#undef  C1
#undef  C8
#undef  C7

#undef                  SQR
//
// SPEC FUNC
//
#define ITMAX   200
#ifdef FLT_EPSILON
	#define MACHEPS FLT_EPSILON // 1.0E-08
#else
	#define MACHEPS 1.0E-08
#endif
#ifndef E_MINEXP
	#define E_MINEXP  (-88.0) // AS239 value, e^-88 = 2^-127 
#endif
#ifndef E_MAXEXP
	#define E_MAXEXP (-E_MINEXP)
#endif
#ifdef FLT_MAX
	#define OFLOW   FLT_MAX // 1.0E+37
#else
	#define OFLOW   1.0E+37
#endif
// AS239 value for igamma(a,x>=XBIG) = 1.0 
#define XBIG    1.0E+08
//
// Mathematical constants
//
//#define LNPI      1.14472988584940016
#define LNSQRT2PI 0.9189385332046727
#define PNT68     0.6796875
//
// Prefer lgamma 
//
#ifndef GAMMA
	#ifdef HAVE_LGAMMA
		#define GAMMA(x) lgamma(x)
	#elif defined(HAVE_GAMMA)
		#define GAMMA(x) gamma(x)
	#else
		#undef GAMMA
	#endif
#endif
#if defined(GAMMA) && !HAVE_DECL_SIGNGAM
	extern int signgam; // this is not always declared in math.h 
#endif
//
// Local function declarations, not visible outside this file 
//
static int mtherr(char *, int);
static double polevl(double x, const double coef[], int N);
static double p1evl(double x, const double coef[], int N);
static double confrac(double a, double b, double x);
static double ibeta(double a, double b, double x);
static double igamma(double a, double x);
static double ranf(t_value & rInit);
static double inverse_error_func(double p);
static double inverse_normal_func(double p);
static double lambertw(double x);
#if(0) // Only used by low-precision Airy version 
	static double airy_neg(double x);
	static double airy_pos(double x);
#endif
#ifndef HAVE_LIBCERF
	static double humlik(double x, double y);
#endif
static double expint(double n, double x);
#ifndef GAMMA
	static int ISNAN(double x);
	static int ISFINITE(double x);
	static double lngamma(double z);
#endif
#ifndef HAVE_ERF
	static double erf(double a);
#endif
#ifndef HAVE_ERFC
	static double erfc(double a);
#endif

/* Macros to configure routines taken from CEPHES: */

/* Unknown arithmetic, invokes coefficients given in
 * normal decimal format.  Beware of range boundary
 * problems (MACHEP, MAXLOG, etc. in const.c) and
 * roundoff problems in pow.c:
 * (Sun SPARCstation)
 */
#define UNK 1
#define MACHEP SMathConst::Epsilon
#define MAXNUM DBL_MAX
#define DENORMAL   1 // Define to support tiny denormal numbers, else undefine
#define INFINITIES 1 // Define to ask for infinity support, else undefine
// Define to ask for support of numbers that are Not-a-Number,
// else undefine.  This may automatically define INFINITIES in some files
#define NANS       1
#define MINUSZERO  1 // Define to distinguish between -0.0 and +0.0
//
// Cephes Math Library Release 2.0:  April, 1987
// Copyright 1984, 1987 by Stephen L. Moshier
// Direct inquiries to 30 Frost Street, Cambridge, MA 02140
//
static int merror = 0;
//
// Notice: the order of appearance of the following messages cannot be bound
// to error codes defined in mconf.h or math.h or similar, as these files are
// not available on every platform. Thus, enumerate them explicitly.
//
#define MTHERR_DOMAIN    1
#define MTHERR_SING      2
#define MTHERR_OVERFLOW  3
#define MTHERR_UNDERFLOW 4
#define MTHERR_TLPREC    5
#define MTHERR_PLPREC    6

static int mtherr(char * name, int code)
{
	static const char * ermsg[7] = {
		"unknown",          /* error code 0 */
		"domain",           /* error code 1 */
		"singularity",      /* et seq.      */
		"overflow",
		"underflow",
		"total loss of precision",
		"partial loss of precision"
	};
	/* Display string passed by calling program,
	 * which is supposed to be the name of the
	 * function in which the error occurred:
	 */
	printf("\n%s ", name);
	/* Set global error message word */
	merror = code;
	/* Display error message defined by the code argument.  */
	if((code <= 0) || (code >= 7))
		code = 0;
	printf("%s error\n", ermsg[code]);
	/* Return to calling program */
	return (0);
}

/*                                                      polevl.c
 *                                                      p1evl.c
 *
 *      Evaluate polynomial
 *
 *
 *
 * SYNOPSIS:
 *
 * int N;
 * double x, y, coef[N+1], polevl[];
 *
 * y = polevl( x, coef, N );
 *
 *
 *
 * DESCRIPTION:
 *
 * Evaluates polynomial of degree N:
 *
 *                     2          N
 * y  =  C  + C x + C x  +...+ C x
 *        0    1     2          N
 *
 * Coefficients are stored in reverse order:
 *
 * coef[0] = C  , ..., coef[N] = C  .
 *            N                   0
 *
 *  The function p1evl() assumes that coef[N] = 1.0 and is
 * omitted from the array.  Its calling arguments are
 * otherwise the same as polevl().
 *
 *
 * SPEED:
 *
 * In the interest of speed, there are no checks for out
 * of bounds arithmetic.  This routine is used by most of
 * the functions in the library.  Depending on available
 * equipment features, the user may wish to rewrite the
 * program in microcode or assembly language.
 *
 */

/*
   Cephes Math Library Release 2.1:  December, 1988
   Copyright 1984, 1987, 1988 by Stephen L. Moshier
   Direct inquiries to 30 Frost Street, Cambridge, MA 02140
 */
static double polevl(double x, const double coef[], int N)
{
	const double * p = coef;
	double ans = *p++;
	int    i = N;
	do {
		ans = ans * x + *p++;
	} while(--i);
	return (ans);
}

/*                                          N
 * Evaluate polynomial when coefficient of x  is 1.0.
 * Otherwise same as polevl.
 */
static double p1evl(double x, const double coef[], int N)
{
	const  double * p = coef;
	double ans = x + *p++;
	int    i = N - 1;
	do
		ans = ans * x + *p++;
	while(--i);
	return (ans);
}

#ifndef GAMMA
//
// Provide GAMMA function for those who do not already have one 
//
int sgngam;

static int ISNAN(double x)
{
	volatile double a = x;
	return (a != a) ? 1 : 0;
}

static int ISFINITE(double x)
{
	volatile double a = x;
	return (a < DBL_MAX) ? 1 : 0;
}

double lngamma(double x)
{
	/* A[]: Stirling's formula expansion of log gamma
	 * B[], C[]: log gamma function between 2 and 3
	 */
#ifdef UNK
	static const double A[] = {
		8.11614167470508450300E-4,
		-5.95061904284301438324E-4,
		7.93650340457716943945E-4,
		-2.77777777730099687205E-3,
		8.33333333333331927722E-2
	};
	static const double B[] = {
		-1.37825152569120859100E3,
		-3.88016315134637840924E4,
		-3.31612992738871184744E5,
		-1.16237097492762307383E6,
		-1.72173700820839662146E6,
		-8.53555664245765465627E5
	};
	static const double C[] = {
		/* 1.00000000000000000000E0, */
		-3.51815701436523470549E2,
		-1.70642106651881159223E4,
		-2.20528590553854454839E5,
		-1.13933444367982507207E6,
		-2.53252307177582951285E6,
		-2.01889141433532773231E6
	};
	/* log( sqrt( 2*pi ) ) */
	static const double LS2PI = 0.91893853320467274178;
	#define MAXLGM 2.556348e305
#endif /* UNK */
#ifdef DEC
	static const ushort A[] = {
		0035524, 0141201, 0034633, 0031405,
		0135433, 0176755, 0126007, 0045030,
		0035520, 0006371, 0003342, 0172730,
		0136066, 0005540, 0132605, 0026407,
		0037252, 0125252, 0125252, 0125132
	};
	static const ushort B[] = {
		0142654, 0044014, 0077633, 0035410,
		0144027, 0110641, 0125335, 0144760,
		0144641, 0165637, 0142204, 0047447,
		0145215, 0162027, 0146246, 0155211,
		0145322, 0026110, 0010317, 0110130,
		0145120, 0061472, 0120300, 0025363
	};
	static const ushort C[] = {
		/*0040200,0000000,0000000,0000000*/
		0142257, 0164150, 0163630, 0112622,
		0143605, 0050153, 0156116, 0135272,
		0144527, 0056045, 0145642, 0062332,
		0145213, 0012063, 0106250, 0001025,
		0145432, 0111254, 0044577, 0115142,
		0145366, 0071133, 0050217, 0005122
	};
	// log( sqrt( 2*pi ) ) 
	static const ushort LS2P[] = {040153, 037616, 041445, 0172645, };
	#define LS2PI *(double*)LS2P
	#define MAXLGM 2.035093e36
#endif // DEC 
#ifdef IBMPC
	static const ushort A[] = {
		0x6661, 0x2733, 0x9850, 0x3f4a,
		0xe943, 0xb580, 0x7fbd, 0xbf43,
		0x5ebb, 0x20dc, 0x019f, 0x3f4a,
		0xa5a1, 0x16b0, 0xc16c, 0xbf66,
		0x554b, 0x5555, 0x5555, 0x3fb5
	};
	static const ushort B[] = {
		0x6761, 0x8ff3, 0x8901, 0xc095,
		0xb93e, 0x355b, 0xf234, 0xc0e2,
		0x89e5, 0xf890, 0x3d73, 0xc114,
		0xdb51, 0xf994, 0xbc82, 0xc131,
		0xf20b, 0x0219, 0x4589, 0xc13a,
		0x055e, 0x5418, 0x0c67, 0xc12a
	};
	static const ushort C[] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0x12b2, 0x1cf3, 0xfd0d, 0xc075,
		0xd757, 0x7b89, 0xaa0d, 0xc0d0,
		0x4c9b, 0xb974, 0xeb84, 0xc10a,
		0x0043, 0x7195, 0x6286, 0xc131,
		0xf34c, 0x892f, 0x5255, 0xc143,
		0xe14a, 0x6a11, 0xce4b, 0xc13e
	};
	// log( sqrt( 2*pi ) ) 
	static const ushort LS2P[] = {
		0xbeb5, 0xc864, 0x67f1, 0x3fed
	};
	#define LS2PI *(double*)LS2P
	#define MAXLGM 2.556348e305
#endif // IBMPC 
#ifdef MIEEE
	static const ushort A[] = {
		0x3f4a, 0x9850, 0x2733, 0x6661,
		0xbf43, 0x7fbd, 0xb580, 0xe943,
		0x3f4a, 0x019f, 0x20dc, 0x5ebb,
		0xbf66, 0xc16c, 0x16b0, 0xa5a1,
		0x3fb5, 0x5555, 0x5555, 0x554b
	};
	static const ushort B[] = {
		0xc095, 0x8901, 0x8ff3, 0x6761,
		0xc0e2, 0xf234, 0x355b, 0xb93e,
		0xc114, 0x3d73, 0xf890, 0x89e5,
		0xc131, 0xbc82, 0xf994, 0xdb51,
		0xc13a, 0x4589, 0x0219, 0xf20b,
		0xc12a, 0x0c67, 0x5418, 0x055e
	};
	static const ushort C[] = {
		0xc075, 0xfd0d, 0x1cf3, 0x12b2,
		0xc0d0, 0xaa0d, 0x7b89, 0xd757,
		0xc10a, 0xeb84, 0xb974, 0x4c9b,
		0xc131, 0x6286, 0x7195, 0x0043,
		0xc143, 0x5255, 0x892f, 0xf34c,
		0xc13e, 0xce4b, 0x6a11, 0xe14a
	};
	/* log( sqrt( 2*pi ) ) */
	static const ushort LS2P[] = {
		0x3fed, 0x67f1, 0xc864, 0xbeb5
	};
#define LS2PI *(double*)LS2P
#define MAXLGM 2.556348e305
#endif /* MIEEE */

	static const double LOGPI = 1.1447298858494001741434273513530587116472948129153;
	double p, q, u, w, z;
	int i;
	sgngam = 1;
#ifdef NANS
	if(ISNAN(x))
		return (x);
#endif
#ifdef INFINITIES
	if(!ISFINITE((x)))
		return (DBL_MAX * DBL_MAX);
#endif
	if(x < -34.0) {
		q = -x;
		w = lngamma(q);    /* note this modifies sgngam! */
		p = floor(q);
		if(p == q) {
lgsing:
#ifdef INFINITIES
			mtherr("lngamma", MTHERR_SING);
			return (DBL_MAX * DBL_MAX);
#else
			goto loverf;
#endif
		}
		i = (int)p;
		if((i & 1) == 0)
			sgngam = -1;
		else
			sgngam = 1;
		z = q - p;
		if(z > 0.5) {
			p += 1.0;
			z = p - q;
		}
		z = q * sin(SMathConst::Pi * z);
		if(z == 0.0)
			goto lgsing;
		//      z = log(SMathConst::Pi) - log(z) - w;
		z = LOGPI - log(z) - w;
		return (z);
	}
	if(x < 13.0) {
		z = 1.0;
		p = 0.0;
		u = x;
		while(u >= 3.0) {
			p -= 1.0;
			u = x + p;
			z *= u;
		}
		while(u < 2.0) {
			if(u == 0.0)
				goto lgsing;
			z /= u;
			p += 1.0;
			u = x + p;
		}
		if(z < 0.0) {
			sgngam = -1;
			z = -z;
		}
		else
			sgngam = 1;
		if(u == 2.0)
			return (log(z));
		p -= 2.0;
		x = x + p;
		p = x * polevl(x, B, 5) / p1evl(x, C, 6);
		return (log(z) + p);
	}
	if(x > MAXLGM) {
#ifdef INFINITIES
		return (sgngam * (DBL_MAX * DBL_MAX));
#else
loverf:
		mtherr("lngamma", MTHERR_OVERFLOW);
		return (sgngam * MAXNUM);
#endif
	}
	q = (x - 0.5) * log(x) - x + LS2PI;
	if(x > 1.0e8)
		return (q);

	p = 1.0 / (x * x);
	if(x >= 1000.0)
		q += ((7.9365079365079365079365e-4 * p
			    - 2.7777777777777777777778e-3) * p
		    + 0.0833333333333333333333) / x;
	else
		q += polevl(p, A, 4) / x;
	return (q);
}

#define GAMMA(x) lngamma((x))
/* HBB 20030816: must override name of sgngam so f_gamma() uses it */
#define signgam sgngam

#endif /* !GAMMA */
//
// Make all the following internal routines f_whatever() perform
// autoconversion from string to numeric value.
//
//#define pop__(x) pop_or_convert_from_string(x)

void GpEval::f_erf(GpArgument * pArg)
{
	t_value a;
	double x;
	x = PopOrConvertFromString(a).Real();
	x = erf(x);
	Push(a.SetComplex(x, 0.0));
}

void GpEval::f_erfc(GpArgument * pArg)
{
	t_value a;
	double x = PopOrConvertFromString(a).Real();
	x = erfc(x);
	Push(a.SetComplex(x, 0.0));
}

void GpEval::f_ibeta(GpArgument * pArg)
{
	t_value a;
	double x;
	double arg1;
	double arg2;
	x = PopOrConvertFromString(a).Real();
	arg2 = PopOrConvertFromString(a).Real();
	arg1 = PopOrConvertFromString(a).Real();
	x = ibeta(arg1, arg2, x);
	if(x == -1.0) {
		undefined = true;
		Push(a.SetInt(0));
	}
	else
		Push(a.SetComplex(x, 0.0));
}

void GpEval::f_igamma(GpArgument * pArg)
{
	t_value a;
	double x;
	double arg1;
	x = PopOrConvertFromString(a).Real();
	arg1 = PopOrConvertFromString(a).Real();
	x = igamma(arg1, x);
	if(x == -1.0) {
		undefined = true;
		Push(a.SetInt(0));
	}
	else
		Push(a.SetComplex(x, 0.0));
}

void GpEval::f_gamma(GpArgument * pArg)
{
	double y;
	t_value a;
	y = GAMMA(PopOrConvertFromString(a).Real());
	if(y > E_MAXEXP) {
		undefined = true;
		Push(a.SetInt(0));
	}
	else
		Push(a.SetComplex(signgam * gp_exp(y), 0.0));
}

void GpEval::f_lgamma(GpArgument * pArg)
{
	t_value a;
	Push(a.SetComplex(GAMMA(PopOrConvertFromString(a).Real()), 0.0));
}

#ifndef BADRAND

void GpEval::f_rand(GpArgument * pArg)
{
	t_value a;
	Push(a.SetComplex(ranf(PopOrConvertFromString(a)), 0.0));
}

#else /* BADRAND */

/* Use only to observe the effect of a "bad" random number generator. */
void GpEval::f_rand(GpArgument * pArg)
{
	t_value a;
	(void)arg;                      
	static uint y = 0;
	uint maxran = 1000;
	(void)pop__(&a)->Real();
	y = (781 * y + 387) % maxran;
	Push(a.SetComplex((double)y / maxran, 0.0));
}

#endif /* BADRAND */

/*
 * Fallback implementation of the Faddeeva/Voigt function
 *	w(z) = exp(*-z^2) * erfc(-i*z)
 * if not available from libcerf or some other library
 */
#ifndef HAVE_LIBCERF
void GpEval::f_voigt(GpArgument * pArg)
{
	t_value a;
	double x, y;
	y = PopOrConvertFromString(a).Real();
	x = PopOrConvertFromString(a).Real();
	Push(a.SetComplex(humlik(x, y), 0.0));
}

/*
 * Calculate the Voigt/Faddeeva function with relative error less than 10^(-4).
 *     (see http://www.atm.ox.ac.uk/user/wells/voigt.html)
 *
 * K(x,y) = \frac{y}{\pi} \int{\frac{e^{-t^2}}{(x-t)^2+y^2}}dt
 *
 *  arguments:
 *	x, y - real and imaginary components of complex argument
 *  return value
 *	real value K(x,y)
 *
 * Algorithm: Josef Humlicek JQSRT 27 (1982) pp 437
 * Fortran program by J.R. Wells  JQSRT 62 (1999) pp 29-48.
 * Translated to C++ with f2c program and modified by Marcin Wojdyr
 * Minor adaptations from C++ to C by E. Stambulchik
 * Adapted for gnuplot by Tommaso Vinci
 */
static double humlik(double x, double y)
{
	const double c[6] = { 1.0117281,     -0.75197147,      0.012557727, 0.010022008,   -2.4206814e-4,    5.0084806e-7 };
	const double s[6] = { 1.393237,       0.23115241,     -0.15535147, 0.0062183662,   9.1908299e-5,   -6.2752596e-7 };
	const double t[6] = { 0.31424038,     0.94778839,      1.5976826, 2.2795071,      3.020637,        3.8897249 };
	const double rrtpi = 0.56418958; /* 1/SQRT(pi) */
	double a0, d0, d2, e0, e2, e4, h0, h2, h4, h6, p0, p2, p4, p6, p8, z0, z2, z4, z6, z8;
	double mf[6], pf[6], mq[6], pq[6], xm[6], ym[6], xp[6], yp[6];
	bool rg1, rg2, rg3;
	double xlim0, xlim1, xlim2, xlim3, xlim4;
	double yq, yrrtpi;
	double abx, xq;
	double k;
	yq = y * y;
	yrrtpi = y * rrtpi;
	rg1 = true, rg2 = true, rg3 = true;
	abx = fabs(x);
	xq = abx * abx;
	if(y >= 70.55)
		return yrrtpi / (xq + yq);
	xlim0 = sqrt(y * (40. - y * 3.6) + 15100.);
	xlim1 = (y >= 8.425 ?  0. : sqrt(164. - y * (y * 1.8 + 4.3)));
	xlim2 = 6.8 - y;
	xlim3 = y * 2.4;
	xlim4 = y * 18.1 + 1.65;
	if(y <= 1e-6)
		xlim2 = xlim1 = xlim0;
	if(abx >= xlim0)                /* Region 0 algorithm */
		return yrrtpi / (xq + yq);
	else if(abx >= xlim1) {         /* Humlicek W4 Region 1 */
		if(rg1) {               /* First point in Region 1 */
			rg1 = false;
			a0 = yq + 0.5;  /* Region 1 y-dependents */
			d0 = a0 * a0;
			d2 = yq + yq - 1.;
		}
		return rrtpi / (d0 + xq * (d2 + xq)) * y * (a0 + xq);
	}
	else if(abx > xlim2) {          /* Humlicek W4 Region 2 */
		if(rg2) {               /* First point in Region 2 */
			rg2 = false;
			/* Region 2 y-dependents */
			h0 = yq * (yq * (yq * (yq + 6.) + 10.5) + 4.5) + 0.5625;
			h2 = yq * (yq * (yq * 4. + 6.) + 9.) - 4.5;
			h4 = 10.5 - yq * (6. - yq * 6.);
			h6 = yq * 4. - 6.;
			e0 = yq * (yq * (yq + 5.5) + 8.25) + 1.875;
			e2 = yq * (yq * 3. + 1.) + 5.25;
			e4 = h6 * 0.75;
		}
		return rrtpi / (h0 + xq * (h2 + xq * (h4 + xq * (h6 + xq))))
		       * y * (e0 + xq * (e2 + xq * (e4 + xq)));
	}

	else if(abx < xlim3) {          /* Humlicek W4 Region 3 */
		if(rg3) {               /* First point in Region 3 */
			rg3 = false;
			/* Region 3 y-dependents */
			z0 = y * (y * (y * (y * (y * (y * (y * (y * (y * (y + 13.3988) + 88.26741) + 369.1989) + 1074.409)
				+ 2256.981) + 3447.629) + 3764.966) + 2802.87) + 1280.829) + 272.1014;
			z2 = y * (y * (y * (y * (y * (y * (y * (y * 5.  + 53.59518) + 266.2987) + 793.4273) + 1549.675) + 2037.31)
				+ 1758.336) + 902.3066) + 211.678;
			z4 = y * (y * (y * (y * (y * (y * 10. + 80.39278) + 269.2916) + 479.2576) + 497.3014) + 308.1852) + 78.86585;
			z6 = y * (y * (y * (y * 10. + 53.59518) + 92.75679) + 55.02933) + 22.03523;
			z8 = y * (y * 5. + 13.3988) + 1.49646;
			p0 = y * (y * (y * (y * (y * (y * (y * (y * (y * 0.3183291
				+ 4.264678) + 27.93941) + 115.3772) + 328.2151) + 662.8097) + 946.897) + 919.4955) + 549.3954) + 153.5168;
			p2 = y * (y * (y * (y * (y * (y * (y * 1.2733163 + 12.79458)
							    + 56.81652) + 139.4665) + 189.773) + 124.5975)
			    - 1.322256) - 34.16955;
			p4 = y * (y * (y * (y * (y * 1.9099744 + 12.79568)
					    + 29.81482) + 24.01655) + 10.46332) + 2.584042;
			p6 = y * (y * (y * 1.273316 + 4.266322) + 0.9377051)
			    - 0.07272979;
			p8 = y * .3183291 + 5.480304e-4;
		}
		return 1.7724538 / (z0 + xq * (z2 + xq * (z4 + xq * (z6 +
					    xq * (z8 + xq)))))
		       * (p0 + xq * (p2 + xq * (p4 + xq * (p6 + xq * p8))));
	}

	else {                          /* Humlicek CPF12 algorithm */
		double ypy0 = y + 1.5;
		double ypy0q = ypy0 * ypy0;
		int j;
		for(j = 0; j <= 5; ++j) {
			double d = x - t[j];
			mq[j] = d * d;
			mf[j] = 1. / (mq[j] + ypy0q);
			xm[j] = mf[j] * d;
			ym[j] = mf[j] * ypy0;
			d = x + t[j];
			pq[j] = d * d;
			pf[j] = 1. / (pq[j] + ypy0q);
			xp[j] = pf[j] * d;
			yp[j] = pf[j] * ypy0;
		}
		k = 0.;
		if(abx <= xlim4)        /* Humlicek CPF12 Region I */
			for(j = 0; j <= 5; ++j)
				k += c[j] * (ym[j]+yp[j]) - s[j] * (xm[j]-xp[j]);
		else {                  /* Humlicek CPF12 Region II */
			double yf = y + 3.;
			for(j = 0; j <= 5; ++j)
				k += (c[j] * (mq[j] * mf[j] - ym[j] * 1.5)
				    + s[j] * yf * xm[j]) / (mq[j] + 2.25)
				    + (c[j] * (pq[j] * pf[j] - yp[j] * 1.5)
				    - s[j] * yf * xp[j]) / (pq[j] + 2.25);
			k = y * k + exp(-xq);
		}
		return k;
	}
}

#endif /* libcerf not available */

/* ** ibeta.c
 *
 *   DESCRIBE  Approximate the incomplete beta function Ix(a, b).
 *
 *                           _
 *                          |(a + b)     /x  (a-1)         (b-1)
 *             Ix(a, b) = -_-------_--- * |  t     * (1 - t)     dt (a,b > 0)
 *                        |(a) * |(b)   /0
 *
 *
 *
 *   CALL      p = ibeta(a, b, x)
 *
 *             double    a    > 0
 *             double    b    > 0
 *             double    x    [0, 1]
 *
 *   WARNING   none
 *
 *   RETURN    double    p    [0, 1]
 *                            -1.0 on error condition
 *
 *   XREF      lngamma()
 *
 *   BUGS      This approximation is only accurate on the domain
 *             x < (a-1)/(a+b-2)
 *
 *   REFERENCE The continued fraction expansion as given by
 *             Abramowitz and Stegun (1964) is used.
 *
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 *
 * Note: this function was translated from the Public Domain Fortran
 *       version available from http://lib.stat.cmu.edu/apstat/xxx
 *
 */

static double ibeta(double a, double b, double x)
{
	/* Test for admissibility of arguments */
	if(a <= 0.0 || b <= 0.0)
		return -1.0;
	if(x < 0.0 || x > 1.0)
		return -1.0; ;

	/* If x GpGg.Gp__C.Eq 0 or 1, return x as prob */
	if(x == 0.0 || x == 1.0)
		return x;

	/* Swap a, b if necessary for more efficient evaluation */
	if(a < x * (a + b)) {
		double temp = confrac(b, a, 1.0 - x);
		return (temp < 0.0) ? temp : 1.0 - temp;
	}
	else {
		return confrac(a, b, x);
	}
}

static double confrac(double a, double b, double x)
{
	double Alo = 0.0;
	double Ahi;
	double Aev;
	double Aod;
	double Blo = 1.0;
	double Bhi = 1.0;
	double Bod = 1.0;
	double Bev = 1.0;
	double f;
	double fold;
	double Apb = a + b;
	double d;
	int i;
	int j;

	/* Set up continued fraction expansion evaluation. */
	Ahi = gp_exp(GAMMA(Apb) + a * log(x) + b * log(1.0 - x) -
	    GAMMA(a + 1.0) - GAMMA(b));

	/*
	 * Continued fraction loop begins here. Evaluation continues until
	 * maximum iterations are exceeded, or convergence achieved.
	 */
	for(i = 0, j = 1, f = Ahi; i <= ITMAX; i++, j++) {
		d = a + j + i;
		Aev = -(a + i) * (Apb + i) * x / d / (d - 1.0);
		Aod = j * (b - j) * x / d / (d + 1.0);
		Alo = Bev * Ahi + Aev * Alo;
		Blo = Bev * Bhi + Aev * Blo;
		Ahi = Bod * Alo + Aod * Ahi;
		Bhi = Bod * Blo + Aod * Bhi;

		if(fabs(Bhi) < MACHEPS)
			Bhi = 0.0;

		if(Bhi != 0.0) {
			fold = f;
			f = Ahi / Bhi;
			if(fabs(f - fold) < fabs(f) * MACHEPS)
				return f;
		}
	}

	return -1.0;
}

/* ** igamma.c
 *
 *   DESCRIBE  Approximate the incomplete gamma function P(a, x).
 *
 *                         1     /x  -t   (a-1)
 *             P(a, x) = -_--- * |  e  * t     dt      (a > 0)
 *                       |(a)   /0
 *
 *   CALL      p = igamma(a, x)
 *
 *             double    a    >  0
 *             double    x    >= 0
 *
 *   WARNING   none
 *
 *   RETURN    double    p    [0, 1]
 *                            -1.0 on error condition
 *
 *   XREF      lngamma()
 *
 *   BUGS      Values 0 <= x <= 1 may lead to inaccurate results.
 *
 *   REFERENCE ALGORITHM AS239  APPL. STATIST. (1988) VOL. 37, NO. 3
 *
 * Copyright (c) 1992 Jos van der Woude, jvdwoude@hut.nl
 *
 * Note: this function was translated from the Public Domain Fortran
 *       version available from http://lib.stat.cmu.edu/apstat/239
 *
 */

double igamma(double a, double x)
{
	double arg;
	double aa;
	double an;
	double b;
	int i;

	/* Check that we have valid values for a and x */
	if(x < 0.0 || a <= 0.0)
		return -1.0;

	/* Deal with special cases */
	if(x == 0.0)
		return 0.0;
	if(x > XBIG)
		return 1.0;

	/* Check value of factor arg */
	arg = a * log(x) - x - GAMMA(a + 1.0);
	/* HBB 20031006: removed a spurious check here */
	arg = gp_exp(arg);

	/* Choose infinite series or continued fraction. */

	if((x > 1.0) && (x >= a + 2.0)) {
		/* Use a continued fraction expansion */
		double pn1, pn2, pn3, pn4, pn5, pn6;
		double rn;
		double rnold;

		aa = 1.0 - a;
		b = aa + x + 1.0;
		pn1 = 1.0;
		pn2 = x;
		pn3 = x + 1.0;
		pn4 = x * b;
		rnold = pn3 / pn4;

		for(i = 1; i <= ITMAX; i++) {
			aa++;
			b += 2.0;
			an = aa * (double)i;

			pn5 = b * pn3 - an * pn1;
			pn6 = b * pn4 - an * pn2;

			if(pn6 != 0.0) {
				rn = pn5 / pn6;
				if(fabs(rnold - rn) <= MIN(MACHEPS, MACHEPS * rn))
					return 1.0 - arg * rn * a;

				rnold = rn;
			}
			pn1 = pn3;
			pn2 = pn4;
			pn3 = pn5;
			pn4 = pn6;

			/* Re-scale terms in continued fraction if terms are large */
			if(fabs(pn5) >= OFLOW) {
				pn1 /= OFLOW;
				pn2 /= OFLOW;
				pn3 /= OFLOW;
				pn4 /= OFLOW;
			}
		}
	}
	else {
		/* Use Pearson's series expansion. */

		for(i = 0, aa = a, an = b = 1.0; i <= ITMAX; i++) {
			aa++;
			an *= x / aa;
			b += an;
			if(an < b * MACHEPS)
				return arg * b;
		}
	}
	return -1.0;
}

/* ----------------------------------------------------------------
    Cummulative distribution function of the ChiSquare distribution
   ---------------------------------------------------------------- */
double chisq_cdf(int dof, double chisqr)
{
	if(dof <= 0)
		return not_a_number();
	if(chisqr <= 0.)
		return 0;
	return igamma(0.5 * dof, 0.5 * chisqr);
}

/***********************************************************************
     double ranf(double init)
                Random number generator as a Function
     Returns a random floating point number from a uniform distribution
     over 0 - 1 (endpoints of this interval are not returned) using a
     large integer generator.
     This is a transcription from Pascal to Fortran of routine
     Uniform_01 from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)

               Generate Large Integer
     Returns a random integer following a uniform distribution over
     (1, 2147483562) using the generator.
     This is a transcription from Pascal to Fortran of routine
     Random from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
***********************************************************************/
static double ranf(t_value & rInit)
{
	long k, z;
	static int firsttime = 1;
	static long seed1, seed2;
	static const long Xm1 = 2147483563L;
	static const long Xm2 = 2147483399L;
	static const long Xa1 = 40014L;
	static const long Xa2 = 40692L;
	// Seed values must be integer, but check for both values equal zero
	// before casting for speed 
	if(rInit.Real() != 0.0 || imag(&rInit) != 0.0) {
		/* Construct new seed values from input parameter */
		long seed1cvrt = (long)rInit.Real();
		long seed2cvrt = (long)imag(&rInit);
		if(rInit.Real() != (double)seed1cvrt || imag(&rInit) != (double)seed2cvrt || seed1cvrt > 017777777777L ||
		    seed2cvrt > 017777777777L || (seed1cvrt <= 0 && seed2cvrt != 0) || seed2cvrt < 0)
			GpGg.IntErrorNoCaret("Illegal seed value");
		else if(seed1cvrt < 0)
			firsttime = 1;
		else {
			seed1 = seed1cvrt;
			seed2 = (seed2cvrt) ? seed2cvrt : seed1cvrt;
			firsttime = 0;
		}
	}
	// (Re)-Initialize seeds if necessary 
	if(firsttime) {
		firsttime = 0;
		seed1 = 1234567890L;
		seed2 = 1234567890L;
	}
	FPRINTF((stderr, "ranf: seed = %lo %lo        %ld %ld\n", seed1, seed2));
	// Generate pseudo random integers, which always end up positive 
	k = seed1 / 53668L;
	seed1 = Xa1 * (seed1 - k * 53668L) - k * 12211;
	if(seed1 < 0)
		seed1 += Xm1;
	k = seed2 / 52774L;
	seed2 = Xa2 * (seed2 - k * 52774L) - k * 3791;
	if(seed2 < 0)
		seed2 += Xm2;
	z = seed1 - seed2;
	if(z < 1)
		z += (Xm1 - 1);
	/*
	 * 4.656613057E-10 is 1/Xm1.  Xm1 is set at the top of this file and is
	 * currently 2147483563. If Xm1 changes, change this also.
	 */
	return (double)4.656613057E-10 *z;
}

/* ----------------------------------------------------------------
   Following to specfun.c made by John Grosh (jgrosh@arl.mil)
   on 28 OCT 1992.
   ---------------------------------------------------------------- */

void GpEval::f_normal(GpArgument * pArg)
{                               /* Normal or Gaussian Probability Function */
	t_value a;
	double x;
	/* ref. Abramowitz and Stegun 1964, "Handbook of Mathematical
	   Functions", Applied Mathematics Series, vol 55,
	   Chapter 26, page 934, Eqn. 26.2.29 and Jos van der Woude
	   code found above */
	x = PopOrConvertFromString(a).Real();
	x = 0.5 * SMathConst::Sqrt2 * x;
	x = 0.5 * erfc(-x); // by using erfc instead of erf, we can get accurate values for -38 < arg < -8 
	Push(a.SetComplex(x, 0.0));
}

void GpEval::f_inverse_normal(GpArgument * pArg)
{                               /* Inverse normal distribution function */
	t_value a;
	double x;
	x = PopOrConvertFromString(a).Real();
	if(x <= 0.0 || x >= 1.0) {
		undefined = true;
		Push(a.SetComplex(0.0, 0.0));
	}
	else {
		Push(a.SetComplex(inverse_normal_func(x), 0.0));
	}
}

void GpEval::f_inverse_erf(GpArgument * pArg)
{                               /* Inverse error function */
	t_value a;
	double x = PopOrConvertFromString(a).Real();
	if(fabs(x) >= 1.0) {
		undefined = true;
		Push(a.SetComplex(0.0, 0.0));
	}
	else {
		Push(a.SetComplex(inverse_error_func(x), 0.0));
	}
}

/*                                                      ndtri.c
 *
 *      Inverse of Normal distribution function
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, y, ndtri();
 *
 * x = ndtri( y );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns the argument, x, for which the area under the
 * Gaussian probability density function (integrated from
 * minus infinity to x) is equal to y.
 *
 *
 * For small arguments 0 < y < exp(-2), the program computes
 * z = sqrt( -2.0 * log(y) );  then the approximation is
 * x = z - log(z)/z  - (1/z) P(1/z) / Q(1/z).
 * There are two rational functions P/Q, one for 0 < y < exp(-32)
 * and the other for y up to exp(-2).  For larger arguments,
 * w = y - 0.5, and  x/sqrt(2pi) = w + w**3 R(w**2)/S(w**2)).
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain        # trials      peak         rms
 *    DEC      0.125, 1         5500       9.5e-17     2.1e-17
 *    DEC      6e-39, 0.135     3500       5.7e-17     1.3e-17
 *    IEEE     0.125, 1        20000       7.2e-16     1.3e-16
 *    IEEE     3e-308, 0.135   50000       4.6e-16     9.8e-17
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition    value returned
 * ndtri domain       x <= 0        -DBL_MAX
 * ndtri domain       x >= 1         DBL_MAX
 *
 */

/*
   Cephes Math Library Release 2.8:  June, 2000
   Copyright 1984, 1987, 1989, 2000 by Stephen L. Moshier
 */

#ifdef UNK
/* sqrt(2pi) */
static double s2pi = 2.50662827463100050242E0;
#endif

#ifdef DEC
static ushort s2p[] = {0040440, 0066230, 0177661, 0034055};
#define s2pi *(double*)s2p
#endif

#ifdef IBMPC
static ushort s2p[] = {0x2706, 0x1ff6, 0x0d93, 0x4004};
#define s2pi *(double*)s2p
#endif

#ifdef MIEEE
static ushort s2p[] = {
	0x4004, 0x0d93, 0x1ff6, 0x2706
};
#define s2pi *(double*)s2p
#endif

static double inverse_normal_func(double y0)
{
	/* approximation for 0 <= |y - 0.5| <= 3/8 */
#ifdef UNK
	static const double P0[5] = {
		-5.99633501014107895267E1,
		9.80010754185999661536E1,
		-5.66762857469070293439E1,
		1.39312609387279679503E1,
		-1.23916583867381258016E0,
	};
	static const double Q0[8] = {
		/* 1.00000000000000000000E0,*/
		1.95448858338141759834E0,
		4.67627912898881538453E0,
		8.63602421390890590575E1,
		-2.25462687854119370527E2,
		2.00260212380060660359E2,
		-8.20372256168333339912E1,
		1.59056225126211695515E1,
		-1.18331621121330003142E0,
	};
#endif
#ifdef DEC
	static const ushort P0[20] = {
		0141557, 0155170, 0071360, 0120550,
		0041704, 0000214, 0172417, 0067307,
		0141542, 0132204, 0040066, 0156723,
		0041136, 0163161, 0157276, 0007747,
		0140236, 0116374, 0073666, 0051764,
	};
	static const ushort Q0[32] = {
		/*0040200,0000000,0000000,0000000,*/
		0040372, 0026256, 0110403, 0123707,
		0040625, 0122024, 0020277, 0026661,
		0041654, 0134161, 0124134, 0007244,
		0142141, 0073162, 0133021, 0131371,
		0042110, 0041235, 0043516, 0057767,
		0141644, 0011417, 0036155, 0137305,
		0041176, 0076556, 0004043, 0125430,
		0140227, 0073347, 0152776, 0067251,
	};
#endif
#ifdef IBMPC
	static const ushort P0[20] = {
		0x142d, 0x0e5e, 0xfb4f, 0xc04d,
		0xedd9, 0x9ea1, 0x8011, 0x4058,
		0xdbba, 0x8806, 0x5690, 0xc04c,
		0xc1fd, 0x3bd7, 0xdcce, 0x402b,
		0xca7e, 0x8ef6, 0xd39f, 0xbff3,
	};
	static const ushort Q0[36] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0x74f9, 0xd220, 0x4595, 0x3fff,
		0xe5b6, 0x8417, 0xb482, 0x4012,
		0x81d4, 0x350b, 0x970e, 0x4055,
		0x365f, 0x56c2, 0x2ece, 0xc06c,
		0xcbff, 0xa8e9, 0x0853, 0x4069,
		0xb7d9, 0xe78d, 0x8261, 0xc054,
		0x7563, 0xc104, 0xcfad, 0x402f,
		0xcdd5, 0xfabf, 0xeedc, 0xbff2,
	};
#endif
#ifdef MIEEE
	static const ushort P0[20] = {
		0xc04d, 0xfb4f, 0x0e5e, 0x142d,
		0x4058, 0x8011, 0x9ea1, 0xedd9,
		0xc04c, 0x5690, 0x8806, 0xdbba,
		0x402b, 0xdcce, 0x3bd7, 0xc1fd,
		0xbff3, 0xd39f, 0x8ef6, 0xca7e,
	};
	static const ushort Q0[32] = {
		/*0x3ff0,0x0000,0x0000,0x0000,*/
		0x3fff, 0x4595, 0xd220, 0x74f9,
		0x4012, 0xb482, 0x8417, 0xe5b6,
		0x4055, 0x970e, 0x350b, 0x81d4,
		0xc06c, 0x2ece, 0x56c2, 0x365f,
		0x4069, 0x0853, 0xa8e9, 0xcbff,
		0xc054, 0x8261, 0xe78d, 0xb7d9,
		0x402f, 0xcfad, 0xc104, 0x7563,
		0xbff2, 0xeedc, 0xfabf, 0xcdd5,
	};
#endif

	/* Approximation for interval z = sqrt(-2 log y ) between 2 and 8
	 * i.e., y between exp(-2) = .135 and exp(-32) = 1.27e-14.
	 */
#ifdef UNK
	static const double P1[9] = {
		4.05544892305962419923E0,
		3.15251094599893866154E1,
		5.71628192246421288162E1,
		4.40805073893200834700E1,
		1.46849561928858024014E1,
		2.18663306850790267539E0,
		-1.40256079171354495875E-1,
		-3.50424626827848203418E-2,
		-8.57456785154685413611E-4,
	};
	static const double Q1[8] = {
		/*  1.00000000000000000000E0,*/
		1.57799883256466749731E1,
		4.53907635128879210584E1,
		4.13172038254672030440E1,
		1.50425385692907503408E1,
		2.50464946208309415979E0,
		-1.42182922854787788574E-1,
		-3.80806407691578277194E-2,
		-9.33259480895457427372E-4,
	};
#endif
#ifdef DEC
	static const ushort P1[36] = {
		0040601, 0143074, 0150744, 0073326,
		0041374, 0031554, 0113253, 0146016,
		0041544, 0123272, 0012463, 0176771,
		0041460, 0051160, 0103560, 0156511,
		0041152, 0172624, 0117772, 0030755,
		0040413, 0170713, 0151545, 0176413,
		0137417, 0117512, 0022154, 0131671,
		0137017, 0104257, 0071432, 0007072,
		0135540, 0143363, 0063137, 0036166,
	};
	static const ushort Q1[32] = {
		/*0040200,0000000,0000000,0000000,*/
		0041174, 0075325, 0004736, 0120326,
		0041465, 0110044, 0047561, 0045567,
		0041445, 0042321, 0012142, 0030340,
		0041160, 0127074, 0166076, 0141051,
		0040440, 0046055, 0040745, 0150400,
		0137421, 0114146, 0067330, 0010621,
		0137033, 0175162, 0025555, 0114351,
		0135564, 0122773, 0145750, 0030357,
	};
#endif
#ifdef IBMPC
	static const ushort P1[36] = {
		0x8edb, 0x9a3c, 0x38c7, 0x4010,
		0x7982, 0x92d5, 0x866d, 0x403f,
		0x7fbf, 0x42a6, 0x94d7, 0x404c,
		0x1ba9, 0x10ee, 0x0a4e, 0x4046,
		0x463e, 0x93ff, 0x5eb2, 0x402d,
		0xbfa1, 0x7a6c, 0x7e39, 0x4001,
		0x9677, 0x448d, 0xf3e9, 0xbfc1,
		0x41c7, 0xee63, 0xf115, 0xbfa1,
		0xe78f, 0x6ccb, 0x18de, 0xbf4c,
	};
	static const ushort Q1[32] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0xd41b, 0xa13b, 0x8f5a, 0x402f,
		0x296f, 0x89ee, 0xb204, 0x4046,
		0x461c, 0x228c, 0xa89a, 0x4044,
		0xd845, 0x9d87, 0x15c7, 0x402e,
		0xba20, 0xa83c, 0x0985, 0x4004,
		0x0232, 0xcddb, 0x330c, 0xbfc2,
		0xb31d, 0x456d, 0x7f4e, 0xbfa3,
		0x061e, 0x797d, 0x94bf, 0xbf4e,
	};
#endif
#ifdef MIEEE
	static const ushort P1[36] = {
		0x4010, 0x38c7, 0x9a3c, 0x8edb,
		0x403f, 0x866d, 0x92d5, 0x7982,
		0x404c, 0x94d7, 0x42a6, 0x7fbf,
		0x4046, 0x0a4e, 0x10ee, 0x1ba9,
		0x402d, 0x5eb2, 0x93ff, 0x463e,
		0x4001, 0x7e39, 0x7a6c, 0xbfa1,
		0xbfc1, 0xf3e9, 0x448d, 0x9677,
		0xbfa1, 0xf115, 0xee63, 0x41c7,
		0xbf4c, 0x18de, 0x6ccb, 0xe78f,
	};
	static const ushort Q1[32] = {
		/*0x3ff0,0x0000,0x0000,0x0000,*/
		0x402f, 0x8f5a, 0xa13b, 0xd41b,
		0x4046, 0xb204, 0x89ee, 0x296f,
		0x4044, 0xa89a, 0x228c, 0x461c,
		0x402e, 0x15c7, 0x9d87, 0xd845,
		0x4004, 0x0985, 0xa83c, 0xba20,
		0xbfc2, 0x330c, 0xcddb, 0x0232,
		0xbfa3, 0x7f4e, 0x456d, 0xb31d,
		0xbf4e, 0x94bf, 0x797d, 0x061e,
	};
#endif

	/* Approximation for interval z = sqrt(-2 log y ) between 8 and 64
	 * i.e., y between exp(-32) = 1.27e-14 and exp(-2048) = 3.67e-890.
	 */

#ifdef UNK
	static const double P2[9] = {
		3.23774891776946035970E0,
		6.91522889068984211695E0,
		3.93881025292474443415E0,
		1.33303460815807542389E0,
		2.01485389549179081538E-1,
		1.23716634817820021358E-2,
		3.01581553508235416007E-4,
		2.65806974686737550832E-6,
		6.23974539184983293730E-9,
	};
	static const double Q2[8] = {
		/*  1.00000000000000000000E0,*/
		6.02427039364742014255E0,
		3.67983563856160859403E0,
		1.37702099489081330271E0,
		2.16236993594496635890E-1,
		1.34204006088543189037E-2,
		3.28014464682127739104E-4,
		2.89247864745380683936E-6,
		6.79019408009981274425E-9,
	};
#endif
#ifdef DEC
	static const ushort P2[36] = {
		0040517, 0033507, 0036236, 0125641,
		0040735, 0044616, 0014473, 0140133,
		0040574, 0012567, 0114535, 0102541,
		0040252, 0120340, 0143474, 0150135,
		0037516, 0051057, 0115361, 0031211,
		0036512, 0131204, 0101511, 0125144,
		0035236, 0016627, 0043160, 0140216,
		0033462, 0060512, 0060141, 0010641,
		0031326, 0062541, 0101304, 0077706,
	};
	static const ushort Q2[32] = {
		/*0040200,0000000,0000000,0000000,*/
		0040700, 0143322, 0132137, 0040501,
		0040553, 0101155, 0053221, 0140257,
		0040260, 0041071, 0052573, 0010004,
		0037535, 0066472, 0177261, 0162330,
		0036533, 0160475, 0066666, 0036132,
		0035253, 0174533, 0027771, 0044027,
		0033502, 0016147, 0117666, 0063671,
		0031351, 0047455, 0141663, 0054751,
	};
#endif
#ifdef IBMPC
	static const ushort P2[36] = {
		0xd574, 0xe793, 0xe6e8, 0x4009,
		0x780b, 0xc327, 0xa931, 0x401b,
		0xb0ac, 0xf32b, 0x82ae, 0x400f,
		0x9a0c, 0x18e7, 0x541c, 0x3ff5,
		0x2651, 0xf35e, 0xca45, 0x3fc9,
		0x354d, 0x9069, 0x5650, 0x3f89,
		0x1812, 0xe8ce, 0xc3b2, 0x3f33,
		0x2234, 0x4c0c, 0x4c29, 0x3ec6,
		0x8ff9, 0x3058, 0xccac, 0x3e3a,
	};
	static const ushort Q2[32] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0xe828, 0x568b, 0x18da, 0x4018,
		0x3816, 0xaad2, 0x704d, 0x400d,
		0x6200, 0x2aaf, 0x0847, 0x3ff6,
		0x3c9b, 0x5fd6, 0xada7, 0x3fcb,
		0xc78b, 0xadb6, 0x7c27, 0x3f8b,
		0x2903, 0x65ff, 0x7f2b, 0x3f35,
		0xccf7, 0xf3f6, 0x438c, 0x3ec8,
		0x6b3d, 0xb876, 0x29e5, 0x3e3d,
	};
#endif
#ifdef MIEEE
	static const ushort P2[36] = {
		0x4009, 0xe6e8, 0xe793, 0xd574,
		0x401b, 0xa931, 0xc327, 0x780b,
		0x400f, 0x82ae, 0xf32b, 0xb0ac,
		0x3ff5, 0x541c, 0x18e7, 0x9a0c,
		0x3fc9, 0xca45, 0xf35e, 0x2651,
		0x3f89, 0x5650, 0x9069, 0x354d,
		0x3f33, 0xc3b2, 0xe8ce, 0x1812,
		0x3ec6, 0x4c29, 0x4c0c, 0x2234,
		0x3e3a, 0xccac, 0x3058, 0x8ff9,
	};
	static const ushort Q2[32] = {
		/*0x3ff0,0x0000,0x0000,0x0000,*/
		0x4018, 0x18da, 0x568b, 0xe828,
		0x400d, 0x704d, 0xaad2, 0x3816,
		0x3ff6, 0x0847, 0x2aaf, 0x6200,
		0x3fcb, 0xada7, 0x5fd6, 0x3c9b,
		0x3f8b, 0x7c27, 0xadb6, 0xc78b,
		0x3f35, 0x7f2b, 0x65ff, 0x2903,
		0x3ec8, 0x438c, 0xf3f6, 0xccf7,
		0x3e3d, 0x29e5, 0xb876, 0x6b3d,
	};
#endif

	double x, y, z, y2, x0, x1;
	int code;

	if(y0 <= 0.0) {
		mtherr("inverse_normal_func", MTHERR_DOMAIN);
		return (-DBL_MAX);
	}
	if(y0 >= 1.0) {
		mtherr("inverse_normal_func", MTHERR_DOMAIN);
		return (DBL_MAX);
	}
	code = 1;
	y = y0;
	if(y > (1.0 - 0.13533528323661269189)) { /* 0.135... = exp(-2) */
		y = 1.0 - y;
		code = 0;
	}
	if(y > 0.13533528323661269189) {
		y = y - 0.5;
		y2 = y * y;
		x = y + y * (y2 * polevl(y2, P0, 4) / p1evl(y2, Q0, 8));
		x = x * s2pi;
		return (x);
	}
	x = sqrt(-2.0 * log(y));
	x0 = x - log(x) / x;

	z = 1.0 / x;
	if(x < 8.0)             /* y > exp(-32) = 1.2664165549e-14 */
		x1 = z * polevl(z, P1, 8) / p1evl(z, Q1, 8);
	else
		x1 = z * polevl(z, P2, 8) / p1evl(z, Q2, 8);
	x = x0 - x1;
	if(code != 0)
		x = -x;
	return (x);
}

/*
   Cephes Math Library Release 2.8:  June, 2000
   Copyright 1984, 1987, 1988, 1992, 2000 by Stephen L. Moshier
 */

#ifndef HAVE_ERFC
/*                                                     erfc.c
 *
 *      Complementary error function
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, y, erfc();
 *
 * y = erfc( x );
 *
 *
 *
 * DESCRIPTION:
 *
 *
 *  1 - erf(x) =
 *
 *                           inf.
 *                             -
 *                  2         | |          2
 *   erfc(x)  =  --------     |    exp( - t  ) dt
 *               sqrt(pi)   | |
 *                           -
 *                            x
 *
 *
 * For small x, erfc(x) = 1 - erf(x); otherwise rational
 * approximations are computed.
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       0, 9.2319   12000       5.1e-16     1.2e-16
 *    IEEE      0,26.6417   30000       5.7e-14     1.5e-14
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition              value returned
 * erfc underflow    x > 9.231948545 (DEC)       0.0
 *
 *
 */

static double erfc(double a)
{
#ifdef UNK
	static const double P[] = {
		2.46196981473530512524E-10,
		5.64189564831068821977E-1,
		7.46321056442269912687E0,
		4.86371970985681366614E1,
		1.96520832956077098242E2,
		5.26445194995477358631E2,
		9.34528527171957607540E2,
		1.02755188689515710272E3,
		5.57535335369399327526E2
	};
	static const double Q[] = {
		/* 1.00000000000000000000E0,*/
		1.32281951154744992508E1,
		8.67072140885989742329E1,
		3.54937778887819891062E2,
		9.75708501743205489753E2,
		1.82390916687909736289E3,
		2.24633760818710981792E3,
		1.65666309194161350182E3,
		5.57535340817727675546E2
	};
	static const double R[] = {
		5.64189583547755073984E-1,
		1.27536670759978104416E0,
		5.01905042251180477414E0,
		6.16021097993053585195E0,
		7.40974269950448939160E0,
		2.97886665372100240670E0
	};
	static const double S[] = {
		/* 1.00000000000000000000E0,*/
		2.26052863220117276590E0,
		9.39603524938001434673E0,
		1.20489539808096656605E1,
		1.70814450747565897222E1,
		9.60896809063285878198E0,
		3.36907645100081516050E0
	};
#endif /* UNK */

#ifdef DEC
	static const ushort P[] = {
		0030207, 0054445, 0011173, 0021706,
		0040020, 0067272, 0030661, 0122075,
		0040756, 0151236, 0173053, 0067042,
		0041502, 0106175, 0062555, 0151457,
		0042104, 0102525, 0047401, 0003667,
		0042403, 0116176, 0011446, 0075303,
		0042551, 0120723, 0061641, 0123275,
		0042600, 0070651, 0007264, 0134516,
		0042413, 0061102, 0167507, 0176625
	};
	static const ushort Q[] = {
		/*0040200,0000000,0000000,0000000,*/
		0041123, 0123257, 0165741, 0017142,
		0041655, 0065027, 0173413, 0115450,
		0042261, 0074011, 0021573, 0004150,
		0042563, 0166530, 0013662, 0007200,
		0042743, 0176427, 0162443, 0105214,
		0043014, 0062546, 0153727, 0123772,
		0042717, 0012470, 0006227, 0067424,
		0042413, 0061103, 0003042, 0013254
	};
	static const ushort R[] = {
		0040020, 0067272, 0101024, 0155421,
		0040243, 0037467, 0056706, 0026462,
		0040640, 0116017, 0120665, 0034315,
		0040705, 0020162, 0143350, 0060137,
		0040755, 0016234, 0134304, 0130157,
		0040476, 0122700, 0051070, 0015473
	};
	static const ushort S[] = {
		/*0040200,0000000,0000000,0000000,*/
		0040420, 0126200, 0044276, 0070413,
		0041026, 0053051, 0007302, 0063746,
		0041100, 0144203, 0174051, 0061151,
		0041210, 0123314, 0126343, 0177646,
		0041031, 0137125, 0051431, 0033011,
		0040527, 0117362, 0152661, 0066201
	};
#endif /* DEC */

#ifdef IBMPC
	static const ushort P[] = {
		0x6479, 0xa24f, 0xeb24, 0x3df0,
		0x3488, 0x4636, 0x0dd7, 0x3fe2,
		0x6dc4, 0xdec5, 0xda53, 0x401d,
		0xba66, 0xacad, 0x518f, 0x4048,
		0x20f7, 0xa9e0, 0x90aa, 0x4068,
		0xcf58, 0xc264, 0x738f, 0x4080,
		0x34d8, 0x6c74, 0x343a, 0x408d,
		0x972a, 0x21d6, 0x0e35, 0x4090,
		0xffb3, 0x5de8, 0x6c48, 0x4081
	};
	static const ushort Q[] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0x23cc, 0xfd7c, 0x74d5, 0x402a,
		0x7365, 0xfee1, 0xad42, 0x4055,
		0x610d, 0x246f, 0x2f01, 0x4076,
		0x41d0, 0x02f6, 0x7dab, 0x408e,
		0x7151, 0xfca4, 0x7fa2, 0x409c,
		0xf4ff, 0xdafa, 0x8cac, 0x40a1,
		0xede2, 0x0192, 0xe2a7, 0x4099,
		0x42d6, 0x60c4, 0x6c48, 0x4081
	};
	static const ushort R[] = {
		0x9b62, 0x5042, 0x0dd7, 0x3fe2,
		0xc5a6, 0xebb8, 0x67e6, 0x3ff4,
		0xa71a, 0xf436, 0x1381, 0x4014,
		0x0c0c, 0x58dd, 0xa40e, 0x4018,
		0x960e, 0x9718, 0xa393, 0x401d,
		0x0367, 0x0a47, 0xd4b8, 0x4007
	};
	static const ushort S[] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0xce21, 0x0917, 0x1590, 0x4002,
		0x4cfd, 0x21d8, 0xcac5, 0x4022,
		0x2c4d, 0x7f05, 0x1910, 0x4028,
		0x7ff5, 0x959c, 0x14d9, 0x4031,
		0x26c1, 0xaa63, 0x37ca, 0x4023,
		0x2d90, 0x5ab6, 0xf3de, 0x400a
	};
#endif /* IBMPC */

#ifdef MIEEE
	static const ushort P[] = {
		0x3df0, 0xeb24, 0xa24f, 0x6479,
		0x3fe2, 0x0dd7, 0x4636, 0x3488,
		0x401d, 0xda53, 0xdec5, 0x6dc4,
		0x4048, 0x518f, 0xacad, 0xba66,
		0x4068, 0x90aa, 0xa9e0, 0x20f7,
		0x4080, 0x738f, 0xc264, 0xcf58,
		0x408d, 0x343a, 0x6c74, 0x34d8,
		0x4090, 0x0e35, 0x21d6, 0x972a,
		0x4081, 0x6c48, 0x5de8, 0xffb3
	};
	static const ushort Q[] = {
		0x402a, 0x74d5, 0xfd7c, 0x23cc,
		0x4055, 0xad42, 0xfee1, 0x7365,
		0x4076, 0x2f01, 0x246f, 0x610d,
		0x408e, 0x7dab, 0x02f6, 0x41d0,
		0x409c, 0x7fa2, 0xfca4, 0x7151,
		0x40a1, 0x8cac, 0xdafa, 0xf4ff,
		0x4099, 0xe2a7, 0x0192, 0xede2,
		0x4081, 0x6c48, 0x60c4, 0x42d6
	};
	static const ushort R[] = {
		0x3fe2, 0x0dd7, 0x5042, 0x9b62,
		0x3ff4, 0x67e6, 0xebb8, 0xc5a6,
		0x4014, 0x1381, 0xf436, 0xa71a,
		0x4018, 0xa40e, 0x58dd, 0x0c0c,
		0x401d, 0xa393, 0x9718, 0x960e,
		0x4007, 0xd4b8, 0x0a47, 0x0367
	};
	static const ushort S[] = {
		0x4002, 0x1590, 0x0917, 0xce21,
		0x4022, 0xcac5, 0x21d8, 0x4cfd,
		0x4028, 0x1910, 0x7f05, 0x2c4d,
		0x4031, 0x14d9, 0x959c, 0x7ff5,
		0x4023, 0x37ca, 0xaa63, 0x26c1,
		0x400a, 0xf3de, 0x5ab6, 0x2d90
	};
#endif /* MIEEE */

	double p, q, x, y, z;

	if(a < 0.0)
		x = -a;
	else
		x = a;

	if(x < 1.0)
		return (1.0 - erf(a));

	z = -a * a;

	if(z < DBL_MIN_10_EXP) {
under:
		mtherr("erfc", MTHERR_UNDERFLOW);
		if(a < 0)
			return (2.0);
		else
			return (0.0);
	}
	z = exp(z);

	if(x < 8.0) {
		p = polevl(x, P, 8);
		q = p1evl(x, Q, 8);
	}
	else {
		p = polevl(x, R, 5);
		q = p1evl(x, S, 6);
	}
	y = (z * p) / q;

	if(a < 0)
		y = 2.0 - y;

	if(y == 0.0)
		goto under;

	return (y);
}

#endif /* !HAVE_ERFC */

#ifndef HAVE_ERF
/*                                                     erf.c
 *
 *      Error function
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, y, erf();
 *
 * y = erf( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * The integral is
 *
 *                           x
 *                            -
 *                 2         | |          2
 *   erf(x)  =  --------     |    exp( - t  ) dt.
 *              sqrt(pi)   | |
 *                          -
 *                           0
 *
 * The magnitude of x is limited to 9.231948545 for DEC
 * arithmetic; 1 or -1 is returned outside this range.
 *
 * For 0 <= |x| < 1, erf(x) = x * P4(x**2)/Q5(x**2); otherwise
 * erf(x) = 1 - erfc(x).
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       0,1         14000       4.7e-17     1.5e-17
 *    IEEE      0,1         30000       3.7e-16     1.0e-16
 *
 */

static double erf(double x)
{
# ifdef UNK
	static const double T[] = {
		9.60497373987051638749E0,
		9.00260197203842689217E1,
		2.23200534594684319226E3,
		7.00332514112805075473E3,
		5.55923013010394962768E4
	};
	static const double U[] = {
		/* 1.00000000000000000000E0,*/
		3.35617141647503099647E1,
		5.21357949780152679795E2,
		4.59432382970980127987E3,
		2.26290000613890934246E4,
		4.92673942608635921086E4
	};
# endif

# ifdef DEC
	static const ushort T[] = {
		0041031, 0126770, 0170672, 0166101,
		0041664, 0006522, 0072360, 0031770,
		0043013, 0100025, 0162641, 0126671,
		0043332, 0155231, 0161627, 0076200,
		0044131, 0024115, 0021020, 0117343
	};
	static const ushort U[] = {
		/*0040200,0000000,0000000,0000000,*/
		0041406, 0037461, 0177575, 0032714,
		0042402, 0053350, 0123061, 0153557,
		0043217, 0111227, 0032007, 0164217,
		0043660, 0145000, 0004013, 0160114,
		0044100, 0071544, 0167107, 0125471
	};
# endif

# ifdef IBMPC
	static const ushort T[] = {
		0x5d88, 0x1e37, 0x35bf, 0x4023,
		0x067f, 0x4e9e, 0x81aa, 0x4056,
		0x35b7, 0xbcb4, 0x7002, 0x40a1,
		0xef90, 0x3c72, 0x5b53, 0x40bb,
		0x13dc, 0xa442, 0x2509, 0x40eb
	};
	static const ushort U[] = {
		/*0x0000,0x0000,0x0000,0x3ff0,*/
		0xa6ba, 0x3fef, 0xc7e6, 0x4040,
		0x3aee, 0x14c6, 0x4add, 0x4080,
		0xfd12, 0xe680, 0xf252, 0x40b1,
		0x7c0a, 0x0101, 0x1940, 0x40d6,
		0xf567, 0x9dc8, 0x0e6c, 0x40e8
	};
# endif

# ifdef MIEEE
	static const ushort T[] = {
		0x4023, 0x35bf, 0x1e37, 0x5d88,
		0x4056, 0x81aa, 0x4e9e, 0x067f,
		0x40a1, 0x7002, 0xbcb4, 0x35b7,
		0x40bb, 0x5b53, 0x3c72, 0xef90,
		0x40eb, 0x2509, 0xa442, 0x13dc
	};
	static const ushort U[] = {
		0x4040, 0xc7e6, 0x3fef, 0xa6ba,
		0x4080, 0x4add, 0x14c6, 0x3aee,
		0x40b1, 0xf252, 0xe680, 0xfd12,
		0x40d6, 0x1940, 0x0101, 0x7c0a,
		0x40e8, 0x0e6c, 0x9dc8, 0xf567
	};
# endif

	double y, z;

	if(fabs(x) > 1.0)
		return (1.0 - erfc(x));
	z = x * x;
	y = x * polevl(z, T, 4) / p1evl(z, U, 5);
	return (y);
}

#endif /* !HAVE_ERF */

/* ----------------------------------------------------------------
   Following function for the inverse error function is taken from
   NIST on 16. May 2002.
   Use Newton-Raphson correction also for range -1 to -y0 and
   add 3rd cycle to improve convergence -  E A Merritt 21.10.2003
   ----------------------------------------------------------------
 */

static double inverse_error_func(double y)
{
	double x = 0.0; /* The output */
	double z = 0.0; /* Intermadiate variable */
	double y0 = 0.7; /* Central range variable */

	/* Coefficients in rational approximations. */
	static const double a[4] = {
		0.886226899, -1.645349621, 0.914624893, -0.140543331
	};
	static const double b[4] = {
		-2.118377725, 1.442710462, -0.329097515, 0.012229801
	};
	static const double c[4] = {
		-1.970840454, -1.624906493, 3.429567803, 1.641345311
	};
	static const double d[2] = {
		3.543889200, 1.637067800
	};

	if((y < -1.0) || (1.0 < y)) {
		printf("inverse_error_func: The value out of the range of the function");
		x = log(-1.0);
		return (x);
	}
	else if((y == -1.0) || (1.0 == y)) {
		x = -y * log(0.0);
		return (x);
	}
	else if((-1.0 < y) && (y < -y0)) {
		z = sqrt(-log((1.0 + y) / 2.0));
		x = -(((c[3] * z + c[2]) * z + c[1]) * z + c[0]) / ((d[1] * z + d[0]) * z + 1.0);
	}
	else {
		if((-y0 <= y) && (y <= y0)) {
			z = y * y;
			x = y * (((a[3] * z + a[2]) * z + a[1]) * z + a[0]) /
			    ((((b[3] * z + b[3]) * z + b[1]) * z + b[0]) * z + 1.0);
		}
		else if((y0 < y) && (y < 1.0)) {
			z = sqrt(-log((1.0 - y) / 2.0));
			x = (((c[3] * z + c[2]) * z + c[1]) * z + c[0]) / ((d[1] * z + d[0]) * z + 1.0);
		}
	}
	/* Three steps of Newton-Raphson correction to full accuracy. OK - four */
	x = x - (erf(x) - y) / (2.0 / SMathConst::SqrtPi * gp_exp(-x * x));
	x = x - (erf(x) - y) / (2.0 / SMathConst::SqrtPi * gp_exp(-x * x));
	x = x - (erf(x) - y) / (2.0 / SMathConst::SqrtPi * gp_exp(-x * x));
	x = x - (erf(x) - y) / (2.0 / SMathConst::SqrtPi * gp_exp(-x * x));

	return (x);
}

/* Implementation of Lamberts W-function which is defined as
 * w(x)*e^(w(x))=x
 * Implementation by Gunter Kuhnle, gk@uni-leipzig.de
 * Algorithm originally developed by
 * KEITH BRIGGS, DEPARTMENT OF PLANT SCIENCES,
 * e-mail:kmb28@cam.ac.uk
 * http://epidem13.plantsci.cam.ac.uk/~kbriggs/W-ology.html */

static double lambertw(double x)
{
	double p, e, t, w;
	int i;
	double eps = MACHEPS;
	if(x < -exp(-1.0))
		return -1;      /* error, value undefined */

	if(fabs(x) <= eps)
		return x;

	if(x < 1) {
		p = sqrt(2.0 * (exp(1.0) * x + 1.0));
		w = -1.0 + p - p * p / 3.0 + 11.0 / 72.0 * p * p * p;
	}
	else {
		w = log(x);
	}

	if(x > 3) {
		w = w - log(w);
	}
	for(i = 0; i < 20; i++) {
		e = gp_exp(w);
		t = w * e - x;
		t = t / (e * (w + 1.0) - 0.5 * (w + 2.0) * t / (w + 1.0));
		w = w - t;
		if(fabs(t) < eps * (1.0 + fabs(w)))
			return w;
	}
	return -1;             /* error: iteration didn't converge */
}

void GpEval::f_lambertw(GpArgument * pArg)
{
	t_value a;
	double x = PopOrConvertFromString(a).Real();
	x = lambertw(x);
	if(x <= -1)
		undefined = true; // Error return from lambertw --> flag 'undefined' 
	Push(a.SetComplex(x, 0.0));
}

#if(0) /* This approximation of the airy function saves 2200 bytes relative
         * to the one from Cephes, but has low precision (2% relative error)
         */
/* ------------------------------------------------------------
   Airy Function Ai(x)

   After:
   "Two-Point Quasi-Fractional Approximations to the Airy Function Ai(x)"
   by Pablo Martin, Ricardo Perez, Antonio L. Guerrero
   Journal of Computational Physics 99, 337-340 (1992)

   Beware of a misprint in equation (5) in this paper: The second term in
   parentheses must be multiplied by "x", as is clear from equation (3)
   and by comparison with equation (6). The implementation in this file
   uses the CORRECT formula (with the "x").

   This is not a very high accuracy approximation, but sufficient for
   plotting and similar applications. Higher accuracy formulas are
   available, but are much more complicated (typically requiring iteration).

   Added: janert (PKJ) 2009-09-05
   ------------------------------------------------------------ */

static double airy_neg(double x) 
{
	double z = sqrt(0.37 + pow(fabs(x), 3.0) );
	double t = (2.0/3.0)*pow(fabs(x), 1.5);
	double y = 0;
	y += ( -0.043883564 + 0.3989422*z )*cos(t)/pow(z, 7.0/6.0);
	y += x*( -0.013883003 - 0.3989422*z )*sin(t)/( pow(z, 5.0/6.0) * 1.5 * t );
	return y;
}

static double airy_pos(double x) 
{
	double z = sqrt(0.0425 + pow(fabs(x), 3.0) );
	double y = 0;
	y += (-0.002800908 + 0.326662423*z )/pow(z, 7.0/6.0);
	y += x * ( -0.007232251 - 0.044567423*z )/pow(z, 11.0/6.0);
	y *= exp(-(2.0/3.0)*z);
	return y;
}

void GpEval::f_airy(GpArgument * pArg)
{
	t_value a;
	(void)arg;                     
	double x = pop__(&a)->Real();
	if(x < 0) {
		x = airy_neg(x);
	}
	else {
		x = airy_pos(x);
	}
	Push(a.SetComplex(x, 0.0));
}

#else // Airy function from the Cephes library 
/*							airy.c
 *
 *	Airy function
 *
 * SYNOPSIS:
 *
 * double x, ai, aip, bi, bip;
 * int airy();
 *
 * airy( x, _&ai, _&aip, _&bi, _&bip );
 *
 *
 * DESCRIPTION:
 *
 * Solution of the differential equation
 *
 *	y"(x) = xy.
 *
 * The function returns the two independent solutions Ai, Bi
 * and their first derivatives Ai'(x), Bi'(x).
 *
 * Evaluation is by power series summation for small x,
 * by rational minimax approximations for large x.
 *
 *
 * ACCURACY:
 * Error criterion is absolute when function <= 1, relative
 * when function > 1, except * denotes relative error criterion.
 * For large negative x, the absolute error increases as x^1.5.
 * For large positive x, the relative error increases as x^1.5.
 *
 * Arithmetic  domain   function  # trials      peak         rms
 * IEEE        -10, 0     Ai        10000       1.6e-15     2.7e-16
 * IEEE          0, 10    Ai        10000       2.3e-14*    1.8e-15*
 * IEEE        -10, 0     Ai'       10000       4.6e-15     7.6e-16
 * IEEE          0, 10    Ai'       10000       1.8e-14*    1.5e-15*
 * IEEE        -10, 10    Bi        30000       4.2e-15     5.3e-16
 * IEEE        -10, 10    Bi'       30000       4.9e-15     7.3e-16
 * DEC         -10, 0     Ai         5000       1.7e-16     2.8e-17
 * DEC           0, 10    Ai         5000       2.1e-15*    1.7e-16*
 * DEC         -10, 0     Ai'        5000       4.7e-16     7.8e-17
 * DEC           0, 10    Ai'       12000       1.8e-15*    1.5e-16*
 * DEC         -10, 10    Bi        10000       5.5e-16     6.8e-17
 * DEC         -10, 10    Bi'        7000       5.3e-16     8.7e-17
 *
 */

/*
   Cephes Math Library Release 2.8:  June, 2000
   Copyright 1984, 1987, 1989, 2000 by Stephen L. Moshier
 */

static double c1 = 0.35502805388781723926;
static double c2 = 0.258819403792806798405;
static double sqrt3 = 1.732050807568877293527;
static double sqpii = 5.64189583547756286948E-1;

#ifdef UNK
	#define MAXAIRY 25.77
#endif
#ifdef DEC
	#define MAXAIRY 25.77
#endif
#ifdef IBMPC
	#define MAXAIRY 103.892
#endif
#ifdef MIEEE
	#define MAXAIRY 103.892
#endif

#ifdef UNK
static double AN[8] = {
	3.46538101525629032477E-1,
	1.20075952739645805542E1,
	7.62796053615234516538E1,
	1.68089224934630576269E2,
	1.59756391350164413639E2,
	7.05360906840444183113E1,
	1.40264691163389668864E1,
	9.99999999999999995305E-1,
};
static double AD[8] = {
	5.67594532638770212846E-1,
	1.47562562584847203173E1,
	8.45138970141474626562E1,
	1.77318088145400459522E2,
	1.64234692871529701831E2,
	7.14778400825575695274E1,
	1.40959135607834029598E1,
	1.00000000000000000470E0,
};
#endif
#ifdef DEC
static ushort AN[32] = {
	0037661, 0066561, 0024675, 0131301,
	0041100, 0017434, 0034324, 0101466,
	0041630, 0107450, 0067427, 0007430,
	0042050, 0013327, 0071000, 0034737,
	0042037, 0140642, 0156417, 0167366,
	0041615, 0011172, 0075147, 0051165,
	0041140, 0066152, 0160520, 0075146,
	0040200, 0000000, 0000000, 0000000,
};
static ushort AD[32] = {
	0040021, 0046740, 0011422, 0064606,
	0041154, 0014640, 0024631, 0062450,
	0041651, 0003435, 0101152, 0106401,
	0042061, 0050556, 0034605, 0136602,
	0042044, 0036024, 0152377, 0151414,
	0041616, 0172247, 0072216, 0115374,
	0041141, 0104334, 0124154, 0166007,
	0040200, 0000000, 0000000, 0000000,
};
#endif
#ifdef IBMPC
static ushort AN[32] = {
	0xb658, 0x2537, 0x2dae, 0x3fd6,
	0x9067, 0x871a, 0x03e3, 0x4028,
	0xe1e3, 0x0de2, 0x11e5, 0x4053,
	0x073c, 0xee40, 0x02da, 0x4065,
	0xfddf, 0x5ba1, 0xf834, 0x4063,
	0xea4f, 0x4f4c, 0xa24f, 0x4051,
	0x0f4d, 0x5c2a, 0x0d8d, 0x402c,
	0x0000, 0x0000, 0x0000, 0x3ff0,
};
static ushort AD[32] = {
	0x4d31, 0x0262, 0x29bc, 0x3fe2,
	0x2ca5, 0x0533, 0x8334, 0x402d,
	0x51a0, 0xb04d, 0x20e3, 0x4055,
	0xb7b0, 0xc730, 0x2a2d, 0x4066,
	0xfa61, 0x9a9f, 0x8782, 0x4064,
	0xd35f, 0xee91, 0xde94, 0x4051,
	0x9d81, 0x950d, 0x311b, 0x402c,
	0x0000, 0x0000, 0x0000, 0x3ff0,
};
#endif
#ifdef MIEEE
static ushort AN[32] = {
	0x3fd6, 0x2dae, 0x2537, 0xb658,
	0x4028, 0x03e3, 0x871a, 0x9067,
	0x4053, 0x11e5, 0x0de2, 0xe1e3,
	0x4065, 0x02da, 0xee40, 0x073c,
	0x4063, 0xf834, 0x5ba1, 0xfddf,
	0x4051, 0xa24f, 0x4f4c, 0xea4f,
	0x402c, 0x0d8d, 0x5c2a, 0x0f4d,
	0x3ff0, 0x0000, 0x0000, 0x0000,
};
static ushort AD[32] = {
	0x3fe2, 0x29bc, 0x0262, 0x4d31,
	0x402d, 0x8334, 0x0533, 0x2ca5,
	0x4055, 0x20e3, 0xb04d, 0x51a0,
	0x4066, 0x2a2d, 0xc730, 0xb7b0,
	0x4064, 0x8782, 0x9a9f, 0xfa61,
	0x4051, 0xde94, 0xee91, 0xd35f,
	0x402c, 0x311b, 0x950d, 0x9d81,
	0x3ff0, 0x0000, 0x0000, 0x0000,
};
#endif

#ifdef UNK
static double APN[8] = {
	6.13759184814035759225E-1,
	1.47454670787755323881E1,
	8.20584123476060982430E1,
	1.71184781360976385540E2,
	1.59317847137141783523E2,
	6.99778599330103016170E1,
	1.39470856980481566958E1,
	1.00000000000000000550E0,
};
static double APD[8] = {
	3.34203677749736953049E-1,
	1.11810297306158156705E1,
	7.11727352147859965283E1,
	1.58778084372838313640E2,
	1.53206427475809220834E2,
	6.86752304592780337944E1,
	1.38498634758259442477E1,
	9.99999999999999994502E-1,
};
#endif
#ifdef DEC
static ushort APN[32] = {
	0040035, 0017522, 0065145, 0054755,
	0041153, 0166556, 0161471, 0057174,
	0041644, 0016750, 0034445, 0046462,
	0042053, 0027515, 0152316, 0046717,
	0042037, 0050536, 0067023, 0023264,
	0041613, 0172252, 0007240, 0131055,
	0041137, 0023503, 0052472, 0002305,
	0040200, 0000000, 0000000, 0000000,
};
static ushort APD[32] = {
	0037653, 0016276, 0112106, 0126625,
	0041062, 0162577, 0067111, 0111761,
	0041616, 0054160, 0140004, 0137455,
	0042036, 0143460, 0104626, 0157206,
	0042031, 0032330, 0067131, 0114260,
	0041611, 0054667, 0147207, 0134564,
	0041135, 0114412, 0070653, 0146015,
	0040200, 0000000, 0000000, 0000000,
};
#endif
#ifdef IBMPC
static ushort APN[32] = {
	0xab3e, 0x4d4c, 0xa3ea, 0x3fe3,
	0x2bcf, 0xdc67, 0x7dad, 0x402d,
	0xa9a6, 0x0724, 0x83bd, 0x4054,
	0xc9ba, 0xba99, 0x65e9, 0x4065,
	0x64d7, 0xcdc2, 0xea2b, 0x4063,
	0x1646, 0x41d4, 0x7e95, 0x4051,
	0x4099, 0x6aa7, 0xe4e8, 0x402b,
	0x0000, 0x0000, 0x0000, 0x3ff0,
};
static ushort APD[32] = {
	0xd5b3, 0xd288, 0x6397, 0x3fd5,
	0x327e, 0xedc9, 0x5caf, 0x4026,
	0x97e6, 0x1800, 0xcb0e, 0x4051,
	0xdbd1, 0x1132, 0xd8e6, 0x4063,
	0x3316, 0x0dcb, 0x269b, 0x4063,
	0xf72f, 0xf9d0, 0x2b36, 0x4051,
	0x7982, 0x4e35, 0xb321, 0x402b,
	0x0000, 0x0000, 0x0000, 0x3ff0,
};
#endif
#ifdef MIEEE
static ushort APN[32] = {
	0x3fe3, 0xa3ea, 0x4d4c, 0xab3e,
	0x402d, 0x7dad, 0xdc67, 0x2bcf,
	0x4054, 0x83bd, 0x0724, 0xa9a6,
	0x4065, 0x65e9, 0xba99, 0xc9ba,
	0x4063, 0xea2b, 0xcdc2, 0x64d7,
	0x4051, 0x7e95, 0x41d4, 0x1646,
	0x402b, 0xe4e8, 0x6aa7, 0x4099,
	0x3ff0, 0x0000, 0x0000, 0x0000,
};
static ushort APD[32] = {
	0x3fd5, 0x6397, 0xd288, 0xd5b3,
	0x4026, 0x5caf, 0xedc9, 0x327e,
	0x4051, 0xcb0e, 0x1800, 0x97e6,
	0x4063, 0xd8e6, 0x1132, 0xdbd1,
	0x4063, 0x269b, 0x0dcb, 0x3316,
	0x4051, 0x2b36, 0xf9d0, 0xf72f,
	0x402b, 0xb321, 0x4e35, 0x7982,
	0x3ff0, 0x0000, 0x0000, 0x0000,
};
#endif

#ifdef UNK
static double BN16[5] = {
	-2.53240795869364152689E-1,
	5.75285167332467384228E-1,
	-3.29907036873225371650E-1,
	6.44404068948199951727E-2,
	-3.82519546641336734394E-3,
};
static double BD16[5] = {
/* 1.00000000000000000000E0,*/
	-7.15685095054035237902E0,
	1.06039580715664694291E1,
	-5.23246636471251500874E0,
	9.57395864378383833152E-1,
	-5.50828147163549611107E-2,
};
#endif
#ifdef DEC
static ushort BN16[20] = {
	0137601, 0124307, 0010213, 0035210,
	0040023, 0042743, 0101621, 0016031,
	0137650, 0164623, 0036056, 0074511,
	0037203, 0174525, 0000473, 0142474,
	0136172, 0130041, 0066726, 0064324,
};
static ushort BD16[20] = {
/*0040200,0000000,0000000,0000000,*/
	0140745, 0002354, 0044335, 0055276,
	0041051, 0124717, 0170130, 0104013,
	0140647, 0070135, 0046473, 0103501,
	0040165, 0013745, 0033324, 0127766,
	0137141, 0117204, 0076164, 0033107,
};
#endif
#ifdef IBMPC
static ushort BN16[20] = {
	0x6751, 0xe211, 0x3518, 0xbfd0,
	0x2383, 0x7072, 0x68bc, 0x3fe2,
	0xcf29, 0x6785, 0x1d32, 0xbfd5,
	0x78a8, 0xa027, 0x7f2a, 0x3fb0,
	0xcd1b, 0x2dba, 0x5604, 0xbf6f,
};
static ushort BD16[20] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
	0xab58, 0x891b, 0xa09d, 0xc01c,
	0x1101, 0xfe0b, 0x3539, 0x4025,
	0x70e8, 0xa9a7, 0xee0b, 0xc014,
	0x95ff, 0xa6da, 0xa2fc, 0x3fee,
	0x86c9, 0x8f8e, 0x33d0, 0xbfac,
};
#endif
#ifdef MIEEE
static ushort BN16[20] = {
	0xbfd0, 0x3518, 0xe211, 0x6751,
	0x3fe2, 0x68bc, 0x7072, 0x2383,
	0xbfd5, 0x1d32, 0x6785, 0xcf29,
	0x3fb0, 0x7f2a, 0xa027, 0x78a8,
	0xbf6f, 0x5604, 0x2dba, 0xcd1b,
};
static ushort BD16[20] = {
/*0x3ff0,0x0000,0x0000,0x0000,*/
	0xc01c, 0xa09d, 0x891b, 0xab58,
	0x4025, 0x3539, 0xfe0b, 0x1101,
	0xc014, 0xee0b, 0xa9a7, 0x70e8,
	0x3fee, 0xa2fc, 0xa6da, 0x95ff,
	0xbfac, 0x33d0, 0x8f8e, 0x86c9,
};
#endif

#ifdef UNK
static double BPPN[5] = {
	4.65461162774651610328E-1,
	-1.08992173800493920734E0,
	6.38800117371827987759E-1,
	-1.26844349553102907034E-1,
	7.62487844342109852105E-3,
};
static double BPPD[5] = {
/* 1.00000000000000000000E0,*/
	-8.70622787633159124240E0,
	1.38993162704553213172E1,
	-7.14116144616431159572E0,
	1.34008595960680518666E0,
	-7.84273211323341930448E-2,
};
#endif
#ifdef DEC
static ushort BPPN[20] = {
	0037756, 0050354, 0167531, 0135731,
	0140213, 0101216, 0032767, 0020375,
	0040043, 0104147, 0106312, 0177632,
	0137401, 0161574, 0032015, 0043714,
	0036371, 0155035, 0143165, 0142262,
};
static ushort BPPD[20] = {
/*0040200,0000000,0000000,0000000,*/
	0141013, 0046265, 0115005, 0161053,
	0041136, 0061631, 0072445, 0156131,
	0140744, 0102145, 0001127, 0065304,
	0040253, 0103757, 0146453, 0102513,
	0137240, 0117200, 0155402, 0113500,
};
#endif
#ifdef IBMPC
static ushort BPPN[20] = {
	0x377b, 0x9deb, 0xca1d, 0x3fdd,
	0xe420, 0xc6be, 0x7051, 0xbff1,
	0x5ff3, 0xf199, 0x710c, 0x3fe4,
	0xa8fa, 0x8681, 0x3c6f, 0xbfc0,
	0xb896, 0xb8ce, 0x3b43, 0x3f7f,
};
static ushort BPPD[20] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
	0xbc45, 0xb340, 0x6996, 0xc021,
	0xbb8b, 0x2ea4, 0xcc73, 0x402b,
	0xed59, 0xa04a, 0x908c, 0xc01c,
	0x70a9, 0xf9a5, 0x70fd, 0x3ff5,
	0x52e8, 0x1b60, 0x13d0, 0xbfb4,
};
#endif
#ifdef MIEEE
static ushort BPPN[20] = {
	0x3fdd, 0xca1d, 0x9deb, 0x377b,
	0xbff1, 0x7051, 0xc6be, 0xe420,
	0x3fe4, 0x710c, 0xf199, 0x5ff3,
	0xbfc0, 0x3c6f, 0x8681, 0xa8fa,
	0x3f7f, 0x3b43, 0xb8ce, 0xb896,
};
static ushort BPPD[20] = {
/*0x3ff0,0x0000,0x0000,0x0000,*/
	0xc021, 0x6996, 0xb340, 0xbc45,
	0x402b, 0xcc73, 0x2ea4, 0xbb8b,
	0xc01c, 0x908c, 0xa04a, 0xed59,
	0x3ff5, 0x70fd, 0xf9a5, 0x70a9,
	0xbfb4, 0x13d0, 0x1b60, 0x52e8,
};
#endif

#ifdef UNK
static double AFN[9] = {
	-1.31696323418331795333E-1,
	-6.26456544431912369773E-1,
	-6.93158036036933542233E-1,
	-2.79779981545119124951E-1,
	-4.91900132609500318020E-2,
	-4.06265923594885404393E-3,
	-1.59276496239262096340E-4,
	-2.77649108155232920844E-6,
	-1.67787698489114633780E-8,
};
static double AFD[9] = {
/* 1.00000000000000000000E0,*/
	1.33560420706553243746E1,
	3.26825032795224613948E1,
	2.67367040941499554804E1,
	9.18707402907259625840E0,
	1.47529146771666414581E0,
	1.15687173795188044134E-1,
	4.40291641615211203805E-3,
	7.54720348287414296618E-5,
	4.51850092970580378464E-7,
};
#endif
#ifdef DEC
static ushort AFN[36] = {
	0137406, 0155546, 0124127, 0033732,
	0140040, 0057564, 0141263, 0041222,
	0140061, 0071316, 0013674, 0175754,
	0137617, 0037522, 0056637, 0120130,
	0137111, 0075567, 0121755, 0166122,
	0136205, 0020016, 0043317, 0002201,
	0135047, 0001565, 0075130, 0002334,
	0133472, 0051700, 0165021, 0131551,
	0131620, 0020347, 0132165, 0013215,
};
static ushort AFD[36] = {
/*0040200,0000000,0000000,0000000,*/
	0041125, 0131131, 0025627, 0067623,
	0041402, 0135342, 0021703, 0154315,
	0041325, 0162305, 0016671, 0120175,
	0041022, 0177101, 0053114, 0141632,
	0040274, 0153131, 0147364, 0114306,
	0037354, 0166545, 0120042, 0150530,
	0036220, 0043127, 0000727, 0130273,
	0034636, 0043275, 0075667, 0034733,
	0032762, 0112715, 0146250, 0142474,
};
#endif
#ifdef IBMPC
static ushort AFN[36] = {
	0xe6fb, 0xd50a, 0xdb6c, 0xbfc0,
	0x6852, 0x9856, 0x0bee, 0xbfe4,
	0x9f7d, 0xc2f7, 0x2e59, 0xbfe6,
	0xf40b, 0x4bb3, 0xe7ea, 0xbfd1,
	0xbd8a, 0xf47d, 0x2f6e, 0xbfa9,
	0xe090, 0xc8d9, 0xa401, 0xbf70,
	0x009c, 0xaf4b, 0xe06e, 0xbf24,
	0x366d, 0x1d42, 0x4a78, 0xbec7,
	0xa2d2, 0xf68e, 0x041c, 0xbe52,
};
static ushort AFD[36] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
	0xedf2, 0x2572, 0xb64b, 0x402a,
	0x7b1a, 0x4478, 0x575c, 0x4040,
	0x3410, 0xa3b7, 0xbc98, 0x403a,
	0x9873, 0x2ac9, 0x5fc8, 0x4022,
	0x9319, 0x39de, 0x9acb, 0x3ff7,
	0x5a2b, 0xb404, 0x9dac, 0x3fbd,
	0xf617, 0xe03a, 0x08ca, 0x3f72,
	0xe73b, 0xaf76, 0xc8d7, 0x3f13,
	0x18a7, 0xb995, 0x52b9, 0x3e9e,
};
#endif
#ifdef MIEEE
static ushort AFN[36] = {
	0xbfc0, 0xdb6c, 0xd50a, 0xe6fb,
	0xbfe4, 0x0bee, 0x9856, 0x6852,
	0xbfe6, 0x2e59, 0xc2f7, 0x9f7d,
	0xbfd1, 0xe7ea, 0x4bb3, 0xf40b,
	0xbfa9, 0x2f6e, 0xf47d, 0xbd8a,
	0xbf70, 0xa401, 0xc8d9, 0xe090,
	0xbf24, 0xe06e, 0xaf4b, 0x009c,
	0xbec7, 0x4a78, 0x1d42, 0x366d,
	0xbe52, 0x041c, 0xf68e, 0xa2d2,
};
static ushort AFD[36] = {
/*0x3ff0,0x0000,0x0000,0x0000,*/
	0x402a, 0xb64b, 0x2572, 0xedf2,
	0x4040, 0x575c, 0x4478, 0x7b1a,
	0x403a, 0xbc98, 0xa3b7, 0x3410,
	0x4022, 0x5fc8, 0x2ac9, 0x9873,
	0x3ff7, 0x9acb, 0x39de, 0x9319,
	0x3fbd, 0x9dac, 0xb404, 0x5a2b,
	0x3f72, 0x08ca, 0xe03a, 0xf617,
	0x3f13, 0xc8d7, 0xaf76, 0xe73b,
	0x3e9e, 0x52b9, 0xb995, 0x18a7,
};
#endif

#ifdef UNK
static double AGN[11] = {
	1.97339932091685679179E-2,
	3.91103029615688277255E-1,
	1.06579897599595591108E0,
	9.39169229816650230044E-1,
	3.51465656105547619242E-1,
	6.33888919628925490927E-2,
	5.85804113048388458567E-3,
	2.82851600836737019778E-4,
	6.98793669997260967291E-6,
	8.11789239554389293311E-8,
	3.41551784765923618484E-10,
};
static double AGD[10] = {
/*  1.00000000000000000000E0,*/
	9.30892908077441974853E0,
	1.98352928718312140417E1,
	1.55646628932864612953E1,
	5.47686069422975497931E0,
	9.54293611618961883998E-1,
	8.64580826352392193095E-2,
	4.12656523824222607191E-3,
	1.01259085116509135510E-4,
	1.17166733214413521882E-6,
	4.91834570062930015649E-9,
};
#endif
#ifdef DEC
static ushort AGN[44] = {
	0036641, 0124456, 0167175, 0157354,
	0037710, 0037250, 0001441, 0136671,
	0040210, 0066031, 0150401, 0123532,
	0040160, 0066545, 0003570, 0153133,
	0037663, 0171516, 0072507, 0170345,
	0037201, 0151011, 0007510, 0045702,
	0036277, 0172317, 0104572, 0101030,
	0035224, 0045663, 0000160, 0136422,
	0033752, 0074753, 0047702, 0135160,
	0032256, 0052225, 0156550, 0107103,
	0030273, 0142443, 0166277, 0071720,
};
static ushort AGD[40] = {
/*0040200,0000000,0000000,0000000,*/
	0041024, 0170537, 0117253, 0055003,
	0041236, 0127256, 0003570, 0143240,
	0041171, 0004333, 0172476, 0160645,
	0040657, 0041161, 0055716, 0157161,
	0040164, 0046226, 0006257, 0063431,
	0037261, 0010357, 0065445, 0047563,
	0036207, 0034043, 0057434, 0116732,
	0034724, 0055416, 0130035, 0026377,
	0033235, 0041056, 0154071, 0023502,
	0031250, 0177071, 0167254, 0047242,
};
#endif
#ifdef IBMPC
static ushort AGN[44] = {
	0xbbde, 0xddcf, 0x3525, 0x3f94,
	0x37b7, 0x0064, 0x07d5, 0x3fd9,
	0x34eb, 0x3a20, 0x0d83, 0x3ff1,
	0x1acb, 0xa0ef, 0x0dac, 0x3fee,
	0xfe1d, 0xcea8, 0x7e69, 0x3fd6,
	0x0978, 0x21e9, 0x3a41, 0x3fb0,
	0x5043, 0xf12f, 0xfe99, 0x3f77,
	0x17a2, 0x600e, 0x8976, 0x3f32,
	0x574e, 0x69f8, 0x4f3d, 0x3edd,
	0x11c8, 0xbbad, 0xca92, 0x3e75,
	0xee7a, 0x7d97, 0x78a4, 0x3df7,
};
static ushort AGD[40] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
	0x6b40, 0xf3d5, 0x9e2b, 0x4022,
	0x18d4, 0xc0ef, 0xd5d5, 0x4033,
	0xdc35, 0x7ea7, 0x211b, 0x402f,
	0xdbce, 0x2b79, 0xe84e, 0x4015,
	0xece3, 0xc195, 0x8992, 0x3fee,
	0xa9ee, 0xed64, 0x221d, 0x3fb6,
	0x93bb, 0x6be3, 0xe704, 0x3f70,
	0xa5a0, 0xd603, 0x8b61, 0x3f1a,
	0x24e8, 0xdb07, 0xa845, 0x3eb3,
	0x89d4, 0x3dd5, 0x1fc7, 0x3e35,
};
#endif
#ifdef MIEEE
static ushort AGN[44] = {
	0x3f94, 0x3525, 0xddcf, 0xbbde,
	0x3fd9, 0x07d5, 0x0064, 0x37b7,
	0x3ff1, 0x0d83, 0x3a20, 0x34eb,
	0x3fee, 0x0dac, 0xa0ef, 0x1acb,
	0x3fd6, 0x7e69, 0xcea8, 0xfe1d,
	0x3fb0, 0x3a41, 0x21e9, 0x0978,
	0x3f77, 0xfe99, 0xf12f, 0x5043,
	0x3f32, 0x8976, 0x600e, 0x17a2,
	0x3edd, 0x4f3d, 0x69f8, 0x574e,
	0x3e75, 0xca92, 0xbbad, 0x11c8,
	0x3df7, 0x78a4, 0x7d97, 0xee7a,
};
static ushort AGD[40] = {
/*0x3ff0,0x0000,0x0000,0x0000,*/
	0x4022, 0x9e2b, 0xf3d5, 0x6b40,
	0x4033, 0xd5d5, 0xc0ef, 0x18d4,
	0x402f, 0x211b, 0x7ea7, 0xdc35,
	0x4015, 0xe84e, 0x2b79, 0xdbce,
	0x3fee, 0x8992, 0xc195, 0xece3,
	0x3fb6, 0x221d, 0xed64, 0xa9ee,
	0x3f70, 0xe704, 0x6be3, 0x93bb,
	0x3f1a, 0x8b61, 0xd603, 0xa5a0,
	0x3eb3, 0xa845, 0xdb07, 0x24e8,
	0x3e35, 0x1fc7, 0x3dd5, 0x89d4,
};
#endif

#ifdef UNK
static double APFN[9] = {
	1.85365624022535566142E-1,
	8.86712188052584095637E-1,
	9.87391981747398547272E-1,
	4.01241082318003734092E-1,
	7.10304926289631174579E-2,
	5.90618657995661810071E-3,
	2.33051409401776799569E-4,
	4.08718778289035454598E-6,
	2.48379932900442457853E-8,
};
static double APFD[9] = {
/*  1.00000000000000000000E0,*/
	1.47345854687502542552E1,
	3.75423933435489594466E1,
	3.14657751203046424330E1,
	1.09969125207298778536E1,
	1.78885054766999417817E0,
	1.41733275753662636873E-1,
	5.44066067017226003627E-3,
	9.39421290654511171663E-5,
	5.65978713036027009243E-7,
};
#endif
#ifdef DEC
static ushort APFN[36] = {
	0037475, 0150174, 0071752, 0166651,
	0040142, 0177621, 0164246, 0101757,
	0040174, 0142670, 0106760, 0006573,
	0037715, 0067570, 0116274, 0022404,
	0037221, 0074157, 0053341, 0117207,
	0036301, 0104257, 0015075, 0004777,
	0035164, 0057502, 0164034, 0001313,
	0033611, 0022254, 0176000, 0112565,
	0031725, 0055523, 0025153, 0166057,
};
static ushort APFD[36] = {
/*0040200,0000000,0000000,0000000,*/
	0041153, 0140334, 0130506, 0061402,
	0041426, 0025551, 0024440, 0070611,
	0041373, 0134750, 0047147, 0176702,
	0041057, 0171532, 0105430, 0017674,
	0040344, 0174416, 0001726, 0047754,
	0037421, 0021207, 0020167, 0136264,
	0036262, 0043621, 0151321, 0124324,
	0034705, 0001313, 0163733, 0016407,
	0033027, 0166702, 0150440, 0170561,
};
#endif
#ifdef IBMPC
static ushort APFN[36] = {
	0x5db5, 0x8e7d, 0xba0f, 0x3fc7,
	0xd07e, 0x3d14, 0x5ff2, 0x3fec,
	0x01af, 0x11be, 0x98b7, 0x3fef,
	0x84a1, 0x1397, 0xadef, 0x3fd9,
	0x33d1, 0xeadc, 0x2f0d, 0x3fb2,
	0xa140, 0xe347, 0x3115, 0x3f78,
	0x8059, 0x5d03, 0x8be8, 0x3f2e,
	0x12af, 0x9f80, 0x2495, 0x3ed1,
	0x7d86, 0x654d, 0xab6a, 0x3e5a,
};
static ushort APFD[36] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
	0xcc60, 0x9628, 0x781b, 0x402d,
	0x0e31, 0x2524, 0xc56d, 0x4042,
	0xffb8, 0x09cc, 0x773d, 0x403f,
	0x03f7, 0x5163, 0xfe6b, 0x4025,
	0xc9fd, 0xc07a, 0x9f21, 0x3ffc,
	0xf796, 0xe40e, 0x2450, 0x3fc2,
	0x351a, 0x3a5a, 0x48f2, 0x3f76,
	0x63a1, 0x7cfb, 0xa059, 0x3f18,
	0x1e2e, 0x5a24, 0xfdb8, 0x3ea2,
};
#endif
#ifdef MIEEE
static ushort APFN[36] = {
	0x3fc7, 0xba0f, 0x8e7d, 0x5db5,
	0x3fec, 0x5ff2, 0x3d14, 0xd07e,
	0x3fef, 0x98b7, 0x11be, 0x01af,
	0x3fd9, 0xadef, 0x1397, 0x84a1,
	0x3fb2, 0x2f0d, 0xeadc, 0x33d1,
	0x3f78, 0x3115, 0xe347, 0xa140,
	0x3f2e, 0x8be8, 0x5d03, 0x8059,
	0x3ed1, 0x2495, 0x9f80, 0x12af,
	0x3e5a, 0xab6a, 0x654d, 0x7d86,
};
static ushort APFD[36] = {
/*0x3ff0,0x0000,0x0000,0x0000,*/
	0x402d, 0x781b, 0x9628, 0xcc60,
	0x4042, 0xc56d, 0x2524, 0x0e31,
	0x403f, 0x773d, 0x09cc, 0xffb8,
	0x4025, 0xfe6b, 0x5163, 0x03f7,
	0x3ffc, 0x9f21, 0xc07a, 0xc9fd,
	0x3fc2, 0x2450, 0xe40e, 0xf796,
	0x3f76, 0x48f2, 0x3a5a, 0x351a,
	0x3f18, 0xa059, 0x7cfb, 0x63a1,
	0x3ea2, 0xfdb8, 0x5a24, 0x1e2e,
};
#endif

#ifdef UNK
static double APGN[11] = {
	-3.55615429033082288335E-2,
	-6.37311518129435504426E-1,
	-1.70856738884312371053E0,
	-1.50221872117316635393E0,
	-5.63606665822102676611E-1,
	-1.02101031120216891789E-1,
	-9.48396695961445269093E-3,
	-4.60325307486780994357E-4,
	-1.14300836484517375919E-5,
	-1.33415518685547420648E-7,
	-5.63803833958893494476E-10,
};
static double APGD[11] = {
/*  1.00000000000000000000E0,*/
	9.85865801696130355144E0,
	2.16401867356585941885E1,
	1.73130776389749389525E1,
	6.17872175280828766327E0,
	1.08848694396321495475E0,
	9.95005543440888479402E-2,
	4.78468199683886610842E-3,
	1.18159633322838625562E-4,
	1.37480673554219441465E-6,
	5.79912514929147598821E-9,
};
#endif
#ifdef DEC
static ushort APGN[44] = {
	0137021, 0124372, 0176075, 0075331,
	0140043, 0023330, 0177672, 0161655,
	0140332, 0131126, 0010413, 0171112,
	0140300, 0044263, 0175560, 0054070,
	0140020, 0044206, 0142603, 0073324,
	0137321, 0015130, 0066144, 0144033,
	0136433, 0061243, 0175542, 0103373,
	0135361, 0053721, 0020441, 0053203,
	0134077, 0141725, 0160277, 0130612,
	0132417, 0040372, 0100363, 0060200,
	0130432, 0175052, 0171064, 0034147,
};
static ushort APGD[40] = {
/*0040200,0000000,0000000,0000000,*/
	0041035, 0136420, 0030124, 0140220,
	0041255, 0017432, 0034447, 0162256,
	0041212, 0100456, 0154544, 0006321,
	0040705, 0134026, 0127154, 0123414,
	0040213, 0051612, 0044470, 0172607,
	0037313, 0143362, 0053273, 0157051,
	0036234, 0144322, 0054536, 0007264,
	0034767, 0146170, 0054265, 0170342,
	0033270, 0102777, 0167362, 0073631,
	0031307, 0040644, 0167103, 0021763,
};
#endif
#ifdef IBMPC
static ushort APGN[44] = {
	0xaf5b, 0x5f87, 0x351f, 0xbfa2,
	0x5c76, 0x1ff7, 0x64db, 0xbfe4,
	0x7e49, 0xc221, 0x564a, 0xbffb,
	0x0b07, 0x7f6e, 0x0916, 0xbff8,
	0x6edb, 0xd8b0, 0x0910, 0xbfe2,
	0x9903, 0x0d8c, 0x234b, 0xbfba,
	0x50df, 0x7f6c, 0x6c54, 0xbf83,
	0x2ad0, 0x2424, 0x2afa, 0xbf3e,
	0xf631, 0xbc17, 0xf87a, 0xbee7,
	0x6c10, 0x501e, 0xe81f, 0xbe81,
	0x870d, 0x5e46, 0x5f45, 0xbe03,
};
static ushort APGD[40] = {
/*0x0000,0x0000,0x0000,0x3ff0,*/
	0x9812, 0x060a, 0xb7a2, 0x4023,
	0xfc96, 0x4724, 0xa3e3, 0x4035,
	0x819a, 0xdb2c, 0x5025, 0x4031,
	0x94e2, 0xd5cd, 0xb702, 0x4018,
	0x1eb1, 0x4927, 0x6a71, 0x3ff1,
	0x7bc5, 0x4ad7, 0x78de, 0x3fb9,
	0xc1d7, 0x4b2b, 0x991a, 0x3f73,
	0xbe1c, 0x0b16, 0xf98f, 0x3f1e,
	0x4ef3, 0xfdde, 0x10bf, 0x3eb7,
	0x647e, 0x9dc8, 0xe834, 0x3e38,
};
#endif
#ifdef MIEEE
static ushort APGN[44] = {
	0xbfa2, 0x351f, 0x5f87, 0xaf5b,
	0xbfe4, 0x64db, 0x1ff7, 0x5c76,
	0xbffb, 0x564a, 0xc221, 0x7e49,
	0xbff8, 0x0916, 0x7f6e, 0x0b07,
	0xbfe2, 0x0910, 0xd8b0, 0x6edb,
	0xbfba, 0x234b, 0x0d8c, 0x9903,
	0xbf83, 0x6c54, 0x7f6c, 0x50df,
	0xbf3e, 0x2afa, 0x2424, 0x2ad0,
	0xbee7, 0xf87a, 0xbc17, 0xf631,
	0xbe81, 0xe81f, 0x501e, 0x6c10,
	0xbe03, 0x5f45, 0x5e46, 0x870d,
};
static ushort APGD[40] = {
/*0x3ff0,0x0000,0x0000,0x0000,*/
	0x4023, 0xb7a2, 0x060a, 0x9812,
	0x4035, 0xa3e3, 0x4724, 0xfc96,
	0x4031, 0x5025, 0xdb2c, 0x819a,
	0x4018, 0xb702, 0xd5cd, 0x94e2,
	0x3ff1, 0x6a71, 0x4927, 0x1eb1,
	0x3fb9, 0x78de, 0x4ad7, 0x7bc5,
	0x3f73, 0x991a, 0x4b2b, 0xc1d7,
	0x3f1e, 0xf98f, 0x0b16, 0xbe1c,
	0x3eb7, 0x10bf, 0xfdde, 0x4ef3,
	0x3e38, 0xe834, 0x9dc8, 0x647e,
};
#endif

#ifdef ANSIPROT
extern double fabs(double);
extern double exp(double);
extern double sqrt(double);
extern double polevl(double, void *, int);
extern double p1evl(double, void *, int);
extern double sin(double);
extern double cos(double);
#else
double fabs(), exp(), sqrt();
double polevl(), p1evl(), sin(), cos();
#endif

int airy(double x, double * ai, double * aip, double * bi, double * bip)
{
	double z, zz, t, f, g, uf, ug, k, zeta, theta;
	int    domflg = 0;
	if(x > MAXAIRY) {
		*ai = 0;
		*aip = 0;
		*bi = MAXNUM;
		*bip = MAXNUM;
		return(-1);
	}
	if(x < -2.09) {
		domflg = 15;
		t = sqrt(-x);
		zeta = -2.0 * x * t / 3.0;
		t = sqrt(t);
		k = sqpii / t;
		z = 1.0/zeta;
		zz = z * z;
		uf = 1.0 + zz * polevl(zz, AFN, 8) / p1evl(zz, AFD, 9);
		ug = z * polevl(zz, AGN, 10) / p1evl(zz, AGD, 10);
		theta = zeta + 0.25 * SMathConst::Pi;
		f = sin(theta);
		g = cos(theta);
		*ai = k * (f * uf - g * ug);
		*bi = k * (g * uf + f * ug);
		uf = 1.0 + zz * polevl(zz, APFN, 8) / p1evl(zz, APFD, 9);
		ug = z * polevl(zz, APGN, 10) / p1evl(zz, APGD, 10);
		k = sqpii * t;
		*aip = -k * (g * uf + f * ug);
		*bip = k * (f * uf - g * ug);
		return(0);
	}
	if(x >= 2.09) { /* cbrt(9) */
		domflg = 5;
		t = sqrt(x);
		zeta = 2.0 * x * t / 3.0;
		g = exp(zeta);
		t = sqrt(t);
		k = 2.0 * t * g;
		z = 1.0/zeta;
		f = polevl(z, AN, 7) / polevl(z, AD, 7);
		*ai = sqpii * f / k;
		k = -0.5 * sqpii * t / g;
		f = polevl(z, APN, 7) / polevl(z, APD, 7);
		*aip = f * k;

		if(x > 8.3203353) { /* zeta > 16 */
			f = z * polevl(z, BN16, 4) / p1evl(z, BD16, 5);
			k = sqpii * g;
			*bi = k * (1.0 + f) / t;
			f = z * polevl(z, BPPN, 4) / p1evl(z, BPPD, 5);
			*bip = k * t * (1.0 + f);
			return(0);
		}
	}

	f = 1.0;
	g = x;
	t = 1.0;
	uf = 1.0;
	ug = x;
	k = 1.0;
	z = x * x * x;
	while(t > MACHEP) {
		uf *= z;
		k += 1.0;
		uf /= k;
		ug *= z;
		k += 1.0;
		ug /= k;
		uf /= k;
		f += uf;
		k += 1.0;
		ug /= k;
		g += ug;
		t = fabs(uf/f);
	}
	uf = c1 * f;
	ug = c2 * g;
	if( (domflg & 1) == 0)
		*ai = uf - ug;
	if( (domflg & 2) == 0)
		*bi = sqrt3 * (uf + ug);

	/* the deriviative of ai */
	k = 4.0;
	uf = x * x/2.0;
	ug = z/3.0;
	f = uf;
	g = 1.0 + ug;
	uf /= 3.0;
	t = 1.0;

	while(t > MACHEP) {
		uf *= z;
		ug /= k;
		k += 1.0;
		ug *= z;
		uf /= k;
		f += uf;
		k += 1.0;
		ug /= k;
		uf /= k;
		g += ug;
		k += 1.0;
		t = fabs(ug/g);
	}

	uf = c1 * f;
	ug = c2 * g;
	if((domflg & 4) == 0)
		*aip = uf - ug;
	if((domflg & 8) == 0)
		*bip = sqrt3 * (uf + ug);
	return(0);
}

void GpEval::f_airy(GpArgument * pArg)
{
	t_value a;
	double ai, aip, bi, bip;
	double x = PopOrConvertFromString(a).Real();
	airy(x, &ai, &aip, &bi, &bip);
	Push(a.SetComplex(ai, 0.0));
}

#endif  /* End choice of low- or high-precision airy function */

/* ** expint.c
 *
 *   DESCRIBE  Approximate the exponential integral function
 *
 *
 *                       /inf   -n    -zt
 *             E_n(z) =  |     t   * e    dt (n = 0, 1, 2, ...)
 *                       /1
 *
 *
 *   CALL      p = expint(n, z)
 *
 *             double    n    >= 0
 *             double    z    >= 0
 *               also: n must be an integer
 *                     either z > 0 or n > 1
 *
 *   WARNING   none
 *
 *   RETURN    double    p    > 0
 *                            -1.0 on error condition
 *
 *   REFERENCE Abramowitz and Stegun (1964)
 *
 * Copyright (c) 2010 James R. Van Zandt, jrvz@comcast.net
 */

static double expint(double n, double z)
{
	double y; /* the answer */
	{
		/* Test for admissibility of arguments */
		double junk;
		if(n < 0 || z < 0 || modf(n, &junk))
			return -1.0;
		if(z == 0 && n < 2)
			return -1.0;
	}
	// special cases 
	if(n == 0) 
		return exp(-z)/z;
	if(z == 0) 
		return 1/(n-1);
	// for z=3, CF requires 36 terms and series requires 29 
	if(z > 3) { 
		//
		// For large z, use continued fraction (Abramowitz & Stegun 5.1.22):
		// E_n(z) = exp(-z)(1/(z+n/(1+1/(z+(n+1)/1+2/(z+...)))))
		// The CF is valid and stable for z>0, and efficient for z>1 or so.
		//
		double n2, n3, d2, d3, y_prev = 1;
		double n0 = 0; 
		double n1 = 1;
		double d0 = 1; 
		double d1 = z;
		for(int i = 0; i < 333; i++) { 
			// evaluate the CF "top down" using the recurrence
			// relations for the numerators and denominators of
			// successive convergents 
			n2 = n0*(n+i) + n1;
			d2 = d0*(n+i) + d1;
			n3 = n1*(1+i) + n2*z;
			d3 = d1*(1+i) + d2*z;
			y = n3/d3;
			if(y == y_prev) break;
			y_prev = y;
			n0 = n2; n1 = n3;
			d0 = d2; d1 = d3;
			// Re-scale terms in continued fraction if terms are large 
			if(d3 >= OFLOW) {
				n0 /= OFLOW;
				n1 /= OFLOW;
				d0 /= OFLOW;
				d1 /= OFLOW;
			}
		}
		y = exp(-z)*y;
	}
	else { 
		// For small z, use series (Abramowitz & Stegun 5.1.12):
		// E_1(z) = -\gamma + \ln z + \sum_{m=1}^\infty { (-z)^m \over (m) m! }
		// The series is valid for z>0, and efficient for z<4 or so.  
		//
		// from Abramowitz & Stegun, Table 1.1 
		//
		double euler_constant = .577215664901532860606512;

		double y_prev = 0;
		double t, m;

		y = -euler_constant - log(z);
		t = 1;
		for(m = 1; m<333; m++) {
			t = -t*z/m;
			y = y - t/m;
			if(y == y_prev) break;
			y_prev = y;
		}

		/* For n > 1, use recurrence relation (Abramowitz & Stegun 5.1.14):
		   n E_{n+1}(z) + z E_n(z) = e^{-z}, n >= 1
		   The recurrence is unstable for increasing n and z>4 or so,
		   but okay for z<3.  */

		for(m = 1; m<n; m++)
			y = (exp(-z) - z*y)/m;
	}
	return y;
}

void GpEval::f_expint(GpArgument * pArg)
{
	t_value a;
	double n, x;
	x = PopOrConvertFromString(a).Real();
	n = PopOrConvertFromString(a).Real();
	x = expint(n, x);
	if(x <= -1)
		undefined = true; // Error return from expint --> flag 'undefined'
	Push(a.SetComplex(x, 0.0));
}
//
// INTERNAL
//
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
	return (GpGg.Ev.undefined = true); /* don't print error message */
}

#endif

static enum DATA_TYPES sprintf_specifier(const char* format);

#define BADINT_DEFAULT R_Gg.IntErrorNoCaret("error: bit shift applied to non-INT");
#define BAD_TYPE(type) R_Gg.IntErrorNoCaret((type==NOTDEFINED) ? "uninitialized user variable" : "internal error : type neither INT nor CMPLX");

static int recursion_depth = 0;
void eval_reset_after_error()
{
	recursion_depth = 0;
}

void GpEval::f_push(GpArgument * x)
{
	UdvtEntry * udv = x->udv_arg;
	if(udv->udv_value.type == NOTDEFINED) {
		if(GpGg.Gp__C.P.IsStringResultOnly)
			udv = udv_NaN; // We're only here to check whether this is a string. It isn't
		else
			R_Gg.IntErrorNoCaret("undefined variable: %s", udv->udv_name);
	}
	Push(&(udv->udv_value));
}

void GpEval::f_pushc(GpArgument * x)
{
	Push(&(x->v_arg));
}

void GpEval::f_pushd1(GpArgument * x)
{
	Push(&(x->udf_arg->dummy_values[0]));
}

void GpEval::f_pop(GpArgument * x)
{
	t_value dummy;
	Pop(dummy);
	if(dummy.type == STRING)
		gpfree_string(&dummy);
#ifdef ARRAY_COPY_ON_REFERENCE
	if(dummy.type == ARRAY)
		gpfree_string(&dummy);
#endif
}

void GpEval::f_pushd2(GpArgument * x)
{
	Push(&(x->udf_arg->dummy_values[1]));
}

void GpEval::f_pushd(GpArgument * x)
{
	t_value param;
	Pop(param);
	Push(&(x->udf_arg->dummy_values[param.v.int_val]));
}
//
// execute a udf 
//
void GpEval::f_call(GpArgument * x)
{
	t_value save_dummy;
	UdftEntry * udf = x->udf_arg;
	if(!udf->at) {
		if(GpGg.Gp__C.P.IsStringResultOnly) {
			// We're only here to check whether this is a string. It isn't
			f_pop(x);
			Push(&udv_NaN->udv_value);
			return;
		}
		R_Gg.IntErrorNoCaret("undefined function: %s", udf->udf_name);
	}
	save_dummy = udf->dummy_values[0];
	Pop(udf->dummy_values[0]);
	if(udf->dummy_values[0].type == ARRAY)
		R_Gg.IntErrorNoCaret("f_call: unsupported array operation");
	if(udf->dummy_num != 1)
		R_Gg.IntErrorNoCaret("function %s requires %d variables", udf->udf_name, udf->dummy_num);
	if(recursion_depth++ > STACK_DEPTH)
		R_Gg.IntErrorNoCaret("recursion depth limit exceeded");
	ExecuteAt(udf->at);
	gpfree_string(&udf->dummy_values[0]);
	udf->dummy_values[0] = save_dummy;
	recursion_depth--;
}
//
// execute a udf of n variables
//
void GpEval::f_calln(GpArgument * x)
{
	t_value save_dummy[MAX_NUM_VAR];
	int i;
	int num_pop;
	t_value num_params;
	UdftEntry * udf = x->udf_arg;
	if(!udf->at)            /* undefined */
		R_Gg.IntErrorNoCaret("undefined function: %s", udf->udf_name);
	for(i = 0; i < MAX_NUM_VAR; i++)
		save_dummy[i] = udf->dummy_values[i];
	Pop(num_params);
	if(num_params.v.int_val != udf->dummy_num)
		R_Gg.IntErrorNoCaret("function %s requires %d variable%c", udf->udf_name, udf->dummy_num, (udf->dummy_num == 1) ? '\0' : 's');
	/* if there are more parameters than the function is expecting */
	/* simply ignore the excess */
	if(num_params.v.int_val > MAX_NUM_VAR) {
		/* pop and discard the dummies that there is no room for */
		num_pop = num_params.v.int_val - MAX_NUM_VAR;
		for(i = 0; i < num_pop; i++)
			Pop((udf->dummy_values[0]));
		num_pop = MAX_NUM_VAR;
	}
	else {
		num_pop = num_params.v.int_val;
	}
	/* pop parameters we can use */
	for(i = num_pop - 1; i >= 0; i--) {
		Pop((udf->dummy_values[i]));
		if(udf->dummy_values[i].type == ARRAY)
			R_Gg.IntErrorNoCaret("f_calln: unsupported array operation");
	}
	if(recursion_depth++ > STACK_DEPTH)
		R_Gg.IntErrorNoCaret("recursion depth limit exceeded");
	ExecuteAt(udf->at);
	recursion_depth--;
	for(i = 0; i < MAX_NUM_VAR; i++) {
		gpfree_string(&udf->dummy_values[i]);
		udf->dummy_values[i] = save_dummy[i];
	}
}

void GpEval::f_sum(GpArgument * pArg)
{
	t_value beg, end, varname; /* [<var> = <start>:<end>] */
	UdftEntry * udf;           /* function to evaluate */
	UdvtEntry * udv;           /* iteration variable */
	t_value ret;           /* result */
	t_value z;
	int i;
	Pop(end);
	Pop(beg);
	Pop(varname);
	if(beg.type != INTGR || end.type != INTGR)
		R_Gg.IntErrorNoCaret("range specifiers of sum must have integer values");
	if(varname.type != STRING)
		R_Gg.IntErrorNoCaret("internal error: f_sum expects argument (varname) of type string.");
	udv = GetUdvByName(varname.v.string_val);
	if(!udv)
		R_Gg.IntErrorNoCaret("internal error: f_sum could not access iteration variable.");
	udf = pArg->udf_arg;
	if(!udf)
		R_Gg.IntErrorNoCaret("internal error: f_sum could not access summation coefficient function");
	ret.SetComplex(0, 0);
	for(i = beg.v.int_val; i<=end.v.int_val; ++i) {
		// calculate f_i = f() with user defined variable i 
		udv->udv_value.SetInt(i);
		ExecuteAt(udf->at);
		Pop(z);
		ret.SetComplex(ret.Real() + z.Real(), imag(&ret) + imag(&z));
	}
	gpfree_string(&varname);
	Push(z.SetComplex(ret.Real(), imag(&ret)));
}

void GpEval::f_lnot(GpArgument * pArg)
{
	t_value a;
	int_check(Pop(a));
	Push(a.SetInt(!a.v.int_val));
}

void GpEval::f_bnot(GpArgument * pArg)
{
	t_value a;
	int_check(Pop(a));
	Push(a.SetInt(~a.v.int_val));
}

void GpEval::f_lor(GpArgument * pArg)
{
	t_value a, b;
	int_check(Pop(b));
	int_check(Pop(a));
	Push(a.SetInt(a.v.int_val || b.v.int_val));
}

void GpEval::f_land(GpArgument * pArg)
{
	t_value a, b;
	int_check(Pop(b));
	int_check(Pop(a));
	Push(a.SetInt(a.v.int_val && b.v.int_val));
}

void GpEval::f_bor(GpArgument * pArg)
{
	t_value a, b;
	int_check(Pop(b));
	int_check(Pop(a));
	Push(a.SetInt(a.v.int_val | b.v.int_val));
}

void GpEval::f_xor(GpArgument * pArg)
{
	t_value a, b;
	int_check(Pop(b));
	int_check(Pop(a));
	Push(a.SetInt(a.v.int_val ^ b.v.int_val));
}

void GpEval::f_band(GpArgument * pArg)
{
	t_value a, b;
	int_check(Pop(b));
	int_check(Pop(a));
	Push(a.SetInt(a.v.int_val & b.v.int_val));
}
//
// Make all the following internal routines perform autoconversion
// from string to numeric value.
//
//#define pop__(x) pop_or_convert_from_string(x)

void GpEval::f_uminus(GpArgument * pArg)
{
	t_value a;
	PopOrConvertFromString(a);
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
	Push(&a);
}

void GpEval::f_eq(GpArgument * pArg)
{
	/* note: floating point equality is rare because of roundoff error! */
	t_value a, b;
	int result = 0;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(a.SetInt(result));
}

void GpEval::f_ne(GpArgument * pArg)
{
	t_value a, b;
	int result = 0;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(a.SetInt(result));
}

void GpEval::f_gt(GpArgument * pArg)
{
	t_value a, b;
	int result = 0;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(a.SetInt(result));
}

void GpEval::f_lt(GpArgument * pArg)
{
	t_value a, b;
	int result = 0;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(a.SetInt(result));
}

void GpEval::f_ge(GpArgument * pArg)
{
	t_value a, b;
	int result = 0;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(a.SetInt(result));
}

void GpEval::f_le(GpArgument * pArg)
{
	t_value a, b;
	int result = 0;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(a.SetInt(result));
}

void GpEval::f_leftshift(GpArgument * pArg)
{
	t_value a, b, result;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(&result);
}

void GpEval::f_rightshift(GpArgument * pArg)
{
	t_value a, b, result;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(&result);
}

void GpEval::f_plus(GpArgument * pArg)
{
	t_value a, b, result;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);
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
	Push(&result);
}

void GpEval::f_minus(GpArgument * pArg)
{
	t_value a, b, result;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);          /* now do a - b */
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
	Push(&result);
}

void GpEval::f_mult(GpArgument * pArg)
{
	t_value a, b, result;
	double product;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);          /* now do a*b */
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
	Push(&result);
}

void GpEval::f_div(GpArgument * pArg)
{
	t_value a, b, result;
	double square;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);          /* now do a/b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				if(b.v.int_val)
					result.SetInt(a.v.int_val / b.v.int_val);
				else {
					result.SetInt(0);
					undefined = true;
				}
				break;
			    case CMPLX:
				square = b.v.cmplx_val.real * b.v.cmplx_val.real + b.v.cmplx_val.imag * b.v.cmplx_val.imag;
				if(square)
					result.SetComplex(a.v.int_val * b.v.cmplx_val.real / square, -a.v.int_val * b.v.cmplx_val.imag / square);
				else {
					result.SetComplex(0.0, 0.0);
					undefined = true;
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
					undefined = true;
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
					undefined = true;
				}
				break;
			    default:
				BAD_TYPE(b.type)
		    }
		    break;
		default:
		    BAD_TYPE(a.type)
	}
	Push(&result);
}

void GpEval::f_mod(GpArgument * pArg)
{
	t_value a, b;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);          /* now do a%b */
	if(a.type != INTGR || b.type != INTGR)
		R_Gg.IntErrorNoCaret("non-integer operand for %%");
	if(b.v.int_val)
		Push(a.SetInt(a.v.int_val % b.v.int_val));
	else {
		Push(a.SetInt(0));
		undefined = true;
	}
}

void GpEval::f_power(GpArgument * pArg)
{
	t_value a, b, result;
	int i, t;
	double mag, ang;
	PopOrConvertFromString(b);
	PopOrConvertFromString(a);          /* now find a**b */
	switch(a.type) {
		case INTGR:
		    switch(b.type) {
			    case INTGR:
				if(a.v.int_val == 0) {
					if(b.v.int_val < 0)
						undefined = true;
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
						undefined = true;
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
							undefined = true;
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
							undefined = true;
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
							undefined = true;
					}
					ang = angle(&a) * b.v.int_val;
					result.SetComplex(mag * cos(ang),
				    mag * sin(ang));
				}
				break;
			    case CMPLX:
				if(a.v.cmplx_val.real == 0 && a.v.cmplx_val.imag == 0) {
					if(b.v.cmplx_val.imag != 0 || b.v.cmplx_val.real < 0) {
						undefined = true;
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
							undefined = true;
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
	Push(&result);
}

void GpEval::f_factorial(GpArgument * pArg)
{
	t_value a;
	int i;
	double val = 0.0;
	PopOrConvertFromString(a);          /* find a! (factorial) */
	switch(a.type) {
		case INTGR:
		    val = 1.0;
		    for(i = a.v.int_val; i > 1; i--) /*fpe's should catch overflows */
			    val *= i;
		    break;
		default:
		    R_Gg.IntErrorNoCaret("factorial (!) argument must be an integer");
		    return;     /* avoid gcc -Wall warning about val */
	}
	Push(a.SetComplex(val, 0.0));
}
/*
 * Terminate the autoconversion from string to numeric values
 */
#undef pop__

void GpEval::f_concatenate(GpArgument * pArg)
{
	t_value a, b, result;
	Pop(b);
	Pop(a);
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
		R_Gg.IntErrorNoCaret("internal error : STRING operator applied to non-STRING type");

	(void)Gstring(&result, gp_stradd(a.v.string_val, b.v.string_val));
	gpfree_string(&a);
	gpfree_string(&b);
	Push(&result);
	gpfree_string(&result); /* free string allocated within gp_stradd() */
}

void GpEval::f_eqs(GpArgument * pArg)
{
	t_value a, b, result;
	Pop(b);
	Pop(a);
	if(a.type != STRING || b.type != STRING)
		R_Gg.IntErrorNoCaret("internal error : STRING operator applied to non-STRING type");
	result.SetInt(!strcmp(a.v.string_val, b.v.string_val));
	gpfree_string(&a);
	gpfree_string(&b);
	Push(&result);
}

void GpEval::f_nes(GpArgument * pArg)
{
	t_value a, b, result;
	Pop(b);
	Pop(a);
	if(a.type != STRING || b.type != STRING)
		R_Gg.IntErrorNoCaret("internal error : STRING operator applied to non-STRING type");
	result.SetInt((int)(strcmp(a.v.string_val, b.v.string_val)!=0));
	gpfree_string(&a);
	gpfree_string(&b);
	Push(&result);
}

void GpEval::f_strlen(GpArgument * pArg)
{
	t_value a, result;
	Pop(a);
	if(a.type != STRING)
		R_Gg.IntErrorNoCaret("internal error : strlen of non-STRING argument");
	result.SetInt((int)gp_strlen(a.v.string_val));
	gpfree_string(&a);
	Push(&result);
}

void GpEval::f_strstrt(GpArgument * pArg)
{
	t_value needle, haystack, result;
	char * start;
	Pop(needle);
	Pop(haystack);
	if(needle.type != STRING || haystack.type != STRING)
		R_Gg.IntErrorNoCaret("internal error : non-STRING argument to strstrt");
	start = strstr(haystack.v.string_val, needle.v.string_val);
	result.SetInt((int)(start ? (start-haystack.v.string_val)+1 : 0));
	gpfree_string(&needle);
	gpfree_string(&haystack);
	Push(&result);
}
/*
 * f_range() handles both explicit calls to substr(string, beg, end)
 * and the short form string[beg:end].  The calls to gp_strlen() and
 * gp_strchrn() allow it to handle utf8 strings.
 */
void GpEval::f_range(GpArgument * pArg)
{
	t_value beg, end, full;
	t_value substr;
	int ibeg, iend;
	Pop(end);
	Pop(beg);
	Pop(full);
	if(beg.type == INTGR)
		ibeg = beg.v.int_val;
	else if(beg.type == CMPLX)
		ibeg = (int)floor(beg.v.cmplx_val.real);
	else
		R_Gg.IntErrorNoCaret("internal error: non-numeric substring range specifier");
	if(end.type == INTGR)
		iend = end.v.int_val;
	else if(end.type == CMPLX)
		iend = (int)floor(end.v.cmplx_val.real);
	else
		R_Gg.IntErrorNoCaret("internal error: non-numeric substring range specifier");
	if(full.type != STRING)
		R_Gg.IntErrorNoCaret("internal error: substring range operator applied to non-STRING type");
	FPRINTF((stderr, "f_range( \"%s\", %d, %d)\n", full.v.string_val, beg.v.int_val, end.v.int_val));
	if(iend > (int)gp_strlen(full.v.string_val))
		iend = gp_strlen(full.v.string_val);
	if(ibeg < 1)
		ibeg = 1;
	if(ibeg > iend) {
		Push(Gstring(&substr, ""));
	}
	else {
		char * begp = gp_strchrn(full.v.string_val, ibeg-1);
		char * endp = gp_strchrn(full.v.string_val, iend);
		*endp = '\0';
		Push(Gstring(&substr, begp));
	}
	gpfree_string(&full);
}

/*
 * f_index() extracts the value of a single element from an array.
 */
void GpEval::f_index(GpArgument * pArg)
{
	t_value array, index;
	int i = -1;
	Pop(index);
	Pop(array);
	if(array.type != ARRAY)
		R_Gg.IntErrorNoCaret("internal error: attempt to index non-array variable");
	if(index.type == INTGR)
		i = index.v.int_val;
	else if(index.type == CMPLX)
		i = (int)floor(index.v.cmplx_val.real);
	if(i <= 0 || i > array.v.value_array[0].v.int_val)
		R_Gg.IntErrorNoCaret("array index out of range");
	Push(&array.v.value_array[i]);
}

/* Magic number! */
#define RETURN_WORD_COUNT (-17*23*61)

void GpEval::f_words(GpArgument * pArg)
{
	t_value a;
	// "words(s)" is implemented as "word(s,RETURN_WORD_COUNT)" 
	Push(a.SetInt(RETURN_WORD_COUNT));
	f_word(pArg);
}

void GpEval::f_word(GpArgument * pArg)
{
	t_value a, b, result;
	int nwords = 0;
	int in_string = 0;
	int ntarget;
	char q;
	char * s;
	if(Pop(b).type != INTGR)
		R_Gg.IntErrorNoCaret("internal error : non-INTGR argument");
	ntarget = b.v.int_val;

	if(Pop(a).type != STRING)
		R_Gg.IntErrorNoCaret("internal error : non-STRING argument");
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

	Push(&result);
	gpfree_string(&a);
}

#undef RETURN_WORD_COUNT

/* EAM July 2004  (revised to dynamic buffer July 2005)
 * There are probably an infinite number of things that can
 * go wrong if the user mis-matches arguments and format strings
 * in the call to sprintf, but I hope none will do worse than
 * result in a garbage output string.
 */
void GpEval::f_sprintf(GpArgument * pArg)
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
	// Retrieve number of parameters from top of stack 
	Pop(num_params);
	nargs = num_params.v.int_val;
	if(nargs > 10) { /* Fall back to slow but sure allocation */
		args = (t_value *)malloc(sizeof(t_value)*nargs);
	}
	else
		args = a;
	for(i = 0; i<nargs; i++)
		Pop(args[i]);  /* pop next argument */
	// Make sure we got a format string of some sort
	if(args[nargs-1].type != STRING)
		R_Gg.IntErrorNoCaret("First parameter to sprintf must be a format string");

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
			R_Gg.IntErrorNoCaret("f_sprintf: attempt to print numeric value with string format");
		if(spec_type != STRING && next_param->type == STRING)
			R_Gg.IntErrorNoCaret("f_sprintf: attempt to print string value with numeric format");
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
			    R_Gg.IntErrorNoCaret("internal error: invalid spec_type");
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
			    R_Gg.IntErrorNoCaret("internal error: invalid spec_type");
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
	Push(Gstring(&result, buffer));
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
void GpEval::f_gprintf(GpArgument * pArg)
{
	t_value fmt, val, result;
	char * buffer;
	int length;
	double base = 10.;
	// Retrieve parameters from top of stack 
	Pop(val);
	Pop(fmt);
	// Make sure parameters are of the correct type 
	if(fmt.type != STRING)
		R_Gg.IntErrorNoCaret("First parameter to gprintf must be a format string");
	// Make sure we have at least as much space in the output as the format itself 
	length = 80 + strlen(fmt.v.string_val);
	buffer = (char *)malloc(length);
	// Call the old internal routine 
	gprintf(buffer, length, fmt.v.string_val, base, val.Real());
	FPRINTF((stderr, " gprintf result = \"%s\"\n", buffer));
	Push(Gstring(&result, buffer));
	gpfree_string(&fmt);
	free(buffer);
}

/* Output time given in seconds from year 2000 into string */
void GpEval::f_strftime(GpArgument * pArg)
{
	t_value fmt, val;
	char * fmtstr, * buffer;
	int fmtlen, buflen, length;
	/* Retrieve parameters from top of stack */
	Pop(val);
	Pop(fmt);
	if(fmt.type != STRING)
		R_Gg.IntErrorNoCaret("First parameter to strftime must be a format string");

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
		R_Gg.IntErrorNoCaret("Resulting string is too long");

	/* Remove trailing space */
	assert(buffer[length-1] == ' ');
	buffer[length-1] = NUL;

	gpfree_string(&val);
	gpfree_string(&fmt);
	free(fmtstr);

	Push(Gstring(&val, buffer));
	free(buffer);
}

/* Convert string into seconds from year 2000 */
void GpEval::f_strptime(GpArgument * pArg)
{
	t_value fmt, val;
	struct tm time_tm;
	double usec = 0.0;
	double result;
	Pop(val);
	Pop(fmt);
	if(fmt.type != STRING || val.type != STRING)
		R_Gg.IntErrorNoCaret("Both parameters to strptime must be strings");
	if(!fmt.v.string_val || !val.v.string_val)
		R_Gg.IntErrorNoCaret("Internal error: string not allocated");
	/* string -> time_tm  plus extra fractional second */
	gstrptime(val.v.string_val, fmt.v.string_val, &time_tm, &usec);
	/* time_tm -> result */
	result = gtimegm(&time_tm);
	FPRINTF((stderr, " strptime result = %g seconds \n", result));
	/* Add back any extra fractional second */
	result += usec;
	gpfree_string(&val);
	gpfree_string(&fmt);
	Push(val.SetComplex(result, 0.0));
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
void GpEval::f_time(GpArgument * pArg)
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
	Pop(val);
	switch(val.type) {
		case INTGR:
		    Push(val.SetInt((int)time_now));
		    break;
		case CMPLX:
		    Push(val.SetComplex(time_now, 0.0));
		    break;
		case STRING:
		    Push(&val); /* format string */
		    Push(val2.SetComplex(time_now, 0.0));
		    f_strftime(pArg);
		    break;
		default:
		    R_Gg.IntErrorNoCaret("internal error: invalid argument type");
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
		GpGg.IntErrorNoCaret("internal error: sprintf_specifier called without '%'\n");
	string_pos  = strcspn(format, string_spec);
	real_pos    = strcspn(format, real_spec);
	int_pos     = strcspn(format, int_spec);
	illegal_pos = strcspn(format, illegal_spec);
	if(illegal_pos < int_pos && illegal_pos < real_pos && illegal_pos < string_pos)
		GpGg.IntErrorNoCaret("sprintf_specifier: used with invalid format specifier\n");
	else if(string_pos < real_pos && string_pos < int_pos)
		return STRING;
	else if(real_pos < int_pos)
		return CMPLX;
	else if(int_pos < (int)strlen(format) )
		return INTGR;
	else
		GpGg.IntErrorNoCaret("sprintf_specifier: no format specifier\n");
	return INTGR; /* Can't happen, but the compiler doesn't realize that */
}
//
// execute a system call and return stream from STDOUT 
//
void GpEval::f_system(GpArgument * pArg)
{
	t_value val, result;
	char * output;
	int output_len, ierr;
	// Retrieve parameters from top of stack 
	Pop(val);
	// Make sure parameters are of the correct type 
	if(val.type != STRING)
		R_Gg.IntErrorNoCaret("non-string argument to system()");
	FPRINTF((stderr, " f_system input = \"%s\"\n", val.v.string_val));
	ierr = do_system_func(val.v.string_val, &output);
	FillGpValInteger("GPVAL_ERRNO", ierr);
	output_len = strlen(output);
	/* chomp result */
	if(output_len > 0 && output[output_len-1] == '\n')
		output[output_len-1] = NUL;
	FPRINTF((stderr, " f_system result = \"%s\"\n", output));
	Push(Gstring(&result, output));
	gpfree_string(&result); /* free output */
	gpfree_string(&val); /* free command string */
}
//
// Variable assignment operator 
//
void GpEval::f_assign(GpArgument * pArg)
{
	UdvtEntry * udv;
	t_value a, b, index;
	Pop(b);  /* new value */
	Pop(index); /* index (only used if this is an array assignment) */
	Pop(a);  /* name of variable */

	if(a.type != STRING)
		R_Gg.IntErrorNoCaret("attempt to assign to something other than a named variable");
	if(!strncmp(a.v.string_val, "GPVAL_", 6) || !strncmp(a.v.string_val, "MOUSE_", 6))
		R_Gg.IntErrorNoCaret("attempt to assign to a read-only variable");
	if(b.type == ARRAY)
		R_Gg.IntErrorNoCaret("unsupported array operation");
	udv = AddUdvByName(a.v.string_val);
	gpfree_string(&a);
	if(udv->udv_value.type == ARRAY) {
		int i;
		if(index.type == INTGR)
			i = index.v.int_val;
		else if(index.type == CMPLX)
			i = (int)floor(index.v.cmplx_val.real);
		else
			R_Gg.IntErrorNoCaret("non-numeric array index");
		if(i <= 0 || i > udv->udv_value.v.value_array[0].v.int_val)
			R_Gg.IntErrorNoCaret("array index out of range");
		gpfree_string(&udv->udv_value.v.value_array[i]);
		udv->udv_value.v.value_array[i] = b;
	}
	else {
		gpfree_string(&(udv->udv_value));
		udv->udv_value = b;
	}
	Push(&b);
}
/*
 * Retrieve the current value of a user-defined variable whose name is known.
 * B = value("A") has the same result as B = A.
 */
void GpEval::f_value(GpArgument * pArg)
{
	UdvtEntry * p = first_udv;
	t_value a;
	t_value result;
	Pop(a);
	if(a.type != STRING) {
		// GpGg.IntWarn(NO_CARET,"non-string value passed to value()");
		Push(&a);
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
			// GpGg.IntWarn(NO_CARET,"undefined variable name passed to value()");
			result.type = CMPLX;
			result.v.cmplx_val.real = not_a_number();
		}
		Push(&result);
	}
}
//
// User-callable builtin color conversion
//
void GpEval::f_hsv2rgb(GpArgument * pArg)
{
	t_value h, s, v, result;
	rgb_color color = {0., 0., 0.};
	Pop(v);
	Pop(s);
	Pop(h);
	if(h.type == INTGR)
		color.R = h.v.int_val;
	else if(h.type == CMPLX)
		color.R = h.v.cmplx_val.real;
	if(s.type == INTGR)
		color.G = s.v.int_val;
	else if(s.type == CMPLX)
		color.G = s.v.cmplx_val.real;
	if(v.type == INTGR)
		color.B = v.v.int_val;
	else if(v.type == CMPLX)
		color.B = v.v.cmplx_val.real;
	//SETMAX(color.r, 0);
	//SETMAX(color.g, 0);
	//SETMAX(color.b, 0);
	//SETMIN(color.r, 1.0);
	//SETMIN(color.g, 1.0);
	//SETMIN(color.b, 1.0);
	color.Constrain();
	result.SetInt(hsv2rgb(&color));
	Push(&result);
}
//
// stuff for implementing the call-backs for picking up data values
// do it here so we can make the variables private to this file
//
void GpEval::f_dollars(GpArgument * x)
{
	int column = x->v_arg.v.int_val;
	t_value a;
	if(column == -3) // pseudocolumn -3 means "last column" 
		column = GpDf.df_no_cols;
	if(column == 0) {
		Push(a.SetComplex((double)GpDf.df_datum, 0.0)); /* $0 */
	}
	else if(column > GpDf.df_no_cols || GpDf.df_column[column-1].good != DF_GOOD) {
		undefined = true;
		// Nov 2014: This is needed in case the value is referenced 
		// in an expression inside a 'using' clause.		    
		Push(a.SetComplex(not_a_number(), 0.0));
	}
	else
		Push(a.SetComplex(GpDf.df_column[column-1].datum, 0.0));
}

void GpEval::f_column(GpArgument * pArg)
{
	t_value a;
	int column;
	Pop(a);
	if(!GpDf.evaluate_inside_using)
		R_Gg.IntError(GpGg.Gp__C.CToken-1, "column() called from invalid context");
	if(a.type == STRING) {
		int j;
		char * name = a.v.string_val;
		column = DF_COLUMN_HEADERS;
		for(j = 0; j < GpDf.df_no_cols; j++) {
			if(GpDf.df_column[j].header) {
				int offset = (*GpDf.df_column[j].header == '"') ? 1 : 0;
				if(streq(name, GpDf.df_column[j].header + offset)) {
					column = j+1;
					SETIFZ(GpDf.df_key_title, gp_strdup(GpDf.df_column[j].header));
					break;
				}
			}
		}
		/* This warning should only trigger once per problematic input file */
		if(column == DF_COLUMN_HEADERS && (*name) && GpDf.df_warn_on_missing_columnheader) {
			GpDf.df_warn_on_missing_columnheader = false;
			GpGg.IntWarn(NO_CARET, "no column with header \"%s\"", a.v.string_val);
			for(j = 0; j < GpDf.df_no_cols; j++) {
				if(GpDf.df_column[j].header) {
					int offset = (*GpDf.df_column[j].header == '"') ? 1 : 0;
					if(!strncmp(name, GpDf.df_column[j].header + offset, strlen(name)))
						GpGg.IntWarn(NO_CARET, "partial match against column %d header \"%s\"", j+1, GpDf.df_column[j].header);
				}
			}
		}
		gpfree_string(&a);
	}
	else
		column = (int)a.Real();
	if(column == -2)
		Push(a.SetInt(GpDf.df_current_index));
	else if(column == -1)
		Push(a.SetInt(GpDf.line_count));
	else if(column == 0)    /* $0 = df_datum */
		Push(a.SetComplex((double)GpDf.df_datum, 0.0));
	else if(column == -3)   /* pseudocolumn -3 means "last column" */
		Push(a.SetComplex(GpDf.df_column[GpDf.df_no_cols - 1].datum, 0.0));
	else if(column < 1 || column > GpDf.df_no_cols || GpDf.df_column[column - 1].good != DF_GOOD) {
		undefined = true;
		/* Nov 2014: This is needed in case the value is referenced */
		/* in an expression inside a 'using' clause.		    */
		Push(a.SetComplex(not_a_number(), 0.0));
	}
	else
		Push(a.SetComplex(GpDf.df_column[column - 1].datum, 0.0));
}
//
//
//
/* Internal operators of the stack-machine, not directly represented
 * by any user-visible operator, or using private status variables
 * directly */

/* converts top-of-stack to boolean */
void GpEval::f_bool(GpArgument * x)
{
	(void)x;                
	int_check(TopOfStack());
	TopOfStack().v.int_val = !!TopOfStack().v.int_val;
}

void GpEval::f_jump(GpArgument * x)
{
	(void)x;                
	JumpOffset = x->j_arg;
}

void GpEval::f_jumpz(GpArgument * x)
{
	t_value a;
	(void)x;                
	int_check(TopOfStack());
	if(TopOfStack().v.int_val) {    /* non-zero --> no jump*/
		Pop(a);
	}
	else
		JumpOffset = x->j_arg;  /* leave the argument on TOS */
}

void GpEval::f_jumpnz(GpArgument * x)
{
	t_value a;
	(void)x;                
	int_check(TopOfStack());
	if(TopOfStack().v.int_val) /* non-zero */
		JumpOffset = x->j_arg;  /* leave the argument on TOS */
	else {
		Pop(a);
	}
}

void GpEval::f_jtern(GpArgument * x)
{
	t_value a;
	int_check(Pop(a));
	if(!a.v.int_val)
		JumpOffset = x->j_arg;  /* go jump to false code */
}

void GpEval::f_columnhead(GpArgument * pArg)
{
	static char placeholder[] = "@COLUMNHEAD0000@";
	t_value a;
	if(!GpDf.evaluate_inside_using)
		R_Gg.IntError(GpGg.Gp__C.CToken-1, "columnhead() called from invalid context");
	Pop(a);
	GpDf.column_for_key_title = (int)a.Real();
	if(GpDf.column_for_key_title < 0 || GpDf.column_for_key_title > 9999)
		GpDf.column_for_key_title = 0;
	snprintf(placeholder+11, 6, "%4d@", GpDf.column_for_key_title);
	Push(Gstring(&a, placeholder));
}

void GpEval::f_valid(GpArgument * pArg)
{
	t_value a;
	int column, good;
	Pop(a);
	column = (int)magnitude(&a) - 1;
	good = (column >= 0 && column < GpDf.df_no_cols && GpDf.df_column[column].good == DF_GOOD);
	Push(a.SetInt(good));
}

/*{{{  void f_timecolumn() */
/* Version 5 - replace the old and very broken timecolumn(N) with
 * a 2-parameter version that requires an explicit time format
 * timecolumn(N, "format").
 */
void GpEval::f_timecolumn(GpArgument * pArg)
{
	t_value a;
	t_value b;
	struct tm tm;
	int num_param;
	int column;
	double usec = 0.0;
	Pop(b);          /* this is the number of parameters */
	num_param = b.v.int_val;
	Pop(b);          /* this is the time format string */
	switch(num_param) {
		case 2:
		    column = (int)magnitude(&Pop(a));
		    break;
		case 1:
		    // No format parameter passed (v4-style call) 
		    // Only needed for backward compatibility 
		    column = (int)magnitude(&b);
		    b.v.string_val = gp_strdup(R_Gg.P_TimeFormat);
		    b.type = STRING;
		    break;
		default:
		    R_Gg.IntErrorNoCaret("wrong number of parameters to timecolumn");
	}
	if(!GpDf.evaluate_inside_using)
		R_Gg.IntError(GpGg.Gp__C.CToken-1, "timecolumn() called from invalid context");
	if(b.type != STRING)
		R_Gg.IntErrorNoCaret("non-string passed as a format to timecolumn");
	if(column < 1 || column > GpDf.df_no_cols || !GpDf.df_column[column - 1].position ||
		!gstrptime(GpDf.df_column[column - 1].position, b.v.string_val, &tm, &usec)) {
		undefined = true;
		Push(&a);
	}
	else {
		Push(a.SetComplex(gtimegm(&tm) + usec, 0.0));
	}
	gpfree_string(&b);
}
