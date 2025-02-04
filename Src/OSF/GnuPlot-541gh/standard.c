// GNUPLOT - standard.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

static double carlson_elliptic_rc(double x, double y);
static double carlson_elliptic_rf(double x, double y, double z);
static double carlson_elliptic_rd(double x, double y, double z);
static double carlson_elliptic_rj(double x, double y, double z, double p);

#define PI_ON_FOUR       0.78539816339744830961566084581987572
#define PI_ON_TWO        1.57079632679489661923131269163975144
#define THREE_PI_ON_FOUR 2.35619449019234492884698253745962716
#define TWO_ON_PI        0.63661977236758134307553505349005744
// 
// April 2018
// libm does not provide Bessel approximations for i0 and i1
// so we do not include these in the conditional HAVE_LIBM
// 
static double ri0(double x);
static double ri1(double x);

#ifndef HAVE_LIBM
// 
// June 2017
// These hard-coded Bessel approximations are used only if we can't use the functions in libm
// 
// The bessel function approximations here are from "Computer Approximations"
// by Hart, Cheney et al. John Wiley & Sons, 1968
// 
static double dzero = 0.0; // don't make const: compiler will answer with "divide of mod by zero" error
// 
// There appears to be a mistake in Hart, Cheney et al. on page 149.
// Where it list Qn(x)/x ~ P(z*z)/Q(z*z), z = 8/x, it should read Qn(x)/z ~ P(z*z)/Q(z*z), z = 8/x
// In the functions below, Qn(x) is implemented using the later equation.
// These bessel functions are accurate to about 1e-13.
// 
// jzero for x in [0,8]
// Index 5849, 19.22 digits precision
// 
static const double pjzero[] = {
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

static const double qjzero[] = {
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
static const double pqzero[] = {
	-0.8922660020080009409846916e+2,
	-0.18591953644342993800252169e+3,
	-0.11183429920482737611262123e+3,
	-0.2230026166621419847169915e+2,
	-0.124410267458356384591379e+1,
	-0.8803330304868075181663e-2,
};

static const double qqzero[] = {
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
static const double pyzero[] = {
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

static const double qyzero[] = {
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

/* jone for x in [0,8]
 * Index 6050, 20.98 digits precision
 */
static const double pjone[] = {
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

static const double qjone[] = {
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
static const double ppone[] = {
	0.352246649133679798341724373e+5,
	0.62758845247161281269005675e+5,
	0.313539631109159574238669888e+5,
	0.49854832060594338434500455e+4,
	0.2111529182853962382105718e+3,
	0.12571716929145341558495e+1
};

static const double qpone[] = {
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
static const double pqone[] = {
	0.3511751914303552822533318e+3,
	0.7210391804904475039280863e+3,
	0.4259873011654442389886993e+3,
	0.831898957673850827325226e+2,
	0.45681716295512267064405e+1,
	0.3532840052740123642735e-1
};

static const double qqone[] = {
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
static const double pyone[] = {
	-0.2923821961532962543101048748e+20,
	0.7748520682186839645088094202e+19,
	-0.3441048063084114446185461344e+18,
	0.5915160760490070618496315281e+16,
	-0.4863316942567175074828129117e+14,
	0.2049696673745662182619800495e+12,
	-0.4289471968855248801821819588e+9,
	0.3556924009830526056691325215e+6
};

static const double qyone[] = {
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

#endif  /* hard-coded Bessel approximations if not HAVE_LIBM */

/* Chebyshev coefficients for exp(-x) I0(x)
 * in the interval [0,8].
 *
 * lim(x->0){ exp(-x) I0(x) } = 1.
 */
static const double cheb_i0_A[] = {
	-4.41534164647933937950E-18,
	3.33079451882223809783E-17,
	-2.43127984654795469359E-16,
	1.71539128555513303061E-15,
	-1.16853328779934516808E-14,
	7.67618549860493561688E-14,
	-4.85644678311192946090E-13,
	2.95505266312963983461E-12,
	-1.72682629144155570723E-11,
	9.67580903537323691224E-11,
	-5.18979560163526290666E-10,
	2.65982372468238665035E-9,
	-1.30002500998624804212E-8,
	6.04699502254191894932E-8,
	-2.67079385394061173391E-7,
	1.11738753912010371815E-6,
	-4.41673835845875056359E-6,
	1.64484480707288970893E-5,
	-5.75419501008210370398E-5,
	1.88502885095841655729E-4,
	-5.76375574538582365885E-4,
	1.63947561694133579842E-3,
	-4.32430999505057594430E-3,
	1.05464603945949983183E-2,
	-2.37374148058994688156E-2,
	4.93052842396707084878E-2,
	-9.49010970480476444210E-2,
	1.71620901522208775349E-1,
	-3.04682672343198398683E-1,
	6.76795274409476084995E-1
};

/* Chebyshev coefficients for exp(-x) I1(x) / x
 * in the interval [0,8].
 *
 * lim(x->0){ exp(-x) I1(x) / x } = 1/2.
 */
static const double cheb_i1_A[] = {
	2.77791411276104639959E-18,
	-2.11142121435816608115E-17,
	1.55363195773620046921E-16,
	-1.10559694773538630805E-15,
	7.60068429473540693410E-15,
	-5.04218550472791168711E-14,
	3.22379336594557470981E-13,
	-1.98397439776494371520E-12,
	1.17361862988909016308E-11,
	-6.66348972350202774223E-11,
	3.62559028155211703701E-10,
	-1.88724975172282928790E-9,
	9.38153738649577178388E-9,
	-4.44505912879632808065E-8,
	2.00329475355213526229E-7,
	-8.56872026469545474066E-7,
	3.47025130813767847674E-6,
	-1.32731636560394358279E-5,
	4.78156510755005422638E-5,
	-1.61760815825896745588E-4,
	5.12285956168575772895E-4,
	-1.51357245063125314899E-3,
	4.15642294431288815669E-3,
	-1.05640848946261981558E-2,
	2.47264490306265168283E-2,
	-5.29459812080949914269E-2,
	1.02643658689847095384E-1,
	-1.76416518357834055153E-1,
	2.52587186443633654823E-1
};

/* Chebyshev coefficients for exp(-x) sqrt(x) I0(x)
 * in the inverted interval [8,infinity].
 *
 * lim(x->inf){ exp(-x) sqrt(x) I0(x) } = 1/sqrt(2pi).
 */
static const double cheb_i0_B[] = {
	-7.23318048787475395456E-18,
	-4.83050448594418207126E-18,
	4.46562142029675999901E-17,
	3.46122286769746109310E-17,
	-2.82762398051658348494E-16,
	-3.42548561967721913462E-16,
	1.77256013305652638360E-15,
	3.81168066935262242075E-15,
	-9.55484669882830764870E-15,
	-4.15056934728722208663E-14,
	1.54008621752140982691E-14,
	3.85277838274214270114E-13,
	7.18012445138366623367E-13,
	-1.79417853150680611778E-12,
	-1.32158118404477131188E-11,
	-3.14991652796324136454E-11,
	1.18891471078464383424E-11,
	4.94060238822496958910E-10,
	3.39623202570838634515E-9,
	2.26666899049817806459E-8,
	2.04891858946906374183E-7,
	2.89137052083475648297E-6,
	6.88975834691682398426E-5,
	3.36911647825569408990E-3,
	8.04490411014108831608E-1
};

/* Chebyshev coefficients for exp(-x) sqrt(x) I1(x)
 * in the inverted interval [8,infinity].
 *
 * lim(x->inf){ exp(-x) sqrt(x) I1(x) } = 1/sqrt(2pi).
 */
static const double cheb_i1_B[] = {
	7.51729631084210481353E-18,
	4.41434832307170791151E-18,
	-4.65030536848935832153E-17,
	-3.20952592199342395980E-17,
	2.96262899764595013876E-16,
	3.30820231092092828324E-16,
	-1.88035477551078244854E-15,
	-3.81440307243700780478E-15,
	1.04202769841288027642E-14,
	4.27244001671195135429E-14,
	-2.10154184277266431302E-14,
	-4.08355111109219731823E-13,
	-7.19855177624590851209E-13,
	2.03562854414708950722E-12,
	1.41258074366137813316E-11,
	3.25260358301548823856E-11,
	-1.89749581235054123450E-11,
	-5.58974346219658380687E-10,
	-3.83538038596423702205E-9,
	-2.63146884688951950684E-8,
	-2.51223623787020892529E-7,
	-3.88256480887769039346E-6,
	-1.10588938762623716291E-4,
	-9.76109749136146840777E-3,
	7.78576235018280120474E-1
};
// 
// Make all the following internal routines perform autoconversion
// from string to numeric value.
// 
#define __POP__(x) PopOrConvertFromString(x)

//void f_real(union argument * /*arg*/)
void GnuPlot::F_Real(union argument * /*arg*/)
{
	GpValue a;
	Push(Gcomplex(&a, Real(__POP__(&a)), 0.0));
}

//void f_imag(union argument * /*arg*/)
void GnuPlot::F_Imag(union argument * /*arg*/)
{
	GpValue a;
	Push(Gcomplex(&a, Imag(__POP__(&a)), 0.0));
}

// ang2rad is 1 if we are in radians, or pi/180 if we are in degrees 
//void f_arg(union argument * /*arg*/)
void GnuPlot::F_Arg(union argument * /*arg*/)
{
	GpValue a;
	Push(Gcomplex(&a, Angle(__POP__(&a)) / Gg.ang2rad, 0.0));
}

//void f_conjg(union argument * /*arg*/)
void GnuPlot::F_Conjg(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	Push(Gcomplex(&a, Real(&a), -Imag(&a)));
}

/* ang2rad is 1 if we are in radians, or pi/180 if we are in degrees */

//void f_sin(union argument * /*arg*/)
void GnuPlot::F_Sin(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	Push(Gcomplex(&a, sin(Gg.ang2rad * Real(&a)) * cosh(Gg.ang2rad * Imag(&a)), cos(Gg.ang2rad * Real(&a)) * sinh(Gg.ang2rad * Imag(&a))));
}

//void f_cos(union argument * /*arg*/)
void GnuPlot::F_Cos(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	Push(Gcomplex(&a, cos(Gg.ang2rad * Real(&a)) * cosh(Gg.ang2rad * Imag(&a)), -sin(Gg.ang2rad * Real(&a)) * sinh(Gg.ang2rad * Imag(&a))));
}

//void f_tan(union argument * /*arg*/)
void GnuPlot::F_Tan(union argument * /*arg*/)
{
	GpValue a;
	double den;
	__POP__(&a);
	if(Imag(&a) == 0.0)
		Push(Gcomplex(&a, tan(Gg.ang2rad * Real(&a)), 0.0));
	else {
		den = cos(2 * Gg.ang2rad * Real(&a)) + cosh(2 * Gg.ang2rad * Imag(&a));
		if(den == 0.0) {
			Ev.IsUndefined_ = true;
			Push(&a);
		}
		else
			Push(Gcomplex(&a, sin(2 * Gg.ang2rad * Real(&a)) / den, sinh(2 * Gg.ang2rad * Imag(&a)) / den));
	}
}

//void f_asin(union argument * /*arg*/)
void GnuPlot::F_ASin(union argument * /*arg*/)
{
	GpValue a;
	double alpha, beta;
	double t;
	__POP__(&a);
	double x = Real(&a);
	double y = Imag(&a);
	int ysign = (y >= 0) ? 1 : -1;
	if(y == 0.0 && fabs(x) <= 1.0) {
		Push(Gcomplex(&a, asin(x) / Gg.ang2rad, 0.0));
	}
	else if(x == 0.0) {
		// Rearrange terms to avoid loss of precision 
		// t = -log(-y + sqrt(y * y + 1)) 
		alpha = sqrt(1 + y * y);
		t = ysign * log(alpha + sqrt(alpha * alpha - 1));
		Push(Gcomplex(&a, 0.0, t / Gg.ang2rad));
	}
	else {
		beta = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		if(beta > 1)
			beta = 1; // Avoid rounding error problems 
		alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		t = ysign * log(alpha + sqrt(alpha * alpha - 1));
		Push(Gcomplex(&a, asin(beta) / Gg.ang2rad, t / Gg.ang2rad));
	}
}

//void f_acos(union argument * /*arg*/)
void GnuPlot::F_ACos(union argument * /*arg*/)
{
	GpValue a;
	double ysign;
	__POP__(&a);
	double x = Real(&a);
	double y = Imag(&a);
	if(y == 0.0 && fabs(x) <= 1.0) {
		// real result 
		Push(Gcomplex(&a, acos(x) / Gg.ang2rad, 0.0));
	}
	else {
		double alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		double beta = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		if(beta > 1)
			beta = 1; // Avoid rounding error problems 
		else if(beta < -1)
			beta = -1;
		ysign = (y >= 0) ? 1 : -1;
		Push(Gcomplex(&a, acos(beta) / Gg.ang2rad, -ysign * log(alpha + sqrt(alpha * alpha - 1)) / Gg.ang2rad));
	}
}

//void f_atan(union argument * /*arg*/)
void GnuPlot::F_ATan(union argument * /*arg*/)
{
	GpValue a;
	double u, v, w, z;
	__POP__(&a);
	double x = Real(&a);
	double y = Imag(&a);
	if(y == 0.0)
		Push(Gcomplex(&a, atan(x) / Gg.ang2rad, 0.0));
	else if(x == 0.0 && fabs(y) >= 1.0) {
		Ev.IsUndefined_ = true;
		Push(Gcomplex(&a, 0.0, 0.0));
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
			z = z + 2 * PI_ON_TWO;
		if(x < 0) {
			z = -z;
			w = -w;
		}
		Push(Gcomplex(&a, 0.5 * z / Gg.ang2rad, w));
	}
}
//
// real parts only 
//
//void f_atan2(union argument * /*arg*/)
void GnuPlot::F_ATan2(union argument * /*arg*/)
{
	GpValue a;
	double x = Real(__POP__(&a));
	double y = Real(__POP__(&a));
	if(x == 0.0 && y == 0.0) {
		Ev.IsUndefined_ = true;
		Push(Ginteger(&a, 0));
	}
	Push(Gcomplex(&a, atan2(y, x) / Gg.ang2rad, 0.0));
}

//void f_sinh(union argument * /*arg*/)
void GnuPlot::F_Sinh(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	Push(Gcomplex(&a, sinh(Real(&a)) * cos(Imag(&a)), cosh(Real(&a)) * sin(Imag(&a))));
}

//void f_cosh(union argument * /*arg*/)
void GnuPlot::F_Cosh(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	Push(Gcomplex(&a, cosh(Real(&a)) * cos(Imag(&a)), sinh(Real(&a)) * sin(Imag(&a))));
}

//void f_tanh(union argument * /*arg*/)
void GnuPlot::F_Tanh(union argument * /*arg*/)
{
	GpValue a;
	double den;
	__POP__(&a);
	double real_2arg = 2.0 * Real(&a);
	double imag_2arg = 2.0 * Imag(&a);
#ifdef E_MINEXP
	if(-fabs(real_2arg) < E_MINEXP) {
		Push(Gcomplex(&a, real_2arg < 0 ? -1.0 : 1.0, 0.0));
		return;
	}
#else
	{
		int old_errno = errno;
		if(exp(-fabs(real_2arg)) == 0.0) {
			/* some libm's will raise a silly ERANGE in cosh() and sin() */
			errno = old_errno;
			Push(Gcomplex(&a, real_2arg < 0 ? -1.0 : 1.0, 0.0));
			return;
		}
	}
#endif
	den = cosh(real_2arg) + cos(imag_2arg);
	Push(Gcomplex(&a, sinh(real_2arg) / den, sin(imag_2arg) / den));
}

//void f_asinh(union argument * arg)
void GnuPlot::F_ASinh(union argument * arg)
{
	GpValue a; /* asinh(z) = -I*asin(I*z) */
	double alpha, beta, x, y;
	double t;
	int ysign;
	__POP__(&a);
	x = -Imag(&a);
	y = Real(&a);
	ysign = (y >= 0) ? 1 : -1;
	if(y == 0.0 && fabs(x) <= 1.0) {
		Push(Gcomplex(&a, 0.0, -asin(x) / Gg.ang2rad));
	}
	else if(y == 0.0) {
		Push(Gcomplex(&a, 0.0, 0.0));
		Ev.IsUndefined_ = true;
	}
	else if(x == 0.0) {
		// Rearrange terms to avoid loss of precision 
		// t = log(y + sqrt(y * y + 1)) 
		alpha = sqrt(y * y + 1);
		t = ysign * log(alpha + sqrt(alpha * alpha - 1));
		Push(Gcomplex(&a, t / Gg.ang2rad, 0.0));
	}
	else {
		beta  = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		t = ysign * log(alpha + sqrt(alpha * alpha - 1));
		Push(Gcomplex(&a, t / Gg.ang2rad, -asin(beta) / Gg.ang2rad));
	}
}

//void f_acosh(union argument * arg)
void GnuPlot::F_ACosh(union argument * arg)
{
	GpValue a;
	double alpha, beta, x, y; /* acosh(z) = I*acos(z) */
	__POP__(&a);
	x = Real(&a);
	y = Imag(&a);
	if(y == 0.0 && fabs(x) <= 1.0) {
		Push(Gcomplex(&a, 0.0, acos(x) / Gg.ang2rad));
	}
	else if(y == 0.0 && x > 1.0) {
		Push(Gcomplex(&a, log(x + sqrt(x * x - 1)) / Gg.ang2rad, 0.0));
	}
	else {
		alpha = sqrt((x + 1) * (x + 1) + y * y) / 2 + sqrt((x - 1) * (x - 1) + y * y) / 2;
		beta = sqrt((x + 1) * (x + 1) + y * y) / 2 - sqrt((x - 1) * (x - 1) + y * y) / 2;
		Push(Gcomplex(&a, log(alpha + sqrt(alpha * alpha - 1)) / Gg.ang2rad, (y<0 ? -1 : 1) * acos(beta) / Gg.ang2rad));
	}
}

//void f_atanh(union argument * arg)
void GnuPlot::F_ATanh(union argument * arg)
{
	GpValue a;
	double x, y, u, v, w, z; /* atanh(z) = -I*atan(I*z) */
	__POP__(&a);
	x = -Imag(&a);
	y = Real(&a);
	if(y == 0.0)
		Push(Gcomplex(&a, 0.0, -atan(x) / Gg.ang2rad));
	else if(x == 0.0 && fabs(y) >= 1.0) {
		Ev.IsUndefined_ = true;
		Push(Gcomplex(&a, 0.0, 0.0));
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
			z = z + 2 * PI_ON_TWO;
		if(x < 0) {
			z = -z;
			w = -w;
		}
		Push(Gcomplex(&a, w, -0.5 * z / Gg.ang2rad));
	}
}

//void f_ellip_first(union argument * arg)
void GnuPlot::F_EllipFirst(union argument * arg)
{
	GpValue a;
	double ak, q;
	__POP__(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do elliptic integrals of reals");
	ak = Real(&a);
	q = (1.0-ak)*(1.0+ak);
	if(q > 0.0)
		Push(Gcomplex(&a, carlson_elliptic_rf(0.0, q, 1.0), 0.0));
	else {
		Push(&a);
		Ev.IsUndefined_ = true;
	}
}

//void f_ellip_second(union argument * /*arg*/)
void GnuPlot::F_EllipSecond(union argument * /*arg*/)
{
	GpValue a;
	double ak, q, e;
	__POP__(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do elliptic integrals of reals");
	ak = Real(&a);
	q = (1.0-ak)*(1.0+ak);
	if(q > 0.0) {
		e = carlson_elliptic_rf(0.0, q, 1.0)-(ak*ak)*carlson_elliptic_rd(0.0, q, 1.0)/3.0;
		Push(Gcomplex(&a, e, 0.0));
	}
	else if(q < 0.0) {
		Ev.IsUndefined_ = true;
		Push(&a);
	}
	else {
		e = 1.0;
		Push(Gcomplex(&a, e, 0.0));
	}
}

//void f_ellip_third(union argument * arg)
void GnuPlot::F_EllipThird(union argument * arg)
{
	GpValue a1, a2;
	double ak, en, q;
	__POP__(&a1);
	__POP__(&a2);
	if(fabs(Imag(&a1)) > Gg.Zero || fabs(Imag(&a2)) > Gg.Zero)
		IntError(NO_CARET, "can only do elliptic integrals of reals");
	ak = Real(&a1);
	en = Real(&a2);
	q = (1.0-ak)*(1.0+ak);
	if(q > 0.0 && en < 1.0)
		Push(Gcomplex(&a2, carlson_elliptic_rf(0.0, q, 1.0)+en*carlson_elliptic_rj(0.0, q, 1.0, 1.0-en)/3.0, 0.0));
	else {
		Ev.IsUndefined_ = true;
		Push(&a1);
	}
}
// 
// int(x) returns the integer portion of x, rounded towards zero.
// This is trunc(x) as defined in C99 except that we always return an int.
// If x > largest 64bit int we return NaN.
// Unlike floor, ceil, and round,  int(x) will return an int even if x
// is so large that the limited precision in the floating point representation
// means that several integers share that same representation.
// This turns out to be roughly the range 10^15 < x < 10^19.
// I.e. in that range |int(real(N)) - N| may be non-zero.
// 
//void f_int(union argument * /*arg*/)
void GnuPlot::F_Int(union argument * /*arg*/)
{
	GpValue a;
	double foo = Real(__POP__(&a));
	if(a.Type == NOTDEFINED || isnan(foo)) {
		Push(Gcomplex(&a, fgetnan(), 0.0));
		Ev.IsUndefined_ = true;
	}
	else if(a.Type == INTGR) {
		Push(&a);
#ifdef GNUPLOT_INT64_SUPPORT
	}
	else if(fabs(foo) >= LARGEST_GUARANTEED_NONOVERFLOW) {
		if(Ev.OverflowHandling == INT64_OVERFLOW_UNDEFINED)
			Ev.IsUndefined_ = true;
		Push(Gcomplex(&a, fgetnan(), 0.0));
#else
	}
	else if(fabs(foo) > LARGEST_GUARANTEED_NONOVERFLOW) {
		Push(Gcomplex(&a, fgetnan(), 0.0));
#endif
	}
	else
		Push(Ginteger(&a, (intgr_t)trunc(foo)));
}
// 
// round(x) returns the integer nearest to the real part of x.
// This is a wrapper for llround(x) as defined in C99.
// If x is so large that round(x) would not necessarily be the nearest int
// due to limited precision, we return NaN rather than an incorrect integer.
// 
//void f_round(union argument * /*arg*/)
void GnuPlot::F_Round(union argument * /*arg*/)
{
	GpValue a;
	double foo = Real(__POP__(&a));
	if(a.Type == NOTDEFINED || isnan(foo)) {
		Push(Gcomplex(&a, fgetnan(), 0.0));
		Ev.IsUndefined_ = true;
	}
	else if(a.Type == INTGR) {
		Push(&a);
#ifdef GNUPLOT_INT64_SUPPORT
	}
	else if(fabs(foo) >= LARGEST_EXACT_INT/2.) {
		if(Ev.OverflowHandling == INT64_OVERFLOW_UNDEFINED)
			Ev.IsUndefined_ = true;
		Push(Gcomplex(&a, fgetnan(), 0.0));
#else
	}
	else if(fabs(foo) >= LARGEST_GUARANTEED_NONOVERFLOW) {
		Push(Gcomplex(&a, fgetnan(), 0.0));
#endif
	}
	else
		Push(Ginteger(&a, (intgr_t)llround(foo)));
}

//void f_abs(union argument * /*arg*/)
void GnuPlot::F_Abs(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	switch(a.Type) {
		case INTGR:
		    Push(Ginteger(&a, a.v.int_val < 0 ? -a.v.int_val : a.v.int_val));
		    break;
		case CMPLX:
		    Push(Gcomplex(&a, Magnitude(&a), 0.0));
		    break;
		default: IntError(NO_CARET, "internal error : argument neither INT or CMPLX");
	}
}

//void f_sgn(union argument * /*arg*/)
void GnuPlot::F_Sgn(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	switch(a.Type) {
		case INTGR:
		    Push(Ginteger(&a, (a.v.int_val > 0) ? 1 : (a.v.int_val < 0) ? -1 : 0));
		    break;
		case CMPLX:
		    Push(Ginteger(&a, (a.v.cmplx_val.real > 0.0) ? 1 : (a.v.cmplx_val.real < 0.0) ? -1 : 0));
		    break;
		default: IntError(NO_CARET, "internal error : argument neither INT or CMPLX");
	}
}

//void f_sqrt(union argument * /*arg*/)
void GnuPlot::F_Sqrt(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	double mag = sqrt(Magnitude(&a));
	if(Imag(&a) == 0.0) {
		if(Real(&a) < 0.0)
			Push(Gcomplex(&a, 0.0, mag));
		else
			Push(Gcomplex(&a, mag, 0.0));
	}
	else {
		// -pi < ang < pi, so real(sqrt(z)) >= 0 
		double ang = Angle(&a) / 2.0;
		Push(Gcomplex(&a, mag * cos(ang), mag * sin(ang)));
	}
}

//void f_exp(union argument * /*arg*/)
void GnuPlot::F_Exp(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	double mag = gp_exp(Real(&a));
	double ang = Imag(&a);
	Push(Gcomplex(&a, mag * cos(ang), mag * sin(ang)));
}

//void f_log10(union argument * /*arg*/)
void GnuPlot::F_Log10(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	if(Magnitude(&a) == 0.0) {
		Ev.IsUndefined_ = true;
		Push(&a);
	}
	else
		Push(Gcomplex(&a, log(Magnitude(&a)) / SMathConst::Ln10, Angle(&a) / SMathConst::Ln10));
}

//void f_log(union argument * /*arg*/)
void GnuPlot::F_Log(union argument * /*arg*/)
{
	GpValue a;
	__POP__(&a);
	if(Magnitude(&a) == 0.0) {
		Ev.IsUndefined_ = true;
		Push(&a);
	}
	else
		Push(Gcomplex(&a, log(Magnitude(&a)), Angle(&a)));
}
// 
// This is a wrapper for C99 floor(x) except that we always return an int.
// If x is so large that floor(x) would not necessarily be the nearest int
// due to limited precision, we return NaN rather than an incorrect integer.
// 
//void f_floor(union argument * /*arg*/)
void GnuPlot::F_Floor(union argument * /*arg*/)
{
	GpValue a;
	double foo;
	__POP__(&a);
	switch(a.Type) {
		case INTGR:
		    Push(&a);
		    break;
		case CMPLX:
		    foo = a.v.cmplx_val.real;
#ifdef GNUPLOT_INT64_SUPPORT
		    // Note: this test catches NaN also 
		    if(!(fabs(foo) < LARGEST_EXACT_INT/2.)) {
			    if(Ev.OverflowHandling == INT64_OVERFLOW_UNDEFINED)
				    Ev.IsUndefined_ = true;
			    Push(Gcomplex(&a, fgetnan(), 0.0));
#else
		    if(fabs(foo) >= LARGEST_GUARANTEED_NONOVERFLOW) {
			    Push(Gcomplex(&a, fgetnan(), 0.0));
#endif
		    }
		    else {
			    Push(Ginteger(&a, (intgr_t)floor(foo)));
		    }
		    break;
		default: IntError(NO_CARET, "internal error : argument neither INT or CMPLX");
	}
}
// 
// This is a wrapper for C99 ceil(x) except that we always return an int.
// If x is so large that ceil(x) would not necessarily be the nearest int
// due to limited precision, we return NaN rather than an incorrect integer.
// 
//void f_ceil(union argument * /*arg*/)
void GnuPlot::F_Ceil(union argument * /*arg*/)
{
	GpValue a;
	double foo;
	__POP__(&a);
	switch(a.Type) {
		case INTGR:
		    Push(&a);
		    break;
		case CMPLX:
		    foo = a.v.cmplx_val.real;
#ifdef GNUPLOT_INT64_SUPPORT
		    // Note: this test catches NaN also 
		    if(!(fabs(foo) < LARGEST_EXACT_INT/2.)) {
			    if(Ev.OverflowHandling == INT64_OVERFLOW_UNDEFINED)
				    Ev.IsUndefined_ = true;
			    Push(Gcomplex(&a, fgetnan(), 0.0));
#else
		    if(fabs(foo) >= LARGEST_GUARANTEED_NONOVERFLOW) {
			    Push(Gcomplex(&a, fgetnan(), 0.0));
#endif
		    }
		    else {
			    Push(Ginteger(&a, (intgr_t)ceil(foo)));
		    }
		    break;
		default: IntError(NO_CARET, "internal error : argument neither INT or CMPLX");
	}
}

#undef __POP__ // Terminate the autoconversion from string to numeric values 
//
// EAM - replacement for defined(foo) + f_pushv + f_isvar
//       implements      exists("foo") instead
// 
void GnuPlot::F_Exists(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(a.Type == STRING) {
		udvt_entry * udv = Ev.AddUdvByName(a.v.string_val);
		gpfree_string(&a);
		Push(Ginteger(&a, udv->udv_value.Type == NOTDEFINED ? 0 : 1));
	}
	else {
		Push(Ginteger(&a, 0));
	}
}

#ifdef HAVE_LIBM
/* June 2017
 * Use the Bessel functions from libm if possible
 */
#define rj0(x) j0(x)
#define rj1(x) j1(x)
#define ry0(x) y0(x)
#define ry1(x) y1(x)
#else

/* June 2017
 * These hard-coded Bessel approximations are used only if we
 * can't use the functions in ligm
 */

/* bessel function approximations */
static double jzero(double x)
{
	int n;
	double x2 = x * x;
	double p = pjzero[8];
	double q = qjzero[8];
	for(n = 7; n >= 0; n--) {
		p = p * x2 + pjzero[n];
		q = q * x2 + qjzero[n];
	}
	return (p / q);
}

static double pzero(double x)
{
	int n;
	double z = 8.0 / x;
	double z2 = z * z;
	double p = ppzero[5];
	double q = qpzero[5];
	for(n = 4; n >= 0; n--) {
		p = p * z2 + ppzero[n];
		q = q * z2 + qpzero[n];
	}
	return (p / q);
}

static double qzero(double x)
{
	int n;
	double z = 8.0 / x;
	double z2 = z * z;
	double p = pqzero[5];
	double q = qqzero[5];
	for(n = 4; n >= 0; n--) {
		p = p * z2 + pqzero[n];
		q = q * z2 + qqzero[n];
	}
	return (p / q);
}

static double yzero(double x)
{
	int n;
	double x2 = x * x;
	double p = pyzero[8];
	double q = qyzero[8];
	for(n = 7; n >= 0; n--) {
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
		return (sqrt(TWO_ON_PI / x) * (pzero(x) * cos(x - PI_ON_FOUR) - 8.0 / x * qzero(x) * sin(x - PI_ON_FOUR)));
}

static double ry0(double x)
{
	if(x < 0.0)
		return (dzero / dzero); // error 
	if(x < 8.0)
		return (yzero(x) + TWO_ON_PI * rj0(x) * log(x));
	else
		return (sqrt(TWO_ON_PI / x) * (pzero(x) * sin(x - PI_ON_FOUR) + (8.0 / x) * qzero(x) * cos(x - PI_ON_FOUR)));
}

static double jone(double x)
{
	int n;
	double x2 = x * x;
	double p = pjone[8];
	double q = qjone[8];
	for(n = 7; n >= 0; n--) {
		p = p * x2 + pjone[n];
		q = q * x2 + qjone[n];
	}
	return (p / q);
}

static double pone(double x)
{
	double z = 8.0 / x;
	double z2 = z * z;
	double p = ppone[5];
	double q = qpone[5];
	for(int n = 4; n >= 0; n--) {
		p = p * z2 + ppone[n];
		q = q * z2 + qpone[n];
	}
	return (p / q);
}

static double qone(double x)
{
	double z = 8.0 / x;
	double z2 = z * z;
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
	int n;
	double x2 = x * x;
	double p = 0.0;
	double q = qyone[8];
	for(n = 7; n >= 0; n--) {
		p = p * x2 + pyone[n];
		q = q * x2 + qyone[n];
	}
	return (p / q);
}

static double rj1(double x)
{
	double v = x;
	if(x < 0.0)
		x = -x;
	if(x < 8.0)
		return (v * jone(x));
	else {
		double w = sqrt(TWO_ON_PI / x) * (pone(x) * cos(x - THREE_PI_ON_FOUR) - 8.0 / x * qone(x) * sin(x - THREE_PI_ON_FOUR));
		if(v < 0.0)
			w = -w;
		return (w);
	}
}

static double ry1(double x)
{
	if(x <= 0.0)
		return (dzero / dzero); /* error */
	if(x < 8.0)
		return (x * yone(x) + TWO_ON_PI * (rj1(x) * log(x) - 1.0 / x));
	else
		return (sqrt(TWO_ON_PI / x) * (pone(x) * sin(x - THREE_PI_ON_FOUR) + (8.0 / x) * qone(x) * cos(x - THREE_PI_ON_FOUR)));
}

#define jn(n, x) fgetnan()
#define yn(n, x) fgetnan()

#endif  /* hard-coded Bessel approximations if not HAVE_LIBM */

/*
 * Evaluates the series (with n coefficients stored in array[])
 * of Chebyshev polynomials Ti at argument x/2.
 */
static double chbevl(double x, const double array[], int n) 
{
	double b2;
	const double * p = array;
	double b0 = *p++;
	double b1 = 0;
	int i = n - 1;
	do {
		b2 = b1;
		b1 = b0;
		b0 = x * b1  -  b2  + *p++;
	} while(--i);
	return( 0.5*(b0 - b2) );
}

static double ri0(double x)
{
	double ax = fabs(x);
	double y;
	if(ax <= 8.0) {
		y = (ax/2.0) - 2.0;
		return( exp(ax) * chbevl(y, cheb_i0_A, 30) );
	}
	return( exp(ax) * chbevl(32.0/ax - 2.0, cheb_i0_B, 25) / sqrt(ax) );
}

static double ri1(double x)
{
	double y;
	double z = fabs(x);
	if(z <= 8.0) {
		y = (z/2.0) - 2.0;
		z = chbevl(y, cheb_i1_A, 29) * z * exp(z);
	}
	else {
		z = exp(z) * chbevl(32.0/z - 2.0, cheb_i1_B, 25) / sqrt(z);
	}
	if(x < 0.0) 
		z = -z;
	return z;
}
// 
// FIXME HBB 20010726: should bessel functions really call int_error,
// right in the middle of evaluating some mathematical expression?
// Couldn't they just flag 'undefined', or ignore the real part of the
// complex number? 
// 
void GnuPlot::F_Besi0(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do bessel functions of reals");
	Push(Gcomplex(&a, ri0(Real(&a)), 0.0));
}

void GnuPlot::F_Besi1(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do bessel functions of reals");
	Push(Gcomplex(&a, ri1(Real(&a)), 0.0));
}

void GnuPlot::F_Besj0(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do bessel functions of reals");
	Push(Gcomplex(&a, rj0(Real(&a)), 0.0));
}

void GnuPlot::F_Besj1(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do bessel functions of reals");
	Push(Gcomplex(&a, rj1(Real(&a)), 0.0));
}

void GnuPlot::F_Besy0(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do bessel functions of reals");
	if(Real(&a) > 0.0)
		Push(Gcomplex(&a, ry0(Real(&a)), 0.0));
	else {
		Push(Gcomplex(&a, 0.0, 0.0));
		Ev.IsUndefined_ = true;
	}
}

void GnuPlot::F_Besy1(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	if(fabs(Imag(&a)) > Gg.Zero)
		IntError(NO_CARET, "can only do bessel functions of reals");
	if(Real(&a) > 0.0)
		Push(Gcomplex(&a, ry1(Real(&a)), 0.0));
	else {
		Push(Gcomplex(&a, 0.0, 0.0));
		Ev.IsUndefined_ = true;
	}
}

void GnuPlot::F_Besjn(union argument * /*arg*/)
{
	GpValue a, n;
	Pop(&a);
	Pop(&n);
	if((n.Type != INTGR) || (fabs(Imag(&a)) > Gg.Zero)) {
		Push(Gcomplex(&a, 0.0, 0.0));
		Ev.IsUndefined_ = true;
		IntError(NO_CARET, "improper argument to besjn(int,real)");
	}
	else {
		Push(Gcomplex(&a, jn(n.v.int_val, Real(&a)), 0.0));
	}
}

void GnuPlot::F_Besyn(union argument * /*arg*/)
{
	GpValue a, n;
	Pop(&a);
	Pop(&n);
	if((n.Type != INTGR) || (fabs(Imag(&a)) > Gg.Zero)) {
		Push(Gcomplex(&a, 0.0, 0.0));
		Ev.IsUndefined_ = true;
		IntError(NO_CARET, "improper argument to besyn(int,real)");
	}
	else {
		Push(Gcomplex(&a, yn(n.v.int_val, Real(&a)), 0.0));
	}
}
//
// functions for accessing fields from tm structure, for time series
// they are all the same, so define a macro
///
#define TIMEFUNC(name, field)                                   \
	void name(union argument * /*arg*/) \
	{                                                               \
		GpValue a;                                          \
		struct tm tm;                                               \
		Pop(&a);                                             \
		GGmTime(&tm, Real(&a));                                     \
		Push(Gcomplex(&a, (double)tm.field, 0.0));                  \
	}

TIMEFUNC(GnuPlot::F_TmSec, tm_sec)
TIMEFUNC(GnuPlot::F_TmMin, tm_min)
TIMEFUNC(GnuPlot::F_TmHour, tm_hour)
TIMEFUNC(GnuPlot::F_TmMDay, tm_mday)
TIMEFUNC(GnuPlot::F_TmMon, tm_mon)
TIMEFUNC(GnuPlot::F_TmYear, tm_year)
TIMEFUNC(GnuPlot::F_TmWDay, tm_wday)
TIMEFUNC(GnuPlot::F_TmYDay, tm_yday)

//void f_tmweek(union argument * /*arg*/)
void GnuPlot::F_TmWeek(union argument * /*arg*/)
{
	GpValue a;
	Pop(&a);
	int week = TmWeek(Real(&a));
	Push(Gcomplex(&a, (double)week, 0.0));
}

// 

#define SQR(a)          ((a)*(a))

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
	double ave, delx, dely, delz, e2, e3;
	double xt = x;
	double yt = y;
	double zt = z;
	do {
		double sqrtx = sqrt(xt);
		double sqrty = sqrt(yt);
		double sqrtz = sqrt(zt);
		double alamb = sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
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
	return((1.0+((1.0/24.0)*e2-(0.1)-(3.0/44.0)*e3)*e2+(1.0/14.0)*e3)/sqrt(ave));
}

#define C1 (3.0/14.0)
#define C2 (1.0/6.0)
#define C3 (9.0/22.0)
#define C4 (3.0/26.0)
#define C5 (0.25*C3)
#define C6 (1.5*C4)

static double carlson_elliptic_rd(double x, double y, double z)
{
	double ave, delx, dely, delz, ans;
	double xt = x;
	double yt = y;
	double zt = z;
	double sum = 0.0;
	double fac = 1.0;
	do {
		const double sqrtx = sqrt(xt);
		const double sqrty = sqrt(yt);
		const double sqrtz = sqrt(zt);
		const double alamb = sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
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
	{
		const double ea = delx*dely;
		const double eb = delz*delz;
		const double ec = ea-eb;
		const double ed = ea-6.0*eb;
		const double ee = ed+ec+ec;
		ans = 3.0*sum+fac*(1.0+ed*(-C1+C5*ed-C6*delz*ee)+delz*(C2*ee+delz*(-C3*ec+delz*C4*ea)))/(ave*sqrt(ave));
		return ans;
	}
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
	double a, ans, ave, b, delp, delx, dely, delz, pt, rcx, rho, tau, xt, yt, zt;
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
		xt = smin3(x, y, z);
		zt = smax3(x, y, z);
		yt = x+y+z-xt-zt;
		a = 1.0/(yt-p);
		b = a*(zt-yt)*(yt-xt);
		pt = yt+b;
		rho = xt*zt/yt;
		tau = p*pt/yt;
		rcx = carlson_elliptic_rc(rho, tau);
	}
	do {
		const double sqrtx = sqrt(xt);
		const double sqrty = sqrt(yt);
		const double sqrtz = sqrt(zt);
		const double alamb = sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
		const double alpha = SQR(pt*(sqrtx+sqrty+sqrtz)+sqrtx*sqrty*sqrtz);
		const double beta = pt*SQR(pt+alamb);
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
	{
		const double ea = delx*(dely+delz)+dely*delz;
		const double eb = delx*dely*delz;
		const double ec = delp*delp;
		const double ed = ea-3.0*ec;
		const double ee = eb+2.0*delp*(ea-ec);
		ans = 3.0*sum+fac*(1.0+ed*(-C1+C5*ed-C6*ee)+eb*(C7+delp*(-C8+delp*C4)) + delp*ea*(C2-delp*C3)-C2*delp*ec)/(ave*sqrt(ave));
		if(p <= 0.0)
			ans = a*(b*ans+3.0*(rcx-carlson_elliptic_rf(xt, yt, zt)));
		return ans;
	}
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
