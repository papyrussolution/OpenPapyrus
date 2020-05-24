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
	  2,     3,     5,     7,    11,    13,    17,    19,    23,    29,
	 31,    37,    41,    43,    47,    53,    59,    61,    67,    71,
	 73,    79,    83,    89,    97,   101,   103,   107,   109,   113,
	127,   131,   137,   139,   149,   151,   157,   163,   167,   173,
	179,   181,   191,   193,   197,   199,   211,   223,   227,   229,
	233,   239,   241,   251,   257,   263,   269,   271,   277,   281,
	283,   293,   307,   311,   313,   317,   331,   337,   347,   349,
	353,   359,   367,   373,   379,   383,   389,   397,   401,   409,
	419,   421,   431,   433,   439,   443,   449,   457,   461,   463,
	467,   479,   487,   491,   499,   503,   509,   521,   523,   541,
	547,   557,   563,   569,   571,   577,   587,   593,   599,   601,
	607,   613,   617,   619,   631,   641,   643,   647,   653,   659,
	661,   673,   677,   683,   691,   701,   709,   719,   727,   733,
	739,   743,   751,   757,   761,   769,   773,   787,   797,   809,
	811,   821,   823,   827,   829,   839,   853,   857,   859,   863,
	877,   881,   883,   887,   907,   911,   919,   929,   937,   941,
	947,   953,   967,   971,   977,   983,   991,   997,  1009,  1013,
   1019,  1021,  1031,  1033,  1039,  1049,  1051,  1061,  1063,  1069,
   1087,  1091,  1093,  1097,  1103,  1109,  1117,  1123,  1129,  1151,
   1153,  1163,  1171,  1181,  1187,  1193,  1201,  1213,  1217,  1223,
   1229,  1231,  1237,  1249,  1259,  1277,  1279,  1283,  1289,  1291,
   1297,  1301,  1303,  1307,  1319,  1321,  1327,  1361,  1367,  1373,
   1381,  1399,  1409,  1423,  1427,  1429,  1433,  1439,  1447,  1451,
   1453,  1459,  1471,  1481,  1483,  1487,  1489,  1493,  1499,  1511,
   1523,  1531,  1543,  1549,  1553,  1559,  1567,  1571,  1579,  1583,
   1597,  1601,  1607,  1609,  1613,  1619,  1621,  1627,  1637,  1657,
   1663,  1667,  1669,  1693,  1697,  1699,  1709,  1721,  1723,  1733,
   1741,  1747,  1753,  1759,  1777,  1783,  1787,  1789,  1801,  1811,
   1823,  1831,  1847,  1861,  1867,  1871,  1873,  1877,  1879,  1889,
   1901,  1907,  1913,  1931,  1933,  1949,  1951,  1973,  1979,  1987,
   1993,  1997,  1999,  2003,  2011,  2017,  2027,  2029,  2039,  2053,
   2063,  2069,  2081,  2083,  2087,  2089,  2099,  2111,  2113,  2129,
   2131,  2137,  2141,  2143,  2153,  2161,  2179,  2203,  2207,  2213,
   2221,  2237,  2239,  2243,  2251,  2267,  2269,  2273,  2281,  2287,
   2293,  2297,  2309,  2311,  2333,  2339,  2341,  2347,  2351,  2357,
   2371,  2377,  2381,  2383,  2389,  2393,  2399,  2411,  2417,  2423,
   2437,  2441,  2447,  2459,  2467,  2473,  2477,  2503,  2521,  2531,
   2539,  2543,  2549,  2551,  2557,  2579,  2591,  2593,  2609,  2617,
   2621,  2633,  2647,  2657,  2659,  2663,  2671,  2677,  2683,  2687,
   2689,  2693,  2699,  2707,  2711,  2713,  2719,  2729,  2731,  2741,
   2749,  2753,  2767,  2777,  2789,  2791,  2797,  2801,  2803,  2819,
   2833,  2837,  2843,  2851,  2857,  2861,  2879,  2887,  2897,  2903,
   2909,  2917,  2927,  2939,  2953,  2957,  2963,  2969,  2971,  2999,
   3001,  3011,  3019,  3023,  3037,  3041,  3049,  3061,  3067,  3079,
   3083,  3089,  3109,  3119,  3121,  3137,  3163,  3167,  3169,  3181,
   3187,  3191,  3203,  3209,  3217,  3221,  3229,  3251,  3253,  3257,
   3259,  3271,  3299,  3301,  3307,  3313,  3319,  3323,  3329,  3331,
   3343,  3347,  3359,  3361,  3371,  3373,  3389,  3391,  3407,  3413,
   3433,  3449,  3457,  3461,  3463,  3467,  3469,  3491,  3499,  3511,
   3517,  3527,  3529,  3533,  3539,  3541,  3547,  3557,  3559,  3571,
   3581,  3583,  3593,  3607,  3613,  3617,  3623,  3631,  3637,  3643,
   3659,  3671,  3673,  3677,  3691,  3697,  3701,  3709,  3719,  3727,
   3733,  3739,  3761,  3767,  3769,  3779,  3793,  3797,  3803,  3821,
   3823,  3833,  3847,  3851,  3853,  3863,  3877,  3881,  3889,  3907,
   3911,  3917,  3919,  3923,  3929,  3931,  3943,  3947,  3967,  3989,
   4001,  4003,  4007,  4013,  4019,  4021,  4027,  4049,  4051,  4057,
   4073,  4079,  4091,  4093,  4099,  4111,  4127,  4129,  4133,  4139,
   4153,  4157,  4159,  4177,  4201,  4211,  4217,  4219,  4229,  4231,
   4241,  4243,  4253,  4259,  4261,  4271,  4273,  4283,  4289,  4297,
   4327,  4337,  4339,  4349,  4357,  4363,  4373,  4391,  4397,  4409,
   4421,  4423,  4441,  4447,  4451,  4457,  4463,  4481,  4483,  4493,
   4507,  4513,  4517,  4519,  4523,  4547,  4549,  4561,  4567,  4583,
   4591,  4597,  4603,  4621,  4637,  4639,  4643,  4649,  4651,  4657,
   4663,  4673,  4679,  4691,  4703,  4721,  4723,  4729,  4733,  4751,
   4759,  4783,  4787,  4789,  4793,  4799,  4801,  4813,  4817,  4831,
   4861,  4871,  4877,  4889,  4903,  4909,  4919,  4931,  4933,  4937,
   4943,  4951,  4957,  4967,  4969,  4973,  4987,  4993,  4999,  5003,
   5009,  5011,  5021,  5023,  5039,  5051,  5059,  5077,  5081,  5087,
   5099,  5101,  5107,  5113,  5119,  5147,  5153,  5167,  5171,  5179,
   5189,  5197,  5209,  5227,  5231,  5233,  5237,  5261,  5273,  5279,
   5281,  5297,  5303,  5309,  5323,  5333,  5347,  5351,  5381,  5387,
   5393,  5399,  5407,  5413,  5417,  5419,  5431,  5437,  5441,  5443,
   5449,  5471,  5477,  5479,  5483,  5501,  5503,  5507,  5519,  5521,
   5527,  5531,  5557,  5563,  5569,  5573,  5581,  5591,  5623,  5639,
   5641,  5647,  5651,  5653,  5657,  5659,  5669,  5683,  5689,  5693,
   5701,  5711,  5717,  5737,  5741,  5743,  5749,  5779,  5783,  5791,
   5801,  5807,  5813,  5821,  5827,  5839,  5843,  5849,  5851,  5857,
   5861,  5867,  5869,  5879,  5881,  5897,  5903,  5923,  5927,  5939,
   5953,  5981,  5987,  6007,  6011,  6029,  6037,  6043,  6047,  6053,
   6067,  6073,  6079,  6089,  6091,  6101,  6113,  6121,  6131,  6133,
   6143,  6151,  6163,  6173,  6197,  6199,  6203,  6211,  6217,  6221,
   6229,  6247,  6257,  6263,  6269,  6271,  6277,  6287,  6299,  6301,
   6311,  6317,  6323,  6329,  6337,  6343,  6353,  6359,  6361,  6367,
   6373,  6379,  6389,  6397,  6421,  6427,  6449,  6451,  6469,  6473,
   6481,  6491,  6521,  6529,  6547,  6551,  6553,  6563,  6569,  6571,
   6577,  6581,  6599,  6607,  6619,  6637,  6653,  6659,  6661,  6673,
   6679,  6689,  6691,  6701,  6703,  6709,  6719,  6733,  6737,  6761,
   6763,  6779,  6781,  6791,  6793,  6803,  6823,  6827,  6829,  6833,
   6841,  6857,  6863,  6869,  6871,  6883,  6899,  6907,  6911,  6917,
   6947,  6949,  6959,  6961,  6967,  6971,  6977,  6983,  6991,  6997,
   7001,  7013,  7019,  7027,  7039,  7043,  7057,  7069,  7079,  7103,
   7109,  7121,  7127,  7129,  7151,  7159,  7177,  7187,  7193,  7207,
   7211,  7213,  7219,  7229,  7237,  7243,  7247,  7253,  7283,  7297,
   7307,  7309,  7321,  7331,  7333,  7349,  7351,  7369,  7393,  7411,
   7417,  7433,  7451,  7457,  7459,  7477,  7481,  7487,  7489,  7499,
   7507,  7517,  7523,  7529,  7537,  7541,  7547,  7549,  7559,  7561,
   7573,  7577,  7583,  7589,  7591,  7603,  7607,  7621,  7639,  7643,
   7649,  7669,  7673,  7681,  7687,  7691,  7699,  7703,  7717,  7723,
   7727,  7741,  7753,  7757,  7759,  7789,  7793,  7817,  7823,  7829,
   7841,  7853,  7867,  7873,  7877,  7879,  7883,  7901,  7907,  7919
};

static int FASTCALL SearchPrimeTab(ulong u)
{
	const ushort * p_org = FirstPrimeNumbers;
	const uint count = SIZEOFARRAY(FirstPrimeNumbers);
	for(uint i = 0, lo = 0, up = count-1; lo <= up;) {
		const ushort * p = p_org + (i = (lo + up) >> 1);
		const int cmp = CMPSIGN((ulong)*p, u);
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
		ulong n = (ulong)sqrt((double)val);
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