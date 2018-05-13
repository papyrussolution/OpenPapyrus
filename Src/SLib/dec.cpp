// DEC.CPP
// Copyright (c) Sobolev A. 1995-2001, 2003, 2004, 2008, 2010, 2016, 2017, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#if 0 // {

typedef struct {
	uint16 frac [4];
	uint16 signExp;
} IEEE80;

static const long e0toF [8] =
{
	1, 10, 100, 1000, 10000, 100000L, 1000000L, 10000000L
};

static const IEEE80 expo[10] =
{
	{{0,      0,      0x2000, 0xBEBC}, 0x4019},    /* 1e8    */
	{{0,      0x0400, 0xC9BF, 0x8E1B}, 0x4034},    /* 1e16   */
	{{0xB59E, 0x2B70, 0xADA8, 0x9DC5}, 0x4069},    /* 1e32   */
	{{0xA6D5, 0xFFCF, 0x1F49, 0xC278}, 0x40D3},    /* 1e64   */
	{{0x8CE0, 0x80E9, 0x47C9, 0x93BA}, 0x41A8},    /* 1e128  */
	{{0xDE8E, 0x9DF9, 0xEBFB, 0xAA7E}, 0x4351},    /* 1e256  */
	{{0x91C7, 0xA60E, 0xA0AE, 0xE319}, 0x46A3},    /* 1e512  */
	{{0x0C17, 0x8175, 0x7586, 0xC976}, 0x4D48},    /* 1e1024 */
	{{0x5DE5, 0xC53D, 0x3B5D, 0x9E8B}, 0x5A92},    /* 1e2048 */
	{{0x979B, 0x8A20, 0x5202, 0xC460}, 0x7525},    /* 1e4096 */
};

#endif // } 0

double FASTCALL dectobin(const char * dc, int16 len, int16 prec)
{
	char   buf[64];
	return atof(dectostr(dc, len, prec, buf));
}

void FASTCALL dectobcd(char * dc, char * bcd, int16 len)
{
	uchar * d = (uchar *)dc;
	uchar * b = (uchar *)bcd;
	int    i, j;
	memzero(dc, len);
	uchar  _dl = (b[9] == 0x80) ? 0xd : 0xc;
	for(i = len-1, j = 0; i; i--, j++) {
		d[i--] = (b[j] << 4) + (_dl & 0xf);
		d[i]   = (b[j+1] << 4) + (b[j] >> 4);
	}
}

char * FASTCALL dectostr(const char * dc, int16 len, int16 prec, char * b)
{
	uchar _dl = 0;
	uchar _al;
	uchar * buf = (uchar *)b;
	if((dc[len-1] & 0x0f) == 0x0d)
		*buf++ = '-';
	for(; len > 0; len--) {
		if((((len-1)<<1)+1) == prec) {
			*buf++ = '.';
			_dl = 0x80;
		}
		_al = *(dc++);
		if(_dl|(_al>>4)) {
			_dl = 0x80;
			*buf++ = ((_al>>4) & 0x7f) + '0';
		}
		if((len-1)<<1 == prec) {
			_dl = 0x80;
			*buf++ = '.';
		}
		_al &= 0xf;
		if(len!=1 && (_al | _dl)) {
			_dl = 0x80;
			*buf++ = (_al & 0x7f) + '0';
		}
	}
	*buf = 0;
	return b;
}

int FASTCALL deccmp(const char * dec1, const char * dec2, int16 len)
{
	int    r1 = BIN((dec1[len] & 0xf) == 0xd);
	int    r2 = BIN((dec2[len] & 0xf) == 0xd);
	int    r  = r1 ? (r2 ? 0 : -1) : (r2 ? 1 : 0);
	if(!r)
		r = memcmp(dec1, dec2, len-1);
	if(!r)
		r = (dec1[len] & 0xf0) - (dec2[len] & 0xf0);
	r = (r < 0) ? -1 : ((r > 0) ? 1 : 0);
	return r;
}

void FASTCALL dectodec(double val, char * dc, int16 len, int16 prec)
{
	int    sign, dec;
	uchar  al;
	uchar * tmp = (uchar *)fcvt(val, prec, &dec, &sign);
	memzero(dc, len);
	uchar * di = (uchar*)(dc + len - 1);
	size_t i = strlen((char*)tmp);
	uchar * si = tmp + i - 1;
	*di  = sign ? 0xd : 0xc;
	sign = 1;
	for(; i; i--) {
		al = *(si--) - '0';
		if(sign) {
			*di-- |= (al << 4);
			if(di < (uchar *)dc)
				break;
		}
		else
			*di = al;
		sign = !sign;
	}
}
