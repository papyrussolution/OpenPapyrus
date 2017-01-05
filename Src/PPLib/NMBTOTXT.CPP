// NMBTOTXT.CPP
// Copyright (c) A.Sobolev 1996-2000, 2002, 2004, 2006, 2007, 2016
//
#include <pp.h>
#pragma hdrstop

#if 0

static const char * digit_word[] = {
	"один", "одна",
	"два", "две",
	"три",
	"четыре",
	"пять",
	"шесть",
	"семь",
	"восемь",
	"девять",		// count 11
	"десять",
	"одиннадцать",
	"двенадцать",
	"тринадцать",
	"четырнадцать",
	"пятнадцать",
	"шестнадцать",
	"семнадцать",
	"восемнадцать",
	"девятнадцать",		// count 10
	"двадцать",
	"тридцать",
	"сорок",
	"пятьдесят",
	"шестьдесят",
	"семьдесят",
	"восемьдесят",
	"девяносто",		// count 8
	"сто",
	"двести",
	"триста",
	"четыреста",
	"пятьсот",
	"шестьсот",
	"семьсот",
	"восемьсот",
	"девятьсот",		// count 9
	/* 38 */
	"миллиард", "миллиарда", "миллиардов",
	"миллион", "миллиона", "миллионов",
	"тысяча", "тысячи", "тысяч"
};

static const char *currency_word[] = {
	"рубль", "рубля", "рублей", "копейка", "копейки", "копеек" };
static const char * curr_word[] = {"руб", "коп"};

#endif // 0

static const char * zero_word = "ноль";

static void SLAPI ParseNumber(double nmb, int parts[])
{
	double v, d;
	for(int i = 0; i < 5; i++) {
		if(i == 4)
			d = fpow10i(-2);
		else
			d = fpow10i(3 * (3 - i));
		if((v = floor(round(nmb / d, 12))) >= 1)
			nmb -= ((parts[i] = (int)v) * d);
		else
			parts[i] = 0;
	}
}

static char * SLAPI CopyDigitWord(const char * pStrings, char * pBuf, int i)
{
	*pBuf++ = ' ';
	PPGetSubStr(pStrings, i, pBuf, 0);
	return (pBuf + strlen(pBuf));
}

static void SLAPI BelowThousandToText(int n, int fem, const char * pStrings, char * pBuf)
{
	char * p = pBuf;
	int    i;
	int    h = n / 100;
	int    d = (n % 100) / 10;
	int    o = (n % 10);
	if(h)
		p = CopyDigitWord(pStrings, p, 28 + h);
	if(d > 1)
		p = CopyDigitWord(pStrings, p, 19 + d);
	if(d == 1 || o) {
		if(d != 1 && o > 0 && o < 3)
			i = o * 2 - (fem ? 1 : 2);
		else
			i = (d == 1 ? n % 100 : o) + 1;
		p = CopyDigitWord(pStrings, p, i);
	}
}

static int _fastcall sfx_form(int n)
{
	char   one_form[10] = {2, 0, 1, 1, 1, 2, 2, 2, 2, 2};
	n %= 100;
	return (n > 9 && n < 20) ? 2 : one_form[n % 10];
}

char * SLAPI numbertotext(double nmb, long fmt, char * pBuf)
{
	SString digit_words;
	int    n, i, parts[10];
	char * p = pBuf;
	pBuf[0] = 0;
	if(!(fmt & NTTF_NOZERO) || R2(nmb) != 0) {
		ParseNumber(nmb, parts);
		if(round(nmb, 0/*2*/) == 0) {
			p = stpcpy(p, zero_word);
			parts[3] = 0;
		}
		else {
			PPLoadText(PPTXT_NUMBERWORDS, digit_words);
			for(i = 0; i < 4; i++)
				if((n = parts[i]) != 0) {
					BelowThousandToText(n, i == 2 || (fmt & NTTF_FEMIN), digit_words, p);
					if(p == pBuf)
						trimleft(p);
					p += strlen(p);
					if(i < 3)
						p = CopyDigitWord(digit_words, p, 38 + i * 3 + sfx_form(n));
				}
		}
		i = (fmt & NTTF_DECCURR) ? 1 : 0;
		if(fmt & NTTF_ABBRCURR) {
			*p++ = ' ';
			PPGetSubStr(PPTXT_CURRENCYWORDS, i * 4, p, 0);
		}
		else if(fmt & NTTF_CURRENCY) {
			*p++ = ' ';
			PPGetSubStr(PPTXT_CURRENCYWORDS, i * 4 + 1 + sfx_form(parts[3]), p, 0);
		}
		if(fmt & NTTF_FIRSTCAP)
			*pBuf = ToUpper866(*pBuf);
	}
	return _commfmt(fmt, pBuf);
}
