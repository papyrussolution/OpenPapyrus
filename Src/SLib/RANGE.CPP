// RANGE.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

int STDCALL strtorrng(const char * pBuf, double * pLow, double * pUpp)
{
	int    dot = 0;
	char * p_tmp = sstrdup(pBuf);
	char * t = strip(p_tmp);
	{
		//
		// Удаляем из строки разделители: пробелы, запятые и апострофы.
		// Так как иногда пользователи вместо двойной точки вводять двойную
		// запятую (,,), то обрабатываем этот случай отдельно.
		//
		size_t i = sstrlen(p_tmp);
		if(i)
			do {
				i--;
				if(oneof2(p_tmp[i], '\'', ' '))
					strcpy(p_tmp+i, p_tmp+i+1);
				else if(p_tmp[i] == ',')
					p_tmp[i] = '.';
			} while(i);
	}
	char * n;
	while(t[0] && !(t[0] == '.' && t[1] == '.') && !(t[0] == ',' && t[1] == ','))
		t++;
	n = t[0] ? (dot = 1, t + 2) : t;
	*pLow = satof(p_tmp); // @v10.7.9 atof-->satof
	*pUpp = (*strip(n) == 0 && !dot) ? *pLow : satof(n); // @v10.7.9 atof-->satof
	SAlloc::F(p_tmp);
	return 1;
}

int FASTCALL strtorrng(const char * pBuf, RealRange & rRng)
{
	return strtorrng(pBuf, &rRng.low, &rRng.upp);
}
//
//
//
IntRange & IntRange::Z()
{
	low = 0;
	upp = 0;
	return *this;
}

bool IntRange::IsZero() const { return (low == 0 && upp == 0); }
bool FASTCALL IntRange::CheckVal(int test) const { return (low == 0 || test >= low) && (upp == 0 || test <= upp); }

IntRange & FASTCALL IntRange::operator = (const IntRange & rS)
{
	low = rS.low;
	upp = rS.upp;
	return *this;
}

IntRange & FASTCALL IntRange::operator = (int val)
{
	low = upp = val;
	return *this;
}

IntRange & FASTCALL IntRange::Set(int aLow, int aUpp)
{
	low = aLow;
	upp = aUpp;
	return *this;
}

IntRange & FASTCALL IntRange::Set(int val)
{
	low = upp = val;
	return *this;
}

int FASTCALL IntRange::Intersect(const IntRange & rD) const
{
	int    ret = 0;
	if(low <= upp && rD.low <= rD.upp) {
		const int32 c1 = MAX(low, rD.low);
		const int32 c2 = MIN(upp, rD.upp);
		if(c1 < c2)
			ret = 1;
		else if(c1 == c2)
			ret = 2;
	}
	else
		ret = -1;
	return ret;
}

int IntRange::Intersect(const IntRange & rD, IntRange * pResult) const
{
	int    ret = 0;
	int32  c1 = 0;
	int32  c2 = 0;
	if(low <= upp && rD.low <= rD.upp) {
		c1 = MAX(low, rD.low);
		c2 = MIN(upp, rD.upp);
		if(c1 < c2)
			ret = 1;
		else if(c1 == c2)
			ret = 2;
	}
	else
		ret = -1;
	CALLPTRMEMB(pResult, Set(c1, c2));
	return ret;
}

int IntRange::Clamp(int v) const
{
	return sclamp(v, low, upp);
}

SString & IntRange::Format(long fmt, SString & rBuf) const
{
	rBuf.Z();
	if(low)
		rBuf.Cat(low);
	if(upp) {
		if(upp != low)
			rBuf.CatCharN('.', 2).Cat(upp);
	}
	else if(low)
		rBuf.CatCharN('.', 2);
	return rBuf;
}
//
//
//
SSzChunk & SSzChunk::Z()
{
	Begin = 0;
	Len = 0;
	return *this;
}

SSzChunk & SSzChunk::Set(int begin, int len)
{
	Begin = begin;
	Len = len;
	return *this;
}

SSzChunk & SSzChunk::SetRange(int begin, int end)
{
	Begin = begin;
	Len = (end - begin + 1);
	return *this;
}

int SSzChunk::operator !() const { return (Len <= 0); }
int SSzChunk::operator == (const SSzChunk & rS) const { return (Begin == rS.Begin && Len == rS.Len); }
int SSzChunk::GetEnd() const { return (Begin+Len-1); }
int SSzChunk::IsEmpty() const { return BIN(Len <= 0); }
//
//
//
RealRange & RealRange::Z()
{
	low = 0.0;
	upp = 0.0;
	return *this;
}

bool   RealRange::IsNan() const { return _isnan(low) || _isnan(upp); }
bool   RealRange::IsDegraded() const { return (low == upp); }
bool   RealRange::IsPositive() const { return (low > 0.0 && upp > 0.0); }
bool   RealRange::IsNegative() const { return (low < 0.0 && upp < 0.0); }
bool   RealRange::IsNormalized() const { return (low <= upp); }
bool   RealRange::IsStrictlyNormalized() const { return (low < upp); }
RealRange & FASTCALL RealRange::operator = (const IntRange & rS) { return Set((double)rS.low, (double)rS.upp); }
int FASTCALL RealRange::IsEqual(const RealRange & rS) const { return BIN(low == rS.low && upp == rS.upp); }
int FASTCALL RealRange::LessThan(double v) const { return (low < v && upp < v); }
int FASTCALL RealRange::GreaterThan(double v) const { return (low > v && upp > v); }
int  FASTCALL RealRange::operator != (const RealRange & rS) const { return !IsEqual(rS); }
bool RealRange::IsZero() const { return (low == 0.0 && upp == 0.0); }
bool RealRange::CheckVal(double v) const { return (low == 0.0 || v >= low) && (upp == 0.0 || v <= upp); }

bool RealRange::CheckValEps(double val, double eps) const
{
	if(low != 0.0) {
		if(!feqeps(val, low, eps) && val < low)
			return false;
	}
	if(upp != 0.0) {
		if(!feqeps(val, upp, eps) && val > upp)
			return false;
	}
	return true;
}

bool RealRange::Check(double v) const { return (v >= low && v <= upp); }
bool RealRange::CheckX(double v) const { return ((low <= upp) ? (v >= low && v <= upp) : (v >= upp && v <= low)); }

double RealRange::Clip(double v) const
{
	if(low < upp) {
		if(v > upp)
			v = upp;
		else if(v < low)
			v = low;
	}
	else {
		if(v > low)
			v = low;
		else if(v < upp)
			v = upp;
	}
	return v;
}

RealRange & RealRange::SetVal(double v)
{
	low = upp = v;
	return *this;
}

RealRange & RealRange::Set(double lo, double up)
{
	low = lo;
	upp = up;
	return *this;
}

RealRange & RealRange::SetupMinMax(double v)
{
	SETMIN(low, v);
	SETMAX(upp, v);
	return *this;
}

RealRange & RealRange::SetDelta(double lo, double delta)
{
	low = lo;
	upp = lo+delta;
	return *this;
}

RealRange & RealRange::Scale(double sc)
{
	low *= sc;
	upp *= sc;
	return *this;
}

RealRange & RealRange::Normalize()
{
	ExchangeForOrder(&low, &upp);
	return *this;
}

RealRange & RealRange::Extend(double delta)
{
	low = low-delta;
	upp = upp+delta;
	return *this;
}

RealRange & FASTCALL RealRange::Round(int prec)
{
	round(low, prec);
	round(upp, prec);
	return *this;
}

RealRange & RealRange::SetMiddle(double middle, double delta)
{
	low = middle-delta;
	upp = middle+delta;
	return *this;
}

double RealRange::GetMiddle() const { return (low+upp) / 2.0; }
double RealRange::GetDistance() const { return (upp-low); }
double RealRange::GetActualLow() const { return MIN(low, upp); }
double RealRange::GetActualUpp() const { return MAX(low, upp); }

int FASTCALL RealRange::Intersect(const RealRange & rTest, RealRange * pResult) const
{
	int    is = 0;
	double st, fn;
	if(low > rTest.upp || upp < rTest.low) {
		st = 0.0;
		fn = 0.0;
	}
	else {
		st = (low > rTest.low) ? low : rTest.low;
		fn = (upp < rTest.upp) ? upp : rTest.upp;
		is = 1;
	}
	CALLPTRMEMB(pResult, Set(st, fn));
	return is;
}
//
//
//
IMPL_INVARIANT_C(DateRange)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(checkdate(low, 1), pInvP);
	S_ASSERT_P(checkdate(upp, 1), pInvP);
	S_ASSERT_P(low <= upp || upp == ZERODATE, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

int FASTCALL DateRange::IsEqual(const DateRange & rS) const
{
	return (low == rS.low && upp == rS.upp);
}

long DateRange::GetLength() const
{
	long   len = 0;
	if(low == upp) {
		len = (!low) ? (MAXLONG-1) : 0;
	}
	else if(!low)
		len = diffdate(upp, encodedate(1, 1, 0));
	else if(!upp)
		len = (MAXLONG-2);
	else
		len = diffdate(upp, low);
	return len;
}

int FASTCALL DateRange::Cmp(const DateRange & rS) const
{
	int    r = IsEqual(rS);
	if(r)
		return 0;
	else if(low < rS.low)
		return -1;
	else if(low > rS.low)
		return +1;
	else {
		//
		// Нижние границы равны
		//
		// Случай upp == rS.upp здесь не возможен из-за того, что он был охвачен вызовом IsEqual (see above)
		//
		assert(upp != rS.upp); // paranoic assertion
		if(upp) {
			if(rS.upp)
				return (upp < rS.upp) ? -1 : +1;
			else
				return -1; // rS.upp - бесконечность, а this->upp - определенная дата, следовательно this < rS
		}
		else
			return +1; // this->upp - бесконечность, а rS.upp - определенная дата, следовательно this > rS
	}
}

bool DateRange::IsZero() const { return (!low && !upp); }

int FASTCALL DateRange::CheckDate(LDATE dt) const
{
	int r = BIN(dt >= low && (!upp || dt <= upp));
	if(!r) {
		const int lf = low.hasanycomponent();
		const int uf = upp.hasanycomponent();
		if(lf || uf) {
			LDATE temp_low = low;
			LDATE temp_upp = upp;
			if(lf & 0x01)
				temp_low.setyear(dt.year());
			if(lf & 0x02)
				temp_low.setmonth(dt.month());
			if(lf & 0x04)
				temp_low.setday(dt.day());
			if(uf & 0x01)
				temp_upp.setyear(dt.year());
			if(uf & 0x02)
				temp_upp.setmonth(dt.month());
			if(uf & 0x04)
				temp_upp.setday(dt.day());
			r = BIN(dt >= temp_low && (!temp_upp || dt <= temp_upp));
		}
	}
	return r;
}

DateRange & DateRange::Set(LDATE lo, LDATE up)
{
	low = lo;
	upp = up;
	return *this;
}

DateRange & FASTCALL DateRange::Set(const DateRange * pSrc)
{
	if(pSrc) {
		low = pSrc->low;
		upp = pSrc->upp;
		return *this;
	}
	else
		return Z();
}

int DateRange::GetQuart(int * pQuartN, int * pYear) const
{
	int    ok = 0;
	int    q = 0;
	int    y = 0;
	int    lc = checkdate(low);
	int    uc = checkdate(upp);
	if(lc && uc) {
		int    d1, m1, y1;
		int    d2, m2, y2;
		low.decode(&d1, &m1, &y1);
		upp.decode(&d2, &m2, &y2);
		if(y1 == y2 && oneof4(m1, 1, 4, 7, 10) && d1 == 1 && oneof4(m2, 3, 6, 9, 12) && d2 == dayspermonth(m2, y2)) {
			ok = 1;
			y = y1;
			if(m1 == 1)
				q = 1;
			else if(m1 == 4)
				q = 2;
			else if(m1 == 7)
				q = 3;
			else if(m1 == 10)
				q = 4;
			else {
				assert(0); // @paranoic
				ok = 0;
			}
		}
	}
	ASSIGN_PTR(pQuartN, q);
	ASSIGN_PTR(pYear, y);
	return ok;
}

int DateRange::SetPeriod(LDATE dt, int prd)
{
	int    ok = 1;
	if(checkdate(dt)) {
		if(prd == PRD_DAY) {
			Set(dt, dt);
		}
		else if(prd == PRD_WEEK) {
			int    w_day = dayofweek(&dt, 1) - 1;
			low = plusdate(dt, -w_day);
		}
		else if(prd != 0) {
			int    d = 0, m = 0, y = 0;
			decodedate(&d, &m, &y, &dt);
			if(prd == PRD_MONTH)
				d = 1;
			else if(prd == PRD_QUART) {
				d = 1;
				if(m <= 3)
					m = 1;
				else if(m <= 6)
					m = 4;
				else if(m <= 9)
					m = 7;
				else
					m = 10;
			}
			else if(prd == PRD_SEMIAN) {
				d = 1;
				m = (m >= 7) ? 7 : 1;
			}
			else if(prd == PRD_ANNUAL) {
				d = 1;
				m = 1;
			}
			else
				ok = 0;
			low = encodedate(d, m, y);
		}
		if(ok) {
			LDATE  t = plusdate(low, -1);
			plusperiod(&t, prd, 1, 0);
			upp = t;
		}
	}
	else
		ok = 0;
	return ok;
}

DateRange & DateRange::Z()
{
	low = upp = ZERODATE;
	return *this;
}

void FASTCALL DateRange::SetDate(LDATE dt)
{
	low = upp = dt;
}

void DateRange::CheckAndSwap()
{
	if(upp && low > upp)
		Exchange(&low.v, &upp.v);
}

int FASTCALL DateRange::Intersect(const DateRange * pR)
{
	LDATE  b  = MAX(low, pR->low);
	LDATE  e  = NZOR(upp, MAXDATE);
	LDATE  e2 = NZOR(pR->upp, MAXDATE);
	e = MIN(e, e2);
	if(b <= e) {
		low = b;
		upp = (e == MAXDATE) ? ZERODATE : e;
		return 1;
	}
	else
		return 0;
}

int DateRange::IsIntersect(DateRange r) const
{
	DateRange temp = *this;
	return temp.Intersect(&r);
}

int FASTCALL DateRange::AdjustToDate(LDATE dt)
{
	int    ok = -1;
	int    cls = dt.getclass();
	if(cls == LDATE::cNormal) {
		if(low > dt) {
			low = dt;
			ok = 1;
		}
		if(upp && upp < dt) {
			upp = dt;
			ok = 1;
		}
	}
	else if(cls == LDATE::cInvalid)
		ok = 0;
	else
		ok = -2;
	return ok;
}

DateRange & FASTCALL DateRange::Actualize(LDATE rel)
{
	low = low.getactual(rel);
	upp = upp.getactual(rel);
	return *this;
}

DateRange & DateRange::ActualizeCmp(LDATE rel, LDATE cmp)
{
	low = low.getactualcmp(rel, cmp);
	upp = upp.getactualcmp(rel, cmp);
	return *this;
}
//
//
//
void TimeRange::Set(LTIME lowTm, LTIME uppTm)
{
	low = lowTm;
	upp = uppTm;
}

void TimeRange::SetZero()
{
	low = ZEROTIME;
	upp = ZEROTIME;
}

bool TimeRange::IsZero() const { return (low == ZEROTIME && upp == ZEROTIME); }

int FASTCALL TimeRange::Check(LTIME tm) const
{
	if(low == ZEROTIME && upp == ZEROTIME)
		return 1;
	else if(low <= upp)
		return BIN(tm >= low && tm <= upp);
	else {
		const LTIME tm24 = encodetime(24, 0, 0, 0);
		if(tm >= low && tm < tm24)
			return 1;
		else if(tm >= ZEROTIME && tm <= upp)
			return 2;
		else
			return 0;
	}
}

SString & TimeRange::ToStr(long tmFormat, SString & rBuf) const
{
	rBuf.Z();
	if(low != ZEROTIME || upp != ZEROTIME) {
		if(low != ZEROTIME)
			rBuf.Cat(low, tmFormat);
		rBuf.Dot().Dot();
		if(upp != ZEROTIME)
			rBuf.Cat(upp, tmFormat);
	}
	return rBuf;
}
