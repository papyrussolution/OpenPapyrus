// SSYSTEM.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2016, 2017, 2019, 2020, 2022, 2023
//
#include <slib-internal.h>
#pragma hdrstop
//#include <wininet.h>

static bool is_bigendian_for_test()
{
	union {
		uint32 i;
		char c[4];
	} bint = {0x01020304};
	return bint.c[0] == 1;
}

/*static*/bool SSystem::BigEndian()
{
    bool   yes = ((reinterpret_cast<const int *>("\0\x1\x2\x3\x4\x5\x6\x7")[0] & 255) != 0);
	assert(is_bigendian_for_test() == yes); // @v10.5.6
	return yes;
}

char FASTCALL SSystem::TranslateWmCharToAnsi(uintptr_t wparam)
{
	char   a = 0;
	char   mb_buf[16];
	union {
		wchar_t wc[8];
		char    mc[16];
		uintptr_t wp;
	} u;
	u.wc[0] = 0;
	u.wc[1] = 0;
	u.wc[2] = 0;
	u.wp = wparam;
	if(u.mc[0] && u.mc[1] == 0)
		a = u.mc[0];
	else {
		WideCharToMultiByte(CP_ACP, 0, u.wc, 1, mb_buf, SIZEOFARRAY(mb_buf), 0, 0);
		a = mb_buf[0];
	}
	return a;
}

/*static*/int SSystem::SGetModuleFileName(void * hModule, SString & rFileName)
{
	rFileName.Z();
	DWORD size = 0;
#ifdef _UNICODE
	wchar_t buf[1024];
	const size_t buf_size = SIZEOFARRAY(buf);
	buf[0] = 0;
	size = ::GetModuleFileNameW(static_cast<HMODULE>(hModule), buf, buf_size);
	if(size >= buf_size)
		buf[buf_size-1] = 0;
	rFileName.CopyUtf8FromUnicode(buf, sstrlen(buf), 1);
	rFileName.Transf(CTRANSF_UTF8_TO_OUTER);
#else
	char   buf[1024];
	const size_t buf_size = SIZEOFARRAY(buf);
	buf[0] = 0;
	size = ::GetModuleFileNameA(static_cast<HMODULE>(hModule), buf, buf_size);
	if(size >= buf_size)
		buf[buf_size-1] = 0;
	rFileName = buf;
#endif
	return BIN(size > 0);
}

/*static*/uint SSystem::SFormatMessage(int sysErrCode, SString & rMsg)
{
	rMsg.Z();
	DWORD ret = 0;
	//DWORD code = GetLastError();
	STempBuffer temp_buf(2048 * sizeof(TCHAR));
	DWORD buf_len = temp_buf.GetSize()/sizeof(TCHAR);
	int   intr_result = 0;
	if(sysErrCode == ERROR_INTERNET_EXTENDED_ERROR) {
		DWORD iec;
		intr_result = ::InternetGetLastResponseInfo(&iec, static_cast<LPTSTR>(temp_buf.vptr()), &buf_len);
		if(intr_result)
			ret = sstrlen(static_cast<LPTSTR>(temp_buf.vptr()));
	}
	if(!intr_result) {
		ret = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, sysErrCode,
			MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), static_cast<LPTSTR>(temp_buf.vptr()), buf_len, 0);
	}
	if(ret) {
		static_cast<LPTSTR>(temp_buf.vptr())[ret] = 0;
		rMsg = SUcSwitch(static_cast<LPTSTR>(temp_buf.vptr()));
	}
	return ret;
}

/*static*/uint SSystem::SFormatMessage(SString & rMsg)
{
	return SFormatMessage(::GetLastError(), rMsg);
}

SSystem::SSystem(int imm) : IsBigEndian(SSystem::BigEndian()), Flags(0), 
	CpuV(cpuvUnkn), CpuCs(cpucsUnkn), CpuCacheSizeL0(0), CpuCacheSizeL1(0), CpuCacheSizeL2(0), CpuA(0), CpuB(0), CpuC(0), CpuD(0), PerfFreq(0LL),
	CpuCount(0), PageSize(0)
{
	if(imm) {
        {
			// Этот блок должен исполнятся раньше всех, поскольку другие блоки могут использовать значение PerfFreq
			LARGE_INTEGER perf_frequency;
			if(::QueryPerformanceFrequency(&perf_frequency)) {
				PerfFreq = perf_frequency.QuadPart;
				Flags |= fPerfFreqIsOk;
			}
        }
		GetCpuInfo();
		{
			SYSTEM_INFO si;
			::GetSystemInfo(&si);
			CpuCount = si.dwNumberOfProcessors;
			PageSize = si.dwPageSize;
		}
	}
}

int64 SSystem::GetSystemTimestampMks() const
{
	LARGE_INTEGER perf_count;
	if(Flags & fPerfFreqIsOk && ::QueryPerformanceCounter(&perf_count)) {
		return ((perf_count.QuadPart * 1000000) / PerfFreq);
	}
	else
		return 0;
}

/*static*/int SSystem::CpuId(int feature, uint32 * pA, uint32 * pB, uint32 * pC, uint32 * pD)
{
	int    abcd[4];
	int    ret = 1;
	cpuid_abcd(abcd, feature);
	ASSIGN_PTR(pA, abcd[0]);
	ASSIGN_PTR(pB, abcd[1]);
	ASSIGN_PTR(pC, abcd[2]);
	ASSIGN_PTR(pD, abcd[3]);
	return ret;
}

int SSystem::GetCpuInfo()
{
	int    vendor_idx = 0;
	int    family_idx = 0;
	int    model_idx = 0;
	CpuType(&vendor_idx, &family_idx, &model_idx);
	if(vendor_idx == 0)
		CpuV = cpuvUnkn;
	else if(vendor_idx == 1)
		CpuV = cpuvIntel;
	else if(vendor_idx == 2)
		CpuV = cpuvAMD;
	else if(vendor_idx == 3)
		CpuV = cpuvVIA;
	else if(vendor_idx == 4)
		CpuV = cpuvCyrix;
	else if(vendor_idx == 5)
		CpuV = cpuvNexGen;
	else
		CpuV = cpuvUnkn;
	CpuCs = (CpuCmdSet)InstructionSet();
	CpuCacheSizeL0 = DataCacheSize(0);
	CpuCacheSizeL1 = DataCacheSize(1);
	CpuCacheSizeL2 = DataCacheSize(2);
	CpuBaseFreq = GetBaseCpuFreqHz(); // @v11.7.6
	return 1;
}
//
// Следующий блок кода заимствован из библиотеки intel-cpu-frequency-library (https://github.com/intel/intel-cpu-frequency-library.git) 
// с необходимой адаптацией {
// Copyright (c) 2022, Intel Corporation All rights reserved. 
//
#ifdef __GNUC__

#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
#include <immintrin.h>

static __inline int64 get_usec()
{
	struct timeval t_usecime;
	gettimeofday(&t_usecime, NULL);
	return ((int64)(t_usecime.tv_sec) * 1000000 + t_usecime.tv_usec);
}

static __inline int64 get_tsc(unsigned int* ppid)
{
	unsigned int lo, hi, pid;
	__asm__ __volatile__("rdtscp"
		: "=a" (lo), "=d" (hi), "=c" (pid)
		: : "%ebx");
	ASSIGN_PTR(ppid, pid);
	return ((int64)hi << 32) | lo;
}

static  __attribute__((noinline)) void fixed_loop(int count)
{
    // executes the loop in 'count' + fixed_overhead number of cycles
    // due to 1-cycle latency dependency or all non-ancient CPUs
    __asm__ __volatile__(
        "          lfence"
        "\n        .p2align 3"
        "\n    2:"
        "\n        and %[cnt], %[v1]"
        "\n        sub %[cnt], %[v2]"
        "\n        jg  2b"
        : : [cnt] "r" (count), [v1] "r" (-1), [v2] "r" (2) : "cc");
}

#elif _WIN32

static FORCEINLINE int64 get_usec()
{
	static int64 perf_freq;
	static BOOL init_freq = 0;
	__int64  counter;
	if(!init_freq)
		init_freq = QueryPerformanceFrequency((LARGE_INTEGER*)&perf_freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&counter);
	return counter * 1000000 / perf_freq; // convert to usec
}

static FORCEINLINE int64 get_tsc(uint * ppid)
{
    uint tsc_aux_;
    return (int64)__rdtscp(ppid ? ppid : &tsc_aux_);
}

// implemented in a separate MASM file since VS doesn't support inline assembly 
extern "C" void __fastcall fixed_loop(int); // src/osf/asmlib/fixed_loop.asm

/*void __fastcall fixed_loop(int)
{
	// @stub (реализован в модуле fixedloop.asm, надо подобрать masm-параметры для сборки)
}*/

#else
#error "Unknown platform"
#endif

static float get_curr_to_base_freq_ratio(int attempts)
{
	const int fixed_loop_clocks = 10000;
	for(int i = 0; i < attempts; i++) {
		uint pid1, pid2, pid3;
		int64 rdtsc_1 = get_tsc(&pid1);
		fixed_loop(fixed_loop_clocks); // single charge
		int64 rdtsc_2 = get_tsc(&pid2);
		fixed_loop(2 * fixed_loop_clocks); // double charge
		int64 rdtsc_3 = get_tsc(&pid3);
		int64 clocks1 = rdtsc_2 - rdtsc_1;
		int64 clocks2 = rdtsc_3 - rdtsc_2;
		int64 clocks = clocks2 - clocks1; // single charge minus common overhead
		// clocks should be close to half of clocks2 (clocks > clocks2/2.125)
		if(pid1 == pid2 && pid2 == pid3 && clocks < clocks1 && (clocks << 1) + (clocks >> 3) > clocks2) {
			return ((float)fixed_loop_clocks) / clocks;
		}
	}
	return 0.0f;
}

static float measure_base_freq_hz_once(int attempts)
{
	const int   test_clocks = 500000;
	const int   test_margin = (test_clocks >> 7);
	int64  usec = 0, usec1, usec2;
	int64  clks = 0, clks1, clks2;
	uint pid;
	for(int i = 0; i < attempts; ++i) {
		usec1 = get_usec();
		clks1 = get_tsc(&pid);
		fixed_loop(test_clocks);
		clks1 = get_tsc(&pid) - clks1;
		usec1 = get_usec() - usec1;
		usec = get_usec();
		clks = get_tsc(&pid);
		fixed_loop(test_clocks << 1);
		clks = get_tsc(&pid) - clks;
		usec = get_usec() - usec;
		usec2 = get_usec();
		clks2 = get_tsc(&pid);
		fixed_loop(test_clocks);
		clks2 = get_tsc(&pid) - clks2;
		usec2 = get_usec() - usec2;
		clks -= clks1; /* removing const overhead */
		usec -= usec1;
		// checks to skip results affected by interrupts/state change/etc 
		if(!(clks < 0 || clks1 < 0 || clks > clks1 || clks1 >(clks + test_margin) || 
			clks2 > (clks1 + test_margin) || clks1 > (clks2 + test_margin) || 
			usec < 0 || usec1 < 0 || usec > usec1 || usec1 >(usec + 1) || usec > (usec1 + 1))) {
			return (1e6f * clks) / usec;
		}
	}
	return 0.0f; /* error value */
}

static float round_freq(float freq, float rndtohz) { return rndtohz * (int)((freq + rndtohz / 2) / rndtohz); }

/*static*/float SSystem::GetBaseCpuFreqHz()
{
	//static float s_base_freq_hz = -1.0f;
	//if(s_base_freq_hz == -1.0f) {
		constexpr int attempts = 1000;
		//static float measure_base_freq_hz(int attempts)
		//{
			measure_base_freq_hz_once(1);
			float s_base_freq_hz = round_freq(measure_base_freq_hz_once(attempts) + measure_base_freq_hz_once(attempts) + 
				measure_base_freq_hz_once(attempts) + measure_base_freq_hz_once(attempts) / 4.0f, 100e6f/* rounded to 100 MHz */);
			return s_base_freq_hz;
		//}
		//s_base_freq_hz = round_freq(measure_base_freq_hz(attempts), 100e6f/* rounded to 100 MHz */);
	//}
	//return s_base_freq_hz;
}

float SSystem::GetCurrCpuFreqHz() const
{
	const float ratio = get_curr_to_base_freq_ratio(10);
	return round_freq(CpuBaseFreq * ratio, 100e6f/* rounded to 100 MHz */);
}

// } intel-cpu-frequency-library
//
//
//
#if 0 // @v10.9.11 @construction {
#if !defined(PSNIP_ONCE__H)
	//#include "../once/once.h"
#endif
#if defined(_WIN32)
	#define PSNIP_CPU__IMPL_WIN32
#elif defined(unix) || defined(__unix__) || defined(__unix)
	#include <unistd.h>
	#if defined(_SC_NPROCESSORS_ONLN) || defined(_SC_NPROC_ONLN)
		#define PSNIP_CPU__IMPL_SYSCONF
	#else
		#include <sys/sysctl.h>
	#endif
#endif
#if(CXX_ARCH_X86) || (CXX_ARCH_X86_64)
	#if defined(_MSC_VER)
		static void psnip_cpu_getid(int func, int* data) 
		{
			__cpuid(data, func);
		}
	#else
		static void psnip_cpu_getid(int func, int* data) 
		{
			__asm__ ("cpuid" : "=a" (data[0]), "=b" (data[1]), "=c" (data[2]), "=d" (data[3]) : "0" (func), "2" (0));
		}
	#endif
#elif (CXX_ARCH_ARM32) || (CXX_ARCH_ARM64)
	#if(defined(__GNUC__) && ((__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 16)))
		#define PSNIP_CPU__IMPL_GETAUXVAL
		#include <sys/auxv.h>
	#endif
#endif

static psnip_once psnip_cpu_once = PSNIP_ONCE_INIT;

#if(CXX_ARCH_X86) || (CXX_ARCH_X86_64)
	static unsigned int psnip_cpuinfo[8 * 4] = { 0, };
#elif (CXX_ARCH_ARM32) || (CXX_ARCH_ARM64)
	static unsigned long psnip_cpuinfo[2] = { 0, };
#endif

static void psnip_cpu_init(void) 
{
	#if(CXX_ARCH_X86) || (CXX_ARCH_X86_64)
		for(int i = 0 ; i < 8 ; i++) {
			psnip_cpu_getid(i, (int*) &(psnip_cpuinfo[i * 4]));
		}
	#elif (CXX_ARCH_ARM32) || (CXX_ARCH_ARM64)
		psnip_cpuinfo[0] = getauxval (AT_HWCAP);
		psnip_cpuinfo[1] = getauxval (AT_HWCAP2);
	#endif
}

int psnip_cpu_feature_check(uint feature) 
{
#if(CXX_ARCH_X86) || (CXX_ARCH_X86_64)
	uint i;
	uint r;
	uint b;
#elif defined(CXX_ARCH_ARM32) || defined(CXX_ARCH_ARM64)
	ulong b, i;
#endif
#if(CXX_ARCH_X86) || (CXX_ARCH_X86_64)
	if((feature & SSystem::cpuftr_CPU_MASK) != SSystem::cpuftr_X86)
		return 0;
#elif (CXX_ARCH_ARM32) || (CXX_ARCH_ARM64)
	if((feature & SSystem::cpuftr_CPU_MASK) != SSystem::cpuftr_ARM)
		return 0;
#else
	return 0;
#endif
	feature &= ~SSystem::cpuftr_CPU_MASK;
#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable:4152)
#endif
	psnip_once_call(&psnip_cpu_once, psnip_cpu_init);
#if defined(_MSC_VER)
	#pragma warning(pop)
#endif
#if defined(CXX_ARCH_X86) || defined(CXX_ARCH_X86_64)
	i = (feature >> 16) & 0xff;
	r = (feature >>  8) & 0xff;
	b = (feature      ) & 0xff;
	if(i > 7 || r > 3 || b > 31)
		return 0;
	return (psnip_cpuinfo[(i * 4) + r] >> b) & 1;
#elif (CXX_ARCH_ARM32) || (CXX_ARCH_ARM64)
	b = 1 << ((feature & 0xff) - 1);
	i = psnip_cpuinfo[(feature >> 0x08) & 0xff];
	return (psnip_cpuinfo[(feature >> 0x08) & 0xff] & b) == b;
#endif
}

int psnip_cpu_feature_check_many(enum PSnipCPUFeature* feature) 
{
	for(int n = 0 ; feature[n] != SSystem::cpuftr_NONE ; n++)
		if(!psnip_cpu_feature_check(feature[n]))
			return 0;
	return 1;
}

int psnip_cpu_count(void) 
{
	static int count = 0;
	int c = 0;
#if defined(_WIN32)
	DWORD_PTR lpProcessAffinityMask;
	DWORD_PTR lpSystemAffinityMask;
	int i;
#elif defined(PSNIP_CPU__IMPL_SYSCONF) && defined(HW_NCPU)
	int mib[2];
	size_t len;
#endif
	if(count != 0)
		return count;
#if defined(_WIN32)
	if(!GetProcessAffinityMask(GetCurrentProcess(), &lpProcessAffinityMask, &lpSystemAffinityMask))
		c = -1;
	else {
		for(i = 0 ; lpProcessAffinityMask != 0 ; lpProcessAffinityMask >>= 1)
			c += lpProcessAffinityMask & 1;
	}
#elif defined(_SC_NPROCESSORS_ONLN)
	c = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
	c = sysconf(_SC_NPROC_ONLN);
#elif defined(_hpux)
	c = mpctl(MPC_GETNUMSPUS, NULL, NULL);
#elif defined(HW_NCPU)
	c = 0;
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(c);
	sysctl(mib, 2, &c, &len, NULL, 0);
#endif
	count = (c > 0) ? c : -1;
	return count;
}

#endif // } 0
//
// Собственная реализация функции _locking
// Причина: в rtl MSVS2015 есть ошибка, приводящая к неоправданной задержке
// в 1000мс при отсутствии требования повторных попыток блокировки
//
#if _MSC_VER >= 1900 // {
//
//@v11.7.1 #include <sys/locking.h>
//@v11.7.1 #include <sys/stat.h>
//@v11.7.1 #include <errno.h>

extern "C" /*_Check_return_opt_*/ __int64 __cdecl _lseeki64_nolock(int _FileHandle, __int64 _Offset, int _Origin);
extern "C" void __cdecl __acrt_lowio_lock_fh(int _FileHandle);
extern "C" void __cdecl __acrt_lowio_unlock_fh(int _FileHandle);
extern "C" void __cdecl __acrt_errno_map_os_error(ulong);
// The number of handles for which file objects have been allocated.  This
// number is such that for any fh in [0, _nhandle), _pioinfo(fh) is well-formed.
extern "C" extern int _nhandle;
//
// Locks or unlocks the requested number of bytes in the specified file.
//
// Note that this function acquires the lock for the specified file and holds
// this lock for the entire duration of the call, even during the one second
// delays between calls into the operating system.  This is to prevent other
// threads from changing the file during the call.
//
// Returns 0 on success; returns -1 and sets errno on failure.
//
extern "C" int __cdecl _locking(int const fh, int const locking_mode, long const number_of_bytes)
{
    //_CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1);
    //_VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && (uint)fh < (uint)_nhandle, EBADF, -1);
    //_VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN, EBADF, -1);
    //_VALIDATE_CLEAR_OSSERR_RETURN(number_of_bytes >= 0, EINVAL, -1);
    __acrt_lowio_lock_fh(fh);
    int    result = -1;
	if(fh <= 0 || fh >= _nhandle)
		errno = EBADF;
	else if(number_of_bytes < 0)
		errno = EINVAL;
	else {
		/*
		if((_osfile(fh) & FOPEN) == 0) {
			errno = EBADF;
			_doserrno = 0;
			_ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
			__leave;
		}
		*/
		//result = locking_nolock(fh, locking_mode, number_of_bytes);
		//static int __cdecl locking_nolock(int const fh, int const locking_mode, long const number_of_bytes) throw()
		HANDLE h_file = reinterpret_cast<HANDLE>(_get_osfhandle(fh));
		if(h_file) {
			const __int64 lock_offset = _lseeki64_nolock(fh, 0L, SEEK_CUR);
			if(lock_offset != -1) {
				OVERLAPPED overlapped = { 0 };
				overlapped.Offset     = static_cast<DWORD>(lock_offset);
				overlapped.OffsetHigh = static_cast<DWORD>((lock_offset >> 32) & 0xffffffff);
				// Set the retry count, based on the mode:
				const bool allow_retry = (locking_mode == _LK_LOCK || locking_mode == _LK_RLCK);
				const uint retry_count = allow_retry ? 10 : 1;
				// Ask the OS to lock the file either until the request succeeds or the
				// retry count is reached, whichever comes first.  Note that the only error
				// possible is a locking violation, since an invalid handle would have
				// already failed above.
				bool   succeeded = false;
				uint   try_no = 0;
				do {
					if(try_no++)
						Sleep(1000);
					if(locking_mode == _LK_UNLCK)
						succeeded = UnlockFileEx(h_file, 0, number_of_bytes, 0, &overlapped) == TRUE;
					else // Ensure exclusive lock access, and return immediately if lock acquisition fails:
						succeeded = LockFileEx(h_file, LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY, 0, number_of_bytes, 0, &overlapped) == TRUE;
				} while(!succeeded && try_no < retry_count);
				// If an OS error occurred (e.g., if the file was already locked), return
				// EDEADLOCK if this was ablocking call; otherwise map the error noramlly:
				if(!succeeded) {
					__acrt_errno_map_os_error(GetLastError());
					if(oneof2(locking_mode, _LK_LOCK, _LK_RLCK))
						errno = EDEADLOCK;
				}
				else
					result = 0;
			}
		}
		__acrt_lowio_unlock_fh(fh);
	}
    return result;
}

#endif // } _MSC_VER >= 1900
