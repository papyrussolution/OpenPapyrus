// SFORMAT.CPP
// Copyright (c) A.Sobolev 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

int SFormat_TranslateFlagsToStringSet(long fmt, TYPEID typeId, StringSet & rSs)
{
	rSs.Z();
	int    ok = 1;
	const   int  bt = stbase(typeId);
	{
		if(SFMTFLAG(fmt) & ALIGN_RIGHT)
			rSs.add("align_right");
		if(SFMTFLAG(fmt) & ALIGN_LEFT)
			rSs.add("align_left");
		if(SFMTFLAG(fmt) & ALIGN_CENTER)
			rSs.add("align_center");
		if(SFMTFLAG(fmt) & COMF_FILLOVF)
			rSs.add("filloverflow");
		if(SFMTFLAG(fmt) & COMF_SQL)
			rSs.add("sql");
		switch(bt) {
			case BTS_STRING:
				{
					if(SFMTFLAG(fmt) & STRF_UPPER)
						rSs.add("uppercase");
					if(SFMTFLAG(fmt) & STRF_LOWER)
						rSs.add("lowercase");
					if(SFMTFLAG(fmt) & STRF_PASSWORD) {
						rSs.add("password");
					}
					if(SFMTFLAG(fmt) & STRF_OEM)
						rSs.add("oem");
					if(SFMTFLAG(fmt) & STRF_ANSI)
						rSs.add("ansi");
				}
				break;
			case BTS_INT:
			case BTS_INT64_:
				{
					if(SFMTFLAG(fmt) & INTF_BIN)
						rSs.add("binary");
					else if(SFMTFLAG(fmt) & INTF_OCT)
						rSs.add("octal");
					else if(SFMTFLAG(fmt) & INTF_HEX) {
						rSs.add("hex");
						if(SFMTFLAG(fmt) & INTF_UPPERCASE) {
							rSs.add("uppercase");
						}
					}
					//
					if(SFMTFLAG(fmt) & INTF_FORCEPOS)
						rSs.add("forceplus");
					if(SFMTFLAG(fmt) & INTF_NOZERO)
						rSs.add("nozero");
				}
				break;
			case BTS_REAL:
				{
					if(SFMTFLAG(fmt) & NMBF_NONEG)
						rSs.add("noneg");
					if(SFMTFLAG(fmt) & NMBF_NEGPAR)
						rSs.add("negpar");
					if(SFMTFLAG(fmt) & NMBF_FORCEPOS)
						rSs.add("forceplus");
					if(SFMTFLAG(fmt) & NMBF_NOZERO)
						rSs.add("nozero");
					if(SFMTFLAG(fmt) & NMBF_TRICOMMA)
						rSs.add("tricomma");
					if(SFMTFLAG(fmt) & NMBF_TRIAPOSTR)
						rSs.add("triapostr");
					if(SFMTFLAG(fmt) & NMBF_TRISPACE)
						rSs.add("trispace");
					if(SFMTFLAG(fmt) & NMBF_NOTRAILZ) {
						rSs.add("notrailz");
						if(SFMTFLAG(fmt) & NMBF_EXPLFLOAT)
							rSs.add("explfloat");
					}
					if(SFMTFLAG(fmt) & NMBF_DECCOMMA)
						rSs.add("deccomma");
					if(SFMTFLAG(fmt) & NMBF_OMITEPS)
						rSs.add("omiteps");
				}
				break;
			case BTS_DATE:
				{
					const uint df = (SFMTFLAG(fmt) & 0x0f);
					const char * p_df_symb = 0;
					switch(df) {
						case DATF_AMERICAN: p_df_symb = "american"; break;
						case DATF_ANSI:     p_df_symb = "ansi";     break;
						case DATF_BRITISH:  p_df_symb = "british";  break;
						case DATF_FRENCH:   p_df_symb = "french";   break;
						case DATF_GERMAN:   p_df_symb = "german";   break;
						case DATF_ITALIAN:  p_df_symb = "italian";  break;
						case DATF_JAPAN:    p_df_symb = "japan";    break;
						case DATF_USA:      p_df_symb = "usa";      break;
						case DATF_MDY:      p_df_symb = "mdy";      break;
						case DATF_DMY:      p_df_symb = "dmy";      break;
						case DATF_YMD:      p_df_symb = "ymd";      break;
						case DATF_SQL:      p_df_symb = "sql";      break;
						case DATF_INTERNET: p_df_symb = "internet"; break;
						case DATF_ISO8601:  p_df_symb = "iso8601";  break; // @default
					}
					if(p_df_symb)
						rSs.add(p_df_symb);
					if(SFMTFLAG(fmt) & DATF_CENTURY)
						rSs.add("century");
					if(SFMTFLAG(fmt) & DATF_NOZERO)
						rSs.add("nozero");
					if(SFMTFLAG(fmt) & DATF_NODIV)
						rSs.add("nodiv");
				}
				break;
			case BTS_TIME:
				{
					const uint tf = (SFMTFLAG(fmt) & 0x07);
					const char * p_tf_symb = 0;
					switch(tf) {
						case TIMF_HMS: p_tf_symb = "hms"; break; // @default
						case TIMF_HM:  p_tf_symb = "hm"; break;
						case TIMF_MS:  p_tf_symb = "ms"; break;
						case TIMF_S:   p_tf_symb = "s"; break;
						case TIMF_SQL: p_tf_symb = "sql"; break;
					}
					if(p_tf_symb)
						rSs.add(p_tf_symb);
					if(SFMTFLAG(fmt) & TIMF_MSEC)
						rSs.add("msec");
					if(SFMTFLAG(fmt) & TIMF_BLANK)
						rSs.add("nozero");
					if(SFMTFLAG(fmt) & TIMF_TIMEZONE)
						rSs.add("timezone");
					if(SFMTFLAG(fmt) & TIMF_NODIV)
						rSs.add("nodiv");
					if(SFMTFLAG(fmt) & TIMF_DOTDIV)
						rSs.add("dotdiv");
				}
				break;
			case BTS_POINT2:
				break;
			case BTS_BOOL:
				break;
			default:
				break;
		}
	}
	return ok;
}

long SFormat_TranslateFlagFromString(const char * pText, TYPEID typeId)
{
	long    flags = 0;
	long    date_style = 0;
	long    time_style = 0;
	int     int_base = 0;
	const   int  bt = stbase(typeId);
	if(sstreqi_ascii(pText, "align_center")) {
		flags = ALIGN_CENTER;
	}
	else if(sstreqi_ascii(pText, "align_left")) {
		flags = ALIGN_LEFT;
	}
	else if(sstreqi_ascii(pText, "align_right")) {
		flags = ALIGN_RIGHT;
	}
	else if(sstreqi_ascii(pText, "american")) {
		date_style = DATF_AMERICAN;
	}
	else if(sstreqi_ascii(pText, "ansi")) { // 2dup
		if(bt == BTS_DATE) {
			flags = DATF_ANSI;
		}
		else if(bt == BTS_STRING) {
			flags = STRF_ANSI;
		}
	}
	else if(sstreqi_ascii(pText, "binary")) {
		int_base = 2;
	}
	else if(sstreqi_ascii(pText, "british")) {
		date_style = DATF_BRITISH;
	}
	else if(sstreqi_ascii(pText, "century")) {
		flags = DATF_CENTURY;
	}
	else if(sstreqi_ascii(pText, "deccomma")) {
		flags = NMBF_DECCOMMA;
	}
	else if(sstreqi_ascii(pText, "dmy")) {
		date_style = DATF_DMY;
	}
	else if(sstreqi_ascii(pText, "dotdiv")) {
		flags = TIMF_DOTDIV;
	}
	else if(sstreqi_ascii(pText, "explfloat")) {
		flags = NMBF_EXPLFLOAT;
	}
	else if(sstreqi_ascii(pText, "filloverflow")) {
		flags = COMF_FILLOVF;
	}
	else if(sstreqi_ascii(pText, "forceplus")) { // 2dup
		if(bt == BTS_REAL)
			flags = NMBF_FORCEPOS;
		else if(oneof2(bt, BTS_INT, BTS_INT64_))
			flags = INTF_FORCEPOS;
	}
	else if(sstreqi_ascii(pText, "french")) {
		date_style = DATF_FRENCH;
	}
	else if(sstreqi_ascii(pText, "german")) {
		date_style = DATF_GERMAN;
	}
	else if(sstreqi_ascii(pText, "hex")) {
		int_base = 16;
	}
	else if(sstreqi_ascii(pText, "hm")) {
		time_style = TIMF_HM;
	}
	else if(sstreqi_ascii(pText, "hms")) {
		time_style = TIMF_HMS;
	}
	else if(sstreqi_ascii(pText, "internet")) {
		date_style = DATF_INTERNET;
	}
	else if(sstreqi_ascii(pText, "iso8601")) {
		date_style = DATF_ISO8601;
	}
	else if(sstreqi_ascii(pText, "italian")) {
		date_style = DATF_ITALIAN;
	}
	else if(sstreqi_ascii(pText, "japan")) {
		date_style = DATF_JAPAN;
	}
	else if(sstreqi_ascii(pText, "lowercase")) {
		flags = STRF_LOWER;
	}
	else if(sstreqi_ascii(pText, "mdy")) {
		date_style = DATF_MDY;
	}
	else if(sstreqi_ascii(pText, "ms")) {
		time_style = TIMF_MS;
	}
	else if(sstreqi_ascii(pText, "msec")) {
		flags = TIMF_MSEC;
	}
	else if(sstreqi_ascii(pText, "negpar")) {
		flags = NMBF_NEGPAR;
	}
	else if(sstreqi_ascii(pText, "nodiv")) { // 2dup
		flags = TIMF_NODIV;
	}
	else if(sstreqi_ascii(pText, "noneg")) {
		flags = NMBF_NONEG;
	}
	else if(sstreqi_ascii(pText, "notrailz")) {
		flags = NMBF_NOTRAILZ;
	}
	else if(sstreqi_ascii(pText, "nozero")) { // 4dup
		if(bt == BTS_REAL)
			flags = NMBF_NOZERO;
		else if(oneof2(bt, BTS_INT, BTS_INT64_))
			flags = INTF_NOZERO;
		else if(bt == BTS_DATE)
			flags = DATF_NOZERO;
		else if(bt == BTS_TIME)
			flags = TIMF_NOZERO;
	}
	else if(sstreqi_ascii(pText, "octal")) {
		int_base = 8;
	}
	else if(sstreqi_ascii(pText, "oem")) {
		flags = STRF_OEM;
	}
	else if(sstreqi_ascii(pText, "omiteps")) {
		flags = NMBF_OMITEPS;
	}
	else if(sstreqi_ascii(pText, "password")) {
		flags = STRF_PASSWORD;
	}
	else if(sstreqi_ascii(pText, "s")) {
		time_style = TIMF_S;
	}
	else if(sstreqi_ascii(pText, "sql")) { // 3dup
		if(bt == BTS_DATE)
			flags = DATF_SQL;
		else if(bt == BTS_TIME)
			flags = TIMF_SQL;
		else
			flags = COMF_SQL;
	}
	else if(sstreqi_ascii(pText, "timezone")) {
		flags = TIMF_TIMEZONE;
	}
	else if(sstreqi_ascii(pText, "triapostr")) {
		flags = NMBF_TRIAPOSTR;
	}
	else if(sstreqi_ascii(pText, "tricomma")) {
		flags = NMBF_TRICOMMA;
	}
	else if(sstreqi_ascii(pText, "trispace")) {
		flags = NMBF_TRISPACE;
	}
	else if(sstreqi_ascii(pText, "uppercase")) { // 2dup
		if(bt == BTS_STRING)
			flags = STRF_UPPER;
		else if(oneof2(bt, BTS_INT, BTS_INT64_))
			flags = INTF_UPPERCASE;
	}
	else if(sstreqi_ascii(pText, "usa")) {
		date_style = DATF_USA;
	}
	else if(sstreqi_ascii(pText, "ymd")) {
		date_style = DATF_YMD;
	}
	{
		uint  _ac = 0;
		if(date_style)
			_ac++;
		if(time_style)
			_ac++;
		if(int_base)
			_ac++;
		assert(_ac == 0 || _ac == 1);
		assert(_ac == 0 || flags == 0);
	}
	if(date_style)
		flags = date_style;
	else if(time_style)
		flags = time_style;
	else if(int_base == 2) {
		flags = INTF_BIN;
	}
	else if(int_base == 8) {
		flags = INTF_OCT;
	}
	else if(int_base == 16) {
		flags = INTF_HEX;
	}
	return flags;
}

SFormatParam::SFormatParam() : Flags(0), FDate(DATF_DMY), FTime(TIMF_HMS), FStr(0), FReal(0)
{
}
//
// Common formatting
//
char * FASTCALL _commfmt(long fmt, char * pBuf)
{
	const size_t len = SFMTLEN(fmt);
	if(len > 0) {
		const size_t src_len = sstrlen(pBuf);
		if(src_len > len) {
			if(SFMTFLAG(fmt) & COMF_FILLOVF)
				memset(pBuf, SlConst::DefaultOverflowSymb, len);
			pBuf[len] = '\0';
		}
		else if(src_len < len) {
			int    _adj;
			switch(static_cast<uint>(SFMTALIGN(fmt))) {
				case ALIGN_LEFT: _adj = ADJ_LEFT; break;
				case ALIGN_RIGHT: _adj = ADJ_RIGHT; break;
				case ALIGN_CENTER: _adj = ADJ_CENTER; break;
				default: _adj = ADJ_RIGHT; break;
			}
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
				rBuf.Z().CatCharN(SlConst::DefaultOverflowSymb, len);
			rBuf.Trim(len);
		}
		else if(src_len < len) {
			int    _adj;
			switch(static_cast<uint>(SFMTALIGN(fmt))) {
				case ALIGN_LEFT: _adj = ADJ_LEFT; break;
				case ALIGN_RIGHT: _adj = ADJ_RIGHT; break;
				case ALIGN_CENTER: _adj = ADJ_CENTER; break;
				default: _adj = ADJ_RIGHT;
			}
			rBuf.Align(len, _adj);
		}
	}
	return rBuf;
}
//
//
//
char * STDCALL strfmt(const char * str, long fmt, char * buf)
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
		char   temp_buf[4096];
		int    use_temp_buf = 0;
		if(flag & COMF_SQL) {
			if(buf == str) {
				buf = temp_buf;
				use_temp_buf = 1;
			}
			*buf++ = '\'';
		}
		if(flag & STRF_OEM) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			(r_temp_buf = str).Transf(CTRANSF_OUTER_TO_INNER).CopyTo(buf, 0);
		}
		else if(flag & STRF_ANSI) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			(r_temp_buf = str).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(buf, 0);
		}
		else if(buf != str)
			strcpy(buf, str);
		if(flag & STRF_UPPER)
			strupr866(buf);
		else if(flag & STRF_LOWER)
			strlwr866(buf);
		if(flag & STRF_PASSWORD)
			strset(buf, SlConst::DefaultPasswordSymb);
		if(flag & COMF_SQL) {
			const size_t len = sstrlen(buf);
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

char * STDCALL _datefmt(int day, int mon, int year, int style, char * pBuf)
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
			size_t len = sstrlen(sd);
			memcpy(pBuf+p, sd, len);
			p += len;
			if(!(style & DATF_NODIV))
				pBuf[p++] = div;
			len = sstrlen(sm);
			memcpy(pBuf+p, sm, len);
			p += len;
			if(!(style & DATF_NODIV))
				pBuf[p++] = div;
			len = sstrlen(sy);
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

char * STDCALL datefmt(const void * binDate, long fmt, char * txtDate)
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

char * FASTCALL periodfmt(const DateRange & rPeriod, char * pBuf)
{
	char * p = pBuf;
	const LDATE  beg = rPeriod.low;
	const LDATE  end = rPeriod.upp;
	if(beg)
		p += sstrlen(datefmt(&beg, DATF_DMY|DATF_CENTURY, p));
	if(beg != end) {
		*p++ = '.';
		*p++ = '.';
		if(end)
			p += sstrlen(datefmt(&end, DATF_DMY|DATF_CENTURY, p));
	}
	*p = 0;
	return pBuf;
}

#if 0 // @v12.3.7 (replaced with DateRange::ToStr) {
int STDCALL periodfmtex(const DateRange & rPeriod, char * pBuf, size_t bufLen)
{
	static const char * quart[4] = { "I", "II", "III", "IV" };
	int  r = -1;
	int  d1 = 0;
	int  m1 = 0;
	int  y1 = 0;
	int  d2 = 0;
	int  m2 = 0;
	int  y2 = 0;
	char period[64];
	char * p = period;
	decodedate(&d1, &m1, &y1, &rPeriod.low);
	decodedate(&d2, &m2, &y2, &rPeriod.upp);
	if(d1 == 1 && (m2 && d2 == dayspermonth(m2, y2))) {
		if(m1 == 1 && m2 == 12) {
			if(y1 == y2)
				itoa(y1, p, 10);
			else if(y1 == 0) {
				*p++ = '.';
				*p++ = '.';
				itoa(y2, p, 10);
			}
			else if(y2 == 0) {
				p += sstrlen(itoa(y1, p, 10));
				*p++ = '.';
				*p++ = '.';
				*p++ = 0;
			}
			else {
				p += sstrlen(itoa(y1, p, 10));
				*p++ = '.';
				*p++ = '.';
				itoa(y2, p, 10);
			}
			r = PRD_ANNUAL;
		}
		else if(((m1-1) % 3) == 0 && (m2 % 3) == 0) {
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
		periodfmt(rPeriod, period);
		r = PRD_DAY;
	}
	strnzcpy(pBuf, period, bufLen);
	return r;
}
#endif // } 0 @v12.3.7 (replaced with DateRange::ToStr)
//
//
//
SString & STDCALL TimeZoneFmt(int tz, int flags, SString & rBuf)
{
	if(!(flags & tzfmtConcat))
		rBuf.Z();
	if(flags & tzfmtCurrent)
		tz = gettimezone(); 
	if(tz >= -(11*60) && tz <= (14*60)) {
		if(flags & tzfmtSpace)
			rBuf.Space();
		if(tz == 0) {
			rBuf.Cat("UTC");
		}
		else {
			if(tz < 0) {
				rBuf.CatChar('+');
				tz = -tz;
			}
			else
				rBuf.CatChar('-');
			rBuf.CatLongZ(tz / 60, 2);
			if(flags & tzfmtColon)
				rBuf.Colon();
			rBuf.CatLongZ(tz % 60, 2);
		}
	}
	return rBuf;
}
//
// Time formatting
//
char * STDCALL timefmt(LTIME t, long fmt, char * pBuf)
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
			sprintf(pBuf + sstrlen(pBuf), ".%03d", t.hs()*10);
		if(fmt & TIMF_TIMEZONE) {
			int    tz = gettimezone();
			char * p = pBuf + sstrlen(pBuf);
			*p++ = ' ';
			*p++ = ((tz < 0) ? '+' : '-');
			tz = abs(tz);
			sprintf(p, "%02d%02d", tz / 60, tz % 60);
		}
	}
	return _commfmt(fmt, pBuf);
}

char * STDCALL datetimefmt(LDATETIME dtm, long dtfmt, long tmfmt, char * pBuf, size_t bufLen)
{
	const int df = SFMTFLAG(dtfmt);
	const int tf = SFMTFLAG(tmfmt);
	char   temp_buf[256];
	if(df == DATF_SQL || tf == TIMF_SQL) {
		//const char * p_format = "TIMESTAMP '%04d-%02d-%02d %02d:%02d:%02d.%02d'";
		const char * p_format = "%02d:%02d:%04d %02d:%02d:%02d.%02d";
		sprintf(temp_buf, p_format, dtm.d.month(), dtm.d.day(), dtm.d.year(), dtm.t.hour(), dtm.t.minut(), dtm.t.sec(), dtm.t.hs());
	}
	else {
		char * p = temp_buf;
		datefmt(&dtm.d, dtfmt, p);
		p += sstrlen(p);
		*p++ = ' ';
		timefmt(dtm.t, tmfmt, p);
	}
	return strnzcpy(pBuf, temp_buf, bufLen);
}

static int FASTCALL checkdeccount(const char * pBuf, uint decCount)
{
	for(uint i = 0; i < decCount; i++) {
		if(!isdec(pBuf[i]))
			return 0;
	}
	return 1;
}

int STDCALL strtotime(const char * pBuf, long fmt, LTIME * v)
{
	int    ok = 1;
	int    h = 0;
	int    m = 0;
	int    s = 0;
	int    ms = 0;
	if(!isempty(pBuf)) {
		while(oneof2(pBuf[0], ' ', '\t'))
			pBuf++;
		if(fmt & TIMF_NODIV) {
			switch(fmt & 7) {
				case TIMF_HMS:
					if(checkdeccount(pBuf, 6)) {
						h = static_cast<int>(_texttodec32(pBuf, 2));
						m = static_cast<int>(_texttodec32(pBuf+2, 2));
						s = static_cast<int>(_texttodec32(pBuf+4, 2));
					}
					else
						ok = 0;
					break;
				case TIMF_HM:
					if(checkdeccount(pBuf, 4)) {
						h = static_cast<int>(_texttodec32(pBuf, 2));
						m = static_cast<int>(_texttodec32(pBuf+2, 2));
					}
					else
						ok = 0;
					break;
				case TIMF_MS:
					if(checkdeccount(pBuf, 4)) {
						m = static_cast<int>(_texttodec32(pBuf, 2));
						s = static_cast<int>(_texttodec32(pBuf+2, 2));
					}
					else
						ok = 0;
					break;
				case TIMF_S:
					{
						uint   p = 0;
						if(isdec(pBuf[p])) {
							do {
								p++;
							} while(isdec(pBuf[p]));
							s = static_cast<int>(_texttodec32(pBuf, p));
						}
						else
							ok = 0;
					}
					break;
				default:
					ok = 0;
			}
		}
		else {
			uint   p = 0;
			if(isdec(pBuf[0])) {
				do { p++; } while(isdec(pBuf[p]));
				h = static_cast<int>(_texttodec32(pBuf, p));
				if(oneof3(pBuf[p], ':', ';', ' ')) {
					pBuf = pBuf+p+1;
					p = 0;
					if(isdec(pBuf[0])) {
						do { p++; } while(isdec(pBuf[p]));
						m = static_cast<int>(_texttodec32(pBuf, p));
						if(oneof3(pBuf[p], ':', ';', ' ')) {
							pBuf = pBuf+p+1;
							p = 0;
							if(isdec(pBuf[0])) {
								do { p++; } while(isdec(pBuf[p]));
								s = static_cast<int>(_texttodec32(pBuf, p));
								if(pBuf[p] == '.') {
									pBuf = pBuf+p+1;
									p = 0;
									if(isdec(pBuf[0])) {
										do { p++; } while(isdec(pBuf[p]));
										ms = static_cast<int>(_texttodec32(pBuf, p));
									}
								}
							}
						}
					}
				}
			}
			else
				ok = 0;
		}
	}
	else
		ok = 0;
	encodetime(h, m, s, ms/10, v);
	return ok;
}
//
//
//
static bool FASTCALL is_zero(const char * c)
{
	while(*c)
		if(*c != '0' && *c != ' ')
			return false;
		else
			c++;
	return true;
}

static char * FASTCALL fmtnumber(const char * ptr, int dec, int sign, long fmt, char * buf)
{
	const  int16  prec = static_cast<int16>(static_cast<signed char>(SFMTPRC(fmt)));
	const  uint16 flags = SFMTFLAG(fmt);
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
			const char decimal_sep = (flags & NMBF_DECCOMMA) ? ',' : '.';
			*b++ = decimal_sep;
			b = stpcpy(b, ptr);
			if(flags & NMBF_NOTRAILZ) {
				while(*(b-1) == '0') {
					if(flags & NMBF_EXPLFLOAT && *(b-2) == decimal_sep) {
						break;
					}
					else
						*--b = 0;
				}
				if(*(b-1) == decimal_sep)
					*--b = 0;
			}
		}
		if(sign && (flags & NMBF_NEGPAR))
			*b++ = ')';
	}
	*b = '\0';
	return _commfmt(fmt, buf);
}

char * STDCALL decfmt(const BCD_T val, int len, int prec, long fmt, char * pBuf)
{
	char   str[64];
	char * c, *s = str;
	int    sign;
	dectostr(val, (int16)len, (int16)prec, s);
	if(*s == '-')
		s++, sign = 1;
	else
		sign = 0;
	c = sstrchr(s, '.');
	if(c)
		strcpy(c, c + 1);
	return fmtnumber(s, sstrlen(s) - prec, sign, fmt, pBuf);
}

char * STDCALL realfmt(double val, long fmt, char * pBuf)
{
	int    dec, sign;
	int    prc = static_cast<int>(static_cast<signed char>(SFMTPRC(fmt)));
	if(prc < 0) {
		val = val * fpow10i(prc);
		prc = 0;
	}
	else if(prc > 0) {
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
			const uint _len = sstrlen(str);
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
						if(last_n09_pos == 0 && str[0] == '9')
							do_carry = 1;
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

char * STDCALL intfmt(long val, long fmt, char * pBuf)
{
	char   s[64];
	ltoa(labs(val), s, 10);
	return fmtnumber(s, sstrlen(s), val < 0, fmt, pBuf);
}

char * STDCALL int64fmt(int64 val, long fmt, char * pBuf)
{
	char   s[128];
	_i64toa(_abs64(val), s, 10);
	return fmtnumber(s, sstrlen(s), val < 0, fmt, pBuf);
}

char * STDCALL uintfmt(ulong val, long fmt, char * pBuf)
{
	char   s[64];
	ultoa(val, s, 10);
	return fmtnumber(s, sstrlen(s), 0, fmt, pBuf);
}

char * STDCALL uint64fmt(uint64 val, long fmt, char * pBuf)
{
	char   s[128];
	_ui64toa(val, s, 10);
	return fmtnumber(s, sstrlen(s), 0, fmt, pBuf);
}

static char * FASTCALL clearDelimiters(char * b)
{
	char * c = b;
	while(c = strpbrk(c, "\' "))
		strcpy(c, c + 1);
	return b;
}

int FASTCALL satof(const char * pBuf, double * pVal) 
{
	if(isempty(pBuf)) {
		ASSIGN_PTR(pVal, 0.0);
		return 0;
	}
	else {
		const char * p_end = 0;
		int   erange = 0;
		return SIEEE754::Scan(pBuf, &p_end, pVal, &erange);
	}
}

double FASTCALL satof(const char * pBuf)
{
	if(isempty(pBuf))
		return 0.0;
	else {
		const  char * p_end = 0;
		int    erange = 0;
		double val = 0.0;
		return SIEEE754::Scan(pBuf, &p_end, &val, &erange) ? val : 0.0;
	}
}

int FASTCALL strtodoub(const char * pBuf, double * pVal)
{
	char   temp[128];
	char * p = sstrchr(clearDelimiters(STRNSCPY(temp, pBuf)), '.');
	if(!p) {
		p = sstrchr(temp, ',');
		if(p)
			*p = '.';
	}
	ASSIGN_PTR(pVal, satof(temp));
	return 1;
}

int FASTCALL strtolong(const char * pBuf, long * pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, atol(clearDelimiters(STRNSCPY(temp, pBuf))));
	return 1;
}

int FASTCALL strtouint(const char * pBuf, uint * pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, strtoul(clearDelimiters(STRNSCPY(temp, pBuf)), 0, 10));
	return 1;
}

int FASTCALL strtouint(const char * pBuf, ulong * pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, strtoul(clearDelimiters(STRNSCPY(temp, pBuf)), 0, 10));
	return 1;
}
//
// @construction fast int format
//
char * format_decimal64(uint64 value, char * pBuf, size_t bufSize);
char * format_decimal32(uint32 value, char * pBuf, size_t bufSize);
char * format_signed64(int64 value, char * pBuf, size_t bufSize);
char * format_signed32(int32 value, char * pBuf, size_t bufSize);

#ifndef NDEBUG
	//#define FORMATINT_DO_TEST_SENTINEL
#endif

static FORCEINLINE SString & FormatInt_Finish(const char * pResultBuf, const char * pCBuf, size_t cbufSize, SString & rBuf)
{
	rBuf.Z().Cat_Unsafe(reinterpret_cast<const uint8 *>(pResultBuf), (pCBuf+cbufSize-1)-pResultBuf);
	return rBuf;
}

SString & FASTCALL FormatInt(int value, SString & rBuf)
{
	char   cbuf[128];
#ifdef FORMATINT_DO_TEST_SENTINEL
	char   sentinel_buf[32];
	memzero(sentinel_buf, sizeof(sentinel_buf));
#endif
	const char * p = format_signed32(static_cast<int32>(value), cbuf, sizeof(cbuf));
#ifdef FORMATINT_DO_TEST_SENTINEL
	assert(ismemzero(sentinel_buf, sizeof(sentinel_buf)));
#endif
	return FormatInt_Finish(p, cbuf, sizeof(cbuf), rBuf);
}

SString & FASTCALL FormatUInt(uint value, SString & rBuf)
{
	char   cbuf[128];
#ifdef FORMATINT_DO_TEST_SENTINEL
	char   sentinel_buf[32];
	memzero(sentinel_buf, sizeof(sentinel_buf));
#endif
	const char * p = format_decimal32(static_cast<uint32>(value), cbuf, sizeof(cbuf));
#ifdef FORMATINT_DO_TEST_SENTINEL
	assert(ismemzero(sentinel_buf, sizeof(sentinel_buf)));
#endif
	return FormatInt_Finish(p, cbuf, sizeof(cbuf), rBuf);
}

SString & FASTCALL FormatInt64(int64 value, SString & rBuf)
{
	char   cbuf[128];
#ifdef FORMATINT_DO_TEST_SENTINEL
	char   sentinel_buf[32];
	memzero(sentinel_buf, sizeof(sentinel_buf));
#endif
	char * p = format_signed64(static_cast<int64>(value), cbuf, sizeof(cbuf));
#ifdef FORMATINT_DO_TEST_SENTINEL
	assert(ismemzero(sentinel_buf, sizeof(sentinel_buf)));
#endif
	return FormatInt_Finish(p, cbuf, sizeof(cbuf), rBuf);
}

SString & FASTCALL FormatUInt64(uint64 value, SString & rBuf)
{
	char   cbuf[128];
#ifdef FORMATINT_DO_TEST_SENTINEL
	char   sentinel_buf[32];
	memzero(sentinel_buf, sizeof(sentinel_buf));
#endif
	char * p = format_decimal64(static_cast<uint64>(value), cbuf, sizeof(cbuf));
#ifdef FORMATINT_DO_TEST_SENTINEL
	assert(ismemzero(sentinel_buf, sizeof(sentinel_buf)));
#endif
	return FormatInt_Finish(p, cbuf, sizeof(cbuf), rBuf);
}
