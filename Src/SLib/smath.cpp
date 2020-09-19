// SMATH.CPP
// Copyright (c) A.Sobolev 2004, 2006, 2007, 2008, 2010, 2012, 2014, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <float.h>

#define PI__  3.1415926535897932384626433832795
#define PI__f 3.14159265358979323846f

const double SMathConst::Pi       = PI__;
const double SMathConst::Pi2      = (PI__ * 2.0);
const float  SMathConst::Pi2_f    = (PI__f * 2.0f);
const float  SMathConst::Pi_f     = PI__f;
const double SMathConst::PiDiv180 = (PI__ / 180.0);
const float  SMathConst::PiDiv180_f = (PI__f / 180.0f);
const double SMathConst::PiDiv4   = (PI__ / 4.0);
const float  SMathConst::PiDiv4_f = (PI__f / 4.0f);
const double SMathConst::PiDiv2   = (PI__ / 2.0);
const double SMathConst::E        = 2.71828182845904523536;
const double SMathConst::LnPi     = 1.14472988584940017414342735135; // ln(pi)
const double SMathConst::Ln2      = 0.69314718055994530941723212146; // ln(2)
const double SMathConst::Epsilon  = 2.2204460492503131e-16;          // smallest such that 1.0+DBL_EPSILON != 1.0
const double SMathConst::Root4Epsilon = 1.2207031250000000e-04;
const double SMathConst::Root5Epsilon = 7.4009597974140505e-04;
const double SMathConst::SqrtMin  = 1.4916681462400413e-154;
const double SMathConst::SqrtMax  = 1.3407807929942596e+154;
const double SMathConst::LogMin   = -7.0839641853226408e+02;
const double SMathConst::LogMax   = 7.0978271289338397e+02;
const double SMathConst::Min      = 2.2250738585072014e-308;         // min value
const double SMathConst::Max      = 1.7976931348623158e+308;         // max value
const double SMathConst::Euler    = 0.57721566490153286060651209008; // Euler constant
const double SMathConst::Sqrt2    = 1.41421356237309504880168872421; // sqrt(2)
const double SMathConst::SqrtPi   = 1.77245385090551602729816748334; // sqrt(pi)
const double SMathConst::Sqrt1_2  = 0.707106781186547524400844362104849039; // sqrt(0.5)

/* @experimental
static double FASTCALL MakePositiveDouble(uint64 mantissa, int32 order)
{
    if(mantissa != 0) {
		union U {;
			uint32 i32[2];
			uint64 i64;
			double R;
		} v;
		U vt;
		vt.R = 1.5;
		vt.R = 0.15;
		v.i64 = mantissa;
		uint32 msb_h = msb32(v.i32[1]);
		if(msb_h > 0x001fffff) {
			do {
				v.i64 >>= 1;
				order++;
				msb_h = msb32(v.i32[1]);
			} while(msb_h > 0x001fffff);
		}
		if(msb_h) {
			v.i32[1] &= ~msb_h;
			//order++;
		}
		else {
			uint32 msb_l = msb32(v.i32[0]);
			if(msb_l) {
				v.i32[0] &= ~msb_l;
				if(msb_l > 1)
					order++;
			}
		}
		v.i32[1] |= ((order + 1023) << 20);
		return v.R;
	}
	else
		return 0.0;
}

void SLAPI ExploreIEEE754()
{
	static double temp;
	double r = atof("2.625");
	temp = r;
}
*/

//int SLAPI IsValidIEEE(double v) { return _finite(v) ? 1 : 0; }

int SLAPI IsValidIEEE(double v)
{
	// _FPCLASS_SNAN	Signaling NaN
	// _FPCLASS_QNAN	Quiet NaN
	// _FPCLASS_NINF	Negative infinity ( –INF)
	// _FPCLASS_NN		Negative normalized non-zero
	// _FPCLASS_ND		Negative denormalized
	// _FPCLASS_NZ		Negative zero ( – 0)
	// _FPCLASS_PZ		Positive 0 (+0)
	// _FPCLASS_PD		Positive denormalized
	// _FPCLASS_PN		Positive normalized non-zero
	// _FPCLASS_PINF	Positive infinity (+INF)
	int    c = _fpclass(v);
	return oneof6(c, _FPCLASS_NN, _FPCLASS_ND, _FPCLASS_NZ, _FPCLASS_PZ, _FPCLASS_PD, _FPCLASS_PN);
}

int fisnan(double v)
{
	return _isnan(v);
}

int fisinf(double v)
{
	const int fpc = _fpclass(v);
	return (fpc == _FPCLASS_PINF) ? +1 : ((fpc == _FPCLASS_NINF) ? -1 : 0);
}
//
// SMathResult {
//
void SMathResult::SetErr(double e, double adjMult) { E = (e + (adjMult * SMathConst::Epsilon * fabs(V))); }
void SMathResult::AdjustErr(double mult) { E += mult * SMathConst::Epsilon * fabs(V); }
void SMathResult::SetZero() { V = E = 0.0; }

int SMathResult::SetDomainViolation()
{
	V = E = fgetnan();
	return (SLibError = SLERR_MATH_DOMAIN, 0);
}

int SMathResult::SetOverflow()
{
	V = E = fgetposinf();
	return (SLibError = SLERR_MATH_OVERFLOW, 0);
}

int SMathResult::SetUnderflow()
{
	V = 0.0;
	E = SMathConst::Min;
	return (SLibError = SLERR_MATH_UNDERFLOW, 0);
}
//
// }
//
static double _fdiv(double x, double y) { return x / y; }
double fgetnan()    { return _fdiv(0.0, 0.0); }
double fgetposinf() { return _fdiv(+1.0, 0.0); }
double fgetneginf() { return _fdiv(-1.0, 0.0); }

int    FASTCALL smax(int a, int b)       { return MAX(a, b); }
int    FASTCALL smax(uint a, uint b)     { return MAX(a, b); }
int    FASTCALL smin(int a, int b)       { return MIN(a, b); }
int    FASTCALL smin(uint a, uint b)     { return MIN(a, b); }
double FASTCALL smax(double a, double b) { return MAX(a, b); }
double FASTCALL smin(double a, double b) { return MIN(a, b); }
float  FASTCALL smax(float a, float b)   { return MAX(a, b); }
float  FASTCALL smin(float a, float b)   { return MIN(a, b); }

int    FASTCALL sclamp(int v, int lo, int up)          { return (v < lo) ? lo : ((v > up) ? up : v); }
uint   FASTCALL sclamp(uint v, uint lo, uint up)       { return (v < lo) ? lo : ((v > up) ? up : v); }
double FASTCALL sclamp(double v, double lo, double up) { return (v < lo) ? lo : ((v > up) ? up : v); }
float  FASTCALL sclamp(float v, float lo, float up)    { return (v < lo) ? lo : ((v > up) ? up : v); }

double FASTCALL fdiv100r(double v)    { return v / 100.0; }
double FASTCALL fdiv100i(long v)      { return (static_cast<double>(v)) / 100.0; }
double FASTCALL fdiv1000i(long v)     { return (static_cast<double>(v)) / 1000.0; }
double FASTCALL fdivi(long a, long b) { return b ? (static_cast<double>(a) / static_cast<double>(b)) : 0.0; }
double FASTCALL fdivui(uint a, uint b) { return b ? (static_cast<double>(a) / static_cast<double>(b)) : 0.0; }
long   FASTCALL fmul100i(double v)   { return R0i(v * 100.0); }
long   FASTCALL fmul1000i(double v)   { return R0i(v * 1000.0); }
double FASTCALL fdivnz(double dd, double dr) { return (dr != 0.0) ? (dd / dr) : 0.0; }
int    FASTCALL feqeps(double v1, double v2, double epsilon) { return BIN(fabs(v1-v2) < epsilon); }

double FASTCALL fint(double v)
{
	double ip;
	modf(v, &ip);
	return ip;
}

double FASTCALL ffrac(double v)
{
	double ip;
	return modf(v, &ip);
}

double fgetwsign(double val, int sign) { return (sign > 0) ? val : ((sign < 0) ? -val : 0.0); }
double faddwsign(double val, double addendum, int sign) { return (sign > 0) ? (val + addendum) : ((sign < 0) ? (val - addendum) : val); }

float  fdotproduct(uint count, const float * pV1, const float * pV2)
{
	float result = 0.0f;
	for(uint i = 0; i < count; i++) {
		result += (pV1[i] * pV2[i]);
	}
	return result;
}

double fdotproduct(uint count, const double * pV1, const double * pV2)
{
	double result = 0.0;
	const  uint org_count = count;
	while(count) {
		uint p = org_count - count;
		switch(count) {
			case 1:
				result += (pV1[p] * pV2[p]);
				count--;
				break;
			case 2:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]);
				count -= 2;
				break;
			case 3:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]) + (pV1[p+2] * pV2[p+2]);
				count -= 3;
				break;
			case 4:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]) + (pV1[p+2] * pV2[p+2]) + (pV1[p+3] * pV2[p+3]);
				count -= 4;
				break;
			case 5:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]) + (pV1[p+2] * pV2[p+2]) + (pV1[p+3] * pV2[p+3]) + 
					(pV1[p+4] * pV2[p+4]);
				count -= 5;
				break;
			case 6:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]) + (pV1[p+2] * pV2[p+2]) + (pV1[p+3] * pV2[p+3]) + 
					(pV1[p+4] * pV2[p+4]) + (pV1[p+5] * pV2[p+5]);
				count -= 6;
				break;
			case 7:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]) + (pV1[p+2] * pV2[p+2]) + (pV1[p+3] * pV2[p+3]) + 
					(pV1[p+4] * pV2[p+4]) + (pV1[p+5] * pV2[p+5]) + (pV1[p+6] * pV2[p+6]);
				count -= 7;
				break;
			default:
				result += (pV1[p] * pV2[p]) + (pV1[p+1] * pV2[p+1]) + (pV1[p+2] * pV2[p+2]) + (pV1[p+3] * pV2[p+3]) 
					+ (pV1[p+4] * pV2[p+4]) + (pV1[p+5] * pV2[p+5]) + (pV1[p+6] * pV2[p+6]) + (pV1[p+7] * pV2[p+7]);
				count -= 8;
				break;
		}
	}
	/*for(uint i = 0; i < count; i++) {
		result += (pV1[i] * pV2[i]);
	}*/
	return result;
}

float  feuclideannorm(uint count, const float * pV)
{
	float result = 0.0f;
	for(uint i = 0; i < count; i++) {
		result += (pV[i] * pV[i]);
	}
	result = sqrtf(result);
	return result;
}

double feuclideannorm(uint count, const double * pV)
{
	double result = 0.0;
	for(uint i = 0; i < count; i++) {
		result += (pV[i] * pV[i]);
	}
	result = sqrt(result);
	return result;
}

float  feuclideandistance(uint count, const float * pV1, const float * pV2)
{
	float result = 0.0f;
	for(uint i = 0; i < count; i++) {
		float d = (pV1[i] - pV2[i]);
		result += (d * d);
	}
	result = sqrtf(result);
	return result;
}

double feuclideandistance(uint count, const double * pV1, const double * pV2)
{
	double result = 0.0;
	const  uint org_count = count;
	while(count) {
		uint p = org_count - count;
		double d[8];
		switch(count) {
			case 1:
				d[0] = pV1[p] - pV2[p];
				result += (d[0] * d[0]);
				count -= 1;
				break;
			case 2:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				result += (d[0] * d[0]) + (d[1] * d[1]);
				count -= 2;
				break;
			case 3:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				d[2] = pV1[p+2] - pV2[p+2];
				result += (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]);
				count -= 3;
				break;
			case 4:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				d[2] = pV1[p+2] - pV2[p+2];
				d[3] = pV1[p+3] - pV2[p+3];
				result += (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]) + (d[3] * d[3]);
				count -= 4;
				break;
			case 5:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				d[2] = pV1[p+2] - pV2[p+2];
				d[3] = pV1[p+3] - pV2[p+3];
				d[4] = pV1[p+4] - pV2[p+4];
				result += (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]) + (d[3] * d[3]) + (d[4] * d[4]);
				count -= 5;
				break;
			case 6:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				d[2] = pV1[p+2] - pV2[p+2];
				d[3] = pV1[p+3] - pV2[p+3];
				d[4] = pV1[p+4] - pV2[p+4];
				d[5] = pV1[p+5] - pV2[p+5];
				result += (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]) + (d[3] * d[3]) + (d[4] * d[4]) + (d[5] * d[5]);
				count -= 6;
				break;
			case 7:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				d[2] = pV1[p+2] - pV2[p+2];
				d[3] = pV1[p+3] - pV2[p+3];
				d[4] = pV1[p+4] - pV2[p+4];
				d[5] = pV1[p+5] - pV2[p+5];
				d[6] = pV1[p+6] - pV2[p+6];
				result += (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]) + (d[3] * d[3]) + (d[4] * d[4]) + (d[5] * d[5]) + (d[6] * d[6]);
				count -= 7;
				break;
			default:
				d[0] = pV1[p] - pV2[p];
				d[1] = pV1[p+1] - pV2[p+1];
				d[2] = pV1[p+2] - pV2[p+2];
				d[3] = pV1[p+3] - pV2[p+3];
				d[4] = pV1[p+4] - pV2[p+4];
				d[5] = pV1[p+5] - pV2[p+5];
				d[6] = pV1[p+6] - pV2[p+6];
				d[7] = pV1[p+7] - pV2[p+7];
				result += (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]) + (d[3] * d[3]) + (d[4] * d[4]) + (d[5] * d[5]) + (d[6] * d[6]) + (d[7] * d[7]);
				count -= 8;
				break;
		}
	}
	/*for(uint i = 0; i < count; i++) {
		double d = pV1[i] - pV2[i];
		result += (d * d);
	}*/
	result = sqrt(result);
	return result;
}

uint64 SLAPI ui64pow(uint64 x, uint n)
{
	uint64 result = 1;
	if(n) {
		if(n == 1)
			result = x;
		else {
			do {
				if(n & 1)
					result *= x;
				n >>= 1;
				if(n)
					x *= x;
			} while(n);
		}
	}
	return result;
}

double FASTCALL fpowi(double x, int n)
{
	if(n == 0)
		return 1.0;
	else {
		//
		// Ничего умнее, чем продублировать код для разных знаков степени я не придумал.
		// Остальные варианты работать будут (по моему мнению) медленнее.
		//
		if(n > 0) {
			if(n == 1)
				return x;
			else {
				double value = 1.0;
				do {
					if(n & 1)
						value *= x;
					n >>= 1;
					if(!n)
						return value;
					else
						x *= x;
				} while(1);
			}
		}
		else {
			n = -n;
			if(n == 1)
				return 1.0 / x;
			else {
				double value = 1.0;
				do {
					if(n & 1)
						value *= x;
					n >>= 1;
					if(!n)
						return (1.0 / value);
					else
						x *= x;
				} while(1);
			}
		}
	}
}

float FASTCALL fpowfi(float x, int n)
{
	if(n == 0)
		return 1.0f;
	else {
		//
		// Ничего умнее, чем продублировать код для разных знаков степени я не придумал.
		// Остальные варианты работать будут (по моему мнению) медленнее.
		//
		if(n > 0) {
			if(n == 1)
				return x;
			else {
				float value = 1.0f;
				do {
					if(n & 1)
						value *= x;
					n >>= 1;
					if(!n)
						return value;
					else
						x *= x;
				} while(1);
			}
		}
		else {
			n = -n;
			if(n == 1)
				return 1.0f / x;
			else {
				float value = 1.0f;
				do {
					if(n & 1)
						value *= x;
					n >>= 1;
					if(!n)
						return (1.0f / value);
					else
						x *= x;
				} while(1);
			}
		}
	}
}

uint64 FASTCALL ui64pow10(uint n)
{
	switch(n) {
		case  0: return 1ULL;
		case  1: return 10ULL;
		case  2: return 100ULL;
		case  3: return 1000ULL;
		case  4: return 10000ULL;
		case  5: return 100000ULL;
		case  6: return 1000000ULL;
		case  9: return 1000000000ULL;
		case 12: return 1000000000000ULL;
		case 15: return 1000000000000000ULL;
		case 18: return 1000000000000000000ULL;
		default: return ui64pow(10, n);
	}
}

double FASTCALL fpow10i(int n)
{
	switch(n) {
		case  0: return 1.0;
		case  1: return 10.0;
		case  2: return 100.0;
		case  3: return 1000.0;
		case  4: return 10000.0;
		case  5: return 100000.0;
		case  6: return 1000000.0;
		case -1: return 0.1;
		case -2: return 0.01;
		case -3: return 0.001;
		case -4: return 0.0001;
		case -5: return 0.00001;
		case -6: return 0.000001;
		default: return fpowi(10., n);
	}
}

float  FASTCALL fpow10fi(int n)
{
	switch(n) {
		case  0: return 1.0f;
		case  1: return 10.0f;
		case  2: return 100.0f;
		case  3: return 1000.0f;
		case  4: return 10000.0f;
		case  5: return 100000.0f;
		case  6: return 1000000.0f;
		case -1: return 0.1f;
		case -2: return 0.01f;
		case -3: return 0.001f;
		case -4: return 0.0001f;
		case -5: return 0.00001f;
		case -6: return 0.000001f;
		default: return fpowfi(10.0f, n);
	}
}

double FASTCALL ffactr(uint i)
{
	static double * p_fact_tab = 0;
	if(p_fact_tab == 0) {
		ENTER_CRITICAL_SECTION
			if(p_fact_tab == 0) {
				p_fact_tab = static_cast<double *>(SAlloc::C(FACT_TAB_SIZE, sizeof(*p_fact_tab)));
				p_fact_tab[0] = p_fact_tab[1] = 1.;
				double prev = 1.;
				for(uint k = 2; k < FACT_TAB_SIZE; k++) {
					prev *= k;
					p_fact_tab[k] = prev;
				}
			}
		LEAVE_CRITICAL_SECTION
	}
	return (i < FACT_TAB_SIZE) ? p_fact_tab[i] : 0.0;
}

double FASTCALL degtorad(double deg) { return (SMathConst::PiDiv180 * deg); }
float  FASTCALL degtorad(float deg) { return ((float)SMathConst::PiDiv180 * deg); }
double FASTCALL degtorad(int deg) { return (SMathConst::PiDiv180 * deg); }
double FASTCALL radtodeg(double rad) { return (rad / SMathConst::PiDiv180); }
double fscale(double v, double low, double upp) { return (v - low) / (upp - low); }
double SLAPI sigmoid(double a, double x) { return (1.0 / (1.0 + exp(-(a * x)))); }
//
//
//
int flnfact(uint n, SMathResult * pResult)
{
	/* CHECK_POINTER(result) */
	if(n < FACT_TAB_SIZE){
		pResult->V = log(ffactr(n));
		pResult->E = 2.0 * SMathConst::Epsilon * fabs(pResult->V);
	}
	else
		flngamma(n+1.0, pResult);
	return 1;
}

double lnfact(uint n)
{
	SMathResult lf;
	flnfact(n, &lf);
	return lf.V;
}
//
// Descr: Calculate Pearson correlation = cov(X, Y) / (sigma_X * sigma_Y)
//   This routine efficiently computes the correlation in one pass of the
//   data and makes use of the algorithm described in:
//
// B. P. Welford, "Note on a Method for Calculating Corrected Sums of
// Squares and Products", Technometrics, Vol 4, No 3, 1962.
//
// This paper derives a numerically stable recurrence to compute a sum of products
//
// S = sum_{i=1..N} [ (x_i - mu_x) * (y_i - mu_y) ]
//
// with the relation
//
// S_n = S_{n-1} + ((n-1)/n) * (x_n - mu_x_{n-1}) * (y_n - mu_y_{n-1})
//
double SLAPI scorrelation(const double * pData1, const double * pData2, const size_t n)
{
	double result = 0.0;
	if(pData1 && pData2 && n > 1) {
		double sum_xsq = 0.0;
		double sum_ysq = 0.0;
		double sum_cross = 0.0;
		//
		// Compute:
		// sum_xsq = Sum [ (x_i - mu_x)^2 ],
		// sum_ysq = Sum [ (y_i - mu_y)^2 ] and
		// sum_cross = Sum [ (x_i - mu_x) * (y_i - mu_y) ]
		// using the above relation from Welford's paper
		//
		double mean_x = pData1[0];
		double mean_y = pData2[0];
		for(size_t i = 1; i < n; i++) {
			const double ratio = i / (i + 1.0);
			const double delta_x = pData1[i] - mean_x;
			const double delta_y = pData2[i] - mean_y;
			sum_xsq += delta_x * delta_x * ratio;
			sum_ysq += delta_y * delta_y * ratio;
			sum_cross += delta_x * delta_y * ratio;
			mean_x += delta_x / (i + 1.0);
			mean_y += delta_y / (i + 1.0);
		}
		result = sum_cross / (sqrt(sum_xsq) * sqrt(sum_ysq));
	}
	return result;
}

//
//
//
SLAPI SHistogram::SHistogram() : Flags(0), LeftEdge(0.0), Step(0.0), P_Stat(0), DevMean(0.0), DevWidth(0.0)
{
}

SLAPI SHistogram::~SHistogram()
{
	delete P_Stat;
}

void SLAPI SHistogram::Setup()
{
	BinList.freeAll();
	ValList.freeAll();
	Flags = 0;
	LeftEdge = 0.0;
	Step = 0.0;
	DevWidthSigm = 0.0;
	DevBinCount = 0;
	ZDELETE(P_Stat);
	DevMean = 0.0;
	DevWidth = 0.0;
}

void SLAPI SHistogram::SetupDynamic(double leftEdge, double step)
{
	Setup();
	Flags |= fDynBins;
	LeftEdge = leftEdge;
	Step = step;
}

void SLAPI SHistogram::SetupDev(int even, double widthSigm, uint binCount)
{
	Setup();
	Flags |= fDeviation;
	P_Stat = new StatBase(0);
	// @v10.2.10 P_Stat->Init(0);
	if(even)
		Flags |= fEven;
	DevWidthSigm = widthSigm;
	DevBinCount = binCount;
}

int SLAPI SHistogram::AddBin(long binId, double lowBound)
{
	int    ok = 1;
	uint   pos = 0;
	if(BinList.Search(binId, 0, &pos, 0)) {
		BinList.at(pos).Val = lowBound;
		ok = 2;
	}
	else
		BinList.Add(binId, lowBound, 0, 0);
	BinList.SortByVal();
	return ok;
}

int SLAPI SHistogram::GetBin(long binId, Val * pVal) const
{
	uint   pos = 0;
	if(ValList.bsearch(&binId, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pVal, ValList.at(pos));
		return 1;
	}
	else
		return 0;
}

int SLAPI SHistogram::GetBinByVal(double val, long * pBinId) const
{
	int    r = 0;
	uint   c = BinList.getCount();
	long   val_id;
	for(uint i = 0; i < c; i++)
		if(val < BinList.at(i).Val) {
			val_id = (i == 0) ? -MAXLONG : BinList.at(i-1).Key;
			r = 1;
			break;
		}
	if(r == 0) {
		val_id = c ? BinList.at(c-1).Key : -MAXLONG;
		r = 2;
	}
	ASSIGN_PTR(pBinId, val_id);
	return r;
}

const StatBase * SLAPI SHistogram::GetDeviationStat() const
{
	return (Flags & fDeviation) ? P_Stat : 0;
}

int SLAPI SHistogram::GetDeviationParams(double * pMean, double * pWidth) const
{
	if(Flags & fDeviation) {
		ASSIGN_PTR(pMean, DevMean);
		ASSIGN_PTR(pWidth, DevWidth);
		return 1;
	}
	else {
		ASSIGN_PTR(pMean, 0.0);
		ASSIGN_PTR(pWidth, 0.0);
		return 0;
	}
}

int SLAPI SHistogram::PreparePut(double val)
{
	int    ok = 1;
	assert(Flags & fDeviation);
	assert(P_Stat);
	P_Stat->Step(val); //StatBase
	return ok;
}

long SLAPI SHistogram::Put(double val)
{
	int    r = 0;
	uint   i;
	long   val_id;
	uint   c = BinList.getCount();
	if(Flags & fDynBins) {
		double rem = fmod(val, Step);
		long   bin_no;
		if(val < LeftEdge)
			bin_no = -1;
		else {
			bin_no = static_cast<long>((val - LeftEdge) / Step) + 1;
			uint   pos = 0;
			if(!BinList.Search(bin_no, 0, &pos, 0)) {
				long   last_bin = c ? BinList.at(c-1).Key : 0;
				for(long j = last_bin+1; j <= bin_no; j++) {
					AddBin(j, LeftEdge + Step * (j-1));
				}
				c = BinList.getCount();
			}
		}
	}
	else if(Flags & fDeviation) {
		THROW(P_Stat); // StatBase
		if(c == 0) {
			THROW(P_Stat->GetCount());
			THROW(P_Stat->Finish());
			THROW(DevBinCount);
			THROW(DevWidthSigm > 0.0);
			DevMean  = P_Stat->GetExp();
			DevWidth = P_Stat->GetStdDev() * DevWidthSigm;
			if(Flags & fEven) {
				uint hc = ((DevBinCount / 2) * 2) / 2;
				for(i = 0; i < hc; i++) {
					AddBin(i+1,    DevMean - DevWidth * (i+1));
					AddBin(i+1+hc, DevMean + DevWidth * i);
				}
				//
				// Добавляем одну корзину, которая заберет на себя все значения,
				// превышающие avg + width * DevBinCount / 2.
				// Значения, которые ниже (avg - width * DevBinCount / 2) возмет на себя //
				// корзина по умолчанию (-MAXLONG).
				//
				AddBin(MAXLONG, DevMean + DevWidth * hc);
			}
			else {
				uint hc = (((DevBinCount - 1) / 2) * 2) / 2;
				//
				// Средняя корзина
				//
				AddBin(hc+1, DevMean - DevWidth / 2);
				//
				for(i = 0; i < hc; i++) {
					AddBin(i+1,    DevMean - DevWidth / 2.0 - DevWidth * (i+1));
					AddBin(i+2+hc, DevMean + DevWidth / 2.0 + DevWidth * i);
				}
				//
				// См. примечание выше.
				//
				AddBin(MAXLONG, DevMean + DevWidth / 2.0 + DevWidth * hc);
			}
			c = BinList.getCount();
#ifndef NDEBUG
			//
			// Отладочная проверка на равенство всех диапазонов
			//
			if(c > 2) {
				double wd = BinList.at(1).Val - BinList.at(0).Val;
				double prev = BinList.at(1).Val;
				const double eps = 1e-10;
				for(i = 2; i < c; i++) {
					double val = BinList.at(i).Val;
					assert((wd - DevWidth) <= eps);
					assert(fabs(val - prev - wd) <= eps);
					wd = val - prev;
					prev = val;
				}
			}
#endif
		}
	}
	for(i = 0; i < c; i++)
		if(val < BinList.at(i).Val) {
			val_id = (i == 0) ? -MAXLONG : BinList.at(i-1).Key;
			r = 1;
			break;
		}
	if(r == 0)
		val_id = c ? BinList.at(c-1).Key : -MAXLONG;
	if(ValList.bsearch(&val_id, &(c = 0), CMPF_LONG)) {
		SHistogram::Val & r_val = ValList.at(c);
		r_val.Count++;
		r_val.Sum += val;
	}
	else {
		SHistogram::Val new_val;
		MEMSZERO(new_val);
		new_val.Id = val_id;
		new_val.Count = 1;
		new_val.Sum = val;
		ValList.ordInsert(&new_val, 0, CMPF_LONG);
	}
	CATCH
		val_id = 0;
	ENDCATCH
	return val_id;
}

int SLAPI SHistogram::GetTotal(Val * pVal) const
{
	int    ok = -1;
	Val t;
	MEMSZERO(t);
	const uint c = ValList.getCount();
	if(c) {
		for(uint i = 0; i < c; i++) {
			const SHistogram::Val & r_val = ValList.at(c);
			t.Count += r_val.Count;
			t.Sum += r_val.Sum;
		}
		ok = 1;
	}
	return ok;
}

uint SLAPI SHistogram::GetResultCount() const
{
	return ValList.getCount();
}

int  SLAPI SHistogram::GetResult(uint pos, Result * pResult) const
{
	int    ok = 1;
	if(pos < ValList.getCount()) {
		if(pResult) {
			const Val & r_val = ValList.at(pos);
			pResult->Id = r_val.Id;
			pResult->Count = r_val.Count;
			pResult->Sum = r_val.Sum;

			const uint bin_count = BinList.getCount();
			double bin_low = 0.0;
			uint bin_pos = 0;
			if(BinList.Search(r_val.Id, &bin_low, &bin_pos, 0)) {
				pResult->Low = bin_low;
				pResult->Upp = (bin_pos < (bin_count-1)) ? BinList.at(bin_pos+1).Val : SMathConst::Max;
			}
			else if(r_val.Id == -MAXLONG) {
				pResult->Low = -SMathConst::Max;
				pResult->Upp = bin_count ? BinList.at(0).Val : SMathConst::Max;
			}
			else if(r_val.Id == MAXLONG) {
				pResult->Upp = SMathConst::Max;
				pResult->Low = bin_count ? BinList.at(bin_count-1).Val : -SMathConst::Max;
			}
			else {
				pResult->Low = -SMathConst::Max;
				pResult->Upp = SMathConst::Max;
			}
		}
	}
	else
		ok = 0;
	return ok;
}
//
//
//
static const double _mizer = 1.0E-8; //0.00000001

static FORCEINLINE double implement_round(double n, int prec)
{
	const  int sign = (fsign(n) - 1);
	if(sign)
		n = _chgsign(n);
	if(prec == 0) {
		const double f = floor(n);
		n = ((n - f + _mizer) < 0.5) ? f : ceil(n);
	}
	else {
		const double p = fpow10i(prec);
		const double t = n * p;
		const double f = floor(t);
		n = (((t - f + _mizer) < 0.5) ? f : ceil(t)) / p;
	}
	return sign ? _chgsign(n) : n;
}

double FASTCALL round(double n, int prec)
{
	return implement_round(n, prec);
	/*
	static const double _mizer = 1.0E-8; //0.00000001
	double p;
	int    sign = (fsign(n) - 1);
	if(sign)
		n = _chgsign(n);
	if(prec == 0) {
		p = floor(n);
		n = ((n - p + _mizer) < 0.5) ? p : ceil(n);
	}
	else {
		p = fpow10i(prec);
		double t = n * p;
		double f = floor(t);
		n = (((t - f + _mizer) < 0.5) ? f : ceil(t)) / p;
	}
	return sign ? _chgsign(n) : n;
	*/
}

double FASTCALL round(double v, double prec, int dir)
{
	SETIFZ(prec, 0.01);
	double r = (v / prec);
	const double f_ = floor(r);
	const double c_ = ceil(r);
	/*
	Экспериментальный участок кода: надежда решить проблему мизерного отклонения от требуемого значения.
	В строй не вводим из-за риска напороться на неожиданные эффекты.
	if(feqeps(r, c_, 1E-10))
		return prec * c_;
	else if(feqeps(r, f_, 1E-10))
		return prec * f_;
	else
	*/
	if(dir > 0) {
		return prec * c_;
	}
	else if(dir < 0) {
		return prec * f_;
	}
	else {
		const double down = prec * f_;
		const double up   = prec * c_;
		return ((2.0 * v - down - up) >= 0) ? up : down;
	}
}

double FASTCALL R0(double v)  { return implement_round(v, 0); }
long   FASTCALL R0i(double v) { return static_cast<long>(implement_round(v, 0)); }
double FASTCALL R2(double v)  { return implement_round(v, 2); }
double FASTCALL R3(double v)  { return implement_round(v, 3); }
double FASTCALL R4(double v)  { return implement_round(v, 4); }
double FASTCALL R5(double v)  { return implement_round(v, 5); }
double FASTCALL R6(double v)  { return implement_round(v, 6); }

double FASTCALL roundnev(double n, int prec)
{
	if(prec == 0) {
		const double p = floor(n);
		const double diff = n - p;
		if(diff < 0.5)
			n = p;
		else if(diff > 0.5)
			n = ceil(n);
		else // diff == 0.5
			if(!(static_cast<long>(p) & 1))
				n = p;
			else
				n = ceil(n);
	}
	else {
		const double p = fpow10i(prec);
		const double t = n * p;
		const double f = floor(t);
		const double diff = t - f;
		if(diff < 0.5)
			n = f / p;
		else if(diff > 0.5)
			n = (ceil(t) / p);
		else { // diff == 0.5
			if(!(static_cast<long>(f) & 1))
				n = f / p;
			else
				n = (ceil(t) / p);
		}
	}
	return n;
}

double SLAPI trunc(double n, int prec)
{
	double p = fpow10i(prec);
	double f = floor(n * p);
	return f / p;
}
//
// Note about next 4 functions:
// Функции intmnytodbl и dbltointmny идентичны соответственно inttodbl2 и dbltoint2
// Исторически, intmnytodbl и dbltointmny появились раньше, давно и успешно работают.
// Вполне возможно, что реализацию можно уточнить - тогда начинать следует с inttodbl2 и dbltoint2
// так как они реже используются.
//
double FASTCALL inttodbl2(long v) { return implement_round(fdiv100i(v), 2); }
long   FASTCALL dbltoint2(double r) { return static_cast<long>(implement_round(r * 100.0, 0)); }
double FASTCALL intmnytodbl(long m) { return implement_round(fdiv100i(m), 2); }
long   FASTCALL dbltointmny(double r) { return static_cast<long>(implement_round(r * 100.0, 0)); }
//
//
//
uint FASTCALL sshrinkuint64(uint64 value, void * pBuf)
{
	uint   size = 0;
    if(value == 0) {
		PTR8(pBuf)[0] = 0;
		size = 1;
    }
    else {
		uint i = 7;
		while(PTR8(&value)[i] == 0) {
			i--;
		}
		size = i+1;
		for(i = 0; i < size; i++) {
			PTR8(pBuf)[i] = PTR8(&value)[i];
		}
    }
    return size;
}

uint64 FASTCALL sexpanduint64(const void * pBuf, uint size)
{
	uint64 result = 0;
	if(size > 1 || PTR8C(pBuf)[0] != 0) {
		for(uint i = 0; i < size; i++) {
			PTR8(&result)[i] = PTR8C(pBuf)[i];
		}
	}
	return result;
}
//
// Descr: Вычисляет наибольший общий делитель (GCD) a и b
//
ulong  FASTCALL Gcd(ulong a, ulong b)
{
	ulong result = 0;
	if(b == 0 || a == b)
		result = a;
	else if(a == 0)
		result = b;
	else if(a > b) {
		for(ulong rem = a%b; rem > 0; rem = a%b) {
			a = b;
			b = rem;
		}
		result = b;
	}
	else if(a < b) {
		for(ulong rem = b%a; rem > 0; rem = b%a) {
			b = a;
			a = rem;
		}
		result = a;
	}
	assert((a % result) == 0 && (b % result) == 0 && (result <= a) && (result <= b));
	return result;
}
//
// Descr: Вычисляет наименьшее общее кратное (LCM) a и b
//
ulong  FASTCALL Lcm(ulong a, ulong b)
{
	const ulong  _gcd = Gcd(a, b);
    const ulong result = _gcd ? (a / _gcd * b) : 0;
    assert((result % a) == 0 && (result % b) == 0);
    return result;
}

static const ushort FirstPrimeNumbers[] = {
        2,     3,     5,     7,    11,    13,    17,    19,    23,    29,    31,    37,    41,    43,    47,    53,
       59,    61,    67,    71,    73,    79,    83,    89,    97,   101,   103,   107,   109,   113,   127,   131,
      137,   139,   149,   151,   157,   163,   167,   173,   179,   181,   191,   193,   197,   199,   211,   223,
      227,   229,   233,   239,   241,   251,   257,   263,   269,   271,   277,   281,   283,   293,   307,   311,
      313,   317,   331,   337,   347,   349,   353,   359,   367,   373,   379,   383,   389,   397,   401,   409,
      419,   421,   431,   433,   439,   443,   449,   457,   461,   463,   467,   479,   487,   491,   499,   503,
      509,   521,   523,   541,   547,   557,   563,   569,   571,   577,   587,   593,   599,   601,   607,   613,
      617,   619,   631,   641,   643,   647,   653,   659,   661,   673,   677,   683,   691,   701,   709,   719,
      727,   733,   739,   743,   751,   757,   761,   769,   773,   787,   797,   809,   811,   821,   823,   827,
      829,   839,   853,   857,   859,   863,   877,   881,   883,   887,   907,   911,   919,   929,   937,   941,
      947,   953,   967,   971,   977,   983,   991,   997,  1009,  1013,  1019,  1021,  1031,  1033,  1039,  1049,
     1051,  1061,  1063,  1069,  1087,  1091,  1093,  1097,  1103,  1109,  1117,  1123,  1129,  1151,  1153,  1163,
     1171,  1181,  1187,  1193,  1201,  1213,  1217,  1223,  1229,  1231,  1237,  1249,  1259,  1277,  1279,  1283,
     1289,  1291,  1297,  1301,  1303,  1307,  1319,  1321,  1327,  1361,  1367,  1373,  1381,  1399,  1409,  1423,
     1427,  1429,  1433,  1439,  1447,  1451,  1453,  1459,  1471,  1481,  1483,  1487,  1489,  1493,  1499,  1511,
     1523,  1531,  1543,  1549,  1553,  1559,  1567,  1571,  1579,  1583,  1597,  1601,  1607,  1609,  1613,  1619,
     1621,  1627,  1637,  1657,  1663,  1667,  1669,  1693,  1697,  1699,  1709,  1721,  1723,  1733,  1741,  1747,
     1753,  1759,  1777,  1783,  1787,  1789,  1801,  1811,  1823,  1831,  1847,  1861,  1867,  1871,  1873,  1877,
     1879,  1889,  1901,  1907,  1913,  1931,  1933,  1949,  1951,  1973,  1979,  1987,  1993,  1997,  1999,  2003,
     2011,  2017,  2027,  2029,  2039,  2053,  2063,  2069,  2081,  2083,  2087,  2089,  2099,  2111,  2113,  2129,
     2131,  2137,  2141,  2143,  2153,  2161,  2179,  2203,  2207,  2213,  2221,  2237,  2239,  2243,  2251,  2267,
     2269,  2273,  2281,  2287,  2293,  2297,  2309,  2311,  2333,  2339,  2341,  2347,  2351,  2357,  2371,  2377,
     2381,  2383,  2389,  2393,  2399,  2411,  2417,  2423,  2437,  2441,  2447,  2459,  2467,  2473,  2477,  2503,
     2521,  2531,  2539,  2543,  2549,  2551,  2557,  2579,  2591,  2593,  2609,  2617,  2621,  2633,  2647,  2657,
     2659,  2663,  2671,  2677,  2683,  2687,  2689,  2693,  2699,  2707,  2711,  2713,  2719,  2729,  2731,  2741,
     2749,  2753,  2767,  2777,  2789,  2791,  2797,  2801,  2803,  2819,  2833,  2837,  2843,  2851,  2857,  2861,
     2879,  2887,  2897,  2903,  2909,  2917,  2927,  2939,  2953,  2957,  2963,  2969,  2971,  2999,  3001,  3011,
     3019,  3023,  3037,  3041,  3049,  3061,  3067,  3079,  3083,  3089,  3109,  3119,  3121,  3137,  3163,  3167,
     3169,  3181,  3187,  3191,  3203,  3209,  3217,  3221,  3229,  3251,  3253,  3257,  3259,  3271,  3299,  3301,
     3307,  3313,  3319,  3323,  3329,  3331,  3343,  3347,  3359,  3361,  3371,  3373,  3389,  3391,  3407,  3413,
     3433,  3449,  3457,  3461,  3463,  3467,  3469,  3491,  3499,  3511,  3517,  3527,  3529,  3533,  3539,  3541,
     3547,  3557,  3559,  3571,  3581,  3583,  3593,  3607,  3613,  3617,  3623,  3631,  3637,  3643,  3659,  3671,
     3673,  3677,  3691,  3697,  3701,  3709,  3719,  3727,  3733,  3739,  3761,  3767,  3769,  3779,  3793,  3797,
     3803,  3821,  3823,  3833,  3847,  3851,  3853,  3863,  3877,  3881,  3889,  3907,  3911,  3917,  3919,  3923,
     3929,  3931,  3943,  3947,  3967,  3989,  4001,  4003,  4007,  4013,  4019,  4021,  4027,  4049,  4051,  4057,
     4073,  4079,  4091,  4093,  4099,  4111,  4127,  4129,  4133,  4139,  4153,  4157,  4159,  4177,  4201,  4211,
     4217,  4219,  4229,  4231,  4241,  4243,  4253,  4259,  4261,  4271,  4273,  4283,  4289,  4297,  4327,  4337,
     4339,  4349,  4357,  4363,  4373,  4391,  4397,  4409,  4421,  4423,  4441,  4447,  4451,  4457,  4463,  4481,
     4483,  4493,  4507,  4513,  4517,  4519,  4523,  4547,  4549,  4561,  4567,  4583,  4591,  4597,  4603,  4621,
     4637,  4639,  4643,  4649,  4651,  4657,  4663,  4673,  4679,  4691,  4703,  4721,  4723,  4729,  4733,  4751,
     4759,  4783,  4787,  4789,  4793,  4799,  4801,  4813,  4817,  4831,  4861,  4871,  4877,  4889,  4903,  4909,
     4919,  4931,  4933,  4937,  4943,  4951,  4957,  4967,  4969,  4973,  4987,  4993,  4999,  5003,  5009,  5011,
     5021,  5023,  5039,  5051,  5059,  5077,  5081,  5087,  5099,  5101,  5107,  5113,  5119,  5147,  5153,  5167,
     5171,  5179,  5189,  5197,  5209,  5227,  5231,  5233,  5237,  5261,  5273,  5279,  5281,  5297,  5303,  5309,
     5323,  5333,  5347,  5351,  5381,  5387,  5393,  5399,  5407,  5413,  5417,  5419,  5431,  5437,  5441,  5443,
     5449,  5471,  5477,  5479,  5483,  5501,  5503,  5507,  5519,  5521,  5527,  5531,  5557,  5563,  5569,  5573,
     5581,  5591,  5623,  5639,  5641,  5647,  5651,  5653,  5657,  5659,  5669,  5683,  5689,  5693,  5701,  5711,
     5717,  5737,  5741,  5743,  5749,  5779,  5783,  5791,  5801,  5807,  5813,  5821,  5827,  5839,  5843,  5849,
     5851,  5857,  5861,  5867,  5869,  5879,  5881,  5897,  5903,  5923,  5927,  5939,  5953,  5981,  5987,  6007,
     6011,  6029,  6037,  6043,  6047,  6053,  6067,  6073,  6079,  6089,  6091,  6101,  6113,  6121,  6131,  6133,
     6143,  6151,  6163,  6173,  6197,  6199,  6203,  6211,  6217,  6221,  6229,  6247,  6257,  6263,  6269,  6271,
     6277,  6287,  6299,  6301,  6311,  6317,  6323,  6329,  6337,  6343,  6353,  6359,  6361,  6367,  6373,  6379,
     6389,  6397,  6421,  6427,  6449,  6451,  6469,  6473,  6481,  6491,  6521,  6529,  6547,  6551,  6553,  6563,
     6569,  6571,  6577,  6581,  6599,  6607,  6619,  6637,  6653,  6659,  6661,  6673,  6679,  6689,  6691,  6701,
     6703,  6709,  6719,  6733,  6737,  6761,  6763,  6779,  6781,  6791,  6793,  6803,  6823,  6827,  6829,  6833,
     6841,  6857,  6863,  6869,  6871,  6883,  6899,  6907,  6911,  6917,  6947,  6949,  6959,  6961,  6967,  6971,
     6977,  6983,  6991,  6997,  7001,  7013,  7019,  7027,  7039,  7043,  7057,  7069,  7079,  7103,  7109,  7121,
     7127,  7129,  7151,  7159,  7177,  7187,  7193,  7207,  7211,  7213,  7219,  7229,  7237,  7243,  7247,  7253,
     7283,  7297,  7307,  7309,  7321,  7331,  7333,  7349,  7351,  7369,  7393,  7411,  7417,  7433,  7451,  7457,
     7459,  7477,  7481,  7487,  7489,  7499,  7507,  7517,  7523,  7529,  7537,  7541,  7547,  7549,  7559,  7561,
     7573,  7577,  7583,  7589,  7591,  7603,  7607,  7621,  7639,  7643,  7649,  7669,  7673,  7681,  7687,  7691,
     7699,  7703,  7717,  7723,  7727,  7741,  7753,  7757,  7759,  7789,  7793,  7817,  7823,  7829,  7841,  7853,
     7867,  7873,  7877,  7879,  7883,  7901,  7907,  7919,  7927,  7933,  7937,  7949,  7951,  7963,  7993,  8009,
     8011,  8017,  8039,  8053,  8059,  8069,  8081,  8087,  8089,  8093,  8101,  8111,  8117,  8123,  8147,  8161,
     8167,  8171,  8179,  8191,  8209,  8219,  8221,  8231,  8233,  8237,  8243,  8263,  8269,  8273,  8287,  8291,
     8293,  8297,  8311,  8317,  8329,  8353,  8363,  8369,  8377,  8387,  8389,  8419,  8423,  8429,  8431,  8443,
     8447,  8461,  8467,  8501,  8513,  8521,  8527,  8537,  8539,  8543,  8563,  8573,  8581,  8597,  8599,  8609,
     8623,  8627,  8629,  8641,  8647,  8663,  8669,  8677,  8681,  8689,  8693,  8699,  8707,  8713,  8719,  8731,
     8737,  8741,  8747,  8753,  8761,  8779,  8783,  8803,  8807,  8819,  8821,  8831,  8837,  8839,  8849,  8861,
     8863,  8867,  8887,  8893,  8923,  8929,  8933,  8941,  8951,  8963,  8969,  8971,  8999,  9001,  9007,  9011,
     9013,  9029,  9041,  9043,  9049,  9059,  9067,  9091,  9103,  9109,  9127,  9133,  9137,  9151,  9157,  9161,
     9173,  9181,  9187,  9199,  9203,  9209,  9221,  9227,  9239,  9241,  9257,  9277,  9281,  9283,  9293,  9311,
     9319,  9323,  9337,  9341,  9343,  9349,  9371,  9377,  9391,  9397,  9403,  9413,  9419,  9421,  9431,  9433,
     9437,  9439,  9461,  9463,  9467,  9473,  9479,  9491,  9497,  9511,  9521,  9533,  9539,  9547,  9551,  9587,
     9601,  9613,  9619,  9623,  9629,  9631,  9643,  9649,  9661,  9677,  9679,  9689,  9697,  9719,  9721,  9733,
     9739,  9743,  9749,  9767,  9769,  9781,  9787,  9791,  9803,  9811,  9817,  9829,  9833,  9839,  9851,  9857,
     9859,  9871,  9883,  9887,  9901,  9907,  9923,  9929,  9931,  9941,  9949,  9967,  9973, 10007, 10009, 10037,
    10039, 10061, 10067, 10069, 10079, 10091, 10093, 10099, 10103, 10111, 10133, 10139, 10141, 10151, 10159, 10163,
    10169, 10177, 10181, 10193, 10211, 10223, 10243, 10247, 10253, 10259, 10267, 10271, 10273, 10289, 10301, 10303,
    10313, 10321, 10331, 10333, 10337, 10343, 10357, 10369, 10391, 10399, 10427, 10429, 10433, 10453, 10457, 10459,
    10463, 10477, 10487, 10499, 10501, 10513, 10529, 10531, 10559, 10567, 10589, 10597, 10601, 10607, 10613, 10627,
    10631, 10639, 10651, 10657, 10663, 10667, 10687, 10691, 10709, 10711, 10723, 10729, 10733, 10739, 10753, 10771,
    10781, 10789, 10799, 10831, 10837, 10847, 10853, 10859, 10861, 10867, 10883, 10889, 10891, 10903, 10909, 10937,
    10939, 10949, 10957, 10973, 10979, 10987, 10993, 11003, 11027, 11047, 11057, 11059, 11069, 11071, 11083, 11087,
    11093, 11113, 11117, 11119, 11131, 11149, 11159, 11161, 11171, 11173, 11177, 11197, 11213, 11239, 11243, 11251,
    11257, 11261, 11273, 11279, 11287, 11299, 11311, 11317, 11321, 11329, 11351, 11353, 11369, 11383, 11393, 11399,
    11411, 11423, 11437, 11443, 11447, 11467, 11471, 11483, 11489, 11491, 11497, 11503, 11519, 11527, 11549, 11551,
    11579, 11587, 11593, 11597, 11617, 11621, 11633, 11657, 11677, 11681, 11689, 11699, 11701, 11717, 11719, 11731,
    11743, 11777, 11779, 11783, 11789, 11801, 11807, 11813, 11821, 11827, 11831, 11833, 11839, 11863, 11867, 11887,
    11897, 11903, 11909, 11923, 11927, 11933, 11939, 11941, 11953, 11959, 11969, 11971, 11981, 11987, 12007, 12011,
    12037, 12041, 12043, 12049, 12071, 12073, 12097, 12101, 12107, 12109, 12113, 12119, 12143, 12149, 12157, 12161,
    12163, 12197, 12203, 12211, 12227, 12239, 12241, 12251, 12253, 12263, 12269, 12277, 12281, 12289, 12301, 12323,
    12329, 12343, 12347, 12373, 12377, 12379, 12391, 12401, 12409, 12413, 12421, 12433, 12437, 12451, 12457, 12473,
    12479, 12487, 12491, 12497, 12503, 12511, 12517, 12527, 12539, 12541, 12547, 12553, 12569, 12577, 12583, 12589,
    12601, 12611, 12613, 12619, 12637, 12641, 12647, 12653, 12659, 12671, 12689, 12697, 12703, 12713, 12721, 12739,
    12743, 12757, 12763, 12781, 12791, 12799, 12809, 12821, 12823, 12829, 12841, 12853, 12889, 12893, 12899, 12907,
    12911, 12917, 12919, 12923, 12941, 12953, 12959, 12967, 12973, 12979, 12983, 13001, 13003, 13007, 13009, 13033,
    13037, 13043, 13049, 13063, 13093, 13099, 13103, 13109, 13121, 13127, 13147, 13151, 13159, 13163, 13171, 13177,
    13183, 13187, 13217, 13219, 13229, 13241, 13249, 13259, 13267, 13291, 13297, 13309, 13313, 13327, 13331, 13337,
    13339, 13367, 13381, 13397, 13399, 13411, 13417, 13421, 13441, 13451, 13457, 13463, 13469, 13477, 13487, 13499,
    13513, 13523, 13537, 13553, 13567, 13577, 13591, 13597, 13613, 13619, 13627, 13633, 13649, 13669, 13679, 13681,
    13687, 13691, 13693, 13697, 13709, 13711, 13721, 13723, 13729, 13751, 13757, 13759, 13763, 13781, 13789, 13799,
    13807, 13829, 13831, 13841, 13859, 13873, 13877, 13879, 13883, 13901, 13903, 13907, 13913, 13921, 13931, 13933,
    13963, 13967, 13997, 13999, 14009, 14011, 14029, 14033, 14051, 14057, 14071, 14081, 14083, 14087, 14107, 14143,
    14149, 14153, 14159, 14173, 14177, 14197, 14207, 14221, 14243, 14249, 14251, 14281, 14293, 14303, 14321, 14323,
    14327, 14341, 14347, 14369, 14387, 14389, 14401, 14407, 14411, 14419, 14423, 14431, 14437, 14447, 14449, 14461,
    14479, 14489, 14503, 14519, 14533, 14537, 14543, 14549, 14551, 14557, 14561, 14563, 14591, 14593, 14621, 14627,
    14629, 14633, 14639, 14653, 14657, 14669, 14683, 14699, 14713, 14717, 14723, 14731, 14737, 14741, 14747, 14753,
    14759, 14767, 14771, 14779, 14783, 14797, 14813, 14821, 14827, 14831, 14843, 14851, 14867, 14869, 14879, 14887,
    14891, 14897, 14923, 14929, 14939, 14947, 14951, 14957, 14969, 14983, 15013, 15017, 15031, 15053, 15061, 15073,
    15077, 15083, 15091, 15101, 15107, 15121, 15131, 15137, 15139, 15149, 15161, 15173, 15187, 15193, 15199, 15217,
    15227, 15233, 15241, 15259, 15263, 15269, 15271, 15277, 15287, 15289, 15299, 15307, 15313, 15319, 15329, 15331,
    15349, 15359, 15361, 15373, 15377, 15383, 15391, 15401, 15413, 15427, 15439, 15443, 15451, 15461, 15467, 15473,
    15493, 15497, 15511, 15527, 15541, 15551, 15559, 15569, 15581, 15583, 15601, 15607, 15619, 15629, 15641, 15643,
    15647, 15649, 15661, 15667, 15671, 15679, 15683, 15727, 15731, 15733, 15737, 15739, 15749, 15761, 15767, 15773,
    15787, 15791, 15797, 15803, 15809, 15817, 15823, 15859, 15877, 15881, 15887, 15889, 15901, 15907, 15913, 15919,
    15923, 15937, 15959, 15971, 15973, 15991, 16001, 16007, 16033, 16057, 16061, 16063, 16067, 16069, 16073, 16087,
    16091, 16097, 16103, 16111, 16127, 16139, 16141, 16183, 16187, 16189, 16193, 16217, 16223, 16229, 16231, 16249,
    16253, 16267, 16273, 16301, 16319, 16333, 16339, 16349, 16361, 16363, 16369, 16381, 16411, 16417, 16421, 16427,
    16433, 16447, 16451, 16453, 16477, 16481, 16487, 16493, 16519, 16529, 16547, 16553, 16561, 16567, 16573, 16603,
    16607, 16619, 16631, 16633, 16649, 16651, 16657, 16661, 16673, 16691, 16693, 16699, 16703, 16729, 16741, 16747,
    16759, 16763, 16787, 16811, 16823, 16829, 16831, 16843, 16871, 16879, 16883, 16889, 16901, 16903, 16921, 16927,
    16931, 16937, 16943, 16963, 16979, 16981, 16987, 16993, 17011, 17021, 17027, 17029, 17033, 17041, 17047, 17053,
    17077, 17093, 17099, 17107, 17117, 17123, 17137, 17159, 17167, 17183, 17189, 17191, 17203, 17207, 17209, 17231,
    17239, 17257, 17291, 17293, 17299, 17317, 17321, 17327, 17333, 17341, 17351, 17359, 17377, 17383, 17387, 17389,
    17393, 17401, 17417, 17419, 17431, 17443, 17449, 17467, 17471, 17477, 17483, 17489, 17491, 17497, 17509, 17519,
    17539, 17551, 17569, 17573, 17579, 17581, 17597, 17599, 17609, 17623, 17627, 17657, 17659, 17669, 17681, 17683,
    17707, 17713, 17729, 17737, 17747, 17749, 17761, 17783, 17789, 17791, 17807, 17827, 17837, 17839, 17851, 17863,
};

static int FASTCALL SearchPrimeTab(ulong u)
{
	const ushort * p_org = FirstPrimeNumbers;
	const uint count = SIZEOFARRAY(FirstPrimeNumbers);
	for(uint i = 0, lo = 0, up = count-1; lo <= up;) {
		const ushort * p = p_org + (i = (lo + up) >> 1);
		const int cmp = CMPSIGN(static_cast<ulong>(*p), u);
		if(cmp < 0)
			lo = i + 1;
		else if(cmp) {
			if(i)
				up = i - 1;
			else
				return 0;
		}
		else
			return 1;
	}
	return 0;
}

static int FASTCALL Helper_IsPrime(ulong val, int test)
{
	static const ushort last_tabbed_value = FirstPrimeNumbers[SIZEOFARRAY(FirstPrimeNumbers)-1];
	int    yes = 1;
	if(val < 2)
		yes = 0;
	else if(val == 2)
		yes = 1;
	else if((val & 1) == 0)
		yes = 0;
	else if(val > last_tabbed_value || test) {
		ulong n = static_cast<ulong>(sqrt(static_cast<double>(val)));
		for(ulong j = 2; j <= n; j++)
			if((val % j) == 0)
				yes = 0;
	}
	else {
		/*
		yes = 0;
		for(uint i = 0; !yes && i < SIZEOFARRAY(FirstPrimeNumbers); i++) {
            if((ulong)FirstPrimeNumbers[i] == val)
                yes = 1;
		}
		*/
		yes = SearchPrimeTab(val);
	}
	return yes;
}

const ushort * FASTCALL GetPrimeTab(size_t * pCount)
{
	ASSIGN_PTR(pCount, SIZEOFARRAY(FirstPrimeNumbers));
	return FirstPrimeNumbers;
}

int FASTCALL IsPrime(ulong val)
{
	return Helper_IsPrime(val, 0);
}

int SLAPI MutualReducePrimeMultiplicators(UlongArray & rA1, UlongArray & rA2, UlongArray * pReduceList)
{
	int    ok = -1;
	CALLPTRMEMB(pReduceList, clear());
    uint   i = rA1.getCount();
    if(i && rA2.getCount()) do {
        ulong  v = rA1.at(--i);
        uint   j = 0;
        if(rA2.lsearch(&v, &j, CMPF_LONG)) {
			CALLPTRMEMB(pReduceList, insert(&v));
            rA1.atFree(i);
            rA2.atFree(j);
            ok = 1;
        }
    } while(i && rA2.getCount());
    return ok;
}

int SLAPI Factorize(ulong val, UlongArray * pList)
{
	if(val != 0 && val != 1) {
		if(val == 2 || val == 3)
			pList->insert(&val);
		else {
			ulong v = val;
			ulong n = (ulong)sqrt((double)v);
			for(ulong i = 2; i <= n && i <= v; i++)
				if(IsPrime(i))
					while((v % i) == 0) {
						pList->insert(&i);
						v = v / i;
					}
			if(IsPrime(v))
				pList->insert(&v);
		}
	}
	return 1;
}
//
//
//
#if SLTEST_RUNNING // {
	SLTEST_R(fpow10i)
	{
		for(int i = -9; i < +9; i++) {
			SLTEST_CHECK_EQ(fpow10i(i), pow(10.0, i));
		}
		return CurrentStatus;
	}

	SLTEST_R(round)
	{
		struct A {
			int    R;  // точность округления //
			double T;  // тестовое значение
			double E0; // ожидаемый результат функции round(x, 0)
			double E1; // ожидаемый результат функции roundnev(x, 0)
		};
		A a[] = {
			{0, 0., 0., 0.},
			{0, -1., -1., -1.},
			{0, -5.00000000103, -5., -5.},
			{0, -4.99999981, -5., -5.},
			{0, -2.5, -3., -2.},
			{0, -17.5, -18., -18.},
			{0, -1023.499999, -1023., -1023.},
			{0, 5.00000000103, 5., 5.},
			{0, 4.99999981, 5., 5.},
			{0, 2.5, 3., 2.},
			{0, 17.5, 18., 18.},
			{0, 1023.499999, 1023., 1023.},
			{2, 72.055, 72.06, 72.06 }
		};
		uint i;
		int  j;
		for(i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
			SLTEST_CHECK_EQ(round   (a[i].T, a[i].R), a[i].E0);
			SLTEST_CHECK_EQ(roundnev(a[i].T, a[i].R), a[i].E1);
		}
		for(j = -11; j < +13; j++) {
			double p_ = fpow10i(j);
			for(i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
				if(a[i].R == 0) { // Для ненулевого начального округления возникает мелкая ошибка
					double v = a[i].T / p_;
					SLTEST_CHECK_EQ(round(v, j),    a[i].E0 / p_);
					SLTEST_CHECK_EQ(roundnev(v, j), a[i].E1 / p_);
				}
			}
		}
		if(CurrentStatus) {
			SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
			for(i = 0; i < 1000; i++) {
				double b, v, r;
				for(j = -11; j < +13; j++) {
					b = p_rng->GetReal();
					if(i % 2)
						b = _chgsign(b);
					v = round(b, j);
					SLTEST_CHECK_LT(fabs(v - b), fpow10i(-j)/2.0);
					r = fabs(b * fpow10i(j));
					SLTEST_CHECK_CRANGE(fabs(v), floor(r)/fpow10i(j), ceil(r)/fpow10i(j));
				}
			}
			delete p_rng;
		}
		return CurrentStatus;
	}

	SLTEST_R(Prime)
	{
		ulong last_tabbed_prime = FirstPrimeNumbers[SIZEOFARRAY(FirstPrimeNumbers)-1];
		for(ulong i = 0; i < last_tabbed_prime; i++) {
			const long isp = Helper_IsPrime(i, 1);
			long is_tabbed_prime = 0;
			if(i && i < SIZEOFARRAY(FirstPrimeNumbers)) {
				SLTEST_CHECK_LT((long)FirstPrimeNumbers[i-1], (long)FirstPrimeNumbers[i]);
			}
			for(uint j = 0; !is_tabbed_prime && j < SIZEOFARRAY(FirstPrimeNumbers); j++) {
				const ushort tv = FirstPrimeNumbers[j];
				if(tv == i)
					is_tabbed_prime = 1;
				else if(tv > i)
					break;
			}
			SLTEST_CHECK_EQ(is_tabbed_prime, isp);
			SLTEST_CHECK_EQ(Helper_IsPrime(i, 0), isp);
		}
		{
			struct TestSValue {
        		uint64 V;
        		uint32 S;
			} _test_row[] = {
        		{ 0ULL,                  1 },
        		{ 0x00000000000000ffULL, 1 },
        		{ 0x000000000000ffffULL, 2 },
        		{ 0x0000000000007f00ULL, 2 },
        		{ 0x0000000000ffffffULL, 3 },
        		{ 0x000000000070ff00ULL, 3 },
        		{ 0x00000000ffffffffULL, 4 },
        		{ 0x00000000af4f00ffULL, 4 },
        		{ 0x000000ffffffffffULL, 5 },
        		{ 0x0000003f00007b00ULL, 5 },
        		{ 0x0000ffffffffffffULL, 6 },
        		{ 0x00000ffff12fbeefULL, 6 },
        		{ 0x00ffffffffffffffULL, 7 },
        		{ 0x0070ff220001ff00ULL, 7 },
        		{ 0xffffffffffffffffULL, 8 },
        		{ 0x7000000000000000ULL, 8 },
        		{ 0x70af81b39ec62da5ULL, 8 }
			};
			uint8 buffer[32];
			uint32 sz;
			uint64 value;
			for(uint i = 0; i < SIZEOFARRAY(_test_row); i++) {
				memzero(buffer, sizeof(buffer));
				value = _test_row[i].V;
				sz = sshrinkuint64(value, buffer);
				SLTEST_CHECK_NZ(ismemzero(buffer+sz, sizeof(buffer)-sz));
				SLTEST_CHECK_EQ(sz, _test_row[i].S);
				SLTEST_CHECK_EQ(sexpanduint64(buffer, sz), value);
			}
		}
		return CurrentStatus;
	}
#endif // } SLTEST_RUNNING

#if 0 // {

void TestFactorize()
{
	ulong val = 0;
	do {
		printf("\n==========Input number:");
		scanf("%lu", &val);
		UlongArray list;
		Factorize(val, &list);
		for(uint i = 0; i < list.getCount(); i++)
			printf("%lu,", list.at(i));
		printf("\n");
	} while(val != 0);
    double rate = 0;
    do {
		printf("\n==========Input VAT rate:");
		scanf("%lf", &rate);
		printf("Min Divisor = %lf\n", GetMinVatDivisor(rate, 2));
    } while(rate != 0);
}

void main()
{
	TestFactorize();
}

#endif // } 0