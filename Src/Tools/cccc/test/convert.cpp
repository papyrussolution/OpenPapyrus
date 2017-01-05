// CONVERT.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2010
// @threadsafe
//
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <slib.h>

#pragma option -Oi

static const struct {
	int a, b;
} delimTbl[4] = {
	{ 3, 0 },
	{ NMBF_DELCOMMA, /*','*/ ' '},
	{ NMBF_DELAPOSTR, '\'' },
	{ NMBF_DELSPACE, ' ' }
};

static int SLAPI is_zero(Pchar c) // @v5.0.4 AHTOXA changed
{
	while(*c)
		if(*c != '0' && *c != ' ')
			return 0;
		else
			c++;
	return 1;
}

static Pchar SLAPI fmtnumber(Pchar ptr, int dec, int sign, long fmt, Pchar buf) // @v5.0.4 AHTOXA
{
	int16  prec = (int16) (signed char) SFMTPRC(fmt);
	int16  flags = SFMTFLAG(fmt);
	char   delim;
	Pchar  b = buf;
	if(!(flags & NMBF_NOZERO) || !is_zero(ptr)) {
		if(sign)
			if(flags & NMBF_NONEG) {
				*b = '\0';
				return buf;
			}
			else
				*b++ = (flags & NMBF_NEGPAR) ? '(' : '-';
		else if(flags & NMBF_FORCEPOS)
			*b++ = '+';
		delim = (char)ai_tab(delimTbl, flags, 0);
		if(dec-- > 0)
			*b++ = *ptr++;
		while(dec > 0) {
			if(delim && (dec % 3) == 0)
				*b++ = delim;
			*b++ = *ptr++;
			dec--;
		}
		if(prec > 0) {
			*b++ = '.';
			b = stpcpy(b, ptr);
			if(flags & NMBF_NOTRAILZ) {
				while(*(b-1) == '0')
					*--b = 0;
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
	char * s;
	int    dec, sign;
	int    prc = (int) (signed char) SFMTPRC(fmt);
	if(prc < 0) {
		val = val * fpow10i(prc);
		prc = 0;
	}
	else if(prc > 0)            /*  Papyrus @v3.6.6 */
		val = round(val, prc);
#ifdef __WIN32__
	if(val > -1.e-10 && val < 1.e-10)
		val = 0;
	if(!prc && val == 0) {
		s = fcvt(val, 1, &dec, &sign);
		dec = 1;
	}
	else
#endif
		s = fcvt(val, prc, &dec, &sign);
	if(prc > 0 && dec <= 0) {
		padleft(s, '0', 1 - dec);
		dec = 1;
	}
	SETSFMTPRC(fmt, prc);
	return fmtnumber(s, dec, sign, fmt, pBuf);
}

char * SLAPI intfmt(long val, long fmt, char * pBuf)
{
	char   s[64];
	ltoa(labs(val), s, 10);
	return fmtnumber(s, strlen(s), val < 0, fmt, pBuf);
}

char * SLAPI longfmtz(long val, int numDigits, char * pBuf, size_t bufLen)
{
	SString fmt_buf;
	fmt_buf.CatChar('%');
	if(numDigits > 0) {
		fmt_buf.CatChar('0');
		fmt_buf.Cat(numDigits);
	}
	char temp_buf[64];
	fmt_buf.CatChar('l').CatChar('d');
	sprintf(temp_buf, fmt_buf, val);
	return strnzcpy(pBuf, temp_buf, bufLen);
}

char * SLAPI uintfmt(ulong val, long fmt, char * pBuf)
{
	char   s[64];
	ultoa(val, s, 10);
	return fmtnumber(s, strlen(s), 0, fmt, pBuf);
}

#pragma warn -pia

static char * SLAPI clearDelimiters(char * b)
{
	long   skipch = 0x00002720L; /* "' " */
	char * c = b;
	while(c = strpbrk(c, (const char *) &skipch))
		strcpy(c, c + 1);
	return b;
}

#pragma warn +pia

int SLAPI strtodoub(PCchar pBuf, Pdouble pVal)
{
	char   temp[64];
	char * p = strchr(clearDelimiters(STRNSCPY(temp, pBuf)), '.');
	if(p == 0)
		if((p = strchr(temp, ',')) != 0)
			*p = '.';
	ASSIGN_PTR(pVal, atof(temp));
	return 1;
}

int SLAPI strtolong(PCchar pBuf, Plong pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, atol(clearDelimiters(STRNSCPY(temp, pBuf))));
	return 1;
}

int SLAPI strtoulong(PCchar pBuf, Pulong pVal)
{
	char   temp[64];
	ASSIGN_PTR(pVal, strtoul(clearDelimiters(STRNSCPY(temp, pBuf)), 0, 10));
	return 1;
}
