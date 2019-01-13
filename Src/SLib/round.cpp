// ROUND.CPP
// Copyright (c) A.Sobolev 1996-2000, 2001, 2003, 2004, 2007, 2009, 2010, 2016, 2017, 2018, 2019
// @threadsafe
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <float.h> // chgsign

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
		double down = prec * f_;
		double up   = prec * c_;
		return ((2.0 * v - down - up) >= 0) ? up : down;
	}
}

double FASTCALL R0(double v)  { return implement_round(v, 0); }
long   FASTCALL R0i(double v) { return (long)implement_round(v, 0); }
double FASTCALL R2(double v)  { return implement_round(v, 2); }
double FASTCALL R3(double v)  { return implement_round(v, 3); }
double FASTCALL R4(double v)  { return implement_round(v, 4); }
double FASTCALL R5(double v)  { return implement_round(v, 5); }
double FASTCALL R6(double v)  { return implement_round(v, 6); }

double FASTCALL roundnev(double n, int prec)
{
	double p;
	if(prec == 0) {
		p = floor(n);
		double diff = n - p;
		if(diff < 0.5)
			n = p;
		else if(diff > 0.5)
			n = ceil(n);
		else // diff == 0.5
			if(!(((long)p) & 1))
				n = p;
			else
				n = ceil(n);
	}
	else {
#ifdef _WIN32_WCE
		p = pow(10, prec);
#else
		p = fpow10i(prec);
#endif
		double t = n * p;
		double f = floor(t);
		double diff = t - f;
		if(diff < 0.5)
			n = f / p;
		else if(diff > 0.5)
			n = (ceil(t) / p);
		else { // diff == 0.5
			if(!(((long)f) & 1))
				n = f / p;
			else
				n = (ceil(t) / p);
		}
	}
	return n;
}

double SLAPI trunc(double n, int prec)
{
	double p = 0;
#ifdef _WIN32_WCE
		p = pow(10, prec);
#else
		p = fpow10i(prec);
#endif
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
double FASTCALL inttodbl2(long v)
{
	return implement_round(((double)v) / 100.0, 2);
}

long   FASTCALL dbltoint2(double r)
{
	return (long)implement_round(r * 100.0, 0);
}

double FASTCALL intmnytodbl(long m)
{
	return implement_round(((double)m) / 100.0, 2);
}

long FASTCALL dbltointmny(double r)
{
	return (long)implement_round(r * 100.0, 0);
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

#endif // } SLTEST_RUNNING
