// SMATH.CPP
// Copyright (c) A.Sobolev 2004, 2006, 2007, 2008, 2010, 2012, 2014, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <float.h>

#define PI__  3.1415926535897932384626433832795
#define PI__f 3.14159265358979323846f

const double SMathConst::Pi       = PI__;
const double SMathConst::Pi2      = (PI__ * 2.0);
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

//int SLAPI IsValidIEEE(double v) { return _finite(v) ? 1 : 0; }

int SLAPI IsValidIEEE(double v)
{
	/*
	_FPCLASS_SNAN	Signaling NaN
	_FPCLASS_QNAN	Quiet NaN
	_FPCLASS_NINF	Negative infinity ( –INF)
	_FPCLASS_NN		Negative normalized non-zero
	_FPCLASS_ND		Negative denormalized
	_FPCLASS_NZ		Negative zero ( – 0)
	_FPCLASS_PZ		Positive 0 (+0)
	_FPCLASS_PD		Positive denormalized
	_FPCLASS_PN		Positive normalized non-zero
	_FPCLASS_PINF	Positive infinity (+INF)
	*/
	int    c = _fpclass(v);
	return oneof6(c, _FPCLASS_NN, _FPCLASS_ND, _FPCLASS_NZ, _FPCLASS_PZ, _FPCLASS_PD, _FPCLASS_PN);
}

int fisnan(double v)
{
	return _isnan(v);
}

int fisinf(double v)
{
	int    fpc = _fpclass(v);
	if(fpc == _FPCLASS_PINF)
		return +1;
	else if(fpc == _FPCLASS_NINF)
		return -1;
	else
		return 0;
}
//
// SMathResult {
//
void SMathResult::SetErr(double e, double adjMult)
{
	E = (e + (adjMult * SMathConst::Epsilon * fabs(V)));
}

void SMathResult::AdjustErr(double mult)
{
	E += mult * SMathConst::Epsilon * fabs(V);
}

void SMathResult::SetZero()
{
	V = E = 0;
}

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
int    FASTCALL smin(int a, int b)       { return MAX(a, b); }
double SLAPI smax(double a, double b) { return MAX(a, b); }
double SLAPI smin(double a, double b) { return MIN(a, b); }
float  SLAPI smax(float a, float b)   { return MAX(a, b); }
float  SLAPI smin(float a, float b)   { return MIN(a, b); }
double FASTCALL fdiv100r(double v)    { return v / 100.0; }
double FASTCALL fdiv100i(long v)      { return ((double)v) / 100.0; }
double FASTCALL fdiv1000i(long v)     { return ((double)v) / 1000.0; }
double FASTCALL fdivnz(double dd, double dr) { return (dr != 0.0) ? (dd / dr) : 0.0; }

int SLAPI feqeps(double v1, double v2, double epsilon)
{
	return (fabs(v1-v2) < epsilon) ? 1 : 0;
}

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

double fgetwsign(double val, int sign)
{
	if(sign > 0)
		return val;
	else if(sign < 0)
		return -val;
	else
		return 0;
}

double faddwsign(double val, double addendum, int sign)
{
	if(sign > 0)
		return (val + addendum);
	else if(sign < 0)
		return (val - addendum);
	else
		return val;
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

double FASTCALL ffactr(uint i)
{
	static double * p_fact_tab = 0;
	if(p_fact_tab == 0) {
		ENTER_CRITICAL_SECTION
			if(p_fact_tab == 0) {
				p_fact_tab = (double *)SAlloc::C(FACT_TAB_SIZE, sizeof(*p_fact_tab));
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

double FASTCALL degtorad(double deg)
{
	return (SMathConst::PiDiv180 * deg);
}

float FASTCALL degtorad(float deg)
{
	return ((float)SMathConst::PiDiv180 * deg);
}

double FASTCALL degtorad(int deg)
{
	return (SMathConst::PiDiv180 * deg);
}

double FASTCALL radtodeg(double rad)
{
	return (rad / SMathConst::PiDiv180);
}

double fscale(double v, double low, double upp)
{
	return (v - low) / (upp - low);
}

double SLAPI sigmoid(double a, double x)
{
	return (1.0 / (1.0 + exp(-(a * x))));
}
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
//
//
SLAPI SHistogram::SHistogram()
{
	Flags = 0;
	LeftEdge = 0.0;
	Step = 0.0;
	P_Stat = 0;
	DevMean = 0.0;
	DevWidth = 0.0;
}

SLAPI SHistogram::~SHistogram()
{
	delete P_Stat;
}

int SLAPI SHistogram::Setup()
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
	return 1;
}

int SLAPI SHistogram::SetupDynamic(double leftEdge, double step)
{
	Setup();
	Flags |= fDynBins;
	LeftEdge = leftEdge;
	Step = step;
	return 1;
}

int SLAPI SHistogram::SetupDev(int even, double widthSigm, uint binCount)
{
	Setup();
	Flags |= fDeviation;
	P_Stat = new StatBase;
	P_Stat->Init();
	if(even)
		Flags |= fEven;
	DevWidthSigm = widthSigm;
	DevBinCount = binCount;
	return 1;
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
			bin_no = ((long)((val - LeftEdge) / Step)) + 1;
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
