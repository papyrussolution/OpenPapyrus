// RANGE.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2025
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
	*pLow = satof(p_tmp);
	*pUpp = (*strip(n) == 0 && !dot) ? *pLow : satof(n);
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

IntRange & IntRange::Normalize()
{
	SExchangeForOrder(&low, &upp);
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

int STDCALL IntRange::Intersect(const IntRange & rD, IntRange * pResult) const
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
	return sclamp(static_cast<int32>(v), low, upp);
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

bool SSzChunk::operator !() const { return (Len <= 0); }
bool FASTCALL SSzChunk::operator == (const SSzChunk & rS) const { return (Begin == rS.Begin && Len == rS.Len); }
int  SSzChunk::GetEnd() const { return (Begin+Len-1); }
bool SSzChunk::IsEmpty() const { return (Len <= 0); }
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
//RealRange & FASTCALL RealRange::operator = (const RealRange & rS) { return Set(rS.low, rS.upp); }
RealRange & FASTCALL RealRange::operator = (const IntRange & rS) { return Set((double)rS.low, (double)rS.upp); }
bool   FASTCALL RealRange::IsEq(const RealRange & rS) const { return (low == rS.low && upp == rS.upp); }
bool   FASTCALL RealRange::LessThan(double v) const { return (low < v && upp < v); }
bool   FASTCALL RealRange::GreaterThan(double v) const { return (low > v && upp > v); }
bool   FASTCALL RealRange::operator != (const RealRange & rS) const { return !IsEq(rS); }
bool   RealRange::IsZero() const { return (low == 0.0 && upp == 0.0); }
bool   RealRange::CheckVal(double v) const { return (low == 0.0 || v >= low) && (upp == 0.0 || v <= upp); }

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
	SExchangeForOrder(&low, &upp);
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

bool FASTCALL DateRange::IsEq(const DateRange & rS) const
{
	return (low == rS.low && upp == rS.upp);
}

long DateRange::GetLength() const
{
	// @v12.3.8 Внес изменение: длина детерменированного периода должна быть на 1 больше, чем разница дат.
	long   len = 0;
	if(low == upp) {
		len = (!low) ? (MAXLONG-1) : 1; // @v12.3.8 0-->1
	}
	else if(!low)
		len = diffdate(upp, encodedate(1, 1, 0));
	else if(!upp)
		len = (MAXLONG-2);
	else
		len = diffdate(upp, low) + 1; // @v12.3.8 (+1)
	return len;
}

int FASTCALL DateRange::Cmp(const DateRange & rS) const
{
	if(IsEq(rS))
		return 0;
	else if(low < rS.low)
		return -1;
	else if(low > rS.low)
		return +1;
	else {
		//
		// Нижние границы равны
		//
		// Случай upp == rS.upp здесь не возможен из-за того, что он был охвачен вызовом IsEq (see above)
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

bool FASTCALL DateRange::CheckDate(LDATE dt) const
{
	bool result = true;
	if(IsZero()) // @v12.2.3 Если обе границы диапазона не определены, то считаем, что любая дата (даже инвалидная) проходит проверку.
		result = true;
	else {
		result = (dt >= low && (!upp || dt <= upp));
		if(!result) {
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
				result = (dt >= temp_low && (!temp_upp || dt <= temp_upp));
			}
		}
	}
	return result;
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
	if(upp)
		SExchangeForOrder(&low.v, &upp.v);
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

int DateRange::SetPredefined(int predefPrdId, LDATE rel)
{
	int    ok = 1;
	if(!rel)
		rel = getcurdate_();
	if(checkdate(rel)) {
		switch(predefPrdId) {
			case PREDEFPRD_TODAY:
				SetDate(rel);
				break;
			case PREDEFPRD_YESTERDAY:
				SetDate(plusdate(rel, -1));
				break;
			case PREDEFPRD_LAST7DAYS:
				Set(plusdate(rel, -7), plusdate(rel, -1));
				break;
			case PREDEFPRD_LASTWEEK:
				{
					LDATE d1 = plusdate(rel, 1 - (7 + dayofweek(&rel, 1)));
					LDATE d2 = plusdate(d1, 6);
					Set(d1, d2);
				}
				break;
			case PREDEFPRD_LASTWORKWEEK:
				{
					LDATE d1 = plusdate(rel, 1 - (7 + dayofweek(&rel, 1)));
					LDATE d2 = plusdate(d1, 4);
					Set(d1, d2);
				}
				break;
			case PREDEFPRD_THISMONTH:
				{
					LDATE d1;
					LDATE d2;
					(d1 = rel).setday(1);
					(d2 = rel).setday(rel.dayspermonth());
					Set(d1, d2);
				}
				break;
			case PREDEFPRD_LASTMONTH:
				{
					LDATE d = rel;
					plusperiod(&d, PRD_MONTH, -1, 0);
					LDATE d1;
					LDATE d2;
					(d1 = d).setday(1);
					(d2 = d).setday(d.dayspermonth());
					Set(d1, d2);
				}
				break;
			default:
				ok = 0;
		}	
	}
	else
		ok = 0;
	return ok;
}
//
//
//
#define TOK_UNKNOWN   -1
#define TOK_EOL        0
#define TOK_NUMBER     1
#define TOK_QUARTER    2
#define TOK_DELIM      3
#define TOK_DBLDOT     4
#define TOK_DATE       5
/*
#define TOK_THISDAY    6
#define TOK_THISWEEK   7
#define TOK_THISMONTH  8
#define TOK_THISQUART  9
#define TOK_THISYEAR  10
#define TOK_LASTDAY   11
#define TOK_LASTWEEK  12
#define TOK_LASTMONTH 13
#define TOK_LASTQUART 14
#define TOK_LASTYEAR  15
*/

/*
static const char * macro[] = {
	"thisday",
	"thisweek",
	"thismonth",
	"thisquart",
	"thisyear",
	"lastday",
	"lastweek",
	"lastmonth",
	"lastquart",
	"lastyear"
};
*/
/*struct TempVar {
	int    D;
	int    M;
	int    Y;
};*/

static int FASTCALL get_strtoperiod_token(const char ** b, long * pNumber, SUniDate_Internal * pV, long flags)
{
	//static const char * quarters[4] = { "I", "II", "III", "IV" };
	int    result = TOK_UNKNOWN;
	//int    i;
	char   buf[64];
	char * t = buf;
	while(oneof2(**b, ' ', '\t'))
		(*b)++;
	long   ret_flags = 0;
	int    offs = _strtodate(*b, DATF_DMY, pV, &ret_flags);
	if(ret_flags & (strtodatefAnyYear|strtodatefAnyMon|strtodatefAnyDay) && !(flags & strtoprdfEnableAnySign)) {
		result = TOK_UNKNOWN;
	}
	else if(offs > 0 && ((ret_flags & (strtodatefRelAny|strtodatefAnyYear|strtodatefAnyMon|strtodatefAnyDay)) || (ret_flags == 0))) {
		(*b) += offs;
		result = TOK_DATE;
	}
	else if(**b == '.') {
		(*b)++;
		if(**b == '.') {
			(*b)++;
			result = TOK_DBLDOT;
		}
		else
			result = TOK_DELIM;
	}
	else if(**b == ',') {
		(*b)++;
		if(**b == ',') {
			(*b)++;
			result = TOK_DBLDOT;
		}
		else
			result = TOK_UNKNOWN;
	}
	else if(**b == '\0')
		result = TOK_EOL;
	else if(oneof2(**b, '/', '-')) {
		(*b)++;
		result = TOK_DELIM;
	}
	else if(isdec(**b)) {
		do {
			*t++ = *(*b)++;
		} while(isdec(**b));
		*t = 0;
		ASSIGN_PTR(pNumber, atol(buf));
		result = TOK_NUMBER;
	}
	else if(strnicmp(*b, "IV", 2) == 0) {
		ASSIGN_PTR(pNumber, 3);
		(*b) += 2;
		result = TOK_QUARTER;
	}
	else if(strnicmp(*b, "III", 3) == 0) {
		ASSIGN_PTR(pNumber, 2);
		(*b) += 3;
		result = TOK_QUARTER;
	}
	else if(strnicmp(*b, "II", 2) == 0) {
		ASSIGN_PTR(pNumber, 1);
		(*b) += 2;
		result = TOK_QUARTER;
	}
	else if(strnicmp(*b, "I", 1) == 0) {
		ASSIGN_PTR(pNumber, 0);
		(*b) += 1;
		result = TOK_QUARTER;
	}
	else {
		(*b)++;
		result = TOK_UNKNOWN;
	}
	return result;
}

static int STDCALL strtoperiod(const char * pStr, DateRange * pRange, long flags)
{
	int    ok = -1;
	const  int _defyear  = DefaultYear;
	const  int _defmonth = DefaultMonth;
	const  char * p = pStr;
	int    beg = 0;    // Признак того, что взята дата начала периода
	int    dbldot = 0;
	SUniDate_Internal _date;
	int    q = 0;
	SUniDate_Internal temp_dt;
	int    nd;
	long   number = 0;
    pRange->Z();
	while(ok < 0) {
		switch(get_strtoperiod_token(&p, &number, &temp_dt, flags)) {
			case TOK_DATE:
				_date = temp_dt;
				break;
			case TOK_NUMBER:
				if(_date.D == 0 && checkirange(number, 1L, 31L))
					_date.D = number;
				else if(_date.M == 0 && checkirange(number, 1L, 12L))
					_date.M = number;
				else if(_date.Y == 0) {
					_date.Y = NZOR(number, 2000);
					if(_date.Y > 31 && _date.M == 0 && _date.D) {
						_date.M = _date.D;
						_date.D = 0;
					}
					if(_date.Y < 100) {
						if(_date.Y >= 50)
							_date.Y += 1900;
						else
							_date.Y += 2000;
					}
					else if(checkirange(_date.Y, 200, 299))
						_date.Y = 2000 + (_date.Y - 200);
				}
				else
					ok = 0;
				break;
			case TOK_QUARTER:
				if(q == 0 || beg) {
					q = number + 1;
					if(beg == 0) {
						_date.D = 1;
						_date.M = (q - 1) * 3 + 1;
					}
					else {
						_date.D = daysPerMonth[q * 3 - 1];
						_date.M = q * 3;
					}
				}
				else
					ok = 0;
				break;
			case TOK_DELIM:
				if(_date.IsFullyNonZero() || _date.IsZero())
					ok = 0;
				break;
			case TOK_DBLDOT:
				if(dbldot)
					ok = 0;
				else {
					if(_date.IsZero())
						pRange->low = ZERODATE;
					else {
						SETIFZ(_date.D, 1);
						SETIFZ(_date.M, _date.Y ? 1 : _defmonth);
						SETIFZ(_date.Y, _defyear);
						encodedate(_date.D, _date.M, _date.Y, &pRange->low);
					}
					beg = 1;
					_date.Z();
					dbldot = 1;
				}
				break;
			case TOK_EOL:
				if(beg == 0) {
					if(q) {
						encodedate(_date.D, _date.M, _date.Y ? _date.Y : _defyear, &pRange->low);
						encodedate(daysPerMonth[q * 3 - 1], q * 3, _date.Y ? _date.Y : _defyear, &pRange->upp);
					}
					else if(_date.IsZero())
						pRange->Z();
					else if(_date.Y && !_date.M && !_date.D) {
						encodedate(1, 1, _date.Y, &pRange->low);
						encodedate(31, 12, _date.Y, &pRange->upp);
					}
					else if(_date.Y && _date.M && !_date.D) {
						encodedate(1, _date.M, _date.Y, &pRange->low);
						nd = dayspermonth(_date.M, _date.Y);
						encodedate(nd, _date.M, _date.Y, &pRange->upp);
					}
					else {
						encodedate(NZOR(_date.D, 1), NZOR(_date.M, (_date.Y ? 1 : _defmonth)), NZOR(_date.Y, _defyear), &pRange->low);
						pRange->upp = pRange->low;
					}
				}
				else if(beg == 1) {
					if(_date.IsZero())
						pRange->upp = ZERODATE;
					else {
						int _d, _m, _y;
						decodedate(&_d, &_m, &_y, &pRange->low);
						SETIFZ(_date.M, _date.Y ? 12 : NZOR(_m, _defmonth));
						_date.Y = NZOR(_date.Y, NZOR(_y, _defyear));
						SETIFZ(_date.D, dayspermonth(_date.M, _date.Y));
						encodedate(_date.D, _date.M, _date.Y, &pRange->upp);
					}
				}
				ok = 1;
				break;
			case TOK_UNKNOWN:
				ok = 0;
				break;
		}
	}
	assert(oneof2(ok, 0, 1));
	return ok;
}

int FASTCALL DateRange::FromStr(const char * pStr, uint flags/*strtoprdfXXX*/) // @v12.3.7
{
	return strtoperiod(pStr, this, flags);
}

int DateRange::GetMultiplicity() const
{
	int    result = PRD_UNDEF;
	int    d1 = 0;
	int    m1 = 0;
	int    y1 = 0;
	int    d2 = 0;
	int    m2 = 0;
	int    y2 = 0;
	const  int  cls1 = low.getclass();
	const  int  cls2 = upp.getclass();
	if(cls1 != LDATE::cInvalid && cls2 != LDATE::cInvalid) {
		const  bool non_zero1 = (cls1 != LDATE::cZero);
		const  bool non_zero2 = (cls2 != LDATE::cZero);
		if(non_zero1 & non_zero2) {
			if(cls1 == LDATE::cNormal && cls2 == LDATE::cNormal) {
				decodedate(&d1, &m1, &y1, &low);
				decodedate(&d2, &m2, &y2, &upp);
				if(d1 == 1 && (m2 && d2 == dayspermonth(m2, y2))) {
					if(m1 == 1 && m2 == 12) {
						result = PRD_ANNUAL;
					}
					else if((m1 == 1 && m2 == 6) || (m1 == 7 && m2 == 12)) {
						result = PRD_SEMIAN;
					}
					else if(((m1-1) % 3) == 0 && (m2 % 3) == 0) {
						result = PRD_QUART;
					}
					else {
						result = PRD_MONTH;
					}
					assert(result != PRD_UNDEF);
				}
				else {
					const int dow1 = low.GetDayOfWeek();
					const int dow2 = upp.GetDayOfWeek();
					if(dow1 == 1 && dow2 == 7)
						result = PRD_WEEK;
					else
						result = PRD_DAY;
				}
			}
		}
		else if(non_zero1) {
			if(cls1 == LDATE::cNormal)
				result = PRD_BEGIN;
		}
		else if(non_zero2) {
			if(cls2 == LDATE::cNormal)
				result = PRD_END;
		}
	}
	return result;
}

SString & DateRange::ToStr(uint fmt, SString & rBuf) const // @v12.3.7
{
	//SString & ToStr(uint fmt/*DATF_XXX, DATRANGF_EXTENDED*/, SString & rBuf) const; // @v12.3.7
	rBuf.Z();
	//periodfmt(*pData, b);
	if(fmt & DATRANGF_EXTENDED) {
		const uint local_fmt = NZOR((fmt & ~DATRANGF_EXTENDED), (DATF_DMY|DATF_CENTURY));
		static const char * quart[4] = { "I", "II", "III", "IV" };
		int    r = -1;
		int    d1 = 0;
		int    m1 = 0;
		int    y1 = 0;
		int    d2 = 0;
		int    m2 = 0;
		int    y2 = 0;
		//char period[64];
		//char * p = period;
		decodedate(&d1, &m1, &y1, &low);
		decodedate(&d2, &m2, &y2, &upp);
		if(d1 == 1 && (m2 && d2 == dayspermonth(m2, y2))) {
			if(m1 == 1 && m2 == 12) {
				if(y1 == y2) {
					//itoa(y1, p, 10);
					rBuf.Cat(y1);
				}
				else if(y1 == 0) {
					//*p++ = '.';
					//*p++ = '.';
					//itoa(y2, p, 10);
					rBuf.Dot().Dot().Cat(y2);
				}
				else if(y2 == 0) {
					//p += sstrlen(itoa(y1, p, 10));
					//*p++ = '.';
					//*p++ = '.';
					//*p++ = 0;
					rBuf.Cat(y1).Dot().Dot();
				}
				else {
					//p += sstrlen(itoa(y1, p, 10));
					//*p++ = '.';
					//*p++ = '.';
					//itoa(y2, p, 10);
					rBuf.Cat(y1).Dot().Dot().Cat(y2);
				}
				r = PRD_ANNUAL;
			}
			else if(((m1-1) % 3) == 0 && (m2 % 3) == 0) {
				if(y1 == y2 && abs(m1-m2) < 3) {
					//sprintf(period, "%s/%d", quart[(m1-1)/3], y1);
					rBuf.Cat(quart[(m1-1)/3]).Slash().Cat(y1);
				}
				else if(y1 == 0) {
					//sprintf(period, "..%s/%d", quart[m2/3-1], y2);
					rBuf.Dot().Dot().Cat(quart[m2/3-1]).Slash().Cat(y2);
				}
				else if(y2 == 0) {
					//sprintf(period, "%s/%d..", quart[(m1-1)/3], y1);
					rBuf.Cat(quart[(m1-1)/3]).Slash().Cat(y1).Dot().Dot();
				}
				else {
					//sprintf(period, "%s/%d..%s/%d", quart[(m1-1)/3], y1, quart[m2/3-1], y2);
					rBuf.Cat(quart[(m1-1)/3]).Slash().Cat(y1).Dot().Dot().Cat(quart[m2/3-1]).Slash().Cat(y2);
				}
				r = PRD_QUART;
			}
			else {
				if(y1 == y2 && m1 == m2) {
					//sprintf(period, "%d/%d", m1, y1);
					rBuf.Cat(m1).Slash().Cat(y1);
				}
				else if(y1 == 0) {
					//sprintf(period, "..%d/%d", m2, y2);
					rBuf.Dot().Dot().Cat(m2).Slash().Cat(y2);
				}
				else if(y2 == 0) {
					//sprintf(period, "%d/%d..", m1, y1);
					rBuf.Cat(m1).Slash().Cat(y1).Dot().Dot();
				}
				else {
					//sprintf(period, "%d/%d..%d/%d", m1, y1, m2, y2);
					rBuf.Cat(m1).Slash().Cat(y1).Dot().Dot().Cat(m2).Slash().Cat(y2);
				}
				r = PRD_MONTH;
			}
			//strnzcpy(pBuf, period, bufLen);
		}
		else {
			//periodfmt(rPeriod, period);
			if(low) {
				rBuf.Cat(low, local_fmt);
			}
			if(low != upp) {
				rBuf.Dot().Dot();
				if(upp)
					rBuf.Cat(upp, local_fmt);
			}
			r = PRD_DAY;
		}
		//return r;
	}
	else {
		const uint local_fmt = NZOR(fmt, (DATF_DMY|DATF_CENTURY));
		{
			if(low) {
				rBuf.Cat(low, local_fmt);
			}
			if(low != upp) {
				rBuf.Dot().Dot();
				if(upp)
					rBuf.Cat(upp, local_fmt);
			}
		}
	}
	return rBuf;
}
//
//
//
void TimeRange::Set(LTIME lowTm, LTIME uppTm)
{
	low = lowTm;
	upp = uppTm;
}

TimeRange & TimeRange::Z()
{
	low = ZEROTIME;
	upp = ZEROTIME;
	return *this;
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

SString & TimeRange::ToStr(uint tmFormat, SString & rBuf) const
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
