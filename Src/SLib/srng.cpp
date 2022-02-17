// SRNG.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2010, 2016, 2017, 2018, 2019, 2020
//
// Random Number Generators
//
#include <slib-internal.h>
#pragma hdrstop

SRng::SRng(int alg, uint level, ulong rndMin, ulong rndMax) : Alg(alg), Level(level), RandMin(rndMin), RandMax(rndMax)
{
}

SRng::~SRng()
{
}

ulong SRng::GetMin() const { return RandMin; }
ulong SRng::GetMax() const { return RandMax; }
//
// Note: to avoid integer overflow in (range+1) we work with scale = range/n = (max-min)/n
// rather than scale=(max-min+1)/n, this reduces
// efficiency slightly but avoids having to check for the out of range
// value.  Note that range is typically O(2^32) so the addition of 1
// is negligible in most usage.
//
ulong FASTCALL SRng::GetUniformInt(ulong n)
{
	const  ulong  offset = RandMin;
	const  ulong  range  = RandMax - offset;
	ulong  k = 0;
	if(n && n <= range) { // GSL_ERROR_VAL ("invalid n, either 0 or exceeds maximum value of generator", GSL_EINVAL, 0) ;
		const ulong scale = range / n;
		do {
			k = (Get() - offset) / scale;
		} while(k >= n);
	}
	return k;
}

double SRng::GetUniformPos()
{
	double x;
	do {
		x = GetReal();
	} while(x == 0.0);
	return x;
}
//
//
//
// Original implementation was copyright (C) 1997 Makoto Matsumoto and
// Takuji Nishimura. Coded by Takuji Nishimura, considering the
// suggestions by Topher Cooper and Marc Rieffel in July-Aug. 1997, "A
// C-program for MT19937: Integer version (1998/4/6)"
//
// This implementation copyright (C) 1998 Brian Gough. I reorganized
// the code to use the module framework of GSL.  The license on this
// implementation was changed from LGPL to GPL, following paragraph 3 of the LGPL, version 2.
//
// Update:
//
// The seeding procedure has been updated to match the 10/99 release of MT19937.
//
// Update:
//
// The seeding procedure has been updated again to match the 2002 release of MT19937
//
// The original code included the comment: "When you use this, send an
// email to: matumoto@math.keio.ac.jp with an appropriate reference to your work".
//
// Makoto Matsumoto has a web page with more information about the
// generator, http://www.math.keio.ac.jp/~matumoto/emt.html.
//
// The paper below has details of the algorithm.
//
// From: Makoto Matsumoto and Takuji Nishimura, "Mersenne Twister: A
// 623-dimensionally equidistributerd uniform pseudorandom number
// generator". ACM Transactions on Modeling and Computer Simulation,
// Vol. 8, No. 1 (Jan. 1998), Pages 3-30
//
// You can obtain the paper directly from Makoto Matsumoto's web page.
//
// The period of this generator is 2^{19937} - 1.
//
#define MT_PERIOD_PARAM 624

class SRngMT : public SRng {
public:
	SRngMT(Algorithm alg, uint level) : SRng(alg, level, 0, 0xffffffffUL)
	{
		assert(alg == algMT); // @v10.3.1 @fix (=)-->(==)
		assert(oneof3(level, 0, 1999, 1998));
	}
	virtual void   Set(ulong seed);
	virtual ulong  Get();
	virtual double GetReal();
private:
	ulong  Mt[MT_PERIOD_PARAM];
	int    Mti;
	// @#{0, 1999, 1998}
};

void SRngMT::Set(ulong seed)
{
	int    i = 0;
	SETIFZ(seed, 4357); // the default seed is 4357
	if(Level == 0) {
		Mt[0] = seed & 0xffffffffUL;
		for(i = 1; i < MT_PERIOD_PARAM; i++) {
			// See Knuth's "Art of Computer Programming" Vol. 2, 3rd Ed. p.106 for multiplier.
			Mt[i] = (1812433253UL * (Mt[i - 1] ^ (Mt[i - 1] >> 30)) + i);
			Mt[i] &= 0xffffffffUL;
		}
	}
	else if(Level == 1999) {
		//
		// This is the October 1999 version of the seeding procedure. It
		// was updated by the original developers to avoid the periodicity
		// in the simple congruence originally used.
		//
		// Note that an ANSI-C ulong  integer arithmetic is
		// automatically modulo 2^32 (or a higher power of two), so we can
		// safely ignore overflow.
		//
		#define LCG(x)	(((69069 * (x)) + 1) & 0xffffffffUL)
		for(i = 0; i < MT_PERIOD_PARAM; i++) {
			Mt[i] = seed & 0xffff0000UL;
			seed = LCG(seed);
			Mt[i] |= (seed & 0xffff0000UL) >> 16;
			seed = LCG(seed);
		}
	}
	else if(Level == 1998) {
		Mt[0] = seed & 0xffffffffUL;
		#define LCG1998(n)	((69069 * (n)) & 0xffffffffUL)
		for(i = 1; i < MT_PERIOD_PARAM; i++)
			Mt[i] = LCG1998(Mt[i - 1]);
	}
	Mti = i;
}

static const ulong  UPPER_MASK = 0x80000000UL; // most significant w-r bits
static const ulong  LOWER_MASK = 0x7fffffffUL; // least significant r bits

ulong SRngMT::Get()
{
	ulong  k;
	const  uint M = 397;
#define MAGIC(y)	(((y) & 0x1) ? 0x9908b0dfUL : 0)
	if(Mti >= MT_PERIOD_PARAM) {
		/* generate N words at one time */
		int    kk;
		for(kk = 0; kk < MT_PERIOD_PARAM - M; kk++) {
			ulong  y = (Mt[kk] & UPPER_MASK) | (Mt[kk + 1] & LOWER_MASK);
			Mt[kk] = Mt[kk + M] ^ (y >> 1) ^ MAGIC(y);
		}
		for(; kk < MT_PERIOD_PARAM - 1; kk++) {
			ulong  y = (Mt[kk] & UPPER_MASK) | (Mt[kk + 1] & LOWER_MASK);
			Mt[kk] = Mt[kk + (M - MT_PERIOD_PARAM)] ^ (y >> 1) ^ MAGIC(y);
		}
		{
			ulong  y = (Mt[MT_PERIOD_PARAM - 1] & UPPER_MASK) | (Mt[0] & LOWER_MASK);
			Mt[MT_PERIOD_PARAM - 1] = Mt[M - 1] ^ (y >> 1) ^ MAGIC(y);
		}
		Mti = 0;
	}
	/* Tempering */
	k = Mt[Mti];
	k ^= (k >> 11);
	k ^= (k << 7) & 0x9d2c5680UL;
	k ^= (k << 15) & 0xefc60000UL;
	k ^= (k >> 18);
	Mti++;
	return k;
}

double SRngMT::GetReal()
{
	return Get() / 4294967296.0;
}
//
//
//
class SRngUnix : public SRng {
public:
	SRngUnix(Algorithm alg, uint bits) : SRng(alg, bits, 0, 0x7fffffffUL), I(0), J(0)
	{
		assert(oneof3(alg, algBSD, algLibC5, algGLibC2));
		assert(oneof5(bits, 8, 32, 64, 128, 256));
	}
	virtual void   Set(ulong seed);
	virtual ulong  Get();
	virtual double GetReal();
private:
	int    I;
	int    J;
	long   X[64];
};

void SRngUnix::Set(ulong s)
{
	if(Level == 8) {
		X[0] = NZOR(s, 1);
	}
	else {
		{
			const uint n = (Level >> 2) - 1;
			if(Alg == algBSD) {
				X[0] = NZOR(s, 1);
				for(uint i = 1; i < n; i++)
					X[i] = 1103515245 * X[i-1] + 12345;
			}
			else if(Alg == algLibC5) {
				X[0] = NZOR(s, 1);
				for(uint i = 1; i < n; i++)
					X[i] = 1103515145 * X[i-1] + 12345;
			}
			else if(Alg == algGLibC2) {
				SETIFZ(s, 1);
				X[0] = s;
				for(uint i = 1; i < n; i++) {
					const long h = s / 127773;
					const long t = 16807 * (s - h * 127773) - h * 2836;
					s = (t < 0) ? (t + 2147483647) : t;
					X[i] = s;
				}
			}
		}
		J = 0;
		if(oneof2(Level, 32, 128))
			I = 3;
		else if(oneof2(Level, 64, 256))
			I = 1;
		const uint n = 10 * ((Level >> 2) - 1);
		for(uint i = 0; i < n; i++)
			Get();
	}
}

ulong SRngUnix::Get()
{
	ulong  ret;
	if(Level == 8) {
		X[0] = (1103515245 * X[0] + 12345) & 0x7fffffffUL;
		ret = (ulong)X[0];
	}
	else {
		const int n = (Level >> 2) - 1;
		X[I] += X[J];
		ret = (X[I] >> 1) & 0x7FFFFFFF;
		if(++I == n)
			I = 0;
		if(++J == n)
			J = 0;
	}
	return ret;
}

double SRngUnix::GetReal()
{
	return Get() / 2147483648.0;
}
//
//
//
class SRngRANLUX : public SRng {
public:
	SRngRANLUX(Algorithm alg, uint rank /* 0, 1, 2 */) : SRng(alg, rank, 0, ((alg == algRANLUX_S) ? 0x00ffffffUL: 0xffffffffUL))
	{
		assert(oneof2(alg, algRANLUX_S, algRANLUX_D));
		if(alg == algRANLUX_S) {
			assert(oneof3(rank, 0, 1, 2));
			IMul = 16777216.0;
		}
		else {
			assert(oneof2(rank, 1, 2));
			IMul = 4294967296.0;
		}
		if(rank == 0)
			Luxury = 109;
		else if(rank == 1)
			Luxury = 202;
		else
			Luxury = 397;
	}
	virtual void   Set(ulong seed);
	virtual ulong  Get();
	virtual double GetReal();
private:
	void   IncrementState();
	double DoCarry(double y);
	int    IS_Loop1(int k);
	int    IS_Loop2(int k, int kmax);
	int    IS_Loop3(int k, int kmax);

	double xdbl[12];
	double ydbl[12]; // D  doubles first so they are 8-byte aligned
	double carry;
	float  xflt[24]; // S
	uint   ir;
	uint   jr;
	uint   ir_old;   // D
	uint   is;       // S
	uint   is_old;   // S
	uint   pr;
	double IMul;
	uint   Luxury;
};

#define NEXT_CYCLE_12(idx) ((idx)+1)%12
#define NEXT_CYCLE_24(idx) ((idx)+1)%24

static const double one_bit = 1.0 / 281474976710656.0; // 1/2^48
static const double shift   = 268435456.0;             // 2^28

//
double SRngRANLUX::DoCarry(double y)
{
	if(y < 0) {
		carry = one_bit;
		y += 1;
	}
	else
		carry = 0;
	return y;
}

int SRngRANLUX::IS_Loop1(int k)
{
	for(; ir > 0; ++k) {
		xdbl[ir] = DoCarry(xdbl[jr] - xdbl[ir] - carry);
		ir = NEXT_CYCLE_12(ir);
		jr = NEXT_CYCLE_12(jr);
	}
	return k;
}

int SRngRANLUX::IS_Loop3(int k, int kmax)
{
	for(; k < kmax; ++k) {
		xdbl[ir] = DoCarry(xdbl[jr] - xdbl[ir] - carry);
		if(Alg == algRANLUX_S)
			ydbl[ir] = xdbl[ir] + shift;
		ir = NEXT_CYCLE_12(ir);
		jr = NEXT_CYCLE_12(jr);
	}
	return k;
}

int SRngRANLUX::IS_Loop2(int k, int kmax)
{
	#define RANLUX_STEP(x1, x2, i1, i2, i3) x1 = xdbl[i1] - xdbl[i2]; if(x2 < 0) { x1 -= one_bit; x2 += 1; } xdbl[i3] = x2
	for(; k <= kmax; k += 12) {
		double y1 = xdbl[7] - xdbl[0] - carry;
		double y2, y3;
		RANLUX_STEP(y2, y1,  8,  1,  0);
		RANLUX_STEP(y3, y2,  9,  2,  1);
		RANLUX_STEP(y1, y3, 10,  3,  2);
		RANLUX_STEP(y2, y1, 11,  4,  3);
		RANLUX_STEP(y3, y2,  0,  5,  4);
		RANLUX_STEP(y1, y3,  1,  6,  5);
		RANLUX_STEP(y2, y1,  2,  7,  6);
		RANLUX_STEP(y3, y2,  3,  8,  7);
		RANLUX_STEP(y1, y3,  4,  9,  8);
		RANLUX_STEP(y2, y1,  5, 10,  9);
		RANLUX_STEP(y3, y2,  6, 11, 10);
		xdbl[11] = DoCarry(y3);
	}
	#undef RANLUX_STEP
	return k;
}

void SRngRANLUX::IncrementState()
{
	IS_Loop3(IS_Loop2(IS_Loop1(0), pr - 12), pr);
	if(Alg == algRANLUX_S) {
		ydbl[ir] = xdbl[ir] + shift;
		{
			for(int k = NEXT_CYCLE_12(ir); k > 0; k = NEXT_CYCLE_12(k))
				ydbl[k] = xdbl[k] + shift;
		}
		{
			for(int k = 0, m = 0; k < 12; ++k) {
				double x = xdbl[k];
				double y = ydbl[k] - shift;
				if(y > x)
					y -= (1.0 / 16777216.0); // (1.0 / 16777216.0) == 1/2^24
				xflt[m++] = (float)((x - y) * 16777216.0); // 16777216.0 == 2^24
				xflt[m++] = (float)y;
			}
		}
		is = 2 * ir;
		is_old = 2 * ir;
	}
	else if(Alg == algRANLUX_D)
		ir_old = ir;
}

void SRngRANLUX::Set(ulong s)
{
	int    xbit[31];
	SETIFZ(s, 1); // default seed is 1
	{
		long   seed = s;
		for(int k = 0, i = seed & 0xFFFFFFFFUL; k < 31; ++k, i /= 2)
			xbit[k] = i % 2;
	}
	for(int k = 0, ibit = 0, jbit = 18; k < 12; ++k) {
		double x = 0;
		for(int m = 1; m <= 48; ++m) {
			if(Alg == algRANLUX_S)
				x += x + (double)xbit[ibit];
			else // algRANLUX_D
				x += x + (double)((xbit[ibit] + 1) % 2);
			xbit[ibit] = (xbit[ibit] + xbit[jbit]) % 2;
			ibit = (ibit + 1) % 31;
			jbit = (jbit + 1) % 31;
		}
		xdbl[k] = one_bit * x;
	}
	if(Alg == algRANLUX_S) {
		ir = 0;
		jr = 7;
		is = 23;
		is_old = 0;
	}
	else { // algRANLUX_D
		ir = 11;
		jr = 7;
		ir_old = 0;
	}
	carry = 0;
	pr = Luxury;
}

double SRngRANLUX::GetReal()
{
	if(Alg == algRANLUX_S) {
		const uint _is = NEXT_CYCLE_24(is);
		is = _is;
		if(_is == is_old)
			IncrementState();
		return xflt[is];
	}
	else { // Alg == algRANLUX_D
		ir = NEXT_CYCLE_12(ir);
		if(ir == ir_old)
			IncrementState();
		return xdbl[ir];
	}
}

ulong SRngRANLUX::Get()
{
	return static_cast<ulong>(GetReal() * IMul);
}
//
//
//
/*static*/SRng * SRng::CreateInstance(Algorithm alg, uint level)
{
	SRng * p_rng = 0;
	if(oneof3(alg, algBSD, algLibC5, algGLibC2))
		p_rng = new SRngUnix(alg, level);
	else if(oneof2(alg, algRANLUX_S, algRANLUX_D))
		p_rng = new SRngRANLUX(alg, level);
	else if(alg == algMT)
		p_rng = new SRngMT(alg, level);
	CALLPTRMEMB(p_rng, Set(0 /* default seed */));
	return p_rng;
}
//
// Distribution
//
double SRng::GetGaussian(double sigma)
{
	double x, y, r2;
	do {
		// choose x,y in uniform square (-1,-1) to (+1,+1) 
		x = -1 + 2 * GetUniformPos();
		y = -1 + 2 * GetUniformPos();
		// see if it is in the unit circle 
		r2 = x * x + y * y;
	} while(r2 > 1.0 || r2 == 0);
	// Box-Muller transform 
	return sigma * y * sqrt(-2.0 * log(r2) / r2);
}

double SRng::GetGaussianPdf(double x, double sigma) const
{
	double u = x / fabs(sigma);
	double p = (1 / (sqrt(SMathConst::Pi2) * fabs(sigma))) * exp(-u * u / 2);
	return p;
}
//
//
//
double SRng::GetGaussianZiggurat(double sigma)
{
	const double PARAM_R = 3.44428647676; // position of right-most step
	/* tabulated values for the heigt of the Ziggurat levels */
	static const double ytab[128] = {
		1, 0.963598623011, 0.936280813353, 0.913041104253,
		0.892278506696, 0.873239356919, 0.855496407634, 0.838778928349,
		0.822902083699, 0.807732738234, 0.793171045519, 0.779139726505,
		0.765577436082, 0.752434456248, 0.739669787677, 0.727249120285,
		0.715143377413, 0.703327646455, 0.691780377035, 0.68048276891,
		0.669418297233, 0.65857233912, 0.647931876189, 0.637485254896,
		0.62722199145, 0.617132611532, 0.607208517467, 0.597441877296,
		0.587825531465, 0.578352913803, 0.569017984198, 0.559815170911,
		0.550739320877, 0.541785656682, 0.532949739145, 0.524227434628,
		0.515614886373, 0.507108489253, 0.498704867478, 0.490400854812,
		0.482193476986, 0.47407993601, 0.466057596125, 0.458123971214,
		0.450276713467, 0.442513603171, 0.434832539473, 0.427231532022,
		0.419708693379, 0.41226223212, 0.404890446548, 0.397591718955,
		0.390364510382, 0.383207355816, 0.376118859788, 0.369097692334,
		0.362142585282, 0.355252328834, 0.348425768415, 0.341661801776,
		0.334959376311, 0.328317486588, 0.321735172063, 0.31521151497,
		0.308745638367, 0.302336704338, 0.29598391232, 0.289686497571,
		0.283443729739, 0.27725491156, 0.271119377649, 0.265036493387,
		0.259005653912, 0.253026283183, 0.247097833139, 0.241219782932,
		0.235391638239, 0.229612930649, 0.223883217122, 0.218202079518,
		0.212569124201, 0.206983981709, 0.201446306496, 0.195955776745,
		0.190512094256, 0.185114984406, 0.179764196185, 0.174459502324,
		0.169200699492, 0.1639876086, 0.158820075195, 0.153697969964,
		0.148621189348, 0.143589656295, 0.138603321143, 0.133662162669,
		0.128766189309, 0.123915440582, 0.119109988745, 0.114349940703,
		0.10963544023, 0.104966670533, 0.100343857232, 0.0957672718266,
		0.0912372357329, 0.0867541250127, 0.082318375932, 0.0779304915295,
		0.0735910494266, 0.0693007111742, 0.065060233529, 0.0608704821745,
		0.056732448584, 0.05264727098, 0.0486162607163, 0.0446409359769,
		0.0407230655415, 0.0368647267386, 0.0330683839378, 0.0293369977411,
		0.0256741818288, 0.0220844372634, 0.0185735200577, 0.0151490552854,
		0.0118216532614, 0.00860719483079, 0.00553245272614, 0.00265435214565
	};
	// tabulated values for 2^24 times x[i]/x[i+1],
 	// used to accept for U*x[i+1]<=x[i] without any floating point operations
	static const ulong ktab[128] = {
		0, 12590644, 14272653, 14988939,
		15384584, 15635009, 15807561, 15933577,
		16029594, 16105155, 16166147, 16216399,
		16258508, 16294295, 16325078, 16351831,
		16375291, 16396026, 16414479, 16431002,
		16445880, 16459343, 16471578, 16482744,
		16492970, 16502368, 16511031, 16519039,
		16526459, 16533352, 16539769, 16545755,
		16551348, 16556584, 16561493, 16566101,
		16570433, 16574511, 16578353, 16581977,
		16585398, 16588629, 16591685, 16594575,
		16597311, 16599901, 16602354, 16604679,
		16606881, 16608968, 16610945, 16612818,
		16614592, 16616272, 16617861, 16619363,
		16620782, 16622121, 16623383, 16624570,
		16625685, 16626730, 16627708, 16628619,
		16629465, 16630248, 16630969, 16631628,
		16632228, 16632768, 16633248, 16633671,
		16634034, 16634340, 16634586, 16634774,
		16634903, 16634972, 16634980, 16634926,
		16634810, 16634628, 16634381, 16634066,
		16633680, 16633222, 16632688, 16632075,
		16631380, 16630598, 16629726, 16628757,
		16627686, 16626507, 16625212, 16623794,
		16622243, 16620548, 16618698, 16616679,
		16614476, 16612071, 16609444, 16606571,
		16603425, 16599973, 16596178, 16591995,
		16587369, 16582237, 16576520, 16570120,
		16562917, 16554758, 16545450, 16534739,
		16522287, 16507638, 16490152, 16468907,
		16442518, 16408804, 16364095, 16301683,
		16207738, 16047994, 15704248, 15472926
	};
	// tabulated values of 2^{-24}*x[i]
	static const double wtab[128] = {
		1.62318314817e-08, 2.16291505214e-08, 2.54246305087e-08, 2.84579525938e-08,
		3.10340022482e-08, 3.33011726243e-08, 3.53439060345e-08, 3.72152672658e-08,
		3.8950989572e-08, 4.05763964764e-08, 4.21101548915e-08, 4.35664624904e-08,
		4.49563968336e-08, 4.62887864029e-08, 4.75707945735e-08, 4.88083237257e-08,
		5.00063025384e-08, 5.11688950428e-08, 5.22996558616e-08, 5.34016475624e-08,
		5.44775307871e-08, 5.55296344581e-08, 5.65600111659e-08, 5.75704813695e-08,
		5.85626690412e-08, 5.95380306862e-08, 6.04978791776e-08, 6.14434034901e-08,
		6.23756851626e-08, 6.32957121259e-08, 6.42043903937e-08, 6.51025540077e-08,
		6.59909735447e-08, 6.68703634341e-08, 6.77413882848e-08, 6.8604668381e-08,
		6.94607844804e-08, 7.03102820203e-08, 7.11536748229e-08, 7.1991448372e-08,
		7.2824062723e-08, 7.36519550992e-08, 7.44755422158e-08, 7.52952223703e-08,
		7.61113773308e-08, 7.69243740467e-08, 7.77345662086e-08, 7.85422956743e-08,
		7.93478937793e-08, 8.01516825471e-08, 8.09539758128e-08, 8.17550802699e-08,
		8.25552964535e-08, 8.33549196661e-08, 8.41542408569e-08, 8.49535474601e-08,
		8.57531242006e-08, 8.65532538723e-08, 8.73542180955e-08, 8.8156298059e-08,
		8.89597752521e-08, 8.97649321908e-08, 9.05720531451e-08, 9.138142487e-08,
		9.21933373471e-08, 9.30080845407e-08, 9.38259651738e-08, 9.46472835298e-08,
		9.54723502847e-08, 9.63014833769e-08, 9.71350089201e-08, 9.79732621669e-08,
		9.88165885297e-08, 9.96653446693e-08, 1.00519899658e-07, 1.0138063623e-07,
		1.02247952126e-07, 1.03122261554e-07, 1.04003996769e-07, 1.04893609795e-07,
		1.05791574313e-07, 1.06698387725e-07, 1.07614573423e-07, 1.08540683296e-07,
		1.09477300508e-07, 1.1042504257e-07, 1.11384564771e-07, 1.12356564007e-07,
		1.13341783071e-07, 1.14341015475e-07, 1.15355110887e-07, 1.16384981291e-07,
		1.17431607977e-07, 1.18496049514e-07, 1.19579450872e-07, 1.20683053909e-07,
		1.21808209468e-07, 1.2295639141e-07, 1.24129212952e-07, 1.25328445797e-07,
		1.26556042658e-07, 1.27814163916e-07, 1.29105209375e-07, 1.30431856341e-07,
		1.31797105598e-07, 1.3320433736e-07, 1.34657379914e-07, 1.36160594606e-07,
		1.37718982103e-07, 1.39338316679e-07, 1.41025317971e-07, 1.42787873535e-07,
		1.44635331499e-07, 1.4657889173e-07, 1.48632138436e-07, 1.50811780719e-07,
		1.53138707402e-07, 1.55639532047e-07, 1.58348931426e-07, 1.61313325908e-07,
		1.64596952856e-07, 1.68292495203e-07, 1.72541128694e-07, 1.77574279496e-07,
		1.83813550477e-07, 1.92166040885e-07, 2.05295471952e-07, 2.22600839893e-07
	};
	int    sign;
	double x, y;
	while(1) {
		ulong i = GetUniformInt(256); // choose the step
		ulong j = GetUniformInt(16777216); // sample from 2^24
		sign = (i & 0x80) ? +1 : -1;
		i &= 0x7f;
		x = j * wtab[i];
		if(j < ktab[i])
			break;
		if(i < 127) {
			double y0 = ytab[i];
			double y1 = ytab[i+1];
			double U1 = GetReal();
			y = y1 + (y0 - y1) * U1;
		}
		else {
			double U1 = 1.0 - GetReal();
			double U2 = GetReal();
			x = PARAM_R - log(U1) / PARAM_R;
			y = exp(-PARAM_R * (x - 0.5 * PARAM_R)) * U2;
		}
		if(y < exp(-0.5 * x * x))
			break;
	}
	return sign * sigma * x;
}
//
// The Gamma distribution of order a>0 is defined by:
//
// p(x) dx = {1 / \Gamma(a) b^a } x^{a-1} e^{-x/b} dx
//
// for x>0.  If X and Y are independent gamma-distributed random
// variables of order a1 and a2 with the same scale parameter b, then
// X+Y has gamma distribution of order a1+a2.
//
// The algorithms below are from Knuth, vol 2, 2nd ed, p. 129. */
//
double SRng::GetGammaKnuth(double a, double b)
{
	/* assume a > 0 */
	uint na = static_cast<uint>(floor(a));
	if(a == na)
		return b * GetGammaInt(na);
	else if(na == 0)
		return b * GetGammaFrac(a);
	else
		return b * (GetGammaInt(na) + GetGammaFrac(a - na));
}

double SRng::GetGammaInt(uint a)
{
	if(a < 12) {
		double prod = 1;
		for(uint i = 0; i < a; i++)
			prod *= GetUniformPos();
		/* Note: for 12 iterations we are safe against underflow, since
		the smallest positive random number is O(2^-32). This means
		the smallest possible product is 2^(-12*32) = 10^-116 which
		is within the range of double precision. */
		return -log(prod);
	}
	else {
		// Works only if a > 1, and is most efficient if a is large
		//
		// This algorithm, reported in Knuth, is attributed to Ahrens.  A
		// faster one, we are told, can be found in: J. H. Ahrens and
		// U. Dieter, Computing 12 (1974) 223-246.
		//
		double a_1 = static_cast<double>(a-1);
		double x, y, v;
		double sqa = sqrt(2 * a_1);
		do {
			do {
				y = tan(SMathConst::Pi * GetReal());
				x = sqa * y + a_1;
			} while(x <= 0);
			v = GetReal();
		} while(v > (1 + y * y) * exp(a_1 * log(x / a_1) - sqa * y));
		return x;
	}
}

double SRng::GetGammaFrac(double a)
{
	// This is exercise 16 from Knuth; see page 135, and the solution is on page 551.
	double q, x;
	double p = SMathConst::E / (a + SMathConst::E);
	do {
		double u = GetReal();
		double v = GetUniformPos();
		if(u < p) {
			x = exp((1 / a) * log(v));
			q = exp(-x);
		}
		else {
			x = 1 - log(v);
			q = exp((a - 1) * log(x));
		}
	} while(GetReal() >= q);
	return x;
}

double SRng::GetGammaPdf(double x, double a, double b)
{
	if(x < 0)
		return 0;
	else if(x == 0) {
		if(a == 1)
			return 1/b;
		else
			return 0;
	}
	else if(a == 1) {
		return exp(-x/b) / b;
	}
	else {
		SMathResult lngamma;
		flngamma(a, &lngamma);
		return exp((a - 1) * log(x/b) - x/b - lngamma.V)/b;
	}
}

double SRng::GetGamma(double a, double b)
{
	/* assume a > 0 */
	if(a < 1.0) {
		double u = GetUniformPos();
		return GetGamma(1.0 + a, b) * pow(u, 1.0 / a); // @recursion
	}
	else {
		double x, v, u;
		double d = a - 1.0 / 3.0;
		double c = (1.0 / 3.0) / sqrt (d);
		while(1) {
			do {
				x = GetGaussianZiggurat(1.0);
				v = 1.0 + c * x;
			} while(v <= 0);
			v = v * v * v;
			u = GetUniformPos();
			if(u < 1 - 0.0331 * SQ(x) * SQ(x))
				break;
			if(log(u) < 0.5 * SQ(x) + d * (1 - v + log(v)))
				break;
		}
		return b * d * v;
	}
}
//
//	The binomial distribution has the form,
//
//	f(x) =  n!/(x!(n-x)!) * p^x (1-p)^(n-x) for integer 0 <= x <= n
//    	=  0                               otherwise
//
//	This implementation follows the public domain ranlib function
//	"ignbin", the bulk of which is the BTPE (Binomial Triangle
//	Parallelogram Exponential) algorithm introduced in
//	Kachitvichyanukul and Schmeiser[1].  It has been translated to use
//	modern C coding standards.

//	If n is small and/or p is near 0 or near 1 (specifically, if
//	n*min(p,1-p) < SMALL_MEAN), then a different algorithm, called
//	BINV, is used which has an average runtime that scales linearly with n*min(p,1-p).
//
//	But for larger problems, the BTPE algorithm takes the form of two
//	functions b(x) and t(x) -- "bottom" and "top" -- for which b(x) <
//	f(x)/f(M) < t(x), with M = floor(n*p+p).  b(x) defines a triangular
//	region, and t(x) includes a parallelogram and two tails.  Details
//	(including a nice drawing) are in the paper.
//
//	[1] Kachitvichyanukul, V. and Schmeiser, B. W.  Binomial Random
//	Variate Generation.  Communications of the ACM, 31, 2 (February, 1988) 216.
//
//	Note, Bruce Schmeiser (personal communication) points out that if
//	you want very fast binomial deviates, and you are happy with
//	approximate results, and/or n and n*p are both large, then you can
//	just use gaussian estimates: mean=n*p, variance=n*p*(1-p).
//
//	This implementation by James Theiler, April 2003, after obtaining
//	permission -- and some good advice -- from Drs. Kachitvichyanukul
//	and Schmeiser to use their code as a starting point, and then doing
//	a little bit of tweaking.
//
//	Additional polishing for GSL coding standards by Brian Gough.
//

#define SMALL_MEAN    14 // If n*p < SMALL_MEAN then use BINV algorithm. The ranlib implementation used cutoff=30; but
	// on my computer 14 works better
#define BINV_CUTOFF  110 // In BINV, do not permit ix too large
#define FAR_FROM_MEAN 20 // If ix-n*p is larger than this, then use the "squeeze" algorithm.
	// Ranlib used 20, and this seems to be the best choice on my machine as well

#define LNFACT(x) gsl_sf_lnfact(x)

inline static double Stirling(double y1)
{
	double y2 = y1 * y1;
	return (13860.0 - (462.0 - (132.0 - (99.0 - 140.0 / y2) / y2) / y2) / y2) / y1 / 166320.0;
}

ulong SRng::GetBinomial(double p, ulong n)
{
	int    ix; // return value
	int    flipped = 0;
	if(n == 0)
		return 0;
	if(p > 0.5) {
		p = 1.0 - p; // work with small p
		flipped = 1;
	}
	double q = 1 - p;
	double s = p / q;
	double np = n * p;
	//
	// Inverse cdf logic for small mean (BINV in K+S)
	//
	if(np < SMALL_MEAN) {
		double f0 = fpowi(q, n); /* f(x), starting with x=0 */
		while(1) {
			//
			// This while(1) loop will almost certainly only loop once; but if u=1 to within a few epsilons
			// of machine precision, then it is possible for roundoff to prevent the main loop over ix to
			// achieve its proper value. following the ranlib implementation, we introduce a check for that
			// situation, and when it occurs, we just try again.
			//
			double f = f0;
			double u = GetReal();
			for(ix = 0; ix <= BINV_CUTOFF; ++ix) {
				if(u < f)
					goto Finish;
				u -= f;
				/* Use recursion f(x+1) = f(x)*[(n-x)/(x+1)]*[p/(1-p)] */
				f *= s * (n - ix) / (ix + 1);
			}
			/* It should be the case that the 'goto Finish' was encountered
			* before this point was ever reached.  But if we have reached
			* this point, then roundoff has prevented u from decreasing
			* all the way to zero.  This can happen only if the initial u
			* was very nearly equal to 1, which is a rare situation.  In
			* that rare situation, we just try again.
			*
			* Note, following the ranlib implementation, we loop ix only to
			* a hardcoded value of SMALL_MEAN_LARGE_N=110; we could have
			* looped to n, and 99.99...% of the time it won't matter.  This
			* choice, I think is a little more robust against the rare
			* roundoff error.  If n>LARGE_N, then it is technically
			* possible for ix>LARGE_N, but it is astronomically rare, and
			* if ix is that large, it is more likely due to roundoff than
			* probability, so better to nip it at LARGE_N than to take a
			* chance that roundoff will somehow conspire to produce an even
			* larger (and more improbable) ix.  If n<LARGE_N, then once
			* ix=n, f=0, and the loop will continue until ix=LARGE_N.
			*/
		}
	}
	else {
		/* For n >= SMALL_MEAN, we invoke the BTPE algorithm */
		int    k;
		double ffm = np + p; /* ffm = n*p+p             */
		int    m = static_cast<int>(ffm); //m = int floor[n*p+p]
		double fm = m; /* fm = double m;          */
		double xm = fm + 0.5; /* xm = half integer mean (tip of triangle)  */
		double npq = np * q; /* npq = n*p*q            */

		/* Compute cumulative area of tri, para, exp tails */

		/* p1: radius of triangle region; since height=1, also: area of region */
		/* p2: p1 + area of parallelogram region */
		/* p3: p2 + area of left tail */
		/* p4: p3 + area of right tail */
		/* pi/p4: probability of i'th area (i=1,2,3,4) */

		/* Note: magic numbers 2.195, 4.6, 0.134, 20.5, 15.3 */
		/* These magic numbers are not adjustable...at least not easily! */

		double p1 = floor(2.195 * sqrt(npq) - 4.6 * q) + 0.5;

		/* xl, xr: left and right edges of triangle */
		double xl = xm - p1;
		double xr = xm + p1;

		/* Parameter of exponential tails */
		/* Left tail:  t(x) = c*exp(-lambda_l*[xl - (x+0.5)]) */
		/* Right tail: t(x) = c*exp(-lambda_r*[(x+0.5) - xr]) */

		double c = 0.134 + 20.5 / (15.3 + fm);
		double p2 = p1 * (1.0 + c + c);

		double al = (ffm - xl) / (ffm - xl * p);
		double lambda_l = al * (1.0 + 0.5 * al);
		double ar = (xr - ffm) / (xr * q);
		double lambda_r = ar * (1.0 + 0.5 * ar);
		double p3 = p2 + c / lambda_l;
		double p4 = p3 + c / lambda_r;

		double var, accept;
TryAgain:
		/* generate random variates, u specifies which region: Tri, Par, Tail */
		double u = GetReal() * p4; // random variates
		double v = GetReal();      // random variates
		if(u <= p1) {
			/* Triangular region */
			ix = (int)(xm - p1 * v + u);
			goto Finish;
		}
		else if(u <= p2) {
			/* Parallelogram region */
			double x = xl + (u - p1) / c;
			v = v * c + 1.0 - fabs(x - xm) / p1;
			if(v > 1.0 || v <= 0.0)
				goto TryAgain;
			ix = (int)x;
		}
		else if(u <= p3) {
			/* Left tail */
			ix = (int)(xl + log(v) / lambda_l);
			if(ix < 0)
				goto TryAgain;
			v *= ((u - p2) * lambda_l);
		}
		else {
			/* Right tail */
			ix = (int)(xr - log (v) / lambda_r);
			if(ix > (double)n)
				goto TryAgain;
			v *= ((u - p3) * lambda_r);
		}
		//
		// At this point, the goal is to test whether v <= f(x)/f(m)
		//
		// v <= f(x)/f(m) = (m!(n-m)! / (x!(n-x)!)) * (p/q)^{x-m}
		//
		// Here is a direct test using logarithms.  It is a little
		// slower than the various "squeezing" computations below, but
		// if things are working, it should give exactly the same answer
		// (given the same random number seed).
		//
#ifdef DIRECT
		var = log(v);
		accept = LNFACT(m) + LNFACT(n - m) - LNFACT(ix) - LNFACT(n - ix) + (ix - m) * log (p / q);
#else /* SQUEEZE METHOD */

		/* More efficient determination of whether v < f(x)/f(M) */
		k = abs(ix - m);
		if(k <= FAR_FROM_MEAN) {
			//
			// If ix near m (ie, |ix-m|<FAR_FROM_MEAN), then do
			// explicit evaluation using recursion relation for f(x)
			//
			double g = (n + 1) * s;
			double f = 1.0;
			var = v;
			if(m < ix) {
				for(int i = m + 1; i <= ix; i++)
					f *= (g / i - s);
			}
			else if(m > ix) {
				for(int i = ix + 1; i <= m; i++)
					f /= (g / i - s);
			}
			accept = f;
		}
		else {
			/* If ix is far from the mean m: k=ABS(ix-m) large */
			var = log(v);
			if(k < npq / 2 - 1) {
				// "Squeeze" using upper and lower bounds on log(f(x)) The squeeze condition was derived
				// under the condition k < npq/2-1
				double amaxp = k / npq * ((k * (k / 3.0 + 0.625) + (1.0 / 6.0)) / npq + 0.5);
				double ynorm = -(k * k / (2.0 * npq));
				if(var < ynorm - amaxp)
					goto Finish;
				if(var > ynorm + amaxp)
					goto TryAgain;
			}
			// Now, again: do the test log(v) vs. log f(x)/f(M)
#if USE_EXACT
			//
			// This is equivalent to the above, but is a little (~20%) slower */
			// There are five log's vs three above, maybe that's it?
			accept = LNFACT(m) + LNFACT(n - m) - LNFACT(ix) - LNFACT(n - ix) + (ix - m) * log (p / q);
#else /* USE STIRLING */
			// The "#define Stirling" above corresponds to the first five terms in asymptoic formula for
			// log Gamma (y) - (y-0.5)log(y) + y - 0.5 log(2*pi);
			// See Abramowitz and Stegun, eq 6.1.40
			//
			// Note below: two Stirling's are added, and two are subtracted. In both K+S, and in the ranlib
			// implementation, all four are added.  I (jt) believe that is a mistake -- this has been confirmed by personal
			// correspondence w/ Dr. Kachitvichyanukul.  Note, however, the corrections are so small, that I couldn't find an
			// example where it made a difference that could be observed, let alone tested.  In fact, define'ing Stirling
			// to be zero gave identical results!!  In practice, alv is O(1), ranging 0 to -10 or so, while the Stirling
			// correction is typically O(10^{-5}) ...setting the correction to zero gives about a 2% performance boost;
			// might as well keep it just to be pendantic.
			//
			{
				double x1 = ix + 1.0;
				double w1 = n - ix + 1.0;
				double f1 = fm + 1.0;
				double z1 = n + 1.0 - fm;
				accept = xm * log(f1 / x1) + (n - m + 0.5) * log(z1 / w1) + (ix - m) * log(w1 * p / (x1 * q))
					+ Stirling(f1) + Stirling(z1) - Stirling(x1) - Stirling(w1);
			}
#endif
#endif
		}
		if(var <= accept)
			goto Finish;
		else
			goto TryAgain;
	}
Finish:
	return (flipped) ? (n - ix) : (ulong)ix;
}

ulong SRng::GetPoisson(double mu)
{
	double emu;
	double prod = 1.0;
	ulong  k = 0;
	while(mu > 10) {
		ulong  m = static_cast<ulong>(mu * (7.0 / 8.0));
		double X = GetGammaInt(m);
		if(X >= mu)
			return (k + GetBinomial(mu / X, m - 1));
		else {
			k  += m;
			mu -= X;
		}
	}
	// This following method works well when mu is small
	emu = exp(-mu);
	do {
		prod *= GetReal();
		k++;
	} while(prod > emu);
	return (k - 1);
}

/*
void gsl_ran_poisson_array(const gsl_rng * r, size_t n, ulong array[], double mu)
{
	for(size_t i = 0; i < n; i++)
		array[i] = gsl_ran_poisson(r, mu);
}
*/

double gsl_ran_poisson_pdf(ulong k, double mu)
{
	SMathResult lf;
	flnfact(k, &lf);
	return exp(log(mu) * k - lf.V - mu);
}
//
//
//
SRandGenerator::SRandGenerator(SRng::Algorithm alg, uint level) : P_Inner(SRng::CreateInstance(alg, level))
{
}

SRandGenerator::~SRandGenerator()
{
	ZDELETE(P_Inner);
}

void SRandGenerator::Set(ulong seed)
{
	CALLPTRMEMB(P_Inner, Set(seed));
}

ulong  SRandGenerator::Get() { return P_Inner ? P_Inner->Get() : 0U; }
double SRandGenerator::GetReal() { return P_Inner ? P_Inner->GetReal() : 0.0; }
ulong  SRandGenerator::GetUniformInt(ulong n) { return P_Inner ? P_Inner->GetUniformInt(n) : 0U; }
double SRandGenerator::GetUniformPos() { return P_Inner ? P_Inner->GetUniformPos() : 0.0; }
ulong  SRandGenerator::GetMin() const { return P_Inner ? P_Inner->GetMin() : 0U; }
ulong  SRandGenerator::GetMax() const { return P_Inner ? P_Inner->GetMax() : 0U; }
double SRandGenerator::GetGaussian(double sigma) { return P_Inner ? P_Inner->GetGaussian(sigma) : 0.0; }
double SRandGenerator::GetGaussianZiggurat(double sigma) { return P_Inner ? P_Inner->GetGaussianZiggurat(sigma) : 0.0; }
double SRandGenerator::GetGaussianPdf(double x, double sigma) { return P_Inner ? P_Inner->GetGaussianPdf(x, sigma) : 0.0; }
ulong  SRandGenerator::GetBinomial(double p, ulong n) { return P_Inner ? P_Inner->GetBinomial(p, n) : 0U; }
ulong  SRandGenerator::GetPoisson(double mu) { return P_Inner ? P_Inner->GetPoisson(mu) : 0U; }
// double SRandGenerator::GetPoissonPdf(ulong k) { return P_Inner ? P_Inner->GetPoissonPdf(k) : 0.0; }
double SRandGenerator::GetGamma(double a, double b) { return P_Inner ? P_Inner->GetGamma(a, b) : 0.0; }
double SRandGenerator::GetGammaInt(uint a) { return P_Inner ? P_Inner->GetGammaInt(a) : 0.0; }
double SRandGenerator::GetGammaPdf(double x, double a, double b) { return P_Inner ? P_Inner->GetGammaPdf(x, a, b) : 0.0; }

void SRandGenerator::ObfuscateBuffer(void * pBuffer, size_t bufferSize) const
{
	assert(P_Inner);
	if(P_Inner) {
		size_t p = 0;
		while(p < bufferSize) {
			const size_t rest = (bufferSize - p);
			if((rest % 4) == 0) {
				PTR32(PTR8(pBuffer)+p)[0] = P_Inner->Get();
				p += 4;
			}
			else {
                PTR8(pBuffer)[p] = static_cast<uint8>(P_Inner->GetUniformInt(256));
                p++;
			}
		}
		assert(p == bufferSize);
	}
}

int SRandGenerator::GetProbabilityEvent(double prob)
{
	assert(prob >= 0.0 && prob <= 1.0);
	int    result = 0;
	if(P_Inner) {
		const double rv = P_Inner->GetReal();		
		if(rv <= prob)
			result = 1;
	}
	return result;
}
//
// TEST
//
#if SLTEST_RUNNING // {

#define N   10000
#define N2 200000

static void TestRng(STestCase * pCase, SRng::Algorithm alg, uint level, ulong seed, ulong n, ulong expectation)
{
	SRng * p_rng = SRng::CreateInstance(alg, level);
	ulong  result = 0;
	p_rng->Set(seed);
	for(uint i = 0; i < n; i++)
		result = p_rng->Get();
	pCase->SLTEST_CHECK_EQ(result, expectation);
	delete p_rng;
}

static void TestRngFloat(STestCase * pCase, SRng::Algorithm alg, uint level)
{
	SRng * p_ri = SRng::CreateInstance(alg, level);
	SRng * p_rf = SRng::CreateInstance(alg, level);
	double u, c;
	ulong  k = 0;
	do {
		k = p_ri->Get();
		u = p_rf->Get();
	} while(k == 0);
	c = k / u;
	for(uint i = 0; i < N2; i++) {
		k = p_ri->Get();
		u = p_rf->Get();
		pCase->SLTEST_CHECK_EQ((c * k), u);
	}
	delete p_ri;
	delete p_rf;
}

static void TestRngMax(STestCase * pCase, SRng * pRng, ulong * kmax, ulong ran_max)
{
	ulong  actual_uncovered;
	double expect_uncovered;
	ulong  _max = 0;
	for(int i = 0; i < N2; ++i) {
		ulong k = pRng->Get();
		if(k > _max)
			_max = k;
	}
	*kmax = _max;
	actual_uncovered = ran_max - _max;
	expect_uncovered = static_cast<double>(ran_max) / static_cast<double>(N2);
	pCase->SLTEST_CHECK_LE(_max, ran_max);
	pCase->SLTEST_CHECK_LE(static_cast<double>(actual_uncovered), 7 * expect_uncovered);
}

static void TestRngMin(STestCase * pCase, SRng * pRng, ulong * kmin, ulong ran_min, ulong ran_max)
{
	ulong  actual_uncovered;
	double expect_uncovered;
	ulong  _min = 1000000000UL;
	for(int i = 0; i < N2; ++i) {
		ulong k = pRng->Get();
		if(k < _min)
			_min = k;
	}
	*kmin = _min;
	actual_uncovered = _min - ran_min;
	expect_uncovered = (double)ran_max / (double)N2;
	pCase->SLTEST_CHECK_LE(ran_min, _min);
	pCase->SLTEST_CHECK_LE((double)actual_uncovered, 7 * expect_uncovered);
}

static void TestRngSum(STestCase * pCase, SRng * pRng, double * sigma)
{
	double sum = 0;
	for(int i = 0; i < N2; ++i)
		sum += (pRng->GetReal() - 0.5);
	sum /= N2;
	/* expect the average to have a variance of 1/(12 n) */
	*sigma = sum * sqrt(12.0 * N2);
	/* more than 3 sigma is an error (increased to 3.1 to avoid false alarms) */
	pCase->SLTEST_CHECK_CRANGE(fabs(*sigma), 0.003, 3.1);
}

static void TestRngBin(STestCase * pCase, SRng * pRng, double * sigma)
{
	const  size_t BINS  = 17;
	const  size_t EXTRA = 10;
	int    count[BINS+EXTRA];
	double chisq = 0;
	int    i;
	memzero(count, sizeof(count));
	for(i = 0; i < N2; i++) {
		int j = pRng->GetUniformInt(BINS);
		count[j]++;
	}
	chisq = 0 ;
	for(i = 0; i < BINS; i++) {
		double x = (double)N2/(double)BINS ;
		double d = (count[i] - x) ;
		chisq += (d*d) / x;
	}
	*sigma = sqrt(chisq/BINS) ;
	/* more than 3 sigma is an error */
	pCase->SLTEST_CHECK_CRANGE(fabs(*sigma), 0.003, 3);
	for(i = BINS; i < BINS+EXTRA; i++) {
		//
		// GetUniformInt не должен выходить за пределы определенных для него границ (BINS)
		//
		long count_i =count[i];
		pCase->SLTEST_CHECK_EQ(count_i, 0L);
	}
}

static void TestRngGeneric(STestCase * pCase, SRng::Algorithm alg, uint level)
{
	SRng * p_rng = SRng::CreateInstance(alg, level);
	ulong  kmax = 0, kmin = 1000;
	double sigma = 0;
	const  ulong ran_max = p_rng->GetMax();
	const  ulong ran_min = p_rng->GetMin();

	TestRngMax(pCase, p_rng, &kmax, ran_max);
	TestRngMin(pCase, p_rng, &kmin, ran_min, ran_max);
	TestRngSum(pCase, p_rng, &sigma);
	TestRngBin(pCase, p_rng, &sigma);
	p_rng->Set(1);
	TestRngMax(pCase, p_rng, &kmax, ran_max);
	p_rng->Set(1);
	TestRngMin(pCase, p_rng, &kmin, ran_min, ran_max);
	p_rng->Set(1);
	TestRngSum(pCase, p_rng, &sigma);
	p_rng->Set(12345); /* set seed to a "typical" value */
	TestRngMax(pCase, p_rng, &kmax, ran_max);
	p_rng->Set(12345); /* set seed to a "typical" value */
	TestRngMin(pCase, p_rng, &kmin, ran_min, ran_max);
	p_rng->Set(12345); /* set seed to a "typical" value */
	TestRngSum(pCase, p_rng, &sigma);
	delete p_rng;
}

SLTEST_R(RandomNumberGenerator)
{
	struct RngType {
		SRng::Algorithm Alg;
		uint   Level;
	} rng_types[] = {
		{SRng::algRANLUX_S, 0},
		{SRng::algRANLUX_S, 1},
		{SRng::algRANLUX_S, 2},
		{SRng::algRANLUX_D, 1},
		{SRng::algRANLUX_D, 2},

		{SRng::algGLibC2,   8},
		{SRng::algGLibC2,  32},
		{SRng::algGLibC2,  64},
		{SRng::algGLibC2, 128},
		{SRng::algGLibC2, 256},

		{SRng::algBSD,      8},
		{SRng::algBSD,     32},
		{SRng::algBSD,     64},
		{SRng::algBSD,    128},
		{SRng::algBSD,    256},

		{SRng::algLibC5,    8},
		{SRng::algLibC5,   32},
		{SRng::algLibC5,   64},
		{SRng::algLibC5,  128},
		{SRng::algLibC5,  256},

		{SRng::algMT,    0},
		{SRng::algMT, 1999},
		{SRng::algMT, 1998}
	};
#if 0 // {
	TestRng(gsl_rng_rand,       1, 10000, 1910041713);
	TestRng(gsl_rng_randu,      1, 10000, 1623524161);
	TestRng(gsl_rng_cmrg,       1, 10000, 719452880);
	TestRng(gsl_rng_minstd,     1, 10000, 1043618065);
	TestRng(gsl_rng_mrg,        1, 10000, 2064828650);
	TestRng(gsl_rng_taus,       1, 10000, 2733957125UL);
	TestRng(gsl_rng_taus2,      1, 10000, 2733957125UL);
	TestRng(gsl_rng_taus113,    1,  1000, 1925420673UL);
	TestRng(gsl_rng_transputer, 1, 10000, 1244127297UL);
	TestRng(gsl_rng_vax,        1, 10000, 3051034865UL);

	/* Borosh13 test value from PARI: (1812433253^10000)%(2^32) */
	TestRng(gsl_rng_borosh13,   1, 10000, 2513433025UL);

	/* Fishman18 test value from PARI: (62089911^10000)%(2^31-1) */
	TestRng(gsl_rng_fishman18,  1, 10000, 330402013UL);

	/* Fishman2x test value from PARI: ((48271^10000)%(2^31-1) - (40692^10000)%(2^31-249))%(2^31-1) */
	TestRng(gsl_rng_fishman2x,  1, 10000, 540133597UL);

	/* Knuthran2 test value from PARI:
	{ xn1=1; xn2=1; for (n=1,10000, xn = (271828183*xn1 - 314159269*xn2)%(2^31-1);
	xn2=xn1; xn1=xn; print(xn); ) } */
	TestRng(gsl_rng_knuthran2, 1, 10000, 1084477620UL);

	/* Knuthran test value taken from p188 in Knuth Vol 2. 3rd Ed */
	TestRng(gsl_rng_knuthran, 310952, 1009 * 2009 + 1, 461390032);

	/* Knuthran improved test value from Knuth's source */
	TestRng(gsl_rng_knuthran2002, 310952, 1, 708622036);
	TestRng(gsl_rng_knuthran2002, 310952, 2, 1005450560);
	TestRng(gsl_rng_knuthran2002, 310952, 100  * 2009 + 1, 995235265);
	TestRng(gsl_rng_knuthran2002, 310952, 1009 * 2009 + 1, 704987132);

	/* Lecuyer21 test value from PARI: (40692^10000)%(2^31-249) */
	TestRng(gsl_rng_lecuyer21, 1, 10000, 2006618587UL);

	/* Waterman14 test value from PARI: (1566083941^10000)%(2^32) */
	TestRng(gsl_rng_waterman14, 1, 10000, 3776680385UL);

	/* specific tests of known results for 10000 iterations with seed = 6 */

	/* Coveyou test value from PARI: x=6; for(n=1,10000,x=(x*(x+1))%(2^32);print(x);) */
	TestRng(gsl_rng_coveyou, 6, 10000, 1416754246UL);

	/* Fishman20 test value from PARI: (6*48271^10000)%(2^31-1) */
	TestRng(gsl_rng_fishman20, 6, 10000, 248127575UL);

	/* FIXME: the ranlux tests below were made by running the fortran code and
	getting the expected value from that. An analytic calculation would be preferable. */

	TestRng(gsl_rng_ranlux,    314159265, 10000, 12077992);
	TestRng(gsl_rng_ranlux389, 314159265, 10000,   165942);
#endif // } 0
	TestRng(this, SRng::algRANLUX_S, 0, 1, 10000,     11904320); /* 0.709552764892578125 * ldexp(1.0,24) */
	TestRng(this, SRng::algRANLUX_S, 1, 1, 10000,      8734328); /* 0.520606517791748047 * ldexp(1.0,24) */
	TestRng(this, SRng::algRANLUX_S, 2, 1, 10000,      6843140); /* 0.407882928848266602 * ldexp(1.0,24) */
	TestRng(this, SRng::algRANLUX_D, 1, 1, 10000, 1998227290UL); /* 0.465248546261094020 * ldexp(1.0,32) */
	TestRng(this, SRng::algRANLUX_D, 2, 1, 10000, 3949287736UL); /* 0.919515205581550532 * ldexp(1.0,32) */

	TestRng(this, SRng::algGLibC2,   8, 0, 10000, 1910041713);
	TestRng(this, SRng::algGLibC2,  32, 0, 10000, 1587395585);
	TestRng(this, SRng::algGLibC2,  64, 0, 10000, 52848624);
	TestRng(this, SRng::algGLibC2, 128, 0, 10000, 1908609430);
	TestRng(this, SRng::algGLibC2, 256, 0, 10000, 179943260);

	TestRng(this, SRng::algBSD,   8,   0, 10000, 1910041713);
	TestRng(this, SRng::algBSD,  32,   0, 10000, 1663114331);
	TestRng(this, SRng::algBSD,  64,   0, 10000, 864469165);
	TestRng(this, SRng::algBSD, 128,   0, 10000, 1457025928);
	TestRng(this, SRng::algBSD, 256,   0, 10000, 1216357476);

	TestRng(this, SRng::algLibC5,   8, 0, 10000, 1910041713);
	TestRng(this, SRng::algLibC5,  32, 0, 10000, 1967452027);
	TestRng(this, SRng::algLibC5,  64, 0, 10000, 2106639801);
	TestRng(this, SRng::algLibC5, 128, 0, 10000, 428084942);
	TestRng(this, SRng::algLibC5, 256, 0, 10000, 116367984);

	TestRng(this, SRng::algMT,    0, 4357, 1000, 1186927261);
	TestRng(this, SRng::algMT, 1999, 4357, 1000, 1030650439);
	TestRng(this, SRng::algMT, 1998, 4357, 1000, 1309179303);
#if 0 // {
	/* FIXME: the tests below were made by running the original code in the ../random directory
	and getting the expected value from that. An analytic calculation would be preferable. */

	TestRng(gsl_rng_slatec,           1, 10000, 45776);
	TestRng(gsl_rng_uni,              1, 10000, 9214);
	TestRng(gsl_rng_uni32,            1, 10000, 1155229825);
	TestRng(gsl_rng_zuf,              1, 10000, 3970);
	//
	// The tests below were made by running the original code and
	// getting the expected value from that. An analytic calculation would be preferable.
	//
	TestRng(gsl_rng_r250,             1, 10000, 1100653588);
	TestRng(gsl_rng_tt800,            0, 10000, 2856609219UL);
	TestRng(gsl_rng_ran0,             0, 10000, 1115320064);
	TestRng(gsl_rng_ran1,             0, 10000, 1491066076);
	TestRng(gsl_rng_ran2,             0, 10000, 1701364455);
	TestRng(gsl_rng_ran3,             0, 10000, 186340785);
	TestRng(gsl_rng_ranmar,           1, 10000, 14428370);
	TestRng(gsl_rng_rand48,           0, 10000, 0xDE095043UL);
	TestRng(gsl_rng_rand48,           1, 10000, 0xEDA54977UL);
	TestRng(gsl_rng_ranf, 0, 10000, 2152890433UL);
	TestRng(gsl_rng_ranf, 2, 10000, 339327233);
#endif // } 0
	//
	// Test constant relationship between int and double functions
	//
	uint   k;
	for(k = 0; k < sizeof(rng_types) / sizeof(rng_types[0]); k++)
		TestRngFloat(this, rng_types[k].Alg, rng_types[k].Level);
	//
	// generic statistical tests (these are just to make sure that we
	// don't get any crazy results back from the generator, i.e. they
	// aren't a test of the algorithm, just the implementation)
	//
	for(k = 0; k < sizeof(rng_types) / sizeof(rng_types[0]); k++)
		TestRngGeneric(this, rng_types[k].Alg, rng_types[k].Level);
	{
        //
        // Тестирование обфьюскатора
        //
        const uint8 fix_byte = 0xFAU;
        SRandGenerator obf_rng;
        uint8  test_buffer[1024];
        memset(test_buffer, fix_byte, sizeof(test_buffer));
        const size_t prefix_size = 7;
        const size_t suffix_size = 13;
        for(size_t i = prefix_size; i < sizeof(test_buffer) - suffix_size - prefix_size; i++) {
			obf_rng.ObfuscateBuffer(test_buffer + prefix_size, i - prefix_size);
			for(size_t p = 0; p < prefix_size; p++) {
                SLTEST_CHECK_EQ(test_buffer[p], fix_byte);
			}
			for(size_t e = i; e < sizeof(test_buffer); e++) {
				SLTEST_CHECK_EQ(test_buffer[e], fix_byte);
			}
        }
	}
	{
		//
		// Тестирование функции SRandGenerator::GetProbabilityEvent
		//
		SRandGenerator rg;
		const uint tc_list[]     = { /*1000, 10000,*/ 100000, 1000000 };
		const double prob_list[] = { 0.01, 0.37, 0.701, 0.1223 };
		for(uint tci = 0; tci < SIZEOFARRAY(tc_list); tci++) {
			for(uint pi = 0; pi < SIZEOFARRAY(prob_list); pi++) {
				double prob = prob_list[pi];
				const uint tc = tc_list[tci];
				uint true_result_count = 0;
				for(uint i = 0; i < tc; i++) {
					int ev = rg.GetProbabilityEvent(prob);
					if(ev)
						true_result_count++;
				}
				double result_prob = static_cast<double>(true_result_count) / static_cast<double>(tc);
				SLTEST_CHECK_EQ_TOL(result_prob, prob, 0.001);
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(RNG_SAMPLE_GAUSSIAN)
{
	const STestSuite::Entry * p_entry = GetSuiteEntry();
	SString file_name = p_entry->OutPath;
	file_name.SetLastSlash().Cat("rng_sample_gaussian");
	SFile file(file_name, SFile::mWrite);
	SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
	SString line_buf;
	for(uint i = 0; i < 20000; i++) {
		line_buf.Cat(p_rng->GetGaussian(3.0), MKSFMTD(0, 27, 0)).CR();
	}
	file.WriteLine(line_buf);
	delete p_rng;
	return 1;
}

SLTEST_R(RNG_SAMPLE_POISSON)
{
	const STestSuite::Entry * p_entry = GetSuiteEntry();
	SString file_name = p_entry->OutPath;
	file_name.SetLastSlash().Cat("rng_sample_poisson");
	SFile file(file_name, SFile::mWrite);
	SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
	SString line_buf;
	for(uint i = 0; i < 20000; i++) {
		line_buf.Cat(p_rng->GetPoisson(10.0)).CR();
	}
	file.WriteLine(line_buf);
	delete p_rng;
	return 1;
}

#endif // } SLTEST_RUNNING
