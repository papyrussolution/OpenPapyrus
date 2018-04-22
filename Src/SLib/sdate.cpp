// SDATE.CPP
// Copyright (C) Sobolev A. 1994, 1995, 1996-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#ifndef _WIN32_WCE
	#define USE_DF_CLARION
#endif

const LDATE ZERODATE = {0L};
const LDATE MAXDATE  = {MAXLONG};
const LDATE MAXDATEVALID = {0x0bb80101}; // 01/01/3000
const LTIME ZEROTIME = {0L};
const LTIME MAXTIME  = {MAXLONG};
const LTIME MAXDAYTIME = { 0x173B3B63U }; // 23:59:59.99
const LDATETIME ZERODATETIME = {{0}, {0}};
const LDATETIME MAXDATETIME = {{MAXLONG}, {MAXLONG}};

const char daysPerMonth[NUM_MONTHES] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const struct { char div, ord; } fmtParams[] = {
	{47,0}, // DATF_AMERICAN
	{46,2}, // DATF_ANSI
	{47,1}, // DATF_BRITISH
	{47,1}, // DATF_FRENCH
	{46,1}, // DATF_GERMAN
	{45,1}, // DATF_ITALIAN
	{47,2}, // DATF_JAPAN
	{45,0}, // DATF_USA
	{47,0}, // DATF_MDY
	{47,1}, // DATF_DMY
	{47,2}, // DATF_YMD
	{45,2}, // DATF_SQL то же, что ISO8601 но с префиксом DATE и в апострофах: DATE 'YYYY-MM-DD'
	{20,1}, // DATF_INTERNET (формальный подход не сработает - структура сложная)
	{45,2}  // DATF_ISO8601
};

int FASTCALL _decode_date_fmt(int style, int * pDiv)
{
	int    ord = 1;
	style &= 0x000f;
	if(style > 0 && style <= SIZEOFARRAY(fmtParams)) {
		*pDiv = fmtParams[style-1].div;
		ord = fmtParams[style-1].ord;
	}
	else {
		*pDiv = 47;
	}
	return ord;
}

int FASTCALL IsLeapYear(uint y)
{
	return ((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0));
}

// ReactOS: static __inline int IsLeapYear(int Year) { return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0; }

int FASTCALL DateToDaysSinceChristmas(int y, int m, int d)
{
	int    n = 0;
	//
	// Add days for months in given date
	//
	switch(m) {
		case  1: 
			if((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0))
				n--;
			break;
		case  2: 
			if((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0))
				n += 30;
			else
				n += 31;
			break;
		case  3: n += 31+28; break;
		case  4: n += 31+28+31; break;
		case  5: n += 31+28+31+30; break;
		case  6: n += 31+28+31+30+31; break;
		case  7: n += 31+28+31+30+31+30; break;
		case  8: n += 31+28+31+30+31+30+31; break;
		case  9: n += 31+28+31+30+31+30+31+31; break;
		case 10: n += 31+28+31+30+31+30+31+31+30; break;
		case 11: n += 31+28+31+30+31+30+31+31+30+31; break;
		case 12: n += 31+28+31+30+31+30+31+31+30+31+30; break;
	}
	// Since every leap year is of 366 days,
	// Add a day for every leap year
	return d + n + ((y-1) * 365) + ((y / 4) - (y / 100) + (y / 400));
}
//
// convert day number to y,m,d format 
//
static int FASTCALL getMon(int * pD, int leap)
{
	int    i, nd;
	if(*pD == 0) {
		*pD = 31;
		i  = 12;
	}
	else {
		for(i = nd = 0; *pD > nd;) {
			*pD -= nd;
			nd = (leap && i == 1) ? 29 : daysPerMonth[i];
			i++;
		}
	}
	return i;
}

int FASTCALL DaysSinceChristmasToDate(int g, int * pYear, int * pMon, int * pDay) 
{
	int    ok = 1;
	int    day = 0;
	int    _year = 0;
	int    mon = 0;
	if(g >= 0) {
		const int    h4y = g / (400 * 365 + 97);
		const int    h4d_rem = g % (400 * 365 + 97);
		const int    hy = h4d_rem / (100 * 365 + 24);
		const int    hy_rem = h4d_rem % (100 * 365 + 24);
		const int    fy = hy_rem / (4 * 365 + 1);
		const int    fy_rem = hy_rem % (4 * 365 + 1);
		const int    y = fy_rem / 365;
		day = fy_rem % 365;
		_year = (h4y * 400) + (hy * 100) + (fy * 4) + y + 1;
		mon = 0;
		if(day == 0) {
			_year--;
			mon = 12;
			day = (y == 4 || hy == 4) ? 30 : 31; // Граничная проблема: конец четверки годов или 400-летия
		}
		else if(IsLeapYear(_year)) {
			for(int nd = 0; day > nd;) {
				day -= nd;
				nd = (mon == 1) ? 29 : daysPerMonth[mon];
				mon++;
			}
		}
		else {
			for(int nd = 0; day > nd;) {
				day -= nd;
				nd = daysPerMonth[mon];
				mon++;
			}
		}
	}
	else
		ok = 0;
	ASSIGN_PTR(pYear, _year);
	ASSIGN_PTR(pMon, mon);
	ASSIGN_PTR(pDay, day);
	return ok;
}

int FASTCALL dayspermonth(int month, int year)
{
	int    dpm = 0;
	if(month > 0 && month <= 12) {
		dpm = daysPerMonth[month-1];
		if(month == 2 && IsLeapYear(year))
			dpm++;
	}
	return dpm;
}

const char * monthNames[NUM_MONTHES] = {
	"Январ[ь|я]", "Феврал[ь|я]", "Март[|а]", "Апрел[ь|я]", "Ма[й|я]", "Июн[ь|я]", 
	"Июл[ь|я]", "Август[|а]", "Сентябр[ь|я]", "Октябр[ь|я]", "Ноябр[ь|я]", "Декабр[ь|я]"
};

static char * FASTCALL extractVarPart(const char * word, char * buf)
{
	const char * p = word;
	char * b = buf;
	while(*p && *p != ']')
		*b++ = *p++;
	*b = 0;
	if(*p)
		p++;
	return (char *)p;
}

static char * SLAPI extractFormFromVarPart(const char * vp, int n /*[1..]*/, char * buf)
{
	const char * c = vp;
	const char * p = vp;
	char  * b = buf;
	int i = 0;
	while(i < n && ((c = p), p = strchr(p, '|')) != 0) {
		i++;
		p++;
	}
	while(*c && !(p && *c == '|'))
		*b++ = *c++;
	*b = 0;
	return b;
}

static char * SLAPI selectVarPart(const char * word, int n, char * pBuf)
{
	char   vp[256];
	char * e = extractVarPart(word, vp);
	extractFormFromVarPart(vp, n, pBuf);
	return e;
}

static char * getWordForm(const char * pattern, long fmt, char * pBuf)
{
	char   temp[32];
	char * t = temp;
	if(fmt & MONF_SHORT) {
		memcpy(temp, pattern, 3);
		temp[3] = 0;
	}
	else {
		const char * p = pattern;
		while(*p && *p != '[')
			*t++ = *p++;
		if(*p == '[') {
			const int _case = (fmt & MONF_CASEGEN) ? 2 : 1;
			p = selectVarPart(++p, _case, t);
			t += sstrlen(t);
		}
		while(*p)
			*t++ = *p++;
	}
	return strcpy(pBuf, temp);
}

static char * FASTCALL getMonthText(int mon, long fmt, char * pBuf)
{
	if(pBuf)
		if(mon >= 1 && mon <= 12) {
			getWordForm(monthNames[mon-1], fmt, pBuf);
			if(mon == 5 && fmt & MONF_SHORT) {
				pBuf[2] = 'й';
				pBuf[3] = 0;
			}
			if(fmt & MONF_OEM)
				SCharToOem(pBuf);
		}
		else
			pBuf[0] = 0;
	return pBuf;
}
//
//
//
static const char * FASTCALL _extractVarPart(const char * word, char * buf)
{
	const char * p = word;
	char * b = buf;
	while(*p && *p != ']')
		*b++ = *p++;
	*b = 0;
	if(*p)
		p++;
	return p;
}

static void SLAPI _extractFormFromVarPart(const char * vp, int n /*[1..]*/, SString & rBuf)
{
	const char * c = vp;
	const char * p = vp;
	//char  * b = buf;
	for(int i = 0; i < n && ((c = p), p = strchr(p, '|')) != 0;) {
		i++;
		p++;
	}
	while(*c && !(p && *c == '|')) {
		rBuf.CatChar(*c++);
	}
	//*b = 0;
	//return b;
}

static const char * SLAPI _selectVarPart(const char * word, int n, SString & rBuf)
{
	char   vp[256];
	const  char * e = _extractVarPart(word, vp);
	_extractFormFromVarPart(vp, n, rBuf);
	return e;
}

static SString & _getWordForm(const char * pPattern, long fmt, SString & rBuf)
{
	rBuf.Z();
	//char   temp[32];
	//char * t = temp;
	if(fmt & MONF_SHORT) {
		rBuf.CatN(pPattern, 3);
	}
	else {
		const char * p = pPattern;
		while(*p && *p != '[') {
			rBuf.CatChar(*p++);
		}
		if(*p == '[') {
			const int _case = (fmt & MONF_CASEGEN) ? 2 : 1;
			p = _selectVarPart(++p, _case, rBuf);
		}
		while(*p) {
			rBuf.CatChar(*p++);
		}
	}
	return rBuf;
}

SString & FASTCALL SGetMonthText(int mon, long fmt, SString & rBuf)
{
	rBuf.Z();
	if(mon >= 1 && mon <= 12) {
		_getWordForm(monthNames[mon-1], fmt, rBuf);
		if(mon == 5 && fmt & MONF_SHORT) {
			rBuf.Trim(2).CatChar('й');
		}
		if(fmt & MONF_OEM) {
			rBuf.ToOem();
		}
	}
	return rBuf;
}

static const char * P_WeekDays = {
	"Monday,Mo,Понедельник,Пнд;Tuesday,Tu,Вторник,Вт;Wednesday,We,Среда,Ср;Thursday,Th,Четверг,Чтв;"
	"Friday,Fr,Пятница,Птн;Saturday,Sa,Суббота,Сбт;Sunday,Su,Воскресенье,Вскр"
};

int FASTCALL GetDayOfWeekByText(const char * pText)
{
	SString temp_buf;
	StringSet ss(';', P_WeekDays);
	int    dow = 0;
	for(uint i = 0; ss.get(&i, temp_buf);) {
		++dow;
		StringSet ss2(',', temp_buf);
		for(uint j = 0; ss2.get(&j, temp_buf);)
			if(temp_buf.CmpNC(pText) == 0)
				return dow;
	}
	return 0;
}

int FASTCALL GetDayOfWeekText(int options, int dayOfWeek /* 1..7 */, SString & rBuf)
{
	rBuf.Z();
	if(dayOfWeek >= 1 && dayOfWeek <= 7 && options >= 1 && options <= 4) {
		rBuf.GetSubFrom(P_WeekDays, ';', dayOfWeek-1);
		rBuf.GetSubFrom(rBuf, ',', options-1);
		return 1;
	}
	else
		return (SLibError = SLERR_INVRANGE, 0);
}

// @v7.9.5 int16-->int (faster)
static const int16 __dpm[11] = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334/*, 365*/ };

static inline int FASTCALL getDays360(int mon, int leap) { return (mon > 1 && mon <= 12) ? ((mon-1) * 30) : 0; }
static inline int FASTCALL getDays(int mon, int leap) { return (mon > 1 && mon <= 12) ? ((leap && mon > 2) ? __dpm[mon-2]+1 : __dpm[mon-2]) : 0; }

static void _ltodate(long nd, void * dt, int format)
{
	int    y, m, d;
	DaysSinceChristmasToDate(nd, &y, &m, &d);
	/*
	ldiv_t xd = ldiv(nd, 1461L);
	y = (int)(4 * xd.quot);
	xd = ldiv(xd.rem, 365L);
	y += (int)xd.quot;
	if(xd.rem) {
		y++;
		d = (int)xd.rem;
	}
	else
		d = xd.quot ? 365 : 366;
	m = getMon(&d, !(y % 4));
	*/
	_encodedate(d, m, y, dt, format);
}

static void _ltodate360(long nd, void * dt, int format)
{
	ldiv_t xd = ldiv(nd, 360L);
	int    y = (int)xd.quot + 1;
	int    d = (int)xd.rem;
	int    m = getMon(&d, !(y % 4));
	_encodedate(d, m, y, dt, format);
}

static long FASTCALL _datetol(const void * dt, int format)
{
	int    d, m, y;
	_decodedate(&d, &m, &y, dt, format);
	if(y) {
		//int leap = !(y-- & 3);
		//return (1461L * (y>>2) + 365L * (y&3) + getDays(m, leap) + d);
		return DateToDaysSinceChristmas(y, m, d);
	}
	else
		return 0L;
}

static long FASTCALL _datetol360(const void * dt, int format)
{
	int    d, m, y;
	_decodedate(&d, &m, &y, dt, format);
	return (360L * y + getDays360(m, 0) + d);
}
//
// Преобразование даты в формате Clarion
//
#ifdef USE_DF_CLARION

//const struct date firstPoint = { 1801, 1, 1 };
#define firstYear 1801
#define firstMon     1
#define firstDay     1
#define firstOff    4L

static long SLAPI CLADateToLong(const SDosDate * d)
{
	SDosDate doff;
	long loff;
	int  leap;
	doff.da_year = d->da_year - firstYear;
	doff.da_day  = d->da_day  - firstDay;
	leap = (d->da_year % 4 == 0);
	loff = firstOff + doff.da_year * 365L + (doff.da_year >> 2) + doff.da_day + getDays(d->da_mon, leap) - getDays(firstMon, 0) - 1;
	return loff;
}

static void SLAPI CLALongToDate(long off, SDosDate * d)
{
	long dev  = off - ((off / 365) >> 2) - 2;
	int  rest = (int)(dev % 365);
	d->da_year = (int)(dev / 365) + firstYear;
	if(rest == 0)
		d->da_year--;
	d->da_mon = getMon(&rest, d->da_year % 4 == 0);
	d->da_day = rest;
}

#endif /* USE_DF_CLARION */

static void formatNotSupported(const char * pFormatName)
{
	fprintf(stderr, "%s date format not supported", pFormatName);
	exit(-1);
}

void FASTCALL _encodedate(int day, int mon, int year, void * pBuf, int format)
{
	char   tmp[64];
	switch(format) {
		case DF_BTRIEVE:
			{
				/*LDATE temp_dt;
				temp_dt.encode(day, mon, year);
				*(LDATE *)pBuf = temp_dt;*/
				((LDATE *)pBuf)->encode(day, mon, year);
			}
			break;
#ifndef _WIN32_WCE
		case DF_DOS:
			((SDosDate *)pBuf)->da_day  = day;
			((SDosDate *)pBuf)->da_mon  = mon;
			((SDosDate *)pBuf)->da_year = year;
			break;
#endif
		case DF_FAT:
			*(uint *)pBuf = (((year - 1980) << 9) | (mon << 5) | day);
			break;
		case DF_XBASE:
			sprintf(tmp, "%04d%02d%02d", year, mon, day);
			memcpy(pBuf, tmp, 8);
			break;
		case DF_PARADOX:
			formatNotSupported("Paradox");
			break;
		case DF_CLARION:
			{
#ifdef USE_DF_CLARION
				SDosDate dt;
				dt.da_day  = day;
				dt.da_mon  = mon;
				dt.da_year = year;
				*(long *)pBuf = CLADateToLong(&dt);
#else
				formatNotSupported("Clarion");
#endif
			}
			break;
		default:
			//printf("Not supplied date format\n");
			//abort();
			break;
	}
}

void FASTCALL _decodedate(int * day, int * mon, int * year, const void * pBuf, int format)
{
	char   tmp[64];
#ifndef _WIN32_WCE
	SDosDate d;
#endif
	switch(format)  {
		case DF_BTRIEVE:
			/*
			*year = (int)(*(long *)pBuf >> 16);
			*mon  = (int)((*(long *)pBuf & 0x0000ff00) >> 8);
			*day  = (int)(*(long *)pBuf & 0x000000ff);
			*/
			((const LDATE *)pBuf)->decode(day, mon, year);
			break;
#ifndef _WIN32_WCE
		case DF_DOS:
			*day  = ((SDosDate *)pBuf)->da_day;
			*mon  = ((SDosDate *)pBuf)->da_mon;
			*year = ((SDosDate *)pBuf)->da_year;
			break;
#endif
		case DF_FAT:
			*year = (*(uint *)pBuf >> 9) + 1980;
			*mon  = (*(uint *)pBuf >> 5) & 0x000f;
			*day  = (*(uint *)pBuf) & 0x001f;
			break;
		case DF_XBASE:
			((char *)memcpy(tmp, pBuf, 8))[8] = '\0';
			sscanf(tmp, "%4d%2d%2d", year, mon, day);
			break;
		case DF_PARADOX:
			formatNotSupported("Paradox");
			break;
		case DF_CLARION:
#ifdef USE_DF_CLARION
			CLALongToDate(*(long *)pBuf, &d);
			*day  = d.da_day;
			*mon  = d.da_mon;
			*year = d.da_year;
#else
			formatNotSupported("Clarion");
#endif
			break;
		default:
			//printf("Not supplied date format\n");
			//abort();
			break;
	}
}

long FASTCALL _diffdate(const void * dest, const void * src, int format, int _360)
{
	if(!_360)
		return (_datetol(dest, format) - _datetol(src, format));
	else
		return (_datetol360(dest, format) - _datetol360(src, format));
}

void FASTCALL _plusdate(void * dt, int nd, int fmt, int _360)
{
	if(!_360)
		_ltodate(_datetol(dt, fmt) + nd, dt, fmt);
	else
		_ltodate360(_datetol360(dt, fmt) + nd, dt, fmt);
}

void SLAPI _plusperiod(void * dest, int prd, int numperiods, int format, int _360)
{
	int d, m, y;
	if(numperiods) {
		if((prd & PRD_PRECDAYSMASK) == PRD_PRECDAYSMASK) {
			int   t = (int16)(prd & ~PRD_PRECDAYSMASK);
			if(t <= 0)
				t = 1;
			numperiods *= t;
			prd = PRD_DAY;
		}
		else if(prd == PRD_QUART) {
			prd = PRD_MONTH;
			numperiods *= 3;
		}
		else if(prd == PRD_SEMIAN) {
			prd = PRD_MONTH;
			numperiods *= 6;
		}
		else if(prd == PRD_WEEK || (prd > 0 && prd != PRD_MONTH && prd != PRD_ANNUAL)) {
			numperiods *= prd;
			prd = PRD_DAY;
		}
		if(prd == PRD_DAY)
			_plusdate(dest, numperiods, format, _360);
		else {
			_decodedate(&d, &m, &y, dest, format);
			if(prd == PRD_MONTH) {
				y += (numperiods+=m) / 12;
				if((m = numperiods % 12) <= 0) {
					m += 12;
					y--;
				}
			}
			else if(prd == PRD_ANNUAL)
				y += numperiods;
			if(d > daysPerMonth[m-1])
				d = ((y % 4) == 0 && m == 2 && d >= 29) ? 29 : daysPerMonth[m-1];
			_encodedate(d, m, y, dest, format);
		}
	}
}

int FASTCALL _dayofweek(const void * pDate, int format)
{
	char   beg[32];
	_encodedate(1, 1, 1970, beg, format); /* 1/1/1970 - —ҐвўҐаЈ (4) */
	return (int)((4 + _diffdate(pDate, beg, format, 0) % 7) % 7);
}

void FASTCALL encodedate(int day, int mon, int year, void * pBinDate)
	{ _encodedate(day, mon, year, pBinDate, BinDateFmt); }
void FASTCALL decodedate(int * pDay, int * pMon, int * pYear, const void * pBinDate)
	{ _decodedate(pDay, pMon, pYear, pBinDate, BinDateFmt); }
long SLAPI diffdate(const void * pDest, const void * pSrc, int _360)
	{ return _diffdate(pDest, pSrc, BinDateFmt, _360); }
void FASTCALL plusdate(void * pDest, int numdays, int _360)
	{ _plusdate(pDest, numdays, BinDateFmt, _360); }
void SLAPI plusperiod(void * pDest, int period, int numperiods, int _360)
	{ _plusperiod(pDest, period, numperiods, BinDateFmt, _360); }

int FASTCALL dayofweek(const void * pDate, int sundayIsSeventh)
{
	int    dow = _dayofweek(pDate, BinDateFmt);
	if(sundayIsSeventh)
		dow = dow ? dow : 7;
	return dow;
}

LDATE FASTCALL encodedate(int day, int month, int year)
{
	LDATE dt;
	encodedate(day, month, year, &dt);
	return dt;
}

LDATE FASTCALL plusdate(LDATE d, long a)
{
	if(a) // @v10.0.02
		plusdate(&d, (int)a, 0);
	return d;
}

long FASTCALL diffdate(LDATE d, LDATE s)
{
	return (d != s) ? diffdate(&d, &s, 0) : 0; // @v10.0.02 (x)-->(d != s) ? (x) : 0
}
//
//
//
int FASTCALL _checkdate(int day, int mon, int year)
{
	int    err = SLERR_SUCCESS;
	if(!(year & REL_DATE_MASK) && (year != ANY_DATE_VALUE) && (year < 1801 || year > 2099))
		err = SLERR_INVYEAR;
	else if(!(mon & REL_DATE_MASK) && (mon != ANY_DATE_VALUE) && (mon < 1 || mon > 12))
		err = SLERR_INVMONTH;
	else if(!(day & (REL_DATE_MASK|THRSMDAY_DATE_MASK)) && (day != ANY_DATE_VALUE)) {
		if(day < 1)
			err = SLERR_INVDAY;
		else if(mon & REL_DATE_MASK) {
			if(day > 31)
				err = SLERR_INVDAY;
		}
		else if(year & REL_DATE_MASK) {
			if(day > dayspermonth(mon, 2000)) // any leap year
				err = SLERR_INVDAY;
		}
		else if(oneof2(mon, ANY_MONITEM_VALUE, ANY_DATE_VALUE)) {
			if(day > 31)
				err = SLERR_INVDAY;
		}
		else if(day > dayspermonth(mon, year))
			err = SLERR_INVDAY;
	}
	return err ? (SLibError = err, 0) : 1;
}

int FASTCALL checkdate(const void * pBinDate)
{
	int    d, m, y;
	_decodedate(&d, &m, &y, pBinDate, BinDateFmt);
	return _checkdate(d, m, y);
}

int FASTCALL checkdate(LDATE dt, int zeroIsOk)
{
	if(dt) {
		int    d, m, y;
		_decodedate(&d, &m, &y, &dt, DF_BTRIEVE);
		return _checkdate(d, m, y);
	}
	else
		return zeroIsOk ? 1 : 0;
	//return ((!dt && zeroIsOk) || _checkdate(dt.day(), dt.month(), dt.year()));
}
//
//
//
IMPL_INVARIANT_C(LTIME)
{
	S_INVARIANT_PROLOG(pInvP);
	if(v != 0) {
		S_ASSERT_P(hour() > 0, pInvP);
		S_ASSERT_P(minut() >= 0 && minut() < 60, pInvP);
		S_ASSERT_P(sec() >= 0 && sec() < 60, pInvP);
	}
	S_INVARIANT_EPILOG(pInvP);
}

int FASTCALL checktime(LTIME tm)
{
	int    err = SLERR_SUCCESS;
	int    h, m, s, ts;
	decodetime(&h, &m, &s, &ts, &tm);
	if(h < 0 || h > 23)
		err = SLERR_INVHOUR;
	else if(m < 0 || m > 59)
		err = SLERR_INVMIN;
	else if(s < 0 || s > 59)
		err = SLERR_INVSEC;
	else if(ts < 0 || ts > 99)
		err = SLERR_INVTSEC;
	if(err) {
#ifndef _WIN32_WCE // {
		char   temp_buf[64];
		timefmt(tm, TIMF_HMS | TIMF_MSEC, temp_buf);
		SLS.SetError(err, temp_buf);
#endif // } _WIN32_WCE
		return 0;
	}
	else
		return 1;
}

#ifndef _WIN32_WCE // {

LTIME::operator OleDate() const
{
	LDATETIME dtm;
	dtm.Set(ZERODATE, *this);
	return dtm;
}

LTIME LTIME::operator = (OleDate od)
{
	LDATETIME dtm;
	dtm = od;
	return (*this = dtm.t);
}

LTIME LTIME::operator = (double od)
{
	LDATETIME dtm;
	OleDate _od;
	_od.v = od;
	dtm = _od;
	return (*this = dtm.t);
}

LTIME SLAPI LTIME::encode(int h, int m, int s, int ms)
{
	*this = encodetime(h, m, s, ms / 10);
	return *this;
}

#endif // }

long FASTCALL LTIME::settotalsec(long s)
{
	long   inc_dt = s / (3600 * 60 * 60);
	encodetime(s / 3600, (s % 3600) / 60, s % 60, 0, this);
	return inc_dt;
}

LTIME & FASTCALL LTIME::addhs(long n)
{
	int    h, m, s, hs;
	decodetime(&h, &m, &s, &hs, this);
	hs += n;
	if(hs >= 100) {
		s += (hs / 100);
		hs = hs % 100;
		if(s >= 60) {
			m += s / 60;
			s = s % 60;
		}
		if(m > 60) {
			h += m / 60;
			m = m % 60;
		}
	}
	else if(hs < 0) {
		s += (hs / 100) - 1;
		hs = (-hs) % 100;
		if(s < 0) {
			m += (s / 60) - 1;
			s = (-s) % 60;
		}
		if(m < 0) {
			h += (m / 60) - 1;
			m = (-m) % 60;
		}
	}
	encodetime(h, m, s, hs, this);
	return *this;
}

#define __ENCODEDATE(h, m, s, ts, pTm) PTR8(pTm)[0] = ts; PTR8(pTm)[1] = s; PTR8(pTm)[2] = m; PTR8(pTm)[3] = h;

void FASTCALL encodetime(int h, int m, int s, int ts, void * tm)
{
	__ENCODEDATE(h, m, s, ts, tm);
}

LTIME FASTCALL encodetime(int h, int m, int s, int ts)
{
	LTIME  tm;
	__ENCODEDATE(h, m, s, ts, &tm.v);
	return tm;
}

void FASTCALL decodetime(int * h, int * m, int * s, int * ts, void * tm)
{
	ASSIGN_PTR(h,  ((char *)tm)[3]);
	ASSIGN_PTR(m,  ((char *)tm)[2]);
	ASSIGN_PTR(s,  ((char *)tm)[1]);
	ASSIGN_PTR(ts, ((char *)tm)[0]);
}

long FASTCALL DiffTime(LTIME t1, LTIME t2, int dim)
{
	int    h1, m1, s1, ts1;
	int    h2, m2, s2, ts2;
	decodetime(&h1, &m1, &s1, &ts1, &t1);
	decodetime(&h2, &m2, &s2, &ts2, &t2);
	long   d = ((ts1-ts2) * 10) + 1000L * ((s1-s2) + 60L * ((m1-m2) + 60L * (h1-h2)));
	if(dim == 1) // Hours
		return (d / (3600L * 1000L));
	else if(dim == 2) // Minuts
		return (d / (60L * 1000L));
	else if(dim == 3) // Seconds
		return (d / 1000L);
	else /*if(dim == 4)*/
		return d;
}

long SLAPI diffdatetime(LDATE d1, LTIME t1, LDATE d2, LTIME t2, int dim, long * pDiffDays)
{
	long   dd = _diffdate(&d1, &d2, DF_BTRIEVE, 0);
	long   dt = DiffTime(t1, t2, 4/*dim*/);
	if(dd != 0) {
		if(dd > 0 && dt < 0) {
			dd--;
			dt += (24 * 3600L * 1000L);
		}
		else if(dd < 0 && dt > 0) {
			dd++;
			dt -= (24 * 3600L * 1000L);
		}
	}
	ASSIGN_PTR(pDiffDays, dd);
	if(dim == 1)
		return (dt / (3600L * 1000L));
	else if(dim == 2)
		return (dt / (60L * 1000L));
	else if(dim == 3)
		return (dt / 1000L);
	else /*if(dim == 4)*/
		return dt;
}

long  SLAPI diffdatetime(const LDATETIME & dtm1, const LDATETIME & dtm2, int dim, long * pDiffDays)
{
	return diffdatetime(dtm1.d, dtm1.t, dtm2.d, dtm2.t, dim, pDiffDays);
}

long SLAPI diffdatetimesec(LDATE d1, LTIME t1, LDATE d2, LTIME t2)
{
	long dif_days = 0;
	long ds = diffdatetime(d1, t1, d2, t2, 3, &dif_days);
	ds += dif_days * 3600 * 24;
	return ds;
}

long FASTCALL diffdatetimesec(const LDATETIME & dtm1, const LDATETIME & dtm2)
{
	long   dif_days = 0;
	long   ds = diffdatetime(dtm1, dtm2, 3, &dif_days);
	ds += dif_days * 3600 * 24;
	return ds;
}

LDATETIME FASTCALL plusdatetime(const LDATETIME & dtm1, long plus, int dim)
{
	int    days = 0;
	const  LTIME  tm = dtm1.t;
	LDATETIME dest;
	int    h = tm.hour();
	int    m = tm.minut();
	int    s = tm.sec();
	int    hs = tm.hs();
	if(dim == 1)
		h += plus;
	else if(dim == 2)
		m += plus;
	else if(dim == 3)
		s += plus;
	else if(dim == 4)
		hs += plus;
	//
	// Нормализуем величины в случае, если они выходят за допустимые пределы
	//
	s    += (hs / 100); if(hs < 0) { s--;    hs = 100 + hs%100; } else hs %= 100;
	m    += (s  / 60);  if(s  < 0) { m--;    s  = 60  + s%60;   } else s  %= 60;
	h    += (m  / 60);  if(m  < 0) { h--;    m  = 60  + m%60;   } else m  %= 60;
	days += (h  / 24);  if(h  < 0) { days--; h  = 24  + h%24;   } else h  %= 24;
	//
	dest.d = plusdate(dtm1.d, days);
	dest.t = encodetime(h, m, s, hs);
	return dest;
}
//
//
//
int SLAPI setcurdatetime(LDATETIME dtm)
{
	SYSTEMTIME st;
	if(SetLocalTime(&dtm.Get(st))) {
		::SendMessage(HWND_TOPMOST, WM_TIMECHANGE, 0, 0);
		return 1;
	}
	else
		return SLS.SetOsError();
}

int FASTCALL getcurdatetime(LDATE * pDt, LTIME * pTm)
{
#if defined(__WIN32__) || defined(_WIN32_WCE)
	SYSTEMTIME st;
	::GetLocalTime(&st);
	if(pDt) {
		// @v10.0.02 _encodedate((char)st.wDay, (char)st.wMonth, (int)st.wYear, pDt, DF_BTRIEVE);
		pDt->encode(st.wDay, st.wMonth, st.wYear); // @v10.0.02
	}
	if(pTm) {
		char * _tm = (char *)pTm;
		_tm[0] = (char)(st.wMilliseconds/10);
		_tm[1] = (char)st.wSecond;
		_tm[2] = (char)st.wMinute;
		_tm[3] = (char)st.wHour;
	}
#else
	struct date d;
	getdate(&d);
	_encodedate((char)d.da_day, (char)d.da_mon, (int)d.da_year, pDt, DF_BTRIEVE);
	if(pTm) {
		char * _tm = (char *)pTm;
		struct time t;
		gettime(&t);
		_tm[0] = t.ti_hund;
		_tm[1] = t.ti_sec;
		_tm[2] = t.ti_min;
		_tm[3] = t.ti_hour;
	}
#endif
	return 1;
}

int FASTCALL getcurdate(LDATE * dt) { return getcurdatetime(dt, 0); }
int FASTCALL getcurtime(LTIME * tm) { return getcurdatetime(0, tm); }

LDATE FASTCALL getcurdate_()
{
	LDATE dt;
	getcurdatetime(&dt, 0);
	return dt;
}

LTIME FASTCALL getcurtime_()
{
	LTIME tm;
	getcurdatetime(0, &tm);
	return tm;
}

int FASTCALL getcurdatetime(LDATETIME * pTm)
{
	return getcurdatetime(&pTm->d, &pTm->t);
}

LDATETIME FASTCALL getcurdatetime_()
{
	LDATETIME dtm;
	getcurdatetime(&dtm.d, &dtm.t);
	return dtm;
}

int gettimezone()
{
	TIME_ZONE_INFORMATION tz;
	::GetTimeZoneInformation(&tz);
	return (tz.Bias+tz.DaylightBias);
}
//
//
//
LDATE WorkDate::InitDate = {0x07CB0C1FL}; // 31/12/1995

int WorkDate::GetVal() const { return V; }

// static
int FASTCALL WorkDate::ShrinkDate(LDATE dt)
{
	int16  diff;
	if(dt == ZERODATE)
		diff = 0;
	else if(dt == MAXDATE)
		diff = MAXSHORT;
	else
		diff = (int16)diffdate(dt, WorkDate::InitDate);
	return diff;
}

// static
LDATE FASTCALL WorkDate::ExpandDate(int16 sdt)
{
	LDATE  dt;
	if(sdt <= 0)
		dt = ZERODATE;
	else if(sdt == MAXSHORT)
		dt = MAXDATE;
	else
		dt = plusdate(WorkDate::InitDate, sdt);
	return dt;
}

int FASTCALL WorkDate::SetDate(LDATE dt)
{
	if(!dt || checkdate(&dt)) {
		int    d = ShrinkDate(dt);
		if(d >= 0) {
			V = d;
			return 1;
		}
	}
	return 0;
}

int FASTCALL WorkDate::SetDayOfWeek(int dayOfWeek)
{
	if(dayOfWeek >= 1 && dayOfWeek <= 7) {
		V = -dayOfWeek;
		return 1;
	}
	else
		return 0;
}

int WorkDate::SetDayOfYear(int day, int mon)
{
	LDATE test = encodedate(day, mon, 1996);
	if(checkdate(&test)) {
		V = -ShrinkDate(test)-100;
		return 1;
	}
	else
		return 0;
}

LDATE  WorkDate::IsDate() const { return (V > 0) ? ExpandDate(V) : ZERODATE; }
int    WorkDate::IsDayOfWeek() const { return (V >= -7 && V <= -1) ? -V : 0; }
LDATE  WorkDate::IsDayOfYear() const { return (V <= -100) ? ExpandDate(-V) : ZERODATE; }
int    FASTCALL WorkDate::IsEqual(WorkDate wd) const { return (wd.V == V); }

int FASTCALL WorkDate::IsEqual(LDATE dt) const
{
	int    dow = IsDayOfWeek();
	if(dow)
		return (dow == dayofweek(&dt, 1));
	else {
		LDATE d = IsDate();
		if(d)
			return (d == dt);
		else {
			d = IsDayOfYear();
			if(d)
				return (d.day() == dt.day() && d.month() == dt.month());
		}
	}
	return 0;
}
//
// LDATETIME
//
static LDATETIME FarMoment;
//
// Descr: Специализированный класс для автоматической инициализации FarMoment
//
static const struct InitFarMoment { InitFarMoment() { FarMoment.d.v = MAXLONG; FarMoment.t.v = 0; } } IFM;

int  SLAPI LDATETIME::IsFar() const
{
	return cmp(*this, FarMoment) ? 0 : 1;
}

LDATETIME & SLAPI LDATETIME::SetFar()
{
	*this = FarMoment;
	return *this;
}

LDATETIME & FASTCALL LDATETIME::SetTimeT(time_t _tm)
{
	const struct tm * p_temp_tm = gmtime(&_tm);
	if(p_temp_tm) {
		d.encode(p_temp_tm->tm_mday, p_temp_tm->tm_mon+1, p_temp_tm->tm_year + 1900);
		t = encodetime(p_temp_tm->tm_hour, p_temp_tm->tm_min, p_temp_tm->tm_sec, 0);
	}
	else
		SetZero();
	return *this;
}

time_t SLAPI LDATETIME::GetTimeT() const
{
	if(!*this)
		return 0;
	else {
		struct tm _t;
		_t.tm_year = d.year()-1900;
		_t.tm_mon = d.month()-1; // @v10.0.02 @fix (-1)
		_t.tm_mday = d.day();
		_t.tm_hour = t.hour();
		_t.tm_min = t.minut();
		_t.tm_sec = t.sec();
		return (sizeof(time_t) == 8) ? _mktime64(&_t) : mktime(&_t);
	}
}

LDATETIME & SLAPI LDATETIME::Set(LDATE _d, LTIME _t)
{
	d = _d;
	t = _t;
	return *this;
}

int SLAPI LDATETIME::Set(const char * pText, long datf, long timf) { return strtodatetime(pText, this, datf, timf); }
int LDATETIME::operator !() const { return (d == ZERODATE && t == ZEROTIME); }
int FASTCALL LDATETIME::operator == (const LDATETIME & s) const { return (d == s.d && t == s.t); }
int FASTCALL LDATETIME::operator != (const LDATETIME & s) const { return (d != s.d || t != s.t); }

#ifndef _WIN32_WCE // {

SYSTEMTIME & FASTCALL LDATETIME::Get(SYSTEMTIME & rSt) const
{
	rSt.wYear = d.year();
	rSt.wMonth = d.month();
	rSt.wDayOfWeek = 0;
	rSt.wDay = d.day();
	rSt.wHour = t.hour();
	rSt.wMinute = t.minut();
	rSt.wSecond = t.sec();
	rSt.wMilliseconds = (WORD)((t.v & 0x000000ffL) * 10);
	return rSt;
}

LDATETIME FASTCALL LDATETIME::operator = (const SYSTEMTIME & rSt)
{
	d = encodedate(rSt.wDay, rSt.wMonth, rSt.wYear);
	encodetime(rSt.wHour, rSt.wMinute, rSt.wSecond, rSt.wMilliseconds / 10, &t);
	return *this;
}

LDATETIME FASTCALL LDATETIME::operator = (const FILETIME & rS)
{
	SYSTEMTIME st;
	if(::FileTimeToSystemTime(&rS, &st)) {
		d = encodedate(st.wDay, st.wMonth, st.wYear);
		encodetime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds / 10, &t);
	}
	else
		SetZero();
	return *this;
};

LDATETIME::operator OleDate() const
{
	OleDate od;
	if(d.v == 0 && t.v == 0)
		od.v = 0;
	else {
		SYSTEMTIME st;
		SystemTimeToVariantTime(&Get(st), &od.v);
	}
	return od;
}

LDATETIME FASTCALL LDATETIME::operator = (OleDate od)
{
	if(od == 0) {
		d.v = 0;
		t.v = 0;
		return *this;
	}
	else {
		SYSTEMTIME st;
		VariantTimeToSystemTime(od, &st);
		return (*this = st);
	}
}

#endif // }

LDATETIME & SLAPI LDATETIME::SetZero()
{
	d = ZERODATE;
	t = ZEROTIME;
	return *this;
}

long FASTCALL LDATETIME::settotalsec(long s)
{
	long   inc_dt = s / (3600 * 24);
	if(inc_dt)
		d = plusdate(d, inc_dt);
	t.settotalsec(s % (3600 * 24));
	return inc_dt;
}

LDATETIME & FASTCALL LDATETIME::addhs(long n)
{
	int    h = t.addhs(n).hour();
	if(h > 24) {
		const long numdays = h / 24;
		h = h % 24;
		plusdate(d, numdays);
		t = encodetime(h, t.minut(), t.sec(), t.hs()); // @v10.0.0 @fix (h % 24)-->h
	}
	else if(h < 0) {
		const long numdays = (h / 24) - 1;
		h = (-h) % 24;
		plusdate(d, numdays);
		t = encodetime(h, t.minut(), t.sec(), t.hs()); // @v10.0.0 @fix (h % 24)-->h
	}
	return *this;
}

LDATETIME & FASTCALL LDATETIME::addsec(long nsec)
{
	*this = plusdatetime(*this, nsec, 3);
	return *this;
}

int FASTCALL cmp(const LDATETIME & t1, const LDATETIME & t2)
{
	int    r = (t1.d > t2.d) ? 1 : ((t1.d < t2.d) ? -1 : 0);
	return NZOR(r, (t1.t > t2.t) ? 1 : ((t1.t < t2.t) ? -1 : 0));
}

int FASTCALL cmp(const LDATETIME & t1, LDATE dt, LTIME tm)
{
	int    r = cmp_ulong(t1.d, dt);
	return NZOR(r, cmp_ulong(t1.t, tm));
}

#ifndef _WIN32_WCE // {

LDATE::operator OleDate() const
{
	LDATETIME dt;
	dt.Set(*this, ZEROTIME);
	return dt;
}

LDATE LDATE::operator = (OleDate od)
{
	LDATETIME dt;
	dt = od;
	return (*this = dt.d);
}

LDATE LDATE::operator = (double od)
{
	LDATETIME dt;
	OleDate _od;
	_od.v = od;
	dt = _od;
	return (*this = dt.d);
}

#endif // }

time_t SLAPI LDATE::GetTimeT() const
{
	if(!checkdate(*this, 0))
		return 0;
	else {
		struct tm _t;
		_t.tm_year = year()-1900;
		_t.tm_mon = month()-1; // @v10.0.02 @fix (-1)
		_t.tm_mday = day();
		_t.tm_hour = 0;
		_t.tm_min = 0;
		_t.tm_sec = 0;
		return (sizeof(time_t) == 8) ? _mktime64(&_t) : mktime(&_t);
	}
}

int LDATE::dayspermonth() const
{
	return ::dayspermonth(month(), year());
}

int LDATE::weekno() const
{
	int    m = month();
	LDATE  temp_dt = *this;
	int    dow = dayofweek(&temp_dt);
	temp_dt = plusdate(*this, -NZOR(dow, 7));
	if(temp_dt.month() != m)
		return 1; // first week
	else {
		for(int i = 1; i <= 3; i++)
			if(plusdate(temp_dt, -(7*i)).month() != m)
				return (i+1);
		return 5;
	}
}

int FASTCALL LDATE::setday(uint d)
{
	if((d > 0 && d <= 31) || d == ANY_DAYITEM_VALUE) {
		(v &= ~0xff) |= d;
		return 1;
	}
	else
		return 0;
}

int FASTCALL LDATE::setmonth(uint m)
{
	if((m > 0 && m <= 12) || m == ANY_MONITEM_VALUE) {
		(v &= ~0x0000ff00) |= (m << 8);
		return 1;
	}
	else
		return 0;
}

int FASTCALL LDATE::setyear(uint y)
{
	if((y > 0 && y < 6000) || y == ANY_YEARITEM_VALUE) {
		(v &= ~0xffff0000) |= (y << 16);
		return 1;
	}
	else
		return 0;
}

int LDATE::getclass() const
{
	if(v == 0)
		return cZero;
	else if(year() & 0x8000 || year() & 0x4000 || year() & 0x2000 || month() & 0x80 || day() & 0x80)
		return cSpecial;
	else if(checkdate(*this, 0))
		return cNormal;
	else
		return cInvalid;
}

int LDATE::encode(int d, int m, int y)
{
	int    shift;
	int    x;
	int    d_ = 0, m_ = 0, y_ = 0;
	if(d == ANY_DATE_VALUE) {
		d_ = ANY_DAYITEM_VALUE;
	}
	else if(d & REL_DATE_MASK) {
		shift = (int)(int16)LoWord(d);
		if(m == -1 && y == -1) {
			v = MakeLong(shift, 0x8000);
			return 1;
		}
		else {
			if(shift < 0)
				x = (shift <= -31) ? (0x40 | 31) : (0x40 | (-shift));
			else if(shift > 0)
				x = (shift >= 31) ? 31 : shift;
			else
				x = 0;
			d_ = (0x80 | x);
		}
	}
	else if(d & THRSMDAY_DATE_MASK) {
		shift = (int)(int16)LoWord(d);
		if(shift >= 1 && shift <= 31) {
			v = MakeLong(shift, 0x2000);
			return 1;
		}
		else {
			v = 0;
			return 0;
		}
	}
	else
		d_ = d;
	if(m == ANY_DATE_VALUE) {
		m_ = ANY_MONITEM_VALUE;
	}
	else if(m & REL_DATE_MASK) {
		shift = (int)(int16)LoWord(m);
		if(shift < 0)
			x = (shift <= -24) ? (0x40 | 24) : (0x40 | (-shift));
		else if(shift > 0)
			x = (shift >= 24) ? 24 : shift;
		else
			x = 0;
		m_ = (0x80 | x);
	}
	else
		m_ = m;
	if(y == ANY_DATE_VALUE) {
		y_ = ANY_YEARITEM_VALUE;
	}
	else if(y & REL_DATE_MASK) {
		shift = (int)(int16)LoWord(y);
		if(shift < 0)
			x = (shift <= -255) ? (0x0400 | 255) : (0x0400 | (-shift));
		else if(shift > 0)
			x = (shift >= 255) ? 255 : shift;
		else
			x = 0;
		y_ = (0x4000 | x);
	}
	else
		y_ = y;
	v = (ulong)MakeLong((m_ << 8) | d_, y_);
	return 1;
}

int LDATE::decode(int * pD, int * pM, int * pY) const
{
	int    ok = 1;
	if(*this == MAXDATE) {
		ok = MAXDATEVALID.decode(pD, pM, pY);
	}
	else {
		int    d, m, y;
		y = year();
		if(y == 0x8000) {
			d = MakeLong((int16)v, 0x8000);
			y = -1;
			m = -1;
		}
		else if(y == 0x2000) {
			d = MakeLong((int16)v, 0x2000);
			y = -1;
			m = -1;
		}
		else {
			d = day();
			m = month();
			if(m == ANY_MONITEM_VALUE)
				m = ANY_DATE_VALUE;
			else if(m & 0x80) {
				const int plus = (m & 0x40) ? -(m & ~0xc0) : (m & ~0xc0);
				m = MakeLong(plus, 0x8000);
			}
			if(d == ANY_DAYITEM_VALUE)
				d = ANY_DATE_VALUE;
			else if(d & 0x80) {
				const int plus = (d & 0x40) ? -(d & ~0xc0) : (d & ~0xc0);
				d = MakeLong(plus, 0x8000);
			}
			if(y == ANY_YEARITEM_VALUE)
				y = ANY_DATE_VALUE;
			else if(y & 0x4000) {
				const int plus = (y & 0x0400) ? -(y & 0xff) : (y & 0xff);
				y = MakeLong(plus, 0x8000);
			}
		}
		ASSIGN_PTR(pD, d);
		ASSIGN_PTR(pM, m);
		ASSIGN_PTR(pY, y);
	}
	return ok;
}

int SLAPI LDATE::hasanycomponent() const
{
	const int _d = (int)(v & 0x00ff);
	const int _m = (int)((v & 0xff00) >> 8);
	const int _y = (int)(v >> 16);
	int   result = 0;
	if(_y == ANY_YEARITEM_VALUE)
		result |= 0x01;
	if(_m == ANY_MONITEM_VALUE)
		result |= 0x02;
	if(_d == ANY_DAYITEM_VALUE)
		result |= 0x04;
	return result;
}

LDATE SLAPI LDATE::Helper_GetActual(LDATE rel, LDATE cmp) const
{
	LDATE  result;
	if(v == 0)
		result = ZERODATE;
	else {
		int    y = year();
		if(y == 0x8000) {
			SETIFZ(rel, getcurdate_());
			result = (LoWord(v) == 0x8000) ? ZERODATE : plusdate(rel, (int16)v);
		}
		// @v8.7.2 {
		else if(y == 0x2000) {
			SETIFZ(rel, getcurdate_());
			int    thrs = LoWord(v);
			if(thrs >= 1 && thrs <= 31) {
				int    d = rel.day();
				int    m = rel.month();
				int    y = rel.year();
				if(d < thrs) {
					if(m == 1) {
						m = 12;
						y--;
					}
					else
						m--;
				}
				d = 1;
				result = encodedate(d, m, y);
			}
			else
				result = ZERODATE;
		}
		// } @v8.7.2
		else {
			int    m = month();
			int    d = day();
			int    plus_y = 0;
			int    plus_m = 0;
			int    plus_d = 0;
			if(m == ANY_MONITEM_VALUE) {
				if(cmp)
					m = cmp.month();
			}
			else if(m & 0x80) {
				SETIFZ(rel, getcurdate_());
				plus_m = (m & 0x40) ? -(m & ~0xc0) : (m & ~0xc0);
				m = rel.month();
			}
			if(d == ANY_DAYITEM_VALUE) {
				if(cmp)
					d = cmp.day();
			}
			else if(d & 0x80) {
				SETIFZ(rel, getcurdate_());
				plus_d = (d & 0x40) ? -(d & ~0xc0) : (d & ~0xc0);
				d = rel.day();
			}
			if(y == ANY_YEARITEM_VALUE) {
				if(cmp)
					y = cmp.year();
			}
			else if(y & 0x4000) {
				SETIFZ(rel, getcurdate_());
				plus_y = (y & 0x0400) ? -(y & 0xff) : (y & 0xff);
				y = rel.year();
			}
			result.setyear(y);
			result.setmonth(m);
			if(d != ANY_DAYITEM_VALUE && d > daysPerMonth[m-1])
				d = ((y % 4) == 0 && m == 2 && d >= 29) ? 29 : daysPerMonth[m-1];
			result.setday(d);
			if(plus_y)
				plusperiod(&result, PRD_ANNUAL, plus_y, 0);
			if(plus_m)
				plusperiod(&result, PRD_MONTH, plus_m, 0);
			if(plus_d)
				plusperiod(&result, PRD_DAY, plus_d, 0);
		}
	}
	return result;
}

LDATE SLAPI LDATE::getactualcmp(LDATE rel, LDATE cmp) const { return Helper_GetActual(rel, cmp); }
LDATE FASTCALL LDATE::getactual(LDATE rel) const { return Helper_GetActual(rel, ZERODATE); }
//
//
//
CALDATE CALDATE::operator = (long val)
{
	v = val;
	return *this;
}

IMPL_INVARIANT_C(CALDATE)
{
	S_INVARIANT_PROLOG(pInvP);
	int kind = GetKind();
	S_ASSERT_P(oneof3(kind, kDate, kDayOfWeek, kCalDate), pInvP);
	if(kind == kDate) {
		S_ASSERT_P(checkdate(*this, 1), pInvP);
	}
	else if(kind == kCalDate) {
		S_ASSERT_P(_checkdate(day(), month(), 2000), pInvP);
	}
	S_INVARIANT_EPILOG(pInvP);
}

int CALDATE::GetKind() const
{
	int    d, m, y;
	decodedate(&d, &m, &y, this);
	if(!v)
		return kDate;
	else if(d >= 1 && d <= 7 && m == 0 && y == 0) // day of week
		return kDayOfWeek;
	else if(y == 0) // calendar date
		return kCalDate;
	else // simple date
		return kDate;
}

int CALDATE::IsDate(LDATE dt) const
{
	int    d, m, y;
	decodedate(&d, &m, &y, this);
	if(d >= 1 && d <= 7 && !m && !y) // day of week
		return (d == dayofweek(this, 1)) ? 1 : 0;
	else if(d && m && !y) // calendar date
		return (d == dt.day() && m == dt.month()) ? 1 : 0;
	else // simple date
		return (dt == *this) ? 1 : 0;
}

SString & CALDATE::Format(int options, SString & rBuf) const
{
	rBuf.Z();
	int    d, m, y;
	decodedate(&d, &m, &y, this);
	if(d >= 1 && d <= 7 && m == 0 && y == 0) { // day of week
		GetDayOfWeekText(dowtRuShrt, d, rBuf);
		rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	}
	else if(y == 0) { // calendar date
		//char   temp_buf[64];
		SString temp_buf;
		SGetMonthText(m, MONF_SHORT|MONF_OEM, temp_buf);
		rBuf.Cat(d).Space().Cat(/*getMonthText(m, MONF_SHORT|MONF_OEM, temp_buf)*/temp_buf);
	}
	else // simple date
		rBuf.Cat(*this, DATF_DMY);
	return rBuf;
}

int CALDATE::SetDate(LDATE dt) { return checkdate(dt, 1) ? ((*this = dt), 1) : 0; }
int CALDATE::SetDayOfWeek(int dayOfWeek) { return (dayOfWeek >= 1 && dayOfWeek <= 7) ? ((*this = encodedate(dayOfWeek, 0, 0)), 1) : 0; }

int CALDATE::SetCalDate(int day, int mon)
{
	const LDATE temp_date = encodedate(day, mon, 2000);
	return checkdate(temp_date, 0) ? ((*this = encodedate(day, mon, 0).v), 1) : 0;
}
//
//
//
SLAPI STimeChunk::STimeChunk() : Start(ZERODATETIME), Finish(ZERODATETIME)
{
}

SLAPI STimeChunk::STimeChunk(const LDATETIME & rStart, const LDATETIME & rFinish)
{
	Init(rStart, rFinish);
}

void SLAPI STimeChunk::Init(const LDATETIME & start, const LDATETIME & finish)
{
	Start = start;
	if(finish.d)
		Finish = finish;
	else
		Finish.SetFar();
}

void SLAPI STimeChunk::Init(const LDATETIME & start, long cont)
{
	Start = start;
	Finish = plusdatetime(start, cont, 3);
	if(!start.d)
		Finish.d = ZERODATE;
}

int FASTCALL STimeChunk::operator == (const STimeChunk & rTest) const
	{ return BIN(::cmp(this->Start, rTest.Start) == 0 && ::cmp(this->Finish, rTest.Finish) == 0); }
int FASTCALL STimeChunk::operator != (const STimeChunk & rTest) const
	{ return BIN(::cmp(this->Start, rTest.Start) != 0 || ::cmp(this->Finish, rTest.Finish) != 0); }

int FASTCALL STimeChunk::cmp(const STimeChunk & rTest) const
{
	int    r = ::cmp(this->Start, rTest.Start);
	return NZOR(r, ::cmp(this->Finish, rTest.Finish));
}

int FASTCALL STimeChunk::ContainsIn(const STimeChunk & rDur) const
{
	STimeChunk result;
	return BIN(Intersect(rDur, &result) && *this == result);
}

int FASTCALL STimeChunk::Has(const LDATETIME & rTm) const { return BIN(::cmp(rTm, Start) >= 0 && ::cmp(rTm, Finish) <= 0); }

int FASTCALL STimeChunk::Intersect(const STimeChunk & test, STimeChunk * pResult) const
{
	int    is = 0;
	LDATETIME st, fn;
	if(::cmp(Start, test.Finish) > 0 || ::cmp(Finish, test.Start) < 0) {
		st.SetZero();
		fn.SetZero();
	}
	else {
		st = (::cmp(Start, test.Start) > 0) ? Start : test.Start;
		fn = (::cmp(Finish, test.Finish) < 0) ? Finish : test.Finish;
		is = 1;
	}
	CALLPTRMEMB(pResult, Init(st, fn));
	return is;
}

SString & SLAPI STimeChunk::ToStr(SString & rBuf, long fmt) const
{
	const long datf = DATF_DMY;
	const long timf = (fmt & fmtOmitSec) ? TIMF_HM : TIMF_HMS;
	if(Start.d == Finish.d) {
		if(Start.d && !Finish.IsFar())
			rBuf.Cat(Start.d, DATF_DMY).Space();
		rBuf.Cat(Start.t, timf).CatCharN('.', 2).Cat(Finish.t, timf);
	}
	else {
		if(Start.d)
			rBuf.Cat(Start, datf, timf);
		rBuf.CatCharN('.', 2);
		if(!Finish.IsFar())
			rBuf.Cat(Finish, datf, timf);
	}
	return rBuf;
}

long SLAPI STimeChunk::GetDurationDays() const
	{ return (Start.d && Finish.d && !Finish.IsFar()) ? (diffdate(Finish.d, Start.d)+1) : -1; }
long SLAPI STimeChunk::GetDuration() const
	{ return (Start.d && Finish.d && !Finish.IsFar()) ? diffdatetimesec(Finish, Start) : -1; }

int64 SLAPI STimeChunk::GetDurationMs() const
{
	if(Start.d && Finish.d && !Finish.IsFar()) {
		long days = 0;
		long diff = diffdatetime(Finish, Start, 4, &days);
		return ((int64)(diff * 10) + ((int64)days * 1000 * 60 * 60 * 24));
	}
	else
		return -1;
}

IMPL_CMPFUNC(STimeChunk, i1, i2) { return pExtraData ? ((const STimeChunk *)i2)->cmp(*(const STimeChunk *)i1) : ((const STimeChunk *)i1)->cmp(*(const STimeChunk *)i2); }
//
//
//
//static
int FASTCALL DateRepeating::IsValidPrd(int prd)
	{ return oneof6(prd, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL); }
static inline int FASTCALL DateRepeating_IsEqual(const DateRepeating & rS1, const DateRepeating & rS2)
	{ return (rS1.Prd == rS2.Prd && rS1.RepeatKind == rS2.RepeatKind && *PTR32(&rS1.Dtl) == *PTR32(&rS2.Dtl)); }
int FASTCALL DateRepeating::operator == (const DateRepeating & rS) const
	{ return DateRepeating_IsEqual(*this, rS); }
int FASTCALL DateRepeating::operator != (const DateRepeating & rS) const
	{ return !DateRepeating_IsEqual(*this, rS); }

int SLAPI DateRepeating::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	const size_t offs = offsetof(DateRepeating, Prd);
	return pSCtx->SerializeBlock(dir, sizeof(*this)-offs, PTR8(this)+offs, rBuf, 0);
}

int SLAPI DateRepeating::GetMonthlyPeriod(int * pMonthCount, int * pMonthNo) const
{
	int    ok = 0;
	int    month_count = 1;
	int    month_no = 1;
	if(Prd == PRD_MONTH) {
		month_count = (int)(Dtl.ME.NumPrd & 0x00ff);
		month_no = (int)((Dtl.ME.NumPrd >> 8) & 0x00ff);
		month_count = MINMAX(month_count, 1, 100);
		month_no = MINMAX(month_no, 0, month_count);
		SETIFZ(month_no, month_count);
		ok = 1;
	}
	ASSIGN_PTR(pMonthCount, month_count);
	ASSIGN_PTR(pMonthNo, month_no);
	return ok;
}

int SLAPI DateRepeating::SetMonthly(int monthCount, int monthNo, int dayOfMonth)
{
	Prd = PRD_MONTH;
	RepeatKind = 1;
	int month_count = MINMAX(monthCount, 1, 100);
	int month_no = MINMAX(monthNo, 0, month_count);
	int day_of_month = MINMAX(dayOfMonth, 1, 31);
	Dtl.ME.NumPrd = (((int8)month_count) | (((int8)month_no) << 8));
	Dtl.ME.DayOfMonth = (uint8)day_of_month;
	Dtl.ME.Zero = 0;
	return (month_count == monthCount && month_no == monthNo && day_of_month == dayOfMonth) ? 1 : 100;
}

int SLAPI DateRepeating::SetMonthly(int monthCount, int monthNo, int weekNo, int dayOfWeek)
{
	Prd = PRD_MONTH;
	RepeatKind = 2;
	int month_count = MINMAX(monthCount, 1, 100);
	int month_no = MINMAX(monthNo, 0, month_count);
	int week_no = MINMAX(weekNo, 1, 5);
	int day_of_week = MINMAX(dayOfWeek, 1, 7);
	Dtl.MY.NumPrd = (((int8)month_count) | (((int8)month_no) << 8));
	Dtl.MY.WeekNo = (uint8)week_no;
	Dtl.MY.DayOfWeek = (uint8)day_of_week;
	return (month_count == monthCount && month_no == monthNo && week_no == weekNo && day_of_week == dayOfWeek) ? 1 : 100;
}

int SLAPI DateRepeating::DayOfWeekNo(LDATE dt, int weekNo, int dayOfWeek, LDATE * pResult) const
{
	int    ok = 0;
	LDATE  temp_dt = dt;
	if(!checkdate(&dt))
		ok = 0;
	else {
		temp_dt = encodedate(1, dt.month(), dt.year());
		int    dow = dayofweek(&temp_dt, 1);
		if(weekNo == 1 && dayOfWeek >= dow) {
			temp_dt = plusdate(temp_dt, dayOfWeek-dow);
			ok = 1;
		}
		else if(weekNo > 0 && weekNo < 5) {
			temp_dt = plusdate(temp_dt, 7-dow+1 + (weekNo-1) * 7); // temp_dt - понедельник требуемой недели
			temp_dt = plusdate(temp_dt, dayOfWeek - 1);
			ok = 1;
		}
		else { // последняя неделя //
			temp_dt = encodedate(dt.dayspermonth(), dt.month(), dt.year());
			do {
				if(dayofweek(&temp_dt, 1) == dayOfWeek)
					ok = 1;
				else
					temp_dt = plusdate(temp_dt, -1);
			} while(!ok);
		}
	}
	ASSIGN_PTR(pResult, temp_dt);
	return ok;
}

int SLAPI DateRepeating::Init(int prd, int kind, LDATE dt)
{
	int    ok = 1;
	int    dow = -1;
	if(dt)
		dow = dayofweek(&dt, 1);
	THISZERO();
	Prd = prd;
	if(Prd == PRD_DAY) {
		Dtl.D.NumPrd = 1;
	}
	else if(Prd == PRD_WEEK) {
		Dtl.W.NumPrd = 1;
		if(dow > 0)
			Dtl.W.Weekdays |= (1 << dow);
		else
			Dtl.W.Weekdays |= 0x01;
	}
	if(Prd == PRD_MONTH) {
		Dtl.ME.NumPrd = 1;
		if(kind == 1) {
			RepeatKind = 1;
			Dtl.ME.DayOfMonth = dt ? dt.day() : 1;
		}
		else {
			RepeatKind = 2;
			if(dt) {
				Dtl.MY.DayOfWeek = (dow > 0) ? dow : 1;
				Dtl.MY.WeekNo = dt.weekno();
			}
			else {
				Dtl.MY.DayOfWeek = 1;
				Dtl.MY.WeekNo = 1;
			}
		}
	}
	if(Prd == PRD_ANNUAL) {
		if(kind == 1) {
			RepeatKind = 1;
			if(dt) {
				Dtl.AE.Month = dt.month();
				Dtl.AE.DayOfMonth = dt.day();
			}
			else {
				Dtl.AE.Month = 1;
				Dtl.AE.DayOfMonth = 1;
			}
		}
		else {
			RepeatKind = 2;
			if(dt) {
				Dtl.AY.DayOfWeek = (dow > 0) ? dow : 1;
				Dtl.AY.WeekNo = dt.weekno();
			}
			else {
				Dtl.AY.DayOfWeek = 1;
				Dtl.AY.WeekNo = 1;
			}
		}
	}
	else if(Prd == PRD_REPEATAFTERPRD) {
		Dtl.RA.NumPrd     = 1;
		Dtl.RA.AfterStart = 0;
		RepeatKind = PRD_DAY;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DateRepeating::Next_(LDATE startDate, LDATE * pNextDate) const
{
	int    ok = -1;
	LDATE  temp_dt = NZOR(startDate, getcurdate_());
	if(Prd == PRD_DAY) {
		if(Dtl.D.NumPrd > 0) {
			plusperiod(&temp_dt, Prd, Dtl.D.NumPrd, 0);
			ASSIGN_PTR(pNextDate, temp_dt);
			ok = 1;
		}
	}
	else if(Prd == PRD_WEEK) {
		if(Dtl.W.NumPrd > 0) {
			int    i;
			temp_dt = plusdate(temp_dt, 1);
			int    dow = dayofweek(&temp_dt, 1);
			for(i = dow; ok < 0 && i <= 7; i++)
				if(Dtl.W.Weekdays & (1 << (i-1))) {
					temp_dt = plusdate(temp_dt, i-dow);
					ASSIGN_PTR(pNextDate, temp_dt);
					ok = 1;
				}
			if(ok < 0) {
				plusperiod(&temp_dt, Prd, Dtl.W.NumPrd, 0);
				dow = dayofweek(&temp_dt, 1);
				for(i = 1; ok < 0 && i <= 7; i++)
					if(Dtl.W.Weekdays & (1 << (i-1))) {
						temp_dt = plusdate(temp_dt, i-dow);
						ASSIGN_PTR(pNextDate, temp_dt);
						ok = 1;
					}
			}
		}
	}
	else if(Prd == PRD_MONTH) {
		int    month_count = 1;
		int    month_no = 1;
		if(GetMonthlyPeriod(&month_count, &month_no)) {
			plusperiod(&temp_dt, Prd, month_count, 0);
			if(RepeatKind == 1) {
				temp_dt = encodedate(Dtl.ME.DayOfMonth, temp_dt.month(), temp_dt.year());
			}
			else {
				DayOfWeekNo(temp_dt, Dtl.MY.WeekNo, Dtl.MY.DayOfWeek, &temp_dt);
			}
			ASSIGN_PTR(pNextDate, temp_dt);
			ok = 1;
		}
	}
	else if(Prd == PRD_ANNUAL) {
		if(RepeatKind == 1) {
			plusperiod(&temp_dt, Prd, 1, 0);
			temp_dt = encodedate(Dtl.AE.DayOfMonth, Dtl.AE.Month, temp_dt.year());
			ASSIGN_PTR(pNextDate, temp_dt);
			ok = 1;
		}
		else {
			plusperiod(&temp_dt, Prd, 1, 0);
			temp_dt = encodedate(1, Dtl.AE.Month, temp_dt.year());
			DayOfWeekNo(temp_dt, Dtl.AY.WeekNo, Dtl.AY.DayOfWeek, &temp_dt);
			ASSIGN_PTR(pNextDate, temp_dt);
			ok = 1;
		}
	}
	else if(Prd == PRD_REPEATAFTERPRD) {
		plusperiod(&temp_dt, RepeatKind, Dtl.RA.NumPrd, 0);
		ASSIGN_PTR(pNextDate, temp_dt);
		ok = 1;
	}
	return ok;
}
/*
int SLAPI DateRepeating::Format(int fmt, SString & rBuf) const
{
	rBuf.Z();
	switch(Prd) {
		case PRD_DAY:
			rBuf.Cat("DAYLY");
			if(Dtl.D.NumPrd)
				r.Buf.CatDiv('-', 1).Cat((long)Dtl.D.NumPrd);
			if(Dtl.D.Quant) {
				LTIME tm;
				tm.SetTotalSec(Dtl.D.Quant);
				rBuf.CatDiv('-', 1).Cat(tm);
			}
			break;
		case PRD_WEEK:
			rBuf.Cat("WEEKLY");
			if(Dtl.D.NumPrd)
				r.Buf.CatDiv('-', 1).Cat((long)Dtl.D.NumPrd);
			if(Dtl.D.Weekdays) {
				for(uint i = 0; i < 6; i++) {

				}
				LTIME tm;
				tm.SetTotalSec(Dtl.D.Quant);
				rBuf.CatDiv('-', 1).Cat(tm);
			}
			break;
		case PRD_MONTH:
			rBuf.Cat("MONTHLY");
			break;
		case PRD_ANNUAL:
			rBuf.Cat("ANNUALY");
			break;
	}
}
*/

long SLAPI DateRepeating::DtlToLong()
{
	switch(Prd) {
		case PRD_DAY:    return *(long*)&Dtl.D;
		case PRD_WEEK:   return *(long*)&Dtl.W;
		case PRD_MONTH:  return (RepeatKind == 1) ? *(long*)&Dtl.ME : *(long*)&Dtl.MY;
		case PRD_ANNUAL: return (RepeatKind == 1) ? *(long*)&Dtl.AE : *(long*)&Dtl.AY;
		case PRD_REPEATAFTERPRD: return *(long*)&Dtl.RA;
	}
	return 0;
}

int SLAPI DateRepeating::LongToDtl(long v)
{
	if(Prd == PRD_DAY)
		Dtl.D = *(RepeatDay*)&v;
	else if(Prd == PRD_WEEK)
		Dtl.W = *(RepeatWeek*)&v;
	if(Prd == PRD_MONTH) {
		if(RepeatKind == 1)
			Dtl.ME = *(RepeatMonthDate*)&v;
		else
			Dtl.MY = *(RepeatMonthDay*)&v;
	}
	if(Prd == PRD_ANNUAL) {
		if(RepeatKind == 1)
			Dtl.AE = *(RepeatAnnDate*)&v;
		else
			Dtl.AY = *(RepeatAnnDay*)&v;
	}
	else if(Prd == PRD_REPEATAFTERPRD)
		Dtl.RA = *(RepeatAfterPrd*)&v;
	return 1;
}

SString & SLAPI DateRepeating::Format(int fmt, SString & rBuf) const
{
	rBuf.Z();
	SString dowtxt;
	switch(Prd) {
		case PRD_DAY:
			rBuf.Cat("DAYLY").CatDiv('-', 1);
			if(Dtl.D.NumPrd)
				rBuf.Cat((long)Dtl.D.NumPrd).CatDiv(';', 2);
			if(Dtl.D.QuantSec) {
				LTIME tm;
				tm.settotalsec(Dtl.D.QuantSec);
				rBuf.Cat(tm, TIMF_HMS).CatDiv(';', 2);
			}
			break;
		case PRD_WEEK:
			rBuf.Cat("WEEKLY").CatDiv('-', 1);
			if(Dtl.W.NumPrd)
				rBuf.Cat((long)Dtl.W.NumPrd).CatDiv(';', 2);
			if(Dtl.W.Weekdays) {
				int    _first = 1;
				for(uint i = 0; i < 7; i++) {
					uint b = 1 << i;
					if(Dtl.W.Weekdays & b) {
						if(!_first)
							rBuf.CatDiv(',', 2);
						GetDayOfWeekText(dowtEnShrt, i+1, dowtxt);
						rBuf.Cat(dowtxt);
						_first = 0;
					}
				}
				// @v9.4.9 rBuf.TrimRight().ToOem().CatDiv(';', 2);
				rBuf.TrimRight().Transf(CTRANSF_OUTER_TO_INNER).CatDiv(';', 2); // @v9.4.9
			}
			break;
		case PRD_MONTH:
			{
				long num_prd = 0;
				SString buf;
				rBuf.Cat("MONTHLY").CatDiv('-', 1);
				if(RepeatKind == 1) {
					num_prd = (long)Dtl.ME.NumPrd;
					buf.Cat((long)Dtl.ME.DayOfMonth);
				}
				else {
					num_prd = (long)Dtl.MY.NumPrd;
					buf.Cat((long)Dtl.MY.WeekNo);
					if(Dtl.MY.DayOfWeek) {
						GetDayOfWeekText(dowtEnShrt, Dtl.MY.DayOfWeek, dowtxt);
						buf.CatDiv(',', 2).Cat(dowtxt);
					}
				}
				if(num_prd)
					rBuf.CatDiv('-', 1).Cat(num_prd).CatDiv(';', 2);
				// @v9.4.9 rBuf.Cat(buf).ToOem().CatDiv(';', 2);
				rBuf.Cat(buf).Transf(CTRANSF_OUTER_TO_INNER).CatDiv(';', 2); // @v9.4.9
			}
			break;
		case PRD_ANNUAL:
			{
				long month = 0;
				//char month_txt[24];
				SString month_txt;
				SString buf;
				rBuf.Cat("ANNUALY");
				if(RepeatKind == 1) {
					month = (long)Dtl.AE.Month;
					buf.Cat((long)Dtl.AE.DayOfMonth);
				}
				else {
					month = (long)Dtl.AY.Month;
					buf.Cat((long)Dtl.AY.WeekNo);
					if(Dtl.AY.DayOfWeek) {
						GetDayOfWeekText(dowtEnShrt, Dtl.AY.DayOfWeek, dowtxt);
						buf.Comma().Cat(dowtxt);
					}
				}
				if(month) {
					//memzero(month_txt, sizeof(month_txt));
					//getMonthText(month, MONF_CASENOM, month_txt);
					SGetMonthText(month, MONF_CASENOM, month_txt);
				}
				rBuf.CatDiv('-', 1).Cat(buf).Comma().Cat(month_txt).Semicol().Transf(CTRANSF_OUTER_TO_INNER);
			}
			break;
		case PRD_REPEATAFTERPRD:
			rBuf.Cat("REPEAT AFTER PERIOD").CatDiv('-', 1);
			if(Dtl.RA.NumPrd)
				rBuf.Cat((long)Dtl.RA.NumPrd).CatDiv(';', 2);
			break;

	}
	return rBuf;
}
//
//
//
int SLAPI DateTimeRepeating::Init(int prd, int kind, LDATE dt, LTIME tm)
{
	Time = tm;
	return DateRepeating::Init(prd, kind, dt);
}

int SLAPI DateTimeRepeating::Next_(LDATETIME startDtm, LDATETIME * pNextDtm) const
{
	int    ok = -1;
	LDATETIME temp_dtm = startDtm;
	if(Prd == PRD_DAY) {
		SETIFZ(temp_dtm.d, getcurdate_());
		const long quantsec = (long)Dtl.D.QuantSec;
		if(quantsec > 0) {
			if(temp_dtm.t < Time) {
				temp_dtm.t = Time;
				ok = 1;
			}
			else {
				const long s = (temp_dtm.t.totalsec() + quantsec);
				if(s < 24 * 60 * 60) {
					temp_dtm.t.settotalsec(s);
					ok = 1;
				}
			}
		}
		else if(temp_dtm.t < Time) {
			temp_dtm.t = Time;
			ok = 1;
		}
	}
	if(ok < 0)
		if(DateRepeating::Next_(startDtm.d, &temp_dtm.d) > 0) {
			temp_dtm.t = Time;
			ok = 1;
		}
	if(ok > 0)
		ASSIGN_PTR(pNextDtm, temp_dtm);
	return ok;
}

SString & SLAPI DateTimeRepeating::Format(int fmt, SString & rBuf) const
{
	rBuf.Z();
	DateRepeating::Format(fmt, rBuf);
	return rBuf.Cat(Time, TIMF_HMS);
}
//
//
//
DateRepIterator::DateRepIterator(const DateRepeating & rDr, LDATE startDate, LDATE endDate, uint maxCount)
{
	Dr = rDr;
	Start = NZOR(startDate, getcurdate_());
	End = endDate;
	MaxCount = maxCount;
	Count = 0;
	Cur = Start;
}

LDATE DateRepIterator::Next()
{
	if(Cur) {
		LDATE  result = ZERODATE;
		LDATE  temp_dt = Cur;
		switch(Dr.Prd) {
			case PRD_DAY:
				if(Dr.Dtl.D.NumPrd > 0) {
					plusperiod(&temp_dt, PRD_DAY, Dr.Dtl.D.NumPrd, 0);
					result = temp_dt;
				}
				break;
			case PRD_WEEK:
				if(Dr.Dtl.W.NumPrd > 0) {
					int    i;
					temp_dt = plusdate(temp_dt, 1);
					int    dow = dayofweek(&temp_dt, 1);
					for(i = dow; i <= 7; i++)
						if(Dr.Dtl.W.Weekdays & (1 << (i-1))) {
							result = plusdate(temp_dt, i-dow);
							break;
						}
					if(!result) {
						plusperiod(&temp_dt, PRD_WEEK, Dr.Dtl.W.NumPrd, 0);
						dow = dayofweek(&temp_dt, 1);
						for(i = 1; i <= 7; i++)
							if(Dr.Dtl.W.Weekdays & (1 << (i-1))) {
								result = plusdate(temp_dt, i-dow);
								break;
							}
					}
				}
				break;
			case PRD_MONTH:
				{
					int    month_count = 1;
					int    month_no = 1;
					if(Dr.GetMonthlyPeriod(&month_count, &month_no)) {
						temp_dt = Start;
						uint   nm = month_count * (Count+1);
						if(month_no && month_no < month_count)
							nm -= (month_count - month_no);
						plusperiod(&temp_dt, PRD_MONTH, nm, 0);
						if(Dr.RepeatKind == 1) {
							int dom = Dr.Dtl.ME.DayOfMonth;
							int max_dom = temp_dt.dayspermonth();
							SETMIN(dom, max_dom);
							temp_dt = encodedate(dom, temp_dt.month(), temp_dt.year());
						}
						else {
							Dr.DayOfWeekNo(temp_dt, Dr.Dtl.MY.WeekNo, Dr.Dtl.MY.DayOfWeek, &temp_dt);
						}
						result = temp_dt;
					}
				}
				break;
			case PRD_ANNUAL:
				plusperiod(&temp_dt, PRD_ANNUAL, 1, 0);
				if(Dr.RepeatKind == 1) {
					result = encodedate(Dr.Dtl.AE.DayOfMonth, Dr.Dtl.AE.Month, temp_dt.year());
				}
				else {
					temp_dt = encodedate(1, Dr.Dtl.AE.Month, temp_dt.year());
					Dr.DayOfWeekNo(temp_dt, Dr.Dtl.AY.WeekNo, Dr.Dtl.AY.DayOfWeek, &temp_dt);
					result = temp_dt;
				}
				break;
			case PRD_REPEATAFTERPRD:
				plusperiod(&temp_dt, Dr.RepeatKind, Dr.Dtl.RA.NumPrd, 0);
				result = temp_dt;
				break;
		}
		Count++;
		Cur = (!checkdate(result, 0) || (MaxCount && Count > MaxCount) || (End && result > End)) ? ZERODATE : result;
	}
	return Cur;
}

DateTimeRepIterator::DateTimeRepIterator(const DateTimeRepeating & rDr, LDATETIME startDtm, LDATE endDate, uint maxCount) :
	DtIter((const DateRepeating &)rDr, startDtm.d, endDate, 0)
{
	Dr = rDr;
	Start = startDtm;
	if(!Start.d)
		Start.d = getcurdate_();
	Cur = Start;
}

LDATETIME DateTimeRepIterator::Next()
{
	LDATETIME result = ZERODATETIME;
	if(Cur == ZERODATETIME) {
		LDATETIME temp_dtm = Cur;
		if(Dr.Prd == PRD_DAY) {
			if(Dr.Dtl.D.QuantSec > 0) {
				if(temp_dtm.t < Dr.Time) {
					temp_dtm.t = Dr.Time;
					result = temp_dtm;
				}
				else {
					long s = temp_dtm.t.totalsec();
					s += Dr.Dtl.D.QuantSec;
					if(s < 24 * 60 * 60) {
						temp_dtm.t.settotalsec(s);
						result = temp_dtm;
					}
				}
			}
			else if(temp_dtm.t < Dr.Time) {
				temp_dtm.t = Dr.Time;
				result = temp_dtm;
			}
		}
		if(result == ZERODATETIME) {
			LDATE next = DtIter.Next();
			if(next) {
				result.d = next;
				result.t = Dr.Time;
			}
		}
		Count++;
		Cur = (Count && Count > MaxCount) ? ZERODATETIME : result;
	}
	return Cur;
}
//
//
//
SCycleTimer::SCycleTimer(uint32 msDelay)
{
	Restart(msDelay);
}

void FASTCALL SCycleTimer::Restart(uint32 msDelay)
{
	Delay = msDelay;
	Last = getcurdatetime_();
}

int FASTCALL SCycleTimer::Check(LDATETIME * pLast)
{
	int    ok = 0;
	if(Delay) {
		const LDATETIME cur = getcurdatetime_();
		const LDATETIME next = plusdatetime(Last, Delay/10, 4);
		ASSIGN_PTR(pLast, Last);
		if(cmp(cur, next) >= 0) {
			Last = cur;
			ok = 1;
		}
	}
	return ok;
}
//
//
// @construction {
SUniTime::SUniTime()
{
	memzero(D, sizeof(D));
}

int FASTCALL SUniTime::SetSignature(uint8 s)
{
	D[7] = s;
	return 1;
}

int FASTCALL SUniTime::Set(const FILETIME & rD)
{
	PTR32(D)[0] = rD.dwLowDateTime;
	PTR32(D)[1] = rD.dwHighDateTime;
	assert((D[7] & indfScale) == 0);
	return 1;
}

int FASTCALL SUniTime::Set(const LDATE & rD)
{
	int    ok = 1;
	if(checkdate(rD, 0)) {
		PTR64(D)[0] = (((uint64)(indfScale | indDay)) << 56) | DateToDaysSinceChristmas(rD.year(), rD.month(), rD.day());
	}
	else {
		memzero(D, sizeof(D));
		ok = 0;
	}
	return ok;
}

struct SUniTime_Inner {
	SUniTime_Inner()
	{
		THISZERO();
	}
	int    Y;
	int    M;
	int    D;
	int    Hr;
	int    Mn;
	int    Sc;
	int    MSc;  // milliseconds
	int    MkSc; // microseconds
	int    Weekday;
};

#if 0 // {

#define TICKSPERMIN        600000000
#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       1
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12
#define WEEKDAY_OF_1601     1 // This is the week day that January 1st, 1601 fell on (a Monday)
#if defined(__GNUC__)
	#define TICKSTO1970         0x019db1ded53e8000LL
	#define TICKSTO1980         0x01a8e79fe1d58000LL
#else
	#define TICKSTO1970         0x019db1ded53e8000i64
	#define TICKSTO1980         0x01a8e79fe1d58000i64
#endif
//
//  ULONG
//  NumberOfLeapYears (IN ULONG ElapsedYears);
//
//  The number of leap years is simply the number of years divided by 4
//  minus years divided by 100 plus years divided by 400.  This says
//  that every four years is a leap year except centuries, and the
//  exception to the exception is the quadricenturies
//
#define NumberOfLeapYears(YEARS) (((YEARS)/4) - ((YEARS)/100) + ((YEARS)/400))
//
//  ULONG ElapsedYearsToDays (IN ULONG ElapsedYears);
//
//  The number of days contained in elapsed years is simply the number
//  of years times 365 (because every year has at least 365 days) plus
//  the number of leap years there are (i.e., the number of 366 days years)
//
#define ElapsedYearsToDays(YEARS) (((YEARS) * 365) + NumberOfLeapYears(YEARS))

/*static const unsigned int YearLengths[2] = { DAYSPERNORMALYEAR, DAYSPERLEAPYEAR };
static const uint8 MonthLengths[2][MONSPERYEAR] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};*/
//
//  The following two tables map a day offset within a year to the month
//  containing the day.  Both tables are zero based.  For example, day
//  offset of 0 to 30 map to 0 (which is Jan).
//
static const uint8 LeapYearDayToMonth[366] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // January
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // February
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // March
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // April
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // May
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  // June
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // July
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // August
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,  // September
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // October
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, // November
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
};                                                                                                 // December

static const uint8 NormalYearDayToMonth[365] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // January
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,        // February
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // March
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // April
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // May
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  // June
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // July
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // August
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,  // September
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // October
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, // November
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
};                                                                                                 // December
//
//  The following two tables map a month index to the number of days preceding
//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
//  has 31 days preceding it.  To help calculate the maximum number of days
//  in a month each table has 13 entries, so the number of days in a month
//  of index i is the table entry of i+1 minus the table entry of i.
//
static const int16 LeapYearDaysPrecedingMonth[13] = {
	0,                             // January
	31,                            // February
	31+29,                         // March
	31+29+31,                      // April
	31+29+31+30,                   // May
	31+29+31+30+31,                // June
	31+29+31+30+31+30,             // July
	31+29+31+30+31+30+31,          // August
	31+29+31+30+31+30+31+31,       // September
	31+29+31+30+31+30+31+31+30,    // October
	31+29+31+30+31+30+31+31+30+31, // November
	31+29+31+30+31+30+31+31+30+31+30, // December
	31+29+31+30+31+30+31+31+30+31+30+31
};

static const int16 NormalYearDaysPrecedingMonth[13] = {
	0,                             // January
	31,                            // February
	31+28,                         // March
	31+28+31,                      // April
	31+28+31+30,                   // May
	31+28+31+30+31,                // June
	31+28+31+30+31+30,             // July
	31+28+31+30+31+30+31,          // August
	31+28+31+30+31+30+31+31,       // September
	31+28+31+30+31+30+31+31+30,    // October
	31+28+31+30+31+30+31+31+30+31, // November
	31+28+31+30+31+30+31+31+30+31+30, // December
	31+28+31+30+31+30+31+31+30+31+30+31
};

static int FASTCALL DaysSinceEpoch(int year)
{
    year--; // Don't include a leap day from the current year 
    int days = year * DAYSPERNORMALYEAR + year / 4 - year / 100 + year / 400;
    days -= (EPOCHYEAR-1) * DAYSPERNORMALYEAR + (EPOCHYEAR-1) / 4 - (EPOCHYEAR-1) / 100 + (EPOCHYEAR-1) / 400;
    return days;
}

#define Convert100nsToMilliseconds(v) ((v) / 10000LL)
#define ConvertMillisecondsToDays(v)  ((v) / (SECSPERDAY * 1000))
#define ConvertDaysToMilliseconds(DAYS) ((DAYS) * (SECSPERDAY * 1000))
// 
// Descr: This routine converts an input 64-bit time value to the number
//   of total elapsed days and the number of milliseconds in the partial day.
// ARG(Time IN): Supplies the input time to convert from
// ARG(Milliseconds OUT): Receives the number of milliseconds in the partial day
// Returns:
//   ElapsedDays
//
static uint32 FASTCALL __TimeToDaysAndFraction(int64 Time, uint32 * pMilliseconds)
{
	int64  TotalMilliseconds = Convert100nsToMilliseconds(Time); // Convert the input time to total milliseconds
	int64  Temp = ConvertMillisecondsToDays(TotalMilliseconds); // Convert milliseconds to total days
	//
	//  Set the elapsed days from temp, we've divided it enough so that the high part must be zero.
	//
	uint32 elapsed_days = Temp & 0xffffffffLL;
	//
	//  Calculate the exact number of milliseconds in the elapsed days
	//  and subtract that from the total milliseconds to figure out
	//  the number of milliseconds left in the partial day
	//
	Temp = ConvertDaysToMilliseconds(elapsed_days);
	Temp = TotalMilliseconds - Temp;
	//
	//  Set the fraction part from temp, the total number of milliseconds in a day guarantees that the high part must be zero.
	//
	*pMilliseconds = Temp & 0xffffffffLL;
	return elapsed_days;
}
//   
// Descr: This routine computes the number of total years contained in the indicated
//   number of elapsed days.  The computation is to first compute the number of
//   400 years and subtract that it, then do the 100 years and subtract that out,
//   then do the number of 4 years and subtract that out.  Then what we have left
//   is the number of days with in a normalized 4 year block.  Normalized being that
//   the first three years are not leap years.
// ARG(ElapsedDays IN): Supplies the number of days to use
// Returns:
//   ULONG - Returns the number of whole years contained within the input number of days.
//   
static uint32 FASTCALL ElapsedDaysToYears(uint32 elapsedDays)
{
	//
	// A 400 year time block is 365*400 + 400/4 - 400/100 + 400/400 = 146097 days long.  
	// So we simply compute the number of whole 400 year block and the the number days 
	// contained in those whole blocks, and subtract if from the elapsed day total
	//
	uint32 NumberOf400s = elapsedDays / 146097;
	elapsedDays -= NumberOf400s * 146097;
	//
	// A 100 year time block is 365*100 + 100/4 - 100/100 = 36524 days long.
	// The computation for the number of 100 year blocks is biased by 3/4 days per
	// 100 years to account for the extra leap day thrown in on the last year of each 400 year block.
	//
	uint32 NumberOf100s = (elapsedDays * 100 + 75) / 3652425;
	elapsedDays -= NumberOf100s * 36524;
	//
	// A 4 year time block is 365*4 + 4/4 = 1461 days long.
	//
	uint32 NumberOf4s = elapsedDays / 1461;
	elapsedDays -= NumberOf4s * 1461;
	//
	// Now the number of whole years is the number of 400 year blocks times 400,
	// 100 year blocks time 100, 4 year blocks times 4, and the number of elapsed
	// whole years, taking into account the 3/4 day per year needed to handle the leap year.
	//
	return (NumberOf400s * 400) + (NumberOf100s * 100) + (NumberOf4s * 4) + (elapsedDays * 100 + 75) / 36525;
}
// 
// Routine Description:
//   This routine converts an input 64-bit LARGE_INTEGER variable to its corresponding
//   time field record.  It will tell the caller the year, month, day, hour,
//   minute, second, millisecond, and weekday corresponding to the input time variable.
// Arguments:
//   Time - Supplies the time value to interpret
//   TimeFields - Receives a value corresponding to Time
//
static void FASTCALL __RtlTimeToTimeFields(int64 Time, /*InternalTimeFields*/SUniTime_Inner * TimeFields)
{
	uint32 month;
	uint32 hours;
	uint32 minutes;
	uint32 seconds;
	uint32 milliseconds;
	//
	//  First divide the input time 64 bit time variable into
	//  the number of whole days and part days (in milliseconds)
	//
	uint32 days = __TimeToDaysAndFraction(Time, &milliseconds);
	//
	//  Compute which weekday it is and save it away now in the output
	//  variable.  We add the weekday of the base day to bias our computation
	//  which means that if one day has elapsed then we the weekday we want
	//  is the Jan 2nd, 1601.
	//
	TimeFields->Weekday = (int16)((days + WEEKDAY_OF_1601) % 7);
	//
	//  Calculate the number of whole years contained in the elapsed days
	//  For example if Days = 500 then Years = 1
	//
	uint32 years = ElapsedDaysToYears(days);
	//
	//  And subtract the number of whole years from our elapsed days
	//  For example if Days = 500, Years = 1, and the new days is equal
	//  to 500 - 365 (normal year).
	//
	days = days - ElapsedYearsToDays(years);
	//
	//  Now test whether the year we are working on (i.e., The year
	//  after the total number of elapsed years) is a leap year or not.
	//
	if(IsLeapYear(years + 1)) {
		//
		//  The current year is a leap year, so figure out what month
		//  it is, and then subtract the number of days preceding the
		//  month from the days to figure out what day of the month it is
		//
		month = LeapYearDayToMonth[days];
		days = days - LeapYearDaysPrecedingMonth[month];
	}
	else {
		//
		//  The current year is a normal year, so figure out the month
		//  and days as described above for the leap year case
		//
		month = NormalYearDayToMonth[days];
		days = days - NormalYearDaysPrecedingMonth[month];
	}
	//
	//  Now we need to compute the elapsed hour, minute, second, milliseconds
	//  from the millisecond variable.  This variable currently contains
	//  the number of milliseconds in our input time variable that did not
	//  fit into a whole day.  To compute the hour, minute, second part
	//  we will actually do the arithmetic backwards computing milliseconds
	//  seconds, minutes, and then hours.  We start by computing the
	//  number of whole seconds left in the day, and then computing
	//  the millisecond remainder.
	//
	seconds = milliseconds / 1000;
	milliseconds = milliseconds % 1000;
	//
	//  Now we compute the number of whole minutes left in the day
	//  and the number of remainder seconds
	//
	minutes = seconds / 60;
	seconds = seconds % 60;
	//
	//  Now compute the number of whole hours left in the day
	//  and the number of remainder minutes
	//
	hours = minutes / 60;
	minutes = minutes % 60;
	//
	//  As our final step we put everything into the time fields output variable
	//
	TimeFields->Y   = (int)(years + 1601);
	TimeFields->M   = (int)(month + 1);
	TimeFields->D   = (int)(days + 1);
	TimeFields->Hr  = (int)hours;
	TimeFields->Mn  = (int)minutes;
	TimeFields->Sc  = (int)seconds;
	TimeFields->MSc = (int)milliseconds;
	TimeFields->MkSc = (int)(milliseconds * 1000);
}
#endif // } 0

int SLAPI SUniTime::Implement_Set(uint8 signature, const void * pData)
{
	int    ok = 0;
	const SUniTime_Inner * p_inner = (const SUniTime_Inner *)pData;
	return ok;
}

static inline uint8 SUniTime_Decode(const uint8 * pD, uint64 * pValue)
{
	*pValue = (pD[7] & 0x80) ? (PTR64(pD)[0] & 0x00ffffffffffffffLL) : PTR64(pD)[0];
	return pD[7];
}

uint8  SLAPI SUniTime::Implement_Get(void * pData) const
{
	uint64 value = 0;
	const  uint8 signature = SUniTime_Decode(D, &value);
	long   day_count = 0;
	SUniTime_Inner * p_inner = (SUniTime_Inner *)pData;
	if(signature & indfScale) {
		switch(signature & ~(indfScale|indfUncertainty)) {
			case indMkSec: break;
			case indMSec: break;
			case indCSec: break;
			case indSec: break;
			case indMin: break;
			case indHr: break;
			case indEpoch: 
				{
					time_t tt = (time_t)value;
					const struct tm * p_temp_tm = gmtime(&tt);
					if(p_temp_tm) {
						p_inner->Y = p_temp_tm->tm_year + 1900;
						p_inner->M = p_temp_tm->tm_mon+1;
						p_inner->D = p_temp_tm->tm_mday;
					}
				}
				break;
			case indDay:
				day_count = (long)value;
				if(DaysSinceChristmasToDate(day_count, &p_inner->Y, &p_inner->M, &p_inner->D)) {
				}
				break;
			case indMon: break;
			case indQuart: break;
			case indSmYr: break;
			case indYr: break;
			case indDYr: break;
			case indSmCent: break;
			case indCent: break;
			case indMillennium: break;
			case indDayBC: break;
			case indMonBC: break;
			case indYrBC: break;
			case indDYrBC: break;
			case indCentBC: break;
			case indMillenniumBC: break;
		}
	}
	else {
		//FileTimeToSystemTime()
	}
	return signature;
}

int FASTCALL SUniTime::Get(LDATE & rD) const
{
	SUniTime_Inner inner;
	uint8 signature = Implement_Get(&inner);
	if(signature != indInvalid) {
		rD.encode(inner.D, inner.M, inner.Y);
		return 1;
	}
	else
		return 0;
}
// } @construction 
//
//
#if SLTEST_RUNNING // {

SLTEST_R(LDATE)
{
	struct Pair {
		char   In[64];
		char   Out[64];
	} pair_list[] = {
		{"1/1/2008",          "1/1/2008"},
		{"1 . 1 - 2008",      "1/1/2008"},
		{" 20-9",             "20/9/getcurdate_.year()"},                    // *[2]
		{"31.12.201",         "31/12/2001"},
		{"12",                "12/getcurdate_.month()/getcurdate_.year()"},  // *[4]
		{"@",                 "7/11/2007"},
		{"@-10",              "28/10/2007"},
		{"@+365",             "6/11/2008"},
		{"11 .9.@",           "11/9/2007"},
		{"11. @-1",           "11/10/getcurdate_().year()"}, // *[9] месяц берется относительно даты rel, год - из текущей системной даты
		{"@-1.@-1.@-10",      "6/10/1997"},
		{"@-3.@+2.@+2",       "4/1/2010"},
		{"^10",               "1/10/2007"},
		{"^4",                "1/11/2007"}
	};

	struct PairDateTime {
		char   In[128];
		char   Out[128];
	} pair_list_dtm[] = {
		{"Sun, 06 Nov 1994 08:49:37 GMT",   "6/11/1994 8:49:37"},
		{"Sunday, 06-Nov-94 08:49:37 GMT",  "6/11/1994 8:49:37"},
		{"Sun Nov  6 08:49:37 1994",        "6/11/1994 8:49:37"},
		{"Nov  6 08:49:37 1994",            "6/11/1994 8:49:37"},
		{"GMT 08:49:37 06-Nov-94",          "6/11/1994 8:49:37"},
		{"Sun Nov 6 94",                    "6/11/1994"},
		{"Sun/Nov/6/94/GMT",                "6/11/1994"},
		{"Sun, 06 Nov 1994 08:49:37 CET",   "6/11/1994 8:49:37"},
		{"Sun, 12 Sep 2004 15:05:58 -0700", "12/09/2004 15:05:58"},
		{"Sat, 11 Sep 2004 21:32:11 +0200", "11/09/2004 21:32:11"}
	};

	uint i;
	_datefmt(20, 9, getcurdate_().year(), DATF_DMY | DATF_CENTURY, pair_list[2].Out);
	_datefmt(12, getcurdate_().month(), getcurdate_().year(), DATF_DMY | DATF_CENTURY, pair_list[4].Out);
	_datefmt(11, 10, getcurdate_().year(), DATF_DMY | DATF_CENTURY, pair_list[9].Out);
	char   cvt_buf[128];
	for(i = 0; i < SIZEOFARRAY(pair_list_dtm); i++) {
		const PairDateTime & r_item = pair_list_dtm[i];
		LDATETIME test_val, cvt_val;
		strtodatetime(r_item.In, &test_val, DATF_DMY, TIMF_HMS);
		//
		// Проверяем конвертацию из даты в строку
		//
		datetimefmt(test_val, DATF_DMY, TIMF_HMS, cvt_buf, sizeof(cvt_buf));
		strtodatetime(cvt_buf, &cvt_val, DATF_DMY, TIMF_HMS);
		SLTEST_CHECK_EQ(test_val, cvt_val);
	}
	{
		const  LDATE rel = encodedate(7, 11, 2007);
		for(i = 0; i < SIZEOFARRAY(pair_list); i++) {
			const Pair & r_item = pair_list[i];
			LDATE test_val, pattern_val, cvt_val;
			strtodate(r_item.In,  DATF_DMY, &test_val);
			//
			// Проверяем конвертацию из даты в строку
			//
			datefmt(&test_val, DATF_DMY, cvt_buf);
			strtodate(cvt_buf,  DATF_DMY, &cvt_val);
			SLTEST_CHECK_EQ(test_val, cvt_val);
			//
			test_val = test_val.getactual(rel);
			strtodate(r_item.Out, DATF_DMY, &pattern_val);
			SLTEST_CHECK_EQ(test_val, pattern_val);
		}
	}
	{
		int y2, m2, d2;
		{
			int y = 2008, m = 12, d = 30;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 2008, m = 12, d = 31;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1600, m = 12, d = 30;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1600, m = 12, d = 31;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 2001, m = 1, d = 1;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 2004, m = 2, d = 29;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1991, m = 3, d = 1;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1879, m = 12, d = 31;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1, m = 1, d = 1;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1582, m = 10, d = 15;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			// /*719527*/719162 days were between March 1, 1 BC and March 1, 1970,
			int dc1 = DateToDaysSinceChristmas(1, 3, 1);
			int dc2 = DateToDaysSinceChristmas(1970, 3, 1);
			SLTEST_CHECK_EQ((long)(dc2-dc1), /*719527*/719162);
			DaysSinceChristmasToDate(dc1, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1);
			SLTEST_CHECK_EQ((long)m2, (long)3);
			SLTEST_CHECK_EQ((long)d2, (long)1);
			DaysSinceChristmasToDate(dc2, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1970);
			SLTEST_CHECK_EQ((long)m2, (long)3);
			SLTEST_CHECK_EQ((long)d2, (long)1);
		}
		{
			int dc1 = DateToDaysSinceChristmas(1996, 12, 31);
			int dc2 = DateToDaysSinceChristmas(1997, 1, 1);
			SLTEST_CHECK_EQ((long)(dc2-dc1), 1);
			DaysSinceChristmasToDate(dc1, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1996);
			SLTEST_CHECK_EQ((long)m2, (long)12);
			SLTEST_CHECK_EQ((long)d2, (long)31);
			DaysSinceChristmasToDate(dc2, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1997);
			SLTEST_CHECK_EQ((long)m2, (long)1);
			SLTEST_CHECK_EQ((long)d2, (long)1);
		}
	}
	{
		//const  LDATE rel = encodedate(7, 11, 2007);
		const  LDATE rel = encodedate(15, 10, 1582);
		for(i = 1; i < 200000; i++) {
			LDATE test = plusdate(rel, i);
			SLTEST_CHECK_EQ((long)diffdate(test, rel), (long)i);
			SLTEST_CHECK_EQ((long)diffdate(rel, test), -(long)i);
		}
	}
	{
		char  txt_mon[128];
		SString mon_buf;
		for(i = 1; i <= 12; i++) {
			/*
				#define MONF_SHORT     0x0001 // Сокращенная форма
				#define MONF_CASENOM   0x0002 // Полная форма (именительный падеж)
				#define MONF_CASEGEN   0x0004 // Полная форма (родительный падеж)
				#define MONF_OEM       0x0080 // OEM-coding
			*/
			{
				long fmt = MONF_SHORT;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASENOM;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASEGEN;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASENOM|MONF_OEM;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
		}
	}
	{
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 1), 0);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 2), 0);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 3), 0);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 4), 0);

		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 17), encodetime(10, 1, 10, 17), 1), 2);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 7, 10, 17), encodetime(12, 1, 10, 17), 2), 6);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 21, 17), encodetime(12, 1, 10, 17), 3), 11);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 27), encodetime(12, 1, 10, 17), 4), 100);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 17), encodetime(12, 1, 10, 27), 4), -100);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 10, 27), 4), 1000);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 27), encodetime(12, 1, 11, 27), 4), -1000);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 2, 11,  7), encodetime(12, 1, 10, 17), 4), 60900);
	}
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING

