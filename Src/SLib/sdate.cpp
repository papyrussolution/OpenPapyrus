// SDATE.CPP
// Copyright (C) Sobolev A. 1994, 1995, 1996-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8 // @v10.4.5
//
#include <slib-internal.h>
#pragma hdrstop

// @unesed #define TICKSPERMIN        600000000
#define TICKSPERSEC        10000000
// @unesed #define TICKSPERMSEC       10000
//#define SECSPERDAY         86400
// @unesed #define _YEAR_SEC          (365 * SlConst::SecsPerDay) // secs in a year
//#define SECSPERHOUR        3600
// @unesed #define SECSPERMIN         60
// @unesed #define MINSPERHOUR        60
// @unesed #define HOURSPERDAY        24
// @unesed #define EPOCHWEEKDAY       1
// @unesed #define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
// @unesed #define MONSPERYEAR        12
#define WEEKDAY_OF_1601     1 // This is the week day that January 1st, 1601 fell on (a Monday)
#define WEEKDAY_OF_1970     4 // 01-01-70 was a Thursday
//#define EPOCH_BIAS         116444736000000000i64 // Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
//#if defined(__GNUC__)
	//#define TICKSTO1970         0x019db1ded53e8000LL
	//#define TICKSTO1980         0x01a8e79fe1d58000LL
//#else
	//#define TICKSTO1970         0x019db1ded53e8000i64
	//#define TICKSTO1980         0x01a8e79fe1d58000i64
//#endif

#define _IS_LEAP_EPOCH_YEAR(y) ((((y) % 4 == 0) && ((y) % 100 != 0)) || (((y) + 1900) % 400 == 0))
#define _LEAP_YEAR_ADJUST  17 // Leap years 1900 - 1970
#define _MAX__TIME64_T     0x100000000000i64 // number of seconds from 00:00:00, 01/01/1970 UTC to 23:59:59. 12/31/2999 UTC
//
// Number of leap years from 1970 up to, but not including, the specified year
// (expressed as the number of years since 1900).
//
#define _ELAPSED_LEAP_YEARS(y) ((((y)-1)/4) - (((y)-1)/100) + (((y)+299)/400) - _LEAP_YEAR_ADJUST)
//
//  ULONG
//  NumberOfLeapYears (IN ULONG ElapsedYears);
//
//  The number of leap years is simply the number of years divided by 4
//  minus years divided by 100 plus years divided by 400.  This says
//  that every four years is a leap year except centuries, and the
//  exception to the exception is the quadricenturies
//
#define NumberOfLeapYears(y) (((y)/4) - ((y)/100) + ((y)/400))
//
//  ULONG ElapsedYearsToDays (IN ULONG ElapsedYears);
//
//  The number of days contained in elapsed years is simply the number
//  of years times 365 (because every year has at least 365 days) plus
//  the number of leap years there are (i.e., the number of 366 days years)
//
#define ElapsedYearsToDays(y) (((y) * 365) + NumberOfLeapYears(y))
#define ConvertMillisecondsTo100ns(MILLISECONDS) ((MILLISECONDS)*10000LL)
#define ConvertMicrosecondsTo100ns(MILLISECONDS) ((MILLISECONDS)*10LL)
#define Convert100nsToMilliseconds(v) ((v) / 10000LL)
#define Convert100nsToMicroseconds(v) ((v) / 10LL)
#define ConvertMillisecondsToDays(v)  ((v) / (SlConst::SecsPerDay * 1000))
#define ConvertMicrosecondsToDays(v)  ((v) / (static_cast<uint64>(SlConst::SecsPerDay) * 1000000ULL))
#define ConvertDaysToMilliseconds(DAYS) ((DAYS) * (SlConst::SecsPerDay * 1000))
#define ConvertDaysToMicroseconds(DAYS) ((DAYS) * (SlConst::SecsPerDay * 1000000LL))

#ifndef _WIN32_WCE
	#define USE_DF_CLARION
#endif

const LDATE ZERODATE = {0L};
const LDATE MAXDATE  = {MAXLONG};
const LDATE MAXDATEVALID = {0x0bb80101}; // 01/01/3000
const LTIME ZEROTIME = {0L};
const LTIME MAXTIME  = {MAXLONG};
const LTIME MAXDAYTIME = { 0x173B3B63U }; // 23:59:59.99
const LTIME MAXDAYTIMESEC = { 0x173B3B00U }; // 23:59:59.00
const LDATETIME ZERODATETIME = {{0}, {0}};
const LDATETIME MAXDATETIME = {{MAXLONG}, {MAXLONG}};

const uchar daysPerMonth[NUM_MONTHES] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int FASTCALL _decode_date_fmt(int style, int * pDiv)
{
	static const struct { char div, ord; } p_fmt_params[] = {
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
	int    ord = 1;
	style &= 0x000f;
	if(style > 0 && style <= SIZEOFARRAY(p_fmt_params)) {
		*pDiv = p_fmt_params[style-1].div;
		ord = p_fmt_params[style-1].ord;
	}
	else {
		*pDiv = 47;
	}
	return ord;
}

bool FASTCALL IsLeapYear_Gregorian(int y) { return ((y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0)); }
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

/*static*/int SUniDate_Internal::DateToDaysSinceChristmas(int y, uint m, uint d)
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

uint FASTCALL dayspermonth(uint month, uint year)
{
	assert(checkirange(month, 1U, 12U));
	assert(year > 0);
	uint   dpm = 0;
	if(month >= 1U && month <= 12U) {
		dpm = daysPerMonth[month-1];
		if(month == 2 && IsLeapYear_Gregorian(year))
			dpm++;
	}
	return dpm;
}

static char * FASTCALL ExtractVarPart(const char * word, char * buf)
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

static char * extractFormFromVarPart(const char * vp, int n /*[1..]*/, char * buf)
{
	const char * c = vp;
	const char * p = vp;
	char  * b = buf;
	int i = 0;
	while(i < n && ((c = p), p = sstrchr(p, '|')) != 0) {
		i++;
		p++;
	}
	while(*c && !(p && *c == '|'))
		*b++ = *c++;
	*b = 0;
	return b;
}

static char * selectVarPart(const char * word, int n, char * pBuf)
{
	char   vp[256];
	char * e = ExtractVarPart(word, vp);
	extractFormFromVarPart(vp, n, pBuf);
	return e;
}

static char * GetWordForm(const char * pattern, long fmt, char * pBuf)
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
//
//
//
static const char * FASTCALL _ExtractVarPart(const char * word, char * buf)
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

static void _extractFormFromVarPart(const char * vp, int n /*[1..]*/, SString & rBuf)
{
	const char * c = vp;
	const char * p = vp;
	for(int i = 0; i < n && ((c = p), p = sstrchr(p, '|')) != 0;) {
		i++;
		p++;
	}
	while(*c && !(p && *c == '|')) {
		rBuf.CatChar(*c++);
	}
}

static const char * _selectVarPart(const char * word, int n, SString & rBuf)
{
	char   vp[256];
	const  char * e = _ExtractVarPart(word, vp);
	_extractFormFromVarPart(vp, n, rBuf);
	return e;
}

static SString & _getWordForm(const char * pPattern, long fmt, SString & rBuf)
{
	rBuf.Z();
	if(fmt & MONF_SHORT) {
		SStringU temp_buf_u;
		temp_buf_u.CopyFromUtf8(pPattern, sstrlen(pPattern));
		temp_buf_u.Trim(3).CopyToUtf8(rBuf, 1);
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

SString & STDCALL SGetMonthText(int mon, long fmt, SString & rBuf)
{
	static const char * p_month_names[NUM_MONTHES] = {
		"Январ[ь|я]", "Феврал[ь|я]", "Март[|а]", "Апрел[ь|я]", "Ма[й|я]", "Июн[ь|я]",
		"Июл[ь|я]", "Август[|а]", "Сентябр[ь|я]", "Октябр[ь|я]", "Ноябр[ь|я]", "Декабр[ь|я]"
	};
	//"Январ[ь|я],Феврал[ь|я],Март[|а],Апрел[ь|я],Ма[й|я],Июн[ь|я],Июл[ь|я],Август[|а],Сентябр[ь|я],Октябр[ь|я],Ноябр[ь|я],Декабр[ь|я]"
	rBuf.Z();
	if(mon >= 1 && mon <= 12) {
		_getWordForm(p_month_names[mon-1], fmt, rBuf);
		if(mon == 5 && fmt & MONF_SHORT) {
			SStringU temp_buf_u;
			temp_buf_u.CopyFromUtf8(rBuf, rBuf.Len());
			temp_buf_u.Trim(2).CopyToUtf8(rBuf, 1);
			rBuf.Cat("й");
			//rBuf.Trim(2).Cat("й");
		}
		if(fmt & MONF_OEM) {
			// @v10.4.5 rBuf.ToOem();
			rBuf.Transf(CTRANSF_UTF8_TO_INNER); // @v10.4.5
		}
		else {
			rBuf.Transf(CTRANSF_UTF8_TO_OUTER); // @v10.4.5
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
	SString pattern(pText);
	pattern.Transf(CTRANSF_INNER_TO_UTF8); // @v10.4.5
	StringSet ss(';', P_WeekDays);
	int    dow = 0;
	for(uint i = 0; ss.get(&i, temp_buf);) {
		++dow;
		StringSet ss2(',', temp_buf);
		for(uint j = 0; ss2.get(&j, temp_buf);)
			if(temp_buf.CmpNC(pattern) == 0)
				return dow;
	}
	return 0;
}

int STDCALL GetDayOfWeekText(int options, int dayOfWeek /* 1..7 */, SString & rBuf)
{
	rBuf.Z();
	if(dayOfWeek >= 1 && dayOfWeek <= 7 && options >= 1 && options <= 4) {
		rBuf.GetSubFrom(P_WeekDays, ';', dayOfWeek-1);
		rBuf.GetSubFrom(rBuf, ',', options-1);
		rBuf.Transf(CTRANSF_UTF8_TO_OUTER); // @v10.4.5
		return 1;
	}
	else
		return (SLibError = SLERR_INVRANGE, 0);
}

static const int16 __dpm[11] = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334/*, 365*/ };

static inline int FASTCALL getDays360(int mon, bool leap) { return (mon > 1 && mon <= 12) ? ((mon-1) * 30) : 0; }
static inline int FASTCALL getDays(int mon, bool leap) { return (mon > 1 && mon <= 12) ? (((leap && mon > 2) ? __dpm[mon-2]+1 : __dpm[mon-2])) : 0; }

static void _ltodate(long nd, void * dt, int format)
{
	SUniDate_Internal udi;
	udi.SetDaysSinceChristmas(nd);
	_encodedate(udi.D, udi.M, udi.Y, dt, format);
}

static void _ltodate360(long nd, void * dt, int format)
{
	ldiv_t xd = ldiv(nd, 360L);
	int    y = (int)xd.quot + 1;
	int    d = (int)xd.rem;
	int    m = getMon(&d, IsLeapYear_Gregorian(y));
	_encodedate(d, m, y, dt, format);
}

static long FASTCALL _datetol(const void * dt, int format)
{
	int    d, m, y;
	_decodedate(&d, &m, &y, dt, format);
	return y ? SUniDate_Internal::DateToDaysSinceChristmas(y, m, d) : 0;
}

static long FASTCALL _datetol360(const void * dt, int format)
{
	int    d, m, y;
	_decodedate(&d, &m, &y, dt, format);
	return (360L * y + getDays360(m, false) + d);
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

static long ClarionDateToLong(const /*SDosDate*/SUniDate_Internal * pDt)
{
	/*SDosDate*/SUniDate_Internal doff;
	doff.Y = pDt->Y - firstYear;
	doff.D = pDt->D - firstDay;
	const bool leap = IsLeapYear_Gregorian(pDt->Y);
	const long loff = firstOff + doff.Y * 365L + (doff.Y >> 2) + doff.D + getDays(pDt->M, leap) - getDays(firstMon, false) - 1;
	return loff;
}

static void ClarionLongToDate(long off, /*SDosDate*/SUniDate_Internal * pDt)
{
	long dev  = off - ((off / 365) >> 2) - 2;
	int  rest = (int)(dev % 365);
	pDt->Y = (int)(dev / 365) + firstYear;
	if(rest == 0)
		pDt->Y--;
	pDt->M = getMon(&rest, IsLeapYear_Gregorian(pDt->Y));
	pDt->D = rest;
}

#endif /* USE_DF_CLARION */

/* @v10.3.0 static void formatNotSupported(const char * pFormatName)
{
	slfprintf_stderr("%s date format not supported", pFormatName);
	exit(-1);
}*/

void STDCALL _encodedate(int day, int mon, int year, void * pBuf, int format)
{
	char   tmp[64];
	switch(format) {
		case DF_BTRIEVE:
			static_cast<LDATE *>(pBuf)->encode(day, mon, year);
			break;
		case DF_DOS:
			static_cast<SDosDate *>(pBuf)->da_day  = day;
			static_cast<SDosDate *>(pBuf)->da_mon  = mon;
			static_cast<SDosDate *>(pBuf)->da_year = year;
			break;
		case DF_FAT:
			*static_cast<uint *>(pBuf) = (((year - 1980) << 9) | (mon << 5) | day);
			break;
		case DF_XBASE:
			sprintf(tmp, "%04d%02d%02d", year, mon, day);
			memcpy(pBuf, tmp, 8);
			break;
		// @v10.2.8 case DF_PARADOX: formatNotSupported("Paradox"); break;
		case DF_CLARION:
			{
#ifdef USE_DF_CLARION
				SUniDate_Internal dt;
				dt.D  = day;
				dt.M  = mon;
				dt.Y = year;
				*static_cast<long *>(pBuf) = ClarionDateToLong(&dt);
#else
				*static_cast<long *>(pBuf) = 0;
				const int clarion_date_format_not_supported = 0
				assert(clarion_date_format_not_supported);
				//formatNotSupported("Clarion");
#endif
			}
			break;
		default:
			//printf("Not supplied date format\n");
			//abort();
			break;
	}
}

void STDCALL _decodedate(int * day, int * mon, int * year, const void * pBuf, int format)
{
	char   tmp[64];
	switch(format) {
		case DF_BTRIEVE: static_cast<const LDATE *>(pBuf)->decode(day, mon, year); break;
#ifndef _WIN32_WCE
		case DF_DOS:
			*day  = static_cast<const SDosDate *>(pBuf)->da_day;
			*mon  = static_cast<const SDosDate *>(pBuf)->da_mon;
			*year = static_cast<const SDosDate *>(pBuf)->da_year;
			break;
#endif
		case DF_FAT:
			*year = (PTR32C(pBuf)[0] >> 9) + 1980;
			*mon  = (PTR32C(pBuf)[0] >> 5) & 0x000f;
			*day  = PTR32C(pBuf)[0] & 0x001f;
			break;
		case DF_XBASE:
			PTR8(memcpy(tmp, pBuf, 8))[8] = '\0';
			sscanf(tmp, "%4d%2d%2d", year, mon, day);
			break;
		// @v10.2.8 case DF_PARADOX: formatNotSupported("Paradox"); break;
		case DF_CLARION:
#ifdef USE_DF_CLARION
			{
				SUniDate_Internal dt;
				ClarionLongToDate(*static_cast<const long *>(pBuf), &dt);
				*day  = dt.D;
				*mon  = dt.M;
				*year = dt.Y;
			}
#else
			*day  = 0;
			*mon  = 0;
			*year = 0;
			const int clarion_date_format_not_supported = 0
			assert(clarion_date_format_not_supported);
			//formatNotSupported("Clarion");
#endif
			break;
		default:
			//printf("Not supplied date format\n");
			//abort();
			break;
	}
}

long STDCALL _diffdate(const void * dest, const void * src, int format, int _360)
{
	if(!_360)
		return (_datetol(dest, format) - _datetol(src, format));
	else
		return (_datetol360(dest, format) - _datetol360(src, format));
}

void STDCALL _plusdate(void * dt, int nd, int fmt, int _360)
{
	if(!_360)
		_ltodate(_datetol(dt, fmt) + nd, dt, fmt);
	else
		_ltodate360(_datetol360(dt, fmt) + nd, dt, fmt);
}

void _plusperiod(void * dest, int prd, int numperiods, int format, int _360)
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
				d = (m == 2 && d >= 29 && IsLeapYear_Gregorian(y)) ? 29 : daysPerMonth[m-1];
			_encodedate(d, m, y, dest, format);
		}
	}
}

int FASTCALL _dayofweek(const void * pDate, int format)
{
	char   beg[32];
	_encodedate(1, 1, 1970, beg, format); // 1/1/1970 - Thu (4)
	return (int)((4 + _diffdate(pDate, beg, format, 0) % 7) % 7);
}

void STDCALL encodedate(int day, int mon, int year, void * pBinDate) { _encodedate(day, mon, year, pBinDate, BinDateFmt); }
void STDCALL decodedate(int * pDay, int * pMon, int * pYear, const void * pBinDate) { _decodedate(pDay, pMon, pYear, pBinDate, BinDateFmt); }
long STDCALL diffdate(const void * pDest, const void * pSrc, int _360) { return _diffdate(pDest, pSrc, BinDateFmt, _360); }
void STDCALL plusdate(void * pDest, int numdays, int _360) { _plusdate(pDest, numdays, BinDateFmt, _360); }
void STDCALL plusperiod(void * pDest, int period, int numperiods, int _360) { _plusperiod(pDest, period, numperiods, BinDateFmt, _360); }

int FASTCALL dayofweek(const void * pDate, int sundayIsSeventh)
{
	const int dow = _dayofweek(pDate, BinDateFmt);
	return !sundayIsSeventh ? dow : (dow ? dow : 7);
}

LDATE STDCALL encodedate(int day, int month, int year)
{
	LDATE dt;
	encodedate(day, month, year, &dt);
	return dt;
}

LDATE FASTCALL plusdate(LDATE d, long a)
{
	if(a)
		plusdate(&d, (int)a, 0);
	return d;
}

long FASTCALL diffdate(LDATE d, LDATE s) { return (d != s) ? diffdate(&d, &s, 0) : 0; }
//
//
//
int STDCALL _checkdate(int day, int mon, int year)
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
			if(day > (int)dayspermonth(mon, 2000)) // any leap year
				err = SLERR_INVDAY;
		}
		else if(oneof2(mon, ANY_MONITEM_VALUE, ANY_DATE_VALUE)) {
			if(day > 31)
				err = SLERR_INVDAY;
		}
		else if(day > (int)dayspermonth(mon, year))
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
		return BIN(zeroIsOk);
}

int FASTCALL checkdate(LDATE dt)
{
	if(dt) {
		int    d, m, y;
		_decodedate(&d, &m, &y, &dt, DF_BTRIEVE);
		return _checkdate(d, m, y);
	}
	else
		return 0;
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

FORCEINLINE void implement_decodetime(int * h, int * m, int * s, int * ts, const void * tm)
{
	ASSIGN_PTR(h,  static_cast<const char *>(tm)[3]);
	ASSIGN_PTR(m,  static_cast<const char *>(tm)[2]);
	ASSIGN_PTR(s,  static_cast<const char *>(tm)[1]);
	ASSIGN_PTR(ts, static_cast<const char *>(tm)[0]);
}

int FASTCALL checktime(LTIME tm)
{
	int    err = SLERR_SUCCESS;
	int    h, m, s, ts;
	implement_decodetime(&h, &m, &s, &ts, &tm);
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

LTIME LTIME::encode(int h, int m, int s, int ms)
{
	*this = encodetime(h, m, s, ms / 10);
	return *this;
}

#endif // }

long FASTCALL LTIME::settotalsec(long s)
{
	long   inc_dt = s / SlConst::SecsPerDay; // @v11.2.11 @fix (3600 * 60 * 60)-->(3600 * 24)
	encodetime(s / 3600, (s % 3600) / 60, s % 60, 0, this);
	return inc_dt;
}

LTIME & FASTCALL LTIME::addhs(long n)
{
	int    h, m, s, hs;
	implement_decodetime(&h, &m, &s, &hs, this);
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

void STDCALL encodetime(int h, int m, int s, int ts, void * tm)
{
	__ENCODEDATE(h, m, s, ts, tm);
}

LTIME STDCALL encodetime(int h, int m, int s, int ts)
{
	LTIME  tm;
	__ENCODEDATE(h, m, s, ts, &tm.v);
	return tm;
}

void STDCALL decodetime(int * h, int * m, int * s, int * ts, const void * tm)
{
	implement_decodetime(h, m, s, ts, tm);
}

long STDCALL DiffTime(LTIME t1, LTIME t2, int dim)
{
	int    h1, m1, s1, ts1;
	int    h2, m2, s2, ts2;
	implement_decodetime(&h1, &m1, &s1, &ts1, &t1);
	implement_decodetime(&h2, &m2, &s2, &ts2, &t2);
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

long STDCALL diffdatetime(LDATE d1, LTIME t1, LDATE d2, LTIME t2, int dim, long * pDiffDays)
{
	long   dd = _diffdate(&d1, &d2, DF_BTRIEVE, 0);
	long   dt = DiffTime(t1, t2, 4/*dim*/);
	if(dd != 0) {
		if(dd > 0 && dt < 0) {
			dd--;
			dt += (SlConst::SecsPerDay * 1000L);
		}
		else if(dd < 0 && dt > 0) {
			dd++;
			dt -= (SlConst::SecsPerDay * 1000L);
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

long STDCALL diffdatetime(const LDATETIME & dtm1, const LDATETIME & dtm2, int dim, long * pDiffDays)
{
	return diffdatetime(dtm1.d, dtm1.t, dtm2.d, dtm2.t, dim, pDiffDays);
}

long STDCALL diffdatetimesec(LDATE d1, LTIME t1, LDATE d2, LTIME t2)
{
	long dif_days = 0;
	long ds = diffdatetime(d1, t1, d2, t2, 3, &dif_days);
	return (ds + dif_days * SlConst::SecsPerDay);
}

long FASTCALL diffdatetimesec(const LDATETIME & dtm1, const LDATETIME & dtm2)
{
	long   dif_days = 0;
	long   ds = diffdatetime(dtm1, dtm2, 3, &dif_days);
	return (ds + dif_days * SlConst::SecsPerDay);
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
int setcurdatetime(LDATETIME dtm)
{
	SYSTEMTIME st;
	if(SetLocalTime(&dtm.Get(st))) {
		::SendMessageW(HWND_TOPMOST, WM_TIMECHANGE, 0, 0);
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
		// @v11.3.12 pDt->encode(st.wDay, st.wMonth, st.wYear); // @v10.0.02
		pDt->EncodeRegular(st.wDay, st.wMonth, st.wYear); // @v11.3.12
	}
	if(pTm) {
		char * _tm = reinterpret_cast<char *>(pTm);
		_tm[0] = static_cast<char>(st.wMilliseconds/10);
		_tm[1] = static_cast<char>(st.wSecond);
		_tm[2] = static_cast<char>(st.wMinute);
		_tm[3] = static_cast<char>(st.wHour);
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

LDATE getcurdate_()
{
	LDATE dt;
	getcurdatetime(&dt, 0);
	return dt;
}

LTIME getcurtime_()
{
	LTIME tm;
	getcurdatetime(0, &tm);
	return tm;
}

int FASTCALL getcurdate(LDATE * dt) { return getcurdatetime(dt, 0); }
int FASTCALL getcurtime(LTIME * tm) { return getcurdatetime(0, tm); }
int FASTCALL getcurdatetime(LDATETIME * pTm) { return getcurdatetime(&pTm->d, &pTm->t); }

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

/*static*/int FASTCALL WorkDate::ShrinkDate(LDATE dt)
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

/*static*/LDATE FASTCALL WorkDate::ExpandDate(int16 sdt)
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
bool   FASTCALL WorkDate::IsEq(WorkDate wd) const { return (wd.V == V); }

bool   FASTCALL WorkDate::IsEq(LDATE dt) const
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
	return false;
}
//
// LDATETIME
//
static LDATETIME FarMoment;
//
// Descr: Специализированный класс для автоматической инициализации FarMoment
//
static const struct InitFarMoment { InitFarMoment() { FarMoment.d.v = MAXLONG; FarMoment.t.v = 0; } } IFM;

bool LDATETIME::IsFar() const { return (cmp(*this, FarMoment) == 0); }

LDATETIME & LDATETIME::SetFar()
{
	*this = FarMoment;
	return *this;
}

LDATETIME & LDATETIME::SetMax() // max-moment: (LDATE)d = MAXLONG, (LTIME)t = MAXLONG
{
	d = MAXDATE;
	t = MAXTIME;
	return *this;
}

LDATETIME & FASTCALL LDATETIME::SetNs100(int64 ns100Tm) // @v11.8.11
{
	SUniTime_Internal uti;
	uti.SetTime100ns(ns100Tm);
	d.encode(uti.D, uti.M, uti.Y);
	t.encode(uti.Hr, uti.Mn, uti.Sc, uti.MSc);
	return *this;
}

LDATETIME & FASTCALL LDATETIME::SetTimeT(time_t _tm)
{
	const struct tm * p_temp_tm = localtime(&_tm); // @v10.0.08 gmtime-->localtime
	if(p_temp_tm) {
		d.encode(p_temp_tm->tm_mday, p_temp_tm->tm_mon+1, p_temp_tm->tm_year + 1900);
		t = encodetime(p_temp_tm->tm_hour, p_temp_tm->tm_min, p_temp_tm->tm_sec, 0);
	}
	else
		Z();
	return *this;
}

time_t LDATETIME::GetTimeT() const
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
		return (sizeof(time_t) == 8) ? (time_t)_mktime64(&_t) : mktime(&_t);
	}
}

LDATETIME & LDATETIME::Set(LDATE _d, LTIME _t)
{
	d = _d;
	t = _t;
	return *this;
}

int  LDATETIME::Set(const char * pText, long datf, long timf) { return strtodatetime(pText, this, datf, timf); }
bool LDATETIME::operator !() const { return (d == ZERODATE && t == ZEROTIME); }
bool FASTCALL LDATETIME::operator == (const LDATETIME & s) const { return (d == s.d && t == s.t); }
bool FASTCALL LDATETIME::operator != (const LDATETIME & s) const { return (d != s.d || t != s.t); }

#ifndef _WIN32_WCE // {

/*LDATETIME::LDATETIME() : d(ZERODATE), t(ZEROTIME)
{
}*/

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
		Z();
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

LDATETIME & LDATETIME::Z()
{
	d = ZERODATE;
	t = ZEROTIME;
	return *this;
}

long FASTCALL LDATETIME::settotalsec(long s)
{
	long   inc_dt = s / SlConst::SecsPerDay;
	if(inc_dt)
		d = plusdate(d, inc_dt);
	t.settotalsec(s % SlConst::SecsPerDay);
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

LDATETIME FASTCALL LDATETIME::plussec(long nsec) const
{
	LDATETIME result = *this;
	return result.addsec(nsec);
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

OleDate LDATE::GetOleDate() const
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

time_t LDATE::GetTimeT() const
{
	if(!checkdate(*this))
		return 0;
	else {
		struct tm _t;
		_t.tm_year = year()-1900;
		_t.tm_mon = month()-1; // @v10.0.02 @fix (-1)
		_t.tm_mday = day();
		_t.tm_hour = 12; // @v10.0.03 дабы смещение временного пояса не меняло дату, устанавливаем время полдня.
		_t.tm_min = 0;
		_t.tm_sec = 0;
		return (sizeof(time_t) == 8) ? (time_t)_mktime64(&_t) : mktime(&_t);
	}
}

int LDATE::dayspermonth() const
{
	return (int)::dayspermonth(month(), year());
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
	if(checkirange(d, 1U, 31U) || d == ANY_DAYITEM_VALUE) {
		(v &= ~0xff) |= d;
		return 1;
	}
	else
		return 0;
}

int FASTCALL LDATE::setmonth(uint m)
{
	if(checkirange(m, 1U, 12U) || m == ANY_MONITEM_VALUE) {
		(v &= ~0x0000ff00) |= (m << 8);
		return 1;
	}
	else
		return 0;
}

int FASTCALL LDATE::setyear(uint y)
{
	if(checkirange(y, 1U, 6000U) || y == ANY_YEARITEM_VALUE) {
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
	else if(checkdate(*this))
		return cNormal;
	else
		return cInvalid;
}

int LDATE::encode(int d, int m, int y)
{
	int16  shift; // @v10.4.12 @fix int-->int16
	int    x;
	int    d_ = 0, m_ = 0, y_ = 0;
	if(d == ANY_DATE_VALUE) {
		d_ = ANY_DAYITEM_VALUE;
	}
	else if(d & REL_DATE_MASK) {
		shift = static_cast<int16>(LoWord(d));
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
		shift = static_cast<int16>(LoWord(d));
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
		shift = static_cast<int16>(LoWord(m));
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
		shift = static_cast<int16>(LoWord(y));
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
	v = static_cast<ulong>(MakeLong((m_ << 8) | d_, y_));
	return 1;
}

int LDATE::decode(int * pD, int * pM, int * pY) const
{
	int    ok = 1;
	if(*this == MAXDATE) {
		ok = MAXDATEVALID.decode(pD, pM, pY);
	}
	else {
		int    d, m;
		int    y = year();
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

int LDATE::hasanycomponent() const
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

LDATE LDATE::Helper_GetActual(LDATE rel, LDATE cmp) const
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
				d = (m == 2 && d >= 29 && IsLeapYear_Gregorian(y)) ? 29 : daysPerMonth[m-1];
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

LDATE LDATE::getactualcmp(LDATE rel, LDATE cmp) const { return Helper_GetActual(rel, cmp); }
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

bool CALDATE::IsDate(LDATE dt) const
{
	int    d, m, y;
	decodedate(&d, &m, &y, this);
	if(d >= 1 && d <= 7 && !m && !y) // day of week
		return (d == dayofweek(this, 1));
	else if(d && m && !y) // calendar date
		return (d == dt.day() && m == dt.month());
	else // simple date
		return (dt == *this);
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
	return checkdate(temp_date) ? ((*this = encodedate(day, mon, 0).v), 1) : 0;
}
//
//
//
STimeChunk::STimeChunk() : Start(ZERODATETIME), Finish(ZERODATETIME)
{
}

STimeChunk::STimeChunk(const LDATETIME & rStart, const LDATETIME & rFinish)
{
	Init(rStart, rFinish);
}

STimeChunk & STimeChunk::Z()
{
	Start = ZERODATETIME;
	Finish = ZERODATETIME;
	return *this;
}

void STimeChunk::Init(const LDATETIME & start, const LDATETIME & finish)
{
	Start = start;
	if(finish.d)
		Finish = finish;
	else
		Finish.SetFar();
}

void STimeChunk::Init(const LDATETIME & start, long cont)
{
	Start = start;
	Finish = plusdatetime(start, cont, 3);
	if(!start.d)
		Finish.d = ZERODATE;
}

bool FASTCALL STimeChunk::operator == (const STimeChunk & rTest) const { return (::cmp(Start, rTest.Start) == 0 && ::cmp(Finish, rTest.Finish) == 0); }
bool FASTCALL STimeChunk::operator != (const STimeChunk & rTest) const { return (::cmp(Start, rTest.Start) != 0 || ::cmp(Finish, rTest.Finish) != 0); }
bool FASTCALL STimeChunk::Has(const LDATETIME & rTm) const { return (::cmp(rTm, Start) >= 0 && ::cmp(rTm, Finish) <= 0); }
long STimeChunk::GetDurationDays() const { return (Start.d && Finish.d && !Finish.IsFar()) ? (diffdate(Finish.d, Start.d)+1) : -1; }
long STimeChunk::GetDuration() const     { return (Start.d && Finish.d && !Finish.IsFar()) ? diffdatetimesec(Finish, Start) : -1; }

int FASTCALL STimeChunk::cmp(const STimeChunk & rTest) const
{
	int    r = ::cmp(this->Start, rTest.Start);
	return NZOR(r, ::cmp(this->Finish, rTest.Finish));
}

bool FASTCALL STimeChunk::ContainsIn(const STimeChunk & rDur) const
{
	STimeChunk result;
	return (Intersect(rDur, &result) && *this == result);
}

bool STimeChunk::GetUnionIfIntersected(const STimeChunk & rOther, STimeChunk * pResult) const
{
	bool   ok = false;
	STimeChunk is;
	if(Intersect(rOther, &is)) {
		STimeChunk result;
		int sc = ::cmp(Start, rOther.Start);
		int fc = ::cmp(Finish, rOther.Finish);
		if(sc < 0)
			result.Start = Start;
		else
			result.Start = rOther.Start;
		if(fc > 0)
			result.Finish = Finish;
		else
			result.Finish = rOther.Finish;
		ASSIGN_PTR(pResult, result);
		ok = true;
	}
	return ok;
}

bool FASTCALL STimeChunk::Intersect(const STimeChunk & test, STimeChunk * pResult) const
{
	bool   is = false;
	LDATETIME st, fn;
	if(::cmp(Start, test.Finish) > 0 || ::cmp(Finish, test.Start) < 0) {
		st.Z();
		fn.Z();
	}
	else {
		st = (::cmp(Start, test.Start) > 0) ? Start : test.Start;
		fn = (::cmp(Finish, test.Finish) < 0) ? Finish : test.Finish;
		is = true;
	}
	CALLPTRMEMB(pResult, Init(st, fn));
	return is;
}

SString & STimeChunk::ToStr(SString & rBuf, long fmt) const
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

int64 STimeChunk::GetDurationMs() const
{
	if(Start.d && Finish.d && !Finish.IsFar()) {
		long days = 0;
		long diff = diffdatetime(Finish, Start, 4, &days);
		return ((int64)(diff * 10) + ((int64)days * 1000 * SlConst::SecsPerDay));
	}
	else
		return -1;
}

IMPL_CMPFUNC(STimeChunk, i1, i2)
{
	return pExtraData ? static_cast<const STimeChunk *>(i2)->cmp(*static_cast<const STimeChunk *>(i1)) :
		static_cast<const STimeChunk *>(i1)->cmp(*static_cast<const STimeChunk *>(i2));
}
//
//
//
/*static*/int FASTCALL DateRepeating::IsValidPrd(int prd)
	{ return oneof6(prd, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL); }
static inline int FASTCALL DateRepeating_IsEqual(const DateRepeating & rS1, const DateRepeating & rS2)
	{ return (rS1.Prd == rS2.Prd && rS1.RepeatKind == rS2.RepeatKind && *PTR32C(&rS1.Dtl) == *PTR32C(&rS2.Dtl)); }
int FASTCALL DateRepeating::operator == (const DateRepeating & rS) const
	{ return DateRepeating_IsEqual(*this, rS); }
int FASTCALL DateRepeating::operator != (const DateRepeating & rS) const
	{ return !DateRepeating_IsEqual(*this, rS); }

int DateRepeating::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	const size_t offs = offsetof(DateRepeating, Prd);
	return pSCtx->SerializeBlock(dir, sizeof(*this)-offs, PTR8(this)+offs, rBuf, 0);
}

int DateRepeating::GetMonthlyPeriod(int * pMonthCount, int * pMonthNo) const
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

int DateRepeating::SetMonthly(int monthCount, int monthNo, int dayOfMonth)
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

int DateRepeating::SetMonthly(int monthCount, int monthNo, int weekNo, int dayOfWeek)
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

int DateRepeating::DayOfWeekNo(LDATE dt, int weekNo, int dayOfWeek, LDATE * pResult) const
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

int DateRepeating::Init(int prd, int kind, LDATE dt)
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

int DateRepeating::Next_(LDATE startDate, LDATE * pNextDate) const
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
int DateRepeating::Format(int fmt, SString & rBuf) const
{
	rBuf.Z();
	switch(Prd) {
		case PRD_DAY:
			rBuf.Cat("DAILY");
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

long DateRepeating::DtlToLong()
{
	switch(Prd) {
		case PRD_DAY: return *reinterpret_cast<const long *>(&Dtl.D);
		case PRD_WEEK: return *reinterpret_cast<const long *>(&Dtl.W);
		case PRD_MONTH: return (RepeatKind == 1) ? *reinterpret_cast<const long *>(&Dtl.ME) : *reinterpret_cast<const long *>(&Dtl.MY);
		case PRD_ANNUAL: return (RepeatKind == 1) ? *reinterpret_cast<const long *>(&Dtl.AE) : *reinterpret_cast<const long *>(&Dtl.AY);
		case PRD_REPEATAFTERPRD: return *reinterpret_cast<const long *>(&Dtl.RA);
	}
	return 0;
}

int DateRepeating::LongToDtl(long v)
{
	if(Prd == PRD_DAY)
		Dtl.D = *reinterpret_cast<const RepeatDay *>(&v);
	else if(Prd == PRD_WEEK)
		Dtl.W = *reinterpret_cast<const RepeatWeek *>(&v);
	if(Prd == PRD_MONTH) {
		if(RepeatKind == 1)
			Dtl.ME = *reinterpret_cast<const RepeatMonthDate *>(&v);
		else
			Dtl.MY = *reinterpret_cast<const RepeatMonthDay *>(&v);
	}
	if(Prd == PRD_ANNUAL) {
		if(RepeatKind == 1)
			Dtl.AE = *reinterpret_cast<const RepeatAnnDate *>(&v);
		else
			Dtl.AY = *reinterpret_cast<const RepeatAnnDay *>(&v);
	}
	else if(Prd == PRD_REPEATAFTERPRD)
		Dtl.RA = *reinterpret_cast<const RepeatAfterPrd *>(&v);
	return 1;
}

SString & DateRepeating::Format(int fmt, SString & rBuf) const
{
	rBuf.Z();
	SString dowtxt;
	switch(Prd) {
		case PRD_DAY:
			rBuf.Cat("DAILY").CatDiv('-', 1);
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
				rBuf.TrimRight().Transf(CTRANSF_OUTER_TO_INNER).CatDiv(';', 2);
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
				rBuf.Cat(buf).Transf(CTRANSF_OUTER_TO_INNER).CatDiv(';', 2);
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
int DateTimeRepeating::Init(int prd, int kind, LDATE dt, LTIME tm)
{
	Time = tm;
	return DateRepeating::Init(prd, kind, dt);
}

int DateTimeRepeating::Next_(LDATETIME startDtm, LDATETIME * pNextDtm) const
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
				if(s < SlConst::SecsPerDay) {
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

SString & DateTimeRepeating::Format(int fmt, SString & rBuf) const
{
	rBuf.Z();
	DateRepeating::Format(fmt, rBuf);
	return rBuf.Cat(Time, TIMF_HMS);
}
//
//
//
DateRepIterator::DateRepIterator(const DateRepeating & rDr, LDATE startDate, LDATE endDate, uint maxCount) : Dr(rDr), Count(0),
	Start(NZOR(startDate, getcurdate_())), End(endDate), MaxCount(maxCount)
{
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
		Cur = (!checkdate(result) || (MaxCount && Count > MaxCount) || (End && result > End)) ? ZERODATE : result;
	}
	return Cur;
}

DateTimeRepIterator::DateTimeRepIterator(const DateTimeRepeating & rDr, LDATETIME startDtm, LDATE endDate, uint maxCount) :
	DtIter((const DateRepeating &)rDr, startDtm.d, endDate, 0)
{
	Dr = rDr;
	Start = startDtm;
	SETIFZ(Start.d, getcurdate_());
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
					if(s < SlConst::SecsPerDay) {
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
//
///*static*/const int SUniTime_Internal::Undef_TimeZone = 1000;

SUniDate_Internal::SUniDate_Internal() : Y(0), M(0), D(0)
{
}

SUniDate_Internal::SUniDate_Internal(int y, uint m, uint d) : Y(y), M(m), D(d)
{
}

SUniDate_Internal & SUniDate_Internal::Z()
{
	Y = 0;
	M = 0;
	D = 0;
	return *this;
}

bool SUniDate_Internal::SetDaysSinceChristmas(uint g)
{
	bool   ok = true;
	const uint   h4y = g / (400 * 365 + 97);
	const uint   h4d_rem = g % (400 * 365 + 97);
	const uint   hy = h4d_rem / (100 * 365 + 24);
	const uint   hy_rem = h4d_rem % (100 * 365 + 24);
	const uint   fy = hy_rem / (4 * 365 + 1);
	const uint   fy_rem = hy_rem % (4 * 365 + 1);
	const uint   y = fy_rem / 365;
	D = fy_rem % 365;
	M = 0;
	Y = (h4y * 400) + (hy * 100) + (fy * 4) + y + 1;
	if(D == 0) {
		Y--;
		M = 12;
		D = (y == 4 || hy == 4) ? 30 : 31; // Граничная проблема: конец четверки годов или 400-летия
	}
	else if(IsLeapYear_Gregorian(Y)) {
		M = LeapYearDayToMonth[D-1];
		D -= LeapYearDaysPrecedingMonth[M];
		M++;
	}
	else {
		M = NormalYearDayToMonth[D-1];
		D -= NormalYearDaysPrecedingMonth[M];
		M++;
	}
	return ok;
}

/*static*/bool SUniTime_Internal::ValidateTimeZone(int tz)
{
	assert((tz >= -(12 * 3600) && tz <= +(14 * 3600)) || tz == Undef_TimeZone);
	return ((tz >= -(12 * 3600) && tz <= +(14 * 3600)) || tz == Undef_TimeZone);
}

/*static*/int64 SUniTime_Internal::EpochToNs100(int64 epochTimeSec)
{
	return epochTimeSec * TICKSPERSEC + SlConst::Epoch1600_1970_Offs_100Ns;
}

SUniTime_Internal::SUniTime_Internal()
{
	THISZERO();
	TimeZoneSc  = Undef_TimeZone;
}

SUniTime_Internal & SUniTime_Internal::Z()
{
	THISZERO();
	TimeZoneSc  = Undef_TimeZone;
	return *this;
}

SUniTime_Internal::SUniTime_Internal(SCtrGenerate)
{
	//
	// Этот конструктор предназначен для генерации случайного времени для тестирования.
	// Учитывая назначение, здесь мы не применяем каких-либо функций собственного класса и 
	// более низкоуровневых, которые могут быть объектами тестирования.
	//
	THISZERO();
	TimeZoneSc  = Undef_TimeZone;
	SRandGenerator & r_rg = SLS.GetTLA().Rg;
	ulong _ry = 0;
	do {
		_ry = r_rg.GetUniformInt(10000000) % 3000U;
	} while(_ry < 1601 || _ry > 3000);
	Y = _ry;
	ulong _rm = 0;
	do {
		_rm = r_rg.GetUniformInt(10000000) % 13;
	} while(_rm < 1);
	M = _rm;
	ulong _rd = 0;
	do {
		_rd = r_rg.GetUniformInt(10000000) % 32;
	} while(_rd < 1 || _rd > dayspermonth(M, Y));
	D = _rd;
	ulong _rs = r_rg.GetUniformInt(10000000) % (24 * 3600);
	Mn = _rs / 60;
	Sc = _rs % 60;
	Hr = Mn / 60;
	Mn = Mn % 60;
	MSc = r_rg.GetUniformInt(1000);
	assert(IsValid());
}

bool SUniTime_Internal::IsValid() const
{
	return (checkirange(Y, 1, 3000) && checkirange(M, 1U, 12U) && 
		checkirange(D, 1U, dayspermonth(M, Y)) && (Hr < 24) && (Mn < 60) && (Sc < 60) &&
		(MSc < 1000ULL) && ValidateTimeZone(TimeZoneSc));
}
	
int FASTCALL SUniTime_Internal::Cmp(const SUniTime_Internal & rS) const
{
	int    si = 0;
	CMPCASCADE7(si, this, &rS, Y, M, D, Hr, Mn, Sc, MSc);
	return si;
}

uint64 SUniTime_Internal::Cmp(const SUniTime_Internal & rS, uint64 uedTimedMeta) const
{
	int    si = 0;
	uint64 result = UED_CMP_EQUAL;
	switch(uedTimedMeta) {
		case UED_META_TIME_MSEC:
			CMPCASCADE7(si, this, &rS, Y, M, D, Hr, Mn, Sc, MSc);
			break;
		case UED_META_TIME_SEC:
			CMPCASCADE6(si, this, &rS, Y, M, D, Hr, Mn, Sc);
			break;
		case UED_META_TIME_MIN:
			CMPCASCADE5(si, this, &rS, Y, M, D, Hr, Mn);
			break;
		case UED_META_TIME_HR:
			CMPCASCADE4(si, this, &rS, Y, M, D, Hr);
			break;
		case UED_META_DATE_DAY:
			CMPCASCADE3(si, this, &rS, Y, M, D);
			break;
		case UED_META_DATE_MON:
			CMPCASCADE2(si, this, &rS, Y, M);
			break;
		case UED_META_DATE_YR:
			si = CMPSIGN(this->Y, rS.Y);
			break;
		default:
			result = UED_CMP_INCOMPARABLE;
			break;
	}
	if(result != UED_CMP_INCOMPARABLE) {
		if(si == 0)
			result = UED_CMP_EQUAL;
		else if(si < 0)
			result = UED_CMP_LESS;
		else
			result = UED_CMP_GREATER;
	}
	return result;
}

#if 1 // {

static int FASTCALL DaysSinceEpoch(int year)
{
    year--; // Don't include a leap day from the current year
    int days = year * DAYSPERNORMALYEAR + year / 4 - year / 100 + year / 400;
    days -= (EPOCHYEAR-1) * DAYSPERNORMALYEAR + (EPOCHYEAR-1) / 4 - (EPOCHYEAR-1) / 100 + (EPOCHYEAR-1) / 400;
    return days;
}
//
// MaxDaysInMonth(IN ULONG Year, IN ULONG Month)
//
// The maximum number of days in a month depend on the year and month.
// It is the difference between the days to the month and the days to the following month
//
#define MaxDaysInMonth(YEAR, MONTH) (IsLeapYear_Gregorian(YEAR) ? LeapYearDaysPrecedingMonth[(MONTH)+1] - LeapYearDaysPrecedingMonth[(MONTH)] : \
	NormalYearDaysPrecedingMonth[(MONTH)+1] - NormalYearDaysPrecedingMonth[(MONTH)])
//
// Descr: This routine converts an input 64-bit time value to the number
//   of total elapsed days and the number of milliseconds in the partial day.
// ARG(Time IN): Supplies the input time to convert from
// ARG(Milliseconds OUT): Receives the number of milliseconds in the partial day
// Returns:
//   ElapsedDays
//
static uint32 FASTCALL __TimeToDaysAndFraction_ms(uint64 t, uint32 * pMilliseconds)
{
	uint64 total_milliseconds = Convert100nsToMilliseconds(t); // Convert the input time to total milliseconds
	uint64 temp = ConvertMillisecondsToDays(total_milliseconds); // Convert milliseconds to total days
	uint32 elapsed_days = (uint32)(temp & 0xffffffffLL); // Set the elapsed days from temp, we've divided it enough so that the high part must be zero.
	//
	//  Calculate the exact number of milliseconds in the elapsed days
	//  and subtract that from the total milliseconds to figure out
	//  the number of milliseconds left in the partial day
	//
	temp = ConvertDaysToMilliseconds(elapsed_days);
	temp = total_milliseconds - temp;
	*pMilliseconds = (uint32)(temp & 0xffffffffLL); // Set the fraction part from temp, the total number of milliseconds in a day guarantees that the high part must be zero.
	return elapsed_days;
}

static uint32 FASTCALL __TimeToDaysAndFraction_mks(uint64 t, uint64 * pMicroseconds)
{
	uint64 total_microseconds = Convert100nsToMicroseconds(t); // Convert the input time to total microseconds
	//uint64 temp = ConvertMicrosecondsToDays(total_microseconds); // Convert microseconds to total days
	uint64 temp = (total_microseconds / (24ULL * 3600ULL * 1000000ULL));
	uint32 elapsed_days = (uint32)(temp & 0xffffffffULL); // Set the elapsed days from temp, we've divided it enough so that the high part must be zero.
	//
	//  Calculate the exact number of microseconds in the elapsed days
	//  and subtract that from the total microseconds to figure out
	//  the number of microseconds left in the partial day
	//
	*pMicroseconds = total_microseconds - ConvertDaysToMicroseconds(elapsed_days);
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
	const uint32 number_of_400s = elapsedDays / SlConst::DaysPer400Years;
	elapsedDays -= number_of_400s * SlConst::DaysPer400Years;
	//
	// A 100 year time block is 365*100 + 100/4 - 100/100 = 36524 days long.
	// The computation for the number of 100 year blocks is biased by 3/4 days per
	// 100 years to account for the extra leap day thrown in on the last year of each 400 year block.
	//
	uint32 number_of_100s = (elapsedDays * 100 + 75) / 3652425;
	elapsedDays -= number_of_100s * 36524;
	//
	// A 4 year time block is 365*4 + 4/4 = 1461 days long.
	//
	uint32 number_of_4s = elapsedDays / SlConst::DaysPer4Years;
	elapsedDays -= number_of_4s * SlConst::DaysPer4Years;
	//
	// Now the number of whole years is the number of 400 year blocks times 400,
	// 100 year blocks time 100, 4 year blocks times 4, and the number of elapsed
	// whole years, taking into account the 3/4 day per year needed to handle the leap year.
	//
	return (number_of_400s * 400) + (number_of_100s * 100) + (number_of_4s * 4) + (elapsedDays * 100 + 75) / 36525;
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
#if 0 // {
static void FASTCALL __TimeToTimeFields(uint64 time100ns, SUniTime_Internal * pTimeFields)
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
	uint32 days = __TimeToDaysAndFraction_ms(time100ns, &milliseconds);
	//
	//  Compute which weekday it is and save it away now in the output
	//  variable.  We add the weekday of the base day to bias our computation
	//  which means that if one day has elapsed then we the weekday we want
	//  is the Jan 2nd, 1601.
	//
	pTimeFields->Weekday = static_cast<int16>((days + WEEKDAY_OF_1601) % 7);
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
	if(IsLeapYear_Gregorian(years + 1)) {
		//
		// The current year is a leap year, so figure out what month
		// it is, and then subtract the number of days preceding the
		// month from the days to figure out what day of the month it is
		//
		month = LeapYearDayToMonth[days];
		days = days - LeapYearDaysPrecedingMonth[month];
	}
	else {
		//
		// The current year is a normal year, so figure out the month
		// and days as described above for the leap year case
		//
		month = NormalYearDayToMonth[days];
		days = days - NormalYearDaysPrecedingMonth[month];
	}
	//
	// Now we need to compute the elapsed hour, minute, second, milliseconds
	// from the millisecond variable.  This variable currently contains
	// the number of milliseconds in our input time variable that did not
	// fit into a whole day.  To compute the hour, minute, second part
	// we will actually do the arithmetic backwards computing milliseconds
	// seconds, minutes, and then hours.  We start by computing the
	// number of whole seconds left in the day, and then computing
	// the millisecond remainder.
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
	pTimeFields->Y   = static_cast<int>(years + 1601);
	pTimeFields->M   = static_cast<int>(month + 1);
	pTimeFields->D   = static_cast<int>(days + 1);
	pTimeFields->Hr  = static_cast<int>(hours);
	pTimeFields->Mn  = static_cast<int>(minutes);
	pTimeFields->Sc  = static_cast<int>(seconds);
	pTimeFields->MkSc = static_cast<int>(milliseconds * 1000);
}
#endif // } 0
//
// Descr: This routine converts an input 64-bit LARGE_INTEGER variable to its corresponding
//   time field record.  It will tell the caller the year, month, day, hour,
//   minute, second, millisecond, and weekday corresponding to the input time variable.
// ARG(time100ns IN): Supplies the time value (in hundrets of nanoseconds) to interpret
//
void FASTCALL SUniTime_Internal::SetTime100ns(uint64 time100ns)
{
	uint32 month;
	uint32 hours;
	uint32 minutes;
	uint32 seconds;
	uint32 msecs;
	//
	//  First divide the input time 64 bit time variable into
	//  the number of whole days and part days (in milliseconds)
	//
	uint32 days = __TimeToDaysAndFraction_ms(time100ns, &msecs);
	//
	//  Compute which weekday it is and save it away now in the output
	//  variable.  We add the weekday of the base day to bias our computation
	//  which means that if one day has elapsed then we the weekday we want
	//  is the Jan 2nd, 1601.
	//
	Weekday = static_cast<int16>((days + WEEKDAY_OF_1601) % 7);
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
	if(IsLeapYear_Gregorian(years + 1)) {
		//
		// The current year is a leap year, so figure out what month
		// it is, and then subtract the number of days preceding the
		// month from the days to figure out what day of the month it is
		//
		month = LeapYearDayToMonth[days];
		days = days - LeapYearDaysPrecedingMonth[month];
	}
	else {
		//
		// The current year is a normal year, so figure out the month
		// and days as described above for the leap year case
		//
		month = NormalYearDayToMonth[days];
		days = days - NormalYearDaysPrecedingMonth[month];
	}
	//
	// Now we need to compute the elapsed hour, minute, second, milliseconds
	// from the millisecond variable.  This variable currently contains
	// the number of milliseconds in our input time variable that did not
	// fit into a whole day.  To compute the hour, minute, second part
	// we will actually do the arithmetic backwards computing milliseconds
	// seconds, minutes, and then hours.  We start by computing the
	// number of whole seconds left in the day, and then computing
	// the millisecond remainder.
	//
	seconds = static_cast<int>(msecs / 1000);
	msecs = msecs % 1000;
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
	Y   = static_cast<int>(years + 1601);
	M   = static_cast<int>(month + 1);
	D   = static_cast<int>(days + 1);
	Hr  = static_cast<int>(hours);
	Mn  = static_cast<int>(minutes);
	Sc  = static_cast<int>(seconds);
	MSc = msecs;
}

int FASTCALL SUniTime_Internal::GetTime100ns(uint64 * pTime100ns) const
{
	int    ok = 1;
	//
	// Load the time field elements into local variables.  This should
	// ensure that the compiler will only load the input elements
	// once, even if there are alias problems.  It will also make
	// everything (except the year) zero based.  We cannot zero base the
	// year because then we can't recognize cases where we're given a year
	// before 1601.
	//
	const uint32 year  = Y;
	const uint32 month = M-1;
	const uint32 day   = D-1;
	const uint32 hour  = Hr;
	const uint32 minute = Mn;
	const uint32 second = Sc;
	const uint32 msecs  = MSc;
	//
	// Check that the time field input variable contains proper values.
	//
	if((M<1) || (D < 1) || (year<1601) || (month>11) ||
		((int16)day >= MaxDaysInMonth(year, month)) || (hour>23) || (minute>59) || (second>59) || (msecs > 999)) {
		ok = 0;
	}
	else {
		//
		// Compute the total number of elapsed days represented by the input time field variable
		//
		uint32 elapsed_days = ElapsedYearsToDays(year-1601);
		if(IsLeapYear_Gregorian(year-1600))
			elapsed_days += LeapYearDaysPrecedingMonth[month];
		else
			elapsed_days += NormalYearDaysPrecedingMonth[month];
		elapsed_days += day;
		//
		// Now compute the total number of milliseconds in the fractional part of the day
		//
		//uint32 elapsed_milliseconds = ((((hour*60) + minute)*60 + second)*1000 + milliseconds);
		//
		// Given the elapsed days and milliseconds we can now build the output time variable
		//
		//*pTime = DaysAndFractionToTime(elapsed_days, elapsed_milliseconds);
		*pTime100ns = ((((uint64)elapsed_days) * (86400ULL * 1000ULL)) + ((uint64)((((hour*60) + minute)*60 + second)*1000ULL + msecs))) * 10000ULL;
	}
	return ok;
}
//
// Descr: This routine converts an input Time Field variable to a 64-bit NT time
//   value.  It ignores the WeekDay of the time field.
// Arguments:
// ARG(pTimeFields IN): Supplies the time field record to use
// ARG(pTime      OUT): Receives the NT Time corresponding to TimeFields
// Returns:
//   !0 - if the Time Fields is well formed and within the range of time expressible by LARGE_INTEGER
//   0 - error
//
static int FASTCALL __TimeFieldsToTime_Removed(const SUniTime_Internal * pTimeFields, uint64 * pTime)
{
	//
	// Load the time field elements into local variables.  This should
	// ensure that the compiler will only load the input elements
	// once, even if there are alias problems.  It will also make
	// everything (except the year) zero based.  We cannot zero base the
	// year because then we can't recognize cases where we're given a year
	// before 1601.
	//
	uint32 year  = pTimeFields->Y;
	uint32 month = pTimeFields->M-1;
	uint32 day   = pTimeFields->D-1;
	uint32 hour  = pTimeFields->Hr;
	uint32 minute = pTimeFields->Mn;
	uint32 second = pTimeFields->Sc;
	uint32 msecs  = pTimeFields->MSc;
	//
	// Check that the time field input variable contains proper values.
	//
	if((pTimeFields->M<1) || (pTimeFields->D < 1) || (year<1601) || (month>11) ||
		((int16)day >= MaxDaysInMonth(year, month)) || (hour>23) || (minute>59) || (second>59) || (msecs>999)) {
		return FALSE;
	}
	else {
		//
		// Compute the total number of elapsed days represented by the input time field variable
		//
		uint32 elapsed_days = ElapsedYearsToDays(year-1601);
		if(IsLeapYear_Gregorian(year-1600))
			elapsed_days += LeapYearDaysPrecedingMonth[month];
		else
			elapsed_days += NormalYearDaysPrecedingMonth[month];
		elapsed_days += day;
		//
		// Now compute the total number of milliseconds in the fractional part of the day
		//
		//uint32 elapsed_milliseconds = ((((hour*60) + minute)*60 + second)*1000 + milliseconds);
		//
		// Given the elapsed days and milliseconds we can now build the output time variable
		//
		//*pTime = DaysAndFractionToTime(elapsed_days, elapsed_milliseconds);
		*pTime = ((((uint64)elapsed_days) * (86400LL * 1000LL)) + ((uint64)((((hour*60) + minute)*60 + second)*1000 + msecs))) * 10000LL;
		return TRUE; // And return to our caller
	}
}

void FASTCALL __EpochTimeToTimeFields(uint64 epochTime, SUniTime_Internal * pTimeFields)
{
	//__TimeToTimeFields(epochTime * TICKSPERSEC + SlConst::Epoch1600_1970_Offs_100Ns, pTimeFields);
	pTimeFields->SetTime100ns(epochTime * TICKSPERSEC + SlConst::Epoch1600_1970_Offs_100Ns);
}

int FASTCALL __TimeFieldsToEpochTime(const SUniTime_Internal * pTimeFields, uint64 * pEpochTime)
{
	int    ok = 1;
	uint64 t;
	if(pTimeFields->GetTime100ns(&t)) {
		if(t < SlConst::Epoch1600_1970_Offs_100Ns)
			ok = 0;
		else
			*pEpochTime = (t - SlConst::Epoch1600_1970_Offs_100Ns) / TICKSPERSEC;
	}
	else
		ok = 0;
	return ok;
}

#endif // } 0
//
//
//
//#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  //#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
//#else
  //#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
//#endif

int gettimeofday(struct timeval * pTv, struct timezone * pTz)
{
	FILETIME ft;
	uint64 tmpres = 0;
	if(pTv) {
		GetSystemTimeAsFileTime(&ft);
		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;
		tmpres /= 10; // convert into microseconds
		// converting file time to unix epoch
		tmpres -= SlConst::Epoch1600_1970_Offs_Mks;
		pTv->tv_sec  = (long)(tmpres / 1000000UL);
		pTv->tv_usec = (long)(tmpres % 1000000UL);
	}
	if(pTz) {
		static int tzflag = 0;
		if(!tzflag) {
			ENTER_CRITICAL_SECTION
			if(!tzflag) {
				_tzset();
				tzflag++;
			}
			LEAVE_CRITICAL_SECTION
		}
		pTz->tz_minuteswest = _timezone / 60;
		pTz->tz_dsttime = _daylight;
	}
	return 0;
}
//
//
//
#define UNITIME_VALUE_MASK 0x00ffffffffffffffLL

static inline uint8 SUniTime_Decode(const uint8 * pD, uint64 * pValue)
{
	*pValue = (pD[7] & 0x80) ? (PTR64C(pD)[0] & UNITIME_VALUE_MASK) : PTR64C(pD)[0];
	return pD[7];
}

static inline void SUniTime_Encode(uint8 * pD, uint8 signature, uint64 value)
{
	if(signature & 0x80) {
		assert((value & ~UNITIME_VALUE_MASK) == 0);
		PTR64(pD)[0] = (value & UNITIME_VALUE_MASK);
		pD[7] = signature;
	}
	else {
		PTR64(pD)[0] = value;
	}
}

int SUniTime::Implement_Set(uint8 signature, const void * pData)
{
	int    ok = 1;
	const SUniTime_Internal * p_inner = static_cast<const SUniTime_Internal *>(pData);
	uint64 value = 0;
	switch(signature) {
		case indMSec:
			if(p_inner->GetTime100ns(&value))
				value /= 10000LL;
			else
				ok = 0;
			break;
		case indSec:
			if(p_inner->GetTime100ns(&value))
				value /= 10000000LL;
			else
				ok = 0;
			break;
		case indMin:
			if(p_inner->GetTime100ns(&value))
				value /= (60*10000000LL);
			else
				ok = 0;
			break;
		case indHr:
			if(p_inner->GetTime100ns(&value))
				value /= (60*60*10000000LL);
			else
				ok = 0;
			break;
		case indDay:
			value = p_inner->GetDaysSinceChristmas();
			break;
		case indMon:
			value = SUniDate_Internal::DateToDaysSinceChristmas(p_inner->Y, p_inner->M, 2);
			break;
		case indQuart:
			value = SUniDate_Internal::DateToDaysSinceChristmas(p_inner->Y, (((p_inner->M-1) / 3) * 3) + 1, 2);
			break;
		case indSmYr:
			value = SUniDate_Internal::DateToDaysSinceChristmas(p_inner->Y, (((p_inner->M-1) / 6) * 6) + 1, 2);
			break;
		case indYr:
			if(p_inner->Y > 0 && p_inner->Y < 3000)
				value = p_inner->Y;
			else
				ok = 0;
			break;
		case indDYr:
			value = (((p_inner->Y-1) / 10) * 10) + 1;
			break;
		case indSmCent:
			value = (((p_inner->Y-1) / 50) * 50) + 1;
			break;
		case indCent:
			value = (((p_inner->Y-1) / 100) * 100) + 1;
			break;
		case indMillennium:
			value = (((p_inner->Y-1) / 1000) * 1000) + 1;
			break;
		case indMSecTz:
			if(SUniTime_Internal::ValidateTimeZone(p_inner->TimeZoneSc) && p_inner->GetTime100ns(&value))
				value = ((value / 10000LL) << 8) | static_cast<int8>(p_inner->TimeZoneSc);
			else
				ok = 0;
			break;
		case indCSecTz:
			if(SUniTime_Internal::ValidateTimeZone(p_inner->TimeZoneSc) && p_inner->GetTime100ns(&value))
				value = ((value / 100000LL) << 8) | static_cast<int8>(p_inner->TimeZoneSc);
			else
				ok = 0;
			break;
		case indSecTz:
			if(SUniTime_Internal::ValidateTimeZone(p_inner->TimeZoneSc) && p_inner->GetTime100ns(&value))
				value = ((value / 10000000LL) << 8) | static_cast<int8>(p_inner->TimeZoneSc);
			else
				ok = 0;
			break;
		case indMinTz:
			if(SUniTime_Internal::ValidateTimeZone(p_inner->TimeZoneSc) && p_inner->GetTime100ns(&value))
				value = ((value / (60 * 10000000LL)) << 8) | static_cast<int8>(p_inner->TimeZoneSc);
			else
				ok = 0;
			break;
		case indHrTz:
			if(SUniTime_Internal::ValidateTimeZone(p_inner->TimeZoneSc) && p_inner->GetTime100ns(&value))
				value = ((value / (60 * 60 * 10000000LL)) << 8) | static_cast<int8>(p_inner->TimeZoneSc);
			else
				ok = 0;
			break;
		case indDayBC:
			ok = 0; // @construction
			break;
		case indMonBC:
			ok = 0; // @construction
			break;
		case indYrBC:
			ok = 0; // @construction
			break;
		case indDYrBC:
			ok = 0; // @construction
			break;
		case indCentBC:
			ok = 0; // @construction
			break;
		case indMillenniumBC:
			ok = 0; // @construction
			break;
	}
	SUniTime_Encode(D, signature, value);
	return ok;
}

static int FASTCALL IsSUniTimeCompatibleWithInnerStruc(uint8 signature)
{
	return oneof7(signature, SUniTime::indDefault, SUniTime::indMSec, SUniTime::indSec, SUniTime::indMin, SUniTime::indHr, SUniTime::indDay, SUniTime::indMon) ||
		oneof7(signature, SUniTime::indQuart, SUniTime::indSmYr, SUniTime::indYr, SUniTime::indDYr, SUniTime::indSmCent, SUniTime::indCent, SUniTime::indMillennium) ||
		oneof5(signature, SUniTime::indHrTz, SUniTime::indMinTz, SUniTime::indSecTz, SUniTime::indCSecTz, SUniTime::indMSecTz);
}
//
// Descr: Сравнивает точность представления времени с сигнатурами signature1 и signature2.
// Note: Существуют специальные случаи, которые пока не рассматриваются. А именно:
//   -- точность одинакова, но одна из сигнатур предполагает хранение временной зоны;
//   -- сравнение точностей для типов значений, которые нельзя привести один к другому (indMillennium vs indMillenniumBC, например).
//
// Returns:
//   0 - точность эквивалентна
//  -1 - signature1 имеет меньшую точность (большую гранулярность), чем signature2
//  +1 - signature1 имеет большую точность (меньшую гранулярность), чем signature2
//
static int FASTCALL CmpSUniTimePrecisions(uint8 signature1, uint8 signature2)
{
	int    s = 0;
	int    c1 = IsSUniTimeCompatibleWithInnerStruc(signature1);
	int    c2 = IsSUniTimeCompatibleWithInnerStruc(signature2);
	if(c1 && c2) {
		s = (signature1 < signature2) ? -1 : ((signature1 > signature2) ? +1 : 0);
	}
	return s;
}

static int FASTCALL Downgrade_SUniTime_Inner(SUniTime_Internal & rT, uint8 signature)
{
	int    ok = 1;
	switch(signature) {
		case SUniTime::indDefault:
			break;
		case SUniTime::indMSec:
		case SUniTime::indSec:
			rT.MSc = 0;
			break;
		case SUniTime::indMin:
			rT.MSc = 0;
			rT.Sc = 0;
			break;
		case SUniTime::indHr:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			break;
		case SUniTime::indDay:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			break;
		case SUniTime::indMon:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.D = 2;
			break;
		case SUniTime::indQuart:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = (((rT.M-1) / 3) * 3) + 1;
			rT.D = 2;
			break;
		case SUniTime::indSmYr:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = (((rT.M-1) / 6) * 6) + 1;
			rT.D = 2;
			break;
		case SUniTime::indYr:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = 1;
			rT.D = 2;
			break;
		case SUniTime::indDYr:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = 1;
			rT.D = 2;
			rT.Y = (((rT.Y-1) / 10) * 10) + 1;
			break;
		case SUniTime::indSmCent:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = 1;
			rT.D = 2;
			rT.Y = (((rT.Y-1) / 50) * 50) + 1;
			break;
		case SUniTime::indCent:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = 1;
			rT.D = 2;
			rT.Y = (((rT.Y-1) / 100) * 100) + 1;
			break;
		case SUniTime::indMillennium:
			rT.MSc = 0;
			rT.Sc = 0;
			rT.Mn = 0;
			rT.Hr = 0;
			rT.M = 1;
			rT.D = 2;
			rT.Y = (((rT.Y-1) / 1000) * 1000) + 1;
			break;
		case SUniTime::indDayBC:
			ok = 0; // @construction
			break;
		case SUniTime::indMonBC:
			ok = 0; // @construction
			break;
		case SUniTime::indYrBC:
			ok = 0; // @construction
			break;
		case SUniTime::indDYrBC:
			ok = 0; // @construction
			break;
		case SUniTime::indCentBC:
			ok = 0; // @construction
			break;
		case SUniTime::indMillenniumBC:
			ok = 0; // @construction
			break;
	}
	return ok;
}

int FASTCALL SUniTime::Compare(const SUniTime & rS, int * pQualification) const
{
	int    result = 0;
	int    qualification = cqUndef;
	uint64 value = 0;
	uint64 value_s = 0;
	uint8  signature = SUniTime_Decode(D, &value);
	uint8  signature_s = SUniTime_Decode(rS.D, &value_s);
	if(signature == signature_s) {
		qualification = cqSure;
		result = CMPSIGN(value, value_s);
	}
	else {
		int cm = IsSUniTimeCompatibleWithInnerStruc(signature);
		int cm_s = IsSUniTimeCompatibleWithInnerStruc(signature_s);
		if(cm && cm_s) {
			SUniTime_Internal in;
			SUniTime_Internal in_s;
			Implement_Get(&in);
			rS.Implement_Get(&in_s);
			int cp = CmpSUniTimePrecisions(signature, signature_s);
			if(cp != 0) {
				if(cp < 0)
					Downgrade_SUniTime_Inner(in_s, signature);
				else if(cp > 0)
					Downgrade_SUniTime_Inner(in, signature_s);
				result = in.Cmp(in_s);
				qualification = cqUncertain;
			}
			else {
				result = in.Cmp(in_s);
				qualification = cqSure;
			}
		}
		else {
			// Результат не определен (мы не можем пока сопоставить такие значения)
		}
	}
	ASSIGN_PTR(pQualification, qualification);
	return result;
}

int /*not bool!*/ FASTCALL SUniTime::IsEq(const SUniTime & rS) const
{
	int   cq = 0;
	int   result = Compare(rS, &cq);
	if(cq == cqSure)
		return (result == 0) ? cmprSureTrue : cmprSureFalse;
	else if(cq == cqUncertain)
		return (result == 0) ? cmprUncertainTrue : cmprSureFalse;
	else
		return cmprIncompat;
}

uint8  SUniTime::Implement_Get(void * pData) const
{
	uint64 value = 0;
	uint8  signature = SUniTime_Decode(D, &value);
	int8   timezone = 0;
	long   day_count = 0;
	SUniTime_Internal * p_inner = static_cast<SUniTime_Internal *>(pData);
	switch(signature) {
		case indDefault: 
			//__TimeToTimeFields(value, p_inner); 
			p_inner->SetTime100ns(value);
			break;
		case indMSec: 
			//__TimeToTimeFields(value * 10000LL, p_inner); 
			p_inner->SetTime100ns(value * 10000LL);
			break;
		case indSec: 
			//__TimeToTimeFields(value * 10000000LL, p_inner); 
			p_inner->SetTime100ns(value * 10000000LL);
			break;
		case indMin: 
			//__TimeToTimeFields(value * 60*10000000LL, p_inner); 
			p_inner->SetTime100ns(value * 60*10000000LL);
			break;
		case indHr: 
			//__TimeToTimeFields(value * 60*60*10000000LL, p_inner); 
			p_inner->SetTime100ns(value * 60*60*10000000LL);
			break;
		case indMSecTz:
			timezone = static_cast<int8>(value & 0xff);
			value >>= 8;
			//__TimeToTimeFields(value * 10000LL, p_inner);
			p_inner->SetTime100ns(value * 10000LL);
			p_inner->Hr += timezone;
			p_inner->TimeZoneSc = timezone;
			break;
		case indCSecTz:
			timezone = static_cast<int8>(value & 0xff);
			value >>= 8;
			//__TimeToTimeFields(value * 100000LL, p_inner);
			p_inner->SetTime100ns(value * 100000LL);
			p_inner->Hr += timezone;
			p_inner->TimeZoneSc = timezone;
			break;
		case indSecTz:
			timezone = static_cast<int8>(value & 0xff);
			value >>= 8;
			//__TimeToTimeFields(value * 10000000LL, p_inner);
			p_inner->SetTime100ns(value * 10000000LL);
			p_inner->Hr += timezone;
			p_inner->TimeZoneSc = timezone;
			break;
		case indMinTz:
			timezone = static_cast<int8>(value & 0xff);
			value >>= 8;
			//__TimeToTimeFields(value * 60*10000000LL, p_inner);
			p_inner->SetTime100ns(value * 60*10000000LL);
			p_inner->Hr += timezone;
			p_inner->TimeZoneSc = timezone;
			break;
		case indHrTz:
			timezone = static_cast<int8>(value & 0xff);
			value >>= 8;
			//__TimeToTimeFields(value * 60*60*10000000LL, p_inner);
			p_inner->SetTime100ns(value * 60*60*10000000LL);
			p_inner->Hr += timezone;
			p_inner->TimeZoneSc = timezone;
			break;
		case indDay:
			p_inner->SetDaysSinceChristmas((uint)value);
			break;
		case indMon:
			p_inner->SetDaysSinceChristmas((uint)value);
			p_inner->D = 2;
			break;
		case indQuart:
			p_inner->SetDaysSinceChristmas((uint)value);
			p_inner->M = (((p_inner->M-1) / 3) * 3) + 1;
			p_inner->D = 2;
			break;
		case indSmYr:
			p_inner->SetDaysSinceChristmas((uint)value);
			p_inner->M = (((p_inner->M-1) / 6) * 6) + 1;
			p_inner->D = 2;
			break;
		case indYr:
			p_inner->Y = (long)value;
			p_inner->M = 1;
			p_inner->D = 2; // day not equal 1 in order to avoid problem with time zones
			break;
		case indDYr:
			p_inner->Y = (((((long)value)-1) / 10) * 10) + 1;
			p_inner->M = 1;
			p_inner->D = 2;
			break;
		case indSmCent:
			p_inner->Y = (((((long)value)-1) / 50) * 50) + 1;
			p_inner->M = 1;
			p_inner->D = 2;
			break;
		case indCent:
			p_inner->Y = (((((long)value)-1) / 100) * 100) + 1;
			p_inner->M = 1;
			p_inner->D = 2;
			break;
		case indMillennium:
			p_inner->Y = (((((long)value)-1) / 1000) * 1000) + 1;
			p_inner->M = 1;
			p_inner->D = 2;
			break;
		case indDayBC:
			signature = indInvalid; // @construction
			break;
		case indMonBC:
			signature = indInvalid; // @construction
			break;
		case indYrBC:
			signature = indInvalid; // @construction
			break;
		case indDYrBC:
			signature = indInvalid; // @construction
			break;
		case indCentBC:
			signature = indInvalid; // @construction
			break;
		case indMillenniumBC:
			signature = indInvalid; // @construction
			break;
	}
	return signature;
}

SUniTime::SUniTime()
{
	memzero(D, sizeof(D));
}

SUniTime::SUniTime(LDATE d)
{
	Set(d);
}
	
SUniTime::SUniTime(const LDATETIME & rD)
{
	Set(rD);
}

bool SUniTime::operator !() const { return ismemzero(D, sizeof(D)); }

SUniTime & SUniTime::Z()
{
	memzero(D, sizeof(D));
	return *this;
}

uint SUniTime::GetSignature() const
{
	uint64 value;
	return SUniTime_Decode(D, &value);
}

int64  SUniTime::ToInt64() const { return *reinterpret_cast<const int64 *>(D); }

int    SUniTime::FromInt64(int64 v)
{
	int    ok = 1;
	// @todo Необходимо проверить валидность устанавливаемого значение v.
	// Если оно не валидно, то вернуть 0.
	//uint64 value;
	//uint signature = SUniTime_Decode((const uint8 *)&v, &value);
	memcpy(D, &v, sizeof(D));
	return ok;
}

int FASTCALL SUniTime::SetYear(int year)
{
	SUniTime_Internal inner;
	inner.Y = year;
	return Implement_Set(indYr, &inner);
}

int FASTCALL SUniTime::SetMonth(int year, int month)
{
	SUniTime_Internal inner;
	inner.Y = year;
	inner.M = month;
	return Implement_Set(indMon, &inner);
}

int FASTCALL SUniTime::Set(LDATE d)
{
	SUniTime_Internal inner;
	inner.D = d.day();
	inner.M = d.month();
	inner.Y = d.year();
	return Implement_Set(indDay, &inner);
}

int FASTCALL SUniTime::Set(const LDATETIME & rD)
{
	SUniTime_Internal inner;
	inner.D = rD.d.day();
	inner.M = rD.d.month();
	inner.Y = rD.d.year();
	inner.Hr = rD.t.hour();
	inner.Mn = rD.t.minut();
	inner.Sc = rD.t.sec();
	inner.MSc = rD.t.hs() * 10;
	return Implement_Set(indMSec, &inner);
}

int FASTCALL SUniTime::Set(const LDATETIME & rD, uint signature)
{
	assert(oneof11(signature, indSec, indMin, indHr, indDay, indMon, indQuart, indSmYr, indYr, indDYr, indSmCent, indCent));
	if(oneof11(signature, indSec, indMin, indHr, indDay, indMon, indQuart, indSmYr, indYr, indDYr, indSmCent, indCent)) {
		SUniTime_Internal inner;
		inner.D = rD.d.day();
		inner.M = rD.d.month();
		inner.Y = rD.d.year();
		inner.Hr = rD.t.hour();
		inner.Mn = rD.t.minut();
		inner.Sc = rD.t.sec();
		inner.MSc = rD.t.hs() * 10;
		return Implement_Set(signature, &inner);
	}
	else
		return 0;
}

int FASTCALL SUniTime::Set(const LDATETIME & rD, uint signature, int timezone)
{
	assert(oneof5(signature, indHrTz, indMinTz, indSecTz, indMSecTz, indCSecTz));
	assert(timezone >= -12 && timezone <= +14);
	if(oneof5(signature, indHrTz, indMinTz, indSecTz, indMSecTz, indCSecTz)) {
		SUniTime_Internal inner;
		inner.D = rD.d.day();
		inner.M = rD.d.month();
		inner.Y = rD.d.year();
		inner.Hr = rD.t.hour() - timezone;
		inner.Mn = rD.t.minut();
		inner.Sc = rD.t.sec();
		inner.MSc = rD.t.hs() * 10;
		inner.TimeZoneSc = timezone;
		return Implement_Set(signature, &inner);
	}
	else
		return 0;
}

int FASTCALL SUniTime::Set(time_t t)
{
	int    ok = 1;
	if(t >= 0) {
		SUniTime_Encode(D, indSec, t * TICKSPERSEC + SlConst::Epoch1600_1970_Offs_100Ns);
	}
	else {
		Z();
		ok = 0;
	}
	return ok;
}

int FASTCALL SUniTime::Set(time_t t, int timezone)
{
	int    ok = 1;
	assert(timezone >= -12 && timezone <= +14);
	if(t >= 0 && timezone >= -12 && timezone <= +14) {
		SUniTime_Encode(D, indSecTz, t * TICKSPERSEC + SlConst::Epoch1600_1970_Offs_100Ns - (timezone * 3600));
	}
	else {
		Z();
		ok = 0;
	}
	return ok;
}

int FASTCALL SUniTime::Set(const FILETIME & rD)
{
	SUniTime_Encode(D, 0, ((uint64)rD.dwLowDateTime) | (((uint64)rD.dwHighDateTime) << 32));
	return 1;
}

int FASTCALL SUniTime::Get(LDATE & rD) const
{
	int    ok = 1;
	SUniTime_Internal inner;
	uint8 signature = Implement_Get(&inner);
	if(signature != indInvalid) {
		rD.encode(inner.D, inner.M, inner.Y);
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SUniTime::Get(LDATETIME & rD) const
{
	int    ok = 1;
	SUniTime_Internal inner;
	uint8 signature = Implement_Get(&inner);
	if(signature != indInvalid) {
		rD.d.encode(inner.D, inner.M, inner.Y);
		rD.t.encode(inner.Hr, inner.Mn, inner.Sc, static_cast<int>(inner.MSc));
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SUniTime::Get(time_t & rD) const
{
	int    ok = 1;
	SUniTime_Internal inner;
	uint8 signature = Implement_Get(&inner);
	if(signature != indInvalid) {
		uint64 et = 0;
		__TimeFieldsToEpochTime(&inner, &et);
		rD = (time_t)et;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SUniTime::Get(FILETIME & rD) const
{
	int    ok = 1;
	SUniTime_Internal inner;
	uint8 signature = Implement_Get(&inner);
	if(signature != indInvalid) {
		inner.GetTime100ns((uint64 *)&rD);
	}
	else
		ok = 0;
	return ok;
}
//
// SDN date
//

// 
// These are the externally visible components of this file:
// 
//   void SdnToGregorian(long int  sdn, int      *pYear, int      *pMonth, int      *pDay);
// 
// Convert a SDN to a Gregorian calendar date.  If the input SDN is less
// than 1, the three output values will all be set to zero, otherwise
// *pYear will be >= -4714 and != 0; *pMonth will be in the range 1 to 12
// inclusive; *pDay will be in the range 1 to 31 inclusive.
// 
//   int GregorianToSdn(int inputYear, uint inputMonth, uint inputDay);
// 
// Convert a Gregorian calendar date to a SDN.  Zero is returned when the
// input date is detected as invalid or out of the supported range.  The
// return value will be > 0 for all valid, supported dates, but there are
// some invalid dates that will return a positive value.  To verify that a
// date is valid, convert it to SDN and then back and compare with the original.
// 
// char *MonthNameShort[13];
// 
// Convert a Gregorian month number (1 to 12) to the abbreviated (three
// character) name of the Gregorian month (null terminated).  An index of
// zero will return a zero length string.
// 
//   char *MonthNameLong[13];
// 
// Convert a Gregorian month number (1 to 12) to the name of the Gregorian
// month (null terminated).  An index of zero will return a zero length string.
// 
// VALID RANGE
//   4714 B.C. to at least 10000 A.D.
// 
//   Although this software can handle dates all the way back to 4714
//   B.C., such use may not be meaningful.  The Gregorian calendar was
//   not instituted until October 15, 1582 (or October 5, 1582 in the
//   Julian calendar).  Some countries did not accept it until much
//   later.  For example, Britain converted in 1752, The USSR in 1918 and
//   Greece in 1923.  Most European countries used the Julian calendar
//   prior to the Gregorian.
// 
// CALENDAR OVERVIEW
//   The Gregorian calendar is a modified version of the Julian calendar.
//   The only difference being the specification of leap years.  The
//   Julian calendar specifies that every year that is a multiple of 4
//   will be a leap year.  This leads to a year that is 365.25 days long,
//   but the current accepted value for the tropical year is 365.242199 days.
// 
//   To correct this error in the length of the year and to bring the
//   vernal equinox back to March 21, Pope Gregory XIII issued a papal
//   bull declaring that Thursday October 4, 1582 would be followed by
//   Friday October 15, 1582 and that centennial years would only be a
//   leap year if they were a multiple of 400.  This shortened the year
//   by 3 days per 400 years, giving a year of 365.2425 days.
// 
//   Another recently proposed change in the leap year rule is to make
//   years that are multiples of 4000 not a leap year, but this has never
//   been officially accepted and this rule is not implemented in these algorithms.
// 
// ALGORITHMS
// 
//   The calculations are based on three different cycles: a 400 year
//   cycle of leap years, a 4 year cycle of leap years and a 5 month cycle of month lengths.
// 
//   The 5 month cycle is used to account for the varying lengths of
//   months.  You will notice that the lengths alternate between 30
//   and 31 days, except for three anomalies: both July and August
//   have 31 days, both December and January have 31, and February
//   is less than 30.  Starting with March, the lengths are in a
//   cycle of 5 months (31, 30, 31, 30, 31):
// 
//   Mar   31 days  \
//   Apr   30 days   |
//   May   31 days    > First cycle
//   Jun   30 days   |
//   Jul   31 days  /
// 
//   Aug   31 days  \
//   Sep   30 days   |
//   Oct   31 days    > Second cycle
//   Nov   30 days   |
//   Dec   31 days  /
// 
//   Jan   31 days  \
//   Feb 28/9 days   |
//                    > Third cycle (incomplete)
// 
//   For this reason the calculations (internally) assume that the year starts with March 1.
// 
// REFERENCES
// 
//   Conversions Between Calendar Date and Julian Day Number by Robert J.
//   Tantzen, Communications of the Association for Computing Machinery
//   August 1963.  (Also published in Collected Algorithms from CACM, algorithm number 199).
// 
#define GREGOR_SDN_OFFSET  32045
#define JULIAN_SDN_OFFSET  32083
#define DAYS_PER_5_MONTHS  153
//#define DAYS_PER_4_YEARS   1461
//#define DAYS_PER_400_YEARS 146097
//#define DAYS_PER_5_MONTHS  153
//#define DAYS_PER_4_YEARS   1461

bool SUniDate_Internal::SetSdnGregorian(uint g)
{
	Z();
	bool   ok = true;
	THROW(!(g <= 0 || g > (LONG_MAX - 4 * GREGOR_SDN_OFFSET) / 4));
	{
		uint   temp = (g + GREGOR_SDN_OFFSET) * 4 - 1;
		// Calculate the century (year/100)
		uint   century = temp / SlConst::DaysPer400Years;
		// Calculate the year and day of year (1 <= dayOfYear <= 366)
		temp = ((temp % SlConst::DaysPer400Years) / 4) * 4 + 3;
		Y = (century * 100) + (temp / SlConst::DaysPer4Years);
		uint   day_of_year = (temp % SlConst::DaysPer4Years) / 4 + 1;
		// Calculate the month and day of month
		temp = day_of_year * 5 - 3;
		M = temp / DAYS_PER_5_MONTHS;
		D = (temp % DAYS_PER_5_MONTHS) / 5 + 1;
		// Convert to the normal beginning of the year
		if(M < 10) {
			M += 3;
		}
		else {
			Y += 1;
			M -= 9;
		}
		// Adjust to the B.C./A.D. type numbering
		Y -= 4800;
		if(Y <= 0)
			Y--;
	}
	CATCHZOK
	return ok;
}

bool SUniDate_Internal::SetSdnJulian(uint g)
{
	Z();
	bool   ok = true;
	THROW(g > 0);
	// Check for overflow 
	THROW(g <= (LONG_MAX - JULIAN_SDN_OFFSET * 4 + 1) / 4);
	{
		uint   temp = g * 4 + (JULIAN_SDN_OFFSET * 4 - 1);
		// Calculate the year and day of year (1 <= dayOfYear <= 366)
		{
			const int yearl = temp / SlConst::DaysPer4Years;
			THROW(yearl >= INT_MIN && yearl <= INT_MAX);
			Y = yearl;
		}
		int    day_of_year = (temp % SlConst::DaysPer4Years) / 4 + 1;
		// Calculate the month and day of month
		temp = day_of_year * 5 - 3;
		M = temp / DAYS_PER_5_MONTHS;
		D = (temp % DAYS_PER_5_MONTHS) / 5 + 1;
	}
	// Convert to the normal beginning of the year
	if(M < 10) {
		M += 3;
	}
	else {
		Y += 1;
		M -= 9;
	}
	// Adjust to the B.C./A.D. type numbering
	Y -= 4800;
	if(Y <= 0)
		Y--;
	CATCHZOK
	return ok;	
}

/*static*/bool SUniDate_Internal::SdnToGregorian(long sdn, int * pYear, int * pMonth, int * pDay)
{
	bool   ok = true;
	int    century;
	int    year;
	int    month;
	int    day;
	long   temp;
	int    dayOfYear;
	THROW(sdn > 0 && sdn <= (LONG_MAX - 4 * GREGOR_SDN_OFFSET) / 4);
	temp = (sdn + GREGOR_SDN_OFFSET) * 4 - 1;
	// Calculate the century (year/100)
	century = temp / SlConst::DaysPer400Years;
	// Calculate the year and day of year (1 <= dayOfYear <= 366)
	temp = ((temp % SlConst::DaysPer400Years) / 4) * 4 + 3;
	year = (century * 100) + (temp / SlConst::DaysPer4Years);
	dayOfYear = (temp % SlConst::DaysPer4Years) / 4 + 1;
	// Calculate the month and day of month
	temp = dayOfYear * 5 - 3;
	month = temp / DAYS_PER_5_MONTHS;
	day = (temp % DAYS_PER_5_MONTHS) / 5 + 1;
	// Convert to the normal beginning of the year
	if(month < 10) {
		month += 3;
	}
	else {
		year += 1;
		month -= 9;
	}
	// Adjust to the B.C./A.D. type numbering
	year -= 4800;
	if(year <= 0)
		year--;
	*pYear = year;
	*pMonth = month;
	*pDay = day;
	CATCH
		*pYear = 0;
		*pMonth = 0;
		*pDay = 0;
		ok = false;
	ENDCATCH
	return ok;
}

/*static*/int SUniDate_Internal::GregorianToSdn(int inputYear, uint inputMonth, uint inputDay)
{
	int     result = 0;
	int     year;
	int     month;
	// check for invalid dates
	THROW(inputYear != 0 && inputYear >= -4714 && inputMonth >= 1 && inputMonth <= 12 && inputDay >= 1 && inputDay <= 31);
	// check for dates before SDN 1 (Nov 25, 4714 B.C.)
	if(inputYear == -4714) {
		THROW(inputMonth >= 11);
		THROW(!(inputMonth == 11 && inputDay < 25));
	}
	// Make year always a positive number
	year = (inputYear < 0) ? (inputYear + 4801) : (inputYear + 4800);
	// Adjust the start of the year
	if(inputMonth > 2) {
		month = inputMonth - 3;
	}
	else {
		month = inputMonth + 9;
		year--;
	}
	result = (((year / 100) * SlConst::DaysPer400Years) / 4 + ((year % 100) * SlConst::DaysPer4Years) / 4 + 
		(month * DAYS_PER_5_MONTHS + 2) / 5 + inputDay - GREGOR_SDN_OFFSET);
	CATCH
		result = 0;
	ENDCATCH
	return result;
}

//const char * const MonthNameShort[13] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//const char * const MonthNameLong[13] = { "", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
// 
// These are the externally visible components of this file:
// 
//   void SdnToJulian(long  sdn, int      *pYear, int      *pMonth, int      *pDay);
// 
// Convert a SDN to a Julian calendar date.  If the input SDN is less than
// 1, the three output values will all be set to zero, otherwise *pYear
// will be >= -4713 and != 0; *pMonth will be in the range 1 to 12
// inclusive; *pDay will be in the range 1 to 31 inclusive.
// 
//   int JulianToSdn(int inputYear, int inputMonth, int inputDay);
// 
// Convert a Julian calendar date to a SDN.  Zero is returned when the
// input date is detected as invalid or out of the supported range.  The
// return value will be > 0 for all valid, supported dates, but there are
// some invalid dates that will return a positive value.  To verify that a
// date is valid, convert it to SDN and then back and compare with the original.
// 
// VALID RANGE
// 
//   4713 B.C. to at least 10000 A.D.
// 
//   Although this software can handle dates all the way back to 4713
//   B.C., such use may not be meaningful.  The calendar was created in
//   46 B.C., but the details did not stabilize until at least 8 A.D.,
//   and perhaps as late at the 4th century.  Also, the beginning of a
//   year varied from one culture to another - not all accepted January
//   as the first month.
// 
// CALENDAR OVERVIEW
// 
//   Julius Caesar created the calendar in 46 B.C. as a modified form of
//   the old Roman republican calendar which was based on lunar cycles.
//   The new Julian calendar set fixed lengths for the months, abandoning
//   the lunar cycle.  It also specified that there would be exactly 12
//   months per year and 365.25 days per year with every 4th year being a leap year.
// 
//   Note that the current accepted value for the tropical year is
//   365.242199 days, not 365.25.  This lead to an 11 day shift in the
//   calendar with respect to the seasons by the 16th century when the
//   Gregorian calendar was created to replace the Julian calendar.
// 
//   The difference between the Julian and today's Gregorian calendar is
//   that the Gregorian does not make centennial years leap years unless
//   they are a multiple of 400, which leads to a year of 365.2425 days.
//   In other words, in the Gregorian calendar, 1700, 1800 and 1900 are
//   not leap years, but 2000 is.  All centennial years are leap years in the Julian calendar.
// 
//   The details are unknown, but the lengths of the months were adjusted
//   until they finally stablized in 8 A.D. with their current lengths:
// 
//   January          31
//   February         28/29
//   March            31
//   April            30
//   May              31
//   June             30
//   Quintilis/July   31
//   Sextilis/August  31
//   September        30
//   October          31
//   November         30
//   December         31
//   
//   In the early days of the calendar, the days of the month were not
//   numbered as we do today.  The numbers ran backwards (decreasing) and
//   were counted from the Ides (15th of the month - which in the old
//   Roman republican lunar calendar would have been the full moon) or
//   from the Nonae (9th day before the Ides) or from the beginning of the next month.
//   
//   In the early years, the beginning of the year varied, sometimes
//   based on the ascension of rulers.  It was not always the first of January.
//   
//   Also, today's epoch, 1 A.D. or the birth of Jesus Christ, did not
//   come into use until several centuries later when Christianity became a dominant religion.
//   
// ALGORITHMS
//   
//   The calculations are based on two different cycles: a 4 year cycle
//   of leap years and a 5 month cycle of month lengths.
//   
//   The 5 month cycle is used to account for the varying lengths of
//   months.  You will notice that the lengths alternate between 30 and
//   31 days, except for three anomalies: both July and August have 31
//   days, both December and January have 31, and February is less than
//   30.  Starting with March, the lengths are in a cycle of 5 months (31, 30, 31, 30, 31):
//   
//   Mar   31 days  \
//   Apr   30 days   |
//   May   31 days    > First cycle
//   Jun   30 days   |
//   Jul   31 days  /
//   
//   Aug   31 days  \
//   Sep   30 days   |
//   Oct   31 days    > Second cycle
//   Nov   30 days   |
//   Dec   31 days  /
//   
//   Jan   31 days  \
//   Feb 28/9 days   |
//                    > Third cycle (incomplete)
//   
//   For this reason the calculations (internally) assume that the year starts with March 1.
//   
// REFERENCES
// 
//   Conversions Between Calendar Date and Julian Day Number by Robert J.
//   Tantzen, Communications of the Association for Computing Machinery
//   August 1963.  (Also published in Collected Algorithms from CACM,
//   algorithm number 199).  [Note: the published algorithm is for the
//   Gregorian calendar, but was adjusted to use the Julian calendar's
//   simpler leap year rule.]
// 
/*static*/bool SUniDate_Internal::SdnToJulian(long sdn, int * pYear, int * pMonth, int * pDay)
{
	bool   ok = true;
	int    year;
	int    month;
	int    day;
	// Check for overflow 
	THROW(sdn > 0 && sdn <= (LONG_MAX - JULIAN_SDN_OFFSET * 4 + 1) / 4);
	{
		long   temp = sdn * 4 + (JULIAN_SDN_OFFSET * 4 - 1);
		// Calculate the year and day of year (1 <= dayOfYear <= 366)
		{
			const long yearl = temp / SlConst::DaysPer4Years;
			THROW(yearl >= INT_MIN && yearl <= INT_MAX);
			year = (int)yearl;
		}
		int    dayOfYear = (temp % SlConst::DaysPer4Years) / 4 + 1;
		// Calculate the month and day of month
		temp = dayOfYear * 5 - 3;
		month = temp / DAYS_PER_5_MONTHS;
		day = (temp % DAYS_PER_5_MONTHS) / 5 + 1;
	}
	// Convert to the normal beginning of the year
	if(month < 10) {
		month += 3;
	}
	else {
		year += 1;
		month -= 9;
	}
	// Adjust to the B.C./A.D. type numbering
	year -= 4800;
	if(year <= 0)
		year--;
	*pYear = year;
	*pMonth = month;
	*pDay = day;
	CATCH
		*pYear = 0;
		*pMonth = 0;
		*pDay = 0;
		ok = false;
	ENDCATCH
	return ok;
}

/*static*/int SUniDate_Internal::JulianToSdn(int inputYear, int inputMonth, int inputDay)
{
	int  result = 0;
	int  year;
	int  month;
	// check for invalid dates
	THROW(inputYear != 0 && inputYear >= -4713 && inputMonth >= 1 && inputMonth <= 12 && inputDay >= 1 && inputDay <= 31);
	// check for dates before SDN 1 (Jan 2, 4713 B.C.)
	THROW(!(inputYear == -4713 && inputMonth == 1 && inputDay == 1));
	// Make year always a positive number
	year = (inputYear < 0) ? (inputYear + 4801) : (inputYear + 4800);
	// Adjust the start of the year
	if(inputMonth > 2) {
		month = inputMonth - 3;
	}
	else {
		month = inputMonth + 9;
		year--;
	}
	result = ((year * SlConst::DaysPer4Years) / 4 + (month * DAYS_PER_5_MONTHS + 2) / 5 + inputDay - JULIAN_SDN_OFFSET);
	CATCH
		result = 0;
	ENDCATCH
	return result;
}

/*static*/int SUniDate_Internal::SdnDayOfWeek(long sdn) // sunday - 0
{
	int dow = (sdn + 1) % 7;
	return (dow >= 0) ? dow : (dow + 7);
}
