// 7z-windows.cpp
//
#include <7z-internal.h>
#pragma hdrstop
//
using namespace NWindows;
using namespace NFile;
using namespace NName;

#ifndef _UNICODE
	extern bool g_IsNT;
#endif
extern HINSTANCE g_hInstance;
//
// PropVariantConvert.cpp
#define UINT_TO_STR_2(c, val) { s[0] = (c); s[1] = (char)('0' + (val) / 10); s[2] = (char)('0' + (val) % 10); s += 3; }

bool ConvertUtcFileTimeToString(const FILETIME &utc, char * s, int level) throw()
{
	*s = 0;
	FILETIME ft;
	if(!FileTimeToLocalFileTime(&utc, &ft))
		return false;
	SYSTEMTIME st;
	if(!BOOLToBool(FileTimeToSystemTime(&ft, &st)))
		return false;
	{
		unsigned val = st.wYear;
		if(val >= 10000) {
			*s++ = (char)('0' + val / 10000);
			val %= 10000;
		}
		s[3] = (char)('0' + val % 10); val /= 10;
		s[2] = (char)('0' + val % 10); val /= 10;
		s[1] = (char)('0' + val % 10);
		s[0] = (char)('0' + val / 10);
		s += 4;
	}
	UINT_TO_STR_2('-', st.wMonth);
	UINT_TO_STR_2('-', st.wDay);
	if(level > kTimestampPrintLevel_DAY) {
		UINT_TO_STR_2(' ', st.wHour);
		UINT_TO_STR_2(':', st.wMinute);
		if(level >= kTimestampPrintLevel_SEC) {
			UINT_TO_STR_2(':', st.wSecond);
			if(level > kTimestampPrintLevel_SEC) {
				*s++ = '.';
				/*
				   {
				   unsigned val = st.wMilliseconds;
				   s[2] = (char)('0' + val % 10); val /= 10;
				   s[1] = (char)('0' + val % 10);
				   s[0] = (char)('0' + val / 10);
				   s += 3;
				   }
				   *s++ = ' ';
				 */
				{
					unsigned numDigits = 7;
					uint32 val = (uint32)((((uint64)ft.dwHighDateTime << 32) + ft.dwLowDateTime) % 10000000);
					for(uint i = numDigits; i != 0;) {
						i--;
						s[i] = (char)('0' + val % 10); val /= 10;
					}
					if(numDigits > (uint)level)
						numDigits = (uint)level;
					s += numDigits;
				}
			}
		}
	}
	*s = 0;
	return true;
}

bool ConvertUtcFileTimeToString(const FILETIME &ft, wchar_t * dest, int level) throw()
{
	char s[32];
	bool res = ConvertUtcFileTimeToString(ft, s, level);
	for(uint i = 0;; i++) {
		uchar c = s[i];
		dest[i] = c;
		if(c == 0)
			break;
	}
	return res;
}

void ConvertPropVariantToShortString(const PROPVARIANT &prop, char * dest) throw()
{
	*dest = 0;
	switch(prop.vt) {
		case VT_EMPTY: return;
		case VT_BSTR: dest[0] = '?'; dest[1] = 0; return;
		case VT_UI1: ConvertUInt32ToString(prop.bVal, dest); return;
		case VT_UI2: ConvertUInt32ToString(prop.uiVal, dest); return;
		case VT_UI4: ConvertUInt32ToString(prop.ulVal, dest); return;
		case VT_UI8: ConvertUInt64ToString(prop.uhVal.QuadPart, dest); return;
		case VT_FILETIME: ConvertUtcFileTimeToString(prop.filetime, dest); return;
		// case VT_I1: return ConvertInt64ToString(prop.cVal, dest); return;
		case VT_I2: ConvertInt64ToString(prop.iVal, dest); return;
		case VT_I4: ConvertInt64ToString(prop.lVal, dest); return;
		case VT_I8: ConvertInt64ToString(prop.hVal.QuadPart, dest); return;
		case VT_BOOL: dest[0] = VARIANT_BOOLToBool(prop.boolVal) ? '+' : '-'; dest[1] = 0; return;
		default: dest[0] = '?'; dest[1] = ':'; ConvertUInt64ToString(prop.vt, dest + 2);
	}
}

void ConvertPropVariantToShortString(const PROPVARIANT &prop, wchar_t * dest) throw()
{
	*dest = 0;
	switch(prop.vt) {
		case VT_EMPTY: return;
		case VT_BSTR: dest[0] = '?'; dest[1] = 0; return;
		case VT_UI1: ConvertUInt32ToString(prop.bVal, dest); return;
		case VT_UI2: ConvertUInt32ToString(prop.uiVal, dest); return;
		case VT_UI4: ConvertUInt32ToString(prop.ulVal, dest); return;
		case VT_UI8: ConvertUInt64ToString(prop.uhVal.QuadPart, dest); return;
		case VT_FILETIME: ConvertUtcFileTimeToString(prop.filetime, dest); return;
		// case VT_I1: return ConvertInt64ToString(prop.cVal, dest); return;
		case VT_I2: ConvertInt64ToString(prop.iVal, dest); return;
		case VT_I4: ConvertInt64ToString(prop.lVal, dest); return;
		case VT_I8: ConvertInt64ToString(prop.hVal.QuadPart, dest); return;
		case VT_BOOL: dest[0] = VARIANT_BOOLToBool(prop.boolVal) ? (wchar_t)'+' : (wchar_t)'-'; dest[1] = 0; return;
		default: dest[0] = '?'; dest[1] = ':'; ConvertUInt32ToString(prop.vt, dest + 2);
	}
}
//
// PropVariantUtils.cpp
static void FASTCALL AddHex(AString & s, uint32 v)
{
	char sz[16];
	sz[0] = '0';
	sz[1] = 'x';
	ConvertUInt32ToHex(v, sz + 2);
	s += sz;
}

AString TypePairToString(const CUInt32PCharPair * pairs, unsigned num, uint32 value)
{
	char sz[16];
	const char * p = NULL;
	for(uint i = 0; i < num; i++) {
		const CUInt32PCharPair &pair = pairs[i];
		if(pair.Value == value)
			p = pair.Name;
	}
	if(!p) {
		ConvertUInt32ToString(value, sz);
		p = sz;
	}
	return (AString)p;
}

void PairToProp(const CUInt32PCharPair * pairs, unsigned num, uint32 value, NCOM::CPropVariant &prop)
{
	prop = TypePairToString(pairs, num, value);
}

AString TypeToString(const char * const table[], unsigned num, uint32 value)
{
	char sz[16];
	const char * p = NULL;
	if(value < num)
		p = table[value];
	if(!p) {
		ConvertUInt32ToString(value, sz);
		p = sz;
	}
	return (AString)p;
}

void TypeToProp(const char * const table[], unsigned num, uint32 value, NWindows::NCOM::CPropVariant &prop)
{
	char sz[16];
	const char * p = NULL;
	if(value < num)
		p = table[value];
	if(!p) {
		ConvertUInt32ToString(value, sz);
		p = sz;
	}
	prop = p;
}

AString FlagsToString(const char * const * names, unsigned num, uint32 flags)
{
	AString s;
	for(uint i = 0; i < num; i++) {
		uint32 flag = (uint32)1 << i;
		if((flags & flag) != 0) {
			const char * name = names[i];
			if(name && name[0] != 0) {
				s.Add_OptSpaced(name);
				flags &= ~flag;
			}
		}
	}
	if(flags != 0) {
		s.Add_Space_if_NotEmpty();
		AddHex(s, flags);
	}
	return s;
}

AString FlagsToString(const CUInt32PCharPair * pairs, unsigned num, uint32 flags)
{
	AString s;
	for(uint i = 0; i < num; i++) {
		const CUInt32PCharPair &p = pairs[i];
		uint32 flag = (uint32)1 << (uint)p.Value;
		if((flags & flag) != 0) {
			if(p.Name[0] != 0)
				s.Add_OptSpaced(p.Name);
		}
		flags &= ~flag;
	}
	if(flags != 0) {
		s.Add_Space_if_NotEmpty();
		AddHex(s, flags);
	}
	return s;
}

void FlagsToProp(const char * const * names, unsigned num, uint32 flags, NCOM::CPropVariant &prop)
	{ prop = FlagsToString(names, num, flags); }
void FlagsToProp(const CUInt32PCharPair * pairs, unsigned num, uint32 flags, NCOM::CPropVariant &prop)
	{ prop = FlagsToString(pairs, num, flags); }

AString Flags64ToString(const CUInt32PCharPair * pairs, unsigned num, uint64 flags)
{
	AString s;
	for(uint i = 0; i < num; i++) {
		const CUInt32PCharPair &p = pairs[i];
		uint64 flag = (uint64)1 << (uint)p.Value;
		if((flags & flag) != 0) {
			if(p.Name[0] != 0)
				s.Add_OptSpaced(p.Name);
		}
		flags &= ~flag;
	}
	if(flags != 0) {
		{
			char sz[32];
			sz[0] = '0';
			sz[1] = 'x';
			ConvertUInt64ToHex(flags, sz + 2);
			s.Add_OptSpaced(sz);
		}
	}
	return s;
}

void Flags64ToProp(const CUInt32PCharPair * pairs, unsigned num, uint64 flags, NCOM::CPropVariant &prop)
{
	prop = Flags64ToString(pairs, num, flags);
}
//
#if defined(_WIN32) && !defined(UNDER_CE)
	EXTERN_C_BEGIN
		typedef enum {
			My_FindStreamInfoStandard,
			My_FindStreamInfoMaxInfoLevel
		} MY_STREAM_INFO_LEVELS;
		typedef struct {
			LARGE_INTEGER StreamSize;
			WCHAR cStreamName[MAX_PATH + 36];
		} MY_WIN32_FIND_STREAM_DATA, * MY_PWIN32_FIND_STREAM_DATA;
		typedef WINBASEAPI HANDLE (WINAPI *FindFirstStreamW_Ptr)(LPCWSTR fileName, MY_STREAM_INFO_LEVELS infoLevel, LPVOID findStreamData, DWORD flags);
		typedef WINBASEAPI BOOL (APIENTRY *FindNextStreamW_Ptr)(HANDLE findStream, LPVOID findStreamData);
	EXTERN_C_END
#endif
//
// Windows/System.cpp Windows/FileSystem.cpp Windows/FileIO.cpp Windows/FileDir.cpp Windows/FileName.cpp Windows/FileLink.cpp Windows/FileFind.cpp Windows/ErrorMsg.h 
// Windows/MemoryLock.cpp Windows/PropVariant.cpp Windows/DLL.cpp Windows/ResourceString.cpp Windows/TimeUtils.cpp
namespace NWindows {
	#ifndef _UNICODE
		static CSysString MyLoadStringA(HINSTANCE hInstance, UINT resourceID)
		{
			CSysString s;
			int size = 128;
			int len;
			do {
				size <<= 1;
				len = ::LoadString(hInstance, resourceID, s.GetBuf(size - 1), size);
			} while(size - len <= 1);
			s.ReleaseBuf_CalcLen(len);
			return s;
		}
	#endif

	static const int kStartSize = 256;

	static void MyLoadString2(HINSTANCE hInstance, UINT resourceID, UString &s)
	{
		int size = kStartSize;
		int len;
		do {
			size <<= 1;
			len = ::LoadStringW(hInstance, resourceID, s.GetBuf(size - 1), size);
		} while(size - len <= 1);
		s.ReleaseBuf_CalcLen(len);
	}
	//
	// NT4 doesn't support LoadStringW(,,, 0) to get pointer to resource string. So we don't use it.
	//
	UString MyLoadString(UINT resourceID)
	{
	  #ifndef _UNICODE
		if(!g_IsNT)
			return GetUnicodeString(MyLoadStringA(g_hInstance, resourceID));
		else
	  #endif
		{
			{
				wchar_t s[kStartSize];
				s[0] = 0;
				int len = ::LoadStringW(g_hInstance, resourceID, s, kStartSize);
				if(kStartSize - len > 1)
					return s;
			}
			UString dest;
			MyLoadString2(g_hInstance, resourceID, dest);
			return dest;
		}
	}
	void MyLoadString(HINSTANCE hInstance, UINT resourceID, UString &dest)
	{
		dest.Empty();
	  #ifndef _UNICODE
		if(!g_IsNT)
			MultiByteToUnicodeString2(dest, MyLoadStringA(hInstance, resourceID));
		else
	  #endif
		{
			{
				wchar_t s[kStartSize];
				s[0] = 0;
				int len = ::LoadStringW(hInstance, resourceID, s, kStartSize);
				if(kStartSize - len > 1) {
					dest = s;
					return;
				}
			}
			MyLoadString2(hInstance, resourceID, dest);
		}
	}
	void MyLoadString(UINT resourceID, UString &dest)
	{
		MyLoadString(g_hInstance, resourceID, dest);
	}
	//
#ifdef SUPPORT_DEVICE_FILE
	namespace NSystem {
		bool MyGetDiskFreeSpace(CFSTR rootPath, uint64 &clusterSize, uint64 &totalSize, uint64 &freeSize);
	}
#endif
	namespace NSystem {
		uint32 CountAffinity(DWORD_PTR mask)
		{
			uint32 num = 0;
			for(uint i = 0; i < sizeof(mask) * 8; i++)
				num += (uint32)((mask >> i) & 1);
			return num;
		}
		#ifdef _WIN32
			BOOL CProcessAffinity::Get()
			{
			#ifndef UNDER_CE
				return GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
			#else
				return FALSE;
			#endif
			}
			uint32 GetNumberOfProcessors()
			{
				// We need to know how many threads we can use.
				// By default the process is assigned to one group.
				// So we get the number of logical processors (threads)
				// assigned to current process in the current group.
				// Group size can be smaller than total number logical processors, for exammple, 2x36
				CProcessAffinity pa;
				if(pa.Get() && pa.processAffinityMask != 0)
					return pa.GetNumProcessThreads();
				SYSTEM_INFO systemInfo;
				GetSystemInfo(&systemInfo);
				// the number of logical processors in the current group
				return (uint32)systemInfo.dwNumberOfProcessors;
			}
		#else
			uint32 GetNumberOfProcessors()
			{
				return 1;
			}
		#endif
		#ifdef _WIN32
			#ifndef UNDER_CE
				#if !defined(_WIN64) && defined(__GNUC__)
					typedef struct _MY_MEMORYSTATUSEX {
						DWORD dwLength;
						DWORD dwMemoryLoad;
						DWORDLONG ullTotalPhys;
						DWORDLONG ullAvailPhys;
						DWORDLONG ullTotalPageFile;
						DWORDLONG ullAvailPageFile;
						DWORDLONG ullTotalVirtual;
						DWORDLONG ullAvailVirtual;
						DWORDLONG ullAvailExtendedVirtual;
					} MY_MEMORYSTATUSEX, * MY_LPMEMORYSTATUSEX;
				#else
					#define MY_MEMORYSTATUSEX MEMORYSTATUSEX
					#define MY_LPMEMORYSTATUSEX LPMEMORYSTATUSEX
				#endif
				typedef BOOL (WINAPI *GlobalMemoryStatusExP)(MY_LPMEMORYSTATUSEX lpBuffer);
			#endif
		#endif

		bool GetRamSize(uint64 &size)
		{
			size = (uint64)(sizeof(size_t)) << 29;
		  #ifdef _WIN32
		  #ifndef UNDER_CE
			MY_MEMORYSTATUSEX stat;
			stat.dwLength = sizeof(stat);
		  #endif
		  #ifdef _WIN64
			if(!::GlobalMemoryStatusEx(&stat))
				return false;
			size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
			return true;
		  #else
			#ifndef UNDER_CE
			GlobalMemoryStatusExP globalMemoryStatusEx = (GlobalMemoryStatusExP)::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")), "GlobalMemoryStatusEx");
			if(globalMemoryStatusEx && globalMemoryStatusEx(&stat)) {
				size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
				return true;
			}
			#endif
			{
				MEMORYSTATUS stat2;
				stat2.dwLength = sizeof(stat2);
				::GlobalMemoryStatus(&stat2);
				size = MyMin(stat2.dwTotalVirtual, stat2.dwTotalPhys);
				return true;
			}
		  #endif
		  #else
			return false;
		  #endif
		}
	}
	namespace NFile {
		namespace NSystem {
			#ifndef UNDER_CE
			bool MyGetVolumeInformation(CFSTR rootPath, UString &volumeName, LPDWORD volumeSerialNumber, 
				LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, UString &fileSystemName)
			{
				BOOL res;
			  #ifndef _UNICODE
				if(!g_IsNT) {
					TCHAR v[MAX_PATH+2]; v[0] = 0;
					TCHAR f[MAX_PATH+2]; f[0] = 0;
					res = GetVolumeInformation(fs2fas(rootPath), v, MAX_PATH, volumeSerialNumber, maximumComponentLength, fileSystemFlags, f, MAX_PATH);
					volumeName = MultiByteToUnicodeString(v);
					fileSystemName = MultiByteToUnicodeString(f);
				}
				else
			  #endif
				{
					WCHAR v[MAX_PATH+2]; v[0] = 0;
					WCHAR f[MAX_PATH+2]; f[0] = 0;
					res = GetVolumeInformationW(fs2us(rootPath), v, MAX_PATH, volumeSerialNumber, maximumComponentLength, fileSystemFlags, f, MAX_PATH);
					volumeName = v;
					fileSystemName = f;
				}
				return BOOLToBool(res);
			}

			UINT MyGetDriveType(CFSTR pathName)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					return GetDriveType(fs2fas(pathName));
				}
				else
			  #endif
				{
					return GetDriveTypeW(fs2us(pathName));
				}
			}

			typedef BOOL (WINAPI * GetDiskFreeSpaceExA_Pointer)(LPCSTR lpDirectoryName/*directory name*/, PULARGE_INTEGER lpFreeBytesAvailable/*bytes available to caller*/,
				PULARGE_INTEGER lpTotalNumberOfBytes/*bytes on disk*/, PULARGE_INTEGER lpTotalNumberOfFreeBytes/*free bytes on disk*/);
			typedef BOOL (WINAPI * GetDiskFreeSpaceExW_Pointer)(LPCWSTR lpDirectoryName/*directory name*/, PULARGE_INTEGER lpFreeBytesAvailable/*bytes available to caller*/,
				PULARGE_INTEGER lpTotalNumberOfBytes/*bytes on disk*/, PULARGE_INTEGER lpTotalNumberOfFreeBytes/*free bytes on disk*/);

			bool MyGetDiskFreeSpace(CFSTR rootPath, uint64 &clusterSize, uint64 &totalSize, uint64 &freeSize)
			{
				DWORD numSectorsPerCluster, bytesPerSector, numFreeClusters, numClusters;
				bool sizeIsDetected = false;
			  #ifndef _UNICODE
				if(!g_IsNT) {
					GetDiskFreeSpaceExA_Pointer pGetDiskFreeSpaceEx = (GetDiskFreeSpaceExA_Pointer)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetDiskFreeSpaceExA");
					if(pGetDiskFreeSpaceEx) {
						ULARGE_INTEGER freeBytesToCaller2, totalSize2, freeSize2;
						sizeIsDetected = BOOLToBool(pGetDiskFreeSpaceEx(fs2fas(rootPath), &freeBytesToCaller2, &totalSize2, &freeSize2));
						totalSize = totalSize2.QuadPart;
						freeSize = freeSize2.QuadPart;
					}
					if(!::GetDiskFreeSpace(fs2fas(rootPath), &numSectorsPerCluster, &bytesPerSector, &numFreeClusters, &numClusters))
						return false;
				}
				else
			  #endif
				{
					GetDiskFreeSpaceExW_Pointer pGetDiskFreeSpaceEx = (GetDiskFreeSpaceExW_Pointer)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetDiskFreeSpaceExW");
					if(pGetDiskFreeSpaceEx) {
						ULARGE_INTEGER freeBytesToCaller2, totalSize2, freeSize2;
						sizeIsDetected = BOOLToBool(pGetDiskFreeSpaceEx(fs2us(rootPath), &freeBytesToCaller2, &totalSize2, &freeSize2));
						totalSize = totalSize2.QuadPart;
						freeSize = freeSize2.QuadPart;
					}
					if(!::GetDiskFreeSpaceW(fs2us(rootPath), &numSectorsPerCluster, &bytesPerSector, &numFreeClusters, &numClusters))
						return false;
				}
				clusterSize = (uint64)bytesPerSector * (uint64)numSectorsPerCluster;
				if(!sizeIsDetected) {
					totalSize = clusterSize * (uint64)numClusters;
					freeSize = clusterSize * (uint64)numFreeClusters;
				}
				return true;
			}
			#endif
		}
		namespace NIO {
			/*
			   WinXP-64 CreateFile():
			   ""             -  ERROR_PATH_NOT_FOUND
			   :stream        -  OK
			   .:stream       -  ERROR_PATH_NOT_FOUND
			   .\:stream      -  OK

			   folder\:stream -  ERROR_INVALID_NAME
			   folder:stream  -  OK

			   c:\:stream     -  OK

			   c::stream      -  ERROR_INVALID_NAME, if current dir is NOT ROOT ( c:\dir1 )
			   c::stream      -  OK,                 if current dir is ROOT     ( c:\ )
			 */

			bool CFileBase::Create(CFSTR path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
			{
				if(!Close())
					return false;
			  #ifdef SUPPORT_DEVICE_FILE
				IsDeviceFile = false;
			  #endif
			  #ifndef _UNICODE
				if(!g_IsNT) {
					_handle = ::CreateFile(fs2fas(path), desiredAccess, shareMode, (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);
				}
				else
			  #endif
				{
					IF_USE_MAIN_PATH
						_handle = ::CreateFileW(fs2us(path), desiredAccess, shareMode, (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);
				#ifdef WIN_LONG_PATH
					if(_handle == INVALID_HANDLE_VALUE && USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							_handle = ::CreateFileW(superPath, desiredAccess, shareMode, (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);
					}
				#endif
				}
				return (_handle != INVALID_HANDLE_VALUE);
			}
			bool CFileBase::Close() throw()
			{
				if(_handle == INVALID_HANDLE_VALUE)
					return true;
				if(!::CloseHandle(_handle))
					return false;
				_handle = INVALID_HANDLE_VALUE;
				return true;
			}
			bool CFileBase::GetLength(uint64 &length) const throw()
			{
			  #ifdef SUPPORT_DEVICE_FILE
				if(IsDeviceFile && SizeDefined) {
					length = Size;
					return true;
				}
			  #endif
				DWORD sizeHigh;
				DWORD sizeLow = ::GetFileSize(_handle, &sizeHigh);
				if(sizeLow == 0xFFFFFFFF)
					if(::GetLastError() != NO_ERROR)
						return false;
				length = (((uint64)sizeHigh) << 32) + sizeLow;
				return true;
			}

			bool CFileBase::Seek(int64 distanceToMove, DWORD moveMethod, uint64 &newPosition) const throw()
			{
			  #ifdef SUPPORT_DEVICE_FILE
				if(IsDeviceFile && SizeDefined && moveMethod == FILE_END) {
					distanceToMove += Size;
					moveMethod = FILE_BEGIN;
				}
			  #endif
				LONG high = (LONG)(distanceToMove >> 32);
				DWORD low = ::SetFilePointer(_handle, (LONG)(distanceToMove & 0xFFFFFFFF), &high, moveMethod);
				if(low == 0xFFFFFFFF)
					if(::GetLastError() != NO_ERROR)
						return false;
				newPosition = (((uint64)(uint32)high) << 32) + low;
				return true;
			}

			bool CFileBase::GetPosition(uint64 &position) const throw() { return Seek(0, FILE_CURRENT, position); }
			bool CFileBase::Seek(uint64 position, uint64 &newPosition) const throw() { return Seek(position, FILE_BEGIN, newPosition); }
			bool CFileBase::SeekToEnd(uint64 &newPosition) const throw() { return Seek(0, FILE_END, newPosition); }

			bool CFileBase::SeekToBegin() const throw()
			{
				uint64 newPosition;
				return Seek(0, newPosition);
			}
			//
			// CInFile
			//
			#ifdef SUPPORT_DEVICE_FILE

			void CInFile::CorrectDeviceSize()
			{
				// maybe we must decrease kClusterSize to 1 << 12, if we want correct size at tail
				static const uint32 kClusterSize = 1 << 14;
				uint64 pos = Size & ~(uint64)(kClusterSize - 1);
				uint64 realNewPosition;
				if(!Seek(pos, realNewPosition))
					return;
				Byte * buf = static_cast<Byte *>(MidAlloc(kClusterSize));
				bool needbackward = true;
				for(;;) {
					uint32 processed = 0;
					// up test is slow for "PhysicalDrive".
					// processed size for latest block for "PhysicalDrive0" is 0.
					if(!Read1(buf, kClusterSize, processed))
						break;
					if(processed == 0)
						break;
					needbackward = false;
					Size = pos + processed;
					if(processed != kClusterSize)
						break;
					pos += kClusterSize;
				}
				if(needbackward && pos != 0) {
					pos -= kClusterSize;
					for(;;) {
						// break;
						if(!Seek(pos, realNewPosition))
							break;
						if(!buf) {
							buf = static_cast<Byte *>(MidAlloc(kClusterSize));
							if(!buf)
								break;
						}
						uint32 processed = 0;
						// that code doesn't work for "PhysicalDrive0"
						if(!Read1(buf, kClusterSize, processed))
							break;
						if(processed != 0) {
							Size = pos + processed;
							break;
						}
						if(pos == 0)
							break;
						pos -= kClusterSize;
					}
				}
				MidFree(buf);
			}

			void CInFile::CalcDeviceSize(CFSTR s)
			{
				SizeDefined = false;
				Size = 0;
				if(_handle == INVALID_HANDLE_VALUE || !IsDeviceFile)
					return;
			  #ifdef UNDER_CE
				SizeDefined = true;
				Size = 128 << 20;
			  #else
				PARTITION_INFORMATION partInfo;
				bool needCorrectSize = true;
				/*
				   WinXP 64-bit:

				   HDD \\.\PhysicalDrive0 (MBR):
					GetPartitionInfo == GeometryEx :  corrrect size? (includes tail)
					Geometry   :  smaller than GeometryEx (no tail, maybe correct too?)
					MyGetDiskFreeSpace : FAIL
					Size correction is slow and block size (kClusterSize) must be small?

				   HDD partition \\.\N: (NTFS):
					MyGetDiskFreeSpace   :  Size of NTFS clusters. Same size can be calculated after correction
					GetPartitionInfo     :  size of partition data: NTFS clusters + TAIL; TAIL contains extra empty sectors and
					   copy of first sector of NTFS
					Geometry / CdRomGeometry / GeometryEx :  size of HDD (not that partition)

				   CD-ROM drive (ISO):
					MyGetDiskFreeSpace   :  correct size. Same size can be calculated after correction
					Geometry == CdRomGeometry  :  smaller than corrrect size
					GetPartitionInfo == GeometryEx :  larger than corrrect size

				   Floppy \\.\a: (FAT):
					Geometry :  correct size.
					CdRomGeometry / GeometryEx / GetPartitionInfo / MyGetDiskFreeSpace - FAIL
					correction works OK for FAT.
					correction works OK for non-FAT, if kClusterSize = 512.
				 */

				if(GetPartitionInfo(&partInfo)) {
					Size = partInfo.PartitionLength.QuadPart;
					SizeDefined = true;
					needCorrectSize = false;
					if((s)[0] == '\\' && (s)[1] == '\\' && (s)[2] == '.' && (s)[3] == '\\' && (s)[5] == ':' && (s)[6] == 0) {
						FChar path[4] = { s[4], ':', '\\', 0 };
						uint64 clusterSize, totalSize, freeSize;
						if(NSystem::MyGetDiskFreeSpace(path, clusterSize, totalSize, freeSize))
							Size = totalSize;
						else
							needCorrectSize = true;
					}
				}
				if(!SizeDefined) {
					my_DISK_GEOMETRY_EX geomEx;
					SizeDefined = GetGeometryEx(&geomEx);
					if(SizeDefined)
						Size = geomEx.DiskSize.QuadPart;
					else {
						DISK_GEOMETRY geom;
						SizeDefined = GetGeometry(&geom);
						SETIFZ(SizeDefined, GetCdRomGeometry(&geom));
						if(SizeDefined)
							Size = geom.Cylinders.QuadPart * geom.TracksPerCylinder * geom.SectorsPerTrack * geom.BytesPerSector;
					}
				}
				if(needCorrectSize && SizeDefined && Size != 0) {
					CorrectDeviceSize();
					SeekToBegin();
				}

				// SeekToBegin();
			  #endif
			}

			// ((desiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA | GENERIC_WRITE)) == 0 &&

			#define MY_DEVICE_EXTRA_CODE IsDeviceFile = IsDevicePath(fileName); CalcDeviceSize(fileName);
			#else
				#define MY_DEVICE_EXTRA_CODE
			#endif

			bool CInFile::Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
			{
				bool res = Create(fileName, GENERIC_READ, shareMode, creationDisposition, flagsAndAttributes);
				MY_DEVICE_EXTRA_CODE
				return res;
			}
			bool CInFile::OpenShared(CFSTR fileName, bool shareForWrite)
			{
				return Open(fileName, FILE_SHARE_READ | (shareForWrite ? FILE_SHARE_WRITE : 0), OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
			}
			bool CInFile::Open(CFSTR fileName)
			{
				return OpenShared(fileName, false);
			}
			//
			// ReadFile and WriteFile functions in Windows have BUG:
			// If you Read or Write 64MB or more (probably min_failure_size = 64MB - 32KB + 1)
			// from/to Network file, it returns ERROR_NO_SYSTEM_RESOURCES
			// (Insufficient system resources exist to complete the requested service).
			//
			// Probably in some version of Windows there are problems with other sizes:
			// for 32 MB (maybe also for 16 MB).
			// And message can be "Network connection was lost"
			//
			static uint32 kChunkSizeMax = (1 << 22);

			bool CInFile::Read1(void * data, uint32 size, uint32 &processedSize) throw()
			{
				DWORD processedLoc = 0;
				bool res = BOOLToBool(::ReadFile(_handle, data, size, &processedLoc, NULL));
				processedSize = (uint32)processedLoc;
				return res;
			}
			bool CInFile::ReadPart(void * data, uint32 size, uint32 &processedSize) throw()
			{
				SETMIN(size, kChunkSizeMax);
				return Read1(data, size, processedSize);
			}
			bool CInFile::Read(void * data, uint32 size, uint32 &processedSize) throw()
			{
				processedSize = 0;
				do {
					uint32 processedLoc = 0;
					bool res = ReadPart(data, size, processedLoc);
					processedSize += processedLoc;
					if(!res)
						return false;
					if(processedLoc == 0)
						return true;
					data = (void *)((uchar *)data + processedLoc);
					size -= processedLoc;
				} while(size > 0);
				return true;
			}
			//
			// COutFile
			//
			static inline DWORD GetCreationDisposition(bool createAlways) 
				{ return createAlways ? CREATE_ALWAYS : CREATE_NEW; }
			bool COutFile::Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
				{ return CFileBase::Create(fileName, GENERIC_WRITE, shareMode, creationDisposition, flagsAndAttributes); }
			bool COutFile::Open(CFSTR fileName, DWORD creationDisposition)
				{ return Open(fileName, FILE_SHARE_READ, creationDisposition, FILE_ATTRIBUTE_NORMAL); }
			bool COutFile::Create(CFSTR fileName, bool createAlways)
				{ return Open(fileName, GetCreationDisposition(createAlways)); }
			bool COutFile::CreateAlways(CFSTR fileName, DWORD flagsAndAttributes)
				{ return Open(fileName, FILE_SHARE_READ, GetCreationDisposition(true), flagsAndAttributes); }
			bool COutFile::SetTime(const FILETIME * cTime, const FILETIME * aTime, const FILETIME * mTime) throw()
				{ return BOOLToBool(::SetFileTime(_handle, cTime, aTime, mTime)); }
			bool COutFile::SetMTime(const FILETIME * mTime) throw() 
				{ return SetTime(NULL, NULL, mTime); }
			bool COutFile::WritePart(const void * data, uint32 size, uint32 &processedSize) throw()
			{
				SETMIN(size, kChunkSizeMax);
				DWORD processedLoc = 0;
				bool res = BOOLToBool(::WriteFile(_handle, data, size, &processedLoc, NULL));
				processedSize = (uint32)processedLoc;
				return res;
			}
			bool COutFile::Write(const void * data, uint32 size, uint32 &processedSize) throw()
			{
				processedSize = 0;
				do {
					uint32 processedLoc = 0;
					bool res = WritePart(data, size, processedLoc);
					processedSize += processedLoc;
					if(!res)
						return false;
					if(processedLoc == 0)
						return true;
					data = (const void *)((const uchar *)data + processedLoc);
					size -= processedLoc;
				} while(size > 0);
				return true;
			}
			bool COutFile::SetEndOfFile() throw() 
			{
				return BOOLToBool(::SetEndOfFile(_handle));
			}
			bool COutFile::SetLength(uint64 length) throw()
			{
				uint64 newPosition;
				if(!Seek(length, newPosition))
					return false;
				else if(newPosition != length)
					return false;
				else
					return SetEndOfFile();
			}
		#ifndef UNDER_CE
			bool GetReparseData(CFSTR path, CByteBuffer &reparseData, BY_HANDLE_FILE_INFORMATION * fileInfo)
			{
				reparseData.Free();
				CInFile file;
				if(!file.OpenReparse(path))
					return false;
				if(fileInfo)
					file.GetFileInformation(fileInfo);
				const uint kBufSize = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
				CByteArr buf(kBufSize);
				DWORD returnedSize;
				if(!file.DeviceIoControlOut(my_FSCTL_GET_REPARSE_POINT, buf, kBufSize, &returnedSize))
					return false;
				reparseData.CopyFrom(buf, returnedSize);
				return true;
			}
			static bool CreatePrefixDirOfFile(CFSTR path)
			{
				FString path2(path);
				int pos = path2.ReverseFind_PathSepar();
				if(pos < 0)
					return true;
			  #ifdef _WIN32
				if(pos == 2 && path2[1] == L':')
					return true;  // we don't create Disk folder;
			  #endif
				path2.DeleteFrom(pos);
				return NDir::CreateComplexDir(path2);
			}
			// If there is Reprase data already, it still writes new Reparse data
			bool SetReparseData(CFSTR path, bool isDir, const void * data, DWORD size)
			{
				NFile::NFind::CFileInfo fi;
				if(fi.Find(path)) {
					if(fi.IsDir() != isDir) {
						::SetLastError(ERROR_DIRECTORY);
						return false;
					}
				}
				else {
					if(isDir) {
						if(!NDir::CreateComplexDir(path))
							return false;
					}
					else {
						CreatePrefixDirOfFile(path);
						COutFile file;
						if(!file.Create(path, CREATE_NEW))
							return false;
					}
				}
				COutFile file;
				if(!file.Open(path,FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS))
					return false;
				DWORD returnedSize;
				if(!file.DeviceIoControl(my_FSCTL_SET_REPARSE_POINT, (void *)data, size, NULL, 0, &returnedSize))
					return false;
				return true;
			}
		#endif
		}
		namespace NDir {
			#ifndef UNDER_CE
				bool GetWindowsDir(FString &path)
				{
					UINT needLength;
				  #ifndef _UNICODE
					if(!g_IsNT) {
						TCHAR s[MAX_PATH+2];
						s[0] = 0;
						needLength = ::GetWindowsDirectory(s, MAX_PATH + 1);
						path = fas2fs(s);
					}
					else
				  #endif
					{
						WCHAR s[MAX_PATH+2];
						s[0] = 0;
						needLength = ::GetWindowsDirectoryW(s, MAX_PATH + 1);
						path = us2fs(s);
					}
					return (needLength > 0 && needLength <= MAX_PATH);
				}

				bool GetSystemDir(FString &path)
				{
					UINT needLength;
				  #ifndef _UNICODE
					if(!g_IsNT) {
						TCHAR s[MAX_PATH+2];
						s[0] = 0;
						needLength = ::GetSystemDirectory(s, MAX_PATH + 1);
						path = fas2fs(s);
					}
					else
				  #endif
					{
						WCHAR s[MAX_PATH+2];
						s[0] = 0;
						needLength = ::GetSystemDirectoryW(s, MAX_PATH + 1);
						path = us2fs(s);
					}
					return (needLength > 0 && needLength <= MAX_PATH);
				}
			#endif

			bool SetDirTime(CFSTR path, const FILETIME * cTime, const FILETIME * aTime, const FILETIME * mTime)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
					return false;
				}
			  #endif
				HANDLE hDir = INVALID_HANDLE_VALUE;
				IF_USE_MAIN_PATH
					hDir = ::CreateFileW(fs2us(path), GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			  #ifdef WIN_LONG_PATH
				if(hDir == INVALID_HANDLE_VALUE && USE_SUPER_PATH) {
					UString superPath;
					if(GetSuperPath(path, superPath, USE_MAIN_PATH))
						hDir = ::CreateFileW(superPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				}
			  #endif
				bool res = false;
				if(hDir != INVALID_HANDLE_VALUE) {
					res = BOOLToBool(::SetFileTime(hDir, cTime, aTime, mTime));
					::CloseHandle(hDir);
				}
				return res;
			}
			bool SetFileAttrib(CFSTR path, DWORD attrib)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					if(::SetFileAttributes(fs2fas(path), attrib))
						return true;
				}
				else
			  #endif
				{
					IF_USE_MAIN_PATH
					if(::SetFileAttributesW(fs2us(path), attrib))
						return true;
				#ifdef WIN_LONG_PATH
					if(USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							return BOOLToBool(::SetFileAttributesW(superPath, attrib));
					}
				#endif
				}
				return false;
			}
			bool RemoveDir(CFSTR path)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					if(::RemoveDirectory(fs2fas(path)))
						return true;
				}
				else
			  #endif
				{
					IF_USE_MAIN_PATH
					if(::RemoveDirectoryW(fs2us(path)))
						return true;
				#ifdef WIN_LONG_PATH
					if(USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							return BOOLToBool(::RemoveDirectoryW(superPath));
					}
				#endif
				}
				return false;
			}

			bool MyMoveFile(CFSTR oldFile, CFSTR newFile)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					if(::MoveFile(fs2fas(oldFile), fs2fas(newFile)))
						return true;
				}
				else
			  #endif
				{
					IF_USE_MAIN_PATH_2(oldFile, newFile)
					if(::MoveFileW(fs2us(oldFile), fs2us(newFile)))
						return true;
				#ifdef WIN_LONG_PATH
					if(USE_SUPER_PATH_2) {
						UString d1, d2;
						if(GetSuperPaths(oldFile, newFile, d1, d2, USE_MAIN_PATH_2))
							return BOOLToBool(::MoveFileW(d1, d2));
					}
				#endif
				}
				return false;
			}

			#ifndef UNDER_CE
			EXTERN_C_BEGIN
			typedef BOOL (WINAPI *Func_CreateHardLinkW)(LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
			EXTERN_C_END

			bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
					return false;
					/*
					   if(::CreateHardLink(fs2fas(newFileName), fs2fas(existFileName), NULL))
					   return true;
					 */
				}
				else
			  #endif
				{
					Func_CreateHardLinkW my_CreateHardLinkW = (Func_CreateHardLinkW)::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "CreateHardLinkW");
					if(!my_CreateHardLinkW)
						return false;
					IF_USE_MAIN_PATH_2(newFileName, existFileName)
					if(my_CreateHardLinkW(fs2us(newFileName), fs2us(existFileName), NULL))
						return true;
				#ifdef WIN_LONG_PATH
					if(USE_SUPER_PATH_2) {
						UString d1, d2;
						if(GetSuperPaths(newFileName, existFileName, d1, d2, USE_MAIN_PATH_2))
							return BOOLToBool(my_CreateHardLinkW(d1, d2, NULL));
					}
				#endif
				}
				return false;
			}

			#endif

			/*
			   WinXP-64 CreateDir():
			   ""                  - ERROR_PATH_NOT_FOUND
			 \                   - ERROR_ACCESS_DENIED
			   C:\                 - ERROR_ACCESS_DENIED, if there is such drive,

			   D:\folder             - ERROR_PATH_NOT_FOUND, if there is no such drive,
			   C:\nonExistent\folder - ERROR_PATH_NOT_FOUND

			   C:\existFolder      - ERROR_ALREADY_EXISTS
			   C:\existFolder\     - ERROR_ALREADY_EXISTS

			   C:\folder   - OK
			   C:\folder\  - OK

			 \\\Server\nonExistent    - ERROR_BAD_NETPATH
			 \\\Server\Share_Readonly - ERROR_ACCESS_DENIED
			 \\\Server\Share          - ERROR_ALREADY_EXISTS

			 \\\Server\Share_NTFS_drive - ERROR_ACCESS_DENIED
			 \\\Server\Share_FAT_drive  - ERROR_ALREADY_EXISTS
			 */

			bool CreateDir(CFSTR path)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					if(::CreateDirectory(fs2fas(path), NULL))
						return true;
				}
				else
			  #endif
				{
					IF_USE_MAIN_PATH
					if(::CreateDirectoryW(fs2us(path), NULL))
						return true;
				#ifdef WIN_LONG_PATH
					if((!USE_MAIN_PATH || ::GetLastError() != ERROR_ALREADY_EXISTS) && USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							return BOOLToBool(::CreateDirectoryW(superPath, NULL));
					}
				#endif
				}
				return false;
			}
			// 
			// CreateDir2 returns true, if directory can contain files after the call (two cases):
			//   1) the directory already exists
			//   2) the directory was created path must be WITHOUT trailing path separator.
			// 
			// We need CreateDir2, since fileInfo.Find() for reserved names like "com8"
			// returns FILE instead of DIRECTORY. And we need to use SuperPath 
			// 
			static bool CreateDir2(CFSTR path)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					if(::CreateDirectory(fs2fas(path), NULL))
						return true;
				}
				else
			  #endif
				{
					IF_USE_MAIN_PATH
					if(::CreateDirectoryW(fs2us(path), NULL))
						return true;
				#ifdef WIN_LONG_PATH
					if((!USE_MAIN_PATH || ::GetLastError() != ERROR_ALREADY_EXISTS) && USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH)) {
							if(::CreateDirectoryW(superPath, NULL))
								return true;
							if(::GetLastError() != ERROR_ALREADY_EXISTS)
								return false;
							NFind::CFileInfo fi;
							if(!fi.Find(us2fs(superPath)))
								return false;
							return fi.IsDir();
						}
					}
				#endif
				}
				if(::GetLastError() != ERROR_ALREADY_EXISTS)
					return false;
				NFind::CFileInfo fi;
				return fi.Find(path) ? fi.IsDir() : false;
			}
			bool CreateComplexDir(CFSTR _path)
			{
			  #ifdef _WIN32
				{
					DWORD attrib = NFind::GetFileAttrib(_path);
					if(attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
						return true;
				}
			  #ifndef UNDER_CE
				if(IsDriveRootPath_SuperAllowed(_path))
					return false;
				uint   prefixSize = GetRootPrefixSize(_path);
			  #endif
			  #endif
				FString path(_path);
				int pos = path.ReverseFind_PathSepar();
				if(pos >= 0 && (uint)pos == path.Len() - 1) {
					if(path.Len() == 1)
						return true;
					path.DeleteBack();
				}
				const FString path2(path);
				pos = path.Len();
				for(;;) {
					if(CreateDir2(path))
						break;
					if(::GetLastError() == ERROR_ALREADY_EXISTS)
						return false;
					pos = path.ReverseFind_PathSepar();
					if(pos < 0 || pos == 0)
						return false;

				#if defined(_WIN32) && !defined(UNDER_CE)
					if(pos == 1 && isdirslash(path[0]))
						return false;
					if(prefixSize >= (uint)pos + 1)
						return false;
				#endif
					path.DeleteFrom(pos);
				}
				while(pos < (int)path2.Len()) {
					int pos2 = NName::FindSepar(path2.Ptr(pos + 1));
					if(pos2 < 0)
						pos = path2.Len();
					else
						pos += 1 + pos2;
					path.SetFrom(path2, pos);
					if(!CreateDir(path))
						return false;
				}
				return true;
			}
			bool DeleteFileAlways(CFSTR path)
			{
				/* If alt stream, we also need to clear READ-ONLY attribute of main file before delete.
				   SetFileAttrib("name:stream", ) changes attributes of main file. */
				{
					DWORD attrib = NFind::GetFileAttrib(path);
					if(attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) == 0 && (attrib & FILE_ATTRIBUTE_READONLY) != 0) {
						if(!SetFileAttrib(path, attrib & ~FILE_ATTRIBUTE_READONLY))
							return false;
					}
				}
			  #ifndef _UNICODE
				if(!g_IsNT) {
					if(::DeleteFile(fs2fas(path)))
						return true;
				}
				else
			  #endif
				{
					/* DeleteFile("name::$DATA") deletes all alt streams (same as delete DeleteFile("name")).
					   Maybe it's better to open "name::$DATA" and clear data for unnamed stream? */
					IF_USE_MAIN_PATH
					if(::DeleteFileW(fs2us(path)))
						return true;
				#ifdef WIN_LONG_PATH
					if(USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							return BOOLToBool(::DeleteFileW(superPath));
					}
				#endif
				}
				return false;
			}
			bool RemoveDirWithSubItems(const FString &path)
			{
				bool needRemoveSubItems = true;
				{
					NFind::CFileInfo fi;
					if(!fi.Find(path))
						return false;
					if(!fi.IsDir()) {
						::SetLastError(ERROR_DIRECTORY);
						return false;
					}
					if(fi.HasReparsePoint())
						needRemoveSubItems = false;
				}
				if(needRemoveSubItems) {
					FString s(path);
					s.Add_PathSepar();
					const uint prefixSize = s.Len();
					NFind::CEnumerator enumerator;
					enumerator.SetDirPrefix(s);
					NFind::CFileInfo fi;
					while(enumerator.Next(fi)) {
						s.DeleteFrom(prefixSize);
						s += fi.Name;
						if(fi.IsDir()) {
							if(!RemoveDirWithSubItems(s))
								return false;
						}
						else if(!DeleteFileAlways(s))
							return false;
					}
				}
				return SetFileAttrib(path, 0) ? RemoveDir(path) : false;
			}

			#ifdef UNDER_CE
				bool MyGetFullPathName(CFSTR path, FString &resFullPath)
				{
					resFullPath = path;
					return true;
				}
			#else
				bool MyGetFullPathName(CFSTR path, FString &resFullPath) { return GetFullPath(path, resFullPath); }
				bool SetCurrentDir(CFSTR path)
				{
					// SetCurrentDirectory doesn't support \\?\ prefix
				  #ifndef _UNICODE
					if(!g_IsNT) {
						return BOOLToBool(::SetCurrentDirectory(fs2fas(path)));
					}
					else
				  #endif
					{
						return BOOLToBool(::SetCurrentDirectoryW(fs2us(path)));
					}
				}
				bool GetCurrentDir(FString &path)
				{
					path.Empty();
					DWORD needLength;
				  #ifndef _UNICODE
					if(!g_IsNT) {
						TCHAR s[MAX_PATH+2];
						s[0] = 0;
						needLength = ::GetCurrentDirectory(MAX_PATH + 1, s);
						path = fas2fs(s);
					}
					else
				  #endif
					{
						WCHAR s[MAX_PATH+2];
						s[0] = 0;
						needLength = ::GetCurrentDirectoryW(MAX_PATH + 1, s);
						path = us2fs(s);
					}
					return (needLength > 0 && needLength <= MAX_PATH);
				}
			#endif

			bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName)
			{
				bool res = MyGetFullPathName(path, resDirPrefix);
				if(!res)
					resDirPrefix = path;
				int pos = resDirPrefix.ReverseFind_PathSepar();
				resFileName = resDirPrefix.Ptr(pos + 1);
				resDirPrefix.DeleteFrom(pos + 1);
				return res;
			}
			bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix)
			{
				FString resFileName;
				return GetFullPathAndSplit(path, resDirPrefix, resFileName);
			}
			bool MyGetTempPath(FString &path)
			{
				path.Empty();
				DWORD needLength;
			  #ifndef _UNICODE
				if(!g_IsNT) {
					TCHAR s[MAX_PATH+2];
					s[0] = 0;
					needLength = ::GetTempPath(MAX_PATH + 1, s);
					path = fas2fs(s);
				}
				else
			  #endif
				{
					WCHAR s[MAX_PATH+2];
					s[0] = 0;
					needLength = ::GetTempPathW(MAX_PATH + 1, s);;
					path = us2fs(s);
				}
				return (needLength > 0 && needLength <= MAX_PATH);
			}
			static bool CreateTempFile(CFSTR prefix, bool addRandom, FString &path, NIO::COutFile * outFile)
			{
				uint32 d = (GetTickCount() << 12) ^ (GetCurrentThreadId() << 14) ^ GetCurrentProcessId();
				for(uint i = 0; i < 100; i++) {
					path = prefix;
					if(addRandom) {
						char s[16];
						uint32 val = d;
						uint   k;
						for(k = 0; k < 8; k++) {
							uint   t = val & 0xF;
							val >>= 4;
							s[k] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
						}
						s[k] = '\0';
						if(outFile)
							path += '.';
						path += s;
						uint32 step = GetTickCount() + 2;
						if(step == 0)
							step = 1;
						d += step;
					}
					addRandom = true;
					if(outFile)
						path += ".tmp";
					if(NFind::DoesFileOrDirExist(path)) {
						SetLastError(ERROR_ALREADY_EXISTS);
						continue;
					}
					if(outFile) {
						if(outFile->Create(path, false))
							return true;
					}
					else {
						if(CreateDir(path))
							return true;
					}
					DWORD error = GetLastError();
					if(error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
						break;
				}
				path.Empty();
				return false;
			}
			bool CTempFile::Create(CFSTR prefix, NIO::COutFile * outFile)
			{
				if(!Remove())
					return false;
				if(!CreateTempFile(prefix, false, _path, outFile))
					return false;
				_mustBeDeleted = true;
				return true;
			}
			bool CTempFile::CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile * outFile)
			{
				if(!Remove())
					return false;
				FString tempPath;
				if(!MyGetTempPath(tempPath))
					return false;
				if(!CreateTempFile(tempPath + namePrefix, true, _path, outFile))
					return false;
				_mustBeDeleted = true;
				return true;
			}
			bool CTempFile::Remove()
			{
				if(!_mustBeDeleted)
					return true;
				_mustBeDeleted = !DeleteFileAlways(_path);
				return !_mustBeDeleted;
			}
			bool CTempFile::MoveTo(CFSTR name, bool deleteDestBefore)
			{
				if(deleteDestBefore)
					if(NFind::DoesFileExist(name))
						if(!DeleteFileAlways(name))
							return false;
				DisableDeleting();
				return MyMoveFile(_path, name);
			}
			bool CTempDir::Create(CFSTR prefix)
			{
				if(!Remove())
					return false;
				FString tempPath;
				if(!MyGetTempPath(tempPath))
					return false;
				if(!CreateTempFile(tempPath + prefix, true, _path, NULL))
					return false;
				_mustBeDeleted = true;
				return true;
			}
			bool CTempDir::Remove()
			{
				if(!_mustBeDeleted)
					return true;
				_mustBeDeleted = !RemoveDirWithSubItems(_path);
				return !_mustBeDeleted;
			}
		}
		namespace NName {
			//#define IS_SEPAR(c) IS_PATH_SEPAR(c)

			int FASTCALL FindSepar(const wchar_t * s) throw()
			{
				for(const wchar_t * p = s;; p++) {
					const wchar_t c = *p;
					if(c == 0)
						return -1;
					if(isdirslash(c))
						return (int)(p - s);
				}
			}

			void FASTCALL NormalizeDirPathPrefix(UString &dirPath)
			{
				if(!dirPath.IsEmpty()) {
					if(!IsPathSepar(dirPath.Back()))
						dirPath.Add_PathSepar();
				}
			}
			#ifndef USE_UNICODE_FSTRING
				int FASTCALL FindSepar(const FChar * s) throw()
				{
					for(const FChar * p = s;; p++) {
						const FChar c = *p;
						if(c == 0)
							return -1;
						if(isdirslash(c))
							return (int)(p - s);
					}
				}
				void FASTCALL NormalizeDirPathPrefix(FString &dirPath)
				{
					if(!dirPath.IsEmpty()) {
						if(!IsPathSepar(dirPath.Back()))
							dirPath.Add_PathSepar();
					}
				}
			#endif
			//#define IS_LETTER_CHAR(c) ((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z')

			bool IsDrivePath(const wchar_t * s) throw() 
			{
				return isasciialpha(s[0]) && s[1] == ':' && isdirslash(s[2]);
			}
			bool IsAltPathPrefix(CFSTR s) throw()
			{
				uint   len = sstrlen(s);
				if(!len)
					return false;
				if(s[len - 1] != ':')
					return false;
			  #if defined(_WIN32) && !defined(UNDER_CE)
				if(IsDevicePath(s))
					return false;
				if(IsSuperPath(s)) {
					s += kSuperPathPrefixSize;
					len -= kSuperPathPrefixSize;
				}
				if(len == 2 && IsDrivePath2(s))
					return false;
			  #endif

				return true;
			}

			#if defined(_WIN32) && !defined(UNDER_CE)

			const char * const kSuperPathPrefix = "\\\\?\\";
			static const char * const kSuperUncPrefix = "\\\\?\\UNC\\";

			#define IS_DEVICE_PATH(s)          (isdirslash((s)[0]) && isdirslash((s)[1]) && (s)[2] == '.' && isdirslash((s)[3]))
			#define IS_SUPER_PREFIX(s)         (isdirslash((s)[0]) && isdirslash((s)[1]) && (s)[2] == '?' && isdirslash((s)[3]))
			#define IS_SUPER_OR_DEVICE_PATH(s) (isdirslash((s)[0]) && isdirslash((s)[1]) && ((s)[2] == '?' || (s)[2] == '.') && isdirslash((s)[3]))
			#define IS_UNC_WITH_SLASH(s) (((s)[0] == 'U' || (s)[0] == 'u') && ((s)[1] == 'N' || (s)[1] == 'n') && ((s)[2] == 'C' || (s)[2] == 'c') && isdirslash((s)[3]))

			bool IsDevicePath(CFSTR s) throw()
			{
			  #ifdef UNDER_CE
				s = s;
				return false;
				/*
				   // actually we don't know the way to open device file in WinCE.
				   uint   len = sstrlen(s);
				   if(len < 5 || len > 5 || !IsString1PrefixedByString2(s, "DSK"))
				   return false;
				   if(s[4] != ':')
				   return false;
				   // for reading use SG_REQ sg; if(DeviceIoControl(dsk, IOCTL_DISK_READ));
				 */
			  #else
				if(!IS_DEVICE_PATH(s))
					return false;
				uint   len = sstrlen(s);
				if(len == 6 && s[5] == ':')
					return true;
				if(len < 18 || len > 22 || !IsString1PrefixedByString2(s + kDevicePathPrefixSize, "PhysicalDrive"))
					return false;
				for(uint i = 17; i < len; i++)
					if(s[i] < '0' || s[i] > '9')
						return false;
				return true;

			  #endif
			}

			bool IsSuperUncPath(CFSTR s) throw() { return (IS_SUPER_PREFIX(s) && IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize)); }

			bool IsNetworkPath(CFSTR s) throw()
			{
				if(!isdirslash(s[0]) || !isdirslash(s[1]))
					return false;
				if(IsSuperUncPath(s))
					return true;
				FChar c = s[2];
				return (c != '.' && c != '?');
			}
			unsigned GetNetworkServerPrefixSize(CFSTR s) throw()
			{
				if(!isdirslash(s[0]) || !isdirslash(s[1]))
					return 0;
				uint   prefixSize = 2;
				if(IsSuperUncPath(s))
					prefixSize = kSuperUncPathPrefixSize;
				else {
					FChar c = s[2];
					if(c == '.' || c == '?')
						return 0;
				}
				int pos = FindSepar(s + prefixSize);
				if(pos < 0)
					return 0;
				return prefixSize + pos + 1;
			}
			bool IsNetworkShareRootPath(CFSTR s) throw()
			{
				uint   prefixSize = GetNetworkServerPrefixSize(s);
				if(prefixSize == 0)
					return false;
				s += prefixSize;
				int pos = FindSepar(s);
				if(pos < 0)
					return true;
				return s[(uint)pos + 1] == 0;
			}

			static const uint kDrivePrefixSize = 3; /* c:\ */

			bool IsDrivePath2(const wchar_t * s) throw() { return isasciialpha(s[0]) && s[1] == ':'; }
			// bool IsDriveName2(const wchar_t *s) throw() { return isasciialpha(s[0]) && s[1] == ':' && s[2] == 0; }
			bool IsSuperPath(const wchar_t * s) throw() { return IS_SUPER_PREFIX(s); }
			bool IsSuperOrDevicePath(const wchar_t * s) throw() { return IS_SUPER_OR_DEVICE_PATH(s); }

			// bool IsSuperUncPath(const wchar_t *s) throw() { return (IS_SUPER_PREFIX(s) && IS_UNC_WITH_SLASH(s +
			// kSuperPathPrefixSize)); }

			#ifndef USE_UNICODE_FSTRING
				bool IsDrivePath2(CFSTR s) throw() { return isasciialpha(s[0]) && s[1] == ':'; }
				// bool IsDriveName2(CFSTR s) throw() { return isasciialpha(s[0]) && s[1] == ':' && s[2] == 0; }
				bool IsDrivePath(CFSTR s) throw() { return isasciialpha(s[0]) && s[1] == ':' && isdirslash(s[2]); }
				bool IsSuperPath(CFSTR s) throw() { return IS_SUPER_PREFIX(s); }
				bool IsSuperOrDevicePath(CFSTR s) throw() { return IS_SUPER_OR_DEVICE_PATH(s); }
			#endif // USE_UNICODE_FSTRING

			bool IsDrivePath_SuperAllowed(CFSTR s) throw()
			{
				if(IsSuperPath(s))
					s += kSuperPathPrefixSize;
				return IsDrivePath(s);
			}
			bool IsDriveRootPath_SuperAllowed(CFSTR s) throw()
			{
				if(IsSuperPath(s))
					s += kSuperPathPrefixSize;
				return IsDrivePath(s) && s[kDrivePrefixSize] == 0;
			}
			bool IsAbsolutePath(const wchar_t * s) throw()
			{
				return isdirslash(s[0]) || IsDrivePath2(s);
			}
			int FindAltStreamColon(CFSTR path) throw()
			{
				uint   i = 0;
				if(IsDrivePath2(path))
					i = 2;
				int colonPos = -1;
				for(;; i++) {
					FChar c = path[i];
					if(c == 0)
						return colonPos;
					if(c == ':') {
						if(colonPos < 0)
							colonPos = i;
						continue;
					}
					if(isdirslash(c))
						colonPos = -1;
				}
			}
			#ifndef USE_UNICODE_FSTRING
				static unsigned GetRootPrefixSize_Of_NetworkPath(CFSTR s)
				{
					// Network path: we look "server\path\" as root prefix
					int pos = FindSepar(s);
					if(pos < 0)
						return 0;
					int pos2 = FindSepar(s + (uint)pos + 1);
					if(pos2 < 0)
						return 0;
					return pos + pos2 + 2;
				}
				static unsigned GetRootPrefixSize_Of_SimplePath(CFSTR s)
				{
					if(IsDrivePath(s))
						return kDrivePrefixSize;
					if(!isdirslash(s[0]))
						return 0;
					if(s[1] == 0 || !isdirslash(s[1]))
						return 1;
					uint size = GetRootPrefixSize_Of_NetworkPath(s + 2);
					return (size == 0) ? 0 : 2 + size;
				}
				static unsigned GetRootPrefixSize_Of_SuperPath(CFSTR s)
				{
					if(IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize)) {
						uint size = GetRootPrefixSize_Of_NetworkPath(s + kSuperUncPathPrefixSize);
						return (size == 0) ? 0 : kSuperUncPathPrefixSize + size;
					}
					// we support \\?\c:\ paths and volume GUID paths \\?\Volume{GUID}\"
					int pos = FindSepar(s + kSuperPathPrefixSize);
					if(pos < 0)
						return 0;
					return kSuperPathPrefixSize + pos + 1;
				}
				unsigned GetRootPrefixSize(CFSTR s) throw()
				{
					if(IS_DEVICE_PATH(s))
						return kDevicePathPrefixSize;
					else if(IsSuperPath(s))
						return GetRootPrefixSize_Of_SuperPath(s);
					else
						return GetRootPrefixSize_Of_SimplePath(s);
				}
			#endif // USE_UNICODE_FSTRING

			static unsigned GetRootPrefixSize_Of_NetworkPath(const wchar_t * s) throw()
			{
				// Network path: we look "server\path\" as root prefix
				int pos = FindSepar(s);
				if(pos < 0)
					return 0;
				int pos2 = FindSepar(s + (uint)pos + 1);
				if(pos2 < 0)
					return 0;
				return pos + pos2 + 2;
			}
			static unsigned GetRootPrefixSize_Of_SimplePath(const wchar_t * s) throw()
			{
				if(IsDrivePath(s))
					return kDrivePrefixSize;
				if(!isdirslash(s[0]))
					return 0;
				if(s[1] == 0 || !isdirslash(s[1]))
					return 1;
				uint   size = GetRootPrefixSize_Of_NetworkPath(s + 2);
				return (size == 0) ? 0 : 2 + size;
			}
			static unsigned GetRootPrefixSize_Of_SuperPath(const wchar_t * s) throw()
			{
				if(IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize)) {
					uint size = GetRootPrefixSize_Of_NetworkPath(s + kSuperUncPathPrefixSize);
					return (size == 0) ? 0 : kSuperUncPathPrefixSize + size;
				}
				// we support \\?\c:\ paths and volume GUID paths \\?\Volume{GUID}\"
				int pos = FindSepar(s + kSuperPathPrefixSize);
				if(pos < 0)
					return 0;
				return kSuperPathPrefixSize + pos + 1;
			}
			unsigned GetRootPrefixSize(const wchar_t * s) throw()
			{
				if(IS_DEVICE_PATH(s))
					return kDevicePathPrefixSize;
				else if(IsSuperPath(s))
					return GetRootPrefixSize_Of_SuperPath(s);
				else 
					return GetRootPrefixSize_Of_SimplePath(s);
			}
			#else // _WIN32
				bool IsAbsolutePath(const wchar_t * s) { return isdirslash(s[0]); }
				#ifndef USE_UNICODE_FSTRING
					unsigned GetRootPrefixSize(CFSTR s) { return isdirslash(s[0]) ? 1 : 0; }
				#endif
				unsigned GetRootPrefixSize(const wchar_t * s) { return isdirslash(s[0]) ? 1 : 0; }
			#endif // _WIN32

			#ifndef UNDER_CE

			static bool GetCurDir(UString &path)
			{
				path.Empty();
				DWORD needLength;
			  #ifndef _UNICODE
				if(!g_IsNT) {
					TCHAR s[MAX_PATH+2];
					s[0] = 0;
					needLength = ::GetCurrentDirectory(MAX_PATH + 1, s);
					path = fs2us(fas2fs(s));
				}
				else
			  #endif
				{
					WCHAR s[MAX_PATH+2];
					s[0] = 0;
					needLength = ::GetCurrentDirectoryW(MAX_PATH + 1, s);
					path = s;
				}
				return (needLength > 0 && needLength <= MAX_PATH);
			}

			static bool ResolveDotsFolders(UString &s)
			{
			  #ifdef _WIN32
				// s.Replace(L'/', WCHAR_PATH_SEPARATOR);
			  #endif

				for(uint i = 0;;) {
					const wchar_t c = s[i];
					if(c == 0)
						return true;
					if(c == '.' && (i == 0 || isdirslash(s[i - 1]))) {
						const wchar_t c1 = s[i+1];
						if(c1 == '.') {
							const wchar_t c2 = s[i+2];
							if(isdirslash(c2) || c2 == 0) {
								if(i == 0)
									return false;
								int k = i - 2;
								i += 2;

								for(;; k--) {
									if(k < 0)
										return false;
									if(!isdirslash(s[(uint)k]))
										break;
								}
								do {
									k--;
								} while(k >= 0 && !isdirslash(s[(uint)k]));
								unsigned num;
								if(k >= 0) {
									num = i - k;
									i = k;
								}
								else {
									num = (c2 == 0 ? i : (i + 1));
									i = 0;
								}
								s.Delete(i, num);
								continue;
							}
						}
						else if(isdirslash(c1) || c1 == 0) {
							unsigned num = 2;
							if(i != 0)
								i--;
							else if(c1 == 0)
								num = 1;
							s.Delete(i, num);
							continue;
						}
					}
					i++;
				}
			}

			#endif // UNDER_CE

			#define LONG_PATH_DOTS_FOLDERS_PARSING
			/*
			   Windows (at least 64-bit XP) can't resolve "." or ".." in paths that start with SuperPrefix \\?\
			   To solve that problem we check such path:
			   - super path contains        "." or ".." - we use kSuperPathType_UseOnlySuper
			   - super path doesn't contain "." or ".." - we use kSuperPathType_UseOnlyMain
			 */
			#ifdef LONG_PATH_DOTS_FOLDERS_PARSING
			#ifndef UNDER_CE
			static bool FASTCALL AreThereDotsFolders(CFSTR s)
			{
				for(uint i = 0;; i++) {
					FChar c = s[i];
					if(c == 0)
						return false;
					if(c == '.' && (i == 0 || isdirslash(s[i - 1]))) {
						FChar c1 = s[i+1];
						if(c1 == 0 || isdirslash(c1) || (c1 == '.' && (s[i+2] == 0 || isdirslash(s[i+2]))))
							return true;
					}
				}
			}
			#endif
			#endif // LONG_PATH_DOTS_FOLDERS_PARSING

			#ifdef WIN_LONG_PATH

			/*
			   Most of Windows versions have problems, if some file or dir name
			   contains '.' or ' ' at the end of name (Bad Path).
			   To solve that problem, we always use Super Path ("\\?\" prefix and full path)
			   in such cases. Note that "." and ".." are not bad names.

			   There are 3 cases:
			   1) If the path is already Super Path, we use that path
			   2) If the path is not Super Path :
				 2.1) Bad Path;  we use only Super Path.
				 2.2) Good Path; we use Main Path. If it fails, we use Super Path.

			   NeedToUseOriginalPath returns:
				kSuperPathType_UseOnlyMain    : Super already
				kSuperPathType_UseOnlySuper    : not Super, Bad Path
				kSuperPathType_UseMainAndSuper : not Super, Good Path
			 */

			int GetUseSuperPathType(CFSTR s) throw()
			{
				if(IsSuperOrDevicePath(s)) {
				#ifdef LONG_PATH_DOTS_FOLDERS_PARSING
					if((s)[2] != '.')
						if(AreThereDotsFolders(s + kSuperPathPrefixSize))
							return kSuperPathType_UseOnlySuper;
				#endif
					return kSuperPathType_UseOnlyMain;
				}

				for(uint i = 0;; i++) {
					FChar c = s[i];
					if(c == 0)
						return kSuperPathType_UseMainAndSuper;
					if(c == '.' || c == ' ') {
						FChar c2 = s[i+1];
						if(c2 == 0 || isdirslash(c2)) {
							// if it's "." or "..", it's not bad name.
							if(c == '.') {
								if(i == 0 || isdirslash(s[i - 1]))
									continue;
								if(s[i - 1] == '.') {
									if(i - 1 == 0 || isdirslash(s[i - 2]))
										continue;
								}
							}
							return kSuperPathType_UseOnlySuper;
						}
					}
				}
			}

			/*
			   returns false in two cases:
				 - if GetCurDir was used, and GetCurDir returned error.
				 - if we can't resolve ".." name.
			   if path is ".", "..", res is empty.
			   if it's Super Path already, res is empty.
			   for \**** , and if GetCurDir is not drive (c:\), res is empty
			   for absolute paths, returns true, res is Super path.
			 */
			static bool GetSuperPathBase(CFSTR s, UString &res)
			{
				res.Empty();
				FChar c = s[0];
				if(c == 0)
					return true;
				if(c == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
					return true;
				if(IsSuperOrDevicePath(s)) {
			#ifdef LONG_PATH_DOTS_FOLDERS_PARSING
					if((s)[2] == '.')
						return true;
					// we will return true here, so we will try to use these problem paths.
					if(!AreThereDotsFolders(s + kSuperPathPrefixSize))
						return true;
					UString temp = fs2us(s);
					unsigned fixedSize = GetRootPrefixSize_Of_SuperPath(temp);
					if(fixedSize == 0)
						return true;
					UString rem = &temp[fixedSize];
					if(!ResolveDotsFolders(rem))
						return true;
					temp.DeleteFrom(fixedSize);
					res += temp;
					res += rem;
				#endif
					return true;
				}
				if(isdirslash(c)) {
					if(isdirslash(s[1])) {
						UString temp = fs2us(s + 2);
						unsigned fixedSize = GetRootPrefixSize_Of_NetworkPath(temp);
						// we ignore that error to allow short network paths server\share?
						/*
						   if(fixedSize == 0)
						   return false;
						 */
						UString rem = &temp[fixedSize];
						if(!ResolveDotsFolders(rem))
							return false;
						res += kSuperUncPrefix;
						temp.DeleteFrom(fixedSize);
						res += temp;
						res += rem;
						return true;
					}
				}
				else {
					if(IsDrivePath2(s)) {
						UString temp = fs2us(s);
						unsigned prefixSize = 2;
						if(IsDrivePath(s))
							prefixSize = kDrivePrefixSize;
						UString rem = temp.Ptr(prefixSize);
						if(!ResolveDotsFolders(rem))
							return true;
						res += kSuperPathPrefix;
						temp.DeleteFrom(prefixSize);
						res += temp;
						res += rem;
						return true;
					}
				}

				UString curDir;
				if(!GetCurDir(curDir))
					return false;
				NormalizeDirPathPrefix(curDir);

				unsigned fixedSizeStart = 0;
				unsigned fixedSize = 0;
				const char * superMarker = NULL;
				if(IsSuperPath(curDir)) {
					fixedSize = GetRootPrefixSize_Of_SuperPath(curDir);
					if(fixedSize == 0)
						return false;
				}
				else {
					if(IsDrivePath(curDir)) {
						superMarker = kSuperPathPrefix;
						fixedSize = kDrivePrefixSize;
					}
					else {
						if(!IsPathSepar(curDir[0]) || !IsPathSepar(curDir[1]))
							return false;
						fixedSizeStart = 2;
						fixedSize = GetRootPrefixSize_Of_NetworkPath(curDir.Ptr(2));
						if(fixedSize == 0)
							return false;
						superMarker = kSuperUncPrefix;
					}
				}

				UString temp;
				if(isdirslash(c)) {
					temp = fs2us(s + 1);
				}
				else {
					temp += &curDir[fixedSizeStart + fixedSize];
					temp += fs2us(s);
				}
				if(!ResolveDotsFolders(temp))
					return false;
				if(superMarker)
					res += superMarker;
				res += curDir.Mid(fixedSizeStart, fixedSize);
				res += temp;
				return true;
			}
			/*
			   In that case if GetSuperPathBase doesn't return new path, we don't need
			   to use same path that was used as main path

			   GetSuperPathBase  superPath.IsEmpty() onlyIfNew
				 false * *     GetCurDir Error
				 true            false  *     use Super path
				 true            true             true        don't use any path, we already used mainPath
				 true            true             false       use main path as Super Path, we don't try mainMath
															  That case is possible now if GetCurDir returns unknow
															  type of path (not drive and not network)

			   We can change that code if we want to try mainPath, if GetSuperPathBase returns error,
			   and we didn't try mainPath still.
			   If we want to work that way, we don't need to use GetSuperPathBase return code.
			 */
			bool GetSuperPath(CFSTR path, UString &superPath, bool onlyIfNew)
			{
				if(GetSuperPathBase(path, superPath)) {
					if(superPath.IsEmpty()) {
						// actually the only possible when onlyIfNew == true and superPath is empty
						// is case when
						if(onlyIfNew)
							return false;
						superPath = fs2us(path);
					}
					return true;
				}
				return false;
			}
			bool GetSuperPaths(CFSTR s1, CFSTR s2, UString &d1, UString &d2, bool onlyIfNew)
			{
				if(!GetSuperPathBase(s1, d1) || !GetSuperPathBase(s2, d2))
					return false;
				if(d1.IsEmpty() && d2.IsEmpty() && onlyIfNew)
					return false;
				if(d1.IsEmpty()) d1 = fs2us(s1);
				if(d2.IsEmpty()) d2 = fs2us(s2);
				return true;
			}
			/*
			   // returns true, if we need additional use with New Super path.
			   bool GetSuperPath(CFSTR path, UString &superPath)
			   {
			   if(GetSuperPathBase(path, superPath))
				return !superPath.IsEmpty();
			   return false;
			   }
			 */
			#endif // WIN_LONG_PATH
			bool GetFullPath(CFSTR dirPrefix, CFSTR s, FString &res)
			{
				res = s;
			  #ifdef UNDER_CE
				if(!isdirslash(s[0])) {
					if(!dirPrefix)
						return false;
					res = dirPrefix;
					res += s;
				}
			  #else
				uint   prefixSize = GetRootPrefixSize(s);
				if(prefixSize != 0) {
					if(AreThereDotsFolders(s + prefixSize)) {
						UString rem = fs2us(s + prefixSize);
						if(!ResolveDotsFolders(rem))
							return true;  // maybe false;
						res.DeleteFrom(prefixSize);
						res += us2fs(rem);
					}
					return true;
				}
				/*
				   FChar c = s[0];
				   if(c == 0)
				   return true;
				   if(c == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
				   return true;
				   if(isdirslash(c) && isdirslash(s[1]))
				   return true;
				   if(IsDrivePath(s))
				   return true;
				 */
				UString curDir;
				if(dirPrefix)
					curDir = fs2us(dirPrefix);
				else {
					if(!GetCurDir(curDir))
						return false;
				}
				NormalizeDirPathPrefix(curDir);
				uint   fixedSize = 0;
			  #ifdef _WIN32
				if(IsSuperPath(curDir)) {
					fixedSize = GetRootPrefixSize_Of_SuperPath(curDir);
					if(fixedSize == 0)
						return false;
				}
				else {
					if(IsDrivePath(curDir))
						fixedSize = kDrivePrefixSize;
					else {
						if(!IsPathSepar(curDir[0]) || !IsPathSepar(curDir[1]))
							return false;
						fixedSize = GetRootPrefixSize_Of_NetworkPath(curDir.Ptr(2));
						if(fixedSize == 0)
							return false;
						fixedSize += 2;
					}
				}
			  #endif // _WIN32
				UString temp;
				if(isdirslash(s[0])) {
					temp = fs2us(s + 1);
				}
				else {
					temp += curDir.Ptr(fixedSize);
					temp += fs2us(s);
				}
				if(!ResolveDotsFolders(temp))
					return false;
				curDir.DeleteFrom(fixedSize);
				res = us2fs(curDir);
				res += us2fs(temp);
			  #endif // UNDER_CE
				return true;
			}
			bool GetFullPath(CFSTR path, FString & fullPath) { return GetFullPath(NULL, path, fullPath); }
		}
		/*
		   Reparse Points (Junctions and Symbolic Links):
		   struct
		   {
			uint32 Tag;
			uint16 Size;     // not including starting 8 bytes
			uint16 Reserved; // = 0

			uint16 SubstituteOffset; // offset in bytes from  start of namesChars
			uint16 SubstituteLen;    // size in bytes, it doesn't include tailed NUL
			uint16 PrintOffset;      // offset in bytes from  start of namesChars
			uint16 PrintLen;         // size in bytes, it doesn't include tailed NUL

			[uint32] Flags;  // for Symbolic Links only.

			uint16 namesChars[]
		   }

		   MOUNT_POINT (Junction point):
			1) there is NUL wchar after path
			2) Default Order in table:
				 Substitute Path
				 Print Path
			3) pathnames can not contain dot directory names

		   SYMLINK:
			1) there is no NUL wchar after path
			2) Default Order in table:
				 Print Path
				 Substitute Path
		 */

		/*
		   static const uint32 kReparseFlags_Alias       = (1 << 29);
		   static const uint32 kReparseFlags_HighLatency = (1 << 30);
		   static const uint32 kReparseFlags_Microsoft   = ((uint32)1 << 31);

		   #define _my_IO_REPARSE_TAG_HSM          (0xC0000004L)
		   #define _my_IO_REPARSE_TAG_HSM2         (0x80000006L)
		   #define _my_IO_REPARSE_TAG_SIS          (0x80000007L)
		   #define _my_IO_REPARSE_TAG_WIM          (0x80000008L)
		   #define _my_IO_REPARSE_TAG_CSV          (0x80000009L)
		   #define _my_IO_REPARSE_TAG_DFS          (0x8000000AL)
		   #define _my_IO_REPARSE_TAG_DFSR         (0x80000012L)
		 */

		#define Get16(p) GetUi16(p)
		#define Get32(p) GetUi32(p)

		#define Set16(p, v) SetUi16(p, v)
		#define Set32(p, v) SetUi32(p, v)

		static const wchar_t * const k_LinkPrefix = L"\\??\\";
		static const uint k_LinkPrefix_Size = 4;

		static const bool FASTCALL IsLinkPrefix(const wchar_t * s)
		{
			return IsString1PrefixedByString2(s, k_LinkPrefix);
		}

		/*
		   static const wchar_t * const k_VolumePrefix = L"Volume{";
		   static const bool IsVolumeName(const wchar_t *s)
		   {
		   return IsString1PrefixedByString2(s, k_VolumePrefix);
		   }
		 */
		static void FASTCALL WriteString(Byte * dest, const wchar_t * path)
		{
			for(;;) {
				wchar_t c = *path++;
				if(c == 0)
					return;
				Set16(dest, (uint16)c);
				dest += 2;
			}
		}

		#if defined(_WIN32) && !defined(UNDER_CE)
			bool FillLinkData(CByteBuffer &dest, const wchar_t * path, bool isSymLink)
			{
				bool isAbs = IsAbsolutePath(path);
				if(!isAbs && !isSymLink)
					return false;
				bool needPrintName = true;
				if(IsSuperPath(path)) {
					path += kSuperPathPrefixSize;
					if(!IsDrivePath(path))
						needPrintName = false;
				}
				const uint add_Prefix_Len = isAbs ? k_LinkPrefix_Size : 0;
				unsigned len2 = sstrlen(path) * 2;
				const uint len1 = len2 + add_Prefix_Len * 2;
				if(!needPrintName)
					len2 = 0;
				unsigned totalNamesSize = (len1 + len2);
				/* some WIM imagex software uses old scheme for symbolic links.
				   so we can old scheme for byte to byte compatibility */
				bool newOrderScheme = isSymLink;
				// newOrderScheme = false;
				if(!newOrderScheme)
					totalNamesSize += 2 * 2;
				const size_t size = 8 + 8 + (isSymLink ? 4 : 0) + totalNamesSize;
				dest.Alloc(size);
				memzero(dest, size);
				const uint32 tag = isSymLink ? _my_IO_REPARSE_TAG_SYMLINK : _my_IO_REPARSE_TAG_MOUNT_POINT;
				Byte * p = dest;
				Set32(p, tag);
				Set16(p + 4, static_cast<uint16>(size - 8));
				Set16(p + 6, 0);
				p += 8;
				unsigned subOffs = 0;
				unsigned printOffs = 0;
				if(newOrderScheme)
					subOffs = len2;
				else
					printOffs = len1 + 2;
				Set16(p + 0, (uint16)subOffs);
				Set16(p + 2, (uint16)len1);
				Set16(p + 4, (uint16)printOffs);
				Set16(p + 6, (uint16)len2);
				p += 8;
				if(isSymLink) {
					uint32 flags = isAbs ? 0 : _my_SYMLINK_FLAG_RELATIVE;
					Set32(p, flags);
					p += 4;
				}
				if(add_Prefix_Len != 0)
					WriteString(p + subOffs, k_LinkPrefix);
				WriteString(p + subOffs + add_Prefix_Len * 2, path);
				if(needPrintName)
					WriteString(p + printOffs, path);
				return true;
			}
		#endif

		static void GetString(const Byte * p, unsigned len, UString &res)
		{
			wchar_t * s = res.GetBuf(len);
			uint   i;
			for(i = 0; i < len; i++) {
				wchar_t c = Get16(p + i * 2);
				if(c == 0)
					break;
				s[i] = c;
			}
			s[i] = 0;
			res.ReleaseBuf_SetLen(i);
		}
		bool CReparseAttr::Parse(const Byte * p, size_t size)
		{
			if(size < 8)
				return false;
			Tag = Get32(p);
			uint32 len = Get16(p + 4);
			if(len + 8 > size)
				return false;
			/*
			   if((type & kReparseFlags_Alias) == 0 || (type & kReparseFlags_Microsoft) == 0 || (type & 0xFFFF) != 3)
			 */
			if(Tag != _my_IO_REPARSE_TAG_MOUNT_POINT && Tag != _my_IO_REPARSE_TAG_SYMLINK)
				// return true;
				return false;

			if(Get16(p + 6) != 0) // padding
				return false;
			p += 8;
			size -= 8;
			if(len != size) // do we need that check?
				return false;

			if(len < 8)
				return false;
			uint   subOffs = Get16(p);
			uint   subLen = Get16(p + 2);
			uint   printOffs = Get16(p + 4);
			uint   printLen = Get16(p + 6);
			len -= 8;
			p += 8;
			Flags = 0;
			if(Tag == _my_IO_REPARSE_TAG_SYMLINK) {
				if(len < 4)
					return false;
				Flags = Get32(p);
				len -= 4;
				p += 4;
			}
			if((subOffs & 1) != 0 || subOffs > len || len - subOffs < subLen)
				return false;
			if((printOffs & 1) != 0 || printOffs > len || len - printOffs < printLen)
				return false;
			GetString(p + subOffs, subLen >> 1, SubsName);
			GetString(p + printOffs, printLen >> 1, PrintName);
			return true;
		}
		bool CReparseShortInfo::Parse(const Byte * p, size_t size)
		{
			const Byte * start = p;
			Offset = 0;
			Size = 0;
			if(size < 8)
				return false;
			uint32 Tag = Get32(p);
			uint32 len = Get16(p + 4);
			if(len + 8 > size)
				return false;
			/*
			   if((type & kReparseFlags_Alias) == 0 || (type & kReparseFlags_Microsoft) == 0 || (type & 0xFFFF) != 3)
			 */
			if(Tag != _my_IO_REPARSE_TAG_MOUNT_POINT && Tag != _my_IO_REPARSE_TAG_SYMLINK) {
				// return true;
				return false;
			}
			if(Get16(p + 6) != 0) // padding
				return false;
			p += 8;
			size -= 8;
			if(len != size) // do we need that check?
				return false;
			if(len < 8)
				return false;
			uint   subOffs = Get16(p);
			uint   subLen = Get16(p + 2);
			uint   printOffs = Get16(p + 4);
			uint   printLen = Get16(p + 6);
			len -= 8;
			p += 8;
			// uint32 Flags = 0;
			if(Tag == _my_IO_REPARSE_TAG_SYMLINK) {
				if(len < 4)
					return false;
				// Flags = Get32(p);
				len -= 4;
				p += 4;
			}
			if((subOffs & 1) != 0 || subOffs > len || len - subOffs < subLen)
				return false;
			if((printOffs & 1) != 0 || printOffs > len || len - printOffs < printLen)
				return false;
			Offset = (uint)(p - start) + subOffs;
			Size = subLen;
			return true;
		}
		bool CReparseAttr::IsOkNamePair() const
		{
			if(IsLinkPrefix(SubsName)) {
				if(!IsDrivePath(SubsName.Ptr(k_LinkPrefix_Size)))
					return PrintName.IsEmpty();
				if(wcscmp(SubsName.Ptr(k_LinkPrefix_Size), PrintName) == 0)
					return true;
			}
			return wcscmp(SubsName, PrintName) == 0;
		}
		/*
		   bool CReparseAttr::IsVolume() const
		   {
		   if(!IsLinkPrefix(SubsName))
			return false;
		   return IsVolumeName(SubsName.Ptr(k_LinkPrefix_Size));
		   }
		 */

		UString CReparseAttr::GetPath() const
		{
			UString s(SubsName);
			if(IsLinkPrefix(s)) {
				s.ReplaceOneCharAtPos(1, '\\');
				if(IsDrivePath(s.Ptr(k_LinkPrefix_Size)))
					s.DeleteFrontal(k_LinkPrefix_Size);
			}
			return s;
		}
		namespace NFind {
			bool CFileInfo::IsDots() const throw()
			{
				if(!IsDir() || Name.IsEmpty())
					return false;
				else if(Name[0] != '.')
					return false;
				else
					return Name.Len() == 1 || (Name.Len() == 2 && Name[1] == '.');
			}

			#define WIN_FD_TO_MY_FI(fi, fd)	\
				fi.Attrib = fd.dwFileAttributes; \
				fi.CTime = fd.ftCreationTime; \
				fi.ATime = fd.ftLastAccessTime;	\
				fi.MTime = fd.ftLastWriteTime; \
				fi.Size = (((uint64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;	\
				fi.IsAltStream = false;	\
				fi.IsDevice = false;

			/*
			   #ifdef UNDER_CE
			   fi.ObjectID = fd.dwOID;
			   #else
			   fi.ReparseTag = fd.dwReserved0;
			   #endif
			 */

			static void Convert_WIN32_FIND_DATA_to_FileInfo(const WIN32_FIND_DATAW &fd, CFileInfo &fi)
			{
				WIN_FD_TO_MY_FI(fi, fd);
				fi.Name = us2fs(fd.cFileName);
			  #if defined(_WIN32) && !defined(UNDER_CE)
				// fi.ShortName = us2fs(fd.cAlternateFileName);
			  #endif
			}

			#ifndef _UNICODE
				static void Convert_WIN32_FIND_DATA_to_FileInfo(const WIN32_FIND_DATA &fd, CFileInfo &fi)
				{
					WIN_FD_TO_MY_FI(fi, fd);
					fi.Name = fas2fs(fd.cFileName);
				  #if defined(_WIN32) && !defined(UNDER_CE)
					// fi.ShortName = fas2fs(fd.cAlternateFileName);
				  #endif
				}
			#endif
			//
			// CFindFile
			//
			bool CFindFileBase::Close() throw()
			{
				if(_handle == INVALID_HANDLE_VALUE)
					return true;
				if(!::FindClose(_handle))
					return false;
				_handle = INVALID_HANDLE_VALUE;
				return true;
			}
			/*
			   WinXP-64 FindFirstFile():
			   ""      -  ERROR_PATH_NOT_FOUND
			   folder\ -  ERROR_FILE_NOT_FOUND
			 \       -  ERROR_FILE_NOT_FOUND
			   c:\     -  ERROR_FILE_NOT_FOUND
			   c:      -  ERROR_FILE_NOT_FOUND, if current dir is ROOT     ( c:\ )
			   c:      -  OK,                   if current dir is NOT ROOT ( c:\folder )
			   folder  -  OK

			 \\               - ERROR_INVALID_NAME
			 \\\\Server         - ERROR_INVALID_NAME
			 \\\\Server\        - ERROR_INVALID_NAME

			 \\\\Server\Share            - ERROR_BAD_NETPATH
			 \\\\Server\Share            - ERROR_BAD_NET_NAME (Win7).
						 !!! There is problem : Win7 makes some requests for "\\Server\Shar" (look in Procmon),
							 when we call it for "\\Server\Share"

			 \\\\Server\Share\           - ERROR_FILE_NOT_FOUND

			 \\\\?\UNC\Server\Share      - ERROR_INVALID_NAME
			 \\\\?\UNC\Server\Share      - ERROR_BAD_PATHNAME (Win7)
			 \\\\?\UNC\Server\Share\     - ERROR_FILE_NOT_FOUND

			 \\\\Server\Share_RootDrive  - ERROR_INVALID_NAME
			 \\\\Server\Share_RootDrive\ - ERROR_INVALID_NAME

			   c:\* - ERROR_FILE_NOT_FOUND, if thare are no item in that folder
			 */

			bool CFindFile::FindFirst(CFSTR path, CFileInfo &fi)
			{
				if(!Close())
					return false;
			  #ifndef _UNICODE
				if(!g_IsNT) {
					WIN32_FIND_DATAA fd;
					_handle = ::FindFirstFileA(fs2fas(path), &fd);
					if(_handle == INVALID_HANDLE_VALUE)
						return false;
					Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
				}
				else
			  #endif
				{
					WIN32_FIND_DATAW fd;
					IF_USE_MAIN_PATH
						_handle = ::FindFirstFileW(fs2us(path), &fd);
				#ifdef WIN_LONG_PATH
					if(_handle == INVALID_HANDLE_VALUE && USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							_handle = ::FindFirstFileW(superPath, &fd);
					}
				#endif
					if(_handle == INVALID_HANDLE_VALUE)
						return false;
					Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
				}
				return true;
			}

			bool CFindFile::FindNext(CFileInfo &fi)
			{
			  #ifndef _UNICODE
				if(!g_IsNT) {
					WIN32_FIND_DATAA fd;
					if(!::FindNextFileA(_handle, &fd))
						return false;
					Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
				}
				else
			  #endif
				{
					WIN32_FIND_DATAW fd;
					if(!::FindNextFileW(_handle, &fd))
						return false;
					Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
				}
				return true;
			}

			#if defined(_WIN32) && !defined(UNDER_CE)
			//
			// AltStreams
			//
			static FindFirstStreamW_Ptr g_FindFirstStreamW;
			static FindNextStreamW_Ptr g_FindNextStreamW;

			struct CFindStreamLoader {
				CFindStreamLoader()
				{
					g_FindFirstStreamW = (FindFirstStreamW_Ptr) ::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "FindFirstStreamW");
					g_FindNextStreamW = (FindNextStreamW_Ptr) ::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "FindNextStreamW");
				}
			} g_FindStreamLoader;

			bool CStreamInfo::IsMainStream() const throw()
			{
				return sstreqi_ascii(Name, "::$DATA");
			}
			UString CStreamInfo::GetReducedName() const
			{
				// remove ":$DATA" postfix, but keep postfix, if Name is "::$DATA"
				UString s(Name);
				if(s.Len() > 6 + 1 && sstreqi_ascii(s.RightPtr(6), ":$DATA"))
					s.DeleteFrom(s.Len() - 6);
				return s;
			}
			/*
			   UString CStreamInfo::GetReducedName2() const
			   {
			   UString s = GetReducedName();
			   if(!s.IsEmpty() && s[0] == ':')
				s.Delete(0);
			   return s;
			   }
			 */
			static void Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(const MY_WIN32_FIND_STREAM_DATA &sd, CStreamInfo &si)
			{
				si.Size = sd.StreamSize.QuadPart;
				si.Name = sd.cStreamName;
			}
			/*
			   WinXP-64 FindFirstStream():
			   ""      -  ERROR_PATH_NOT_FOUND
			   folder\ -  OK
			   folder  -  OK
			 \       -  OK
			   c:\     -  OK
			   c:      -  OK, if current dir is ROOT     ( c:\ )
			   c:      -  OK, if current dir is NOT ROOT ( c:\folder )
			 \\\Server\Share   - OK
			 \\\Server\Share\  - OK

			 \\               - ERROR_INVALID_NAME
			 \\\\Server         - ERROR_INVALID_NAME
			 \\\\Server\        - ERROR_INVALID_NAME
			 */
			bool CFindStream::FindFirst(CFSTR path, CStreamInfo &si)
			{
				if(!Close())
					return false;
				if(!g_FindFirstStreamW) {
					::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
					return false;
				}
				{
					MY_WIN32_FIND_STREAM_DATA sd;
					SetLastError(0);
					IF_USE_MAIN_PATH
						_handle = g_FindFirstStreamW(fs2us(path), My_FindStreamInfoStandard, &sd, 0);
					if(_handle == INVALID_HANDLE_VALUE) {
						if(::GetLastError() == ERROR_HANDLE_EOF)
							return false;
						// long name can be tricky for path like ".\dirName".
				  #ifdef WIN_LONG_PATH
						if(USE_SUPER_PATH) {
							UString superPath;
							if(GetSuperPath(path, superPath, USE_MAIN_PATH))
								_handle = g_FindFirstStreamW(superPath, My_FindStreamInfoStandard, &sd, 0);
						}
				  #endif
					}
					if(_handle == INVALID_HANDLE_VALUE)
						return false;
					Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(sd, si);
				}
				return true;
			}
			bool CFindStream::FindNext(CStreamInfo &si)
			{
				if(!g_FindNextStreamW) {
					::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
					return false;
				}
				{
					MY_WIN32_FIND_STREAM_DATA sd;
					if(!g_FindNextStreamW(_handle, &sd))
						return false;
					Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(sd, si);
				}
				return true;
			}
			bool CStreamEnumerator::Next(CStreamInfo &si, bool &found)
			{
				bool res = _find.IsHandleAllocated() ? _find.FindNext(si) : _find.FindFirst(_filePath, si);
				if(res) {
					found = true;
					return true;
				}
				else {
					found = false;
					return (::GetLastError() == ERROR_HANDLE_EOF);
				}
			}
			#endif

			#define MY_CLEAR_FILETIME(ft) ft.dwLowDateTime = ft.dwHighDateTime = 0;

			void CFileInfoBase::ClearBase() throw()
			{
				Size = 0;
				MY_CLEAR_FILETIME(CTime);
				MY_CLEAR_FILETIME(ATime);
				MY_CLEAR_FILETIME(MTime);
				Attrib = 0;
				IsAltStream = false;
				IsDevice = false;
			}
			/*
			   WinXP-64 GetFileAttributes():
			   If the function fails, it returns INVALID_FILE_ATTRIBUTES and use GetLastError() to get error code

			 \    - OK
			   C:\  - OK, if there is such drive,
			   D:\  - ERROR_PATH_NOT_FOUND, if there is no such drive,

			   C:\folder     - OK
			   C:\folder\    - OK
			   C:\folderBad  - ERROR_FILE_NOT_FOUND

			 \\\Server\BadShare  - ERROR_BAD_NETPATH
			 \\\Server\Share     - WORKS OK, but MSDN says:
									  GetFileAttributes for a network share, the function fails, and GetLastError
									  returns ERROR_BAD_NETPATH. You must specify a path to a subfolder on that share.
			*/
			DWORD GetFileAttrib(CFSTR path)
			{
			  #ifndef _UNICODE
				if(!g_IsNT)
					return ::GetFileAttributes(fs2fas(path));
				else
			  #endif
				{
					IF_USE_MAIN_PATH
					{
						DWORD dw = ::GetFileAttributesW(fs2us(path));
						if(dw != INVALID_FILE_ATTRIBUTES)
							return dw;
					}
				#ifdef WIN_LONG_PATH
					if(USE_SUPER_PATH) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							return ::GetFileAttributesW(superPath);
					}
				#endif
					return INVALID_FILE_ATTRIBUTES;
				}
			}

			/* if path is "c:" or "c::" then CFileInfo::Find() returns name of current folder for that disk
			   so instead of absolute path we have relative path in Name. That is not good in some calls */

			/* In CFileInfo::Find() we want to support same names for alt streams as in CreateFile(). */

			/* CFileInfo::Find()
			   We alow the following paths (as FindFirstFile):
			   C:\folder
			   c:                      - if current dir is NOT ROOT ( c:\folder )

			   also we support paths that are not supported by FindFirstFile:
			 \
			 \\\.\c:
			   c:\                     - Name will be without tail slash ( c: )
			 \\\?\c:\                 - Name will be without tail slash ( c: )
			 \\\Server\Share
			 \\\?\UNC\Server\Share

			   c:\folder:stream  - Name = folder:stream
			   c:\:stream        - Name = :stream
			   c::stream         - Name = c::stream
			 */
			bool CFileInfo::Find(CFSTR path)
			{
			  #ifdef SUPPORT_DEVICE_FILE
				if(IsDevicePath(path)) {
					ClearBase();
					Name = path + 4;
					IsDevice = true;
					if(NName::IsDrivePath2(path + 4) && path[6] == 0) {
						FChar drive[4] = { path[4], ':', '\\', 0 };
						uint64 clusterSize, totalSize, freeSize;
						if(NSystem::MyGetDiskFreeSpace(drive, clusterSize, totalSize, freeSize)) {
							Size = totalSize;
							return true;
						}
					}
					NIO::CInFile inFile;
					// ::OutputDebugStringW(path);
					if(!inFile.Open(path))
						return false;
					// ::OutputDebugStringW(L"---");
					if(inFile.SizeDefined)
						Size = inFile.Size;
					return true;
				}
			  #endif
			  #if defined(_WIN32) && !defined(UNDER_CE)
				int colonPos = FindAltStreamColon(path);
				if(colonPos >= 0 && path[(uint)colonPos + 1] != 0) {
					UString streamName = fs2us(path + (uint)colonPos);
					FString filePath(path);
					filePath.DeleteFrom(colonPos);
					/* we allow both cases:
					   name:stream
					   name:stream:$DATA
					 */
					const uint kPostfixSize = 6;
					if(streamName.Len() <= kPostfixSize || !sstreqi_ascii(streamName.RightPtr(kPostfixSize), ":$DATA"))
						streamName += ":$DATA";
					bool isOk = true;
					if(IsDrivePath2(filePath) && (colonPos == 2 || colonPos == 3 && filePath[2] == '\\')) {
						// FindFirstFile doesn't work for "c:\" and for "c:" (if current dir is ROOT)
						ClearBase();
						Name.Empty();
						if(colonPos == 2)
							Name = filePath;
					}
					else
						isOk = Find(filePath);
					if(isOk) {
						Attrib &= ~(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT);
						Size = 0;
						CStreamEnumerator enumerator(filePath);
						for(;;) {
							CStreamInfo si;
							bool found;
							if(!enumerator.Next(si, found))
								return false;
							if(!found) {
								::SetLastError(ERROR_FILE_NOT_FOUND);
								return false;
							}
							if(si.Name.IsEqualTo_NoCase(streamName)) {
								// we delete postfix, if alt stream name is not "::$DATA"
								if(si.Name.Len() > kPostfixSize + 1)
									si.Name.DeleteFrom(si.Name.Len() - kPostfixSize);
								Name += us2fs(si.Name);
								Size = si.Size;
								IsAltStream = true;
								return true;
							}
						}
					}
				}
			#endif
				CFindFile finder;
			#if defined(_WIN32) && !defined(UNDER_CE)
				{
					/*
					   DWORD lastError = GetLastError();
					   if(lastError == ERROR_FILE_NOT_FOUND || lastError == ERROR_BAD_NETPATH  // XP64: "\\Server\Share"
							|| lastError == ERROR_BAD_NET_NAME // Win7: "\\Server\Share"
							|| lastError == ERROR_INVALID_NAME // XP64: "\\?\UNC\Server\Share"
							|| lastError == ERROR_BAD_PATHNAME // Win7: "\\?\UNC\Server\Share"
						)
					 */
					uint   rootSize = 0;
					if(IsSuperPath(path))
						rootSize = kSuperPathPrefixSize;
					if(NName::IsDrivePath(path + rootSize) && path[rootSize + 3] == 0) {
						DWORD attrib = GetFileAttrib(path);
						if(attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
							ClearBase();
							Attrib = attrib;
							Name = path + rootSize;
							Name.DeleteFrom(2); // we don't need backslash (C:)
							return true;
						}
					}
					else if(isdirslash(path[0]))
						if(path[1] == 0) {
							DWORD attrib = GetFileAttrib(path);
							if(attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
								ClearBase();
								Name.Empty();
								Attrib = attrib;
								return true;
							}
						}
						else {
							const uint prefixSize = GetNetworkServerPrefixSize(path);
							if(prefixSize > 0 && path[prefixSize] != 0) {
								if(NName::FindSepar(path + prefixSize) < 0) {
									FString s(path);
									s.Add_PathSepar();
									s += '*'; // CHAR_ANY_MASK
									bool isOK = false;
									if(finder.FindFirst(s, *this)) {
										if(Name == FTEXT(".")) {
											Name = path + prefixSize;
											return true;
										}
										isOK = true;
										/* if "\\server\share" maps to root folder "d:\", there is no
										   "." item.
										   But it's possible that there are another items */
									}
									{
										DWORD attrib = GetFileAttrib(path);
										if(isOK || attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
											ClearBase();
											if(attrib != INVALID_FILE_ATTRIBUTES)
												Attrib = attrib;
											else
												SetAsDir();
											Name = path + prefixSize;
											return true;
										}
									}
									// ::SetLastError(lastError);
								}
							}
						}
				}
			  #endif
				return finder.FindFirst(path, *this);
			}
			bool DoesFileExist(CFSTR name)
			{
				CFileInfo fi;
				return fi.Find(name) && !fi.IsDir();
			}
			bool DoesDirExist(CFSTR name)
			{
				CFileInfo fi;
				return fi.Find(name) && fi.IsDir();
			}
			bool DoesFileOrDirExist(CFSTR name)
			{
				CFileInfo fi;
				return fi.Find(name);
			}
			void CEnumerator::SetDirPrefix(const FString &dirPrefix)
			{
				_wildcard = dirPrefix;
				_wildcard += '*';
			}
			bool CEnumerator::NextAny(CFileInfo &fi)
			{
				return _findFile.IsHandleAllocated() ? _findFile.FindNext(fi) : _findFile.FindFirst(_wildcard, fi);
			}
			bool CEnumerator::Next(CFileInfo &fi)
			{
				for(;;) {
					if(!NextAny(fi))
						return false;
					else if(!fi.IsDots())
						return true;
				}
			}
			bool CEnumerator::Next(CFileInfo &fi, bool &found)
			{
				if(Next(fi)) {
					found = true;
					return true;
				}
				else {
					found = false;
					return (::GetLastError() == ERROR_NO_MORE_FILES);
				}
			}
			//
			// CFindChangeNotification
			// FindFirstChangeNotification can return 0. MSDN doesn't tell about it.
			//
			bool CFindChangeNotification::Close() throw()
			{
				if(!IsHandleAllocated())
					return true;
				else if(!::FindCloseChangeNotification(_handle))
					return false;
				else {
					_handle = INVALID_HANDLE_VALUE;
					return true;
				}
			}
			HANDLE CFindChangeNotification::FindFirst(CFSTR path, bool watchSubtree, DWORD notifyFilter)
			{
			  #ifndef _UNICODE
				if(!g_IsNT)
					_handle = ::FindFirstChangeNotification(fs2fas(path), BoolToBOOL(watchSubtree), notifyFilter);
				else
			  #endif
				{
					IF_USE_MAIN_PATH
						_handle = ::FindFirstChangeNotificationW(fs2us(path), BoolToBOOL(watchSubtree), notifyFilter);
				#ifdef WIN_LONG_PATH
					if(!IsHandleAllocated()) {
						UString superPath;
						if(GetSuperPath(path, superPath, USE_MAIN_PATH))
							_handle = ::FindFirstChangeNotificationW(superPath, BoolToBOOL(watchSubtree), notifyFilter);
					}
				#endif
				}
				return _handle;
			}

			#ifndef UNDER_CE
				bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings)
				{
					driveStrings.Clear();
				  #ifndef _UNICODE
					if(!g_IsNT) {
						driveStrings.Clear();
						UINT32 size = GetLogicalDriveStrings(0, NULL);
						if(size == 0)
							return false;
						CObjArray<char> buf(size);
						UINT32 newSize = GetLogicalDriveStrings(size, buf);
						if(newSize == 0 || newSize > size)
							return false;
						AString s;
						UINT32 prev = 0;
						for(UINT32 i = 0; i < newSize; i++) {
							if(buf[i] == 0) {
								s = buf + prev;
								prev = i + 1;
								driveStrings.Add(fas2fs(s));
							}
						}
						return prev == newSize;
					}
					else
				  #endif
					{
						UINT32 size = GetLogicalDriveStringsW(0, NULL);
						if(size == 0)
							return false;
						CObjArray<wchar_t> buf(size);
						UINT32 newSize = GetLogicalDriveStringsW(size, buf);
						if(newSize == 0 || newSize > size)
							return false;
						UString s;
						UINT32 prev = 0;
						for(UINT32 i = 0; i < newSize; i++) {
							if(buf[i] == 0) {
								s = buf + prev;
								prev = i + 1;
								driveStrings.Add(us2fs(s));
							}
						}
						return prev == newSize;
					}
				}
			#endif
		}
	}
	namespace NError {
		static bool MyFormatMessage(DWORD errorCode, UString &message)
		{
			LPVOID msgBuf;
		  #ifndef _UNICODE
			if(!g_IsNT) {
				if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, errorCode, 0, (LPTSTR)&msgBuf, 0, 0) == 0)
					return false;
				message = GetUnicodeString((LPCTSTR)msgBuf);
			}
			else
		  #endif
			{
				if(::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, errorCode, 0, (LPWSTR)&msgBuf, 0, 0) == 0)
					return false;
				message = (LPCWSTR)msgBuf;
			}
			::LocalFree(msgBuf);
			return true;
		}

		UString MyFormatMessage(DWORD errorCode)
		{
			UString m;
			if(!MyFormatMessage(errorCode, m) || m.IsEmpty()) {
				char s[16];
				for(int i = 0; i < 8; i++) {
					unsigned t = errorCode & 0xF;
					errorCode >>= 4;
					s[7 - i] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
				}
				s[8] = 0;
				m += "Error #";
				m += s;
			}
			else if(m.Len() >= 2 && m[m.Len() - 1] == 0x0A && m[m.Len() - 2] == 0x0D)
				m.DeleteFrom(m.Len() - 2);
			return m;
		}
	}
	namespace NSecurity {
		#ifndef UNDER_CE
			#ifdef _UNICODE
				#define MY_FUNC_SELECT(f) :: f
			#else
				#define MY_FUNC_SELECT(f) my_ ## f
				extern "C" {
					typedef BOOL (WINAPI * Func_OpenProcessToken)(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle);
					typedef BOOL (WINAPI * Func_LookupPrivilegeValue)(LPCTSTR lpSystemName, LPCTSTR lpName, PLUID lpLuid);
					typedef BOOL (WINAPI * Func_AdjustTokenPrivileges)(HANDLE TokenHandle, BOOL DisableAllPrivileges,
						PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength);
				}
				#define GET_PROC_ADDR(fff, name) Func_ ## fff my_ ## fff  = (Func_ ## fff)GetProcAddress(hModule, name)
			#endif
			bool EnablePrivilege(LPCTSTR privilegeName, bool enable)
			{
				bool res = false;
			  #ifndef _UNICODE
				HMODULE hModule = ::LoadLibrary(TEXT("Advapi32.dll"));
				if(!hModule)
					return false;
				GET_PROC_ADDR(OpenProcessToken, "OpenProcessToken");
				GET_PROC_ADDR(LookupPrivilegeValue, "LookupPrivilegeValueA");
				GET_PROC_ADDR(AdjustTokenPrivileges, "AdjustTokenPrivileges");
				if(my_OpenProcessToken && my_AdjustTokenPrivileges && my_LookupPrivilegeValue)
			  #endif
				{
					HANDLE token;
					if(MY_FUNC_SELECT(OpenProcessToken) (::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
						TOKEN_PRIVILEGES tp;
						if(MY_FUNC_SELECT(LookupPrivilegeValue) (NULL, privilegeName, &(tp.Privileges[0].Luid))) {
							tp.PrivilegeCount = 1;
							tp.Privileges[0].Attributes = (enable ? SE_PRIVILEGE_ENABLED : 0);
							if(MY_FUNC_SELECT(AdjustTokenPrivileges) (token, FALSE, &tp, 0, NULL, NULL))
								res = (GetLastError() == ERROR_SUCCESS);
						}
						::CloseHandle(token);
					}
				}
			  #ifndef _UNICODE
				::FreeLibrary(hModule);
			  #endif
				return res;
			}
		#endif
	}
	namespace NCOM {
		BSTR AllocBstrFromAscii(const char * s) throw()
		{
			BSTR p = 0;
			if(s) {
				UINT len = (UINT)sstrlen(s);
				p = ::SysAllocStringLen(NULL, len);
				if(p) {
					for(UINT i = 0; i <= len; i++)
						p[i] = (Byte)s[i];
				}
			}
			return p;
		}
		HRESULT PropVarEm_Alloc_Bstr(PROPVARIANT * p, unsigned numChars) throw()
		{
			p->bstrVal = ::SysAllocStringLen(NULL, numChars);
			if(!p->bstrVal) {
				p->vt = VT_ERROR;
				p->scode = E_OUTOFMEMORY;
				return E_OUTOFMEMORY;
			}
			p->vt = VT_BSTR;
			return S_OK;
		}
		HRESULT PropVarEm_Set_Str(PROPVARIANT * p, const char * s) throw()
		{
			p->bstrVal = AllocBstrFromAscii(s);
			if(p->bstrVal) {
				p->vt = VT_BSTR;
				return S_OK;
			}
			p->vt = VT_ERROR;
			p->scode = E_OUTOFMEMORY;
			return E_OUTOFMEMORY;
		}
		CPropVariant::CPropVariant()
		{
			vt = VT_EMPTY;
			wReserved1 = 0;
			// wReserved2 = 0;
			// wReserved3 = 0;
			// uhVal.QuadPart = 0;
			bstrVal = 0;
		}
		CPropVariant::CPropVariant(bool bSrc) 
		{
			vt = VT_BOOL; wReserved1 = 0; boolVal = (bSrc ? VARIANT_TRUE : VARIANT_FALSE);
		}
		CPropVariant::CPropVariant(Byte value) 
		{
			vt = VT_UI1; wReserved1 = 0; bVal = value;
		}
		CPropVariant::CPropVariant(uint32 value) 
		{
			vt = VT_UI4; 
			wReserved1 = 0; 
			ulVal = value;
		}
		CPropVariant::CPropVariant(uint64 value) 
		{
			vt = VT_UI8; 
			wReserved1 = 0; 
			uhVal.QuadPart = value;
		}
		CPropVariant::CPropVariant(int64 value) 
		{
			vt = VT_I8; 
			wReserved1 = 0; 
			hVal.QuadPart = value;
		}
		CPropVariant::CPropVariant(const FILETIME &value) 
		{
			vt = VT_FILETIME; 
			wReserved1 = 0; 
			filetime = value;
		}
		CPropVariant::~CPropVariant() throw() 
		{
			Clear();
		}
		CPropVariant::CPropVariant(const PROPVARIANT &varSrc)
		{
			vt = VT_EMPTY;
			InternalCopy(&varSrc);
		}
		CPropVariant::CPropVariant(const CPropVariant &varSrc)
		{
			vt = VT_EMPTY;
			InternalCopy(&varSrc);
		}
		CPropVariant::CPropVariant(BSTR bstrSrc)
		{
			vt = VT_EMPTY;
			*this = bstrSrc;
		}
		CPropVariant::CPropVariant(LPCOLESTR lpszSrc)
		{
			vt = VT_EMPTY;
			*this = lpszSrc;
		}
		CPropVariant& CPropVariant::operator = (const CPropVariant &varSrc)
		{
			InternalCopy(&varSrc);
			return *this;
		}
		CPropVariant& CPropVariant::operator = (const PROPVARIANT &varSrc)
		{
			InternalCopy(&varSrc);
			return *this;
		}
		CPropVariant& CPropVariant::operator = (BSTR bstrSrc)
		{
			*this = (LPCOLESTR)bstrSrc;
			return *this;
		}

		static const char * const kMemException = SlTxtOutOfMem;

		CPropVariant& CPropVariant::operator = (LPCOLESTR lpszSrc)
		{
			InternalClear();
			vt = VT_BSTR;
			wReserved1 = 0;
			bstrVal = ::SysAllocString(lpszSrc);
			if(!bstrVal && lpszSrc) {
				throw kMemException;
				// vt = VT_ERROR;
				// scode = E_OUTOFMEMORY;
			}
			return *this;
		}

		CPropVariant& CPropVariant::operator = (const UString &s)
		{
			InternalClear();
			vt = VT_BSTR;
			wReserved1 = 0;
			bstrVal = ::SysAllocStringLen(s, s.Len());
			if(!bstrVal)
				throw kMemException;
			return *this;
		}
		CPropVariant& CPropVariant::operator = (const UString2 &s)
		{
			/*
			   if(s.IsEmpty())
			   *this = L"";
			   else
			 */
			{
				InternalClear();
				vt = VT_BSTR;
				wReserved1 = 0;
				bstrVal = ::SysAllocStringLen(s.GetRawPtr(), s.Len());
				if(!bstrVal)
					throw kMemException;
				/* SysAllocStringLen probably appends a null-terminating character for NULL string.
				   But it doesn't specified in MSDN.
				   But we suppose that it works

				   if(!s.GetRawPtr())
				   {
				   *bstrVal = 0;
				   }
				 */

				/* MSDN: Windows CE: SysAllocStringLen() : Passing invalid (and under some circumstances NULL)
									 pointers to this function causes  an unexpected termination of the application.
				   Is it safe? Maybe we must chamnge the code for that case ? */
			}
			return *this;
		}

		CPropVariant& CPropVariant::operator = (const char * s)
		{
			InternalClear();
			vt = VT_BSTR;
			wReserved1 = 0;
			bstrVal = AllocBstrFromAscii(s);
			if(!bstrVal) {
				throw kMemException;
				// vt = VT_ERROR;
				// scode = E_OUTOFMEMORY;
			}
			return *this;
		}
		CPropVariant& CPropVariant::operator = (bool bSrc) throw()
		{
			if(vt != VT_BOOL) {
				InternalClear();
				vt = VT_BOOL;
			}
			boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
			return *this;
		}
		BSTR CPropVariant::AllocBstr(unsigned numChars)
		{
			if(vt != VT_EMPTY)
				InternalClear();
			vt = VT_BSTR;
			wReserved1 = 0;
			bstrVal = ::SysAllocStringLen(NULL, numChars);
			if(!bstrVal) {
				throw kMemException;
				// vt = VT_ERROR;
				// scode = E_OUTOFMEMORY;
			}
			return bstrVal;
		}
		#define SET_PROP_FUNC(type, id, dest) \
			CPropVariant& CPropVariant::operator = (type value) throw() \
			{ if(vt != id) { InternalClear(); vt = id; } \
			  dest = value; return *this; \
			}

		SET_PROP_FUNC(Byte, VT_UI1, bVal)
		// SET_PROP_FUNC(int16, VT_I2, iVal)
		SET_PROP_FUNC(int32, VT_I4, lVal)
		SET_PROP_FUNC(uint32, VT_UI4, ulVal)
		SET_PROP_FUNC(uint64, VT_UI8, uhVal.QuadPart)
		SET_PROP_FUNC(int64, VT_I8, hVal.QuadPart)
		SET_PROP_FUNC(const FILETIME &, VT_FILETIME, filetime)

		HRESULT PropVariant_Clear(PROPVARIANT * prop) throw()
		{
			switch(prop->vt) {
				case VT_EMPTY:
				case VT_UI1:
				case VT_I1:
				case VT_I2:
				case VT_UI2:
				case VT_BOOL:
				case VT_I4:
				case VT_UI4:
				case VT_R4:
				case VT_INT:
				case VT_UINT:
				case VT_ERROR:
				case VT_FILETIME:
				case VT_UI8:
				case VT_R8:
				case VT_CY:
				case VT_DATE:
					prop->vt = VT_EMPTY;
					prop->wReserved1 = 0;
					prop->wReserved2 = 0;
					prop->wReserved3 = 0;
					prop->uhVal.QuadPart = 0;
					return S_OK;
			}
			return ::VariantClear((VARIANTARG*)prop);
			// return ::PropVariantClear(prop);
			// PropVariantClear can clear VT_BLOB.
		}

		HRESULT CPropVariant::Clear() throw()
		{
			return (vt == VT_EMPTY) ? S_OK : PropVariant_Clear(this);
		}

		HRESULT CPropVariant::Copy(const PROPVARIANT* pSrc) throw()
		{
			::VariantClear((tagVARIANT*)this);
			switch(pSrc->vt) {
				case VT_UI1:
				case VT_I1:
				case VT_I2:
				case VT_UI2:
				case VT_BOOL:
				case VT_I4:
				case VT_UI4:
				case VT_R4:
				case VT_INT:
				case VT_UINT:
				case VT_ERROR:
				case VT_FILETIME:
				case VT_UI8:
				case VT_R8:
				case VT_CY:
				case VT_DATE:
					memmove((PROPVARIANT*)this, pSrc, sizeof(PROPVARIANT));
					return S_OK;
			}
			return ::VariantCopy((tagVARIANT*)this, (tagVARIANT*)const_cast<PROPVARIANT *>(pSrc));
		}

		HRESULT CPropVariant::Attach(PROPVARIANT * pSrc) throw()
		{
			HRESULT hr = Clear();
			if(FAILED(hr))
				return hr;
			memcpy(this, pSrc, sizeof(PROPVARIANT));
			pSrc->vt = VT_EMPTY;
			return S_OK;
		}

		HRESULT CPropVariant::Detach(PROPVARIANT * pDest) throw()
		{
			if(pDest->vt != VT_EMPTY) {
				HRESULT hr = PropVariant_Clear(pDest);
				if(FAILED(hr))
					return hr;
			}
			memcpy(pDest, this, sizeof(PROPVARIANT));
			vt = VT_EMPTY;
			return S_OK;
		}

		HRESULT CPropVariant::InternalClear() throw()
		{
			if(vt == VT_EMPTY)
				return S_OK;
			HRESULT hr = Clear();
			if(FAILED(hr)) {
				vt = VT_ERROR;
				scode = hr;
			}
			return hr;
		}
		void FASTCALL CPropVariant::InternalCopy(const PROPVARIANT * pSrc)
		{
			HRESULT hr = Copy(pSrc);
			if(FAILED(hr)) {
				if(hr == E_OUTOFMEMORY)
					throw kMemException;
				vt = VT_ERROR;
				scode = hr;
			}
		}
		int CPropVariant::Compare(const CPropVariant &a) throw()
		{
			if(vt != a.vt)
				return MyCompare(vt, a.vt);
			switch(vt) {
				case VT_EMPTY: return 0;
				// case VT_I1: return MyCompare(cVal, a.cVal);
				case VT_UI1: return MyCompare(bVal, a.bVal);
				case VT_I2: return MyCompare(iVal, a.iVal);
				case VT_UI2: return MyCompare(uiVal, a.uiVal);
				case VT_I4: return MyCompare(lVal, a.lVal);
				case VT_UI4: return MyCompare(ulVal, a.ulVal);
				// case VT_UINT: return MyCompare(uintVal, a.uintVal);
				case VT_I8: return MyCompare(hVal.QuadPart, a.hVal.QuadPart);
				case VT_UI8: return MyCompare(uhVal.QuadPart, a.uhVal.QuadPart);
				case VT_BOOL: return -MyCompare(boolVal, a.boolVal);
				case VT_FILETIME: return ::CompareFileTime(&filetime, &a.filetime);
				case VT_BSTR: return 0; // Not implemented
				default: return 0;
			}
		}
		void FASTCALL ParseNumberString(const UString & s, CPropVariant &prop)
		{
			const wchar_t * end;
			uint64 result = ConvertStringToUInt64(s, &end);
			if(*end != 0 || s.IsEmpty())
				prop = s;
			else if(result <= (uint32)0xFFFFFFFF)
				prop = (uint32)result;
			else
				prop = result;
		}
	}
	namespace NDLL {
		bool CLibrary::Free() throw()
		{
			if(_module == 0)
				return true;
			if(!::FreeLibrary(_module))
				return false;
			_module = 0;
			return true;
		}
		bool CLibrary::LoadEx(CFSTR path, DWORD flags) throw()
		{
			if(!Free())
				return false;
		  #ifndef _UNICODE
			if(!g_IsNT) {
				_module = ::LoadLibraryEx(fs2fas(path), NULL, flags);
			}
			else
		  #endif
			{
				_module = ::LoadLibraryExW(fs2us(path), NULL, flags);
			}
			return (_module != NULL);
		}
		bool CLibrary::Load(CFSTR path) throw()
		{
			if(!Free())
				return false;
		  #ifndef _UNICODE
			if(!g_IsNT) {
				_module = ::LoadLibrary(fs2fas(path));
			}
			else
		  #endif
			{
				_module = ::LoadLibraryW(fs2us(path));
			}
			return (_module != NULL);
		}
		bool MyGetModuleFileName(FString & path)
		{
			HMODULE hModule = g_hInstance;
			path.Empty();
		  #ifndef _UNICODE
			if(!g_IsNT) {
				TCHAR s[MAX_PATH+2];
				s[0] = 0;
				DWORD size = ::GetModuleFileName(hModule, s, MAX_PATH + 1);
				if(size <= MAX_PATH && size != 0) {
					path = fas2fs(s);
					return true;
				}
			}
			else
		  #endif
			{
				WCHAR s[MAX_PATH+2];
				s[0] = 0;
				DWORD size = ::GetModuleFileNameW(hModule, s, MAX_PATH + 1);
				if(size <= MAX_PATH && size != 0) {
					path = us2fs(s);
					return true;
				}
			}
			return false;
		}
		#ifndef _SFX
			FString GetModuleDirPrefix()
			{
				FString s;
				if(MyGetModuleFileName(s)) {
					int pos = s.ReverseFind_PathSepar();
					if(pos >= 0)
						s.DeleteFrom(pos + 1);
				}
				if(s.IsEmpty())
					s = "." STRING_PATH_SEPARATOR;
				return s;
			}
		#endif
	}
	namespace NTime {
		static const uint32 kNumTimeQuantumsInSecond = 10000000;
		static const uint32 kFileTimeStartYear = 1601;
		static const uint32 kDosTimeStartYear = 1980;
		static const uint32 kUnixTimeStartYear = 1970;
		static const uint64 kUnixTimeOffset = (uint64)60 * 60 * 24 * (89 + 365 * (kUnixTimeStartYear - kFileTimeStartYear));
		static const uint64 kNumSecondsInFileTime = (uint64)(int64)-1 / kNumTimeQuantumsInSecond;

		bool DosTimeToFileTime(uint32 dosTime, FILETIME &ft) throw()
		{
		  #if defined(_WIN32) && !defined(UNDER_CE)
			return BOOLToBool(::DosDateTimeToFileTime(static_cast<uint16>(dosTime >> 16), static_cast<uint16>(dosTime & 0xFFFF), &ft));
		  #else
			ft.dwLowDateTime = 0;
			ft.dwHighDateTime = 0;
			uint64 res;
			if(!GetSecondsSince1601(kDosTimeStartYear + (dosTime >> 25), (dosTime >> 21) & 0xF, (dosTime >> 16) & 0x1F,
							(dosTime >> 11) & 0x1F, (dosTime >> 5) & 0x3F, (dosTime & 0x1F) * 2, res))
				return false;
			res *= kNumTimeQuantumsInSecond;
			ft.dwLowDateTime = (uint32)res;
			ft.dwHighDateTime = (uint32)(res >> 32);
			return true;
		  #endif
		}

		static const uint32 kHighDosTime = 0xFF9FBF7D;
		static const uint32 kLowDosTime = 0x210000;

		#define PERIOD_4 (4 * 365 + 1)
		#define PERIOD_100 (PERIOD_4 * 25 - 1)
		#define PERIOD_400 (PERIOD_100 * 4 + 1)

		bool FileTimeToDosTime(const FILETIME &ft, uint32 &dosTime) throw()
		{
		  #if defined(_WIN32) && !defined(UNDER_CE)

			WORD datePart, timePart;
			if(!::FileTimeToDosDateTime(&ft, &datePart, &timePart)) {
				dosTime = (ft.dwHighDateTime >= 0x01C00000) ? kHighDosTime : kLowDosTime;
				return false;
			}
			dosTime = (((uint32)datePart) << 16) + timePart;

		  #else

			unsigned year, mon, day, hour, min, sec;
			uint64 v64 = ft.dwLowDateTime | ((uint64)ft.dwHighDateTime << 32);
			Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			unsigned temp;
			uint32 v;
			v64 += (kNumTimeQuantumsInSecond * 2 - 1);
			v64 /= kNumTimeQuantumsInSecond;
			sec = (uint)(v64 % 60);
			v64 /= 60;
			min = (uint)(v64 % 60);
			v64 /= 60;
			hour = (uint)(v64 % 24);
			v64 /= 24;

			v = (uint32)v64;

			year = (uint)(kFileTimeStartYear + v / PERIOD_400 * 400);
			v %= PERIOD_400;

			temp = (uint)(v / PERIOD_100);
			if(temp == 4)
				temp = 3;
			year += temp * 100;
			v -= temp * PERIOD_100;

			temp = v / PERIOD_4;
			if(temp == 25)
				temp = 24;
			year += temp * 4;
			v -= temp * PERIOD_4;

			temp = v / 365;
			if(temp == 4)
				temp = 3;
			year += temp;
			v -= temp * 365;

			if(year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
				ms[1] = 29;
			for(mon = 1; mon <= 12; mon++) {
				unsigned s = ms[mon - 1];
				if(v < s)
					break;
				v -= s;
			}
			day = (uint)v + 1;

			dosTime = kLowDosTime;
			if(year < kDosTimeStartYear)
				return false;
			year -= kDosTimeStartYear;
			dosTime = kHighDosTime;
			if(year >= 128)
				return false;
			dosTime = (year << 25) | (mon << 21) | (day << 16) | (hour << 11) | (min << 5) | (sec >> 1);
		  #endif
			return true;
		}

		uint64 UnixTimeToFileTime64(uint32 unixTime) throw()
		{
			return (kUnixTimeOffset + (uint64)unixTime) * kNumTimeQuantumsInSecond;
		}

		void UnixTimeToFileTime(uint32 unixTime, FILETIME &ft) throw()
		{
			uint64 v = UnixTimeToFileTime64(unixTime);
			ft.dwLowDateTime = (DWORD)v;
			ft.dwHighDateTime = (DWORD)(v >> 32);
		}

		uint64 UnixTime64ToFileTime64(int64 unixTime) throw()
		{
			return (uint64)(kUnixTimeOffset + unixTime) * kNumTimeQuantumsInSecond;
		}

		bool UnixTime64ToFileTime(int64 unixTime, FILETIME &ft) throw()
		{
			if(unixTime > (int64)(kNumSecondsInFileTime - kUnixTimeOffset)) {
				ft.dwLowDateTime = ft.dwHighDateTime = (uint32)(int32)-1;
				return false;
			}
			int64 v = (int64)kUnixTimeOffset + unixTime;
			if(v < 0) {
				ft.dwLowDateTime = ft.dwHighDateTime = 0;
				return false;
			}
			uint64 v2 = (uint64)v * kNumTimeQuantumsInSecond;
			ft.dwLowDateTime = (DWORD)v2;
			ft.dwHighDateTime = (DWORD)(v2 >> 32);
			return true;
		}

		int64 FileTimeToUnixTime64(const FILETIME &ft) throw()
		{
			uint64 winTime = (((uint64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
			return (int64)(winTime / kNumTimeQuantumsInSecond) - (int64)kUnixTimeOffset;
		}

		bool FileTimeToUnixTime(const FILETIME &ft, uint32 &unixTime) throw()
		{
			uint64 winTime = (((uint64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
			winTime /= kNumTimeQuantumsInSecond;
			if(winTime < kUnixTimeOffset) {
				unixTime = 0;
				return false;
			}
			winTime -= kUnixTimeOffset;
			if(winTime > 0xFFFFFFFF) {
				unixTime = 0xFFFFFFFF;
				return false;
			}
			unixTime = (uint32)winTime;
			return true;
		}

		bool GetSecondsSince1601(uint year, uint month, uint day, uint hour, uint min, uint sec, uint64 &resSeconds) throw()
		{
			resSeconds = 0;
			if(year < kFileTimeStartYear || year >= 10000 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
				return false;
			uint32 numYears = year - kFileTimeStartYear;
			uint32 numDays = numYears * 365 + numYears / 4 - numYears / 100 + numYears / 400;
			Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			if(year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
				ms[1] = 29;
			month--;
			for(uint i = 0; i < month; i++)
				numDays += ms[i];
			numDays += day - 1;
			resSeconds = ((uint64)(numDays * 24 + hour) * 60 + min) * 60 + sec;
			return true;
		}

		void GetCurUtcFileTime(FILETIME &ft) throw()
		{
			// Both variants provide same low resolution on WinXP: about 15 ms.
			// But GetSystemTimeAsFileTime is much faster.

		  #ifdef UNDER_CE
			SYSTEMTIME st;
			GetSystemTime(&st);
			SystemTimeToFileTime(&st, &ft);
		  #else
			GetSystemTimeAsFileTime(&ft);
		  #endif
		}
	}
}
