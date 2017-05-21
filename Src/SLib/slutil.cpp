// SLUTIL.CPP
// Copyright (c) A.Sobolev 2013, 2014, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
// Compare
//
#define COMPARE(a,b) ((a)>(b)) ? 1 : (((a)<(b)) ? -1 : 0)

int FASTCALL cmp_long(long a, long b)    { return COMPARE(a, b); }
int FASTCALL cmp_ulong(ulong a, ulong b) { return COMPARE(a, b); }
int FASTCALL cmp_int64(int64 a, int64 b) { return COMPARE(a, b); }
int FASTCALL cmp_double(double a, double b) { return COMPARE(a, b); }
IMPL_CMPFUNC(PcharNoCase, i1, i2) { return stricmp866((const char *)i1, (const char *)i2); }
IMPL_CMPFUNC(Pchar, i1, i2)       { return strcmp((const char *)i1, (const char *)i2); }
IMPL_CMPFUNC(int,   i1, i2)       { return COMPARE(*(const int *)i1, *(const int *)i2); }
IMPL_CMPFUNC(int16, i1, i2)       { return COMPARE(*(const int16 *)i1, *(const int16 *)i2); }
IMPL_CMPFUNC(long,  i1, i2)       { return COMPARE(*(const long *)i1, *(const long *)i2); }
IMPL_CMPFUNC(int64, i1, i2)       { return COMPARE(*(const int64 *)i1, *(const int64 *)i2); }
IMPL_CMPFUNC(uint,  i1, i2)       { return COMPARE(*(const uint *)i1, *(const uint *)i2); }
IMPL_CMPFUNC(double, i1, i2)      { return COMPARE(*(const double*)i1, *(const double*)i2); }
IMPL_CMPFUNC(LDATE, d1, d2)       { return COMPARE(((const LDATE *)d1)->v, ((const LDATE *)d2)->v); }
IMPL_CMPFUNC(LDATETIME, d1, d2)   { return cmp(*(const LDATETIME *)d1, *(const LDATETIME *)d2); }
IMPL_CMPFUNC(S_GUID, d1, d2)      { return memcmp(d1, d2, sizeof(S_GUID)); }

IMPL_CMPFUNC(_2long, i1, i2)
{
	struct _2long { long   v1, v2; };
	int    r = COMPARE(((_2long *)i1)->v1, ((_2long *)i2)->v1);
	return r ? r : COMPARE(((_2long *)i1)->v2, ((_2long *)i2)->v2);
}

IMPL_CMPFUNC(_2int64, i1, i2)
{
	struct _2longlong { int64   v1, v2; };
	int    r = COMPARE(((_2longlong *)i1)->v1, ((_2longlong *)i2)->v1);
	return r ? r : COMPARE(((_2longlong *)i1)->v2, ((_2longlong *)i2)->v2);
}
//
// Check Range
//
int FASTCALL checkirange(long nmb, long low, long upp)
{
	if(nmb >= low && nmb <= upp)
		return 1;
	else {
#ifndef _WIN32_WCE // {
		SString msg_buf;
		SLS.SetError(SLERR_BOUNDS, msg_buf.CatChar('[').Cat(low).CatCharN('.', 2).Cat(upp).CatChar(']'));
#endif // } _WIN32_WCE
		return 0;
	}
}

int FASTCALL checkupper(uint nmb, uint upper)
{
	if(nmb < upper)
		return 1;
	else {
#ifndef _WIN32_WCE // {
		SString msg_buf;
		SLS.SetError(SLERR_BOUNDS, msg_buf.CatChar('[').Cat((long)0).CatCharN('.', 2).Cat(upper).CatChar(']'));
#endif // } _WIN32_WCE
		return 0;
	}
}

int FASTCALL checkfrange(double nmb, double low, double upp)
{
	if((low == 0 && upp == 0) || (nmb >= low && nmb <= upp))
		return 1;
	else {
#ifndef _WIN32_WCE // {
		SString msg_buf;
		SLS.SetError(SLERR_BOUNDS, msg_buf.CatChar('[').Cat(low, NMBF_NOTRAILZ).CatCharN('.', 2).Cat(upp, NMBF_NOTRAILZ).CatChar(']'));
#endif // } _WIN32_WCE
		return 0;
	}
}

int FASTCALL checkdrange(LDATE dt, LDATE low, LDATE upp)
{
	if((!low || dt >= low) && (!upp || dt <= upp))
		return 1;
	else {
#ifndef _WIN32_WCE // {
		SString msg_buf;
		SLS.SetError(SLERR_BOUNDS, msg_buf.CatChar('[').Cat(low, DATF_DMY|DATF_CENTURY).
			CatCharN('.', 2).Cat(upp, DATF_DMY|DATF_CENTURY).CatChar(']'));
#endif // } _WIN32_WCE
		return 0;
	}
}
//
// SEnum
//
SEnumImp::~SEnumImp()
{
}

SEnum::SEnum(SEnumImp * pE)
{
	P_E = pE;
}

SEnum & FASTCALL SEnum::operator = (SEnumImp * pE)
{
	delete P_E;
	P_E = pE;
	return *this;
}

SEnum::~SEnum()
{
	delete P_E;
}

int SEnum::operator !() const
{
	return (P_E == 0);
}

int FASTCALL SEnum::Next(void * pData)
{
	return P_E ? P_E->Next(pData) : 0;
}

#if 0 // {

class Foo {
public:
	SEnumImp * EnumByName();
};

void foo()
{
	Foo f;
	Abc abc;
	for(SEnum enum = f.EnumByName(); enum.Next(&abc);) {

	}
}

#endif // } 0
