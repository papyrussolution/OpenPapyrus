// SSYSTEM.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2016, 2017, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <wininet.h>

//static
int SSystem::BigEndian()
{
    return BIN((reinterpret_cast<const int *>("\0\x1\x2\x3\x4\x5\x6\x7")[0] & 255) != 0);
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

//static
int SSystem::SGetModuleFileName(void * hModule, SString & rFileName)
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

//static
uint SSystem::SFormatMessage(int sysErrCode, SString & rMsg)
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

//static
uint SSystem::SFormatMessage(SString & rMsg)
{
	return SFormatMessage(::GetLastError(), rMsg);
}

SSystem::SSystem(int imm) : Flags(0)
{
	if(imm) {
		GetCpuInfo();
		if(SSystem::BigEndian())
			Flags |= fBigEndian;
	}
}

int SSystem::CpuId(int feature, uint32 * pA, uint32 * pB, uint32 * pC, uint32 * pD) const
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

int FASTCALL SSystem::GetCpuInfo()
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
	return 1;
}
//
// Собственная реализация функции _locking
// Причина: в rtl MSVS2015 есть ошибка, приводящая к неоправданной задержке
// в 1000мс при отсутствии требования повторных попыток блокировки
//
#if _MSC_VER >= 1900 // {
//
#include <sys\locking.h>
#include <sys\stat.h>
#include <errno.h>

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
