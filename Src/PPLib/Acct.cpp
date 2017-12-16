// ACCT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2007, 2008, 2016, 2017
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
//
//
Acct & FASTCALL Acct::operator = (const AcctRelTbl::Rec & s)
{
	ac = s.Ac;
	sb = s.Sb;
	ar = s.Ar;
	return *this;
}

Acct & FASTCALL Acct::operator = (const PPAccount & s)
{
	ac = s.A.Ac;
	sb = s.A.Sb;
	ar = 0;
	return *this;
}
//
// STAcct
//
SLAPI STAcct::STAcct(uint32 sz) : DataType(sz)
{
}

char * SLAPI STAcct::tostr(const void * a, long fmt, char * b) const
{
	const  int accflen = 8;
	int    ofs;
	if(S == (sizeof(Acct)-sizeof(long)))
		fmt |= ACCF_BAL;
	if(S > sizeof(Acct)) {
		fmt &= ~SFALIGNMASK;
		fmt |= ALIGN_LEFT;
		SETSFMTLEN(fmt, accflen);
	}
	if(S > sizeof(Acct)) {
		if(!(fmt & ACCF_NAMEONLY)) {
			((const Acct *)a)->ToStr(fmt, b);
			ofs = accflen;
		}
		else
			ofs = 0;
		size_t sz = sizeof(int16) * 2;
		strnzcpy(b+ofs, ((char *)a)+sz, S-sz);
	}
	else
		((const Acct *)a)->ToStr(fmt, b);
	return b;
}

int SLAPI STAcct::fromstr(void * a, long fmt, const char * b) const
{
	return ((Acct *)a)->FromStr(fmt, b);
}

void SLAPI RegisterSTAcct()
{
	RegisterSType(S_ACCT, &STAcct());
}
//
// Utils
//
void SLAPI Acct::Clear()
{
	ac = 0;
	sb = 0;
	ar = 0;
}

static int SLAPI delim(long format)
{
	if(format & ACCF_DELDOT)   return '.';
	if(format & ACCF_DELSPACE) return ' ';
	if(format & ACCF_DELHYP)   return '-';
	return 0;
}

// char * SLAPI ToStr(long format, char * pBuf) const; // ACCBIN_NATURE
// SString & SLAPI ToStr(long format, SString & rBuf) const;
// int    SLAPI FromStr(long format, const char *); // ACCBIN_NATURE

//char * SLAPI AccToStr(const Acct * acc, long format, char * buf)
char * SLAPI Acct::ToStr(long format, char * pBuf) const // ACCBIN_NATURE
{
	char * b = pBuf;
	int    dlm = delim(format);
	if(ac != 0) {
		if(ac < 10 && ac > 0 && (format & ACCF_PADACC))
			*b++ = '0';
		b += strlen(itoa(ac, b, 10));
		if(sb) {
			if(dlm)
				*b++ = dlm;
			if(sb < 10 && (format & ACCF_PADSUB))
				*b++ = '0';
			b += strlen(itoa(sb, b, 10));
		}
		if(ar && !(format & ACCF_BAL)) {
			if(dlm)
				*b++ = dlm;
			b += strlen(itoa((int)ar, b, 10));
		}
	}
	else
		*b = 0;
	return _commfmt(format, pBuf);
}

//SString & SLAPI AccToStr(const Acct * acc, long format, SString & rBuf)
SString & SLAPI Acct::ToStr(long format, SString & rBuf) const
{
	rBuf.Z();
	int    dlm = delim(format);
	if(ac != 0) {
		if(format & ACCF_PADACC)
			rBuf.CatLongZ(ac, 2);
		else
			rBuf.Cat(ac);
		if(sb) {
			if(dlm)
				rBuf.CatChar(dlm);
			if(format & ACCF_PADSUB)
				rBuf.CatLongZ(sb, 2);
			else
				rBuf.Cat(sb);
		}
		if(ar && !(format & ACCF_BAL)) {
			if(dlm)
				rBuf.CatChar(dlm);
			rBuf.Cat(ar);
		}
	}
	return _commfmt(format, rBuf);
}

static int FASTCALL searchSideText(const char * pStr, int * pSide)
{
	char   item[64];
	SString buf;
#ifdef DL200C
	(buf = "0DBT,0DB,0D,0ÄÁÒ,0ÄÁ,0Ä,1CRD,1CR,1KRD,1KR,1C,1K,1ÊÐÄ,1ÊÐ,1Ê").Transf(CTRANSF_OUTER_TO_INNER);
#else
	PPLoadString(PPSTR_SYMB, PPSSYM_ACCSIDE, buf);
#endif
	if(buf.NotEmpty()) {
		const char * p = buf.cptr();
		char * s = item;
		do {
			while(*p != ',' && *p)
				*s++ = *p++;
			*s = 0;
			if((s = stristr866(pStr, strip(item)+1)) != 0) {
				*pSide = (item[0] == '0') ? PPDEBIT : PPCREDIT;
				memset(s, ' ', strlen(item)-1);
				return 1;
			}
		} while(*p++);
	}
	*pSide = -1;
	return 0;
}

static int SLAPI parseAccString(const char * pStr, int pTok[], int * pSide)
{
	int    i = 0;
	int    sd = -1;
	char   s[32], dot[2], * p;
	dot[0] = '.';
	dot[1] = 0;
	STRNSCPY(s, pStr);
	if(pSide) {
		searchSideText(s, &sd);
		strip(s);
	}
	if((p = strtok(s, dot)) != 0)
		do {
			pTok[i++] = atoi(p);
		} while(i < 3 && (p = strtok(0, dot)) != 0);
	while(i < 3)
		pTok[i++] = -1; // 0;
	ASSIGN_PTR(pSide, sd);
	return 1;
}

//int SLAPI StrToAcc(Acct * pAcct, long, const char * pStr)
int SLAPI Acct::FromStr(long format, const char * pStr) // ACCBIN_NATURE
{
	int    tok[3];
	parseAccString(pStr, tok, 0);
	ac = (tok[0] == -1) ? 0 : tok[0];
	sb = (tok[1] == -1) ? 0 : tok[1];
	ar = (tok[2] == -1) ? 0 : tok[2];
	return 1;
}

int SLAPI IsAccBelongToList(const Acct * pAcct, int side, const char * pList)
{
	int    found = 0;
	char   buf[32], separator[8];
	char   dup[256];
	char * q, * p = strip(STRNSCPY(dup, pList));
	if(*p == '*')
		found = 1;
	else if(*p != 0) {
		separator[0] = ',';
		separator[1] = 0;
		if((p = strtok(p, separator)) != 0)
			do {
				int sd, tok[3];
				q = p + strlen(p) + 1;
				p = STRNSCPY(buf, p);
				parseAccString(p, tok, &sd);
				if(pAcct->ac == tok[0])
					if(pAcct->sb == tok[1] || tok[1] == -1)
						if(pAcct->ar == tok[2] || tok[2] == -1)
							if(sd == -1 || sd == side)
								found = 1;
			} while(!found && *q && (p = strtok(q, separator)) != 0);
	}
	return found;
}

int SLAPI IsSuitableAcc(Acct * pAcc, int aco /* ACO_X */, Acct * pPattern)
{
	if(aco == ACO_1)
		return (pAcc->ac == pPattern->ac) ? 1 : 0;
	if(aco == ACO_2)
		return (pAcc->ac == pPattern->ac && pAcc->sb == pPattern->sb) ? 1 : 0;
	return (pAcc->ac == pPattern->ac && pAcc->sb == pPattern->sb && pAcc->ar == pPattern->ar) ? 1 : 0;
}
//
//
//
void SLAPI AcctID::Clear()
{
	ac = 0;
	ar = 0;
}

int FASTCALL AcctID::operator == (AcctID s) const
{
	return (ac == s.ac && ar == s.ar);
}

int FASTCALL AcctID::operator != (AcctID s) const
{
	return (ac != s.ac || ar != s.ar);
}
