// SLUTIL.CPP
// Copyright (c) A.Sobolev 2013, 2014, 2016, 2017, 2018, 2019, 2020, 2021
//
#include <slib-internal.h>
#pragma hdrstop
//
// Compare
//
#define COMPARE(a,b) ((a)>(b)) ? 1 : (((a)<(b)) ? -1 : 0)

int FASTCALL cmp_long(long a, long b)    { return COMPARE(a, b); }
int FASTCALL cmp_ulong(ulong a, ulong b) { return COMPARE(a, b); }
int FASTCALL cmp_int64(int64 a, int64 b) { return COMPARE(a, b); }
int FASTCALL cmp_double(double a, double b) { return COMPARE(a, b); }
IMPL_CMPFUNC(PcharNoCase, i1, i2) { return stricmp866(static_cast<const char *>(i1), static_cast<const char *>(i2)); }
IMPL_CMPFUNC(Pchar, i1, i2) { return strcmp(static_cast<const char *>(i1), static_cast<const char *>(i2)); }
IMPL_CMPFUNC(int,   i1, i2) { return COMPARE(*static_cast<const int *>(i1), *static_cast<const int *>(i2)); }
IMPL_CMPFUNC(int16, i1, i2) { return COMPARE(*static_cast<const int16 *>(i1), *static_cast<const int16 *>(i2)); }
IMPL_CMPFUNC(long,  i1, i2) { return COMPARE(*static_cast<const long *>(i1), *static_cast<const long *>(i2)); }
IMPL_CMPFUNC(int64, i1, i2) { return COMPARE(*static_cast<const int64 *>(i1), *static_cast<const int64 *>(i2)); }
IMPL_CMPFUNC(uint,  i1, i2) { return COMPARE(*static_cast<const uint *>(i1), *static_cast<const uint *>(i2)); }
IMPL_CMPFUNC(double, i1, i2)      { return COMPARE(*static_cast<const double *>(i1), *static_cast<const double *>(i2)); }
IMPL_CMPFUNC(LDATE, d1, d2) { return COMPARE(static_cast<const LDATE *>(d1)->v, static_cast<const LDATE *>(d2)->v); }
IMPL_CMPFUNC(LDATETIME, d1, d2)   { return cmp(*static_cast<const LDATETIME *>(d1), *static_cast<const LDATETIME *>(d2)); }
IMPL_CMPFUNC(S_GUID, d1, d2)      { return memcmp(d1, d2, sizeof(S_GUID)); }

IMPL_CMPFUNC(_2long, i1, i2)
{
	struct _2long { long   v1, v2; };
	int    r = COMPARE(static_cast<const _2long *>(i1)->v1, static_cast<const _2long *>(i2)->v1);
	return r ? r : COMPARE(static_cast<const _2long *>(i1)->v2, static_cast<const _2long *>(i2)->v2);
}

IMPL_CMPFUNC(_2int64, i1, i2)
{
	struct _2longlong { int64   v1, v2; };
	int    r = COMPARE(static_cast<const _2longlong *>(i1)->v1, static_cast<const _2longlong *>(i2)->v1);
	return r ? r : COMPARE(static_cast<const _2longlong *>(i1)->v2, static_cast<const _2longlong *>(i2)->v2);
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

int    FASTCALL inrangeordefault(int val, int low, int upp, int def) { return (val >= low && val <= upp) ? val : def; }
long   FASTCALL inrangeordefault(long val, long low, long upp, long def) { return (val >= low && val <= upp) ? val : def; }
uint   FASTCALL inrangeordefault(uint val, uint low, uint upp, uint def) { return (val >= low && val <= upp) ? val : def; }
double FASTCALL inrangeordefault(double val, double low, double upp, double def) { return (val >= low && val <= upp) ? val : def; }

int FASTCALL checkupper(uint nmb, uint upper)
{
	if(nmb < upper)
		return 1;
	else {
#ifndef _WIN32_WCE // {
		SString msg_buf;
		SLS.SetError(SLERR_BOUNDS, msg_buf.CatChar('[').Cat(0L).CatCharN('.', 2).Cat(upper).CatChar(']'));
#endif // } _WIN32_WCE
		return 0;
	}
}

int FASTCALL checkfrange(double nmb, double low, double upp)
{
	if((low == 0.0 && upp == 0.0) || (nmb >= low && nmb <= upp))
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
// IterCounter
//
IterCounter::IterCounter() : Total(0), Count(0)
{
}

void FASTCALL IterCounter::Init(ulong total)
{
	Total = total;
	Count = 0L;
}

//int IterCounter::Init(DBTable * pTbl)
//{
//	RECORDNUMBER num_recs = 0;
//	return (!pTbl || pTbl->getNumRecs(&num_recs)) ? (Init(num_recs), 1) : 0; /*PPSetErrorDB()*/ // @todo Проекция DBERR_XXX в SLERR_XXX
//}
//
// SEnum
//
SEnum::Imp::~Imp()
{
}

SEnum::SEnum(SEnum::Imp * pE) : P_E(pE)
{
}

SEnum & FASTCALL SEnum::operator = (SEnum::Imp * pE)
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
private:
	class EnImp : public SEnum::Imp {
	public:
		EnImp(const void * pArg) : I(0), P_Arg(pArg)
		{
		}
		virtual int Next(void * pData)
		{
			if(I < 10) {
				static_cast<double *>(pData) = I * 1.01;
				I++;
				return 1;
			}
			else
				return 0;
		}
	private:
		uint  I;
		const void * P_Arg;
	};
public:
	SEnum::Imp * Enum(const void * pArg)
	{
		return new EnImp(pArg);
	}
};

void foo()
{
	Foo f;
	double abc;
	for(SEnum en = f.EnumByName(); en.Next(&abc);) {
		// do something with abc
	}
}

#endif // } 0
//
//
//
long MsecClock()
{
#ifdef __WIN32__
	return clock();
#else
	return (10000L * clock() / 182L);
#endif
}

FILETIME QuadWordToFileTime(__int64 src)
{
	FILETIME ft;
	ft.dwLowDateTime  = static_cast<long>(src & 0x00000000FFFFFFFF);
	ft.dwHighDateTime = static_cast<long>(Int64ShrlMod32(src, 32) & 0x00000000FFFFFFFF);
	return ft;
}

#define FILE_TIME_TO_QWORD(ft) (Int64ShllMod32(ft.dwHighDateTime, 32) | ft.dwLowDateTime)

/*static*/__int64 SProfile::NSec100Clock()
{
	FILETIME ct_tm, end_tm, k_tm, user_tm;
	GetThreadTimes(GetCurrentThread(), &ct_tm, &end_tm, &k_tm, &user_tm);
	return (FILE_TIME_TO_QWORD(user_tm) + FILE_TIME_TO_QWORD(k_tm));
}
//
// Returns the time in us since the last call to reset or since the Clock was created.
//
// @return The requested time in microseconds.  Assuming 64-bit
// integers are available, the return value is valid for 2^63
// clock cycles (over 104 years w/ clock frequency 2.8 GHz).
//
uint64 SProfile::Helper_GetAbsTimeMicroseconds()
{
	const uint64 clock_freq = SLS.GetSSys().PerfFreq;
	//
	// Compute the number of elapsed clock cycles since the clock was created/reset.
	// Using 64-bit signed ints, this is valid for 2^63 clock cycles (over 104 years w/ clock
	// frequency 2.8 GHz).
	//
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	const  uint32 tick_count = ::GetTickCount();
	uint64 clock_cycles = current_time.QuadPart - Gtb.StartHrc;
	//
	// Compute the total elapsed seconds.  This is valid for 2^63
	// clock cycles (over 104 years w/ clock frequency 2.8 GHz).
	//
	uint64 sec = (clock_cycles / clock_freq);
	//
	// Check for unexpected leaps in the Win32 performance counter.
	// (This is caused by unexpected data across the PCI to ISA
	// bridge, aka south bridge.  See Microsoft KB274323.)  Avoid
	// the problem with GetTickCount() wrapping to zero after 47
	// days (because it uses 32-bit unsigned ints to represent
	// milliseconds).
	//
	int64  msec1 = (sec * 1000 + (clock_cycles - sec * clock_freq) * 1000 / clock_freq);
	SETMIN(Gtb.StartTick, tick_count);
	int64  msec2 = static_cast<int64>(tick_count - Gtb.StartTick);
	int64  msec_diff = msec1 - msec2;
	if(msec_diff > -100 && msec_diff < 100) {
		// Adjust the starting time forwards.
		uint64 adjustment = MIN(msec_diff * clock_freq / 1000, clock_cycles - Gtb.PrevHrc);
		Gtb.StartHrc += adjustment;
		clock_cycles -= adjustment;
		// Update the measured seconds with the adjustments.
		sec = clock_cycles / clock_freq;
	}
	//
	// Compute the milliseconds part. This is always valid since it will never be greater than 1000000.
	//
	uint64 usec = (clock_cycles - sec * clock_freq) * 1000000 / clock_freq;
	//
	// Store the current elapsed clock cycles for adjustments next time.
	//
	Gtb.PrevHrc = clock_cycles;
	//
	// The return value here is valid for 2^63 clock cycles (over 104 years w/ clock frequency 2.8 GHz).
	//
	return (sec * 1000000 + usec);
}

uint64 SProfile::GetAbsTimeMicroseconds()
{
	uint64 result = 0;
	if(SingleThreaded)
		result = Helper_GetAbsTimeMicroseconds();
	else {
		ENTER_CRITICAL_SECTION
		result = Helper_GetAbsTimeMicroseconds();
		LEAVE_CRITICAL_SECTION
	}
	return result;
}

SProfile::SProfile(int singleThreaded) : SingleThreaded(BIN(singleThreaded)), StartClock(0), EndClock(0)
{
	LARGE_INTEGER cf;
	//QueryPerformanceFrequency(&cf);
	//ClockFrequency = cf.QuadPart;
	QueryPerformanceCounter(&cf);
	Gtb.PrevHrc   = 0;
	Gtb.StartHrc  = static_cast<int64>(cf.QuadPart);
	Gtb.StartTick = GetTickCount();
}

SProfile::~SProfile()
{
}

SProfile::Measure::Measure() : Start(SLS.GetProfileTime())
{
}

uint64 SProfile::Measure::Get()
{
	return (SLS.GetProfileTime() - Start);
}
