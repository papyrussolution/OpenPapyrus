// TSSTAT.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2007, 2008, 2010, 2013, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

SLAPI StatBase::StatBase(long flags)
{
	Init(flags);
}

SLAPI StatBase::~StatBase()
{
}

int SLAPI StatBase::Init(long flags /*=0*/)
{
	Flags = flags;
	Count = 0;
	IterCount = 0;
	memzero(Sum, sizeof(Sum));
	Min = SMathConst::Max;
	Max = SMathConst::Min;
	Exp = Var = 0.0;
	Kurtosis = 0.0;
	Skew = 0.0;
	Test_Z = 0.0;
	Test_ChSq = 0.0;
	Series.freeAll();
	return 1;
}

// @prototype
double Lockes_Z_Test(const RealArray & x);

int SLAPI StatBase::Finish()
{
	if(Count > 0) {
		Exp = Sum[0] / Count;
		// Var = Exp * (Exp + 2.0 * Sum / Count) + SumQ / Count;
		Var = Sum[1] / Count - Exp * (2.0 * Sum[0] / Count - Exp);
		if(Flags & fExt) {
	    	Skew = (Sum[2] / Count) / pow(Sum[1] / Count, 1.5);
	    	Kurtosis = ((Sum[3] / Count) / fpowi(Sum[1] / Count, 2)) - 3.0; /* excess kurtosis */
		}
		if(Flags & fGammaTest) {
			Test_Z = Lockes_Z_Test(Series);
		}
	}
	else {
		Exp = 0.0;
		Var = 0.0;
	}
	return 1;
}

int SLAPI StatBase::Step(double val)
{
	Count++;
	IterCount++;
	Sum[0] += val;
	double sq = val * val;
	Sum[1] += sq;
	SETMAX(Max, val);
	SETMIN(Min, val);
	if(Flags & fExt) {
		Sum[2] += sq * val;
		Sum[3] += sq * sq;
	}
	if(Flags & (fGammaTest|fGaussianTest|fStoreVals)) {
		Series.insert(&val);
	}
	return 1;
}

long SLAPI StatBase::GetCount() const
{
	return Count;
}

double SLAPI StatBase::GetMin() const
{
	return Min;
}

double SLAPI StatBase::GetMax() const
{
	return Max;
}

double SLAPI StatBase::GetSum() const
{
	return Sum[0];
}

double SLAPI StatBase::GetExp() const
{
	return Exp;
}

double SLAPI StatBase::GetVar() const
{
	return Var;
}

double SLAPI StatBase::GetVariance() const
{
	return (Count > 1) ? (Var * ((double)Count) / ((double)(Count - 1))) : 0.0;
}

double SLAPI StatBase::GetStdDev() const
{
	return sqrt(GetVariance());
}

double SLAPI StatBase::GetTestGamma() const
{
	return Test_Z;
}

double SLAPI StatBase::GetTestGaussian() const
{
	return Test_ChSq;
}

int SLAPI StatBase::GetValue(long idx, double * pVal) const
{
	int    ok = -1;
	if(idx < (long)Series.getCount()) {
		ASSIGN_PTR(pVal, Series.at(idx));
		ok = 1;
	}
	return ok;
}

int SLAPI StatBase::GetGammaParams(double * pAlpha, double * pBeta) const
{
	double beta = fdivnz(GetVariance(), GetExp());
	double alpha = fdivnz(GetExp(), beta);
	ASSIGN_PTR(pAlpha, alpha);
	ASSIGN_PTR(pBeta, beta);
	return 1;
}
//
//
//
static int real_kendall_tau(const RealArray & x, const RealArray & y, RPairArray & rXy, double *ptau, double *pz)
{
	assert(x.getCount() == y.getCount());
	double tau, nn1, s2, z;
	int    tt1, tx = 0, ty = 0;
	int    Tx = 0, Ty = 0;
	int    Tx2 = 0, Ty2 = 0;
	int    Tx25 = 0, Ty25 = 0;
	int    tie, N0, N1, S;
	uint   i;
	/*
		populate sorter
	*/
	uint   j = 0;
	rXy.clear();
	for(i = 0; i < x.getCount(); i++) {
		//if(!na(x[i]) && !na(y[i])) {
			RPair p;
			p.X = x.at(i);
			p.Y = y.at(i);
			rXy.insert(&p);
		//}
	}
	rXy.SortByX(); // sort pairs by x
	/*
		make order counts
	*/
	N0 = N1 = 0;
	uint   nn = rXy.getCount();
	for(i = 0; i < nn; i++) {
		const RPair xy_i = rXy[i];
		double y_i = xy_i.Y;
		double x_i = xy_i.X;
		for(j = i+1; j < nn; j++) {
			const RPair xy = rXy[j];
			if(xy.Y == y_i)
				Ty++;
			else if(xy.X != x_i) {
				if(xy.Y > y_i)
					N0++;
				else if(xy.Y < y_i)
					N1++;
			}
		}
		if(i > 0) {
			/*
				account for ties in x
			*/
			tie = (x_i == rXy[i-1].X);
			if(tie)
				tx++;
			if(tx > 0 && (!tie || i == nn - 1)) {
				tx++;
				tt1 = tx * (tx - 1);
				Tx += tt1;
				Tx2 += tt1 * (tx - 2);
				Tx25 += tt1 * (2 * tx + 5);
				tx = 0;
			}
		}
	}
	if(Ty > 0) {
		/*
			account for ties in y
		*/
		Ty = 0;
		rXy.SortByY();
		for(i = 1; i < nn; i++) {
			tie = (rXy[i].Y == rXy[i-1].Y);
			if(tie)
				ty++;
			if(ty > 0 && (!tie || i == nn - 1)) {
				ty++;
				tt1 = ty * (ty - 1);
				Ty += tt1;
				Ty2 += tt1 * (ty - 2);
				Ty25 += tt1 * (2 * ty + 5);
				ty = 0;
			}
		}
	}
	S = N0 - N1;
#if 0
	fprintf(stderr, "N0 = %d, N1 = %d, S = %d\n", N0, N1, S);
	fprintf(stderr, "Tx = %d, Ty = %d\n", Tx, Ty);
#endif
	nn1 = nn * (nn - 1.0);
	/*
		normal approximation as in Shapiro and Chen,
		Journal of Quality Technology, 2001
	*/
	if(Tx == 0 && Ty == 0) {
		tau = 2 * S / nn1;
		s2 = (1.0/18) * nn1 * (2 * nn + 5);
	}
	else {
		double den = (nn1 - Tx) * (nn1 - Ty);
		tau = 2 * S / sqrt(den);
		s2 = (1.0/18) * (nn1 * (2 * nn + 5) - Tx25 - Ty25);
		if(Tx2 != 0 && Ty2 != 0)
			s2 += (1.0/(9*nn1*(nn-2))) * Tx2 * Ty2;
		if(Tx != 0 && Ty != 0)
			s2 += (1.0/(2*nn1)) * Tx * Ty;
	}
	z = (S - 1) / sqrt(s2);
	ASSIGN_PTR(ptau, tau);
	ASSIGN_PTR(pz, z);
	return 1;
}
/*
	check for negative values, screen out missing values,
	and load x into an array for randomization
*/
static int locke_shuffle_init(const RealArray & rX, RealArray & rResult)
{
	uint   i, m = 0;
	rResult.clear();
	for(i = 0; i < rX.getCount(); i++) {
		if(rX.at(i) < 0.0)
			return 0;
		else //if(!na(x[i]))
			m++;
	}
	if(m < 4) {
		return 0;
	}
	else {
		for(i = 0; i < rX.getCount(); i++) {
			//if(!na(x[i])) {
				rResult.insert(&rX.at(i));
			//}
		}
		return 1;
	}
}

IMPL_CMPCFUNC(lockes_test_rand, i1, i2)
{
	SRng * p_rng = (SRng *)pExtraData;
	ulong r = p_rng ? p_rng->GetUniformInt(8096) : 0;
	return (r - 4097);
}
/*
	lockes_test:
	@x: data series.

	Performs Charles Locke's nonparametric test for whether an
	empirical distribution (namely, that of @x over the range @t1 to @t2) is gamma.
	See C. Locke, "A Test for the Composite Hypothesis that a Population has a Gamma Distribution,"
	Commun. Statis.-Theor. Meth. A5(4), 351-364 (1976).  Also see Shapiro and Chen,
	Journal of Quality Technology 33(1), Jan 2001.

	Returns: the z value for test, or #NADBL on error.
*/
double Lockes_Z_Test(const RealArray & x)
{
	double z = 0.0; // NADBL
	double zj;
	uint   m = x.getCount();
	uint   i, t;
	RealArray sx;
	SRng * p_rng = 0;
	if(locke_shuffle_init(x, sx)) {
		m /= 2;
		// dimention = m
		RealArray u;
		RealArray v;
		RPairArray uv;
		double zero = 0.0;
		for(i = 0; i < m; i++) {
			u.insert(&zero);
			v.insert(&zero);
		}
		z = 0.0;
		uint   nrepeat = 100;
		if(x.getCount() > 10000)
			nrepeat = 1;
		else if(x.getCount() > 1000)
			nrepeat = 10;
		/*
			repeat the shuffling of the series NREPEAT times, since the
			test statistic is sensitive to the ordering under the null
		*/
		p_rng = SRng::CreateInstance(SRng::algMT, 0);
		p_rng->Set(getcurtime_());
		for(uint j = 0; j < nrepeat; j++) {
			sx.sort(PTR_CMPCFUNC(lockes_test_rand), p_rng);
			t = 0;
			for(i = 0; i < m; i++) {
				u[i] = sx[t] + sx[t+1];
				v[i] = sx[t] / sx[t+1];
				double temp = sx[t+1] / sx[t];
				SETMAX(v[i], temp);
				t += 2;
			}
			real_kendall_tau(u, v, uv, 0, &zj);
			z += zj;
#if LOCKE_DEBUG
			printf("z[%d] = %g\n", j, zj);
#endif
		}
		z /= (double)nrepeat;
#if LOCKE_DEBUG
		fprintf(stderr, "Kendall's tau: average z = %g\n", z);
#endif
	}
	delete p_rng;
	return z;
}
//
//
//
SLAPI TimSerStat::TimSerStat()
{
	P_Queue = 0;
	P_AC_Add = 0;
	P_AC_Mul = 0;
	Init();
}

SLAPI TimSerStat::~TimSerStat()
{
	delete P_Queue;
	delete P_AC_Add;
	delete P_AC_Mul;
}

int SLAPI TimSerStat::Init()
{
	StatBase::Init();
	NumLags = 0;
	ZDELETE(P_Queue);
	ZDELETE(P_AC_Add);
	ZDELETE(P_AC_Mul);
	return 1;
}

int SLAPI TimSerStat::SetNumLags(long numLags)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	NumLags = numLags;
	ZDELETE(P_Queue);
	ZDELETE(P_AC_Add);
	ZDELETE(P_AC_Mul);
	if(NumLags > 0) {
		size_t n = (size_t)NumLags;
		THROW_V(P_Queue = new DblQueue(n), SLERR_NOMEM);
		THROW_V(P_AC_Add = new double[n], SLERR_NOMEM);
		THROW_V(P_AC_Mul = new double[n], SLERR_NOMEM);
		memzero(P_AC_Add, sizeof(double) * n);
		memzero(P_AC_Mul, sizeof(double) * n);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI TimSerStat::Step(double val, int /*whiteSpace*/)
{
	StatBase::Step(val);
	if(NumLags > 0) {
		uint c = P_Queue->getNumItems();
		for(uint i = 0; i < c; i++) {
			uint   lag = c - i;
			double v_i = P_Queue->get(i);
   	        P_AC_Add[lag-1] += (val + v_i);
	   	    P_AC_Mul[lag-1] += (val * v_i);
		}
		P_Queue->push(val);
	}
	return 1;
}

int SLAPI TimSerStat::Finish()
{
	StatBase::Finish();
	if(NumLags > 0) {
		double n_var = Count * Var;
		double qexp = Exp * Exp;
		for(uint i = 0; i < P_Queue->getNumItems(); i++)
			P_AC_Add[i] = (qexp * (Count-i-1) + P_AC_Mul[i] - Exp * P_AC_Add[i]) / n_var;
		return 1;
	}
	else
		return -1;
}

uint SLAPI TimSerStat::GetNumLags() const
{
	return P_Queue ? P_Queue->getNumItems() : 0;
}

double SLAPI TimSerStat::GetAutocorrel(uint lag) const
{
	return (lag < GetNumLags()) ? P_AC_Add[lag] : 0L;
}
//
//
//
SLAPI TimSerSpikes::TimSerSpikes()
{
	Sign = 0;
	Count = 0;
	P_Spikes = 0;
}

SLAPI TimSerSpikes::~TimSerSpikes()
{
	delete P_Spikes;
}

int SLAPI TimSerSpikes::Init(int sign)
{
	Sign = sign;
	Count = 0;
	MEMSZERO(Prev);
	P_Spikes = new SArray(sizeof(Spike));
	return P_Spikes ? 1 : (SLibError = SLERR_NOMEM, 0);
}

int SLAPI TimSerSpikes::Step(long n, double val)
{
	Count++;
	if(Count > 2) {
		if(Sign >= 0)
			if(Prev[1].V >= Prev[0].V && Prev[1].V > val)
				if(!P_Spikes->insert(&Prev[1]))
					return 0;
		if(Sign <= 0)
			if(Prev[1].V <= Prev[0].V && Prev[1].V < val)
				if(!P_Spikes->insert(&Prev[1]))
					return 0;
		Prev[0] = Prev[1];
		Prev[1].V = val;
		Prev[1].N = n;
	}
	else {
		Prev[(size_t)Count-1].V = val;
		Prev[(size_t)Count-1].N = n;
	}
	return 1;
}

long SLAPI TimSerSpikes::GetNumSpikes() const
{
	return P_Spikes ? P_Spikes->getCount() : 0;
}

double SLAPI TimSerSpikes::GetSpike(long n, long * pN) const
{
	if(n < GetNumSpikes()) {
		Spike * p_spike = (Spike *)P_Spikes->at((uint)n);
		ASSIGN_PTR(pN, p_spike->N);
		return p_spike->V;
	}
	return 0L;
}

struct _PRDINF {
	long   Distance;
	long   Count;
};

long SLAPI TimSerSpikes::GetMostCommonDistance()
{
	uint   i = 0;
	_PRDINF prd, * p_prd;
	SArray prd_ary(sizeof(_PRDINF));
	for(i = 0; (int)i < GetNumSpikes() - 1; i++) {
		long s, n;
		uint idx = 0;
		GetSpike(i,   &s);
   		GetSpike(i+1, &n);
		prd.Distance = labs(s - n);
		if(prd_ary.lsearch(&prd, &idx, CMPF_LONG)) {
			p_prd = (_PRDINF *)prd_ary.at(idx);
			p_prd->Distance = labs(s - n);
			p_prd->Count++;
		}
		else {
			prd.Distance = labs(s - n);
			prd.Count = 1;
			prd_ary.insert(&prd);
		}
	}
	long max_count = 0;
	long distance = 3; // default distance
	for(i = 0; prd_ary.enumItems(&i, (void **)&p_prd);)
		if(p_prd->Count > max_count) {
			distance = p_prd->Distance;
			max_count = p_prd->Count;
		}
	return distance;
}
