// SFORMAT.CPP
// Copyright (c) A.Sobolev 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

/*
struct i_tbl {
	int  a;
	int  b;
};

static int SLAPI _ITab(const void * tbl, int req, int def)
{
	const i_tbl * t = (const i_tbl *)tbl;
	for(int i = 1; i <= t[0].a; i++)
		if(t[i].a == req)
			return t[i].b;
	return def;
}

static int FASTCALL _AiTab(const void * tbl, int req)
{
	const i_tbl * t = (const i_tbl *)tbl;
	for(int i = 1; i <= t[0].a; i++)
		if(t[i].a & req)
			return t[i].b;
	return 0;
}
*/

SLAPI SFormatParam::SFormatParam()
{
	THISZERO();
	FDate = DATF_DMY;
	FTime = TIMF_HMS;
}
//
// Common formatting
//
/*static const struct {
	int    a;
	int    b;
} adjTbl[4] = {
	{ 3, 0 },
	{ ALIGN_LEFT, ADJ_LEFT },
	{ ALIGN_CENTER, ADJ_CENTER },
	{ ALIGN_RIGHT, ADJ_RIGHT }
};*/

char * FASTCALL _commfmt(long fmt, char * pBuf)
{
	const size_t len = SFMTLEN(fmt);
	if(len > 0) {
		const size_t src_len = strlen(pBuf);
		if(src_len > len) {
			if(SFMTFLAG(fmt) & COMF_FILLOVF)
				memset(pBuf, DEFAULT_OVERFLOW_SYMB, len);
			pBuf[len] = '\0';
		}
		else if(src_len < len) {
			const uint _fadj = (uint)SFMTALIGN(fmt);
			int    _adj = ADJ_RIGHT;
			if(_fadj == ALIGN_LEFT)
				_adj = ADJ_LEFT;
			else if(_fadj == ALIGN_RIGHT)
				_adj = ADJ_RIGHT;
			else if(_fadj == ALIGN_CENTER)
				_adj = ADJ_CENTER;
			alignstr(pBuf, len, _adj);
		}
	}
	return pBuf;
}

SString & FASTCALL _commfmt(long fmt, SString & rBuf)
{
	const size_t len = SFMTLEN(fmt);
	if(len > 0) {
		const size_t src_len = rBuf.Len();
		if(src_len > len) {
			if(SFMTFLAG(fmt) & COMF_FILLOVF)
				rBuf.Z().CatCharN(DEFAULT_OVERFLOW_SYMB, len);
			rBuf.Trim(len);
		}
		else if(src_len < len) {
			const uint _fadj = (uint)SFMTALIGN(fmt);
			int    _adj = ADJ_RIGHT;
			if(_fadj == ALIGN_LEFT)
				_adj = ADJ_LEFT;
			else if(_fadj == ALIGN_RIGHT)
				_adj = ADJ_RIGHT;
			else if(_fadj == ALIGN_CENTER)
				_adj = ADJ_CENTER;
			rBuf.Align(len, _adj);
		}
	}
	return rBuf;
}
//
//
//
char * SLAPI strfmt(const char * str, long fmt, char * buf)
{
	char * p_org_buf = buf;
	int    flag = SFMTFLAG(fmt);
	if(str[0] == 0) {
		if(flag & COMF_SQL) {
			buf[0] = '\'';
			buf[1] = ' ';
			buf[2] = '\'';
			buf[3] = 0;
		}
		else
			buf[0] = 0;
	}
	else {
		char   temp_buf[4096]; // @v8.3.11 [1024]-->[4096]
		int    use_temp_buf = 0;
		if(flag & COMF_SQL) {
			if(buf == str) {
				buf = temp_buf;
				use_temp_buf = 1;
			}
			*buf++ = '\'';
		}
		if(flag & STRF_OEM)
			CharToOem(str, buf);
		else if(flag & STRF_ANSI)
			OemToChar(str, buf);
		else if(buf != str)
			strcpy(buf, str);
		if(flag & STRF_UPPER)
			strupr866(buf);
		else if(flag & STRF_LOWER)
			strlwr866(buf);
		if(flag & STRF_PASSWORD)
			strset(buf, DEFAULT_PASSWORD_SYMB);
		if(flag & COMF_SQL) {
			const size_t len = strlen(buf);
			buf[len] = '\'';
			buf[len+1] = 0;
		}
		if(use_temp_buf)
			strcpy(p_org_buf, temp_buf);
	}
	return _commfmt(fmt, p_org_buf);
}
//
// Date formatting
//
int FASTCALL _decode_date_fmt(int style, int * pDiv);

char * SLAPI _datefmt(int day, int mon, int year, int style, char * pBuf)
{
	int    div;
	int    ord;		/* 0 - mm.dd.yy, 1 - dd.mm.yy, 2 - yy.mm.dd */
	int    yr = (style & DATF_CENTURY) ? year : year % 100;
	if(style == DATF_SQL) {
		if(day == 0 && mon == 0 && year == 0) {
			year = 1900;
			mon = 1;
			day = 1;
		}
		sprintf(pBuf, "DATE '%04d-%02d-%02d'", year, mon, day);
	}
	else if(style == DATF_INTERNET) {
		//#define DATF_INTERNET      13  // Wed, 27 Feb 2008
		LDATE _dt = encodedate(day, mon, year);
		int   dow = dayofweek(&_dt, 1); // @v9.7.10 0-->1 в связи с вводом глобальной функции STextConst::Get
		//const char * p_dow_txt[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
		//const char * p_mon_txt[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Seb", "Oct", "Nov", "Dec"};
		//strcat(pBuf, p_dow_txt[dow]);
		//sprintf(pBuf, "%s, %d %s %d", p_dow_txt[dow%7], day, p_mon_txt[(mon >= 12) ? 11 : ((mon < 1) ? 0 : (mon-1))], year);
		if(dow < 1 || dow > 7)
			dow = 1;
		sprintf(pBuf, "%s, %d %s %d", STextConst::Get(STextConst::cDow_En_Sh, dow-1), day,
			STextConst::Get(STextConst::cMon_En_Sh, (mon >= 12) ? 11 : ((mon < 1) ? 0 : (mon-1))), year);
	}
	else if(day == 0 && mon == 0 && year == 0)
		pBuf[0] = 0;
	else {
		int    d_ = 0, m_ = 0, y_ = 0;
		int    shift;
		char   sd[32], sm[32], sy[32];
		sd[0] = sm[0] = sy[0] = 0;
		if(day & 0x20000000) {
			shift = (int)(int16)LoWord(day);
			if(mon == -1 && year == -1) {
				size_t p = 0;
				pBuf[p++] = '^';
				ltoa(shift, pBuf+p, 10);
				return pBuf;
			}
			else {
				size_t p = 0;
				sd[p++] = '^';
				ltoa(shift, sd+p, 10);
			}
		}
		if(day & 0x80000000) {
			shift = (int)(int16)LoWord(day);
			if(mon == -1 && year == -1) {
				size_t p = 0;
				pBuf[p++] = '@';
				if(shift > 0) {
					pBuf[p++] = '+';
					ltoa(shift, pBuf+p, 10);
				}
				else if(shift < 0)
					ltoa(shift, pBuf+p, 10);
				else
					pBuf[p] = 0;
				return pBuf;
			}
			else {
				size_t p = 0;
				sd[p++] = '@';
				if(shift < 0)
					ltoa(shift, sd+p, 10);
				else if(shift > 0) {
					sd[p++] = '+';
					ltoa(shift, sd+p, 10);
				}
				else
					sd[p] = 0;
			}
		}
		else
			sprintf(sd, "%02d", day);
		if(mon & 0x80000000) {
			shift = (int)(int16)LoWord(mon);
			size_t p = 0;
			sm[p++] = '@';
			if(shift < 0)
				ltoa(shift, sm+p, 10);
			else if(shift > 0) {
				sm[p++] = '+';
				ltoa(shift, sm+p, 10);
			}
			else
				sm[p] = 0;
		}
		else
			sprintf(sm, "%02d", mon);
		if(year & 0x80000000) {
			shift = (int)(int16)LoWord(year);
			size_t p = 0;
			sy[p++] = '@';
			if(shift < 0)
				ltoa(shift, sy+p, 10);
			else if(shift > 0) {
				sy[p++] = '+';
				ltoa(shift, sy+p, 10);
			}
			else
				sy[p] = 0;
		}
		else
			sprintf(sy, "%02d", (style & DATF_CENTURY) ? year : year % 100);
		ord = _decode_date_fmt(style, &div);
		if(ord == 0)
			memswap(sd, sm, sizeof(sd));
		else if(ord == 2)
			memswap(sd, sy, sizeof(sd));
		{
			size_t p = 0;
			size_t len = strlen(sd);
			memcpy(pBuf+p, sd, len);
			p += len;
			if(!(style & DATF_NODIV))
				pBuf[p++] = div;
			len = strlen(sm);
			memcpy(pBuf+p, sm, len);
			p += len;
			if(!(style & DATF_NODIV))
				pBuf[p++] = div;
			len = strlen(sy);
			memcpy(pBuf+p, sy, len);
			p += len;
			pBuf[p++] = 0;
		}
		/*
		ord = _decode_date_fmt(style, &div);
		if(ord == 0)
			Exchange((long *)&day, (long *)&mon);
		else if(ord == 2)
			Exchange((long *)&day, (long *)&yr);
		sprintf(pBuf, "%02d%c%02d%c%02d", day, div, mon, div, yr);
		*/
	}
	return pBuf;
}

char * SLAPI datefmt(const void * binDate, long fmt, char * txtDate)
{
	int    d, m, y;
	uint   flag = SFMTFLAG(fmt);
	_decodedate(&d, &m, &y, binDate, BinDateFmt);
	if(fmt & COMF_SQL)
		flag = DATF_SQL;
	else if((flag & 0xf) == 0)
		flag |= TxtDateFmt;
	return _commfmt(fmt, _datefmt(d, m, y, flag, txtDate));
}

char * SLAPI periodfmt(const DateRange * pPeriod, char * pBuf)
{
	char * p = pBuf;
	LDATE  beg, end;
	if(pPeriod) {
		beg = pPeriod->low;
		end = pPeriod->upp;
	}
	else
		beg = end = ZERODATE;
	if(beg)
		p += strlen(datefmt(&beg, DATF_DMY|DATF_CENTURY, p));
	if(beg != end) {
		*p++ = '.';
		*p++ = '.';
		if(end)
			p += strlen(datefmt(&end, DATF_DMY|DATF_CENTURY, p));
	}
	*p = 0;
	return pBuf;
}

int SLAPI periodfmtex(const DateRange * pPeriod, Pchar pBuf, size_t bufLen)
{
	static const char * quart[4] = { "I", "II", "III", "IV" };
	int  r = -1;
	int  d1, m1, y1, d2, m2, y2;
	char period[64];
	char * p = period;
	decodedate(&d1, &m1, &y1, &pPeriod->low);
	decodedate(&d2, &m2, &y2, &pPeriod->upp);
	if(d1 == 1 && d2 == dayspermonth(m2, y2)) {
		if(m1 == 1 && m2 == 12) {
			if(y1 == y2)
				itoa(y1, p, 10);
			else if(y1 == 0) {
				*p++ = '.';
				*p++ = '.';
				itoa(y2, p, 10);
			}
			else if(y2 == 0) {
				p += strlen(itoa(y1, p, 10));
				*p++ = '.';
				*p++ = '.';
				*p++ = 0;
			}
			else {
				p += strlen(itoa(y1, p, 10));
				*p++ = '.';
				*p++ = '.';
				itoa(y2, p, 10);
			}
			r = PRD_ANNUAL;
		}
		else if(((m1-1) % 3) == 0 && (m2 % 3) == 0){
			if(y1 == y2 && abs(m1-m2) < 3)
				sprintf(period, "%s/%d", quart[(m1-1)/3], y1);
			else if(y1 == 0)
				sprintf(period, "..%s/%d", quart[m2/3-1], y2);
			else if(y2 == 0)
				sprintf(period, "%s/%d..", quart[(m1-1)/3], y1);
			else
				sprintf(period, "%s/%d..%s/%d", quart[(m1-1)/3], y1, quart[m2/3-1], y2);
			r = PRD_QUART;
		}
		else {
			if(y1 == y2 && m1 == m2)
				sprintf(period, "%d/%d", m1, y1);
			else if(y1 == 0)
				sprintf(period, "..%d/%d", m2, y2);
			else if(y2 == 0)
				sprintf(period, "%d/%d..", m1, y1);
			else
				sprintf(period, "%d/%d..%d/%d", m1, y1, m2, y2);
			r = PRD_MONTH;
		}
	}
	else {
		periodfmt(pPeriod, period);
		r = PRD_DAY;
	}
	strnzcpy(pBuf, period, bufLen);
	return r;
}
//
// Time formatting
//
char * SLAPI timefmt(LTIME t, long fmt, char * pBuf)
{
	char   fs[64];
	if(t == 0 && (fmt & TIMF_BLANK))
		pBuf[0] = 0;
	else if(fmt & COMF_SQL) {
		sprintf(pBuf, "TIMESTAMP '%04d-%02d-%02d %02d:%02d:%02d.%02d'", 2000, 1, 1, t.hour(), t.minut(), t.sec(), t.hs());
	}
	else {
		const int _no_div = BIN(fmt & TIMF_NODIV);
		if(_no_div)
			strcpy(fs, "%02d%02d%02d");
		else if(fmt & TIMF_DOTDIV)
			strcpy(fs, "%02d.%02d.%02d");
		else
			strcpy(fs, "%02d:%02d:%02d");
		switch(fmt & 7) {
			case 2: // TIMF_HM
				fs[_no_div ? 8 : 9] = 0;
				sprintf(pBuf, fs, t.hour(), t.minut());
				break;
			case 3: // TIMF_MS
				fs[_no_div ? 8 : 9] = 0;
				sprintf(pBuf, fs, t.minut(), t.sec());
				break;
			case 4: // TIMF_S
				fs[4] = 0;
				sprintf(pBuf, fs, t.sec());
				break;
			default: // include 1
				sprintf(pBuf, fs, t.hour(), t.minut(), t.sec());
				break;
		}
		if(fmt & TIMF_MSEC)
			sprintf(pBuf + strlen(pBuf), ".%03d", t.hs()*10);
		if(fmt & TIMF_TIMEZONE) {
			int    tz = gettimezone();
			char * p = pBuf + strlen(pBuf);
			*p++ = ' ';
			*p++ = ((tz < 0) ? '+' : '-');
			tz = abs(tz);
			sprintf(p, "%02d%02d", tz / 60, tz % 60);
		}
	}
	return _commfmt(fmt, pBuf);
}

char * SLAPI datetimefmt(LDATETIME dtm, long dtfmt, long tmfmt, char * pBuf, size_t bufLen)
{
	int    df = SFMTFLAG(dtfmt);
	int    tf = SFMTFLAG(tmfmt);
	char   temp_buf[256];
	if(df == DATF_SQL || tf == TIMF_SQL) {
		//const char * p_format = "TIMESTAMP '%04d-%02d-%02d %02d:%02d:%02d.%02d'";
		const char * p_format = "%02d:%02d:%04d %02d:%02d:%02d.%02d";
		sprintf(temp_buf, p_format,
			dtm.d.month(), dtm.d.day(), dtm.d.year(), dtm.t.hour(), dtm.t.minut(), dtm.t.sec(), dtm.t.hs());
	}
	else {
		char * p = temp_buf;
		datefmt(&dtm.d, dtfmt, p);
		p += strlen(p);
		*p++ = ' ';
		timefmt(dtm.t, tmfmt, p);
	}
	return strnzcpy(pBuf, temp_buf, bufLen);
}

int SLAPI strtotime(const char * pBuf, long fmt, void * v)
{
	int    i = 0;
	long   sDIV = 0x003B3A20L; /* " :;" */
	char   b[64];
	char * p = strtok(strip(strcpy(b, pBuf)), (char*)&sDIV);
	*(long *)v = 0L;
	if(p)
		do {
			((char *)v)[3-i] = atoi(p);
		} while(++i < 4 && (p = strtok(NULL, (char*)&sDIV)) != 0);
	return 1;
}
//
//
//
// @v9.4.3 #pragma option -Oi

/* @v9.4.3
static const struct _SFDelimEntry {
	int    a;
	int    b;
} delimTbl[4] = {
	{ 3, 0 },
	{ NMBF_TRICOMMA, ' '}, // { NMBF_TRICOMMA, ','},
	{ NMBF_TRIAPOSTR, '\'' },
	{ NMBF_TRISPACE, ' ' }
};
*/

static int FASTCALL is_zero(const char * c)
{
	while(*c)
		if(*c != '0' && *c != ' ')
			return 0;
		else
			c++;
	return 1;
}

static char * SLAPI fmtnumber(const char * ptr, int dec, int sign, long fmt, char * buf)
{
	int16  prec = (int16)(signed char)SFMTPRC(fmt);
	uint16 flags = SFMTFLAG(fmt);
	char * b = buf;
	if(!(flags & NMBF_NOZERO) || !is_zero(ptr)) {
		if(sign) {
			if(flags & NMBF_NONEG) {
				*b = '\0';
				return buf;
			}
			else
				*b++ = (flags & NMBF_NEGPAR) ? '(' : '-';
		}
		else if(flags & NMBF_FORCEPOS)
			*b++ = '+';
		//delim = (char)_AiTab(delimTbl, flags);
		char   delim = 0;
		if(flags & NMBF_TRIAPOSTR)
			delim = '\'';
		else if(flags & (NMBF_TRICOMMA|NMBF_TRISPACE))
			delim = ' ';
		//
		if(dec-- > 0)
			*b++ = *ptr++;
		while(dec > 0) {
			if(delim && (dec % 3) == 0)
				*b++ = delim;
			*b++ = *ptr++;
			dec--;
		}
		if(prec > 0) {
			*b++ = (flags & NMBF_DECCOMMA) ? ',' : '.';
			b = stpcpy(b, ptr);
			if(flags & NMBF_NOTRAILZ) {
				while(*(b-1) == '0') {
					if(flags & NMBF_EXPLFLOAT && *(b-2) == '.') { // @v9.7.8
						break;
					}
					else
						*--b = 0;
				}
				if(*(b-1) == '.')
					*--b = 0;
			}
		}
		if(sign && (flags & NMBF_NEGPAR))
			*b++ = ')';
	}
	*b = '\0';
	return _commfmt(fmt, buf);
}

char * SLAPI decfmt(BCD_T val, int len, int prec, long fmt, char * pBuf)
{
	char   str[64];
	char * c, *s = str;
	int    sign;
	dectostr(val, (int16)len, (int16)prec, s);
	if(*s == '-')
		s++, sign = 1;
	else
		sign = 0;
	c = strchr(s, '.');
	if(c)
		strcpy(c, c + 1);
	return fmtnumber(s, strlen(s) - prec, sign, fmt, pBuf);
}

char * SLAPI realfmt(double val, long fmt, char * pBuf)
{
	int    dec, sign;
	int    prc = (int)(signed char)SFMTPRC(fmt);
	if(prc < 0) {
		val = val * fpow10i(prc);
		prc = 0;
	}
	else if(prc > 0) { // Papyrus @v3.6.6
		val = round(val, prc);
	}
	if(val > -1.e-10 && val < 1.e-10)
		val = 0.0;
	{
		char   str[256];
		if(!prc && val == 0.0) {
			strnzcpy(str, fcvt(val, 1, &dec, &sign), sizeof(str));
			dec = 1;
		}
		else
			strnzcpy(str, fcvt(val, prc, &dec, &sign), sizeof(str));
		if(prc > 0 && dec <= 0) {
			padleft(str, '0', 1 - dec);
			dec = 1;
		}
		SETSFMTPRC(fmt, prc);
		if(val != 0.0 && SFMTFLAG(fmt) & NMBF_OMITEPS) {
			const uint _len = strlen(str);
			uint   c_0 = 0; // количество '0'
			uint   c_9 = 0; // количество '9'
			uint   last_n09_pos = 0; // Позиция последней цифры, не являющейся '0' или '9'
			int    do_carry = 0; // Для варианта "999999999999" нужно будет добавить лидирующую '1'
			for(uint i = 0; i < _len; i++) {
				const char _d = str[i];
				if(_d == '0') {
					c_9 = 0;
					if(++c_0 >= OMITEPSDIGITS) {
						for(uint j = i+1; j < _len; j++) {
							str[j] = '0';
						}
						break;
					}
				}
				else if(_d == '9') {
					c_0 = 0;
					if(++c_9 >= OMITEPSDIGITS) {
						if(last_n09_pos == 0 && str[0] == '9') {
							do_carry = 1;
						}
						else
							str[last_n09_pos] += 1;
						for(uint j = last_n09_pos+1; j < _len; j++) {
							str[j] = '0';
						}
						break;
					}
				}
				else if(isdec(_d)) {
					c_0 = 0;
					c_9 = 0;
					last_n09_pos = i;
				}
				else
					break; // something went wrong!
			}
			if(do_carry) {
				padleft(str, '1', 1);
				dec++;
			}
		}
		return fmtnumber(str, dec, sign, fmt, pBuf);
	}
}

char * SLAPI intfmt(long val, long fmt, char * pBuf)
{
	char   s[64];
	ltoa(labs(val), s, 10);
	return fmtnumber(s, strlen(s), val < 0, fmt, pBuf);
}

char * SLAPI int64fmt(int64 val, long fmt, char * pBuf)
{
	char   s[64];
	_i64toa(_abs64(val), s, 10);
	return fmtnumber(s, strlen(s), val < 0, fmt, pBuf);
}

char * SLAPI uintfmt(ulong val, long fmt, char * pBuf)
{
	char   s[64];
	ultoa(val, s, 10);
	return fmtnumber(s, strlen(s), 0, fmt, pBuf);
}

char * SLAPI uint64fmt(uint64 val, long fmt, char * pBuf)
{
	char   s[64];
	_ui64toa(val, s, 10);
	return fmtnumber(s, strlen(s), 0, fmt, pBuf);
}

// @v9.4.3 #pragma warn -pia

static char * FASTCALL clearDelimiters(char * b)
{
	long   skipch = 0x00002720L; /* "' " */
	char * c = b;
	while(c = strpbrk(c, (const char *) &skipch))
		strcpy(c, c + 1);
	return b;
}

// @v9.4.3 #pragma warn +pia

int SLAPI strtodoub(const char * pBuf, double * pVal)
{
	char   temp[128]; // @v9.2.1 [64]-->[128]
	char * p = strchr(clearDelimiters(STRNSCPY(temp, pBuf)), '.');
	if(p == 0)
		if((p = strchr(temp, ',')) != 0)
			*p = '.';
	ASSIGN_PTR(pVal, atof(temp));
	return 1;
}

int SLAPI strtolong(const char * pBuf, long * pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, atol(clearDelimiters(STRNSCPY(temp, pBuf))));
	return 1;
}

int SLAPI strtoulong(const char * pBuf, ulong * pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, strtoul(clearDelimiters(STRNSCPY(temp, pBuf)), 0, 10));
	return 1;
}
