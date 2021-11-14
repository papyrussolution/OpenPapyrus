// STR2DATE.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2006, 2008, 2010, 2013, 2015, 2016, 2017, 2018, 2020, 2021
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

int FASTCALL _decode_date_fmt(int style, int * pDiv);

static int * FASTCALL getnmb(int cnt, int ord, int * d, int * m, int * y)
{
	switch(cnt) {
		case 0: return (ord == 0) ? m : ((ord == 1) ? d : y);
		case 1: return ord ? m : d;
		case 2:
		default: return (ord == 2) ? d : y;
	}
}

int STDCALL _strtodate(const char * pBuf, int style, int * pDay, int * pMon, int * pYear, long * pRetFlags)
{
	// DATF_DMY

	//
	// @          - текущий день
	// @-2        - текущий день минус два
	// ^10        - если день месяца менее 10, то начало предыдущего месяца, иначе - начало текущего.
	// 11/9/@     - 11/9 текущего года
	// @/@/@-1    - текущая дата год назад
	// @/@-2/2007 - текущий день 2007 года минус два месяца
	//
	// @construction {
	// #w - первый день текущей недели
	// #m - первый день текущего месяца
	// #q - первый день текущего квартала
	// #y - первый день текущего года
	// } @construction
	//
	const  char * c = pBuf;
	char   tmp[32];
	char   zero_buf[32];
	int    i, cnt = 0, div;
	int    ord; // 0 - mm.dd.yy, 1 - dd.mm.yy, 2 - yy.mm.dd
	int    not_empty_year = 0;
	int    d = 0;
	int    m = 0;
	int    y = 0;
	int    plus_d = 0;
	int    plus_m = 0;
	int    plus_y = 0;
	int    is_first_subst = 0;
	long   ret_flags = 0;

	if(c == 0) {
		PTR32(zero_buf)[0] = 0;
		c = zero_buf;
		ret_flags |= strtodatefZero;
	}
	while(oneof2(*c, ' ', '\t'))
		c++;
	if(strnicmp(c, "date", 4) == 0) {
		c += 4;
		while(oneof2(*c, ' ', '\t'))
			c++;
		if(*c == '\'')
			c++;
	}
	if(*c) {
		if(*c == '^') {
            c++;
            i = 0;
			if(isdec(*c)) {
				tmp[i++] = *c++;
				if(isdec(*c))
					tmp[i++] = *c++;
			}
			tmp[i] = 0;
			i = atoi(tmp);
			if(i >= 1 && i <= 31) {
				d = MakeLong(i, 0x2000);
                m = -1;
                y = -1;
				ret_flags |= strtodatefThrsMDay;
			}
		}
		else {
			//
			// Препроцессинг с целью выяснить не является ли строка датой без разделителей.
			// Такая дата представляет из себя строку из шести символов
			// без всяких пробелов и разделителей.
			//
			int    dig_count = 0;
			int    year_start = 0;
			int    year_end = 0;
			while(isdec(c[dig_count]))
				dig_count++;
			if(dig_count == 6)
				not_empty_year = 1;
			else if(dig_count == 8) {
				not_empty_year = 1;

				year_start = ((c[0]) - '0') * 1000;
				year_start += ((c[1]) - '0') * 100;
				year_start += ((c[2]) - '0') * 10;
				year_start += ((c[3]) - '0');

				year_end = ((c[4]) - '0') * 1000;
				year_end += ((c[5]) - '0') * 100;
				year_end += ((c[6]) - '0') * 10;
				year_end += ((c[7]) - '0');

				if(year_start < 1900 || year_start > 2100)
					year_start = 0;
				if(year_end < 1900 || year_end > 2100)
					year_end = 0;
			}
			else {
				//
				// Попытка с ходу засечь дату в формате DATF_ISO8601 (yyyy-mm-dd).
				// Так как по счастливой случайности этот формат не перекрывается никакими иными DATF_XXX
				// форматами, мы может рассмотреть этот вариант отдельно.
				// За одно воспользуемся тем, что количество лидирующих цифр уже подсчитано (dig_count == 4)
				//
				if(dig_count == 4 && style != DATF_ISO8601) {
					if(c[4] == '-' && isdec(c[5]) && isdec(c[6]) && c[7] == '-' && isdec(c[8]) && isdec(c[9])) {
						style = DATF_ISO8601;
					}
				}
				dig_count = 0;
			}
			//
			ord = _decode_date_fmt(style, &div);
			if(year_start) {
				if(!year_end)
					ord = 2; // YMD
				else {
					//
					// Если и префикс и суффикс строки может быть трактован как год, то
					// считаем годом то, что ближе к текущей дате.
					//
					const LDATE cdate = getcurdate_();
					const int  _cy = cdate.year();
					if(abs(_cy - year_start) < abs(_cy - year_end)) // @v10.3.1 @fix (abs(_cy - year_end) < abs(_cy - year_end))-->(abs(_cy - year_start) < abs(_cy - year_end))
						ord = 2; // YMD
				}
			}
			for(cnt = 0; cnt < 3; cnt++) {
				int * p_cur_pos = getnmb(cnt, ord, &d, &m, &y);
				if(dig_count) {
					if(dig_count == 8 && p_cur_pos == &y) {
						*p_cur_pos = ((*c++) - '0') * 1000;
						*p_cur_pos += ((*c++) - '0') * 100;
						*p_cur_pos += ((*c++) - '0') * 10;
						*p_cur_pos += ((*c++) - '0');
					}
					else {
						*p_cur_pos = ((*c++) - '0') * 10;
						*p_cur_pos += ((*c++) - '0');
					}
				}
				else {
					while(oneof2(*c, ' ', '\t'))
						c++;
					if(*c == '@') {
						is_first_subst = BIN(cnt == 0);
						c++;
						int * p_cur_pos_plus = getnmb(cnt, ord, &plus_d, &plus_m, &plus_y);
						*p_cur_pos = -1;
						if(oneof2(*c, '+', '-')) {
							int  sign = (*c++ == '-') ? -1 : +1;
							for(i = 0; isdec(*c);)
								tmp[i++] = *c++;
							tmp[i] = '\0';
							*p_cur_pos_plus = atoi(tmp) * sign;
						}
						*p_cur_pos = -1;
					}
					else if(*c == '?') {
						c++;
						*p_cur_pos = ANY_DATE_VALUE;
						if(p_cur_pos == &y)
							not_empty_year = 1;
					}
					else {
						for(i = 0; isdec(*c);)
							tmp[i++] = *c++;
						tmp[i] = '\0';
						*p_cur_pos = atoi(tmp);
						if(p_cur_pos == &y && tmp[0])
							not_empty_year = 1;
					}
					while(oneof2(*c, ' ', '\t'))
						c++;
					if(!(*c == '.' && c[1] != '.') && !oneof3(*c, '/', '-', '\\'))
						break;
					c++;
				}
			}
			if(y || m || d) {
				if(is_first_subst) {
					d = MakeLong(plus_d, 0x8000);
					y = -1;
					m = -1;
					ret_flags |= strtodatefRel;
				}
				else {
					if(y == -1) {
						y = MakeLong(plus_y, 0x8000);
						ret_flags |= strtodatefRelYear;
					}
					else if(y == ANY_DATE_VALUE) {
						ret_flags |= strtodatefAnyYear;
					}
					if(m == -1) {
						m = MakeLong(plus_m, 0x8000);
						ret_flags |= strtodatefRelMon;
					}
					else if(m == ANY_DATE_VALUE) {
						ret_flags |= strtodatefAnyMon;
					}
					if(d == -1) {
						d = MakeLong(plus_d, 0x8000);
						ret_flags |= strtodatefRelDay;
					}
					else if(d == ANY_DATE_VALUE) {
						ret_flags |= strtodatefAnyDay;
					}
				}
				if(!y) {
					if(!not_empty_year) {
						y = DefaultYear;
						ret_flags |= strtodatefDefYear;
					}
					else
						y = 2000;
				}
				if(!m) {
					m = DefaultMonth;
					ret_flags |= strtodatefDefMon;
				}
				if(!d)
					d = 1;
				if(y > 0 && y < 100) {
					if(y >= 70) // @v10.8.5 50-->70
						y += 1900;
					else
						y += 2000;
				}
				else if(y >= 200 && y <= 299)
					y = 2000 + (y - 200);
			}
		}
	}
	if(!_checkdate(d, m, y)) {
		d = 0;
		m = 0;
		y = 0;
		ret_flags |= strtodatefInvalid;
	}
	ASSIGN_PTR(pYear, y);
	ASSIGN_PTR(pMon,  m);
	ASSIGN_PTR(pDay,  d);
	ASSIGN_PTR(pRetFlags, ret_flags);
	return static_cast<int>(c - pBuf);
}

int STDCALL strtodate(const char * pBuf, long fmt, void * pDate)
{
	int    d, m, y;
	_strtodate(pBuf, SFMTFLAG(fmt), &d, &m, &y);
	_encodedate(d, m, y, pDate, BinDateFmt);
	return 1;
}

LDATE STDCALL strtodate_(const char * pBuf, long fmt)
{
	int    d, m, y;
	_strtodate(pBuf, SFMTFLAG(fmt), &d, &m, &y);
	return encodedate(d, m, y);
}

int STDCALL strtodatetime(const char * pBuf, LDATETIME * pDtm, long datFmt, long timFmt)
{
	pDtm->Z();
	const char * p = pBuf;
	if(p) {
		int    done = 0;
		char   dt_buf[128];
		char   tm_buf[128];
		while(oneof2(*p, ' ', '\t'))
			p++;
		if(!isdec(*p)) {
			time_t tt = Sl_Curl_GetDate(p);
			if(tt != -1) {
				pDtm->SetTimeT(tt);
				done = 1;
			}
		}
		if(!done) {
			const  char * p_div = sstrchr(p, ' ');
			PTR32(dt_buf)[0] = 0;
			PTR32(tm_buf)[0] = 0;
			SETIFZ(p_div, sstrchr(p, 'T'));
			SETIFZ(p_div, sstrchr(p, 't'));
			if(p_div) {
				size_t dp = 0;
				while(p != p_div && (dp+1) < sizeof(dt_buf))
					dt_buf[dp++] = *p++;
				dt_buf[dp] = 0;
				if(oneof2(p_div[1], 'T', 't'))
					STRNSCPY(tm_buf, p_div + 2);
				else
					STRNSCPY(tm_buf, p_div + 1);
			}
			else
				STRNSCPY(dt_buf, p);
			strtodate(dt_buf, datFmt, &pDtm->d);
			strtotime(tm_buf, timFmt, &pDtm->t);
		}
		return 1;
	}
	else
		return 0;
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

struct TempVar {
	int    D;
	int    M;
	int    Y;
};

static int FASTCALL gettoken(const char ** b, long * pNumber, TempVar * pV, long flags)
{
	//static const char * quarters[4] = { "I", "II", "III", "IV" };
	int    result = TOK_UNKNOWN;
	//int    i;
	char   buf[64];
	char * t = buf;
	while(oneof2(**b, ' ', '\t'))
		(*b)++;
	long   ret_flags = 0;
	int    offs = _strtodate(*b, DATF_DMY, &pV->D, &pV->M, &pV->Y, &ret_flags);
	if(ret_flags & (strtodatefAnyYear|strtodatefAnyMon|strtodatefAnyDay) && !(flags & strtoprdfEnableAnySign)) {
		result = TOK_UNKNOWN;
	}
	else if(offs > 0 && ((ret_flags & (strtodatefRelAny|strtodatefAnyYear|strtodatefAnyMon|strtodatefAnyDay)) || (ret_flags == 0))) { // @v9.8.3 || (ret_flags == 0)
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

int STDCALL strtoperiod(const char * pStr, DateRange * pRange, long flags)
{
	const  int _defyear  = DefaultYear;
	const  int _defmonth = DefaultMonth;
	const  char * p = pStr;
	int    beg = 0;    // Признак того, что взята дата начала периода
	int    dbldot = 0;
	int    d = 0;
	int    m = 0;
	int    y = 0;
	int    q = 0;
	TempVar temp_dt;
	int    nd;
	long   number = 0;
    pRange->Z();
	while(1) {
		switch(gettoken(&p, &number, &temp_dt, flags)) {
			case TOK_DATE:
				d = temp_dt.D;
				m = temp_dt.M;
				y = temp_dt.Y;
				break;
			case TOK_NUMBER:
				if(d == 0 && checkirange(number, 1, 31))
					d = number;
				else if(m == 0 && checkirange(number, 1, 12))
					m = number;
				else if(y == 0) {
					y = NZOR(number, 2000);
					if(y > 31 && m == 0 && d) {
						m = d;
						d = 0;
					}
					if(y < 100) {
						if(y >= 50)
							y += 1900;
						else
							y += 2000;
					}
					else if(checkirange(y, 200, 299))
						y = 2000 + (y - 200);
				}
				else
					return 0;
				break;
			case TOK_QUARTER:
				if(q == 0 || beg) {
					q = number + 1;
					if(beg == 0) {
						d = 1;
						m = (q - 1) * 3 + 1;
					}
					else {
						d = daysPerMonth[q * 3 - 1];
						m = q * 3;
					}
				}
				else
					return 0;
				break;
			case TOK_DELIM:
				if((d && m && y) || (!d && !m && !y))
					return 0;
				break;
			case TOK_DBLDOT:
				if(dbldot)
					return 0;
				else {
					if(!d && !m && !y)
						pRange->low = ZERODATE;
					else {
						SETIFZ(d, 1);
						SETIFZ(m, y ? 1 : _defmonth);
						SETIFZ(y, _defyear);
						encodedate(d, m, y, &pRange->low);
					}
					beg = 1;
					d = m = y = 0;
					dbldot = 1;
				}
				break;
			case TOK_EOL:
				if(beg == 0) {
					if(q) {
						encodedate(d, m, y ? y : _defyear, &pRange->low);
						encodedate(daysPerMonth[q * 3 - 1], q * 3, y ? y : _defyear, &pRange->upp);
					}
					else if(!d && !m && !y)
						pRange->Z();
					else if(y && !m && !d) {
						encodedate(1, 1, y, &pRange->low);
						encodedate(31, 12, y, &pRange->upp);
					}
					else if(y && m && !d) {
						encodedate(1, m, y, &pRange->low);
						nd = dayspermonth(m, y);
						encodedate(nd, m, y, &pRange->upp);
					}
					else {
						encodedate(NZOR(d, 1), NZOR(m, (y ? 1 : _defmonth)), NZOR(y, _defyear), &pRange->low);
						pRange->upp = pRange->low;
					}
				}
				else if(beg == 1) {
					if(!d && !m && !y)
						pRange->upp = ZERODATE;
					else {
						int _d, _m, _y;
						decodedate(&_d, &_m, &_y, &pRange->low);
						SETIFZ(m, y ? 12 : NZOR(_m, _defmonth));
						y = NZOR(y, NZOR(_y, _defyear));
						SETIFZ(d, dayspermonth(m, y));
						encodedate(d, m, y, &pRange->upp);
					}
				}
				return 1;
			case TOK_UNKNOWN:
				return 0;
		}
	}
}
