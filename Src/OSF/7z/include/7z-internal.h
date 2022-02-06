// 7z-iternal.h
//
#ifndef __7Z_INTERNAL_H
#define __7Z_INTERNAL_H

#include <slib.h>
//#include <Compiler.h>
#ifdef _MSC_VER
  #ifdef UNDER_CE
    #define RPC_NO_WINDOWS_H
    /* #pragma warning(disable : 4115) // '_RPC_ASYNC_STATE' : named type definition in parentheses */
    #pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
    #pragma warning(disable : 4214) // nonstandard extension used : bit field types other than int
  #endif
  #if _MSC_VER >= 1300
    #pragma warning(disable : 4996) // This function or variable may be unsafe
  #else
    #pragma warning(disable : 4511) // copy constructor could not be generated
    #pragma warning(disable : 4512) // assignment operator could not be generated
    #pragma warning(disable : 4514) // unreferenced inline function has been removed
    #pragma warning(disable : 4702) // unreachable code
    #pragma warning(disable : 4710) // not inlined
    #pragma warning(disable : 4714) // function marked as __forceinline not inlined
    #pragma warning(disable : 4786) // identifier was truncated to '255' characters in the debug information
  #endif
#endif

#define UNUSED_VAR(x) (void)x;
// #define UNUSED_VAR(x) x=x; 
//
#include <7zVersion.h>
//#include <MyWindows.h>
#ifdef _WIN32
	//#include <windows.h>
	#ifdef UNDER_CE
		#undef VARIANT_TRUE
		#define VARIANT_TRUE ((VARIANT_BOOL)-1)
	#endif
#else
	//#include <stddef.h> // for wchar_t
	//#include <string.h>
	#include <MyGuidDef.h>

	#define WINAPI

	typedef char CHAR;
	typedef uchar UCHAR;

	#undef BYTE
	typedef uchar BYTE;

	typedef short SHORT;
	typedef unsigned short USHORT;

	#undef WORD
	typedef unsigned short WORD;
	typedef short VARIANT_BOOL;

	typedef int INT;
	typedef int32 INT32;
	typedef unsigned int UINT;
	typedef uint32 UINT32;
	typedef INT32 LONG;   // LONG, ULONG and DWORD must be 32-bit
	typedef UINT32 ULONG;

	#undef DWORD
	typedef UINT32 DWORD;
	typedef long BOOL;
	#ifndef FALSE
		#define FALSE 0
		#define TRUE 1
	#endif
	// typedef size_t ULONG_PTR;
	typedef size_t DWORD_PTR;
	typedef int64 LONGLONG;
	typedef uint64 ULONGLONG;
	typedef struct _LARGE_INTEGER { LONGLONG QuadPart; } LARGE_INTEGER;
	typedef struct _ULARGE_INTEGER { ULONGLONG QuadPart; } ULARGE_INTEGER;
	typedef const CHAR * LPCSTR;
	typedef CHAR TCHAR;
	typedef const TCHAR * LPCTSTR;
	typedef wchar_t WCHAR;
	typedef WCHAR OLECHAR;
	typedef const WCHAR * LPCWSTR;
	typedef OLECHAR * BSTR;
	typedef const OLECHAR * LPCOLESTR;
	typedef OLECHAR * LPOLESTR;

	typedef struct _FILETIME {
		DWORD dwLowDateTime;
		DWORD dwHighDateTime;
	} FILETIME;

	#define HRESULT LONG
	#define FAILED(Status) ((HRESULT)(Status)<0)
	typedef ULONG PROPID;
	typedef LONG SCODE;

	#define S_OK    ((HRESULT)0x00000000L)
	#define S_FALSE ((HRESULT)0x00000001L)
	#define E_NOTIMPL ((HRESULT)0x80004001L)
	#define E_NOINTERFACE ((HRESULT)0x80004002L)
	#define E_ABORT ((HRESULT)0x80004004L)
	#define E_FAIL ((HRESULT)0x80004005L)
	#define STG_E_INVALIDFUNCTION ((HRESULT)0x80030001L)
	#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
	#define E_INVALIDARG ((HRESULT)0x80070057L)
	#ifdef _MSC_VER
		#define STDMETHODCALLTYPE __stdcall
	#else
		#define STDMETHODCALLTYPE
	#endif
	#define STDMETHOD_(t, f) virtual t STDMETHODCALLTYPE f
	#define STDMETHOD(f) STDMETHOD_(HRESULT, f)
	#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
	#define STDMETHODIMP STDMETHODIMP_(HRESULT)
	#define PURE = 0
	#define MIDL_INTERFACE(x) struct

	#ifdef __cplusplus
		DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

		struct IUnknown {
			STDMETHOD(QueryInterface) (REFIID iid, void ** outObject) PURE;
			STDMETHOD_(ULONG, AddRef) () PURE;
			STDMETHOD_(ULONG, Release) () PURE;
		#ifndef _WIN32
			virtual ~IUnknown() 
			{
			}
		#endif
		};

		typedef IUnknown * LPUNKNOWN;
	#endif

	#define VARIANT_TRUE ((VARIANT_BOOL)-1)
	#define VARIANT_FALSE ((VARIANT_BOOL)0)

	enum VARENUM {
		VT_EMPTY = 0,
		VT_NULL = 1,
		VT_I2 = 2,
		VT_I4 = 3,
		VT_R4 = 4,
		VT_R8 = 5,
		VT_CY = 6,
		VT_DATE = 7,
		VT_BSTR = 8,
		VT_DISPATCH = 9,
		VT_ERROR = 10,
		VT_BOOL = 11,
		VT_VARIANT = 12,
		VT_UNKNOWN = 13,
		VT_DECIMAL = 14,
		VT_I1 = 16,
		VT_UI1 = 17,
		VT_UI2 = 18,
		VT_UI4 = 19,
		VT_I8 = 20,
		VT_UI8 = 21,
		VT_INT = 22,
		VT_UINT = 23,
		VT_VOID = 24,
		VT_HRESULT = 25,
		VT_FILETIME = 64
	};

	typedef unsigned short VARTYPE;
	typedef WORD PROPVAR_PAD1;
	typedef WORD PROPVAR_PAD2;
	typedef WORD PROPVAR_PAD3;

	typedef struct tagPROPVARIANT {
		VARTYPE vt;
		PROPVAR_PAD1 wReserved1;
		PROPVAR_PAD2 wReserved2;
		PROPVAR_PAD3 wReserved3;
		union {
			CHAR cVal;
			UCHAR bVal;
			SHORT iVal;
			USHORT uiVal;
			LONG lVal;
			ULONG ulVal;
			INT intVal;
			UINT uintVal;
			LARGE_INTEGER hVal;
			ULARGE_INTEGER uhVal;
			VARIANT_BOOL boolVal;
			SCODE scode;
			FILETIME filetime;
			BSTR bstrVal;
		};
	} PROPVARIANT;

	typedef PROPVARIANT tagVARIANT;
	typedef tagVARIANT VARIANT;
	typedef VARIANT VARIANTARG;

	MY_EXTERN_C HRESULT VariantClear(VARIANTARG * prop);
	MY_EXTERN_C HRESULT VariantCopy(VARIANTARG * dest, const VARIANTARG * src);

	typedef struct tagSTATPROPSTG {
		LPOLESTR lpwstrName;
		PROPID propid;
		VARTYPE vt;
	} STATPROPSTG;

	MY_EXTERN_C BSTR SysAllocStringByteLen(LPCSTR psz, UINT len);
	MY_EXTERN_C BSTR SysAllocStringLen(const OLECHAR * sz, UINT len);
	MY_EXTERN_C BSTR SysAllocString(const OLECHAR * sz);
	MY_EXTERN_C void SysFreeString(BSTR bstr);
	MY_EXTERN_C UINT SysStringByteLen(BSTR bstr);
	MY_EXTERN_C UINT SysStringLen(BSTR bstr);
	MY_EXTERN_C DWORD GetLastError();
	MY_EXTERN_C LONG CompareFileTime(const FILETIME* ft1, const FILETIME* ft2);

	#define CP_ACP    0
	#define CP_OEMCP  1
	#define CP_UTF8   65001

	typedef enum tagSTREAM_SEEK {
		STREAM_SEEK_SET = 0,
		STREAM_SEEK_CUR = 1,
		STREAM_SEEK_END = 2
	} STREAM_SEEK;
#endif
//
#include <MyLinux.h>
//#include <NewHandler.h>
//
// NewHandler.h and NewHandler.cpp allows to solve problem with compilers that
// don't throw exception in operator new().
//
// This file must be included before any code that uses operators new() or delete()
// and you must compile and link "NewHandler.cpp", if you use some old MSVC compiler.
//
// The operator new() in some MSVC versions doesn't throw exception std::bad_alloc.
// MSVC 6.0 (_MSC_VER == 1200) doesn't throw exception.
// The code produced by some another MSVC compilers also can be linked
// to library that doesn't throw exception.
// We suppose that code compiled with VS2015+ (_MSC_VER >= 1900) throws exception std::bad_alloc.
// For older _MSC_VER versions we redefine operator new() and operator delete().
// Our version of operator new() throws CNewException() exception on failure.
//
// It's still allowed to use redefined version of operator new() from "NewHandler.cpp"
// with any compiler. 7-Zip's code can work with std::bad_alloc and CNewException() exceptions.
// But if you use some additional code (outside of 7-Zip's code), you must check
// that redefined version of operator new() is not problem for your code.
//
#ifdef _WIN32
	// We can compile my_new and my_delete with _fastcall
	/*
	void * my_new(size_t size);
	void my_delete(void *p) throw();
	// void * my_Realloc(void *p, size_t newSize, size_t oldSize);
	*/
#endif
#if defined(_MSC_VER) && (_MSC_VER < 1900) && !defined(SLIBINCLUDED)
  // If you want to use default operator new(), you can disable the following line
  #define _7ZIP_REDEFINE_OPERATOR_NEW
#endif
#ifdef _7ZIP_REDEFINE_OPERATOR_NEW
	// std::bad_alloc can require additional DLL dependency.
	// So we don't define CNewException as std::bad_alloc here.
	class CNewException {};
	void * __cdecl operator new(size_t size);
	void __cdecl operator delete(void *p) throw();
#else
	#include <new>
	#define CNewException std::bad_alloc
#endif
/*
#ifdef _WIN32
void *
#ifdef _MSC_VER
__cdecl
#endif
operator new[](size_t size);

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete[](void *p) throw();
#endif
*/
//
//#include <Common.h>
//#define ARRAY_SIZE_Removed(a) (sizeof(a) / sizeof((a)[0]))
// 
// There is BUG in MSVC 6.0 compiler for operator new[]:
// It doesn't check overflow, when it calculates size in bytes for allocated array.
// So we can use MY_ARRAY_NEW macro instead of new[] operator. */
// 
#if defined(_MSC_VER) && (_MSC_VER == 1200) && !defined(_WIN64)
  #define MY_ARRAY_NEW(p, T, size) p = new T[(size > (uint)0xFFFFFFFF / sizeof(T)) ? (uint)0xFFFFFFFF / sizeof(T) : size];
#else
  #define MY_ARRAY_NEW(p, T, size) p = new T[size];
#endif
//
//#include <commctrl.h>
//#include <ShlObj.h>
//#include <shlwapi.h>

//#include <7zTypes.h>
#ifdef _WIN32
/* #include <windows.h> */
#endif
//#include <stddef.h>
#ifndef EXTERN_C_BEGIN
	#ifdef __cplusplus
		#define EXTERN_C_BEGIN extern "C" {
		#define EXTERN_C_END }
	#else
		#define EXTERN_C_BEGIN
		#define EXTERN_C_END
	#endif
#endif

EXTERN_C_BEGIN
#define SZ_OK 0

#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12
#define SZ_ERROR_ARCHIVE 16
#define SZ_ERROR_NO_ARCHIVE 17

typedef int SRes;
typedef uchar Byte;
typedef short Int16_Removed;
typedef unsigned short UInt16_Removed;
typedef int Bool;
#define True 1
#define False 0

#ifdef _WIN32
	// typedef DWORD WRes; 
	typedef unsigned WRes;
#else
	typedef int WRes;
#endif
#ifndef RINOK
	#define RINOK(x) { int __result__ = (x); if(__result__ != 0) return __result__; }
#endif
#ifdef _LZMA_UINT32_IS_ULONG
	typedef long Int32_Removed;
	typedef unsigned long UInt32_Removed;
#else
	typedef int Int32_Removed;
	typedef unsigned int UInt32_Removed;
#endif
#ifdef _SZ_NO_INT_64
	/* define _SZ_NO_INT_64, if your compiler doesn't support 64-bit integers.
	NOTES: Some code will work incorrectly in that case! */
	typedef long int64;
	typedef unsigned long UInt64_Removed;
#else
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		typedef __int64 Int64_Removed;
		typedef unsigned __int64 UInt64_Removed;
		#define UINT64_CONST(n) n
	#else
		typedef long long int Int64_Removed;
		typedef unsigned long long int UInt64_Removed;
		#define UINT64_CONST(n) n ## ULL
	#endif
#endif
#ifdef _LZMA_NO_SYSTEM_SIZE_T
	typedef uint32 SizeT;
#else
	typedef size_t SizeT;
#endif
// @sobolev MY_STD_CALL-->_7Z_STD_CALL
#ifndef _7Z_STD_CALL
	#ifdef _WIN32
		#define _7Z_STD_CALL __stdcall
	#else
		#define _7Z_STD_CALL
	#endif
#endif
#ifdef _MSC_VER
	#if _MSC_VER >= 1300
		#define MY_NO_INLINE __declspec(noinline)
	#else
		#define MY_NO_INLINE
	#endif
	#define MY_FORCE_INLINE_Removed __forceinline
	#define MY_CDECL __cdecl
	//#define MY_FAST_CALL_Removed __fastcall
#else
	#define MY_NO_INLINE
	#define FORCEINLINE
	#define MY_CDECL
	//#define MY_FAST_CALL_Removed
	/* inline keyword : for C++ / C99 */

	/* GCC, clang: */
	/*
	#if defined (__GNUC__) && (__GNUC__ >= 4)
	#define FORCEINLINE __attribute__((always_inline))
	#define MY_NO_INLINE __attribute__((noinline))
	#endif
	*/
#endif

struct ISequentialOutStream;
struct ISequentialInStream;
struct IDirItemsCallback;
//
// The following interfaces use first parameter as pointer to structure 
//
struct IByteIn { Byte (* Read)(const IByteIn * p); /* reads one byte, returns 0 in case of EOF or error */ };
struct IByteOut { void (* Write)(const IByteOut * p, Byte b); };
struct ISeqInStream { SRes (* Read)(const ISeqInStream * p, void * buf, size_t * size); /* if(input(*size) != 0 && output(*size) == 0) means end_of_stream. (output(*size) < input(*size)) is allowed */ };
struct ISeqOutStream { size_t (* Write)(const ISeqOutStream * p, const void * buf, size_t size); /* Returns: result - the number of actually written bytes. (result < size) means error */ };

/* it can return SZ_ERROR_INPUT_EOF */
SRes SeqInStream_Read(const ISeqInStream * stream, void * buf, size_t size);
SRes SeqInStream_Read2(const ISeqInStream * stream, void * buf, size_t size, SRes errorType);
SRes SeqInStream_ReadByte(const ISeqInStream * stream, Byte * buf);

typedef enum {
	SZ_SEEK_SET = 0,
	SZ_SEEK_CUR = 1,
	SZ_SEEK_END = 2
} ESzSeek;

struct ISeekInStream {
	SRes (* Read)(const ISeekInStream * p, void * buf, size_t * size); /* same as ISeqInStream::Read */
	SRes (* Seek)(const ISeekInStream * p, int64 * pos, ESzSeek origin);
};

struct ILookInStream {
	SRes (* Look)(const ILookInStream * p, const void ** buf, size_t * size);
	/* if(input(*size) != 0 && output(*size) == 0) means end_of_stream.
	   (output(*size) > input(*size)) is not allowed
	   (output(*size) < input(*size)) is allowed */
	SRes (* Skip)(const ILookInStream * p, size_t offset);
	/* offset must be <= output(*size) of Look */
	SRes (* Read)(const ILookInStream * p, void * buf, size_t * size);
	/* reads directly (without buffer). It's same as ISeqInStream::Read */
	SRes (* Seek)(const ILookInStream * p, int64 * pos, ESzSeek origin);
};

#define IByteIn_Read(p)                    (p)->Read(p)
#define IByteOut_Write(p, b)               (p)->Write(p, b)
#define ISeqInStream_Read(p, buf, size)    (p)->Read(p, buf, size)
#define ISeqOutStream_Write(p, buf, size)  (p)->Write(p, buf, size)
#define ISeekInStream_Read(p, buf, size)   (p)->Read(p, buf, size)
#define ISeekInStream_Seek(p, pos, origin) (p)->Seek(p, pos, origin)
#define ILookInStream_Look(p, buf, size)   (p)->Look(p, buf, size)
#define ILookInStream_Skip(p, offset)      (p)->Skip(p, offset)
#define ILookInStream_Read(p, buf, size)   (p)->Read(p, buf, size)
#define ILookInStream_Seek(p, pos, origin) (p)->Seek(p, pos, origin)

SRes FASTCALL LookInStream_LookRead(const ILookInStream * stream, void * buf, size_t * size);
SRes FASTCALL LookInStream_SeekTo(const ILookInStream * stream, uint64 offset);
/* reads via ILookInStream::Read */
SRes FASTCALL LookInStream_Read2(const ILookInStream * stream, void * buf, size_t size, SRes errorType);
SRes FASTCALL LookInStream_Read(const ILookInStream * stream, void * buf, size_t size);

typedef struct {
	ILookInStream vt;
	const ISeekInStream * realStream;
	size_t pos;
	size_t size; /* it's data size */
	/* the following variables must be set outside */
	Byte * buf;
	size_t bufSize;
} CLookToRead2;

void LookToRead2_CreateVTable(CLookToRead2 * p, int lookahead);

#define LookToRead2_Init(p) { (p)->pos = (p)->size = 0; }

typedef struct {
	ISeqInStream vt;
	const ILookInStream * realStream;
} CSecToLook;

void SecToLook_CreateVTable(CSecToLook * p);

typedef struct {
	ISeqInStream vt;
	const ILookInStream * realStream;
} CSecToRead;

void SecToRead_CreateVTable(CSecToRead * p);

struct ICompressProgress {
	SRes (* Progress)(const ICompressProgress * p, uint64 inSize, uint64 outSize);
	/* Returns: result. (result != SZ_OK) means break.
	   Value static_cast<uint64>(-1LL) for size means unknown value. */
};

#define ICompressProgress_Progress(p, inSize, outSize) (p)->Progress(p, inSize, outSize)

//typedef struct IByteIn IByteIn;
//typedef struct IByteOut IByteOut;
//typedef struct ISeqInStream ISeqInStream;
//typedef struct ISeqOutStream ISeqOutStream;
//typedef struct ISeekInStream ISeekInStream;
//typedef struct ILookInStream ILookInStream;
//typedef struct ICompressProgress ICompressProgress;
//typedef struct ISzAlloc ISzAlloc;

struct ISzAlloc {
	void *(*Alloc)(const ISzAlloc * p, size_t size);
	void (* Free)(const ISzAlloc * p, void * address); /* address can be 0 */
};

typedef const ISzAlloc * ISzAllocPtr;

#define ISzAlloc_Alloc(p, size) (p)->Alloc(p, size)
#define ISzAlloc_Free(p, a) (p)->Free(p, a)
/* deprecated */
#define IAlloc_Alloc(p, size) ISzAlloc_Alloc(p, size)
#define IAlloc_Free(p, a) ISzAlloc_Free(p, a)
#ifndef MY_offsetof
	#ifdef offsetof
		#define MY_offsetof(type, m) offsetof(type, m)
		//#define MY_offsetof(type, m) FIELD_OFFSET(type, m)
	#else
		#define MY_offsetof(type, m) ((size_t)&(((type*)0)->m))
	#endif
#endif
#ifndef MY_container_of
	//#define MY_container_of(ptr, type, m) container_of(ptr, type, m)
	//#define MY_container_of(ptr, type, m) CONTAINING_RECORD(ptr, type, m)
	//#define MY_container_of(ptr, type, m) ((type *)((char *)(ptr) - offsetof(type, m)))
	//#define MY_container_of(ptr, type, m) (&((type *)0)->m == (ptr), ((type *)(((char *)(ptr)) - MY_offsetof(type, m))))
	/*
	GCC shows warning: "perhaps the 'offsetof' macro was used incorrectly"
		GCC 3.4.4 : classes with constructor
		GCC 4.8.1 : classes with non-public variable members"
	*/
	#define MY_container_of(ptr, type, m) ((type*)((char *)(1 ? (ptr) : &((type*)0)->m) - MY_offsetof(type, m)))
#endif

#define CONTAINER_FROM_VTBL_SIMPLE(ptr, type, m) ((type*)(ptr))
/*
   #define CONTAINER_FROM_VTBL(ptr, type, m) CONTAINER_FROM_VTBL_SIMPLE(ptr, type, m)
 */
#define CONTAINER_FROM_VTBL(ptr, type, m) MY_container_of(ptr, type, m)
#define CONTAINER_FROM_VTBL_CLS(ptr, type, m) CONTAINER_FROM_VTBL_SIMPLE(ptr, type, m)
/*
   #define CONTAINER_FROM_VTBL_CLS(ptr, type, m) CONTAINER_FROM_VTBL(ptr, type, m)
 */
#ifdef _WIN32
	#define CHAR_PATH_SEPARATOR '\\'
	#define WCHAR_PATH_SEPARATOR L'\\'
	#define STRING_PATH_SEPARATOR "\\"
	#define WSTRING_PATH_SEPARATOR L"\\"
#else
	#define CHAR_PATH_SEPARATOR '/'
	#define WCHAR_PATH_SEPARATOR L'/'
	#define STRING_PATH_SEPARATOR "/"
	#define WSTRING_PATH_SEPARATOR L"/"
#endif

EXTERN_C_END
//
//#include <Defs.h>
template <class T> inline T MyMin(T a, T b) { return a < b ? a : b; }
template <class T> inline T MyMax(T a, T b) { return a > b ? a : b; }
template <class T> inline int MyCompare(T a, T b) { return a == b ? 0 : (a < b ? -1 : 1); }
inline int BoolToInt(bool v) { return (v ? 1 : 0); }
inline bool IntToBool(int v) { return (v != 0); }
//
//#include <Alloc.h>
EXTERN_C_BEGIN
	// void * MyAlloc_Removed(size_t size);
	// void MyFree_Removed(void * address);
	#ifdef _WIN32
		void SetLargePageSize();
		void * MidAlloc(size_t size);
		void MidFree(void * address);
		void * BigAlloc(size_t size);
		void BigFree(void * address);
	#else
		#define MidAlloc(size) SAlloc::M(size)
		#define MidFree(address) SAlloc::F(address)
		#define BigAlloc(size) SAlloc::M(size)
		#define BigFree(address) SAlloc::F(address)
	#endif
	extern const ISzAlloc g_Alloc;
	extern const ISzAlloc g_BigAlloc;
EXTERN_C_END
//
//#include <AutoPtr.h>
//
template <class T> class CMyAutoPtr {
public:
	CMyAutoPtr(T * p = 0) : _p(p) {}
	CMyAutoPtr(CMyAutoPtr<T>& p) : _p(p.release()) {}
	CMyAutoPtr<T>& operator=(CMyAutoPtr<T>& p)
	{
		reset(p.release());
		return (*this);
	}
	~CMyAutoPtr() 
	{
		delete _p;
	}
	T & operator*() const { return *_p; }
	// T* operator->() const { return (&**this); }
	T * get() const { return _p; }
	T * release()
	{
		T * tmp = _p;
		_p = 0;
		return tmp;
	}
	void reset(T* p = 0)
	{
		if(p != _p)
			delete _p;
		_p = p;
	}
private:
	T * _p;
};
//
//#include <CpuArch.h>
EXTERN_C_BEGIN
	/*
	MY_CPU_LE means that CPU is LITTLE ENDIAN.
	MY_CPU_BE means that CPU is BIG ENDIAN.
	If MY_CPU_LE and MY_CPU_BE are not defined, we don't know about ENDIANNESS of platform.

	MY_CPU_LE_UNALIGN means that CPU is LITTLE ENDIAN and CPU supports unaligned memory accesses.
	*/
	#if  defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__AMD64__) || defined(__amd64__)
		#define MY_CPU_AMD64
		#ifdef __ILP32__
			#define MY_CPU_NAME "x32"
		#else
			#define MY_CPU_NAME "x64"
		#endif
		#define MY_CPU_64BIT
	#endif
	#if  defined(_M_IX86) || defined(__i386__)
		#define MY_CPU_X86
		#define MY_CPU_NAME "x86"
		#define MY_CPU_32BIT
	#endif
	#if  defined(_M_ARM64) || defined(__AARCH64EL__) || defined(__AARCH64EB__) || defined(__aarch64__)
		#define MY_CPU_ARM64
		#define MY_CPU_NAME "arm64"
		#define MY_CPU_64BIT
	#endif
	#if  defined(_M_ARM) || defined(_M_ARM_NT) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__) || \
		defined(__ARMEL__) || defined(__ARMEB__) || defined(__THUMBEL__) || defined(__THUMBEB__)
		#define MY_CPU_ARM
		#define MY_CPU_NAME "arm"
		#define MY_CPU_32BIT
	#endif
	#if defined(_M_IA64) || defined(__ia64__)
		#define MY_CPU_IA64
		#define MY_CPU_NAME "ia64"
		#define MY_CPU_64BIT
	#endif
	#if  defined(__mips64) || defined(__mips64__) || (defined(__mips) && (__mips == 64 || __mips == 4 || __mips == 3))
		#define MY_CPU_NAME "mips64"
		#define MY_CPU_64BIT
	#elif defined(__mips__)
		#define MY_CPU_NAME "mips"
		/* #define MY_CPU_32BIT */
	#endif
	#if  defined(__ppc64__)	|| defined(__powerpc64__)
	#ifdef __ILP32__
		#define MY_CPU_NAME "ppc64-32"
	#else
		#define MY_CPU_NAME "ppc64"
	#endif
	#define MY_CPU_64BIT
	#elif defined(__ppc__) || defined(__powerpc__)
		#define MY_CPU_NAME "ppc"
		#define MY_CPU_32BIT
	#endif
	#if  defined(__sparc64__)
		#define MY_CPU_NAME "sparc64"
		#define MY_CPU_64BIT
	#elif defined(__sparc__)
		#define MY_CPU_NAME "sparc"
		/* #define MY_CPU_32BIT */
	#endif
	#if defined(MY_CPU_X86) || defined(MY_CPU_AMD64)
		#define MY_CPU_X86_OR_AMD64
	#endif
	#ifdef _WIN32
		#ifdef MY_CPU_ARM
			#define MY_CPU_ARM_LE
		#endif
		#ifdef MY_CPU_ARM64
			#define MY_CPU_ARM64_LE
		#endif
		#ifdef _M_IA64
			#define MY_CPU_IA64_LE
		#endif
	#endif
	#if defined(MY_CPU_X86_OR_AMD64) || defined(MY_CPU_ARM_LE) || defined(MY_CPU_ARM64_LE) || \
		defined(MY_CPU_IA64_LE) || defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || \
		defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(__MIPSEL__) || defined(__MIPSEL) || \
		defined(_MIPSEL) || defined(__BFIN__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
	#define MY_CPU_LE
	#endif
	#if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
		defined(__AARCH64EB__) || defined(__MIPSEB__) || defined(__MIPSEB) || \
		defined(_MIPSEB) || defined(__m68k__) || defined(__s390__) || defined(__s390x__) || \
		defined(__zarch__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
	#define MY_CPU_BE
	#endif
	#if defined(MY_CPU_LE) && defined(MY_CPU_BE)
		#error Stop_Compiling_Bad_Endian
	#endif
	#if defined(MY_CPU_32BIT) && defined(MY_CPU_64BIT)
		#error Stop_Compiling_Bad_32_64_BIT
	#endif
	#ifndef MY_CPU_NAME
		#ifdef MY_CPU_LE
			#define MY_CPU_NAME "LE"
		#elif defined(MY_CPU_BE)
			#define MY_CPU_NAME "BE"
		#else
			//#define MY_CPU_NAME ""
	#endif
	#endif
	#ifdef MY_CPU_LE
		#if defined(MY_CPU_X86_OR_AMD64) || defined(MY_CPU_ARM64) || defined(__ARM_FEATURE_UNALIGNED) || defined(__AARCH64EL__)
			#define MY_CPU_LE_UNALIGN
		#endif
	#endif
	#ifdef MY_CPU_LE_UNALIGN
		#define GetUi16(p) (*(const uint16*)(const void *)(p))
		#define GetUi32(p) (*(const uint32*)(const void *)(p))
		#define GetUi64(p) (*(const uint64*)(const void *)(p))
		#define SetUi16(p, v) { *(uint16 *)(p) = (v); }
		#define SetUi32(p, v) { *(uint32 *)(p) = (v); }
		#define SetUi64(p, v) { *(uint64 *)(p) = (v); }
	#else
		#define GetUi16(p) ( static_cast<uint16>(((const Byte *)(p))[0] | (static_cast<uint16>((const Byte *)(p))[1] << 8) ))
		#define GetUi32(p) ( ((const Byte *)(p))[0]        | \
			((uint32)((const Byte *)(p))[1] <<  8) | ((uint32)((const Byte *)(p))[2] << 16) | ((uint32)((const Byte *)(p))[3] << 24))
		#define GetUi64(p) (GetUi32(p) | ((uint64)GetUi32(((const Byte *)(p)) + 4) << 32))
		#define SetUi16(p, v) { Byte * _ppp_ = (Byte *)(p); uint32 _vvv_ = (v); \
			_ppp_[0] = (Byte)_vvv_;	_ppp_[1] = (Byte)(_vvv_ >> 8); }
		#define SetUi32(p, v) { Byte * _ppp_ = (Byte *)(p); uint32 _vvv_ = (v); \
			_ppp_[0] = (Byte)_vvv_;	_ppp_[1] = (Byte)(_vvv_ >> 8); _ppp_[2] = (Byte)(_vvv_ >> 16); _ppp_[3] = (Byte)(_vvv_ >> 24); }
		#define SetUi64(p, v) { Byte * _ppp2_ = (Byte *)(p); uint64 _vvv2_ = (v); \
			SetUi32(_ppp2_, (uint32)_vvv2_); SetUi32(_ppp2_ + 4, (uint32)(_vvv2_ >> 32)); }
	#endif
	#if defined(MY_CPU_LE_UNALIGN) && /* defined(_WIN64) && */ (_MSC_VER >= 1300)
		/* Note: we use bswap instruction, that is unsupported in 386 cpu */
		//#include <stdlib.h>
		#pragma intrinsic(_byteswap_ulong)
		#pragma intrinsic(_byteswap_uint64)
		#define GetBe32(p) _byteswap_ulong(*(const uint32*)(const Byte *)(p))
		#define GetBe64(p) _byteswap_uint64(*(const uint64*)(const Byte *)(p))
		#define SetBe32(p, v) (*(uint32 *)(void *)(p)) = _byteswap_ulong(v)
	#elif defined(MY_CPU_LE_UNALIGN) && defined (__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
		#define GetBe32(p) __builtin_bswap32(*(const uint32*)(const Byte *)(p))
		#define GetBe64(p) __builtin_bswap64(*(const uint64*)(const Byte *)(p))
		#define SetBe32(p, v) (*(uint32 *)(void *)(p)) = __builtin_bswap32(v)
	#else
		#define GetBe32(p) (((uint32)((const Byte *)(p))[0] << 24) | \
				((uint32)((const Byte *)(p))[1] << 16) | ((uint32)((const Byte *)(p))[2] <<  8) | \
				((const Byte *)(p))[3] )
		#define GetBe64(p) (((uint64)GetBe32(p) << 32) | GetBe32(((const Byte *)(p)) + 4))
		#define SetBe32(p, v) { Byte * _ppp_ = (Byte *)(p); uint32 _vvv_ = (v); _ppp_[0] = (Byte)(_vvv_ >> 24);	\
			_ppp_[1] = (Byte)(_vvv_ >> 16);	_ppp_[2] = (Byte)(_vvv_ >> 8); _ppp_[3] = (Byte)_vvv_; }
	#endif
	#define GetBe16(p) (static_cast<uint16>(((uint16)((const Byte *)(p))[0] << 8) | ((const Byte *)(p))[1]))
	#ifdef MY_CPU_X86_OR_AMD64
		typedef struct {
			uint32 maxFunc;
			uint32 vendor[3];
			uint32 ver;
			uint32 b;
			uint32 c;
			uint32 d;
		} Cx86cpuid;

		enum {
			CPU_FIRM_INTEL,
			CPU_FIRM_AMD,
			CPU_FIRM_VIA
		};

		void MyCPUID(uint32 function, uint32 * a, uint32 * b, uint32 * c, uint32 * d);
		Bool x86cpuid_CheckAndRead(Cx86cpuid * p);
		int x86cpuid_GetFirm(const Cx86cpuid * p);

		#define x86cpuid_GetFamily(ver) (((ver >> 16) & 0xFF0) | ((ver >> 8) & 0xF))
		#define x86cpuid_GetModel(ver)  (((ver >> 12) &  0xF0) | ((ver >> 4) & 0xF))
		#define x86cpuid_GetStepping(ver) (ver & 0xF)

		Bool CPU_Is_InOrder();
		Bool CPU_Is_Aes_Supported();
	#endif
EXTERN_C_END
//
//#include <7zCrc.h>
EXTERN_C_BEGIN
	extern uint32 g_CrcTable[];
	// Call CrcGenerateTable one time before other CRC functions 
	void FASTCALL CrcGenerateTable(void);
	#define CRC_INIT_VAL 0xFFFFFFFF
	#define CRC_GET_DIGEST(crc) ((crc) ^ CRC_INIT_VAL)
	#define CRC_UPDATE_BYTE(crc, b) (g_CrcTable[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))
	uint32 FASTCALL CrcUpdate(uint32 crc, const void *data, size_t size);
	uint32 FASTCALL CrcCalc(const void *data, size_t size);
EXTERN_C_END
//
//#include <Sort.h>
EXTERN_C_BEGIN
	void HeapSort(uint32 *p, size_t size);
	void HeapSort64(uint64 *p, size_t size);
	// void HeapSortRef(uint32 *p, uint32 *vals, size_t size); 
EXTERN_C_END
//
//#include <BwtSort.h>
EXTERN_C_BEGIN
	/* use BLOCK_SORT_EXTERNAL_FLAGS if blockSize can be > 1M */
	/* #define BLOCK_SORT_EXTERNAL_FLAGS */
	#ifdef BLOCK_SORT_EXTERNAL_FLAGS
		#define BLOCK_SORT_EXTERNAL_SIZE(blockSize) ((((blockSize) + 31) >> 5))
	#else
		#define BLOCK_SORT_EXTERNAL_SIZE(blockSize) 0
	#endif
	#define BLOCK_SORT_BUF_SIZE(blockSize) ((blockSize) * 2 + BLOCK_SORT_EXTERNAL_SIZE(blockSize) + (1 << 16))
	uint32 BlockSort(uint32 *indices, const Byte *data, uint32 blockSize);
EXTERN_C_END
//
//#include <MyException.h>
struct CSystemException {
	HRESULT ErrorCode;
	CSystemException(HRESULT errorCode) : ErrorCode(errorCode) 
	{
	}
};
//
//#include <ComTry.h>
#define COM_TRY_BEGIN try {
#define COM_TRY_END } catch(...) { return E_OUTOFMEMORY; }
/*
#define COM_TRY_END } \
  catch(const CNewException &) { return E_OUTOFMEMORY; } \
  catch(...) { return HRESULT_FROM_WIN32(ERROR_NOACCESS); } \
*/
  // catch(const CSystemException &e) { return e.ErrorCode; }
  // catch(...) { return E_FAIL; }
//
//#include <MyCom.h>
#ifndef RINOK
	#define RINOK(x) { HRESULT __result_ = (x); if(__result_ != S_OK) return __result_; }
#endif

template <class T> class CMyComPtr {
public:
	CMyComPtr() : _p(NULL) 
	{
	}
	CMyComPtr(T* p) throw() 
	{
		if((_p = p) != NULL) 
			p->AddRef();
	}
	CMyComPtr(const CMyComPtr<T>& lp) throw() 
	{
		if((_p = lp._p) != NULL) 
			_p->AddRef();
	}
	~CMyComPtr() 
	{
		CALLPTRMEMB(_p, Release());
	}
	void Release() 
	{
		if(_p) {
			_p->Release(); 
			_p = NULL;
		}
	}
	operator T * () const {  return (T*)_p;  }
	// T& operator*() const {  return *_p; }
	T** operator &() { return &_p; }
	T* operator->() const { return _p; }
	T* operator=(T* p)
	{
		CALLPTRMEMB(p, AddRef());
		CALLPTRMEMB(_p, Release());
		_p = p;
		return p;
	}
	T * operator=(const CMyComPtr<T>& lp) { return (*this = lp._p); }
	bool operator!() const { return (_p == NULL); }
	// bool operator == (T* pT) const {  return _p == pT; }
	void Attach(T* p2)
	{
		Release();
		_p = p2;
	}
	T* Detach()
	{
		T * pt = _p;
		_p = NULL;
		return pt;
	}
  #ifdef _WIN32
	HRESULT CoCreateInstance(REFCLSID rclsid, REFIID iid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	{
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, (void **)&_p);
	}
  #endif
	/*
	   HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
	   {
	   CLSID clsid;
	   HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
	   ATLASSERT(_p == NULL);
	   if(SUCCEEDED(hr))
	    hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void **)&_p);
	   return hr;
	   }
	 */
	template <class Q> HRESULT QueryInterface(REFGUID iid, Q** pp) const throw()
	{
		return _p->QueryInterface(iid, (void **)pp);
	}
private:
	T * _p;
};

inline HRESULT StringToBstr(LPCOLESTR src, BSTR * bstr)
{
	*bstr = ::SysAllocString(src);
	return (*bstr) ? S_OK : E_OUTOFMEMORY;
}

class CMyComBSTR {
	BSTR m_str;
public:
	CMyComBSTR() : m_str(NULL) 
	{
	}
	~CMyComBSTR() 
	{
		::SysFreeString(m_str);
	}
	BSTR * operator &() { return &m_str; }
	operator LPCOLESTR() const { return m_str; }
	// operator bool() const { return m_str != NULL; }
	// bool operator!() const { return m_str == NULL; }
private:
	// operator BSTR() const { return m_str; }
	CMyComBSTR(LPCOLESTR src) 
	{
		m_str = ::SysAllocString(src);
	}
	// CMyComBSTR(int nSize) { m_str = ::SysAllocStringLen(NULL, nSize); }
	// CMyComBSTR(int nSize, LPCOLESTR sz) { m_str = ::SysAllocStringLen(sz, nSize);  }
	CMyComBSTR(const CMyComBSTR& src) 
	{
		m_str = src.MyCopy();
	}
	/*
	   CMyComBSTR(REFGUID src)
	   {
	   LPOLESTR szGuid;
	   StringFromCLSID(src, &szGuid);
	   m_str = ::SysAllocString(szGuid);
	   CoTaskMemFree(szGuid);
	   }
	 */
	CMyComBSTR& operator=(const CMyComBSTR& src)
	{
		if(m_str != src.m_str) {
			if(m_str)
				::SysFreeString(m_str);
			m_str = src.MyCopy();
		}
		return *this;
	}
	CMyComBSTR& operator=(LPCOLESTR src)
	{
		::SysFreeString(m_str);
		m_str = ::SysAllocString(src);
		return *this;
	}
	unsigned Len() const { return ::SysStringLen(m_str); }
	BSTR MyCopy() const
	{
		// We don't support Byte BSTRs here
		return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
		/*
		   UINT byteLen = ::SysStringByteLen(m_str);
		   BSTR res = ::SysAllocStringByteLen(NULL, byteLen);
		   if(res && byteLen != 0 && m_str)
		   memcpy(res, m_str, byteLen);
		   return res;
		 */
	}
	/*
	   void Attach(BSTR src) { m_str = src; }
	   BSTR Detach()
	   {
	   BSTR s = m_str;
	   m_str = NULL;
	   return s;
	   }
	 */
	void Empty()
	{
		::SysFreeString(m_str);
		m_str = NULL;
	}
};
/*
   If CMyUnknownImp doesn't use virtual destructor, the code size is smaller.
   But if some class_1 derived from CMyUnknownImp
    uses MY_ADDREF_RELEASE and IUnknown::Release()
    and some another class_2 is derived from class_1,
    then class_1 must use virtual destructor: virtual ~class_1();
    In that case, class_1::Release() calls correct destructor of class_2.

   Also you can use virtual ~CMyUnknownImp(), if you want to disable warning
    "class has virtual functions, but destructor is not virtual".
 */
class CMyUnknownImp {
public:
	CMyUnknownImp() : __m_RefCount(0) 
	{
	}
	//virtual
	~CMyUnknownImp() 
	{
	}
	ULONG __m_RefCount;
};

#define MY_QUERYINTERFACE_BEGIN STDMETHOD(QueryInterface) (REFGUID iid, void ** outObject) throw() { *outObject = NULL;
#define MY_QUERYINTERFACE_ENTRY(i) else if(iid == IID_ ## i) { *outObject = (void *)(i*)this; }
#define MY_QUERYINTERFACE_ENTRY_UNKNOWN(i) if(iid == IID_IUnknown) { *outObject = (void *)(IUnknown*)(i*)this; }
#define MY_QUERYINTERFACE_BEGIN2(i) MY_QUERYINTERFACE_BEGIN MY_QUERYINTERFACE_ENTRY_UNKNOWN(i) MY_QUERYINTERFACE_ENTRY(i)

#define MY_QUERYINTERFACE_END else return E_NOINTERFACE; ++__m_RefCount; /* AddRef(); */ return S_OK; }

#define MY_ADDREF_RELEASE \
	STDMETHOD_(ULONG, AddRef) () throw() { return ++__m_RefCount; }	\
	      STDMETHOD_(ULONG, Release) () { if(--__m_RefCount != 0) return __m_RefCount; delete this; return 0; }

#define MY_UNKNOWN_IMP_SPEC(i) MY_QUERYINTERFACE_BEGIN i MY_QUERYINTERFACE_END MY_ADDREF_RELEASE
#define MY_UNKNOWN_IMP MY_QUERYINTERFACE_BEGIN MY_QUERYINTERFACE_ENTRY_UNKNOWN(IUnknown) MY_QUERYINTERFACE_END MY_ADDREF_RELEASE
#define MY_UNKNOWN_IMP1(i) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i) MY_QUERYINTERFACE_ENTRY(i))
#define MY_UNKNOWN_IMP2(i1, i2) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i1)	MY_QUERYINTERFACE_ENTRY(i1)	MY_QUERYINTERFACE_ENTRY(i2)	)
#define MY_UNKNOWN_IMP3(i1, i2, i3) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i1)	MY_QUERYINTERFACE_ENTRY(i1)	MY_QUERYINTERFACE_ENTRY(i2)	MY_QUERYINTERFACE_ENTRY(i3))
#define MY_UNKNOWN_IMP4(i1, i2, i3, i4) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i1)	MY_QUERYINTERFACE_ENTRY(i1)	MY_QUERYINTERFACE_ENTRY(i2)	MY_QUERYINTERFACE_ENTRY(i3)	MY_QUERYINTERFACE_ENTRY(i4))
#define MY_UNKNOWN_IMP5(i1, i2, i3, i4, i5) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i1)	MY_QUERYINTERFACE_ENTRY(i1)	MY_QUERYINTERFACE_ENTRY(i2)	\
	MY_QUERYINTERFACE_ENTRY(i3)	MY_QUERYINTERFACE_ENTRY(i4)	MY_QUERYINTERFACE_ENTRY(i5))
#define MY_UNKNOWN_IMP6(i1, i2, i3, i4, i5, i6) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i1)	MY_QUERYINTERFACE_ENTRY(i1)	MY_QUERYINTERFACE_ENTRY(i2)	\
	MY_QUERYINTERFACE_ENTRY(i3)	MY_QUERYINTERFACE_ENTRY(i4)	MY_QUERYINTERFACE_ENTRY(i5)	MY_QUERYINTERFACE_ENTRY(i6))
#define MY_UNKNOWN_IMP7(i1, i2, i3, i4, i5, i6, i7) MY_UNKNOWN_IMP_SPEC(MY_QUERYINTERFACE_ENTRY_UNKNOWN(i1)	MY_QUERYINTERFACE_ENTRY(i1)	MY_QUERYINTERFACE_ENTRY(i2)	\
	MY_QUERYINTERFACE_ENTRY(i3)	MY_QUERYINTERFACE_ENTRY(i4)	MY_QUERYINTERFACE_ENTRY(i5)	MY_QUERYINTERFACE_ENTRY(i6)	MY_QUERYINTERFACE_ENTRY(i7))

const HRESULT k_My_HRESULT_WritingWasCut = 0x20000010;
//
//#include <MyTypes.h>
typedef int HRes;

struct CBoolPair {
	CBoolPair() : Val(false), Def(false) 
	{
	}
	void Init()
	{
		Val = false;
		Def = false;
	}
	void SetTrueTrue()
	{
		Val = true;
		Def = true;
	}
	bool   Val;
	bool   Def;
	uint8  Reserve[2]; // @alignment
};

#define CLASS_NO_COPY(cls) private: cls(const cls &); cls & operator=(const cls &);
//
//#include <MyVector.h>
template <class T> class CRecordVector {
public:
	CRecordVector() : _items(0), _size(0), _capacity(0) 
	{
	}
	CRecordVector(const CRecordVector &v) : _items(0), _size(0), _capacity(0)
	{
		uint size = v.Size();
		if(size != 0) {
			_items = new T[size];
			_size = size;
			_capacity = size;
			memcpy(_items, v._items, (size_t)size * sizeof(T));
		}
	}
	unsigned Size() const { return _size; }
	bool IsEmpty() const { return _size == 0; }
	void ConstructReserve(uint size)
	{
		if(size != 0) {
			MY_ARRAY_NEW(_items, T, size)
			// _items = new T[size];
			_capacity = size;
		}
	}
	void Reserve(unsigned newCapacity)
	{
		if(newCapacity > _capacity) {
			T * p;
			MY_ARRAY_NEW(p, T, newCapacity);
			// p = new T[newCapacity];
			if(_size != 0)
				memcpy(p, _items, (size_t)_size * sizeof(T));
			delete []_items;
			_items = p;
			_capacity = newCapacity;
		}
	}
	void ClearAndReserve(unsigned newCapacity)
	{
		Clear();
		if(newCapacity > _capacity) {
			delete []_items;
			_items = NULL;
			_capacity = 0;
			MY_ARRAY_NEW(_items, T, newCapacity)
			// _items = new T[newCapacity];
			_capacity = newCapacity;
		}
	}
	void ClearAndSetSize(unsigned newSize)
	{
		ClearAndReserve(newSize);
		_size = newSize;
	}
	void ChangeSize_KeepData(unsigned newSize)
	{
		if(newSize > _capacity) {
			T * p;
			MY_ARRAY_NEW(p, T, newSize)
			// p = new T[newSize];
			if(_size != 0)
				memcpy(p, _items, (size_t)_size * sizeof(T));
			delete []_items;
			_items = p;
			_capacity = newSize;
		}
		_size = newSize;
	}
	void ReserveDown()
	{
		if(_size != _capacity) {
			T * p = NULL;
			if(_size != 0) {
				p = new T[_size];
				memcpy(p, _items, (size_t)_size * sizeof(T));
			}
			delete []_items;
			_items = p;
			_capacity = _size;
		}
	}
	~CRecordVector() 
	{
		delete []_items;
	}
	void ClearAndFree()
	{
		delete []_items;
		_items = NULL;
		_size = 0;
		_capacity = 0;
	}
	void Clear() { _size = 0; }
	void DeleteBack() { _size--; }
	void DeleteFrom(uint index)
	{
		// if(index <= _size)
		_size = index;
	}
	void DeleteFrontal(unsigned num)
	{
		if(num != 0) {
			MoveItems(0, num);
			_size -= num;
		}
	}
	void Delete(uint index)
	{
		MoveItems(index, index + 1);
		_size -= 1;
	}
	/*
	   void Delete(uint index, unsigned num)
	   {
	   if(num > 0)
	   {
	    MoveItems(index, index + num);
	    _size -= num;
	   }
	   }
	 */
	CRecordVector & operator = (const CRecordVector &v)
	{
		if(&v == this)
			return *this;
		uint size = v.Size();
		if(size > _capacity) {
			delete []_items;
			_capacity = 0;
			_size = 0;
			_items = NULL;
			_items = new T[size];
			_capacity = size;
		}
		_size = size;
		if(size != 0)
			memcpy(_items, v._items, (size_t)size * sizeof(T));
		return *this;
	}
	CRecordVector & operator += (const CRecordVector &v)
	{
		uint size = v.Size();
		Reserve(_size + size);
		if(size != 0)
			memcpy(_items + _size, v._items, (size_t)size * sizeof(T));
		_size += size;
		return *this;
	}
	unsigned Add(const T item)
	{
		ReserveOnePosition();
		_items[_size] = item;
		return _size++;
	}
	void AddInReserved(const T item)
	{
		_items[_size++] = item;
	}
	void Insert(uint index, const T item)
	{
		ReserveOnePosition();
		MoveItems(index + 1, index);
		_items[index] = item;
		_size++;
	}
	void MoveToFront(uint index)
	{
		if(index != 0) {
			T temp = _items[index];
			memmove(_items + 1, _items, (size_t)index * sizeof(T));
			_items[0] = temp;
		}
	}
	const T & operator[] (uint index) const { return _items[index]; }
	T & operator[](uint index) { return _items[index]; }
	const T & Front() const { return _items[0]; }
	T & Front() { return _items[0]; }
	const T& Back() const { return _items[(size_t)_size - 1]; }
	T & Back() { return _items[(size_t)_size - 1]; }
	/*
	   void Swap(unsigned i, unsigned j)
	   {
	   T temp = _items[i];
	   _items[i] = _items[j];
	   _items[j] = temp;
	   }
	 */
	int FindInSorted(const T item) const { return FindInSorted(item, 0, _size); }
	int FindInSorted2(const T &item) const { return FindInSorted2(item, 0, _size); }
	unsigned AddToUniqueSorted(const T item)
	{
		unsigned left = 0, right = _size;
		while(left != right) {
			unsigned mid = (left + right) / 2;
			const T midVal = (*this)[mid];
			if(item == midVal)
				return mid;
			if(item < midVal)
				right = mid;
			else
				left = mid + 1;
		}
		Insert(right, item);
		return right;
	}
	unsigned AddToUniqueSorted2(const T &item)
	{
		unsigned left = 0, right = _size;
		while(left != right) {
			unsigned mid = (left + right) / 2;
			const T& midVal = (*this)[mid];
			int comp = item.Compare(midVal);
			if(comp == 0)
				return mid;
			if(comp < 0)
				right = mid;
			else
				left = mid + 1;
		}
		Insert(right, item);
		return right;
	}
	static void SortRefDown(T* p, unsigned k, uint size, int (* compare)(const T*, const T*, void *), void * param)
	{
		T temp = p[k];
		for(;; ) {
			unsigned s = (k << 1);
			if(s > size)
				break;
			if(s < size && compare(p + s + 1, p + s, param) > 0)
				s++;
			if(compare(&temp, p + s, param) >= 0)
				break;
			p[k] = p[s];
			k = s;
		}
		p[k] = temp;
	}
	void Sort(int (* compare)(const T*, const T*, void *), void * param)
	{
		uint size = _size;
		if(size > 1) {
			T * p = (&Front()) - 1;
			{
				unsigned i = size >> 1;
				do {
					SortRefDown(p, i, size, compare, param);
				} while(--i != 0);
			}
			do {
				T temp = p[size];
				p[size--] = p[1];
				p[1] = temp;
				SortRefDown(p, 1, size, compare, param);
			} while(size > 1);
		}
	}
	static void SortRefDown2(T* p, unsigned k, uint size)
	{
		T temp = p[k];
		for(;; ) {
			unsigned s = (k << 1);
			if(s > size)
				break;
			if(s < size && p[(size_t)s + 1].Compare(p[s]) > 0)
				s++;
			if(temp.Compare(p[s]) >= 0)
				break;
			p[k] = p[s];
			k = s;
		}
		p[k] = temp;
	}
	void Sort2()
	{
		uint size = _size;
		if(size > 1) {
			T * p = (&Front()) - 1;
			{
				unsigned i = size >> 1;
				do {
					SortRefDown2(p, i, size);
				} while(--i != 0);
			}
			do {
				T temp = p[size];
				p[size--] = p[1];
				p[1] = temp;
				SortRefDown2(p, 1, size);
			} while(size > 1);
		}
	}
private:
	void MoveItems(unsigned destIndex, unsigned srcIndex)
	{
		memmove(_items + destIndex, _items + srcIndex, (size_t)(_size - srcIndex) * sizeof(T));
	}
	void ReserveOnePosition()
	{
		if(_size == _capacity) {
			unsigned newCapacity = _capacity + (_capacity >> 2) + 1;
			T * p;
			MY_ARRAY_NEW(p, T, newCapacity);
			// p = new T[newCapacity];
			if(_size != 0)
				memcpy(p, _items, (size_t)_size * sizeof(T));
			delete []_items;
			_items = p;
			_capacity = newCapacity;
		}
	}
	int FindInSorted(const T item, unsigned left, unsigned right) const
	{
		while(left != right) {
			unsigned mid = (left + right) / 2;
			const T midVal = (*this)[mid];
			if(item == midVal)
				return mid;
			if(item < midVal)
				right = mid;
			else
				left = mid + 1;
		}
		return -1;
	}
	int FindInSorted2(const T &item, unsigned left, unsigned right) const
	{
		while(left != right) {
			unsigned mid = (left + right) / 2;
			const T& midVal = (*this)[mid];
			int comp = item.Compare(midVal);
			if(comp == 0)
				return mid;
			if(comp < 0)
				right = mid;
			else
				left = mid + 1;
		}
		return -1;
	}
	T * _items;
	uint   _size;
	uint   _capacity;
};

typedef CRecordVector <int> CIntVector;
typedef CRecordVector <uint> CUIntVector;
typedef CRecordVector <bool> CBoolVector;
typedef CRecordVector <uchar> CByteVector;
typedef CRecordVector <void *> CPointerVector;

template <class T> class CObjectVector {
	CPointerVector _v;
public:
	unsigned Size() const { return _v.Size(); }
	bool IsEmpty() const { return _v.IsEmpty(); }
	void ReserveDown() { _v.ReserveDown(); }
	// void Reserve(unsigned newCapacity) { _v.Reserve(newCapacity); }
	void ClearAndReserve(unsigned newCapacity) 
	{
		Clear(); 
		_v.ClearAndReserve(newCapacity); 
	}
	CObjectVector() 
	{
	}
	CObjectVector(const CObjectVector &v)
	{
		uint size = v.Size();
		_v.ConstructReserve(size);
		for(uint i = 0; i < size; i++)
			_v.AddInReserved(new T(v[i]));
	}
	CObjectVector& operator=(const CObjectVector &v)
	{
		if(&v == this)
			return *this;
		Clear();
		uint size = v.Size();
		_v.Reserve(size);
		for(uint i = 0; i < size; i++)
			_v.AddInReserved(new T(v[i]));
		return *this;
	}
	CObjectVector& operator+=(const CObjectVector &v)
	{
		uint size = v.Size();
		_v.Reserve(Size() + size);
		for(uint i = 0; i < size; i++)
			_v.AddInReserved(new T(v[i]));
		return *this;
	}
	const T & operator[] (uint index) const { return *((T*)_v[index]); }
	T& operator[](uint index) { return *((T*)_v[index]); }
	const T& Front() const { return operator[](0); }
	T& Front() { return operator[](0); }
	const T& Back() const { return *(T*)_v.Back(); }
	T& Back() { return *(T*)_v.Back(); }
	void MoveToFront(uint index) { _v.MoveToFront(index); }
	unsigned Add(const T& item) { return _v.Add(new T(item)); }
	void AddInReserved(const T& item) { _v.AddInReserved(new T(item)); }
	T & AddNew()
	{
		T * p = new T;
		_v.Add(p);
		return *p;
	}
	T & AddNewInReserved()
	{
		T * p = new T;
		_v.AddInReserved(p);
		return *p;
	}
	void Insert(uint index, const T& item) { _v.Insert(index, new T(item)); }
	T& InsertNew(uint index)
	{
		T * p = new T;
		_v.Insert(index, p);
		return *p;
	}
	~CObjectVector()
	{
		for(uint i = _v.Size(); i != 0; )
			delete (T*)_v[--i];
	}
	void ClearAndFree()
	{
		Clear();
		_v.ClearAndFree();
	}
	void Clear()
	{
		for(uint i = _v.Size(); i != 0; )
			delete (T*)_v[--i];
		_v.Clear();
	}
	void DeleteFrom(uint index)
	{
		uint size = _v.Size();
		for(uint i = index; i < size; i++)
			delete (T*)_v[i];
		_v.DeleteFrom(index);
	}
	void DeleteFrontal(unsigned num)
	{
		for(uint i = 0; i < num; i++)
			delete (T*)_v[i];
		_v.DeleteFrontal(num);
	}
	void DeleteBack()
	{
		delete (T*)_v.Back();
		_v.DeleteBack();
	}
	void Delete(uint index)
	{
		delete (T*)_v[index];
		_v.Delete(index);
	}
	/*
	   void Delete(uint index, unsigned num)
	   {
	   for(uint i = 0; i < num; i++)
	    delete (T *)_v[index + i];
	   _v.Delete(index, num);
	   }
	 */
	/*
	   int Find(const T& item) const
	   {
	   uint size = Size();
	   for(uint i = 0; i < size; i++)
	    if(item == (*this)[i])
	      return i;
	   return -1;
	   }
	 */
	int FindInSorted(const T& item) const
	{
		unsigned left = 0, right = Size();
		while(left != right) {
			unsigned mid = (left + right) / 2;
			const T& midVal = (*this)[mid];
			int comp = item.Compare(midVal);
			if(comp == 0)
				return mid;
			if(comp < 0)
				right = mid;
			else
				left = mid + 1;
		}
		return -1;
	}
	unsigned AddToUniqueSorted(const T& item)
	{
		unsigned left = 0, right = Size();
		while(left != right) {
			unsigned mid = (left + right) / 2;
			const T& midVal = (*this)[mid];
			int comp = item.Compare(midVal);
			if(comp == 0)
				return mid;
			if(comp < 0)
				right = mid;
			else
				left = mid + 1;
		}
		Insert(right, item);
		return right;
	}
	/*
	   unsigned AddToSorted(const T& item)
	   {
	   unsigned left = 0, right = Size();
	   while (left != right)
	   {
	    unsigned mid = (left + right) / 2;
	    const T& midVal = (*this)[mid];
	    int comp = item.Compare(midVal);
	    if(comp == 0)
	    {
	      right = mid + 1;
	      break;
	    }
	    if(comp < 0)
	      right = mid;
	    else
	      left = mid + 1;
	   }
	   Insert(right, item);
	   return right;
	   }
	 */
	void Sort(int (* compare)(void * const *, void * const *, void *), void * param)
	{
		_v.Sort(compare, param);
	}
	static int CompareObjectItems(void * const * a1, void * const * a2, void * /* param */)
	{
		return (*(*((const T**)a1))).Compare(*(*((const T**)a2)));
	}
	void Sort() 
	{
		_v.Sort(CompareObjectItems, 0);
	}
};

#define FOR_VECTOR(_i_, _v_) for(uint _i_ = 0; _i_ < (_v_).Size(); _i_++)
//
//#include <MyString.h>
#ifdef _MSC_VER
  #ifdef _NATIVE_WCHAR_T_DEFINED
    #define MY_NATIVE_WCHAR_T_DEFINED
  #endif
#else
    #define MY_NATIVE_WCHAR_T_DEFINED
#endif
/*
   native support for wchar_t:
   _MSC_VER == 1600 : /Zc:wchar_t is not supported
   _MSC_VER == 1310 (VS2003)
   ? _MSC_VER == 1400 (VS2005) : wchar_t <- unsigned short
              /Zc:wchar_t  : wchar_t <- __wchar_t, _WCHAR_T_DEFINED and _NATIVE_WCHAR_T_DEFINED
   _MSC_VER > 1400 (VS2008+)
              /Zc:wchar_t[-]
              /Zc:wchar_t is on by default
 */
#ifdef _WIN32
	#define IS_PATH_SEPAR(c) ((c) == '\\' || (c) == '/')
#else
	#define IS_PATH_SEPAR(c) ((c) == CHAR_PATH_SEPARATOR)
#endif

inline bool IsPathSepar(char c) { return IS_PATH_SEPAR(c); }
inline bool IsPathSepar(wchar_t c) { return IS_PATH_SEPAR(c); }

/*inline unsigned MyStringLen_Removed(const char * s)
{
	uint i;
	for(i = 0; s[i] != 0; i++) 
		;
	return i;
}*/

//inline void MyStringCopy_Removed(char * dest, const char * src) { while((*dest++ = *src++) != 0) ; }

inline char * MyStpCpy(char * dest, const char * src)
{
	for(;; ) {
		char c = *src;
		*dest = c;
		if(c == 0)
			return dest;
		src++;
		dest++;
	}
}

/*inline unsigned MyStringLen_Removed(const wchar_t * s)
{
	uint i;
	for(i = 0; s[i] != 0; i++) 
		;
	return i;
}*/

//inline void MyStringCopy_Removed(wchar_t * dest, const wchar_t * src) { while((*dest++ = *src++) != 0) ; }

inline void MyStringCat(wchar_t * dest, const wchar_t * src)
{
	sstrcpy(dest + sstrlen(dest), src);
}
/*
   inline wchar_t *MyWcpCpy(wchar_t *dest, const wchar_t *src)
   {
   for(;;)
   {
    wchar_t c = *src;
   *dest = c;
    if(c == 0)
      return dest;
    src++;
    dest++;
   }
   }
 */
int FASTCALL FindCharPosInString(const char * s, char c);
int FASTCALL FindCharPosInString(const wchar_t * s, wchar_t c);

#ifdef _WIN32
	#ifndef _UNICODE
		#define STRING_UNICODE_THROW
	#endif
#endif
#ifndef STRING_UNICODE_THROW
	#define STRING_UNICODE_THROW throw()
#endif
/*
   inline wchar_t MyCharUpper_Ascii(wchar_t c)
   {
   if(c >= 'a' && c <= 'z')
    return (wchar_t)(c - 0x20);
   return c;
   }
 */
inline char MyCharUpper_Ascii(char c) { return (c >= 'a' && c <= 'z') ? (char)((uchar)c - 0x20) : c; }
inline char MyCharLower_Ascii(char c) { return (c >= 'A' && c <= 'Z') ? (char)((uchar)c + 0x20) : c; }
inline wchar_t MyCharLower_Ascii(wchar_t c) { return (c >= 'A' && c <= 'Z') ? (wchar_t)(c + 0x20) : c; }

wchar_t MyCharUpper_WIN(wchar_t c) throw();

inline wchar_t MyCharUpper(wchar_t c) throw()
{
	if(c < 'a') return c;
	if(c <= 'z') return (wchar_t)(c - 0x20);
	if(c <= 0x7F) return c;
  #ifdef _WIN32
    #ifdef _UNICODE
	return (wchar_t)(uint)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)(uint)c);
    #else
	return (wchar_t)MyCharUpper_WIN(c);
    #endif
  #else
	return (wchar_t)towupper(c);
  #endif
}
/*
   wchar_t MyCharLower_WIN(wchar_t c) throw();

   inline wchar_t MyCharLower(wchar_t c) throw()
   {
   if(c < 'A') return c;
   if(c <= 'Z') return (wchar_t)(c + 0x20);
   if(c <= 0x7F) return c;
   #ifdef _WIN32
   #ifdef _UNICODE
      return (wchar_t)(uint)(UINT_PTR)CharLowerW((LPWSTR)(UINT_PTR)(uint)c);
   #else
      return (wchar_t)MyCharLower_WIN(c);
   #endif
   #else
    return (wchar_t)tolower(c);
   #endif
   }
 */

// char *MyStringUpper(char *s) throw();
// char *MyStringLower(char *s) throw();

// void MyStringUpper_Ascii(char *s) throw();
// void MyStringUpper_Ascii(wchar_t *s) throw();
void MyStringLower_Ascii(char * s) throw();
void MyStringLower_Ascii(wchar_t * s) throw();
// wchar_t *MyStringUpper(wchar_t *s) STRING_UNICODE_THROW;
// wchar_t *MyStringLower(wchar_t *s) STRING_UNICODE_THROW;

bool StringsAreEqualNoCase(const wchar_t * s1, const wchar_t * s2) throw();

bool IsString1PrefixedByString2(const char * s1, const char * s2) throw();
bool IsString1PrefixedByString2(const wchar_t * s1, const wchar_t * s2) throw();
bool IsString1PrefixedByString2(const wchar_t * s1, const char * s2) throw();
bool IsString1PrefixedByString2_NoCase_Ascii(const wchar_t * u, const char * a) throw();
bool IsString1PrefixedByString2_NoCase(const wchar_t * s1, const wchar_t * s2) throw();

#define MyStringCompare(s1, s2) wcscmp(s1, s2)
int MyStringCompareNoCase(const wchar_t * s1, const wchar_t * s2) throw();
// int MyStringCompareNoCase_N(const wchar_t *s1, const wchar_t *s2, unsigned num) throw();

// ---------- ASCII ----------
// char values in ASCII strings must be less then 128
bool StringsAreEqual_Ascii(const wchar_t * u, const char * a) throw();
// @sobolev (replaced with sstreqi_ascii) bool StringsAreEqualNoCase_Ascii__(const char * s1, const char * s2) throw();
// @sobolev (replaced with sstreqi_ascii) bool StringsAreEqualNoCase_Ascii__(const wchar_t * s1, const char * s2) throw();
// @sobolev (replaced with sstreqi_ascii) bool StringsAreEqualNoCase_Ascii__(const wchar_t * s1, const wchar_t * s2) throw();

#define MY_STRING_DELETE(_p_) delete []_p_;
// #define MY_STRING_DELETE(_p_) my_delete(_p_);

#define FORBID_STRING_OPS_2(cls, t) \
	void Find(t) const; \
	void Find(t, unsigned startIndex) const; \
	void ReverseFind(t) const; \
	void InsertAtFront(t); \
	void RemoveChar(t); \
	void Replace(t, t); \

#define FORBID_STRING_OPS(cls, t) \
	explicit cls(t); \
	explicit cls(const t *); \
	cls &operator=(t); \
	cls &operator=(const t *); \
	cls &operator+=(t); \
	cls &operator+=(const t *); \
	FORBID_STRING_OPS_2(cls, t); \

/*
   cls &operator+(t); \
   cls &operator+(const t *); \
 */

#define FORBID_STRING_OPS_AString(t) FORBID_STRING_OPS(AString, t)
#define FORBID_STRING_OPS_UString(t) FORBID_STRING_OPS(UString, t)
#define FORBID_STRING_OPS_UString2(t) FORBID_STRING_OPS(UString2, t)

class AString {
	char * _chars;
	unsigned _len;
	unsigned _limit;
	void MoveItems(unsigned dest, unsigned src)
	{
		memmove(_chars + dest, _chars + src, (size_t)(_len - src + 1) * sizeof(char));
	}
	void InsertSpace(unsigned &index, uint size);

	void ReAlloc(unsigned newLimit);
	void ReAlloc2(unsigned newLimit);
	void SetStartLen(unsigned len);
	void Grow_1();
	void Grow(unsigned n);

	AString(unsigned num, const char * s);
	AString(unsigned num, const AString &s);
	AString(const AString &s, char c); // it's for String + char
	AString(const char * s1, unsigned num1, const char * s2, unsigned num2);

	friend AString operator+(const AString &s, char c) { return AString(s, c); };
	// friend AString operator+(char c, const AString &s); // is not supported

	friend AString operator + (const AString &s1, const AString &s2);
	friend AString operator + (const AString &s1, const char    * s2);
	friend AString operator + (const char    * s1, const AString &s2);

	// ---------- forbidden functions ----------

  #ifdef MY_NATIVE_WCHAR_T_DEFINED
	FORBID_STRING_OPS_AString(wchar_t)
  #endif
	FORBID_STRING_OPS_AString(signed char)
	FORBID_STRING_OPS_AString(uchar)
	FORBID_STRING_OPS_AString(short)
	FORBID_STRING_OPS_AString(unsigned short)
	FORBID_STRING_OPS_AString(int)
	FORBID_STRING_OPS_AString(uint)
	FORBID_STRING_OPS_AString(long)
	FORBID_STRING_OPS_AString(ulong)
public:
	explicit AString();
	explicit AString(char c);
	explicit AString(const char * s);
	AString(const AString &s);
	~AString() 
	{
		MY_STRING_DELETE(_chars);
	}
	unsigned Len() const { return _len; }
	bool IsEmpty() const { return _len == 0; }
	void Empty() { _len = 0; _chars[0] = 0; }
	operator const char *() const { return _chars; }
	const char * Ptr() const { return _chars; }
	const char * Ptr(unsigned pos) const { return _chars + pos; }
	const char * RightPtr(unsigned num) const { return _chars + _len - num; }
	char Back() const { return _chars[(size_t)_len - 1]; }
	void ReplaceOneCharAtPos(unsigned pos, char c) { _chars[pos] = c; }
	// GetBuf(minLen): provides the buffer that can store
	// at least (minLen) characters and additional null terminator.
	// 9.35: GetBuf doesn't preserve old characters and terminator 
	char * FASTCALL GetBuf(unsigned minLen);
	char * FASTCALL GetBuf_SetEnd(unsigned minLen);
	void FASTCALL ReleaseBuf_SetLen(unsigned newLen);
	void FASTCALL ReleaseBuf_SetEnd(unsigned newLen);
	void FASTCALL ReleaseBuf_CalcLen(unsigned maxLen);
	AString & operator = (char c);
	AString & operator = (const char * s);
	AString & operator = (const AString &s);
	void FASTCALL SetFromWStr_if_Ascii(const wchar_t * s);
	// void SetFromBstr_if_Ascii(BSTR s);
	AString & FASTCALL operator += (char c);
	void Add_Space();
	void Add_Space_if_NotEmpty();
	void Add_OptSpaced(const char * s);
	void Add_LF();
	void Add_PathSepar() { operator+=(CHAR_PATH_SEPARATOR); }
	AString & FASTCALL operator += (const char * s);
	AString & FASTCALL operator += (const AString &s);
	void Add_UInt32(uint32 v);
	void SetFrom(const char * s, unsigned len); // no check
	void SetFrom_CalcLen(const char * s, unsigned len);
	AString Mid(unsigned startIndex, unsigned count) const { return AString(count, _chars + startIndex); }
	AString Left(unsigned count) const { return AString(count, *this); }
	// void MakeUpper() { MyStringUpper(_chars); }
	// void MakeLower() { MyStringLower(_chars); }
	void MakeLower_Ascii() { MyStringLower_Ascii(_chars); }
	bool IsEqualTo(const char * s) const { return strcmp(_chars, s) == 0; }
	bool IsEqualTo_Ascii_NoCase(const char * s) const { return sstreqi_ascii(_chars, s); }
	// int Compare(const char *s) const { return MyStringCompare(_chars, s); }
	// int Compare(const AString &s) const { return MyStringCompare(_chars, s._chars); }
	// int CompareNoCase(const char *s) const { return MyStringCompareNoCase(_chars, s); }
	// int CompareNoCase(const AString &s) const { return MyStringCompareNoCase(_chars, s._chars); }
	bool IsPrefixedBy(const char * s) const { return IsString1PrefixedByString2(_chars, s); }
	bool IsPrefixedBy_Ascii_NoCase(const char * s) const throw();
	bool IsAscii() const;
	int FASTCALL Find(char c) const;
	int Find(char c, unsigned startIndex) const;
	int ReverseFind(char c) const throw();
	int ReverseFind_Dot() const throw() { return ReverseFind('.'); }
	int ReverseFind_PathSepar() const throw();
	int Find(const char * s) const { return Find(s, 0); }
	int Find(const char * s, unsigned startIndex) const throw();
	void TrimLeft() throw();
	void TrimRight() throw();
	void Trim()
	{
		TrimRight();
		TrimLeft();
	}
	void InsertAtFront(char c);
	// void Insert(uint index, char c);
	void Insert(uint index, const char * s);
	void Insert(uint index, const AString &s);
	void RemoveChar(char ch) throw();
	void Replace(char oldChar, char newChar) throw();
	void Replace(const AString &oldString, const AString &newString);
	void Delete(uint index) throw();
	void Delete(uint index, unsigned count) throw();
	void DeleteFrontal(unsigned num) throw();
	void DeleteBack();
	void FASTCALL DeleteFrom(uint index);
};

//bool operator < (const AString & s1, const AString & s2);
//bool operator > (const AString & s1, const AString & s2);
/*
   bool operator == (const AString &s1, const AString &s2);
   bool operator == (const AString &s1, const char    *s2);
   bool operator == (const char    *s1, const AString &s2);

   bool operator!=(const AString &s1, const AString &s2);
   bool operator!=(const AString &s1, const char    *s2);
   bool operator!=(const char    *s1, const AString &s2);
 */
inline bool operator == (const AString &s1, const AString &s2) { return s1.Len() == s2.Len() && strcmp(s1, s2) == 0; }
inline bool operator == (const AString &s1, const char    * s2) { return strcmp(s1, s2) == 0; }
inline bool operator == (const char    * s1, const AString &s2) { return strcmp(s1, s2) == 0; }
inline bool operator!=(const AString &s1, const AString &s2) { return s1.Len() != s2.Len() || strcmp(s1, s2) != 0; }
inline bool operator!=(const AString &s1, const char    * s2) { return strcmp(s1, s2) != 0; }
inline bool operator!=(const char    * s1, const AString &s2) { return strcmp(s1, s2) != 0; }

// ---------- forbidden functions ----------

void operator == (char c1, const AString &s2);
void operator == (const AString &s1, char c2);
void operator+(char c, const AString &s); // this function can be OK, but we don't use it
void operator+(const AString &s, int c);
void operator+(const AString &s, unsigned c);
void operator+(int c, const AString &s);
void operator+(unsigned c, const AString &s);
void operator-(const AString &s, int c);
void operator-(const AString &s, unsigned c);

class UString {
	wchar_t * _chars;
	uint   _len;
	uint   _limit;
	void   MoveItems(unsigned dest, unsigned src);
	void InsertSpace(uint index, uint size);
	void ReAlloc(unsigned newLimit);
	void ReAlloc2(unsigned newLimit);
	void SetStartLen(unsigned len);
	void Grow_1();
	void Grow(unsigned n);
	UString(unsigned num, const wchar_t * s); // for Mid
	UString(unsigned num, const UString &s); // for Left
	UString(const UString &s, wchar_t c); // it's for String + char
	UString(const wchar_t * s1, unsigned num1, const wchar_t * s2, unsigned num2);
	friend UString operator + (const UString &s, wchar_t c) { return UString(s, c); }
	// friend UString operator+(wchar_t c, const UString &s); // is not supported
	friend UString operator+(const UString &s1, const UString &s2);
	friend UString operator+(const UString &s1, const wchar_t * s2);
	friend UString operator+(const wchar_t * s1, const UString &s2);

	// ---------- forbidden functions ----------

	FORBID_STRING_OPS_UString(signed char)
	FORBID_STRING_OPS_UString(uchar)
	FORBID_STRING_OPS_UString(short)

  #ifdef MY_NATIVE_WCHAR_T_DEFINED
	FORBID_STRING_OPS_UString(unsigned short)
  #endif

	FORBID_STRING_OPS_UString(int)
	FORBID_STRING_OPS_UString(uint)
	FORBID_STRING_OPS_UString(long)
	FORBID_STRING_OPS_UString(ulong)
	FORBID_STRING_OPS_2(UString, char)
public:
	UString();
	explicit UString(wchar_t c);
	explicit UString(char c);
	explicit UString(const char * s);
	// UString(const AString &s);
	UString(const wchar_t * s);
	UString(const UString &s);
	~UString();
	unsigned Len() const { return _len; }
	bool IsEmpty() const { return _len == 0; }
	void Empty() 
	{ 
		_len = 0; 
		_chars[0] = 0; 
	}
	operator const wchar_t *() const { return _chars; }
	const wchar_t * Ptr() const { return _chars; }
	const wchar_t * Ptr(unsigned pos) const { return _chars + pos; }
	const wchar_t * FASTCALL RightPtr(unsigned num) const { return _chars + _len - num; }
	wchar_t Back() const { return _chars[(size_t)_len - 1]; }
	void   ReplaceOneCharAtPos(unsigned pos, wchar_t c) { _chars[pos] = c; }
	wchar_t * FASTCALL GetBuf(unsigned minLen);
	wchar_t * FASTCALL GetBuf_SetEnd(unsigned minLen);
	void FASTCALL ReleaseBuf_SetLen(unsigned newLen) { _len = newLen; }
	void FASTCALL ReleaseBuf_SetEnd(unsigned newLen) 
	{
		_len = newLen; 
		_chars[newLen] = 0;
	}
	void FASTCALL ReleaseBuf_CalcLen(unsigned maxLen)
	{
		wchar_t * chars = _chars;
		chars[maxLen] = 0;
		_len = sstrlen(chars);
	}
	UString & operator = (wchar_t c);
	UString & operator = (char c) { return (*this) = ((wchar_t)c); }
	UString & operator = (const wchar_t * s);
	UString & operator = (const UString &s);
	void SetFrom(const wchar_t * s, unsigned len); // no check
	void SetFromBstr(BSTR s);
	UString & operator = (const char * s);
	UString & FASTCALL operator = (const AString &s) { return operator=(s.Ptr()); }
	UString & FASTCALL operator += (wchar_t c);
	UString & FASTCALL operator += (char c) { return (*this) += ((wchar_t)(uchar)c); }
	void Add_Space();
	void Add_Space_if_NotEmpty();
	void Add_LF();
	void Add_PathSepar() { operator+=(WCHAR_PATH_SEPARATOR); }
	UString & FASTCALL operator += (const wchar_t * s);
	UString & operator += (const UString &s);
	UString & operator += (const char * s);
	UString & FASTCALL operator += (const AString &s) { return operator+=(s.Ptr()); }
	void Add_UInt32(uint32 v);
	UString Mid(unsigned startIndex, unsigned count) const { return UString(count, _chars + startIndex); }
	UString FASTCALL Left(unsigned count) const { return UString(count, *this); }
	// void MakeUpper() { MyStringUpper(_chars); }
	// void MakeUpper() { MyStringUpper_Ascii(_chars); }
	// void MakeUpper_Ascii() { MyStringUpper_Ascii(_chars); }
	void MakeLower_Ascii() { MyStringLower_Ascii(_chars); }
	bool IsEqualTo(const char * s) const { return StringsAreEqual_Ascii(_chars, s); }
	bool IsEqualTo_NoCase(const wchar_t * s) const { return StringsAreEqualNoCase(_chars, s); }
	bool IsEqualTo_Ascii_NoCase(const char * s) const { return sstreqi_ascii(_chars, s); }
	int Compare(const wchar_t * s) const { return wcscmp(_chars, s); }
	// int Compare(const UString &s) const { return MyStringCompare(_chars, s._chars); }
	// int CompareNoCase(const wchar_t *s) const { return MyStringCompareNoCase(_chars, s); }
	// int CompareNoCase(const UString &s) const { return MyStringCompareNoCase(_chars, s._chars); }
	bool IsPrefixedBy(const wchar_t * s) const { return IsString1PrefixedByString2(_chars, s); }
	bool IsPrefixedBy_NoCase(const wchar_t * s) const { return IsString1PrefixedByString2_NoCase(_chars, s); }
	bool IsPrefixedBy_Ascii_NoCase(const char * s) const throw();
	bool IsAscii() const;
	int FASTCALL Find(wchar_t c) const { return FindCharPosInString(_chars, c); }
	int Find(wchar_t c, unsigned startIndex) const
	{
		int pos = FindCharPosInString(_chars + startIndex, c);
		return pos < 0 ? -1 : (int)startIndex + pos;
	}
	int ReverseFind(wchar_t c) const throw();
	int ReverseFind_Dot() const throw() { return ReverseFind(L'.'); }
	int ReverseFind_PathSepar() const throw();
	int Find(const wchar_t * s) const { return Find(s, 0); }
	int Find(const wchar_t * s, unsigned startIndex) const throw();
	void TrimLeft() throw();
	void TrimRight() throw();
	void Trim()
	{
		TrimRight();
		TrimLeft();
	}
	void InsertAtFront(wchar_t c);
	// void Insert(uint index, wchar_t c);
	void Insert(uint index, const wchar_t * s);
	void Insert(uint index, const UString &s);
	void RemoveChar(wchar_t ch) throw();
	void Replace(wchar_t oldChar, wchar_t newChar) throw();
	void Replace(const UString &oldString, const UString &newString);
	void Delete(uint index) throw();
	void Delete(uint index, unsigned count) throw();
	void DeleteFrontal(unsigned num) throw();
	void DeleteBack() { _chars[--_len] = 0; }
	void DeleteFrom(uint index)
	{
		if(index < _len) {
			_len = index;
			_chars[index] = 0;
		}
	}
};

bool operator < (const UString &s1, const UString &s2);
bool operator > (const UString &s1, const UString &s2);

inline bool operator == (const UString &s1, const UString &s2) { return s1.Len() == s2.Len() && wcscmp(s1, s2) == 0; }
inline bool operator == (const UString &s1, const wchar_t * s2) { return wcscmp(s1, s2) == 0; }
inline bool operator == (const wchar_t * s1, const UString &s2) { return wcscmp(s1, s2) == 0; }
inline bool operator!=(const UString &s1, const UString &s2) { return s1.Len() != s2.Len() || wcscmp(s1, s2) != 0; }
inline bool operator!=(const UString &s1, const wchar_t * s2) { return wcscmp(s1, s2) != 0; }
inline bool operator!=(const wchar_t * s1, const UString &s2) { return wcscmp(s1, s2) != 0; }

// ---------- forbidden functions ----------

void operator == (wchar_t c1, const UString &s2);
void operator == (const UString &s1, wchar_t c2);
void operator+(wchar_t c, const UString &s); // this function can be OK, but we don't use it
void operator+(const AString &s1, const UString &s2);
void operator+(const UString &s1, const AString &s2);
void operator+(const UString &s1, const char * s2);
void operator+(const char * s1, const UString &s2);
void operator+(const UString &s, char c);
void operator+(const UString &s, uchar c);
void operator+(char c, const UString &s);
void operator+(uchar c, const UString &s);
void operator-(const UString &s1, wchar_t c);

#ifdef _WIN32
	// can we forbid these functions, if wchar_t is 32-bit ?
	void operator+(const UString &s, int c);
	void operator+(const UString &s, unsigned c);
	void operator+(int c, const UString &s);
	void operator+(unsigned c, const UString &s);
	void operator-(const UString &s1, int c);
	void operator-(const UString &s1, unsigned c);
#endif

class UString2 {
	wchar_t * _chars;
	unsigned _len;
	void ReAlloc2(unsigned newLimit);
	void SetStartLen(unsigned len);

	// ---------- forbidden functions ----------

	FORBID_STRING_OPS_UString2(char)
	FORBID_STRING_OPS_UString2(signed char)
	FORBID_STRING_OPS_UString2(uchar)
	FORBID_STRING_OPS_UString2(short)

	UString2 &operator=(wchar_t c);
	UString2(wchar_t c);
public:
	UString2() : _chars(NULL), _len(0) 
	{
	}
	UString2(const wchar_t * s);
	UString2(const UString2 &s);
	~UString2() 
	{
		if(_chars) 
			MY_STRING_DELETE(_chars);
	}
	unsigned Len() const { return _len; }
	bool IsEmpty() const { return _len == 0; }
	// void Empty() { _len = 0; _chars[0] = 0; }
	// operator const wchar_t *() const { return _chars; }
	const wchar_t * GetRawPtr() const { return _chars; }
	int Compare(const wchar_t * s) const { return wcscmp(_chars, s); }
	wchar_t * GetBuf(unsigned minLen)
	{
		if(!_chars || minLen > _len)
			ReAlloc2(minLen);
		return _chars;
	}
	void ReleaseBuf_SetLen(unsigned newLen) { _len = newLen; }

	UString2 &operator=(const wchar_t * s);
	UString2 &operator=(const UString2 &s);
	void SetFromAscii(const char * s);
};

bool operator == (const UString2 &s1, const UString2 &s2);
bool operator == (const UString2 &s1, const wchar_t * s2);
bool operator == (const wchar_t * s1, const UString2 &s2);

inline bool operator!=(const UString2 &s1, const UString2 &s2) { return !(s1 == s2); }
inline bool operator!=(const UString2 &s1, const wchar_t * s2) { return !(s1 == s2); }
inline bool operator!=(const wchar_t * s1, const UString2 &s2) { return !(s1 == s2); }

// ---------- forbidden functions ----------

void operator == (wchar_t c1, const UString2 &s2);
void operator == (const UString2 &s1, wchar_t c2);
bool operator<(const UString2 &s1, const UString2 &s2);
bool operator>(const UString2 &s1, const UString2 &s2);

void operator+(const UString2 &s1, const UString2 &s2);
void operator+(const UString2 &s1, const wchar_t * s2);
void operator+(const wchar_t * s1, const UString2 &s2);
void operator+(wchar_t c, const UString2 &s);
void operator+(const UString2 &s, wchar_t c);
void operator+(const UString2 &s, char c);
void operator+(const UString2 &s, uchar c);
void operator+(char c, const UString2 &s);
void operator+(uchar c, const UString2 &s);
void operator-(const UString2 &s1, wchar_t c);

typedef CObjectVector<AString> AStringVector;
typedef CObjectVector<UString> UStringVector;
#ifdef _UNICODE
	typedef UString CSysString;
#else
	typedef AString CSysString;
#endif
typedef CObjectVector<CSysString> CSysStringVector;

// ---------- FString ----------

#ifdef _WIN32
	#define USE_UNICODE_FSTRING
#endif
#ifdef USE_UNICODE_FSTRING
	#define __FTEXT(quote) L ## quote

	typedef wchar_t FChar;
	typedef UString FString;

	#define fs2us(_x_) (_x_)
	 #define us2fs(_x_) (_x_)
	FString fas2fs(const char * s);
	FString fas2fs(const AString &s);
	AString fs2fas(const FChar * s);
#else
	#define __FTEXT(quote) quote

	typedef char FChar;
	typedef AString FString;

	UString fs2us(const FChar * s);
	UString fs2us(const FString &s);
	FString us2fs(const wchar_t * s);
	#define fas2fs(_x_) (_x_)
	#define fs2fas(_x_) (_x_)
#endif

#define FTEXT(quote) __FTEXT(quote)
#define FCHAR_PATH_SEPARATOR FTEXT(CHAR_PATH_SEPARATOR)
#define FSTRING_PATH_SEPARATOR FTEXT(STRING_PATH_SEPARATOR)
// #define FCHAR_ANY_MASK FTEXT('*')
// #define FSTRING_ANY_MASK FTEXT("*")

typedef const FChar * CFSTR;
typedef CObjectVector<FString> FStringVector;
//
//#include <MyBuffer.h>
//
// 7-Zip now uses CBuffer only as CByteBuffer.
// So there is no need to use MY_ARRAY_NEW macro in CBuffer code. */
//
template <class T> class CBuffer {
private:
	T * _items;
	size_t _size;
public:
	void Free()
	{
		ZDELETEARRAY(_items);
		_size = 0;
	}
	CBuffer() : _items(0), _size(0) 
	{
	}
	CBuffer(size_t size) : _items(0), _size(0) 
	{
		_items = new T[size]; 
		_size = size;
	}
	CBuffer(const CBuffer &buffer) : _items(0), _size(0)
	{
		size_t size = buffer._size;
		if(size != 0) {
			_items = new T[size];
			memcpy(_items, buffer._items, size * sizeof(T));
			_size = size;
		}
	}
	~CBuffer() 
	{
		delete []_items;
	}
	operator T * () { return _items; }
	operator const T *() const { return _items; }
	size_t Size() const { return _size; }
	void Alloc(size_t size)
	{
		if(size != _size) {
			Free();
			if(size != 0) {
				_items = new T[size];
				_size = size;
			}
		}
	}
	void AllocAtLeast(size_t size)
	{
		if(size > _size) {
			Free();
			_items = new T[size];
			_size = size;
		}
	}
	void CopyFrom(const T * data, size_t size)
	{
		Alloc(size);
		if(size != 0)
			memcpy(_items, data, size * sizeof(T));
	}
	void ChangeSize_KeepData(size_t newSize, size_t keepSize)
	{
		if(newSize != _size) {
			T * newBuffer = NULL;
			if(newSize != 0) {
				newBuffer = new T[newSize];
				if(keepSize > _size)
					keepSize = _size;
				if(keepSize != 0)
					memcpy(newBuffer, _items, MyMin(keepSize, newSize) * sizeof(T));
			}
			delete []_items;
			_items = newBuffer;
			_size = newSize;
		}
	}
	CBuffer & operator=(const CBuffer &buffer)
	{
		if(&buffer != this)
			CopyFrom(buffer, buffer._size);
		return *this;
	}
};

template <class T> bool operator == (const CBuffer<T>& b1, const CBuffer<T>& b2)
{
	size_t size1 = b1.Size();
	if(size1 != b2.Size())
		return false;
	else if(size1 == 0)
		return true;
	else
		return memcmp(b1, b2, size1 * sizeof(T)) == 0;
}

template <class T> bool operator != (const CBuffer<T>& b1, const CBuffer<T>& b2)
{
	size_t size1 = b1.Size();
	if(size1 != b2.Size())
		return true;
	else if(size1 == 0)
		return false;
	else
		return memcmp(b1, b2, size1 * sizeof(T)) != 0;
}

// typedef CBuffer<char> CCharBuffer;
// typedef CBuffer<wchar_t> CWCharBuffer;
typedef CBuffer <uchar> CByteBuffer;

template <class T> class CObjArray {
protected:
	T * _items;
private:
	// we disable copy
	CObjArray(const CObjArray &buffer);
	void operator = (const CObjArray &buffer);
public:
	void Free()
	{
		ZDELETEARRAY(_items);
	}
	CObjArray(size_t size) : _items(0)
	{
		if(size != 0) {
			MY_ARRAY_NEW(_items, T, size)
			// _items = new T[size];
		}
	}
	CObjArray() : _items(0) 
	{
	}
	~CObjArray() 
	{
		delete []_items;
	}
	operator T *() { return _items; }
	operator const T *() const { return _items; }
	void Alloc(size_t newSize)
	{
		ZDELETEARRAY(_items);
		MY_ARRAY_NEW(_items, T, newSize)
		// _items = new T[newSize];
	}
};

typedef CObjArray <uchar> CByteArr;
typedef CObjArray <bool> CBoolArr;
typedef CObjArray <int> CIntArr;
//typedef CObjArray <unsigned> CUIntArr;

template <class T> class CObjArray2 {
	T * _items;
	unsigned _size;
	// we disable copy
	CObjArray2(const CObjArray2 &buffer);
	void operator=(const CObjArray2 &buffer);
public:
	void Free()
	{
		ZDELETEARRAY(_items);
		_size = 0;
	}
	CObjArray2() : _items(0), _size(0) 
	{
	}
	/*
	   CObjArray2(const CObjArray2 &buffer): _items(0), _size(0)
	   {
	   size_t newSize = buffer._size;
	   if(newSize != 0)
	   {
	    T *newBuffer = new T[newSize];;
	    _items = newBuffer;
	    _size = newSize;
	    const T *src = buffer;
	    for(size_t i = 0; i < newSize; i++)
	      newBuffer[i] = src[i];
	   }
	   }
	 */
	/*
	   CObjArray2(size_t size): _items(0), _size(0)
	   {
	   if(size != 0) {
	    _items = new T[size];
	    _size = size;
	   }
	   }
	 */
	~CObjArray2() 
	{
		delete []_items;
	}
	operator T * () { return _items; }
	operator const T *() const { return _items; }
	unsigned Size() const { return (uint)_size; }
	bool IsEmpty() const { return _size == 0; }
	// SetSize doesn't keep old items. It allocates new array if size is not equal
	void SetSize(uint size)
	{
		if(size != _size) {
			T * newBuffer = NULL;
			if(size != 0) {
				MY_ARRAY_NEW(newBuffer, T, size)
				// newBuffer = new T[size];
			}
			delete []_items;
			_items = newBuffer;
			_size = size;
		}
	}
	/*
	   CObjArray2& operator=(const CObjArray2 &buffer)
	   {
	   Free();
	   size_t newSize = buffer._size;
	   if(newSize != 0)
	   {
	    T *newBuffer = new T[newSize];;
	    _items = newBuffer;
	    _size = newSize;
	    const T *src = buffer;
	    for(size_t i = 0; i < newSize; i++)
	      newBuffer[i] = src[i];
	   }
	   return *this;
	   }
	 */
};
//
//#include <MyBuffer2.h>
class CMidBuffer {
public:
	CMidBuffer() : _data(NULL), _size(0) 
	{
	}
	~CMidBuffer() 
	{
		::MidFree(_data);
	}
	void Free() 
	{
		::MidFree(_data); 
		_data = NULL; 
		_size = 0; 
	}
	bool IsAllocated() const { return _data != NULL; }
	operator Byte *() { return _data; }
	operator const Byte *() const { return _data; }
	size_t Size() const { return _size; }
	void AllocAtLeast(size_t size)
	{
		if(!_data || size > _size) {
			const size_t kMinSize = (size_t)1 << 16;
			SETMAX(size, kMinSize);
			::MidFree(_data);
			_size = 0;
			_data = 0;
			_data = static_cast<Byte *>(::MidAlloc(size));
			if(_data)
				_size = size;
		}
	}
private:
	Byte * _data;
	size_t _size;
	CLASS_NO_COPY(CMidBuffer)
};
//
//#include <DynamicBuffer.h>
template <class T> class CDynamicBuffer {
	T * _items;
	size_t _size;
	size_t _pos;

	CDynamicBuffer(const CDynamicBuffer &buffer);
	void operator=(const CDynamicBuffer &buffer);
	void Grow(size_t size)
	{
		size_t delta = _size >= 64 ? _size : 64;
		if(delta < size)
			delta = size;
		size_t newCap = _size + delta;
		if(newCap < delta) {
			newCap = _size + size;
			if(newCap < size)
				throw 20120116;
		}
		T * newBuffer = new T[newCap];
		if(_pos != 0)
			memcpy(newBuffer, _items, _pos * sizeof(T));
		delete []_items;
		_items = newBuffer;
		_size = newCap;
	}
public:
	CDynamicBuffer() : _items(0), _size(0), _pos(0) 
	{
	}
	// operator T *() { return _items; }
	operator const T *() const { return _items; }
	~CDynamicBuffer() 
	{
		delete []_items;
	}
	T * GetCurPtrAndGrow(size_t addSize)
	{
		size_t rem = _size - _pos;
		if(rem < addSize)
			Grow(addSize - rem);
		T * res = _items + _pos;
		_pos += addSize;
		return res;
	}
	void AddData(const T * data, size_t size)
	{
		memcpy(GetCurPtrAndGrow(size), data, size * sizeof(T));
	}
	const size_t GetPos() const { return _pos; }
	// void Empty() { _pos = 0; }
};

typedef CDynamicBuffer <uchar> CByteDynamicBuffer;
//
//#include <Property.h>
struct CProperty {
	UString Name;
	UString Value;
};
//
//#include <PropID.h>
enum {
	kpidNoProperty = 0,
	kpidMainSubfile,
	kpidHandlerItemIndex,
	kpidPath,
	kpidName,
	kpidExtension,
	kpidIsDir,
	kpidSize,
	kpidPackSize,
	kpidAttrib,
	kpidCTime,
	kpidATime,
	kpidMTime,
	kpidSolid,
	kpidCommented,
	kpidEncrypted,
	kpidSplitBefore,
	kpidSplitAfter,
	kpidDictionarySize,
	kpidCRC,
	kpidType,
	kpidIsAnti,
	kpidMethod,
	kpidHostOS,
	kpidFileSystem,
	kpidUser,
	kpidGroup,
	kpidBlock,
	kpidComment,
	kpidPosition,
	kpidPrefix,
	kpidNumSubDirs,
	kpidNumSubFiles,
	kpidUnpackVer,
	kpidVolume,
	kpidIsVolume,
	kpidOffset,
	kpidLinks,
	kpidNumBlocks,
	kpidNumVolumes,
	kpidTimeType,
	kpidBit64,
	kpidBigEndian,
	kpidCpu,
	kpidPhySize,
	kpidHeadersSize,
	kpidChecksum,
	kpidCharacts,
	kpidVa,
	kpidId,
	kpidShortName,
	kpidCreatorApp,
	kpidSectorSize,
	kpidPosixAttrib,
	kpidSymLink,
	kpidError,
	kpidTotalSize,
	kpidFreeSpace,
	kpidClusterSize,
	kpidVolumeName,
	kpidLocalName,
	kpidProvider,
	kpidNtSecure,
	kpidIsAltStream,
	kpidIsAux,
	kpidIsDeleted,
	kpidIsTree,
	kpidSha1,
	kpidSha256,
	kpidErrorType,
	kpidNumErrors,
	kpidErrorFlags,
	kpidWarningFlags,
	kpidWarning,
	kpidNumStreams,
	kpidNumAltStreams,
	kpidAltStreamsSize,
	kpidVirtualSize,
	kpidUnpackSize,
	kpidTotalPhySize,
	kpidVolumeIndex,
	kpidSubType,
	kpidShortComment,
	kpidCodePage,
	kpidIsNotArcType,
	kpidPhySizeCantBeDetected,
	kpidZerosTailIsAllowed,
	kpidTailSize,
	kpidEmbeddedStubSize,
	kpidNtReparse,
	kpidHardLink,
	kpidINode,
	kpidStreamId,
	kpidReadOnly,
	kpidOutName,
	kpidCopyLink,

	kpid_NUM_DEFINED,

	kpidUserDefined = 0x10000
};

extern const Byte k7z_PROPID_To_VARTYPE[kpid_NUM_DEFINED]; // VARTYPE

const uint32 kpv_ErrorFlags_IsNotArc      = 1 << 0;
const uint32 kpv_ErrorFlags_HeadersError  = 1 << 1;
const uint32 kpv_ErrorFlags_EncryptedHeadersError = 1 << 2;
const uint32 kpv_ErrorFlags_UnavailableStart      = 1 << 3;
const uint32 kpv_ErrorFlags_UnconfirmedStart      = 1 << 4;
const uint32 kpv_ErrorFlags_UnexpectedEnd = 1 << 5;
const uint32 kpv_ErrorFlags_DataAfterEnd  = 1 << 6;
const uint32 kpv_ErrorFlags_UnsupportedMethod     = 1 << 7;
const uint32 kpv_ErrorFlags_UnsupportedFeature    = 1 << 8;
const uint32 kpv_ErrorFlags_DataError     = 1 << 9;
const uint32 kpv_ErrorFlags_CrcError      = 1 << 10;
// const uint32 kpv_ErrorFlags_Unsupported   = 1 << 11;
//
//#include <PropIDUtils.h>
// provide at least 64 bytes for buffer including zero-end
void ConvertPropertyToShortString2(char *dest, const PROPVARIANT &propVariant, PROPID propID, int level = 0) throw();
void ConvertPropertyToString2(UString &dest, const PROPVARIANT &propVariant, PROPID propID, int level = 0);
bool ConvertNtReparseToString(const Byte *data, uint32 size, UString &s);
void ConvertNtSecureToString(const Byte *data, uint32 size, AString &s);
bool CheckNtSecure(const Byte *data, uint32 size) throw();;
void ConvertWinAttribToString(char *s, uint32 wa) throw();
//
//#include <SortUtils.h>
void SortFileNames(const UStringVector &strings, CUIntVector &indices);
//
//#include <Delta.h>
EXTERN_C_BEGIN
	#define DELTA_STATE_SIZE 256

	void Delta_Init(Byte *state);
	void Delta_Encode(Byte *state, unsigned delta, Byte *data, SizeT size);
	void Delta_Decode(Byte *state, unsigned delta, Byte *data, SizeT size);
EXTERN_C_END
//
//#include <Bra.h>
EXTERN_C_BEGIN
	/*
	These functions convert relative addresses to absolute addresses
	in CALL instructions to increase the compression ratio.
	  
	In:
		data     - data buffer
		size     - size of data
		ip       - current virtual Instruction Pinter (IP) value
		state    - state variable for x86 converter
		encoding - 0 (for decoding), 1 (for encoding)
	  
	Out:
		state    - state variable for x86 converter

	Returns:
		The number of processed bytes. If you call these functions with multiple calls,
		you must start next call with first byte after block of processed bytes.
	  
	Type   Endian  Alignment  LookAhead
	  
	x86    little      1          4
	ARMT   little      2          2
	ARM    little      4          0
	PPC     big        4          0
	SPARC   big        4          0
	IA64   little     16          0

	size must be >= Alignment + LookAhead, if it's not last block.
	If (size < Alignment + LookAhead), converter returns 0.

	Example:

		uint32 ip = 0;
		for()
		{
		; size must be >= Alignment + LookAhead, if it's not last block
		SizeT processed = Convert(data, size, ip, 1);
		data += processed;
		size -= processed;
		ip += processed;
		}
	*/

	#define x86_Convert_Init(state) { state = 0; }
	SizeT x86_Convert(Byte *data, SizeT size, uint32 ip, uint32 *state, int encoding);
	SizeT ARM_Convert(Byte *data, SizeT size, uint32 ip, int encoding);
	SizeT ARMT_Convert(Byte *data, SizeT size, uint32 ip, int encoding);
	SizeT PPC_Convert(Byte *data, SizeT size, uint32 ip, int encoding);
	SizeT SPARC_Convert(Byte *data, SizeT size, uint32 ip, int encoding);
	SizeT IA64_Convert(Byte *data, SizeT size, uint32 ip, int encoding);
EXTERN_C_END
//
//#include <DefaultName.h>
UString GetDefaultName2(const UString &fileName, const UString &extension, const UString &addSubExtension);
//
//#include <IntToString.h>
void FASTCALL ConvertUInt32ToString(uint32 value, char *s) throw();
void FASTCALL ConvertUInt64ToString(uint64 value, char *s) throw();
void FASTCALL ConvertUInt32ToString(uint32 value, wchar_t *s) throw();
void FASTCALL ConvertUInt64ToString(uint64 value, wchar_t *s) throw();
void ConvertUInt64ToOct(uint64 value, char *s) throw();
void ConvertUInt32ToHex(uint32 value, char *s) throw();
void ConvertUInt64ToHex(uint64 value, char *s) throw();
void ConvertUInt32ToHex8Digits(uint32 value, char *s) throw();
// void ConvertUInt32ToHex8Digits(uint32 value, wchar_t *s) throw();
void ConvertInt64ToString(int64 value, char *s) throw();
void ConvertInt64ToString(int64 value, wchar_t *s) throw();
// use RawLeGuid only for RAW bytes that contain stored GUID as Little-endian.
char *RawLeGuidToString(const Byte *guid, char *s) throw();
char *RawLeGuidToString_Braced(const Byte *guid, char *s) throw();
//
//#include <StringToInt.h>
uint32 ConvertStringToUInt32(const char *s, const char **end) throw();
uint64 ConvertStringToUInt64(const char *s, const char **end) throw();
uint32 ConvertStringToUInt32(const wchar_t *s, const wchar_t **end) throw();
uint64 ConvertStringToUInt64(const wchar_t *s, const wchar_t **end) throw();
int32 ConvertStringToInt32(const wchar_t *s, const wchar_t **end) throw();
uint32 ConvertOctStringToUInt32(const char *s, const char **end) throw();
uint64 ConvertOctStringToUInt64(const char *s, const char **end) throw();
uint32 ConvertHexStringToUInt32(const char *s, const char **end) throw();
uint64 ConvertHexStringToUInt64(const char *s, const char **end) throw();
//
//#include <StringConvert.h>
UString FASTCALL MultiByteToUnicodeString(const AString &src, UINT codePage = CP_ACP);
UString FASTCALL MultiByteToUnicodeString(const char *src, UINT codePage = CP_ACP);

// optimized versions that work faster for ASCII strings
void FASTCALL MultiByteToUnicodeString2(UString &dest, const AString &src, UINT codePage = CP_ACP);
// void UnicodeStringToMultiByte2(AString &dest, const UString &s, UINT codePage, char defaultChar, bool &defaultCharWasUsed);
void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT codePage);

AString FASTCALL UnicodeStringToMultiByte(const UString &src, UINT codePage, char defaultChar, bool &defaultCharWasUsed);
AString FASTCALL UnicodeStringToMultiByte(const UString &src, UINT codePage = CP_ACP);

inline const wchar_t * GetUnicodeString(const wchar_t *u) { return u; }
inline const UString& GetUnicodeString(const UString &u) { return u; }

inline UString GetUnicodeString(const AString &a) { return MultiByteToUnicodeString(a); }
inline UString GetUnicodeString(const char *a)     { return MultiByteToUnicodeString(a); }
inline UString GetUnicodeString(const AString &a, UINT codePage) { return MultiByteToUnicodeString(a, codePage); }
inline UString GetUnicodeString(const char *a, UINT codePage) { return MultiByteToUnicodeString(a, codePage); }
inline const wchar_t * GetUnicodeString(const wchar_t *u, UINT) { return u; }
inline const UString& GetUnicodeString(const UString &u, UINT) { return u; }
inline const char*    GetAnsiString(const char    *a) { return a; }
inline const AString& GetAnsiString(const AString &a) { return a; }
inline AString GetAnsiString(const wchar_t *u) { return UnicodeStringToMultiByte(UString(u)); }
inline AString GetAnsiString(const UString &u) { return UnicodeStringToMultiByte(u); }
//inline const char* GetOemString(const char* oem) { return oem; }
//inline const AString& GetOemString(const AString &oem) { return oem; }
const char * GetOemString(const char* oem);
const AString & GetOemString(const AString &oem);
inline AString GetOemString(const UString &u) { return UnicodeStringToMultiByte(u, CP_OEMCP); }
#ifdef _UNICODE
  inline const wchar_t * GetSystemString(const wchar_t *u) { return u;}
  inline const UString& GetSystemString(const UString &u) { return u;}
  inline const wchar_t * GetSystemString(const wchar_t *u, UINT /* codePage */) { return u;}
  inline const UString& GetSystemString(const UString &u, UINT /* codePage */) { return u;}
 
  inline UString GetSystemString(const AString &a, UINT codePage) { return MultiByteToUnicodeString(a, codePage); }
  inline UString GetSystemString(const char    *a, UINT codePage) { return MultiByteToUnicodeString(a, codePage); }
  inline UString GetSystemString(const AString &a) { return MultiByteToUnicodeString(a); }
  inline UString GetSystemString(const char    *a) { return MultiByteToUnicodeString(a); }
#else
  inline const char*    GetSystemString(const char    *a) { return a; }
  inline const AString& GetSystemString(const AString &a) { return a; }
  inline const char*    GetSystemString(const char    *a, UINT) { return a; }
  inline const AString& GetSystemString(const AString &a, UINT) { return a; }
 
  inline AString GetSystemString(const wchar_t *u) { return UnicodeStringToMultiByte(UString(u)); }
  inline AString GetSystemString(const UString &u) { return UnicodeStringToMultiByte(u); }
  inline AString GetSystemString(const UString &u, UINT codePage) { return UnicodeStringToMultiByte(u, codePage); }
  /*
  inline AString GetSystemString(const wchar_t *u)
  {
    UString s;
    s = u;
    return UnicodeStringToMultiByte(s);
  }
  */
#endif
#ifndef UNDER_CE
	AString SystemStringToOemString(const CSysString &src);
#endif
//
//#include <UTFConvert.h>
bool FASTCALL CheckUTF8(const char *src, bool allowReduced = false) throw();
bool FASTCALL ConvertUTF8ToUnicode(const AString &utfString, UString &resultString);
void FASTCALL ConvertUnicodeToUTF8(const UString &unicodeString, AString &resultString);
//
//#include <Wildcard.h>
int CompareFileNames(const wchar_t * s1, const wchar_t * s2) STRING_UNICODE_THROW;
#ifndef USE_UNICODE_FSTRING
	int CompareFileNames(const char * s1, const char * s2);
#endif
bool IsPath1PrefixedByPath2(const wchar_t * s1, const wchar_t * s2);
void FASTCALL SplitPathToParts(const UString &path, UStringVector &pathParts);
void SplitPathToParts_2(const UString &path, UString &dirPrefix, UString &name);
void SplitPathToParts_Smart(const UString &path, UString &dirPrefix, UString &name); // ignores dir delimiter at the end of (path)
UString ExtractDirPrefixFromPath(const UString &path);
UString ExtractFileNameFromPath(const UString &path);
bool DoesNameContainWildcard(const UString &path);
bool DoesWildcardMatchName(const UString &mask, const UString &name);

namespace NWildcard {
	#ifdef _WIN32
		// returns true, if name is like "a:", "c:", ...
		bool   IsDriveColonName(const wchar_t * s);
		uint   GetNumPrefixParts_if_DrivePath(const UStringVector & pathParts);
	#endif

	class CCensorNode {
	public:
		struct CItem {
		#ifdef _WIN32
			bool   IsDriveItem() const { return PathParts.Size() == 1 && !ForFile && ForDir && IsDriveColonName(PathParts[0]); }
		#endif
			// CItem(): WildcardMatching(true) {}
			bool   AreAllAllowed() const;
			bool   CheckPath(const UStringVector &pathParts, bool isFile) const;

			UStringVector PathParts;
			bool   Recursive;
			bool   ForFile;
			bool   ForDir;
			bool   WildcardMatching;
		};
		CCensorNode() : Parent(0) 
		{
		}
		CCensorNode(const UString &name, CCensorNode * parent) : Name(name), Parent(parent) 
		{
		}
		bool   CheckPathVect(const UStringVector &pathParts, bool isFile, bool &include) const;
		bool   AreAllAllowed() const;
		int    FindSubNode(const UString &path) const;
		void   AddItem(bool include, CItem &item, int ignoreWildcardIndex = -1);
		void   AddItem(bool include, const UString &path, bool recursive, bool forFile, bool forDir, bool wildcardMatching);
		void   AddItem2(bool include, const UString &path, bool recursive, bool wildcardMatching);
		bool   NeedCheckSubDirs() const;
		bool   AreThereIncludeItems() const;
		// bool   CheckPath2(bool isAltStream, const UString &path, bool isFile, bool &include) const;
		// bool   CheckPath(bool isAltStream, const UString &path, bool isFile) const;
		bool   CheckPathToRoot(bool include, UStringVector &pathParts, bool isFile) const;
		// bool   CheckPathToRoot(const UString &path, bool isFile, bool include) const;
		void   ExtendExclude(const CCensorNode &fromNodes);

		UString Name; // WIN32 doesn't support wildcards in file names
		CObjectVector <CCensorNode> SubNodes;
		CObjectVector <CItem> IncludeItems;
		CObjectVector <CItem> ExcludeItems;
	private:
		bool CheckPathCurrent(bool include, const UStringVector &pathParts, bool isFile) const;
		void AddItemSimple(bool include, const CItem &item);

		CCensorNode * Parent;
	};

	enum ECensorPathMode {
		k_RelatPath, // absolute prefix as Prefix, remain path in Tree
		k_FullPath, // drive prefix as Prefix, remain path in Tree
		k_AbsPath // full path in Tree
	};

	class CCensor {
	public:
		bool AllAreRelative() const { return (Pairs.Size() == 1 && Pairs.Front().Prefix.IsEmpty()); }
		void AddItem(ECensorPathMode pathMode, bool include, const UString &path, bool recursive, bool wildcardMatching);
		// bool CheckPath(bool isAltStream, const UString &path, bool isFile) const;
		void ExtendExclude();
		void AddPathsToCensor(NWildcard::ECensorPathMode censorPathMode);
		void AddPreItem(bool include, const UString &path, bool recursive, bool wildcardMatching);
		void AddPreItem(const UString &path) { AddPreItem(true, path, false, false); }
		void AddPreItem_Wildcard() { AddPreItem(true, UString("*"), false, true); }

		struct CPair {
			CPair(const UString & prefix) : Prefix(prefix) 
			{
			}
			UString Prefix;
			CCensorNode Head;
		};
		struct CCensorPath {
			CCensorPath() : Include(true), Recursive(false), WildcardMatching(true)
			{
			}
			UString Path;
			bool   Include;
			bool   Recursive;
			bool   WildcardMatching;
			uint8  Reserve[1]; // @alignment
		};
		CObjectVector <CPair> Pairs;
		CObjectVector <CCensorPath> CensorPaths;
	private:
		int FindPrefix(const UString &prefix) const;
	};
}
//
//#include <UniqBlocks.h>
struct CUniqBlocks {
	uint   AddUniq(const Byte * data, size_t size);
	uint64 GetTotalSizeInBytes() const;
	void   GetReverseMap();
	bool   IsOnlyEmpty() const;

	CObjectVector<CByteBuffer> Bufs;
	CUIntVector Sorted;
	CUIntVector BufIndexToSortedIndex;
};
//
//#include <StdOutStream.h>
class CStdOutStream {
public:
	CStdOutStream() : _stream(0), _streamIsOpen(false) 
	{
	}
	CStdOutStream(FILE * stream) : _stream(stream), _streamIsOpen(false) 
	{
	}
	~CStdOutStream() 
	{
		Close();
	}
	// void AttachStdStream(FILE *stream) { _stream  = stream; _streamIsOpen = false; }
	// bool IsDefined() const { return _stream  != NULL; }

	operator FILE *() { return _stream; }
	bool Open(const char * fileName) throw();
	bool Close() throw();
	bool Flush() throw();
	CStdOutStream & operator<<(CStdOutStream & (*func)(CStdOutStream  &))
	{
		(*func)(*this);
		return *this;
	}
	CStdOutStream & operator<<(const char * s) throw()
	{
		fputs(s, _stream);
		return *this;
	}
	CStdOutStream & operator<<(char c) throw()
	{
		fputc((uchar)c, _stream);
		return *this;
	}
	CStdOutStream & operator<<(int32 number) throw();
	CStdOutStream & operator<<(int64 number) throw();
	CStdOutStream & operator<<(uint32 number) throw();
	CStdOutStream & operator<<(uint64 number) throw();
	CStdOutStream & operator<<(const wchar_t * s);
	void PrintUString(const UString &s, AString &temp);
private:
	FILE * _stream;
	bool _streamIsOpen;
};

extern CStdOutStream g_StdOut;
extern CStdOutStream g_StdErr;

CStdOutStream & endl(CStdOutStream & outStream) throw();
void StdOut_Convert_UString_to_AString(const UString &s, AString &temp);
//
//#include <StdInStream.h>
class CStdInStream {
public:
	CStdInStream() : _stream(0), _streamIsOpen(false) 
	{
	}
	CStdInStream(FILE * stream) : _stream(stream), _streamIsOpen(false) 
	{
	}
	~CStdInStream() 
	{
		Close();
	}
	bool Open(LPCTSTR fileName) throw();
	bool Close() throw();
	// returns:
	//   false, if ZERO character in stream
	//   true, if EOF or '\n'
	bool ScanAStringUntilNewLine(AString &s);
	bool ScanUStringUntilNewLine(UString &s);
	// bool ReadToString(AString &resultString);
	bool Eof() const throw() { return (feof(_stream) != 0); }
	bool Error() const throw() { return (ferror(_stream) != 0); }
	int GetChar();
private:
	FILE * _stream;
	bool _streamIsOpen;
};

extern CStdInStream g_StdIn;
//
//#include <MethodId.h>
typedef uint64 CMethodId;
//
//#include <Threads.h>
EXTERN_C_BEGIN
	WRes FASTCALL HandlePtr_Close(HANDLE *h);
	WRes FASTCALL Handle_WaitObject(HANDLE h);

	typedef HANDLE CThread;
	#define Thread_Construct(p) *(p) = NULL
	#define Thread_WasCreated(p) (*(p) != NULL)
	#define Thread_Close(p) HandlePtr_Close(p)
	#define Thread_Wait(p) Handle_WaitObject(*(p))

	typedef
	#ifdef UNDER_CE
		DWORD
	#else
		unsigned
	#endif
	THREAD_FUNC_RET_TYPE;

	#define THREAD_FUNC_CALL_TYPE _7Z_STD_CALL
	#define THREAD_FUNC_DECL THREAD_FUNC_RET_TYPE THREAD_FUNC_CALL_TYPE
	typedef THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE * THREAD_FUNC_TYPE)(void *);
	WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param);

	typedef HANDLE CEvent;
	typedef CEvent CAutoResetEvent;
	typedef CEvent CManualResetEvent;
	#define Event_Construct(p) *(p) = NULL
	#define Event_IsCreated(p) (*(p) != NULL)
	#define Event_Close(p) HandlePtr_Close(p)
	#define Event_Wait(p) Handle_WaitObject(*(p))
	WRes FASTCALL Event_Set(CEvent *p);
	WRes FASTCALL Event_Reset(CEvent *p);
	WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled);
	WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p);
	WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled);
	WRes FASTCALL AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p);

	typedef HANDLE CSemaphore;
	#define Semaphore_Construct(p) (*p) = NULL
	#define Semaphore_Close(p) HandlePtr_Close(p)
	#define Semaphore_Wait(p) Handle_WaitObject(*(p))
	WRes Semaphore_Create(CSemaphore *p, uint32 initCount, uint32 maxCount);
	WRes Semaphore_ReleaseN(CSemaphore *p, uint32 num);
	WRes Semaphore_Release1(CSemaphore *p);

	typedef CRITICAL_SECTION CCriticalSection;
	WRes CriticalSection_Init(CCriticalSection *p);
	#define CriticalSection_Delete(p) DeleteCriticalSection(p)
	#define CriticalSection_Enter(p) EnterCriticalSection(p)
	#define CriticalSection_Leave(p) LeaveCriticalSection(p)
EXTERN_C_END
//
//#include <Windows/Defs.h>
#ifdef _WIN32
	inline bool LRESULTToBool(LRESULT v) { return (v != FALSE); }
	inline bool BOOLToBool(BOOL v) { return (v != FALSE); }
	inline BOOL BoolToBOOL(bool v) { return (v ? TRUE: FALSE); }
#endif
inline VARIANT_BOOL BoolToVARIANT_BOOL(bool v) { return (v ? VARIANT_TRUE: VARIANT_FALSE); }
inline bool VARIANT_BOOLToBool(VARIANT_BOOL v) { return (v != VARIANT_FALSE); }
//
//#include <Windows/DLL.h> <Windows/System.h> <Windows/ErrorMsg.h> <Windows/Handle.h> <Windows/Synchronization.h> <Windows/MemoryLock.h> <Windows/Thread.h> <Windows/TimeUtils.h> <Windows/PropVariant.h>
// <Windows/ResourceString.h> <Windows/FileIO.h> <Windows/FileDir.h> <Windows/FileFind.h> <Windows/FileName.h>

#if defined(_WIN32) && !defined(UNDER_CE)
	#include <winioctl.h>
#endif
#define _my_IO_REPARSE_TAG_MOUNT_POINT  (0xA0000003L)
#define _my_IO_REPARSE_TAG_SYMLINK      (0xA000000CL)
#define _my_SYMLINK_FLAG_RELATIVE 1
#define my_FSCTL_SET_REPARSE_POINT  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER
#define my_FSCTL_GET_REPARSE_POINT  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS) // REPARSE_DATA_BUFFER

namespace NWindows {
	namespace NDLL {
		#ifdef UNDER_CE
			#define My_GetProcAddress(module, procName) ::GetProcAddressA(module, procName)
		#else
			#define My_GetProcAddress(module, procName) ::GetProcAddress(module, procName)
		#endif

		/* Win32: Don't call CLibrary::Free() and FreeLibrary() from another
			FreeLibrary() code: detaching code in DLL entry-point or in
			destructors of global objects in DLL module. */

		class CLibrary {
			HMODULE _module;
			// CLASS_NO_COPY(CLibrary);
		public:
			CLibrary() : _module(NULL) 
			{
			}
			~CLibrary() 
			{
				Free();
			}
			operator HMODULE() const { return _module; }
			HMODULE* operator&() { return &_module; }
			bool IsLoaded() const { return (_module != NULL); }
			void Attach(HMODULE m)
			{
				Free();
				_module = m;
			}
			HMODULE Detach()
			{
				HMODULE m = _module;
				_module = NULL;
				return m;
			}
			bool Free() throw();
			bool LoadEx(CFSTR path, DWORD flags = LOAD_LIBRARY_AS_DATAFILE) throw();
			bool Load(CFSTR path) throw();
			FARPROC GetProc(LPCSTR procName) const { return My_GetProcAddress(_module, procName); }
		};

		bool MyGetModuleFileName(FString &path);
		FString GetModuleDirPrefix();
	}
	namespace NSystem {
		uint32 CountAffinity(DWORD_PTR mask);

		struct CProcessAffinity {
			void InitST()
			{
				// numProcessThreads = 1;
				// numSysThreads = 1;
				processAffinityMask = 1;
				systemAffinityMask = 1;
			}
			uint32 GetNumProcessThreads() const { return CountAffinity(processAffinityMask); }
			uint32 GetNumSystemThreads() const { return CountAffinity(systemAffinityMask); }
			BOOL Get();

			// uint32 numProcessThreads;
			// uint32 numSysThreads;
			DWORD_PTR processAffinityMask;
			DWORD_PTR systemAffinityMask;
		};

		uint32 GetNumberOfProcessors();
		bool GetRamSize(uint64 &size); // returns false, if unknown ram size
	}
	namespace NError {
		UString MyFormatMessage(DWORD errorCode);
	}

	class CHandle {
	protected:
		HANDLE _handle;
	public:
		operator HANDLE() { return _handle; }
		CHandle() : _handle(NULL) 
		{
		}
		~CHandle() 
		{
			Close();
		}
		bool IsCreated() const { return (_handle != NULL); }
		bool Close()
		{
			if(!_handle)
				return true;
			if(!::CloseHandle(_handle))
				return false;
			_handle = NULL;
			return true;
		}
		void Attach(HANDLE handle) { _handle = handle; }
		HANDLE Detach()
		{
			HANDLE handle = _handle;
			_handle = NULL;
			return handle;
		}
	};
	
	namespace NSynchronization {
		class CBaseEvent {
		protected:
			::CEvent _object;
		public:
			bool IsCreated() { return Event_IsCreated(&_object) != 0; }
			operator HANDLE() { return _object; }
			CBaseEvent() { Event_Construct(&_object); }
			~CBaseEvent() { Close(); }
			WRes Close() { return Event_Close(&_object); }
		#ifdef _WIN32
			WRes Create(bool manualReset, bool initiallyOwn, LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES sa = NULL)
			{
				_object = ::CreateEvent(sa, BoolToBOOL(manualReset), BoolToBOOL(initiallyOwn), name);
				if(!name && _object != 0)
					return 0;
				return ::GetLastError();
			}
			WRes Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
			{
				_object = ::OpenEvent(desiredAccess, BoolToBOOL(inheritHandle), name);
				if(_object != 0)
					return 0;
				return ::GetLastError();
			}
		#endif
			WRes Set() { return Event_Set(&_object); }
			// bool Pulse() { return BOOLToBool(::PulseEvent(_handle)); }
			WRes Reset() { return Event_Reset(&_object); }
			WRes Lock() { return Event_Wait(&_object); }
		};

		class CManualResetEvent : public CBaseEvent {
		public:
			WRes Create(bool initiallyOwn = false)
			{
				return ManualResetEvent_Create(&_object, initiallyOwn ? 1 : 0);
			}
			WRes CreateIfNotCreated()
			{
				if(IsCreated())
					return 0;
				return ManualResetEvent_CreateNotSignaled(&_object);
			}
		#ifdef _WIN32
			WRes CreateWithName(bool initiallyOwn, LPCTSTR name)
			{
				return CBaseEvent::Create(true, initiallyOwn, name);
			}
		#endif
		};

		class CAutoResetEvent : public CBaseEvent {
		public:
			WRes Create()
			{
				return AutoResetEvent_CreateNotSignaled(&_object);
			}
			WRes CreateIfNotCreated()
			{
				if(IsCreated())
					return 0;
				return AutoResetEvent_CreateNotSignaled(&_object);
			}
		};

		#ifdef _WIN32
		class CObject : public CHandle {
		public:
			WRes Lock(DWORD timeoutInterval = INFINITE)
			{
				return (::WaitForSingleObject(_handle, timeoutInterval) == WAIT_OBJECT_0 ? 0 : ::GetLastError());
			}
		};
		class CMutex : public CObject {
		public:
			WRes Create(bool initiallyOwn, LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES sa = NULL)
			{
				_handle = ::CreateMutex(sa, BoolToBOOL(initiallyOwn), name);
				if(!name && _handle != 0)
					return 0;
				return ::GetLastError();
			}
		#ifndef UNDER_CE
			WRes Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
			{
				_handle = ::OpenMutex(desiredAccess, BoolToBOOL(inheritHandle), name);
				if(_handle != 0)
					return 0;
				return ::GetLastError();
			}
		#endif
			WRes Release()
			{
				return ::ReleaseMutex(_handle) ? 0 : ::GetLastError();
			}
		};

		class CMutexLock {
			CMutex * _object;
		public:
			CMutexLock(CMutex &object) : _object(&object) 
			{
				_object->Lock();
			}
			~CMutexLock() 
			{
				_object->Release();
			}
		};
		#endif

		class CSemaphore {
			::CSemaphore _object;
		public:
			CSemaphore() 
			{
				Semaphore_Construct(&_object);
			}
			~CSemaphore() 
			{
				Close();
			}
			WRes Close() { return Semaphore_Close(&_object); }
			operator HANDLE() { return _object; }
			WRes Create(uint32 initiallyCount, uint32 maxCount) { return Semaphore_Create(&_object, initiallyCount, maxCount); }
			WRes Release() { return Semaphore_Release1(&_object); }
			WRes Release(uint32 releaseCount) { return Semaphore_ReleaseN(&_object, releaseCount); }
			WRes Lock() { return Semaphore_Wait(&_object); }
		};

		class CCriticalSection {
			::CCriticalSection _object;
		public:
			CCriticalSection() 
			{
				CriticalSection_Init(&_object);
			}
			~CCriticalSection() 
			{
				CriticalSection_Delete(&_object);
			}
			void Enter() { CriticalSection_Enter(&_object); }
			void Leave() { CriticalSection_Leave(&_object); }
		};

		class CCriticalSectionLock {
			CCriticalSection * _object;
			void Unlock() { _object->Leave(); }
		public:
			CCriticalSectionLock(CCriticalSection &object) : _object(&object) 
			{
				_object->Enter();
			}
			~CCriticalSectionLock() 
			{
				Unlock();
			}
		};
	}
	namespace NSecurity {
#ifndef UNDER_CE
		bool EnablePrivilege(LPCTSTR privilegeName, bool enable = true);
		inline bool EnablePrivilege_LockMemory(bool enable = true)
		{
			return EnablePrivilege(SE_LOCK_MEMORY_NAME, enable);
		}
		inline void EnablePrivilege_SymLink()
		{
			/* Probably we do not to set any Privilege for junction points.
			But we need them for Symbolic links */
			NSecurity::EnablePrivilege(SE_RESTORE_NAME);
			/* Probably we need only SE_RESTORE_NAME, but there is also
			SE_CREATE_SYMBOLIC_LINK_NAME. So we set it also. Do we need it? */
			NSecurity::EnablePrivilege(TEXT("SeCreateSymbolicLinkPrivilege")); // SE_CREATE_SYMBOLIC_LINK_NAME
			// Do we need to set SE_BACKUP_NAME ?
		}
#endif
	}

	class CThread {
	public:
		CThread() { Thread_Construct(&thread); }
		~CThread() { Close(); }
		bool IsCreated() { return Thread_WasCreated(&thread) != 0; }
		WRes Close() { return Thread_Close(&thread); }
		WRes Create(THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE * startAddress)(void *), LPVOID parameter) 
			{ return Thread_Create(&thread, startAddress, parameter); }
		WRes Wait() { return Thread_Wait(&thread); }
#ifdef _WIN32
		operator HANDLE() { return thread; }
		void Attach(HANDLE handle) { thread = handle; }
		HANDLE Detach() 
		{ 
			HANDLE h = thread; 
			thread = NULL; 
			return h;
		}
		DWORD Resume() { return ::ResumeThread(thread); }
		DWORD Suspend() { return ::SuspendThread(thread); }
		bool Terminate(DWORD exitCode) { return BOOLToBool(::TerminateThread(thread, exitCode)); }
		int GetPriority() { return ::GetThreadPriority(thread); }
		bool SetPriority(int priority) { return BOOLToBool(::SetThreadPriority(thread, priority)); }
#endif
	private:
		::CThread thread;
	};
	
	namespace NTime {
		bool DosTimeToFileTime(uint32 dosTime, FILETIME &fileTime) throw();
		bool FileTimeToDosTime(const FILETIME &fileTime, uint32 &dosTime) throw();
		// uint32 Unix Time : for dates 1970-2106
		uint64 UnixTimeToFileTime64(uint32 unixTime) throw();
		void UnixTimeToFileTime(uint32 unixTime, FILETIME &fileTime) throw();
		// int64 Unix Time : negative values for dates before 1970
		uint64 UnixTime64ToFileTime64(int64 unixTime) throw();
		bool UnixTime64ToFileTime(int64 unixTime, FILETIME &fileTime) throw();
		bool FileTimeToUnixTime(const FILETIME &fileTime, uint32 &unixTime) throw();
		int64 FileTimeToUnixTime64(const FILETIME &ft) throw();
		bool GetSecondsSince1601(uint year, uint month, uint day, uint hour, uint min, uint sec, uint64 & resSeconds) throw();
		void GetCurUtcFileTime(FILETIME &ft) throw();
	}
	namespace NCOM {
		BSTR   AllocBstrFromAscii(const char * s) throw();
		HRESULT PropVariant_Clear(PROPVARIANT * p) throw();
		HRESULT PropVarEm_Alloc_Bstr(PROPVARIANT * p, unsigned numChars) throw();
		HRESULT PropVarEm_Set_Str(PROPVARIANT * p, const char * s) throw();

		inline void PropVarEm_Set_UInt32(PROPVARIANT * p, uint32 v) throw()
		{
			p->vt = VT_UI4;
			p->ulVal = v;
		}
		inline void PropVarEm_Set_UInt64(PROPVARIANT * p, uint64 v) throw()
		{
			p->vt = VT_UI8;
			p->uhVal.QuadPart = v;
		}
		inline void PropVarEm_Set_FileTime64(PROPVARIANT * p, uint64 v) throw()
		{
			p->vt = VT_FILETIME;
			p->filetime.dwLowDateTime = (DWORD)v;
			p->filetime.dwHighDateTime = (DWORD)(v >> 32);
		}
		inline void PropVarEm_Set_Bool(PROPVARIANT * p, bool b) throw()
		{
			p->vt = VT_BOOL;
			p->boolVal = (b ? VARIANT_TRUE : VARIANT_FALSE);
		}

		class CPropVariant : public tagPROPVARIANT {
		public:
			CPropVariant();
			~CPropVariant() throw();
			CPropVariant(const PROPVARIANT &varSrc);
			CPropVariant(const CPropVariant &varSrc);
			CPropVariant(BSTR bstrSrc);
			CPropVariant(LPCOLESTR lpszSrc);
			CPropVariant(bool bSrc); 
			CPropVariant(Byte value);
		private:
			CPropVariant(int16 value); // { vt = VT_I2; wReserved1 = 0; iVal = value; }
			CPropVariant(int32 value); // { vt = VT_I4; wReserved1 = 0; lVal = value; }
		public:
			CPropVariant(uint32 value);
			CPropVariant(uint64 value);
			CPropVariant(int64 value);
			CPropVariant(const FILETIME &value);
			CPropVariant & operator=(const CPropVariant &varSrc);
			CPropVariant & operator=(const PROPVARIANT &varSrc);
			CPropVariant & operator=(BSTR bstrSrc);
			CPropVariant & operator=(LPCOLESTR lpszSrc);
			CPropVariant & operator=(const UString &s);
			CPropVariant & operator=(const UString2 &s);
			CPropVariant & operator=(const char * s);
			CPropVariant & operator=(const AString &s) { return (*this) = (const char *)s; }
			CPropVariant & operator=(bool bSrc) throw();
			CPropVariant & operator=(Byte value) throw();
		private:
			CPropVariant & operator=(int16 value) throw();
		public:
			CPropVariant & operator=(int32 value) throw();
			CPropVariant & operator=(uint32 value) throw();
			CPropVariant & operator=(uint64 value) throw();
			CPropVariant & operator=(int64 value) throw();
			CPropVariant & operator=(const FILETIME &value) throw();
			BSTR AllocBstr(unsigned numChars);
			HRESULT Clear() throw();
			HRESULT Copy(const PROPVARIANT * pSrc) throw();
			HRESULT Attach(PROPVARIANT * pSrc) throw();
			HRESULT Detach(PROPVARIANT * pDest) throw();
			HRESULT InternalClear() throw();
			void FASTCALL InternalCopy(const PROPVARIANT * pSrc);
			int Compare(const CPropVariant &a) throw();
		};

		// PropVariant.cpp
		BSTR   AllocBstrFromAscii(const char * s) throw();
		void   FASTCALL ParseNumberString(const UString & s, CPropVariant & prop);
	}

	UString MyLoadString(UINT resourceID);
	void MyLoadString(HINSTANCE hInstance, UINT resourceID, UString &dest);
	void MyLoadString(UINT resourceID, UString &dest);
	
	namespace NFile {
		#if defined(_WIN32) && !defined(UNDER_CE)
			bool   FillLinkData(CByteBuffer &dest, const wchar_t * path, bool isSymLink);
		#endif

		struct CReparseShortInfo {
			bool   Parse(const Byte * p, size_t size);
			uint   Offset;
			uint   Size;
		};

		struct CReparseAttr {
			CReparseAttr() : Tag(0), Flags(0) 
			{
			}
			bool Parse(const Byte * p, size_t size);
			bool IsMountPoint() const { return Tag == _my_IO_REPARSE_TAG_MOUNT_POINT; } // it's Junction
			bool IsSymLink() const { return Tag == _my_IO_REPARSE_TAG_SYMLINK; }
			bool IsRelative() const  { return Flags == _my_SYMLINK_FLAG_RELATIVE; }
			// bool IsVolume() const;
			bool IsOkNamePair() const;
			UString GetPath() const;

			uint32 Tag;
			uint32 Flags;
			UString SubsName;
			UString PrintName;
		};

		namespace NIO {
			bool GetReparseData(CFSTR path, CByteBuffer &reparseData, BY_HANDLE_FILE_INFORMATION * fileInfo = NULL);
			bool SetReparseData(CFSTR path, bool isDir, const void * data, DWORD size);

			class CFileBase {
			protected:
				HANDLE _handle;
				bool Create(CFSTR path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
			public:
				bool DeviceIoControl(DWORD controlCode, LPVOID inBuffer, DWORD inSize,
					LPVOID outBuffer, DWORD outSize, LPDWORD bytesReturned, LPOVERLAPPED overlapped = NULL) const
				{
					return BOOLToBool(::DeviceIoControl(_handle, controlCode, inBuffer, inSize, outBuffer, outSize, bytesReturned, overlapped));
				}
				bool DeviceIoControlOut(DWORD controlCode, LPVOID outBuffer, DWORD outSize, LPDWORD bytesReturned) const
				{
					return DeviceIoControl(controlCode, NULL, 0, outBuffer, outSize, bytesReturned);
				}
				bool DeviceIoControlOut(DWORD controlCode, LPVOID outBuffer, DWORD outSize) const
				{
					DWORD bytesReturned;
					return DeviceIoControlOut(controlCode, outBuffer, outSize, &bytesReturned);
				}
			public:
			#ifdef SUPPORT_DEVICE_FILE
				bool IsDeviceFile;
				bool SizeDefined;
				uint64 Size; // it can be larger than real available size
			#endif
				CFileBase() : _handle(INVALID_HANDLE_VALUE) 
				{
				}
				~CFileBase() 
				{
					Close();
				}
				bool Close() throw();
				bool GetPosition(uint64 &position) const throw();
				bool GetLength(uint64 &length) const throw();
				bool Seek(int64 distanceToMove, DWORD moveMethod, uint64 &newPosition) const throw();
				bool Seek(uint64 position, uint64 &newPosition) const throw();
				bool SeekToBegin() const throw();
				bool SeekToEnd(uint64 &newPosition) const throw();
				bool GetFileInformation(BY_HANDLE_FILE_INFORMATION * info) const
				{
					return BOOLToBool(GetFileInformationByHandle(_handle, info));
				}
				static bool GetFileInformation(CFSTR path, BY_HANDLE_FILE_INFORMATION * info)
				{
					NIO::CFileBase file;
					if(!file.Create(path, 0, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS))
						return false;
					return file.GetFileInformation(info);
				}
			};

			#ifndef UNDER_CE
			#define IOCTL_CDROM_BASE  FILE_DEVICE_CD_ROM
			#define IOCTL_CDROM_GET_DRIVE_GEOMETRY  CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
			// #define IOCTL_CDROM_MEDIA_REMOVAL  CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)

			// IOCTL_DISK_GET_DRIVE_GEOMETRY_EX works since WinXP
			#define my_IOCTL_DISK_GET_DRIVE_GEOMETRY_EX  CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

			struct my_DISK_GEOMETRY_EX {
				DISK_GEOMETRY Geometry;
				LARGE_INTEGER DiskSize;
				BYTE Data[1];
			};

			#endif

			class CInFile : public CFileBase {
			#ifdef SUPPORT_DEVICE_FILE
			#ifndef UNDER_CE
				bool GetGeometry(DISK_GEOMETRY * res) const { return DeviceIoControlOut(IOCTL_DISK_GET_DRIVE_GEOMETRY, res, sizeof(*res)); }
				bool GetGeometryEx(my_DISK_GEOMETRY_EX * res) const { return DeviceIoControlOut(my_IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, res, sizeof(*res)); }
				bool GetCdRomGeometry(DISK_GEOMETRY * res) const { return DeviceIoControlOut(IOCTL_CDROM_GET_DRIVE_GEOMETRY, res, sizeof(*res)); }
				bool GetPartitionInfo(PARTITION_INFORMATION * res) { return DeviceIoControlOut(IOCTL_DISK_GET_PARTITION_INFO, LPVOID(res), sizeof(*res)); }
			#endif
				void CorrectDeviceSize();
				void CalcDeviceSize(CFSTR name);
			#endif
			public:
				bool Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
				bool OpenShared(CFSTR fileName, bool shareForWrite);
				bool Open(CFSTR fileName);
			#ifndef UNDER_CE
				bool OpenReparse(CFSTR fileName)
				{
					return Open(fileName, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS);
				}
			#endif
				bool Read1(void * data, uint32 size, uint32 &processedSize) throw();
				bool ReadPart(void * data, uint32 size, uint32 &processedSize) throw();
				bool Read(void * data, uint32 size, uint32 &processedSize) throw();
			};

			class COutFile : public CFileBase {
			public:
				bool Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
				bool Open(CFSTR fileName, DWORD creationDisposition);
				bool Create(CFSTR fileName, bool createAlways);
				bool CreateAlways(CFSTR fileName, DWORD flagsAndAttributes);
				bool SetTime(const FILETIME * cTime, const FILETIME * aTime, const FILETIME * mTime) throw();
				bool SetMTime(const FILETIME * mTime) throw();
				bool WritePart(const void * data, uint32 size, uint32 &processedSize) throw();
				bool Write(const void * data, uint32 size, uint32 &processedSize) throw();
				bool SetEndOfFile() throw();
				bool SetLength(uint64 length) throw();
			};
		}
		namespace NDir {
			bool GetWindowsDir(FString &path);
			bool GetSystemDir(FString &path);
			bool SetDirTime(CFSTR path, const FILETIME * cTime, const FILETIME * aTime, const FILETIME * mTime);
			bool SetFileAttrib(CFSTR path, DWORD attrib);
			bool MyMoveFile(CFSTR existFileName, CFSTR newFileName);
			#ifndef UNDER_CE
				bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName);
			#endif
			bool RemoveDir(CFSTR path);
			bool CreateDir(CFSTR path);

			/* CreateComplexDir returns true, if directory can contain files after the call (two cases):
				1) the directory already exists (network shares and drive paths are supported)
				2) the directory was created
			path can be WITH or WITHOUT trailing path separator. */

			bool CreateComplexDir(CFSTR path);
			bool DeleteFileAlways(CFSTR name);
			bool RemoveDirWithSubItems(const FString &path);
			bool MyGetFullPathName(CFSTR path, FString &resFullPath);
			bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName);
			bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix);
			#ifndef UNDER_CE
				bool SetCurrentDir(CFSTR path);
				bool GetCurrentDir(FString &resultPath);
			#endif
			bool MyGetTempPath(FString &resultPath);

			class CTempFile {
			public:
				CTempFile() : _mustBeDeleted(false) 
				{
				}
				~CTempFile() 
				{
					Remove();
				}
				const FString &GetPath() const { return _path; }
				bool Create(CFSTR pathPrefix, NIO::COutFile * outFile); // pathPrefix is not folder prefix
				bool CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile * outFile);
				bool Remove();
				bool MoveTo(CFSTR name, bool deleteDestBefore);
			private:
				void DisableDeleting() { _mustBeDeleted = false; }
				FString _path;
				bool _mustBeDeleted;
			};

			class CTempDir {
			public:
				CTempDir() : _mustBeDeleted(false) 
				{
				}
				~CTempDir() 
				{
					Remove();
				}
				const FString &GetPath() const { return _path; }
				void DisableDeleting() { _mustBeDeleted = false; }
				bool Create(CFSTR namePrefix);
				bool Remove();
			private:
				FString _path;
				bool _mustBeDeleted;
			};
			#if !defined(UNDER_CE)
				class CCurrentDirRestorer {
				public:
					CCurrentDirRestorer() : NeedRestore(true)
					{
						GetCurrentDir(_path);
					}
					~CCurrentDirRestorer()
					{
						if(NeedRestore) {
							FString s;
							if(GetCurrentDir(s))
								if(s != _path)
									SetCurrentDir(_path);
						}
					}
					bool NeedRestore;
				private:
					FString _path;
				};
			#endif
		}
		namespace NFind {
			namespace NAttributes {
				inline bool IsReadOnly(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_READONLY) != 0; }
				inline bool IsHidden(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_HIDDEN) != 0; }
				inline bool IsSystem(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_SYSTEM) != 0; }
				inline bool IsDir(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0; }
				inline bool IsArchived(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_ARCHIVE) != 0; }
				inline bool IsCompressed(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_COMPRESSED) != 0; }
				inline bool IsEncrypted(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_ENCRYPTED) != 0; }
			}
			class CFileInfoBase {
				bool MatchesMask(UINT32 mask) const { return ((Attrib & mask) != 0); }
			public:
				uint64 Size;
				FILETIME CTime;
				FILETIME ATime;
				FILETIME MTime;
				DWORD Attrib;
				bool IsAltStream;
				bool IsDevice;
				/*
				#ifdef UNDER_CE
				DWORD ObjectID;
				#else
				UINT32 ReparseTag;
				#endif
				*/
				CFileInfoBase() 
				{
					ClearBase();
				}
				void ClearBase() throw();
				void SetAsDir() { Attrib = FILE_ATTRIBUTE_DIRECTORY; }
				bool IsArchived() const { return MatchesMask(FILE_ATTRIBUTE_ARCHIVE); }
				bool IsCompressed() const { return MatchesMask(FILE_ATTRIBUTE_COMPRESSED); }
				bool IsDir() const { return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
				bool IsEncrypted() const { return MatchesMask(FILE_ATTRIBUTE_ENCRYPTED); }
				bool IsHidden() const { return MatchesMask(FILE_ATTRIBUTE_HIDDEN); }
				bool IsNormal() const { return MatchesMask(FILE_ATTRIBUTE_NORMAL); }
				bool IsOffline() const { return MatchesMask(FILE_ATTRIBUTE_OFFLINE); }
				bool IsReadOnly() const { return MatchesMask(FILE_ATTRIBUTE_READONLY); }
				bool HasReparsePoint() const { return MatchesMask(FILE_ATTRIBUTE_REPARSE_POINT); }
				bool IsSparse() const { return MatchesMask(FILE_ATTRIBUTE_SPARSE_FILE); }
				bool IsSystem() const { return MatchesMask(FILE_ATTRIBUTE_SYSTEM); }
				bool IsTemporary() const { return MatchesMask(FILE_ATTRIBUTE_TEMPORARY); }
			};

			struct CFileInfo : public CFileInfoBase {
				FString Name;
			#if defined(_WIN32) && !defined(UNDER_CE)
				// FString ShortName;
			#endif
				bool IsDots() const throw();
				bool Find(CFSTR path);
			};

			class CFindFileBase {
			public:
				CFindFileBase() : _handle(INVALID_HANDLE_VALUE) 
				{
				}
				~CFindFileBase() 
				{
					Close();
				}
				bool IsHandleAllocated() const { return _handle != INVALID_HANDLE_VALUE; }
				bool Close() throw();
			protected:
				HANDLE _handle;
			};

			class CFindFile : public CFindFileBase {
			public:
				bool FindFirst(CFSTR wildcard, CFileInfo &fileInfo);
				bool FindNext(CFileInfo &fileInfo);
			};

			#if defined(_WIN32) && !defined(UNDER_CE)
				struct CStreamInfo {
					UString Name;
					uint64 Size;
					UString GetReducedName() const; // returns ":Name"
					// UString GetReducedName2() const; // returns "Name"
					bool IsMainStream() const throw();
				};

				class CFindStream : public CFindFileBase {
				public:
					bool FindFirst(CFSTR filePath, CStreamInfo &streamInfo);
					bool FindNext(CStreamInfo &streamInfo);
				};

				class CStreamEnumerator {
					CFindStream _find;
					FString _filePath;
					bool NextAny(CFileInfo &fileInfo);
				public:
					CStreamEnumerator(const FString &filePath) : _filePath(filePath) 
					{
					}
					bool Next(CStreamInfo &streamInfo, bool &found);
				};
			#endif

			bool DoesFileExist(CFSTR name);
			bool DoesDirExist(CFSTR name);
			bool DoesFileOrDirExist(CFSTR name);
			DWORD GetFileAttrib(CFSTR path);

			class CEnumerator {
				CFindFile _findFile;
				FString _wildcard;
				bool NextAny(CFileInfo &fileInfo);
			public:
				void SetDirPrefix(const FString &dirPrefix);
				bool Next(CFileInfo &fileInfo);
				bool Next(CFileInfo &fileInfo, bool &found);
			};

			class CFindChangeNotification {
				HANDLE _handle;
			public:
				operator HANDLE() { return _handle; }
				bool IsHandleAllocated() const { return _handle != INVALID_HANDLE_VALUE && _handle != 0; }
				CFindChangeNotification() : _handle(INVALID_HANDLE_VALUE) 
				{ 
				}
				~CFindChangeNotification() 
				{
					Close();
				}
				bool Close() throw();
				HANDLE FindFirst(CFSTR pathName, bool watchSubtree, DWORD notifyFilter);
				bool FindNext() { return BOOLToBool(::FindNextChangeNotification(_handle)); }
			};
			#ifndef UNDER_CE
				bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings);
			#endif
		}
		namespace NName {
			int FASTCALL FindSepar(const wchar_t *s) throw();
			#ifndef USE_UNICODE_FSTRING
			int FASTCALL FindSepar(const FChar *s) throw();
			#endif
			void FASTCALL NormalizeDirPathPrefix(FString &dirPath); // ensures that it ended with '\\', if dirPath is not epmty
			void FASTCALL NormalizeDirPathPrefix(UString &dirPath);
			bool IsDrivePath(const wchar_t *s) throw();  // first 3 chars are drive chars like "a:\\"
			bool IsAltPathPrefix(CFSTR s) throw(); /* name: */
			#if defined(_WIN32) && !defined(UNDER_CE)
			extern const char * const kSuperPathPrefix; /* \\?\ */
			const uint kDevicePathPrefixSize = 4;
			const uint kSuperPathPrefixSize = 4;
			const uint kSuperUncPathPrefixSize = kSuperPathPrefixSize + 4;

			bool IsDevicePath(CFSTR s) throw(); /* \\.\ */
			bool IsSuperUncPath(CFSTR s) throw(); /* \\?\UNC\ */
			bool IsNetworkPath(CFSTR s) throw(); /* \\?\UNC\ or \\SERVER */

			/* GetNetworkServerPrefixSize() returns size of server prefix:
				\\?\UNC\SERVER\
				\\SERVER\
			in another cases it returns 0
			*/

			unsigned GetNetworkServerPrefixSize(CFSTR s) throw();
			bool IsNetworkShareRootPath(CFSTR s) throw(); /* \\?\UNC\SERVER\share or \\SERVER\share or with slash */
			bool IsDrivePath_SuperAllowed(CFSTR s) throw();  // first chars are drive chars like "a:\" or "\\?\a:\"
			bool IsDriveRootPath_SuperAllowed(CFSTR s) throw();  // exact drive root path "a:\" or "\\?\a:\"
			bool IsDrivePath2(const wchar_t *s) throw(); // first 2 chars are drive chars like "a:"
			// bool IsDriveName2(const wchar_t *s) throw(); // is drive name like "a:"
			bool IsSuperPath(const wchar_t *s) throw();
			bool IsSuperOrDevicePath(const wchar_t *s) throw();

			#ifndef USE_UNICODE_FSTRING
			bool IsDrivePath2(CFSTR s) throw(); // first 2 chars are drive chars like "a:"
			// bool IsDriveName2(CFSTR s) throw(); // is drive name like "a:"
			bool IsDrivePath(CFSTR s) throw();
			bool IsSuperPath(CFSTR s) throw();
			bool IsSuperOrDevicePath(CFSTR s) throw();

			/* GetRootPrefixSize() returns size of ROOT PREFIX for cases:
				\
				\\.\
				C:\
				\\?\C:\
				\\?\UNC\SERVER\Shared\
				\\SERVER\Shared\
			in another cases it returns 0
			*/

			unsigned GetRootPrefixSize(CFSTR s) throw();
			#endif
			int FindAltStreamColon(CFSTR path) throw();
			#endif // _WIN32
			bool IsAbsolutePath(const wchar_t *s) throw();
			unsigned GetRootPrefixSize(const wchar_t *s) throw();
			#ifdef WIN_LONG_PATH
				const int kSuperPathType_UseOnlyMain = 0;
				const int kSuperPathType_UseOnlySuper = 1;
				const int kSuperPathType_UseMainAndSuper = 2;

				int GetUseSuperPathType(CFSTR s) throw();
				bool GetSuperPath(CFSTR path, UString &superPath, bool onlyIfNew);
				bool GetSuperPaths(CFSTR s1, CFSTR s2, UString &d1, UString &d2, bool onlyIfNew);

				#define USE_MAIN_PATH (__useSuperPathType != kSuperPathType_UseOnlySuper)
				#define USE_MAIN_PATH_2 (__useSuperPathType1 != kSuperPathType_UseOnlySuper && __useSuperPathType2 != kSuperPathType_UseOnlySuper)
				#define USE_SUPER_PATH (__useSuperPathType != kSuperPathType_UseOnlyMain)
				#define USE_SUPER_PATH_2 (__useSuperPathType1 != kSuperPathType_UseOnlyMain || __useSuperPathType2 != kSuperPathType_UseOnlyMain)

				#define IF_USE_MAIN_PATH int __useSuperPathType = GetUseSuperPathType(path); if(USE_MAIN_PATH)
				#define IF_USE_MAIN_PATH_2(x1, x2) \
					int __useSuperPathType1 = GetUseSuperPathType(x1); \
					int __useSuperPathType2 = GetUseSuperPathType(x2); \
					if(USE_MAIN_PATH_2)

			#else
				#define IF_USE_MAIN_PATH
				#define IF_USE_MAIN_PATH_2(x1, x2)
			#endif // WIN_LONG_PATH
			bool GetFullPath(CFSTR dirPrefix, CFSTR path, FString &fullPath);
			bool GetFullPath(CFSTR path, FString &fullPath);
		}
	}
}
//
#if defined(_WIN32) && !defined(UNDER_CE)  && !defined(_SFX)
	#define _USE_SECURITY_CODE
	//#include <Windows/SecurityUtils.h>
	#include <NTSecAPI.h>

	namespace NWindows {
		namespace NSecurity {
			class CAccessToken {
				HANDLE _handle;
			public:
				CAccessToken() : _handle(NULL) 
				{
				}
				~CAccessToken() 
				{
					Close();
				}
				bool Close()
				{
					if(!_handle)
						return true;
					bool res = BOOLToBool(::CloseHandle(_handle));
					if(res)
						_handle = NULL;
					return res;
				}
				bool OpenProcessToken(HANDLE processHandle, DWORD desiredAccess)
				{
					Close();
					return BOOLToBool(::OpenProcessToken(processHandle, desiredAccess, &_handle));
				}
				/*
				   bool OpenThreadToken(HANDLE threadHandle, DWORD desiredAccess, bool openAsSelf)
				   {
				   Close();
				   return BOOLToBool(::OpenTreadToken(threadHandle, desiredAccess, BoolToBOOL(anOpenAsSelf), &_handle));
				   }
				 */
				bool AdjustPrivileges(bool disableAllPrivileges, PTOKEN_PRIVILEGES newState,
					DWORD bufferLength, PTOKEN_PRIVILEGES previousState, PDWORD returnLength)
				{
					return BOOLToBool(::AdjustTokenPrivileges(_handle, BoolToBOOL(disableAllPrivileges), newState, bufferLength, previousState, returnLength));
				}
				bool AdjustPrivileges(bool disableAllPrivileges, PTOKEN_PRIVILEGES newState)
				{
					return AdjustPrivileges(disableAllPrivileges, newState, 0, NULL, NULL);
				}
				bool AdjustPrivileges(PTOKEN_PRIVILEGES newState)
				{
					return AdjustPrivileges(false, newState);
				}
			};

			#ifndef _UNICODE
				typedef NTSTATUS (NTAPI *LsaOpenPolicyP)(PLSA_UNICODE_STRING SystemName, PLSA_OBJECT_ATTRIBUTES ObjectAttributes, ACCESS_MASK DesiredAccess, PLSA_HANDLE PolicyHandle);
				typedef NTSTATUS (NTAPI *LsaCloseP)(LSA_HANDLE ObjectHandle);
				typedef NTSTATUS (NTAPI *LsaAddAccountRightsP)(LSA_HANDLE PolicyHandle, PSID AccountSid, PLSA_UNICODE_STRING UserRights, ULONG CountOfRights);
				#define MY_STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002L)
			#endif

			struct CPolicy {
			protected:
				LSA_HANDLE _handle;
			  #ifndef _UNICODE
				HMODULE hModule;
			  #endif
			public:
				operator LSA_HANDLE() const { return _handle; }
				CPolicy() : _handle(NULL)
				{
				#ifndef _UNICODE
					hModule = GetModuleHandle(TEXT("Advapi32.dll"));
				#endif
				};
				~CPolicy() 
				{
					Close();
				}
				NTSTATUS Open(PLSA_UNICODE_STRING systemName, PLSA_OBJECT_ATTRIBUTES objectAttributes, ACCESS_MASK desiredAccess)
				{
				#ifndef _UNICODE
					if(!hModule)
						return MY_STATUS_NOT_IMPLEMENTED;
					LsaOpenPolicyP lsaOpenPolicy = (LsaOpenPolicyP)GetProcAddress(hModule, "LsaOpenPolicy");
					if(!lsaOpenPolicy)
						return MY_STATUS_NOT_IMPLEMENTED;
				#endif
					Close();
					return
				  #ifdef _UNICODE
						::LsaOpenPolicy
				  #else
						lsaOpenPolicy
				  #endif
							(systemName, objectAttributes, desiredAccess, &_handle);
				}
				NTSTATUS Close()
				{
					if(!_handle)
						return 0;
				#ifndef _UNICODE
					if(!hModule)
						return MY_STATUS_NOT_IMPLEMENTED;
					LsaCloseP lsaClose = (LsaCloseP)GetProcAddress(hModule, "LsaClose");
					if(!lsaClose)
						return MY_STATUS_NOT_IMPLEMENTED;
				#endif
					NTSTATUS res =
				  #ifdef _UNICODE
								::LsaClose
				  #else
								lsaClose
				  #endif
									(_handle);
					_handle = NULL;
					return res;
				}
				NTSTATUS EnumerateAccountsWithUserRight(PLSA_UNICODE_STRING userRights, PLSA_ENUMERATION_INFORMATION * enumerationBuffer, PULONG countReturned)
				{
					return LsaEnumerateAccountsWithUserRight(_handle, userRights, (void **)enumerationBuffer, countReturned);
				}
				NTSTATUS EnumerateAccountRights(PSID sid, PLSA_UNICODE_STRING* userRights, PULONG countOfRights)
				{
					return ::LsaEnumerateAccountRights(_handle, sid, userRights, countOfRights);
				}
				NTSTATUS LookupSids(ULONG count, PSID* sids, PLSA_REFERENCED_DOMAIN_LIST* referencedDomains, PLSA_TRANSLATED_NAME* names)
				{
					return LsaLookupSids(_handle, count, sids, referencedDomains, names);
				}
				NTSTATUS AddAccountRights(PSID accountSid, PLSA_UNICODE_STRING userRights, ULONG countOfRights)
				{
				#ifndef _UNICODE
					if(!hModule)
						return MY_STATUS_NOT_IMPLEMENTED;
					LsaAddAccountRightsP lsaAddAccountRights = (LsaAddAccountRightsP)GetProcAddress(hModule, "LsaAddAccountRights");
					if(!lsaAddAccountRights)
						return MY_STATUS_NOT_IMPLEMENTED;
				#endif
					return
				  #ifdef _UNICODE
						::LsaAddAccountRights
				  #else
						lsaAddAccountRights
				  #endif
							(_handle, accountSid, userRights, countOfRights);
				}
				NTSTATUS AddAccountRights(PSID accountSid, PLSA_UNICODE_STRING userRights)
				{
					return AddAccountRights(accountSid, userRights, 1);
				}
				NTSTATUS RemoveAccountRights(PSID accountSid, bool allRights, PLSA_UNICODE_STRING userRights, ULONG countOfRights)
				{
					return LsaRemoveAccountRights(_handle, accountSid, (BOOLEAN)(allRights ? TRUE : FALSE), userRights, countOfRights);
				}
			};
			bool AddLockMemoryPrivilege();
		}
	}
	//
#endif
//
//#include <Windows/PropVariantUtils.h>
struct CUInt32PCharPair {
	uint32 Value;
	const char * Name;
};

AString TypePairToString(const CUInt32PCharPair * pairs, unsigned num, uint32 value);
void PairToProp(const CUInt32PCharPair * pairs, unsigned num, uint32 value, NWindows::NCOM::CPropVariant &prop);

AString FlagsToString(const char * const * names, unsigned num, uint32 flags);
AString FlagsToString(const CUInt32PCharPair * pairs, unsigned num, uint32 flags);
void FlagsToProp(const char * const * names, unsigned num, uint32 flags, NWindows::NCOM::CPropVariant &prop);
void FlagsToProp(const CUInt32PCharPair * pairs, unsigned num, uint32 flags, NWindows::NCOM::CPropVariant &prop);

AString TypeToString(const char * const table[], unsigned num, uint32 value);
void TypeToProp(const char * const table[], unsigned num, uint32 value, NWindows::NCOM::CPropVariant &prop);

#define PAIR_TO_PROP(pairs, value, prop) PairToProp(pairs, SIZEOFARRAY(pairs), value, prop)
#define FLAGS_TO_PROP(pairs, value, prop) FlagsToProp(pairs, SIZEOFARRAY(pairs), value, prop)
#define TYPE_TO_PROP(table, value, prop) TypeToProp(table, SIZEOFARRAY(table), value, prop)

void Flags64ToProp(const CUInt32PCharPair * pairs, unsigned num, uint64 flags, NWindows::NCOM::CPropVariant &prop);
#define FLAGS64_TO_PROP(pairs, value, prop) Flags64ToProp(pairs, SIZEOFARRAY(pairs), value, prop)
//
//#include <Windows/PropVariantConv.h>
// provide at least 32 bytes for buffer including zero-end
#define kTimestampPrintLevel_DAY -3
// #define kTimestampPrintLevel_HOUR -2
#define kTimestampPrintLevel_MIN -1
#define kTimestampPrintLevel_SEC 0
#define kTimestampPrintLevel_NTFS 7

bool ConvertUtcFileTimeToString(const FILETIME &ft, char * s, int level = kTimestampPrintLevel_SEC) throw();
bool ConvertUtcFileTimeToString(const FILETIME &ft, wchar_t * s, int level = kTimestampPrintLevel_SEC) throw();

// provide at least 32 bytes for buffer including zero-end
// don't send VT_BSTR to these functions
void ConvertPropVariantToShortString(const PROPVARIANT &prop, char * dest) throw();
void ConvertPropVariantToShortString(const PROPVARIANT &prop, wchar_t * dest) throw();

inline bool ConvertPropVariantToUInt64(const PROPVARIANT &prop, uint64 &value)
{
	switch(prop.vt) {
		case VT_UI8: value = (uint64)prop.uhVal.QuadPart; return true;
		case VT_UI4: value = prop.ulVal; return true;
		case VT_UI2: value = prop.uiVal; return true;
		case VT_UI1: value = prop.bVal; return true;
		case VT_EMPTY: return false;
		default: throw 151199;
	}
}
//

//#include <CommandLineParser.h>
namespace NCommandLineParser {
	bool SplitCommandLine(const UString &src, UString &dest1, UString &dest2);
	void SplitCommandLine(const UString &s, UStringVector &parts);

	namespace NSwitchType {
		enum EEnum {
			kSimple,
			kMinus,
			kString,
			kChar
		};
	}

	struct CSwitchForm {
		const char * Key;
		Byte Type;
		bool Multi;
		Byte MinLen;
		// int MaxLen;
		const char * PostCharSet;
	};

	struct CSwitchResult {
		CSwitchResult() : ThereIs(false) 
		{
		}
		bool ThereIs;
		bool WithMinus;
		int PostCharIndex;
		UStringVector PostStrings;
	};

	class CParser {
	public:
		CParser();
		~CParser();
		bool ParseStrings(const CSwitchForm * switchForms, unsigned numSwitches, const UStringVector &commandStrings);
		const CSwitchResult& operator[](uint index) const { return _switches[index]; }

		UStringVector NonSwitchStrings;
		int StopSwitchIndex; // NonSwitchStrings[StopSwitchIndex+] are after "--"
		AString ErrorMessage;
		UString ErrorLine;
	private:
		bool ParseString(const UString &s, const CSwitchForm * switchForms, unsigned numSwitches);
		CSwitchResult * _switches;
	};
}
//
//#include <DeflateConst.h>
namespace NCompress {
	namespace NDeflate {
		const uint kNumHuffmanBits = 15;

		const uint32 kHistorySize32 = (1 << 15);
		const uint32 kHistorySize64 = (1 << 16);
		const uint kDistTableSize32 = 30;
		const uint kDistTableSize64 = 32;
		const uint kNumLenSymbols32 = 256;
		const uint kNumLenSymbols64 = 255; // don't change it. It must be <= 255.
		const uint kNumLenSymbolsMax = kNumLenSymbols32;
		const uint kNumLenSlots = 29;
		const uint kFixedDistTableSize = 32;
		const uint kFixedLenTableSize = 31;
		const uint kSymbolEndOfBlock = 0x100;
		const uint kSymbolMatch = kSymbolEndOfBlock + 1;
		const uint kMainTableSize = kSymbolMatch + kNumLenSlots;
		const uint kFixedMainTableSize = kSymbolMatch + kFixedLenTableSize;
		const uint kLevelTableSize = 19;
		const uint kTableDirectLevels = 16;
		const uint kTableLevelRepNumber = kTableDirectLevels;
		const uint kTableLevel0Number = kTableLevelRepNumber + 1;
		const uint kTableLevel0Number2 = kTableLevel0Number + 1;
		const uint kLevelMask = 0xF;

		const Byte kLenStart32[kFixedLenTableSize] =
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 255, 0, 0};
		const Byte kLenStart64[kFixedLenTableSize] =
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 0, 0, 0};

		const Byte kLenDirectBits32[kFixedLenTableSize] =
		{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 0, 0, 0};
		const Byte kLenDirectBits64[kFixedLenTableSize] =
		{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 16, 0, 0};

		const uint32 kDistStart[kDistTableSize64] =
		{0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768,
		1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768, 49152};
		const Byte kDistDirectBits[kDistTableSize64] =
		{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14};

		const Byte kLevelDirectBits[3] = {2, 3, 7};

		const Byte kCodeLengthAlphabetOrder[kLevelTableSize] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

		const uint kMatchMinLen = 3;
		const uint kMatchMaxLen32 = kNumLenSymbols32 + kMatchMinLen - 1; // 256 + 2
		const uint kMatchMaxLen64 = kNumLenSymbols64 + kMatchMinLen - 1; // 255 + 2
		const uint kMatchMaxLen = kMatchMaxLen32;

		const uint kFinalBlockFieldSize = 1;

		namespace NFinalBlockField {
			enum {
				kNotFinalBlock = 0,
				kFinalBlock = 1
			};
		}

		const uint kBlockTypeFieldSize = 2;

		namespace NBlockType { 
			enum {
				kStored = 0,
				kFixedHuffman = 1,
				kDynamicHuffman = 2
			};
		}

		const uint kNumLenCodesFieldSize = 5;
		const uint kNumDistCodesFieldSize = 5;
		const uint kNumLevelCodesFieldSize = 4;
		const uint kNumLitLenCodesMin = 257;
		const uint kNumDistCodesMin = 1;
		const uint kNumLevelCodesMin = 4;
		const uint kLevelFieldSize = 3;
		const uint kStoredBlockLengthFieldSize = 16;

		struct CLevels {
			Byte litLenLevels[kFixedMainTableSize];
			Byte distLevels[kFixedDistTableSize];
			void SubClear()
			{
				uint i;
				for(i = kNumLitLenCodesMin; i < kFixedMainTableSize; i++)
					litLenLevels[i] = 0;
				for(i = 0; i < kFixedDistTableSize; i++)
					distLevels[i] = 0;
			}
			void SetFixedLevels()
			{
				unsigned i = 0;
				for(; i < 144; i++) litLenLevels[i] = 8;
				for(; i < 256; i++) litLenLevels[i] = 9;
				for(; i < 280; i++) litLenLevels[i] = 7;
				for(; i < 288; i++) litLenLevels[i] = 8;

				for(i = 0; i < kFixedDistTableSize; i++) // test it: InfoZip only uses kDistTableSize
					distLevels[i] = 5;
			}
		};
	}
}
//
//#include <LzFind.h> <LzFindMt.h>
EXTERN_C_BEGIN

	typedef uint32 CLzRef;

	typedef struct _CMatchFinder {
		Byte * buffer;
		uint32 pos;
		uint32 posLimit;
		uint32 streamPos;
		uint32 lenLimit;
		uint32 cyclicBufferPos;
		uint32 cyclicBufferSize; /* it must be = (historySize + 1) */
		Byte streamEndWasReached;
		Byte btMode;
		Byte bigHash;
		Byte directInput;
		uint32 matchMaxLen;
		CLzRef * hash;
		CLzRef * son;
		uint32 hashMask;
		uint32 cutValue;
		Byte * bufferBase;
		ISeqInStream * stream;
		uint32 blockSize;
		uint32 keepSizeBefore;
		uint32 keepSizeAfter;
		uint32 numHashBytes;
		size_t directInputRem;
		uint32 historySize;
		uint32 fixedHashSize;
		uint32 hashSizeSum;
		SRes result;
		uint32 crc[256];
		size_t numRefs;
	} CMatchFinder;

	#define Inline_MatchFinder_GetPointerToCurrentPos(p) ((p)->buffer)
	#define Inline_MatchFinder_GetNumAvailableBytes(p) ((p)->streamPos - (p)->pos)
	#define Inline_MatchFinder_IsFinishedOK(p) ((p)->streamEndWasReached && (p)->streamPos == (p)->pos && (!(p)->directInput || (p)->directInputRem == 0))

	int FASTCALL MatchFinder_NeedMove(const CMatchFinder * p);
	Byte * MatchFinder_GetPointerToCurrentPos(CMatchFinder * p);
	void FASTCALL MatchFinder_MoveBlock(CMatchFinder * p);
	void FASTCALL MatchFinder_ReadIfRequired(CMatchFinder * p);
	void MatchFinder_Construct(CMatchFinder * p);
	/* Conditions:
		historySize <= 3 GB
		keepAddBufferBefore + matchMaxLen + keepAddBufferAfter < 511MB
	*/
	int MatchFinder_Create(CMatchFinder * p, uint32 historySize, uint32 keepAddBufferBefore, uint32 matchMaxLen, uint32 keepAddBufferAfter, ISzAllocPtr alloc);
	void FASTCALL MatchFinder_Free(CMatchFinder * p, ISzAllocPtr alloc);
	void FASTCALL MatchFinder_Normalize3(uint32 subValue, CLzRef * items, size_t numItems);
	void FASTCALL MatchFinder_ReduceOffsets(CMatchFinder * p, uint32 subValue);
	uint32 * GetMatchesSpec1(uint32 lenLimit, uint32 curMatch, uint32 pos, const Byte * buffer, CLzRef * son,
		uint32 _cyclicBufferPos, uint32 _cyclicBufferSize, uint32 _cutValue, uint32 * distances, uint32 maxLen);
	/*
	Conditions:
	Mf_GetNumAvailableBytes_Func must be called before each Mf_GetMatchLen_Func.
	Mf_GetPointerToCurrentPos_Func's result must be used only before any other function
	*/
	typedef void (*Mf_Init_Func)(void * object);
	typedef uint32 (*Mf_GetNumAvailableBytes_Func)(void * object);
	typedef const Byte * (*Mf_GetPointerToCurrentPos_Func)(void * object);
	typedef uint32 (*Mf_GetMatches_Func)(void * object, uint32 * distances);
	typedef void (*Mf_Skip_Func)(void * object, uint32);

	typedef struct _IMatchFinder {
		Mf_Init_Func Init;
		Mf_GetNumAvailableBytes_Func GetNumAvailableBytes;
		Mf_GetPointerToCurrentPos_Func GetPointerToCurrentPos;
		Mf_GetMatches_Func GetMatches;
		Mf_Skip_Func Skip;
	} IMatchFinder;

	void MatchFinder_CreateVTable(const CMatchFinder * p, IMatchFinder * vTable);
	void FASTCALL MatchFinder_Init_2(CMatchFinder * p, int readData);
	void MatchFinder_Init(CMatchFinder * p); // @fptr
	uint32 Bt3Zip_MatchFinder_GetMatches(CMatchFinder * p, uint32 * distances);
	uint32 Hc3Zip_MatchFinder_GetMatches(CMatchFinder * p, uint32 * distances);
	void Bt3Zip_MatchFinder_Skip(CMatchFinder * p, uint32 num);
	void Hc3Zip_MatchFinder_Skip(CMatchFinder * p, uint32 num);

	#define kMtHashBlockSize (1 << 13)
	#define kMtHashNumBlocks (1 << 3)
	#define kMtHashNumBlocksMask (kMtHashNumBlocks - 1)

	#define kMtBtBlockSize (1 << 14)
	#define kMtBtNumBlocks (1 << 6)
	#define kMtBtNumBlocksMask (kMtBtNumBlocks - 1)

	typedef struct _CMtSync {
		Bool wasCreated;
		Bool needStart;
		Bool exit;
		Bool stopWriting;

		CThread thread;
		CAutoResetEvent canStart;
		CAutoResetEvent wasStarted;
		CAutoResetEvent wasStopped;
		CSemaphore freeSemaphore;
		CSemaphore filledSemaphore;
		Bool csWasInitialized;
		Bool csWasEntered;
		CCriticalSection cs;
		uint32 numProcessedBlocks;
	} CMtSync;

	#define kMtCacheLineDummy 128 // kMtCacheLineDummy must be >= size_of_CPU_cache_line 

	typedef uint32 * (*Mf_Mix_Matches)(void * p, uint32 matchMinPos, uint32 * distances);
	typedef void (*Mf_GetHeads)(const Byte * buffer, uint32 pos, uint32 * hash, uint32 hashMask, uint32 * heads, uint32 numHeads, const uint32 * crc);

	typedef struct _CMatchFinderMt {
		/* LZ */
		const Byte * pointerToCurPos;
		uint32 * btBuf;
		uint32 btBufPos;
		uint32 btBufPosLimit;
		uint32 lzPos;
		uint32 btNumAvailBytes;
		uint32 * hash;
		uint32 fixedHashSize;
		uint32 historySize;
		const uint32 * crc;
		Mf_Mix_Matches MixMatchesFunc;
		/* LZ + BT */
		CMtSync btSync;
		Byte btDummy[kMtCacheLineDummy];
		/* BT */
		uint32 * hashBuf;
		uint32 hashBufPos;
		uint32 hashBufPosLimit;
		uint32 hashNumAvail;
		CLzRef * son;
		uint32 matchMaxLen;
		uint32 numHashBytes;
		uint32 pos;
		const Byte * buffer;
		uint32 cyclicBufferPos;
		uint32 cyclicBufferSize; /* it must be historySize + 1 */
		uint32 cutValue;
		/* BT + Hash */
		CMtSync hashSync;
		/* Byte hashDummy[kMtCacheLineDummy]; */
		/* Hash */
		Mf_GetHeads GetHeadsFunc;
		CMatchFinder * MatchFinder;
	} CMatchFinderMt;

	void MatchFinderMt_Construct(CMatchFinderMt * p);
	void MatchFinderMt_Destruct(CMatchFinderMt * p, ISzAllocPtr alloc);
	SRes MatchFinderMt_Create(CMatchFinderMt * p, uint32 historySize, uint32 keepAddBufferBefore, uint32 matchMaxLen, uint32 keepAddBufferAfter, ISzAllocPtr alloc);
	void MatchFinderMt_CreateVTable(CMatchFinderMt * p, IMatchFinder * vTable);
	void MatchFinderMt_ReleaseStream(CMatchFinderMt * p);

EXTERN_C_END
//
//#include <LzHash.h>
#define kHash2Size (1 << 10)
#define kHash3Size (1 << 16)
#define kHash4Size (1 << 20)

#define kFix3HashSize (kHash2Size)
#define kFix4HashSize (kHash2Size + kHash3Size)
#define kFix5HashSize (kHash2Size + kHash3Size + kHash4Size)

#define HASH2_CALC hv = cur[0] | ((uint32)cur[1] << 8);

#define HASH3_CALC { \
		uint32 temp = p->crc[cur[0]] ^ cur[1]; \
		h2 = temp & (kHash2Size - 1); \
		hv = (temp ^ ((uint32)cur[2] << 8)) & p->hashMask; }

#define HASH4_CALC { \
		uint32 temp = p->crc[cur[0]] ^ cur[1]; \
		h2 = temp & (kHash2Size - 1); \
		temp ^= ((uint32)cur[2] << 8); \
		h3 = temp & (kHash3Size - 1); \
		hv = (temp ^ (p->crc[cur[3]] << 5)) & p->hashMask; }

#define HASH5_CALC { \
		uint32 temp = p->crc[cur[0]] ^ cur[1]; \
		h2 = temp & (kHash2Size - 1); \
		temp ^= ((uint32)cur[2] << 8); \
		h3 = temp & (kHash3Size - 1); \
		temp ^= (p->crc[cur[3]] << 5); \
		h4 = temp & (kHash4Size - 1); \
		hv = (temp ^ (p->crc[cur[4]] << 3)) & p->hashMask; }

/* #define HASH_ZIP_CALC hv = ((cur[0] | ((uint32)cur[1] << 8)) ^ p->crc[cur[2]]) & 0xFFFF; */
#define HASH_ZIP_CALC hv = ((cur[2] | ((uint32)cur[0] << 8)) ^ p->crc[cur[1]]) & 0xFFFF;
#define MT_HASH2_CALC h2 = (p->crc[cur[0]] ^ cur[1]) & (kHash2Size - 1);

#define MT_HASH3_CALC {	\
		uint32 temp = p->crc[cur[0]] ^ cur[1]; \
		h2 = temp & (kHash2Size - 1); \
		h3 = (temp ^ ((uint32)cur[2] << 8)) & (kHash3Size - 1); }

#define MT_HASH4_CALC {	\
		uint32 temp = p->crc[cur[0]] ^ cur[1]; \
		h2 = temp & (kHash2Size - 1); \
		temp ^= ((uint32)cur[2] << 8); \
		h3 = temp & (kHash3Size - 1); \
		h4 = (temp ^ (p->crc[cur[3]] << 5)) & (kHash4Size - 1); }
//
//#include <MyUnknown.h>
#ifdef _WIN32
	#include <basetyps.h>
	#include <unknwn.h>
#else
	//#include "MyWindows.h"
#endif
//
//#include <IDecl.h>
#define k_7zip_GUID_Data1 0x23170F69
#define k_7zip_GUID_Data2 0x40C1
#define k_7zip_GUID_Data3_Common  0x278A
#define k_7zip_GUID_Data3_Decoder 0x2790
#define k_7zip_GUID_Data3_Encoder 0x2791
#define k_7zip_GUID_Data3_Hasher  0x2792

#define DECL_INTERFACE_SUB(i, base, groupId, subId) \
  DEFINE_GUID(IID_ ## i, k_7zip_GUID_Data1, k_7zip_GUID_Data2, k_7zip_GUID_Data3_Common, 0, 0, 0, (groupId), 0, (subId), 0, 0); \
  struct i: public base

#define DECL_INTERFACE(i, groupId, subId) DECL_INTERFACE_SUB(i, IUnknown, groupId, subId)
//
//#include <Bcj2.h>
EXTERN_C_BEGIN
	#define BCJ2_NUM_STREAMS 4

	enum {
		BCJ2_STREAM_MAIN,
		BCJ2_STREAM_CALL,
		BCJ2_STREAM_JUMP,
		BCJ2_STREAM_RC
	};

	enum {
		BCJ2_DEC_STATE_ORIG_0 = BCJ2_NUM_STREAMS,
		BCJ2_DEC_STATE_ORIG_1,
		BCJ2_DEC_STATE_ORIG_2,
		BCJ2_DEC_STATE_ORIG_3,

		BCJ2_DEC_STATE_ORIG,
		BCJ2_DEC_STATE_OK
	};

	enum {
		BCJ2_ENC_STATE_ORIG = BCJ2_NUM_STREAMS,
		BCJ2_ENC_STATE_OK
	};

	#define BCJ2_IS_32BIT_STREAM(s) ((s) == BCJ2_STREAM_CALL || (s) == BCJ2_STREAM_JUMP)

	/*
	CBcj2Dec / CBcj2Enc
	bufs sizes:
	BUF_SIZE(n) = lims[n] - bufs[n]
	bufs sizes for BCJ2_STREAM_CALL and BCJ2_STREAM_JUMP must be mutliply of 4:
		(BUF_SIZE(BCJ2_STREAM_CALL) & 3) == 0
		(BUF_SIZE(BCJ2_STREAM_JUMP) & 3) == 0
	*/

	/*
	CBcj2Dec:
	dest is allowed to overlap with bufs[BCJ2_STREAM_MAIN], with the following conditions:
	bufs[BCJ2_STREAM_MAIN] >= dest &&
	bufs[BCJ2_STREAM_MAIN] - dest >= tempReserv +
			BUF_SIZE(BCJ2_STREAM_CALL) +
			BUF_SIZE(BCJ2_STREAM_JUMP)
		tempReserv = 0 : for first call of Bcj2Dec_Decode
		tempReserv = 4 : for any other calls of Bcj2Dec_Decode
	overlap with offset = 1 is not allowed
	*/

	typedef struct {
		const Byte * bufs[BCJ2_NUM_STREAMS];
		const Byte * lims[BCJ2_NUM_STREAMS];
		Byte * dest;
		const Byte * destLim;
		unsigned state; /* BCJ2_STREAM_MAIN has more priority than BCJ2_STATE_ORIG */
		uint32 ip;
		Byte temp[4];
		uint32 range;
		uint32 code;
		uint16 probs[2 + 256];
	} CBcj2Dec;

	void Bcj2Dec_Init(CBcj2Dec * p);

	/* Returns: SZ_OK or SZ_ERROR_DATA */
	SRes Bcj2Dec_Decode(CBcj2Dec * p);

	#define Bcj2Dec_IsFinished(_p_) ((_p_)->code == 0)

	typedef enum {
		BCJ2_ENC_FINISH_MODE_CONTINUE,
		BCJ2_ENC_FINISH_MODE_END_BLOCK,
		BCJ2_ENC_FINISH_MODE_END_STREAM
	} EBcj2Enc_FinishMode;

	typedef struct {
		Byte * bufs[BCJ2_NUM_STREAMS];
		const Byte * lims[BCJ2_NUM_STREAMS];
		const Byte * src;
		const Byte * srcLim;
		unsigned state;
		EBcj2Enc_FinishMode finishMode;
		Byte prevByte;
		Byte cache;
		uint32 range;
		uint64 low;
		uint64 cacheSize;
		uint32 ip;
		/* 32-bit ralative offset in JUMP/CALL commands is
			- (mod 4 GB)   in 32-bit mode
			- signed int32 in 64-bit mode
		We use (mod 4 GB) check for fileSize.
		Use fileSize up to 2 GB, if you want to support 32-bit and 64-bit code conversion. */
		uint32 fileIp;
		uint32 fileSize; /* (fileSize <= ((uint32)1 << 31)), 0 means no_limit */
		uint32 relatLimit; /* (relatLimit <= ((uint32)1 << 31)), 0 means desable_conversion */
		uint32 tempTarget;
		unsigned tempPos;
		Byte temp[4 * 2];
		unsigned flushPos;
		uint16 probs[2 + 256];
	} CBcj2Enc;

	void Bcj2Enc_Init(CBcj2Enc * p);
	void Bcj2Enc_Encode(CBcj2Enc * p);

	#define Bcj2Enc_Get_InputData_Size(p) ((SizeT)((p)->srcLim - (p)->src) + (p)->tempPos)
	#define Bcj2Enc_IsFinished(p) ((p)->flushPos == 5)

	#define BCJ2_RELAT_LIMIT_NUM_BITS 26
	#define BCJ2_RELAT_LIMIT ((uint32)1 << BCJ2_RELAT_LIMIT_NUM_BITS)

	/* limit for CBcj2Enc::fileSize variable */
	#define BCJ2_FileSize_MAX ((uint32)1 << 31)
EXTERN_C_END
//
//#include <ItemNameUtils.h> <7zProperties.h> <7zHeader.h> <7zItem.h> <ZipHeader.h> <ZipItem.h> <CabHeader.h> <CabItem.h>
namespace NArchive {
	namespace NHandlerPropID {
		enum {
			kName = 0,      // VT_BSTR
			kClassID,       // binary GUID in VT_BSTR
			kExtension,     // VT_BSTR
			kAddExtension,  // VT_BSTR
			kUpdate,        // VT_BOOL
			kKeepName,      // VT_BOOL
			kSignature,     // binary in VT_BSTR
			kMultiSignature, // binary in VT_BSTR
			kSignatureOffset, // VT_UI4
			kAltStreams,    // VT_BOOL
			kNtSecure,      // VT_BOOL
			kFlags          // VT_UI4
			// kVersion          // VT_UI4 ((VER_MAJOR << 8) | VER_MINOR)
		};
	}
	namespace NEventIndexType {
		enum {
			kNoIndex = 0,
			kInArcIndex,
			kBlockIndex,
			kOutArcIndex
		};
	}
	namespace NUpdate {
		namespace NOperationResult {
			enum {
				kOK = 0
				, // kError
			};
		}
	}
	namespace NItemName {
		void ReplaceSlashes_OsToUnix(UString &name);
		UString GetOsPath(const UString &name);
		UString GetOsPath_Remove_TailSlash(const UString &name);
		void ReplaceToOsSlashes_Remove_TailSlash(UString &name);
		bool HasTailSlash(const AString &name, UINT codePage);
		#ifdef _WIN32
			inline UString WinPathToOsPath(const UString &name) { return name; }
		#else
			UString WinPathToOsPath(const UString &name);
		#endif
	}
	namespace N7z {
		enum {
			kpidPackedSize0 = kpidUserDefined,
			kpidPackedSize1,
			kpidPackedSize2,
			kpidPackedSize3,
			kpidPackedSize4
		};

		const uint kSignatureSize = 6;
		extern Byte kSignature[kSignatureSize];

		// #define _7Z_VOL
		// 7z-MultiVolume is not finished yet.
		// It can work already, but I still do not like some
		// things of that new multivolume format.
		// So please keep it commented.

		#ifdef _7Z_VOL
			extern Byte kFinishSignature[kSignatureSize];
		#endif

		struct CArchiveVersion {
			Byte Major;
			Byte Minor;
		};

		const Byte kMajorVersion = 0;

		struct CStartHeader {
			uint64 NextHeaderOffset;
			uint64 NextHeaderSize;
			uint32 NextHeaderCRC;
		};

		const uint32 kStartHeaderSize = 20;

		#ifdef _7Z_VOL
			struct CFinishHeader : public CStartHeader {
				uint64 ArchiveStartOffset; // data offset from end if that struct
				uint64 AdditionalStartBlockSize; // start  signature & start header size
			};

			const uint32 kFinishHeaderSize = kStartHeaderSize + 16;
		#endif

		namespace NID {
			enum EEnum {
				kEnd,
				kHeader,
				kArchiveProperties,
				kAdditionalStreamsInfo,
				kMainStreamsInfo,
				kFilesInfo,
				kPackInfo,
				kUnpackInfo,
				kSubStreamsInfo,
				kSize,
				kCRC,
				kFolder,
				kCodersUnpackSize,
				kNumUnpackStream,
				kEmptyStream,
				kEmptyFile,
				kAnti,
				kName,
				kCTime,
				kATime,
				kMTime,
				kWinAttrib,
				kComment,
				kEncodedHeader,
				kStartPos,
				kDummy

				// kNtSecure,
				// kParent,
				// kIsAux
			};
		}

		const uint32 k_Copy = 0;
		const uint32 k_Delta = 3;
		const uint32 k_LZMA2 = 0x21;
		const uint32 k_SWAP2 = 0x20302;
		const uint32 k_SWAP4 = 0x20304;
		const uint32 k_LZMA  = 0x30101;
		const uint32 k_PPMD  = 0x30401;
		const uint32 k_Deflate = 0x40108;
		const uint32 k_BZip2   = 0x40202;
		const uint32 k_BCJ   = 0x3030103;
		const uint32 k_BCJ2  = 0x303011B;
		const uint32 k_PPC   = 0x3030205;
		const uint32 k_IA64  = 0x3030401;
		const uint32 k_ARM   = 0x3030501;
		const uint32 k_ARMT  = 0x3030701;
		const uint32 k_SPARC = 0x3030805;
		const uint32 k_AES   = 0x6F10701;

		static inline bool IsFilterMethod(uint64 m)
		{
			if(m > (uint64)0xFFFFFFFF)
				return false;
			switch((uint32)m) {
				case k_Delta:
				case k_BCJ:
				case k_BCJ2:
				case k_PPC:
				case k_IA64:
				case k_ARM:
				case k_ARMT:
				case k_SPARC:
				case k_SWAP2:
				case k_SWAP4:
					return true;
			}
			return false;
		}

		typedef uint32 CNum;
		const CNum kNumMax     = 0x7FFFFFFF;
		const CNum kNumNoIndex = 0xFFFFFFFF;

		struct CCoderInfo {
			CMethodId MethodID;
			CByteBuffer Props;
			uint32 NumStreams;
			bool IsSimpleCoder() const { return NumStreams == 1; }
		};

		struct CBond {
			uint32 PackIndex;
			uint32 UnpackIndex;
		};

		struct CFolder {
			CLASS_NO_COPY(CFolder)
		public:
			CObjArray2 <CCoderInfo> Coders;
			CObjArray2 <CBond> Bonds;
			CObjArray2 <uint32> PackStreams;

			CFolder() 
			{
			}
			bool IsDecodingSupported() const { return Coders.Size() <= 32; }
			int Find_in_PackStreams(uint32 packStream) const
			{
				FOR_VECTOR(i, PackStreams) {
					if(PackStreams[i] == packStream)
						return i;
				}
				return -1;
			}
			int FindBond_for_PackStream(uint32 packStream) const
			{
				FOR_VECTOR(i, Bonds) {
					if(Bonds[i].PackIndex == packStream)
						return i;
				}
				return -1;
			}
			/*
			int FindBond_for_UnpackStream(uint32 unpackStream) const
			{
			FOR_VECTOR(i, Bonds) {
				if(Bonds[i].UnpackIndex == unpackStream)
					return i;
			}
			return -1;
			}

			int FindOutCoder() const
			{
			for(int i = (int)Coders.Size() - 1; i >= 0; i--)
				if(FindBond_for_UnpackStream(i) < 0)
				return i;
			return -1;
			}
			*/
			bool IsEncrypted() const
			{
				FOR_VECTOR(i, Coders) {
					if(Coders[i].MethodID == k_AES)
						return true;
				}
				return false;
			}
		};

		struct CUInt32DefVector {
			void ClearAndSetSize(unsigned newSize)
			{
				Defs.ClearAndSetSize(newSize);
				Vals.ClearAndSetSize(newSize);
			}
			void Clear()
			{
				Defs.Clear();
				Vals.Clear();
			}
			void ReserveDown()
			{
				Defs.ReserveDown();
				Vals.ReserveDown();
			}
			bool GetItem(uint index, uint32 &value) const
			{
				if(index < Defs.Size() && Defs[index]) {
					value = Vals[index];
					return true;
				}
				value = 0;
				return false;
			}
			bool ValidAndDefined(unsigned i) const { return i < Defs.Size() && Defs[i]; }
			bool CheckSize(uint size) const { return Defs.Size() == size || Defs.Size() == 0; }
			void SetItem(uint index, bool defined, uint32 value);

			CBoolVector Defs;
			CRecordVector<uint32> Vals;
		};

		struct CUInt64DefVector {
			void Clear()
			{
				Defs.Clear();
				Vals.Clear();
			}
			void ReserveDown()
			{
				Defs.ReserveDown();
				Vals.ReserveDown();
			}
			bool GetItem(uint index, uint64 &value) const
			{
				if(index < Defs.Size() && Defs[index]) {
					value = Vals[index];
					return true;
				}
				value = 0;
				return false;
			}
			bool CheckSize(uint size) const { return Defs.Size() == size || Defs.Size() == 0; }
			void SetItem(uint index, bool defined, uint64 value);

			CBoolVector Defs;
			CRecordVector<uint64> Vals;
		};

		struct CFileItem {
			uint64 Size;
			uint32 Crc;
			/*
			int Parent;
			bool IsAltStream;
			*/
			bool HasStream; // Test it !!! it means that there is
							// stream in some folder. It can be empty stream
			bool IsDir;
			bool CrcDefined;

			/*
			void Clear()
			{
			HasStream = true;
			IsDir = false;
			CrcDefined = false;
			}

			CFileItem():
			// Parent(-1),
			// IsAltStream(false),
			HasStream(true),
			IsDir(false),
			CrcDefined(false),
				{}
			*/
		};
	}
	namespace NZip {
		const uint kMarkerSize = 4;

		namespace NSignature {
			const uint32 kLocalFileHeader   = 0x04034B50;
			const uint32 kDataDescriptor    = 0x08074B50;
			const uint32 kCentralFileHeader = 0x02014B50;
			const uint32 kEcd       = 0x06054B50;
			const uint32 kEcd64     = 0x06064B50;
			const uint32 kEcd64Locator      = 0x07064B50;
			const uint32 kSpan      = 0x08074B50;
			const uint32 kNoSpan    = 0x30304B50;   // PK00, replaces kSpan, if there is only 1 segment
		}

		const uint kLocalHeaderSize = 4 + 26; // including signature
		const uint kDataDescriptorSize32 = 4 + 4 + 4 * 2;  // including signature
		const uint kDataDescriptorSize64 = 4 + 4 + 8 * 2;  // including signature
		const uint kCentralHeaderSize = 4 + 42; // including signature
		const uint kEcdSize = 22; // including signature
		const uint kEcd64_MainSize = 44;
		const uint kEcd64_FullSize = 12 + kEcd64_MainSize;
		const uint kEcd64Locator_Size = 20;

		namespace NFileHeader {
			namespace NCompressionMethod {
				enum EType {
					kStore = 0,
					kShrink = 1,
					kReduce1 = 2,
					kReduce2 = 3,
					kReduce3 = 4,
					kReduce4 = 5,
					kImplode = 6,
					kTokenize = 7,
					kDeflate = 8,
					kDeflate64 = 9,
					kPKImploding = 10,
					kBZip2 = 12,
					kLZMA = 14,
					kTerse = 18,
					kLz77 = 19,
					kXz = 95,
					kJpeg = 96,
					kWavPack = 97,
					kPPMd = 98,
					kWzAES = 99
				};

				const Byte kMadeByProgramVersion = 63;

				const Byte kExtractVersion_Default = 10;
				const Byte kExtractVersion_Dir = 20;
				const Byte kExtractVersion_ZipCrypto = 20;
				const Byte kExtractVersion_Deflate = 20;
				const Byte kExtractVersion_Deflate64 = 21;
				const Byte kExtractVersion_Zip64 = 45;
				const Byte kExtractVersion_BZip2 = 46;
				const Byte kExtractVersion_Aes = 51;
				const Byte kExtractVersion_LZMA = 63;
				const Byte kExtractVersion_PPMd = 63;
				const Byte kExtractVersion_Xz = 20;     // test it
			}
			namespace NExtraID {
				enum {
					kZip64 = 0x01,
					kNTFS = 0x0A,
					kStrongEncrypt = 0x17,
					kUnixTime = 0x5455,
					kUnixExtra = 0x5855,
					kIzUnicodeComment = 0x6375,
					kIzUnicodeName = 0x7075,
					kWzAES = 0x9901
				};
			}
			namespace NNtfsExtra {
				const uint16 kTagTime = 1;
				enum {
					kMTime = 0,
					kATime,
					kCTime
				};
			}
			namespace NUnixTime {
				enum {
					kMTime = 0,
					kATime,
					kCTime
				};
			}
			namespace NUnixExtra {
				enum {
					kATime = 0,
					kMTime
				};
			}

			namespace NFlags {
				const uint kEncrypted = 1 << 0;
				const uint kLzmaEOS = 1 << 1;
				const uint kDescriptorUsedMask = 1 << 3;
				const uint kStrongEncrypted = 1 << 6;
				const uint kUtf8 = 1 << 11;
				const uint kImplodeDictionarySizeMask = 1 << 1;
				const uint kImplodeLiteralsOnMask     = 1 << 2;
				/*
				const uint kDeflateTypeBitStart = 1;
				const uint kNumDeflateTypeBits = 2;
				const uint kNumDeflateTypes = (1 << kNumDeflateTypeBits);
				const uint kDeflateTypeMask = (1 << kNumDeflateTypeBits) - 1;
				*/
			}

			namespace NHostOS {
				enum EEnum {
					kFAT      =  0,
					kAMIGA    =  1,
					kVMS      =  2, // VAX/VMS
					kUnix     =  3,
					kVM_CMS   =  4,
					kAtari    =  5, // what if it's a minix filesystem? [cjh]
					kHPFS     =  6, // filesystem used by OS/2 (and NT 3.x)
					kMac      =  7,
					kZ_System =  8,
					kCPM      =  9,
					kTOPS20   = 10, // pkzip 2.50 NTFS
					kNTFS     = 11, // filesystem used by Windows NT
					kQDOS     = 12, // SMS/QDOS
					kAcorn    = 13, // Archimedes Acorn RISC OS
					kVFAT     = 14, // filesystem used by Windows 95, NT
					kMVS      = 15,
					kBeOS     = 16, // hybrid POSIX/database filesystem
					kTandem   = 17,
					kOS400    = 18,
					kOSX      = 19
				};
			}
			namespace NAmigaAttrib {
				const uint32 kIFMT     = 06000;        // Amiga file type mask
				const uint32 kIFDIR    = 04000;        // Amiga directory
				const uint32 kIFREG    = 02000;        // Amiga regular file
				const uint32 kIHIDDEN  = 00200;        // to be supported in AmigaDOS 3.x
				const uint32 kISCRIPT  = 00100;        // executable script (text command file)
				const uint32 kIPURE    = 00040;        // allow loading into resident memory
				const uint32 kIARCHIVE = 00020;        // not modified since bit was last set
				const uint32 kIREAD    = 00010;        // can be opened for reading
				const uint32 kIWRITE   = 00004;        // can be opened for writing
				const uint32 kIEXECUTE = 00002;        // executable image, a loadable runfile
				const uint32 kIDELETE  = 00001;        // can be deleted
			}
		}

		struct CVersion {
			Byte Version;
			Byte HostOS;
		};

		struct CExtraSubBlock {
			uint32 ID;
			CByteBuffer Data;

			bool ExtractNtfsTime(uint index, FILETIME &ft) const;
			bool ExtractUnixTime(bool isCentral, unsigned index, uint32 &res) const;
			bool ExtractUnixExtraTime(uint index, uint32 &res) const;
			bool ExtractIzUnicode(uint32 crc, AString &name) const
			{
				uint size = (uint)Data.Size();
				if(size < 1 + 4)
					return false;
				else {
					const Byte * p = (const Byte *)Data;
					if(p[0] > 1)
						return false;
					else if(crc != GetUi32(p + 1))
						return false;
					else {
						size -= 5;
						name.SetFrom_CalcLen((const char *)p + 5, size);
						return (size != name.Len()) ? false : CheckUTF8(name, false);
					}
				}
			}
			void PrintInfo(AString &s) const;
		};
		const uint k_WzAesExtra_Size = 7;

		struct CWzAesExtra {
			uint16 VendorVersion; // 1: AE-1, 2: AE-2,
			// uint16 VendorId; // 'A' 'E'
			Byte Strength; // 1: 128-bit, 2: 192-bit, 3: 256-bit
			uint16 Method;
			CWzAesExtra() : VendorVersion(2), Strength(3), Method(0) 
			{
			}
			bool NeedCrc() const { return (VendorVersion == 1); }
			bool ParseFromSubBlock(const CExtraSubBlock &sb)
			{
				if(sb.ID != NFileHeader::NExtraID::kWzAES)
					return false;
				else if(sb.Data.Size() < k_WzAesExtra_Size)
					return false;
				else {
					const Byte * p = (const Byte *)sb.Data;
					VendorVersion = GetUi16(p);
					if(p[2] != 'A' || p[3] != 'E')
						return false;
					else {
						Strength = p[4];
						// 9.31: The BUG was fixed:
						Method = GetUi16(p + 5);
						return true;
					}
				}
			}
			void SetSubBlock(CExtraSubBlock &sb) const
			{
				sb.Data.Alloc(k_WzAesExtra_Size);
				sb.ID = NFileHeader::NExtraID::kWzAES;
				Byte * p = (Byte *)sb.Data;
				p[0] = (Byte)VendorVersion;
				p[1] = (Byte)(VendorVersion >> 8);
				p[2] = 'A';
				p[3] = 'E';
				p[4] = Strength;
				p[5] = (Byte)Method;
				p[6] = (Byte)(Method >> 8);
			}
		};

		namespace NStrongCrypto_AlgId {
			const uint16 kDES = 0x6601;
			const uint16 kRC2old = 0x6602;
			const uint16 k3DES168 = 0x6603;
			const uint16 k3DES112 = 0x6609;
			const uint16 kAES128 = 0x660E;
			const uint16 kAES192 = 0x660F;
			const uint16 kAES256 = 0x6610;
			const uint16 kRC2 = 0x6702;
			const uint16 kBlowfish = 0x6720;
			const uint16 kTwofish = 0x6721;
			const uint16 kRC4 = 0x6801;
		}

		struct CStrongCryptoExtra {
			uint16 Format;
			uint16 AlgId;
			uint16 BitLen;
			uint16 Flags;
			bool   FASTCALL ParseFromSubBlock(const CExtraSubBlock & sb);
			bool CertificateIsUsed() const;
		};

		struct CExtraBlock {
			CObjectVector <CExtraSubBlock> SubBlocks;
			bool Error;
			bool MinorError;
			bool IsZip64;
			bool IsZip64_Error;

			CExtraBlock();
			void Clear();
			size_t GetSize() const;
			bool FASTCALL GetWzAes(CWzAesExtra & e) const;
			bool HasWzAes() const;
			bool FASTCALL GetStrongCrypto(CStrongCryptoExtra & e) const;
			/*
			   bool HasStrongCrypto() const
			   {
			   CStrongCryptoExtra e;
			   return GetStrongCrypto(e);
			   }
			 */
			bool GetNtfsTime(uint index, FILETIME &ft) const;
			bool GetUnixTime(bool isCentral, unsigned index, uint32 &res) const;
			void PrintInfo(AString &s) const;
			void RemoveUnknownSubBlocks();
		};

		class CLocalItem {
		public:
			uint16 Flags;
			uint16 Method;
			CVersion ExtractVersion;
			uint64 Size;
			uint64 PackSize;
			uint32 Time;
			uint32 Crc;
			uint32 Disk;
			AString Name;
			CExtraBlock LocalExtra;

			unsigned GetDescriptorSize() const { return LocalExtra.IsZip64 ? kDataDescriptorSize64 : kDataDescriptorSize32; }
			uint64 GetPackSizeWithDescriptor() const { return PackSize + (HasDescriptor() ? GetDescriptorSize() : 0); }
			bool IsUtf8() const { return (Flags & NFileHeader::NFlags::kUtf8) != 0; }
			bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kEncrypted) != 0; }
			bool IsStrongEncrypted() const { return IsEncrypted() && (Flags & NFileHeader::NFlags::kStrongEncrypted) != 0; }
			bool IsAesEncrypted() const { return IsEncrypted() && (IsStrongEncrypted() || Method == NFileHeader::NCompressionMethod::kWzAES); }
			bool IsLzmaEOS() const { return (Flags & NFileHeader::NFlags::kLzmaEOS) != 0; }
			bool HasDescriptor() const { return (Flags & NFileHeader::NFlags::kDescriptorUsedMask) != 0; }
			unsigned GetDeflateLevel() const { return (Flags >> 1) & 3; }
			bool IsDir() const;
			/*
			   void GetUnicodeString(const AString &s, UString &res) const
			   {
			   bool isUtf8 = IsUtf8();
			   if(isUtf8)
				if(ConvertUTF8ToUnicode(s, res))
				  return;
			   MultiByteToUnicodeString2(res, s, GetCodePage());
			   }
			 */
		private:
			void SetFlag(unsigned bitMask, bool enable) { SETFLAG(Flags, bitMask, enable); }
		public:
			void ClearFlags() { Flags = 0; }
			void SetEncrypted(bool encrypted) { SetFlag(NFileHeader::NFlags::kEncrypted, encrypted); }
			void SetUtf8(bool isUtf8) { SetFlag(NFileHeader::NFlags::kUtf8, isUtf8); }
			void SetDescriptorMode(bool useDescriptor) { SetFlag(NFileHeader::NFlags::kDescriptorUsedMask, useDescriptor); }
			UINT GetCodePage() const { return CP_OEMCP; }
		};

		class CItem : public CLocalItem {
		public:
			CVersion MadeByVersion;
			uint16 InternalAttrib;
			uint32 ExternalAttrib;
			uint64 LocalHeaderPos;
			CExtraBlock CentralExtra;
			CByteBuffer Comment;
			bool FromLocal;
			bool FromCentral;

			// CItem can be used as CLocalItem. So we must clear unused fields
			CItem() : InternalAttrib(0), ExternalAttrib(0), FromLocal(false), FromCentral(false)
			{
				MadeByVersion.Version = 0;
				MadeByVersion.HostOS = 0;
			}
			const CExtraBlock &GetMainExtra() const { return *(FromCentral ? &CentralExtra : &LocalExtra); }
			bool IsDir() const;
			uint32 GetWinAttrib() const;
			bool GetPosixAttrib(uint32 &attrib) const;
			Byte GetHostOS() const { return FromCentral ? MadeByVersion.HostOS : ExtractVersion.HostOS; }
			void GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage, UINT codePage) const;
			bool IsThereCrc() const
			{
				if(Method == NFileHeader::NCompressionMethod::kWzAES) {
					CWzAesExtra aesField;
					if(GetMainExtra().GetWzAes(aesField))
						return aesField.NeedCrc();
				}
				return (Crc != 0 || !IsDir());
			}
			UINT GetCodePage() const
			{
				Byte hostOS = GetHostOS();
				return (UINT)((hostOS == NFileHeader::NHostOS::kFAT || hostOS == NFileHeader::NHostOS::kNTFS || hostOS == NFileHeader::NHostOS::kUnix/*do we need it?*/) ? CP_OEMCP : CP_ACP);
			}
		};
	}
	namespace NCab {
		namespace NHeader {
			const uint kMarkerSize = 8;
			extern const Byte kMarker[kMarkerSize];

			namespace NArcFlags {
				const uint kPrevCabinet = 1;
				const uint kNextCabinet = 2;
				const uint kReservePresent = 4;
			}
			namespace NMethod {
				const Byte kNone = 0;
				const Byte kMSZip = 1;
				const Byte kQuantum = 2;
				const Byte kLZX = 3;
			}

			const uint kFileNameIsUtf8_Mask = 0x80;

			namespace NFolderIndex {
				const uint kContinuedFromPrev    = 0xFFFD;
				const uint kContinuedToNext      = 0xFFFE;
				const uint kContinuedPrevAndNext = 0xFFFF;
			}
		}

		const uint kNumMethodsMax = 16;

		struct CFolder {
			Byte GetMethod() const { return (Byte)(MethodMajor & 0xF); }

			uint32 DataStart; // offset of the first CFDATA block in this folder
			uint16 NumDataBlocks; // number of CFDATA blocks in this folder
			Byte MethodMajor;
			Byte MethodMinor;
		};

		struct CItem {
			AString Name;
			uint32 Offset;
			uint32 Size;
			uint32 Time;
			uint32 FolderIndex;
			uint16 Flags;
			uint16 Attributes;

			uint64 GetEndOffset() const { return (uint64)Offset + Size; }
			uint32 GetWinAttrib() const { return (uint32)Attributes & ~(uint32) NHeader::kFileNameIsUtf8_Mask; }
			bool IsNameUTF() const { return (Attributes & NHeader::kFileNameIsUtf8_Mask) != 0; }
			bool IsDir() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
			bool ContinuedFromPrev() const
			{
				return oneof2(FolderIndex, NHeader::NFolderIndex::kContinuedFromPrev, NHeader::NFolderIndex::kContinuedPrevAndNext);
			}
			bool ContinuedToNext() const
			{
				return oneof2(FolderIndex, NHeader::NFolderIndex::kContinuedToNext, NHeader::NFolderIndex::kContinuedPrevAndNext);
			}
			int GetFolderIndex(unsigned numFolders) const
			{
				if(ContinuedFromPrev())
					return 0;
				else if(ContinuedToNext())
					return numFolders - 1;
				else
					return FolderIndex;
			}
		};
	}
}
//
//#include <HuffEnc.h>
EXTERN_C_BEGIN
	/*
	Conditions:
	num <= 1024 = 2 ^ NUM_BITS
	Sum(freqs) < 4M = 2 ^ (32 - NUM_BITS)
	maxLen <= 16 = kMaxLen
	Num_Items(p) >= HUFFMAN_TEMP_SIZE(num)
	*/
	void Huffman_Generate(const uint32 *freqs, uint32 *p, Byte *lens, uint32 num, uint32 maxLen);
EXTERN_C_END
//
//#include <HuffmanDecoder.h> <BZip2Const.h>
namespace NCompress {
	namespace NHuffman {
		const uint kNumPairLenBits = 4;
		const uint kPairLenMask = (1 << kNumPairLenBits) - 1;

		template <unsigned kNumBitsMax, uint32 m_NumSymbols, unsigned kNumTableBits = 9> class CDecoder {
		public:
			uint32 _limits[kNumBitsMax + 2];
			uint32 _poses[kNumBitsMax + 1];
			uint16 _lens[1 << kNumTableBits];
			uint16 _symbols[m_NumSymbols];

			bool Build(const Byte * lens) throw()
			{
				uint32 counts[kNumBitsMax + 1];
				/*uint   i;
				for(i = 0; i <= kNumBitsMax; i++)
					counts[i] = 0;*/
				memzero(counts, sizeof(counts));
				uint32 sym;
				for(sym = 0; sym < m_NumSymbols; sym++)
					counts[lens[sym]]++;
				const uint32 kMaxValue = (uint32)1 << kNumBitsMax;
				_limits[0] = 0;
				uint32 startPos = 0;
				uint32 sum = 0;
				for(uint i = 1; i <= kNumBitsMax; i++) {
					const uint32 cnt = counts[i];
					startPos += cnt << (kNumBitsMax - i);
					if(startPos > kMaxValue)
						return false;
					_limits[i] = startPos;
					counts[i] = sum;
					_poses[i] = sum;
					sum += cnt;
				}
				counts[0] = sum;
				_poses[0] = sum;
				_limits[kNumBitsMax + 1] = kMaxValue;
				for(sym = 0; sym < m_NumSymbols; sym++) {
					const uint len = lens[sym];
					if(len) {
						uint   offset = counts[len]++;
						_symbols[offset] = (uint16)sym;
						if(len <= kNumTableBits) {
							offset -= _poses[len];
							uint32 num = (uint32)1 << (kNumTableBits - len);
							uint16 val = static_cast<uint16>((sym << kNumPairLenBits) | len);
							uint16 * dest = _lens + (_limits[(size_t)len - 1] >> (kNumBitsMax - kNumTableBits)) + (offset << (kNumTableBits - len));
							for(uint32 k = 0; k < num; k++)
								dest[k] = val;
						}
					}
				}
				return true;
			}
			bool BuildFull(const Byte * lens, uint32 numSymbols = m_NumSymbols) throw()
			{
				uint32 counts[kNumBitsMax + 1];
				/*uint   i;
				for(i = 0; i <= kNumBitsMax; i++)
					counts[i] = 0;*/
				memzero(counts, sizeof(counts));
				uint32 sym;
				for(sym = 0; sym < numSymbols; sym++)
					counts[lens[sym]]++;
				const uint32 kMaxValue = (uint32)1 << kNumBitsMax;
				_limits[0] = 0;
				uint32 startPos = 0;
				uint32 sum = 0;
				for(uint i = 1; i <= kNumBitsMax; i++) {
					const uint32 cnt = counts[i];
					startPos += cnt << (kNumBitsMax - i);
					if(startPos > kMaxValue)
						return false;
					_limits[i] = startPos;
					counts[i] = sum;
					_poses[i] = sum;
					sum += cnt;
				}
				counts[0] = sum;
				_poses[0] = sum;
				_limits[kNumBitsMax + 1] = kMaxValue;
				for(sym = 0; sym < numSymbols; sym++) {
					const uint len = lens[sym];
					if(len) {
						uint   offset = counts[len]++;
						_symbols[offset] = (uint16)sym;
						if(len <= kNumTableBits) {
							offset -= _poses[len];
							uint32 num = (1U << (kNumTableBits - len));
							uint16 val = static_cast<uint16>((sym << kNumPairLenBits) | len);
							uint16 * dest = _lens + (_limits[(size_t)len - 1] >> (kNumBitsMax - kNumTableBits)) + (offset << (kNumTableBits - len));
							for(uint32 k = 0; k < num; k++)
								dest[k] = val;
						}
					}
				}
				return startPos == kMaxValue;
			}
			template <class TBitDecoder> FORCEINLINE uint32 Decode(TBitDecoder * bitStream) const
			{
				uint32 val = bitStream->GetValue(kNumBitsMax);
				if(val < _limits[kNumTableBits]) {
					uint32 pair = _lens[val >> (kNumBitsMax - kNumTableBits)];
					bitStream->MovePos((uint)(pair & kPairLenMask));
					return pair >> kNumPairLenBits;
				}
				unsigned numBits;
				for(numBits = kNumTableBits + 1; val >= _limits[numBits]; numBits++) 
					;
				if(numBits > kNumBitsMax)
					return 0xFFFFFFFF;
				bitStream->MovePos(numBits);
				uint32 index = _poses[numBits] + ((val - _limits[(size_t)numBits - 1]) >> (kNumBitsMax - numBits));
				return _symbols[index];
			}
			template <class TBitDecoder> FORCEINLINE uint32 DecodeFull(TBitDecoder * bitStream) const
			{
				uint32 val = bitStream->GetValue(kNumBitsMax);
				if(val < _limits[kNumTableBits]) {
					uint32 pair = _lens[val >> (kNumBitsMax - kNumTableBits)];
					bitStream->MovePos((uint)(pair & kPairLenMask));
					return pair >> kNumPairLenBits;
				}
				unsigned numBits;
				for(numBits = kNumTableBits + 1; val >= _limits[numBits]; numBits++) 
					;
				bitStream->MovePos(numBits);
				uint32 index = _poses[numBits] + ((val - _limits[(size_t)numBits - 1]) >> (kNumBitsMax - numBits));
				return _symbols[index];
			}
		};

		template <uint32 m_NumSymbols> class CDecoder7b {
			Byte _lens[1 << 7];
		public:
			bool Build(const Byte * lens) throw()
			{
				const uint kNumBitsMax = 7;
				uint32 counts[kNumBitsMax + 1];
				uint32 _poses[kNumBitsMax + 1];
				uint32 _limits[kNumBitsMax + 1];
				/*uint i;
				for(i = 0; i <= kNumBitsMax; i++)
					counts[i] = 0;*/
				memzero(counts, sizeof(counts));
				uint32 sym;
				for(sym = 0; sym < m_NumSymbols; sym++)
					counts[lens[sym]]++;
				const uint32 kMaxValue = (uint32)1 << kNumBitsMax;
				_limits[0] = 0;
				uint32 startPos = 0;
				uint32 sum = 0;
				for(uint i = 1; i <= kNumBitsMax; i++) {
					const uint32 cnt = counts[i];
					startPos += cnt << (kNumBitsMax - i);
					if(startPos > kMaxValue)
						return false;
					_limits[i] = startPos;
					counts[i] = sum;
					_poses[i] = sum;
					sum += cnt;
				}
				counts[0] = sum;
				_poses[0] = sum;
				for(sym = 0; sym < m_NumSymbols; sym++) {
					const uint len = lens[sym];
					if(len) {
						uint offset = counts[len]++;
						{
							offset -= _poses[len];
							uint32 num = (uint32)1 << (kNumBitsMax - len);
							Byte val = (Byte)((sym << 3) | len);
							Byte * dest = _lens + (_limits[(size_t)len - 1]) + (offset << (kNumBitsMax - len));
							for(uint32 k = 0; k < num; k++)
								dest[k] = val;
						}
					}
				}
				{
					uint32 limit = _limits[kNumBitsMax];
					uint32 num = ((uint32)1 << kNumBitsMax) - limit;
					Byte * dest = _lens + limit;
					for(uint32 k = 0; k < num; k++)
						dest[k] = (Byte)(0x1F << 3);
				}
				return true;
			}
			template <class TBitDecoder> uint32 Decode(TBitDecoder * bitStream) const
			{
				uint32 val = bitStream->GetValue(7);
				uint32 pair = _lens[val];
				bitStream->MovePos((uint)(pair & 0x7));
				return pair >> 3;
			}
		};
	}
	namespace NBZip2 {
		const Byte kArSig0 = 'B';
		const Byte kArSig1 = 'Z';
		const Byte kArSig2 = 'h';
		const Byte kArSig3 = '0';

		const Byte kFinSig0 = 0x17;
		const Byte kFinSig1 = 0x72;
		const Byte kFinSig2 = 0x45;
		const Byte kFinSig3 = 0x38;
		const Byte kFinSig4 = 0x50;
		const Byte kFinSig5 = 0x90;

		const Byte kBlockSig0 = 0x31;
		const Byte kBlockSig1 = 0x41;
		const Byte kBlockSig2 = 0x59;
		const Byte kBlockSig3 = 0x26;
		const Byte kBlockSig4 = 0x53;
		const Byte kBlockSig5 = 0x59;

		const uint kNumOrigBits = 24;
		const uint kNumTablesBits = 3;
		const uint kNumTablesMin = 2;
		const uint kNumTablesMax = 6;
		const uint kNumLevelsBits = 5;
		const uint kMaxHuffmanLen = 20; // Check it
		const uint kMaxAlphaSize = 258;
		const uint kGroupSize = 50;
		const uint kBlockSizeMultMin = 1;
		const uint kBlockSizeMultMax = 9;
		const uint32 kBlockSizeStep = 100000;
		const uint32 kBlockSizeMax = kBlockSizeMultMax * kBlockSizeStep;
		const uint kNumSelectorsBits = 15;
		const uint32 kNumSelectorsMax = (2 + (kBlockSizeMax / kGroupSize));
		const uint kRleModeRepSize = 4;
	}
}
//
//#include <BZip2Crc.h>
class CBZip2Crc {
public:
	static void InitTable();
	CBZip2Crc() : _value(0xFFFFFFFF) 
	{
	}
	void Init() { _value = 0xFFFFFFFF; }
	void UpdateByte(Byte b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
	void UpdateByte(unsigned int b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
	uint32 GetDigest() const { return _value ^ 0xFFFFFFFF; }
private:
	static uint32 Table[256];
	uint32 _value;
};

class CBZip2CombinedCrc {
public:
	CBZip2CombinedCrc() :  _value(0)
	{
	}
	void Init() { _value = 0; }
	void Update(uint32 v) { _value = ((_value << 1) | (_value >> 31)) ^ v; }
	uint32 GetDigest() const { return _value; }
private:
	uint32 _value;
};
//
//#include <Mtf8.h>
namespace NCompress {
	struct CMtf8Encoder {
		Byte Buf[256];
		unsigned FindAndMove(Byte v) throw()
		{
			size_t pos;
			for(pos = 0; Buf[pos] != v; pos++) 
				;
			unsigned resPos = (uint)pos;
			for(; pos >= 8; pos -= 8) {
				Buf[pos] = Buf[pos - 1];
				Buf[pos - 1] = Buf[pos - 2];
				Buf[pos - 2] = Buf[pos - 3];
				Buf[pos - 3] = Buf[pos - 4];
				Buf[pos - 4] = Buf[pos - 5];
				Buf[pos - 5] = Buf[pos - 6];
				Buf[pos - 6] = Buf[pos - 7];
				Buf[pos - 7] = Buf[pos - 8];
			}
			for(; pos != 0; pos--)
				Buf[pos] = Buf[pos - 1];
			Buf[0] = v;
			return resPos;
		}
	};
	/*
	   struct CMtf8Decoder {
	   Byte Buf[256];

	   void StartInit() { memzero(Buf, sizeof(Buf)); }
	   void Add(unsigned pos, Byte val) { Buf[pos] = val;  }
	   Byte GetHead() const { return Buf[0]; }
	   Byte GetAndMove(unsigned pos)
	   {
		Byte res = Buf[pos];
		for(; pos >= 8; pos -= 8) {
		  Buf[pos] = Buf[pos - 1];
		  Buf[pos - 1] = Buf[pos - 2];
		  Buf[pos - 2] = Buf[pos - 3];
		  Buf[pos - 3] = Buf[pos - 4];
		  Buf[pos - 4] = Buf[pos - 5];
		  Buf[pos - 5] = Buf[pos - 6];
		  Buf[pos - 6] = Buf[pos - 7];
		  Buf[pos - 7] = Buf[pos - 8];
		}
		for(; pos > 0; pos--)
		  Buf[pos] = Buf[pos - 1];
		Buf[0] = res;
		return res;
	   }
	   };
	 */
	#ifdef MY_CPU_64BIT
		typedef uint64 CMtfVar;
		#define MTF_MOVS 3
	#else
		typedef uint32 CMtfVar;
		#define MTF_MOVS 2
	#endif
	#define MTF_MASK ((1 << MTF_MOVS) - 1)

	struct CMtf8Decoder {
		CMtfVar Buf[256 >> MTF_MOVS];
		void StartInit() { memzero(Buf, sizeof(Buf)); }
		void Add(unsigned pos, Byte val) { Buf[pos >> MTF_MOVS] |= ((CMtfVar)val << ((pos & MTF_MASK) << 3)); }
		Byte GetHead() const { return (Byte)Buf[0]; }
		FORCEINLINE Byte GetAndMove(unsigned pos) throw()
		{
			uint32 lim = ((uint32)pos >> MTF_MOVS);
			pos = (pos & MTF_MASK) << 3;
			CMtfVar prev = (Buf[lim] >> pos) & 0xFF;
			uint32 i = 0;
			/*
			   if((lim & 1) != 0) {
			   CMtfVar next = Buf[0];
			   Buf[0] = (next << 8) | prev;
			   prev = (next >> (MTF_MASK << 3));
			   i = 1;
			   lim -= 1;
			   }
			   for(; i < lim; i += 2) {
			   CMtfVar n0 = Buf[i];
			   CMtfVar n1 = Buf[i+1];
			   Buf[i    ] = (n0 << 8) | prev;
			   Buf[i+1] = (n1 << 8) | (n0 >> (MTF_MASK << 3));
			   prev = (n1 >> (MTF_MASK << 3));
			   }
			 */
			for(; i < lim; i++) {
				CMtfVar n0 = Buf[i];
				Buf[i    ] = (n0 << 8) | prev;
				prev = (n0 >> (MTF_MASK << 3));
			}
			CMtfVar next = Buf[i];
			CMtfVar mask = (((CMtfVar)0x100 << pos) - 1);
			Buf[i] = (next & ~mask) | (((next << 8) | prev) & mask);
			return (Byte)Buf[0];
		}
	};
	/*
	   const int kSmallSize = 64;
	   class CMtf8Decoder {
	   Byte SmallBuffer[kSmallSize];
	   int SmallSize;
	   int Counts[16];
	   int Size;
	   public:
	   Byte Buf[256];

	   Byte GetHead() const { return (SmallSize > 0) ? SmallBuffer[kSmallSize - SmallSize] : Buf[0]; }
	   void Init(int size)
	   {
		Size = size;
		SmallSize = 0;
		for(int i = 0; i < 16; i++) {
		  Counts[i] = ((size >= 16) ? 16 : size);
		  size -= Counts[i];
		}
	   }
	   void Add(unsigned pos, Byte val) { Buf[pos] = val; }
	   Byte GetAndMove(int pos)
	   {
		if(pos < SmallSize) {
		  Byte *p = SmallBuffer + kSmallSize - SmallSize;
		  Byte res = p[pos];
		  for(; pos > 0; pos--)
			p[pos] = p[pos - 1];
		  SmallBuffer[kSmallSize - SmallSize] = res;
		  return res;
		}
		if(SmallSize == kSmallSize) {
		  int i = Size - 1;
		  int g = 16;
		  do {
			g--;
			int offset = (g << 4);
			for(int t = Counts[g] - 1; t >= 0; t--, i--)
			  Buf[i] = Buf[offset + t];
		  } while (g != 0);
		  for(i = kSmallSize - 1; i >= 0; i--)
			Buf[i] = SmallBuffer[i];
		  Init(Size);
		}
		pos -= SmallSize;
		int g;
		for(g = 0; pos >= Counts[g]; g++)
		  pos -= Counts[g];
		int offset = (g << 4);
		Byte res = Buf[offset + pos];
		for(pos; pos < 16 - 1; pos++)
		  Buf[offset + pos] = Buf[offset + pos + 1];
		SmallSize++;
		SmallBuffer[kSmallSize - SmallSize] = res;
		Counts[g]--;
		return res;
	   }
	   };
	 */
}
//
//#include <ListFileUtils.h>
#define MY__CP_UTF16   1200
#define MY__CP_UTF16BE 1201

bool ReadNamesFromListFile(CFSTR fileName, UStringVector &strings, UINT codePage = CP_OEMCP);
//
//#include <ExtractingFilePath.h>
#ifdef _WIN32
	void Correct_AltStream_Name(UString &s);
#endif
// replaces unsuported characters, and replaces "." , ".." and "" to "[]"
UString Get_Correct_FsFile_Name(const UString &name);
void Correct_FsPath(bool absIsAllowed, UStringVector &parts, bool isDir);
UString FASTCALL MakePathFromParts(const UStringVector &parts);
uint   FASTCALL GetNumSlashes(const FChar * s);
//
//#include <SetProperties.h>
HRESULT SetProperties(IUnknown *unknown, const CObjectVector<CProperty> &properties);
//
//#include <PropertyName.h>
UString GetNameOfProperty(PROPID propID, const wchar_t *name);
//
//#include <RotateDefs.h>
#ifdef _MSC_VER
	// don't use _rotl with MINGW. It can insert slow call to function. 
	/* #if(_MSC_VER >= 1200) */
	#pragma intrinsic(_rotl)
	#pragma intrinsic(_rotr)
	/* #endif */
	#define rotlFixed(x, n) _rotl((x), (n))
	#define rotrFixed(x, n) _rotr((x), (n))
#else
	/* new compilers can translate these macros to fast commands. */
	#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
	#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#endif
//
//#include <Sha1.h> <Sha256.h>
EXTERN_C_BEGIN
	#define SHA1_NUM_BLOCK_WORDS  16
	#define SHA1_NUM_DIGEST_WORDS  5

	#define SHA1_BLOCK_SIZE   (SHA1_NUM_BLOCK_WORDS * 4)
	#define SHA1_DIGEST_SIZE  (SHA1_NUM_DIGEST_WORDS * 4)

	typedef struct {
		uint32 state[SHA1_NUM_DIGEST_WORDS];
		uint64 count;
		uint32 buffer[SHA1_NUM_BLOCK_WORDS];
	} CSha1;

	void Sha1_Init(CSha1 * p);
	void Sha1_GetBlockDigest(const CSha1 * p, const uint32 * data, uint32 * destDigest);
	void Sha1_Update(CSha1 * p, const Byte * data, size_t size);
	void Sha1_Final(CSha1 * p, Byte * digest);
	void Sha1_Update_Rar(CSha1 * p, Byte * data, size_t size /* , int rar350Mode */);
	void Sha1_32_PrepareBlock(const CSha1 * p, uint32 * block, uint size);
	void Sha1_32_Update(CSha1 * p, const uint32 * data, size_t size);
	void Sha1_32_Final(CSha1 * p, uint32 * digest);

	#define SHA256_DIGEST_SIZE 32

	typedef struct {
		uint32 state[8];
		uint64 count;
		Byte buffer[64];
	} CSha256;

	void Sha256_Init(CSha256 * p);
	void Sha256_Update(CSha256 * p, const Byte * data, size_t size);
	void Sha256_Final(CSha256 * p, Byte * digest);
EXTERN_C_END
//
//#include <HmacSha256.h> <Sha1Cls.h> <HmacSha1.h> <Pbkdf2HmacSha1.h>
namespace NCrypto {
	namespace NSha256 {
		//
		// Implements HMAC-SHA-256 (RFC2104, FIPS-198)
		//
		const uint kDigestSize = SHA256_DIGEST_SIZE;

		class CHmac {
		public:
			void SetKey(const Byte * key, size_t keySize);
			void Update(const Byte * data, size_t dataSize) { Sha256_Update(&_sha, data, dataSize); }
			void Final(Byte * mac);
			// void Final(Byte *mac, size_t macSize);
		private:
			CSha256 _sha;
			CSha256 _sha2;
		};
	}
	namespace NSha1 {
		const uint kNumBlockWords = SHA1_NUM_BLOCK_WORDS;
		const uint kNumDigestWords = SHA1_NUM_DIGEST_WORDS;
		const uint kBlockSize = SHA1_BLOCK_SIZE;
		const uint kDigestSize = SHA1_DIGEST_SIZE;

		class CContextBase {
		public:
			void Init() throw() { Sha1_Init(&_s); }
			void GetBlockDigest(const uint32 * blockData, uint32 * destDigest) throw() { Sha1_GetBlockDigest(&_s, blockData, destDigest); }
		protected:
			CSha1 _s;
		};

		class CContext : public CContextBase {
		public:
			void Update(const Byte * data, size_t size) throw() { Sha1_Update(&_s, data, size); }
			void UpdateRar(Byte * data, size_t size /* , bool rar350Mode */) throw() { Sha1_Update_Rar(&_s, data, size /* , rar350Mode ? 1 : 0 */); }
			void Final(Byte * digest) throw() { Sha1_Final(&_s, digest); }
		};
		class CContext32 : public CContextBase {
		public:
			void Update(const uint32 * data, size_t size) throw() { Sha1_32_Update(&_s, data, size); }
			void Final(uint32 * digest) throw() { Sha1_32_Final(&_s, digest); }
			/* PrepareBlock can be used only when size <= 13. size in Words
			   _buffer must be empty (_count & 0xF) == 0) */
			void PrepareBlock(uint32 * block, uint size) const throw() { Sha1_32_PrepareBlock(&_s, block, size); }
		};
		//
		// Implements HMAC-SHA-1 (RFC2104, FIPS-198)
		//
		// Use:  SetKey(key, keySize); for() Update(data, size); Final(mac, macSize);
		class CHmac {
			CContext _sha;
			CContext _sha2;
		public:
			void SetKey(const Byte * key, size_t keySize);
			void Update(const Byte * data, size_t dataSize) { _sha.Update(data, dataSize); }
			void Final(Byte * mac, size_t macSize = kDigestSize);
		};
		class CHmac32 {
			CContext32 _sha;
			CContext32 _sha2;
		public:
			void SetKey(const Byte * key, size_t keySize);
			void Update(const uint32 * data, size_t dataSize) { _sha.Update(data, dataSize); }
			void Final(uint32 * mac, size_t macSize = kNumDigestWords);
			// It'sa for hmac function. in,out: mac[kNumDigestWords].
			void GetLoopXorDigest(uint32 * mac, uint32 numIteration);
		};

		void Pbkdf2Hmac(const Byte *pwd, size_t pwdSize, const Byte *salt, size_t saltSize, uint32 numIterations, Byte *key, size_t keySize);
		void Pbkdf2Hmac32(const Byte *pwd, size_t pwdSize, const uint32 *salt, size_t saltSize, uint32 numIterations, uint32 *key, size_t keySize);
	}
}
//
//#include <Aes.h>
// @sobolev EXTERN_C_BEGIN
#define AES_BLOCK_SIZE 16
//
// Call AesGenTables one time before other AES functions 
//
void AesGenTables(void);
//
// uint32 pointers must be 16-byte aligned 
//
// 16-byte (4 * 32-bit words) blocks: 1 (IV) + 1 (keyMode) + 15 (AES-256 roundKeys) 
#define AES_NUM_IVMRK_WORDS ((1 + 1 + 15) * 4)
//
// aes - 16-byte aligned pointer to keyMode+roundKeys sequence 
// keySize = 16 or 24 or 32 (bytes) 
//
typedef void (FASTCALL *AES_SET_KEY_FUNC)(uint32 *aes, const Byte *key, unsigned keySize);
void FASTCALL Aes_SetKey_Enc(uint32 *aes, const Byte *key, unsigned keySize);
void FASTCALL Aes_SetKey_Dec(uint32 *aes, const Byte *key, unsigned keySize);
//
// ivAes - 16-byte aligned pointer to iv+keyMode+roundKeys sequence: uint32[AES_NUM_IVMRK_WORDS] 
//
void AesCbc_Init(uint32 *ivAes, const Byte *iv); /* iv size is AES_BLOCK_SIZE */
//
// data - 16-byte aligned pointer to data 
// numBlocks - the number of 16-byte blocks in data array 
//
typedef void (FASTCALL *AES_CODE_FUNC)(uint32 *ivAes, Byte *data, size_t numBlocks);
extern AES_CODE_FUNC g_AesCbc_Encode;
extern AES_CODE_FUNC g_AesCbc_Decode;
extern AES_CODE_FUNC g_AesCtr_Code;
// @sobolev EXTERN_C_END
//
//#include <RandGen.h>
class CRandomGenerator {
public:
	CRandomGenerator() : _needInit(true) 
	{
	}
	void Generate(Byte * data, uint size);
private:
	void Init();

	Byte _buff[SHA256_DIGEST_SIZE];
	bool _needInit;
};

extern CRandomGenerator g_RandomGenerator;
//
//#include <LzmaDec.h>
EXTERN_C_BEGIN
	// #define _LZMA_PROB32 
	// _LZMA_PROB32 can increase the speed on some CPUs, but memory usage for CLzmaDec::probs will be doubled in that case 
	#ifdef _LZMA_PROB32
		#define CLzmaProb uint32
	#else
		#define CLzmaProb uint16
	#endif
	//
	// LZMA Properties
	//
	#define LZMA_PROPS_SIZE 5

	typedef struct _CLzmaProps {
		unsigned lc, lp, pb;
		uint32 dicSize;
	} CLzmaProps;
	// 
	// LzmaProps_Decode - decodes properties
	// Returns:
	//   SZ_OK
	// SZ_ERROR_UNSUPPORTED - Unsupported properties
	// 
	SRes LzmaProps_Decode(CLzmaProps * p, const Byte * data, uint size);
	// 
	// LZMA Decoder state
	// 
	/* LZMA_REQUIRED_INPUT_MAX = number of required input bytes for worst case.
	   Num bits = log2((2^11 / 31) ^ 22) + 26 < 134 + 26 = 160; */

	#define LZMA_REQUIRED_INPUT_MAX 20

	typedef struct {
		CLzmaProps prop;
		CLzmaProb * probs;
		Byte * dic;
		const Byte * buf;
		uint32 range, code;
		SizeT dicPos;
		SizeT dicBufSize;
		uint32 processedPos;
		uint32 checkDicSize;
		unsigned state;
		uint32 reps[4];
		unsigned remainLen;
		int needFlush;
		int needInitState;
		uint32 numProbs;
		unsigned tempBufSize;
		Byte tempBuf[LZMA_REQUIRED_INPUT_MAX];
	} CLzmaDec;

	#define LzmaDec_Construct(p) { (p)->dic = 0; (p)->probs = 0; }

	void LzmaDec_Init(CLzmaDec * p);

	/* There are two types of LZMA streams:
		 0) Stream with end mark. That end mark adds about 6 bytes to compressed size.
		 1) Stream without end mark. You must know exact uncompressed size to decompress such stream. */

	typedef enum {
		LZMA_FINISH_ANY, /* finish at any point */
		LZMA_FINISH_END /* block must be finished at the end */
	} ELzmaFinishMode;

	/* ELzmaFinishMode has meaning only if the decoding reaches output limit !!!

	   You must use LZMA_FINISH_END, when you know that current output buffer
	   covers last bytes of block. In other cases you must use LZMA_FINISH_ANY.

	   If LZMA decoder sees end marker before reaching output limit, it returns SZ_OK,
	   and output value of destLen will be less than output buffer size limit.
	   You can check status result also.

	   You can use multiple checks to test data integrity after full decompression:
		 1) Check Result and "status" variable.
		 2) Check that output(destLen) = uncompressedSize, if you know real uncompressedSize.
		 3) Check that output(srcLen) = compressedSize, if you know real compressedSize.
			You must use correct finish mode in that case. */

	typedef enum {
		LZMA_STATUS_NOT_SPECIFIED,         /* use main error code instead */
		LZMA_STATUS_FINISHED_WITH_MARK,    /* stream was finished with end mark. */
		LZMA_STATUS_NOT_FINISHED,          /* stream was not finished */
		LZMA_STATUS_NEEDS_MORE_INPUT,      /* you must provide more input bytes */
		LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK /* there is probability that stream was finished without end mark */
	} ELzmaStatus;

	/* ELzmaStatus is used only as output value for function call */

	/* ---------- Interfaces ---------- */

	/* There are 3 levels of interfaces:
		 1) Dictionary Interface
		 2) Buffer Interface
		 3) One Call Interface
	   You can select any of these interfaces, but don't mix functions from different
	   groups for same object. */

	/* There are two variants to allocate state for Dictionary Interface:
		 1) LzmaDec_Allocate / LzmaDec_Free
		 2) LzmaDec_AllocateProbs / LzmaDec_FreeProbs
	   You can use variant 2, if you set dictionary buffer manually.
	   For Buffer Interface you must always use variant 1.

	   LzmaDec_Allocate* can return:
	   SZ_OK
	   SZ_ERROR_MEM         - Memory allocation error
	   SZ_ERROR_UNSUPPORTED - Unsupported properties
	 */

	SRes LzmaDec_AllocateProbs(CLzmaDec * p, const Byte * props, unsigned propsSize, ISzAllocPtr alloc);
	void LzmaDec_FreeProbs(CLzmaDec * p, ISzAllocPtr alloc);
	SRes LzmaDec_Allocate(CLzmaDec * state, const Byte * prop, unsigned propsSize, ISzAllocPtr alloc);
	void LzmaDec_Free(CLzmaDec * state, ISzAllocPtr alloc);

	/* ---------- Dictionary Interface ---------- */

	/* You can use it, if you want to eliminate the overhead for data copying from
	   dictionary to some other external buffer.
	   You must work with CLzmaDec variables directly in this interface.

	   STEPS:
		 LzmaDec_Constr()
		 LzmaDec_Allocate()
		 for(each new stream)
		 {
		   LzmaDec_Init()
		   while (it needs more decompression)
		   {
			 LzmaDec_DecodeToDic()
			 use data from CLzmaDec::dic and update CLzmaDec::dicPos
		   }
		 }
		 LzmaDec_Free()
	 */

	/* LzmaDec_DecodeToDic

	   The decoding to internal dictionary buffer (CLzmaDec::dic).
	   You must manually update CLzmaDec::dicPos, if it reaches CLzmaDec::dicBufSize !!!

	   finishMode:
	   It has meaning only if the decoding reaches output limit (dicLimit).
	   LZMA_FINISH_ANY - Decode just dicLimit bytes.
	   LZMA_FINISH_END - Stream must be finished after dicLimit.

	   Returns:
	   SZ_OK
		status:
		  LZMA_STATUS_FINISHED_WITH_MARK
		  LZMA_STATUS_NOT_FINISHED
		  LZMA_STATUS_NEEDS_MORE_INPUT
		  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
	   SZ_ERROR_DATA - Data error
	 */
	SRes LzmaDec_DecodeToDic(CLzmaDec * p, SizeT dicLimit, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status);

	/* ---------- Buffer Interface ---------- */

	/* It's zlib-like interface.
	   See LzmaDec_DecodeToDic description for information about STEPS and return results,
	   but you must use LzmaDec_DecodeToBuf instead of LzmaDec_DecodeToDic and you don't need
	   to work with CLzmaDec variables manually.

	   finishMode:
	   It has meaning only if the decoding reaches output limit (*destLen).
	   LZMA_FINISH_ANY - Decode just destLen bytes.
	   LZMA_FINISH_END - Stream must be finished after (*destLen).
	 */
	SRes LzmaDec_DecodeToBuf(CLzmaDec * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status);

	/* ---------- One Call Interface ---------- */

	/* LzmaDecode

	   finishMode:
	   It has meaning only if the decoding reaches output limit (*destLen).
	   LZMA_FINISH_ANY - Decode just destLen bytes.
	   LZMA_FINISH_END - Stream must be finished after (*destLen).

	   Returns:
	   SZ_OK
		status:
		  LZMA_STATUS_FINISHED_WITH_MARK
		  LZMA_STATUS_NOT_FINISHED
		  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
	   SZ_ERROR_DATA - Data error
	   SZ_ERROR_MEM  - Memory allocation error
	   SZ_ERROR_UNSUPPORTED - Unsupported properties
	   SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).
	 */
	SRes LzmaDecode(Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen,
		const Byte * propData, unsigned propSize, ELzmaFinishMode finishMode, ELzmaStatus * status, ISzAllocPtr alloc);
EXTERN_C_END
//
//#include <Lzma2Dec.h>
EXTERN_C_BEGIN
	// 
	// State Interface
	// 
	typedef struct {
		CLzmaDec decoder;
		uint32 packSize;
		uint32 unpackSize;
		uint   state;
		Byte   control;
		Bool   needInitDic;
		Bool   needInitState;
		Bool   needInitProp;
	} CLzma2Dec;

	#define Lzma2Dec_Construct(p) LzmaDec_Construct(&(p)->decoder)
	#define Lzma2Dec_FreeProbs(p, alloc) LzmaDec_FreeProbs(&(p)->decoder, alloc);
	#define Lzma2Dec_Free(p, alloc) LzmaDec_Free(&(p)->decoder, alloc);

	SRes Lzma2Dec_AllocateProbs(CLzma2Dec * p, Byte prop, ISzAllocPtr alloc);
	SRes Lzma2Dec_Allocate(CLzma2Dec * p, Byte prop, ISzAllocPtr alloc);
	void Lzma2Dec_Init(CLzma2Dec * p);
	/*
	   finishMode:
	   It has meaning only if the decoding reaches output limit (*destLen or dicLimit).
	   LZMA_FINISH_ANY - use smallest number of input bytes
	   LZMA_FINISH_END - read EndOfStream marker after decoding

	   Returns:
	   SZ_OK
		status:
		  LZMA_STATUS_FINISHED_WITH_MARK
		  LZMA_STATUS_NOT_FINISHED
		  LZMA_STATUS_NEEDS_MORE_INPUT
	   SZ_ERROR_DATA - Data error
	 */

	SRes Lzma2Dec_DecodeToDic(CLzma2Dec * p, SizeT dicLimit, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status);
	SRes Lzma2Dec_DecodeToBuf(CLzma2Dec * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, ELzmaFinishMode finishMode, ELzmaStatus * status);

	/* ---------- One Call Interface ---------- */

	/*
	   finishMode:
	   It has meaning only if the decoding reaches output limit (*destLen).
	   LZMA_FINISH_ANY - use smallest number of input bytes
	   LZMA_FINISH_END - read EndOfStream marker after decoding

	   Returns:
	   SZ_OK
		status:
		  LZMA_STATUS_FINISHED_WITH_MARK
		  LZMA_STATUS_NOT_FINISHED
	   SZ_ERROR_DATA - Data error
	   SZ_ERROR_MEM  - Memory allocation error
	   SZ_ERROR_UNSUPPORTED - Unsupported properties
	   SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).
	 */
	SRes Lzma2Decode(Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen,
		Byte prop, ELzmaFinishMode finishMode, ELzmaStatus * status, ISzAllocPtr alloc);
EXTERN_C_END
//
//#include <LzmaEnc.h>
EXTERN_C_BEGIN
	#define LZMA_PROPS_SIZE 5

	typedef struct _CLzmaEncProps {
		int level; /* 0 <= level <= 9 */
		uint32 dictSize; // (1 << 12) <= dictSize <= (1 << 27) for 32-bit version
			// (1 << 12) <= dictSize <= (1 << 30) for 64-bit version
			// default = (1 << 24) 
		uint64 reduceSize; // estimated size of data that will be compressed. default = 0xFFFFFFFF.
			// Encoder uses this value to reduce dictionary size 
		int lc; /* 0 <= lc <= 8, default = 3 */
		int lp; /* 0 <= lp <= 4, default = 0 */
		int pb; /* 0 <= pb <= 4, default = 2 */
		int algo; /* 0 - fast, 1 - normal, default = 1 */
		int fb; /* 5 <= fb <= 273, default = 32 */
		int btMode; /* 0 - hashChain Mode, 1 - binTree mode - normal, default = 1 */
		int numHashBytes; /* 2, 3 or 4, default = 4 */
		uint32 mc; /* 1 <= mc <= (1 << 30), default = 32 */
		unsigned writeEndMark; /* 0 - do not write EOPM, 1 - write EOPM, default = 0 */
		int numThreads; /* 1 or 2, default = 2 */
	} CLzmaEncProps;

	void FASTCALL LzmaEncProps_Init(CLzmaEncProps * p);
	void FASTCALL LzmaEncProps_Normalize(CLzmaEncProps * p);
	uint32 LzmaEncProps_GetDictSize(const CLzmaEncProps * props2);
	//
	// CLzmaEncHandle Interface
	//
	/* LzmaEnc_* functions can return the following exit codes:
	   Returns:
	   SZ_OK           - OK
	   SZ_ERROR_MEM    - Memory allocation error
	   SZ_ERROR_PARAM  - Incorrect paramater in props
	   SZ_ERROR_WRITE  - Write callback error.
	   SZ_ERROR_PROGRESS - some break from progress callback
	   SZ_ERROR_THREAD - errors in multithreading functions (only for Mt version)
	 */
	typedef void * CLzmaEncHandle;

	CLzmaEncHandle LzmaEnc_Create(ISzAllocPtr alloc);
	void   LzmaEnc_Destroy(CLzmaEncHandle p, ISzAllocPtr alloc, ISzAllocPtr allocBig);
	SRes   LzmaEnc_SetProps(CLzmaEncHandle p, const CLzmaEncProps * props);
	SRes   LzmaEnc_WriteProperties(CLzmaEncHandle p, Byte * properties, SizeT * size);
	uint   LzmaEnc_IsWriteEndMark(CLzmaEncHandle p);
	SRes   LzmaEnc_Encode(CLzmaEncHandle p, ISeqOutStream * outStream, ISeqInStream * inStream, ICompressProgress * progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);
	SRes   LzmaEnc_MemEncode(CLzmaEncHandle p, Byte * dest, SizeT * destLen, const Byte * src, SizeT srcLen, int writeEndMark, ICompressProgress * progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);

	/* ---------- One Call Interface ---------- */

	/* LzmaEncode
	   Return code:
	   SZ_OK               - OK
	   SZ_ERROR_MEM        - Memory allocation error
	   SZ_ERROR_PARAM      - Incorrect paramater
	   SZ_ERROR_OUTPUT_EOF - output buffer overflow
	   SZ_ERROR_THREAD     - errors in multithreading functions (only for Mt version)
	 */

	SRes LzmaEncode(Byte * dest, SizeT * destLen, const Byte * src, SizeT srcLen,
		const CLzmaEncProps * props, Byte * propsEncoded, SizeT * propsSize, int writeEndMark,
		ICompressProgress * progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);
EXTERN_C_END
//
//#include <Lzma2Enc.h>
EXTERN_C_BEGIN
	typedef struct {
		CLzmaEncProps lzmaProps;
		size_t blockSize;
		int numBlockThreads;
		int numTotalThreads;
	} CLzma2EncProps;

	void Lzma2EncProps_Init(CLzma2EncProps * p);
	void Lzma2EncProps_Normalize(CLzma2EncProps * p);
	// 
	// CLzmaEnc2Handle Interface
	// 
	/* Lzma2Enc_* functions can return the following exit codes:
	   Returns:
	   SZ_OK           - OK
	   SZ_ERROR_MEM    - Memory allocation error
	   SZ_ERROR_PARAM  - Incorrect paramater in props
	   SZ_ERROR_WRITE  - Write callback error
	   SZ_ERROR_PROGRESS - some break from progress callback
	   SZ_ERROR_THREAD - errors in multithreading functions (only for Mt version)
	 */

	typedef void * CLzma2EncHandle;

	CLzma2EncHandle Lzma2Enc_Create(ISzAllocPtr alloc, ISzAllocPtr allocBig);
	void Lzma2Enc_Destroy(CLzma2EncHandle p);
	SRes Lzma2Enc_SetProps(CLzma2EncHandle p, const CLzma2EncProps * props);
	Byte Lzma2Enc_WriteProperties(CLzma2EncHandle p);
	SRes Lzma2Enc_Encode(CLzma2EncHandle p, ISeqOutStream * outStream, ISeqInStream * inStream, ICompressProgress * progress);
	// 
	// One Call Interface
	// 
	/* Lzma2Encode
	   Return code:
	   SZ_OK               - OK
	   SZ_ERROR_MEM        - Memory allocation error
	   SZ_ERROR_PARAM      - Incorrect paramater
	   SZ_ERROR_OUTPUT_EOF - output buffer overflow
	   SZ_ERROR_THREAD     - errors in multithreading functions (only for Mt version)
	 */
	/*
	   SRes Lzma2Encode(Byte *dest, SizeT *destLen, const Byte *src, SizeT srcLen,
		const CLzmaEncProps *props, Byte *propsEncoded, int writeEndMark,
		ICompressProgress *progress, ISzAllocPtr alloc, ISzAllocPtr allocBig);
	 */
EXTERN_C_END
//
//#include <Xz.h>
EXTERN_C_BEGIN
	//
	// Xz interface
	//
	#define XZ_ID_Subblock 1
	#define XZ_ID_Delta 3
	#define XZ_ID_X86 4
	#define XZ_ID_PPC 5
	#define XZ_ID_IA64 6
	#define XZ_ID_ARM 7
	#define XZ_ID_ARMT 8
	#define XZ_ID_SPARC 9
	#define XZ_ID_LZMA2 0x21

	// (static) uint FASTCALL Xz_ReadVarInt(const Byte * p, size_t maxSize, uint64 * value);
	// (static) uint FASTCALL Xz_WriteVarInt(Byte * buf, uint64 v);
	//
	// xz block
	//
	#define XZ_BLOCK_HEADER_SIZE_MAX 1024

	#define XZ_NUM_FILTERS_MAX 4
	#define XZ_BF_NUM_FILTERS_MASK 3
	#define XZ_BF_PACK_SIZE (1 << 6)
	#define XZ_BF_UNPACK_SIZE (1 << 7)

	#define XZ_FILTER_PROPS_SIZE_MAX 20

	typedef struct {
		uint64 id;
		uint32 propsSize;
		Byte props[XZ_FILTER_PROPS_SIZE_MAX];
	} CXzFilter;

	typedef struct {
		uint64 packSize;
		uint64 unpackSize;
		Byte flags;
		CXzFilter filters[XZ_NUM_FILTERS_MAX];
	} CXzBlock;

	#define XzBlock_GetNumFilters(p) (((p)->flags & XZ_BF_NUM_FILTERS_MASK) + 1)
	#define XzBlock_HasPackSize(p)   (((p)->flags & XZ_BF_PACK_SIZE) != 0)
	#define XzBlock_HasUnpackSize(p) (((p)->flags & XZ_BF_UNPACK_SIZE) != 0)

	SRes XzBlock_Parse(CXzBlock * p, const Byte * header);
	SRes XzBlock_ReadHeader(CXzBlock * p, ISeqInStream * inStream, Bool * isIndex, uint32 * headerSizeRes);
	//
	// xz stream
	//
	#define XZ_SIG_SIZE 6
	#define XZ_FOOTER_SIG_SIZE 2

	extern const Byte XZ_SIG[XZ_SIG_SIZE];
	extern const Byte XZ_FOOTER_SIG[XZ_FOOTER_SIG_SIZE];

	#define XZ_STREAM_FLAGS_SIZE 2
	#define XZ_STREAM_CRC_SIZE 4

	#define XZ_STREAM_HEADER_SIZE (XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE + XZ_STREAM_CRC_SIZE)
	#define XZ_STREAM_FOOTER_SIZE (XZ_FOOTER_SIG_SIZE + XZ_STREAM_FLAGS_SIZE + XZ_STREAM_CRC_SIZE + 4)

	#define XZ_CHECK_MASK 0xF
	#define XZ_CHECK_NO 0
	#define XZ_CHECK_CRC32 1
	#define XZ_CHECK_CRC64 4
	#define XZ_CHECK_SHA256 10

	typedef struct {
		uint   mode;
		uint32 crc;
		uint64 crc64;
		CSha256 sha;
	} CXzCheck;

	// (static) void   FASTCALL XzCheck_Init(CXzCheck * p, unsigned mode);
	// (static) void   FASTCALL XzCheck_Update(CXzCheck * p, const void * data, size_t size);
	// (static) int    FASTCALL XzCheck_Final(CXzCheck * p, Byte * digest);

	typedef uint16 CXzStreamFlags;

	#define XzFlags_IsSupported(f) ((f) <= XZ_CHECK_MASK)
	#define XzFlags_GetCheckType(f) ((f) & XZ_CHECK_MASK)
	#define XzFlags_HasDataCrc32(f) (Xz_GetCheckType(f) == XZ_CHECK_CRC32)
	// (static) uint   FASTCALL XzFlags_GetCheckSize(CXzStreamFlags f);

	SRes Xz_ParseHeader(CXzStreamFlags * p, const Byte * buf);
	SRes Xz_ReadHeader(CXzStreamFlags * p, ISeqInStream * inStream);

	typedef struct {
		uint64 unpackSize;
		uint64 totalSize;
	} CXzBlockSizes;

	typedef struct {
		CXzStreamFlags flags;
		size_t numBlocks;
		size_t numBlocksAllocated;
		CXzBlockSizes * blocks;
		uint64 startOffset;
	} CXzStream;

	// (static) void FASTCALL Xz_Construct(CXzStream * p);
	// (static) void FASTCALL Xz_Free(CXzStream * p, ISzAllocPtr alloc);

	#define XZ_SIZE_OVERFLOW (static_cast<uint64>(-1LL))

	// (static) uint64 FASTCALL Xz_GetUnpackSize(const CXzStream * p);
	// (static) uint64 FASTCALL Xz_GetPackSize(const CXzStream * p);

	typedef struct {
		size_t num;
		size_t numAllocated;
		CXzStream * streams;
	} CXzs;

	void Xzs_Construct(CXzs * p);
	void Xzs_Free(CXzs * p, ISzAllocPtr alloc);
	SRes Xzs_ReadBackward(CXzs * p, ILookInStream * inStream, int64 * startOffset, ICompressProgress * progress, ISzAllocPtr alloc);

	uint64 Xzs_GetNumBlocks(const CXzs * p);
	uint64 Xzs_GetUnpackSize(const CXzs * p);

	typedef enum {
		CODER_STATUS_NOT_SPECIFIED,         /* use main error code instead */
		CODER_STATUS_FINISHED_WITH_MARK,    /* stream was finished with end mark. */
		CODER_STATUS_NOT_FINISHED,          /* stream was not finished */
		CODER_STATUS_NEEDS_MORE_INPUT       /* you must provide more input bytes */
	} ECoderStatus;

	typedef enum {
		CODER_FINISH_ANY, /* finish at any point */
		CODER_FINISH_END /* block must be finished at the end */
	} ECoderFinishMode;

	typedef struct _IStateCoder {
		void * p;
		void (* Free)(void * p, ISzAllocPtr alloc);
		SRes (* SetProps)(void * p, const Byte * props, size_t propSize, ISzAllocPtr alloc);
		void (* Init)(void * p);
		SRes (* Code)(void * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, int srcWasFinished, ECoderFinishMode finishMode, int * wasFinished);
	} IStateCoder;

	#define MIXCODER_NUM_FILTERS_MAX 4

	typedef struct {
		ISzAllocPtr alloc;
		Byte * buf;
		unsigned numCoders;
		int finished[MIXCODER_NUM_FILTERS_MAX - 1];
		size_t pos[MIXCODER_NUM_FILTERS_MAX - 1];
		size_t size[MIXCODER_NUM_FILTERS_MAX - 1];
		uint64 ids[MIXCODER_NUM_FILTERS_MAX];
		IStateCoder coders[MIXCODER_NUM_FILTERS_MAX];
	} CMixCoder;

	// (static) void FASTCALL MixCoder_Construct(CMixCoder * p, ISzAllocPtr alloc);
	// (static) void FASTCALL MixCoder_Free(CMixCoder * p);
	// (static) void FASTCALL MixCoder_Init(CMixCoder * p);
	// (static) SRes FASTCALL MixCoder_SetFromMethod(CMixCoder * p, unsigned coderIndex, uint64 methodId);
	// (static) SRes FASTCALL MixCoder_Code(CMixCoder * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, int srcWasFinished, ECoderFinishMode finishMode, ECoderStatus * status);

	typedef enum {
		XZ_STATE_STREAM_HEADER,
		XZ_STATE_STREAM_INDEX,
		XZ_STATE_STREAM_INDEX_CRC,
		XZ_STATE_STREAM_FOOTER,
		XZ_STATE_STREAM_PADDING,
		XZ_STATE_BLOCK_HEADER,
		XZ_STATE_BLOCK,
		XZ_STATE_BLOCK_FOOTER
	} EXzState;

	typedef struct {
		EXzState state;
		uint32 pos;
		uint   alignPos;
		uint   indexPreSize;
		CXzStreamFlags streamFlags;
		uint32 blockHeaderSize;
		uint64 packSize;
		uint64 unpackSize;
		uint64 numBlocks;
		uint64 indexSize;
		uint64 indexPos;
		uint64 padSize;
		uint64 numStartedStreams;
		uint64 numFinishedStreams;
		uint64 numTotalBlocks;
		uint32 crc;
		CMixCoder decoder;
		CXzBlock block;
		CXzCheck check;
		CSha256 sha;
		Byte   shaDigest[SHA256_DIGEST_SIZE];
		Byte   buf[XZ_BLOCK_HEADER_SIZE_MAX];
	} CXzUnpacker;

	void XzUnpacker_Construct(CXzUnpacker * p, ISzAllocPtr alloc);
	void XzUnpacker_Init(CXzUnpacker * p);
	void XzUnpacker_Free(CXzUnpacker * p);
	/*
	   finishMode:
	   It has meaning only if the decoding reaches output limit (*destLen).
	   CODER_FINISH_ANY - use smallest number of input bytes
	   CODER_FINISH_END - read EndOfStream marker after decoding

	   Returns:
	   SZ_OK
		status:
		  CODER_STATUS_NOT_FINISHED,
		  CODER_STATUS_NEEDS_MORE_INPUT - maybe there are more xz streams,
										  call XzUnpacker_IsStreamWasFinished to check that current stream was finished
	   SZ_ERROR_MEM  - Memory allocation error
	   SZ_ERROR_DATA - Data error
	   SZ_ERROR_UNSUPPORTED - Unsupported method or method properties
	   SZ_ERROR_CRC  - CRC error
	   // SZ_ERROR_INPUT_EOF - It needs more bytes in input buffer (src).

	   SZ_ERROR_NO_ARCHIVE - the error with xz Stream Header with one of the following reasons:
		 - xz Stream Signature failure
		 - CRC32 of xz Stream Header is failed
		 - The size of Stream padding is not multiple of four bytes.
		It's possible to get that error, if xz stream was finished and the stream
		contains some another data. In that case you can call XzUnpacker_GetExtraSize()
		function to get real size of xz stream.
	 */
	SRes XzUnpacker_Code(CXzUnpacker * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, ECoderFinishMode finishMode, ECoderStatus * status);
	Bool FASTCALL XzUnpacker_IsStreamWasFinished(const CXzUnpacker * p);
	/*
	   Call XzUnpacker_GetExtraSize after XzUnpacker_Code function to detect real size of
	   xz stream in two cases:
	   XzUnpacker_Code() returns:
	   res == SZ_OK && status == CODER_STATUS_NEEDS_MORE_INPUT
	   res == SZ_ERROR_NO_ARCHIVE
	 */
	uint64 FASTCALL XzUnpacker_GetExtraSize(const CXzUnpacker * p);
EXTERN_C_END
//
//#include <XzCrc64.h>
EXTERN_C_BEGIN
	extern uint64 g_Crc64Table[];

	void FASTCALL Crc64GenerateTable(void);

	#define CRC64_INIT_VAL UINT64_CONST(0xFFFFFFFFFFFFFFFF)
	#define CRC64_GET_DIGEST(crc) ((crc) ^ CRC64_INIT_VAL)
	#define CRC64_UPDATE_BYTE(crc, b) (g_Crc64Table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

	uint64 FASTCALL Crc64Update(uint64 crc, const void *data, size_t size);
	uint64 FASTCALL Crc64Calc(const void *data, size_t size);
EXTERN_C_END
//
//#include <XzEnc.h>
EXTERN_C_BEGIN
	typedef struct {
		uint32 id;
		uint32 delta;
		uint32 ip;
		int ipDefined;
	} CXzFilterProps;

	void   FASTCALL XzFilterProps_Init(CXzFilterProps * p);

	typedef struct {
		const CLzma2EncProps * lzma2Props;
		const CXzFilterProps * filterProps;
		unsigned checkId;
	} CXzProps;

	void   FASTCALL XzProps_Init(CXzProps * p);
	SRes   Xz_Encode(ISeqOutStream * outStream, ISeqInStream * inStream, const CXzProps * props, ICompressProgress * progress);
	SRes   Xz_EncodeEmpty(ISeqOutStream * outStream);
EXTERN_C_END
//
//#include <VirtThread.h>
struct CVirtThread {
	~CVirtThread() 
	{
		WaitThreadFinish();
	}
	void WaitThreadFinish(); // call it in destructor of child class !
	WRes Create();
	void Start();
	virtual void Execute() = 0;
	void WaitExecuteFinish() { FinishedEvent.Lock(); }

	NWindows::NSynchronization::CAutoResetEvent StartEvent;
	NWindows::NSynchronization::CAutoResetEvent FinishedEvent;
	NWindows::CThread Thread;
	bool Exit;
};
//
//#include <FilePathAutoRename.h>
bool   AutoRenamePath(FString &fullProcessedPath);
//
//#include <Ppmd.h>
// This code is based on PPMd var.H (2001): Dmitry Shkarin : Public domain
EXTERN_C_BEGIN
	#ifdef MY_CPU_32BIT
		#define PPMD_32BIT
	#endif
	#define PPMD_INT_BITS 7
	#define PPMD_PERIOD_BITS 7
	#define PPMD_BIN_SCALE (1 << (PPMD_INT_BITS + PPMD_PERIOD_BITS))
	#define PPMD_GET_MEAN_SPEC(summ, shift, round) (((summ) + (1 << ((shift) - (round)))) >> (shift))
	#define PPMD_GET_MEAN(summ) PPMD_GET_MEAN_SPEC((summ), PPMD_PERIOD_BITS, 2)
	#define PPMD_UPDATE_PROB_0(prob) ((prob) + (1 << PPMD_INT_BITS) - PPMD_GET_MEAN(prob))
	#define PPMD_UPDATE_PROB_1(prob) ((prob) - PPMD_GET_MEAN(prob))
	#define PPMD_N1 4
	#define PPMD_N2 4
	#define PPMD_N3 4
	#define PPMD_N4 ((128 + 3 - 1 * PPMD_N1 - 2 * PPMD_N2 - 3 * PPMD_N3) / 4)
	#define PPMD_NUM_INDEXES (PPMD_N1 + PPMD_N2 + PPMD_N3 + PPMD_N4)

	#pragma pack(push, 1)
	/* Most compilers works OK here even without #pragma pack(push, 1), but some GCC compilers need it. */

	/* SEE-contexts for PPM-contexts with masked symbols */
	typedef struct {
		uint16 Summ; /* Freq */
		Byte Shift; /* Speed of Freq change; low Shift is for fast change */
		Byte Count; /* Count to next change of Shift */
	} CPpmd_See;

	#define Ppmd_See_Update(p)  if((p)->Shift < PPMD_PERIOD_BITS && --(p)->Count == 0) \
		{ (p)->Summ <<= 1; (p)->Count = (Byte)(3 << (p)->Shift++); }

	typedef struct {
		Byte Symbol;
		Byte Freq;
		uint16 SuccessorLow;
		uint16 SuccessorHigh;
	} CPpmd_State;

	#pragma pack(pop)

	#ifdef PPMD_32BIT
		typedef CPpmd_State * CPpmd_State_Ref;
		typedef void * CPpmd_Void_Ref;
		typedef Byte * CPpmd_Byte_Ref;
	#else
		typedef uint32 CPpmd_State_Ref;
		typedef uint32 CPpmd_Void_Ref;
		typedef uint32 CPpmd_Byte_Ref;
	#endif
	#define PPMD_SetAllBitsIn256Bytes(p) \
		{ size_t z; for(z = 0; z < 256 / sizeof(p[0]); z += 8) { p[z+7] = p[z+6] = p[z+5] = p[z+4] = p[z+3] = p[z+2] = p[z+1] = p[z+0] = ~(size_t)0; }}
EXTERN_C_END
//
//#include <Ppmd7.h>
// This code is based on PPMd var.H (2001): Dmitry Shkarin : Public domain 
//
// This code supports virtual RangeDecoder and includes the implementation
// of RangeCoder from 7z, instead of RangeCoder from original PPMd var.H.
// If you need the compatibility with original PPMd var.H, you can use external RangeDecoder */
EXTERN_C_BEGIN
	#define PPMD7_MIN_ORDER 2
	#define PPMD7_MAX_ORDER 64
	#define PPMD7_MIN_MEM_SIZE (1 << 11)
	#define PPMD7_MAX_MEM_SIZE (0xFFFFFFFF - 12 * 3)

	struct CPpmd7_Context_;

	#ifdef PPMD_32BIT
		typedef struct CPpmd7_Context_ * CPpmd7_Context_Ref;
	#else
		typedef uint32 CPpmd7_Context_Ref;
	#endif

	typedef struct CPpmd7_Context_ {
		uint16 NumStats;
		uint16 SummFreq;
		CPpmd_State_Ref Stats;
		CPpmd7_Context_Ref Suffix;
	} CPpmd7_Context;

	#define Ppmd7Context_OneState(p) ((CPpmd_State*)&(p)->SummFreq)

	typedef struct {
		CPpmd7_Context * MinContext;
		CPpmd7_Context * MaxContext;
		CPpmd_State * FoundState;
		uint   OrderFall;
		uint   InitEsc;
		uint   PrevSuccess;
		uint   MaxOrder;
		uint   HiBitsFlag;
		int32  RunLength; // must be 32-bit at least 
		int32  InitRL;    // must be 32-bit at least 
		uint32 Size;
		uint32 GlueCount;
		Byte * Base;
		Byte * LoUnit;
		Byte * HiUnit;
		Byte * Text;
		Byte * UnitsStart;
		uint32 AlignOffset;
		Byte   Indx2Units[PPMD_NUM_INDEXES];
		Byte   Units2Indx[128];
		CPpmd_Void_Ref FreeList[PPMD_NUM_INDEXES];
		Byte   NS2Indx[256];
		Byte   NS2BSIndx[256];
		Byte   HB2Flag[256];
		CPpmd_See DummySee;
		CPpmd_See See[25][16];
		uint16 BinSumm[128][64];
	} CPpmd7;

	void Ppmd7_Construct(CPpmd7 * p);
	Bool Ppmd7_Alloc(CPpmd7 * p, uint32 size, ISzAllocPtr alloc);
	void Ppmd7_Free(CPpmd7 * p, ISzAllocPtr alloc);
	void Ppmd7_Init(CPpmd7 * p, unsigned maxOrder);
	#define Ppmd7_WasAllocated(p) ((p)->Base != NULL)
	//
	// Internal Functions
	//
	extern const Byte PPMD7_kExpEscape[16];
	#ifdef PPMD_32BIT
		#define Ppmd7_GetPtr(p, ptr) (ptr)
		#define Ppmd7_GetContext(p, ptr) (ptr)
		#define Ppmd7_GetStats(p, ctx) ((ctx)->Stats)
	#else
		#define Ppmd7_GetPtr(p, offs) ((void *)((p)->Base + (offs)))
		#define Ppmd7_GetContext(p, offs) ((CPpmd7_Context*)Ppmd7_GetPtr((p), (offs)))
		#define Ppmd7_GetStats(p, ctx) ((CPpmd_State*)Ppmd7_GetPtr((p), ((ctx)->Stats)))
	#endif
	// (static) void FASTCALL Ppmd7_Update1(CPpmd7 * p);
	// (static) void FASTCALL Ppmd7_Update1_0(CPpmd7 * p);
	// (static) void FASTCALL Ppmd7_Update2(CPpmd7 * p);
	// (static) void FASTCALL Ppmd7_UpdateBin(CPpmd7 * p);

	#define Ppmd7_GetBinSumm(p) &p->BinSumm[(size_t)(uint)Ppmd7Context_OneState(p->MinContext)->Freq - 1][p->PrevSuccess + \
		p->NS2BSIndx[(size_t)Ppmd7_GetContext(p, p->MinContext->Suffix)->NumStats - 1] + \
		(p->HiBitsFlag = p->HB2Flag[p->FoundState->Symbol]) + \
		2 * p->HB2Flag[(uint)Ppmd7Context_OneState(p->MinContext)->Symbol] + ((p->RunLength >> 26) & 0x20)]

	CPpmd_See * Ppmd7_MakeEscFreq(CPpmd7 * p, unsigned numMasked, uint32 * scale);
	//
	// Decode
	//
	typedef struct IPpmd7_RangeDec IPpmd7_RangeDec;

	struct IPpmd7_RangeDec {
		uint32 (* GetThreshold)(const IPpmd7_RangeDec * p, uint32 total);
		void (* Decode)(const IPpmd7_RangeDec * p, uint32 start, uint32 size);
		uint32 (* DecodeBit)(const IPpmd7_RangeDec * p, uint32 size0);
	};

	typedef struct {
		IPpmd7_RangeDec vt;
		uint32 Range;
		uint32 Code;
		IByteIn * Stream;
	} CPpmd7z_RangeDec;

	void Ppmd7z_RangeDec_CreateVTable(CPpmd7z_RangeDec * p);
	Bool Ppmd7z_RangeDec_Init(CPpmd7z_RangeDec * p);
	#define Ppmd7z_RangeDec_IsFinishedOK(p) ((p)->Code == 0)

	int Ppmd7_DecodeSymbol(CPpmd7 * p, const IPpmd7_RangeDec * rc);
	//
	// Encode
	//
	typedef struct {
		uint64 Low;
		uint32 Range;
		Byte Cache;
		uint64 CacheSize;
		IByteOut * Stream;
	} CPpmd7z_RangeEnc;

	void Ppmd7z_RangeEnc_Init(CPpmd7z_RangeEnc * p);
	void Ppmd7z_RangeEnc_FlushData(CPpmd7z_RangeEnc * p);
	void Ppmd7_EncodeSymbol(CPpmd7 * p, CPpmd7z_RangeEnc * rc, int symbol);
EXTERN_C_END
//
//#include <Ppmd8.h>
// This code is based on: PPMd var.I (2002): Dmitry Shkarin : Public domain 
// Carryless rangecoder (1999): Dmitry Subbotin : Public domain 
EXTERN_C_BEGIN
	#define PPMD8_MIN_ORDER 2
	#define PPMD8_MAX_ORDER 16

	struct CPpmd8_Context_;

	#ifdef PPMD_32BIT
		typedef struct CPpmd8_Context_ * CPpmd8_Context_Ref;
	#else
		typedef uint32 CPpmd8_Context_Ref;
	#endif

	#pragma pack(push, 1)
	typedef struct CPpmd8_Context_ {
		Byte   NumStats;
		Byte   Flags;
		uint16 SummFreq;
		CPpmd_State_Ref Stats;
		CPpmd8_Context_Ref Suffix;
	} CPpmd8_Context;
	#pragma pack(pop)

	#define Ppmd8Context_OneState(p) ((CPpmd_State*)&(p)->SummFreq)
	// 
	// The BUG in Shkarin's code for FREEZE mode was fixed, but that fixed
	// code is not compatible with original code for some files compressed
	// in FREEZE mode. So we disable FREEZE mode support. */
	// 
	enum {
		PPMD8_RESTORE_METHOD_RESTART,
		PPMD8_RESTORE_METHOD_CUT_OFF
	  #ifdef PPMD8_FREEZE_SUPPORT
		, PPMD8_RESTORE_METHOD_FREEZE
	  #endif
	};

	typedef struct {
		CPpmd8_Context * MinContext, * MaxContext;
		CPpmd_State * FoundState;
		uint   OrderFall;
		uint   InitEsc;
		uint   PrevSuccess;
		uint   MaxOrder;
		int32  RunLength, InitRL; // must be 32-bit at least 
		uint32 Size;
		uint32 GlueCount;
		Byte * Base, * LoUnit, * HiUnit, * Text, * UnitsStart;
		uint32 AlignOffset;
		uint   RestoreMethod;
		//
		// Range Coder 
		//
		uint32 Range;
		uint32 Code;
		uint32 Low;
		union {
			IByteIn * In;
			IByteOut * Out;
		} Stream;

		Byte   Indx2Units[PPMD_NUM_INDEXES];
		Byte   Units2Indx[128];
		CPpmd_Void_Ref FreeList[PPMD_NUM_INDEXES];
		uint32 Stamps[PPMD_NUM_INDEXES];
		Byte   NS2BSIndx[256], NS2Indx[260];
		CPpmd_See DummySee, See[24][32];
		uint16 BinSumm[25][64];
	} CPpmd8;

	void Ppmd8_Construct(CPpmd8 * p);
	Bool Ppmd8_Alloc(CPpmd8 * p, uint32 size, ISzAllocPtr alloc);
	void Ppmd8_Free(CPpmd8 * p, ISzAllocPtr alloc);
	void Ppmd8_Init(CPpmd8 * p, unsigned maxOrder, unsigned restoreMethod);
	#define Ppmd8_WasAllocated(p) ((p)->Base != NULL)
	// 
	// Internal Functions
	// 
	extern const Byte PPMD8_kExpEscape[16];

	#ifdef PPMD_32BIT
		#define Ppmd8_GetPtr(p, ptr) (ptr)
		#define Ppmd8_GetContext(p, ptr) (ptr)
		#define Ppmd8_GetStats(p, ctx) ((ctx)->Stats)
	#else
		#define Ppmd8_GetPtr(p, offs) ((void *)((p)->Base + (offs)))
		#define Ppmd8_GetContext(p, offs) ((CPpmd8_Context*)Ppmd8_GetPtr((p), (offs)))
		#define Ppmd8_GetStats(p, ctx) ((CPpmd_State*)Ppmd8_GetPtr((p), ((ctx)->Stats)))
	#endif

	// (static) void FASTCALL Ppmd8_Update1(CPpmd8 * p);
	// (static) void FASTCALL Ppmd8_Update1_0(CPpmd8 * p);
	// (static) void FASTCALL Ppmd8_Update2(CPpmd8 * p);
	// (static) void FASTCALL Ppmd8_UpdateBin(CPpmd8 * p);

	#define Ppmd8_GetBinSumm(p) &p->BinSumm[p->NS2Indx[(size_t)Ppmd8Context_OneState(p->MinContext)->Freq - 1]][ \
		p->NS2BSIndx[Ppmd8_GetContext(p, p->MinContext->Suffix)->NumStats] + p->PrevSuccess + p->MinContext->Flags + ((p->RunLength >> 26) & 0x20)]
	// (static) CPpmd_See * FASTCALL Ppmd8_MakeEscFreq(CPpmd8 * p, uint numMasked, uint32 * scale);
	// 
	// Decode
	// 
	Bool   Ppmd8_RangeDec_Init(CPpmd8 * p);
	#define Ppmd8_RangeDec_IsFinishedOK(p) ((p)->Code == 0)
	int    FASTCALL Ppmd8_DecodeSymbol(CPpmd8 * p); /* returns: -1 as EndMarker, -2 as DataError */
	// 
	// Encode
	// 
	#define Ppmd8_RangeEnc_Init(p) { (p)->Low = 0; (p)->Range = 0xFFFFFFFF; }
	void Ppmd8_RangeEnc_FlushData(CPpmd8 * p);
	void Ppmd8_EncodeSymbol(CPpmd8 * p, int symbol); /* symbol = -1 means EndMarker */
EXTERN_C_END
//
//#include <Blake2.h>
EXTERN_C_BEGIN
	#define BLAKE2S_BLOCK_SIZE 64
	#define BLAKE2S_DIGEST_SIZE 32
	#define BLAKE2SP_PARALLEL_DEGREE 8

	typedef struct {
		uint32 h[8];
		uint32 t[2];
		uint32 f[2];
		Byte buf[BLAKE2S_BLOCK_SIZE];
		uint32 bufPos;
		uint32 lastNode_f1;
		uint32 dummy[2]; /* for sizeof(CBlake2s) alignment */
	} CBlake2s;
	// 
	// You need to xor CBlake2s::h[i] with input parameter block after Blake2s_Init0() 
	/*
	   void Blake2s_Init0(CBlake2s *p);
	   void Blake2s_Update(CBlake2s *p, const Byte *data, size_t size);
	   void Blake2s_Final(CBlake2s *p, Byte *digest);
	 */
	typedef struct {
		CBlake2s S[BLAKE2SP_PARALLEL_DEGREE];
		unsigned bufPos;
	} CBlake2sp;

	void Blake2sp_Init(CBlake2sp * p);
	void Blake2sp_Update(CBlake2sp * p, const Byte * data, size_t size);
	void Blake2sp_Final(CBlake2sp * p, Byte * digest);
EXTERN_C_END
//
//#include <TarHeader.h> <TarItem.h>
namespace NArchive {
	namespace NTar {
		namespace NFileHeader {
			const uint kRecordSize = 512;
			const uint kNameSize = 100;
			const uint kUserNameSize = 32;
			const uint kGroupNameSize = 32;
			const uint kPrefixSize = 155;

			const uint kUstarMagic_Offset = 257;
			/*
			   struct CHeader
			   {
			   char Name[kNameSize];
			   char Mode[8];
			   char UID[8];
			   char GID[8];
			   char Size[12];
			   char ModificationTime[12];
			   char CheckSum[8];
			   char LinkFlag;
			   char LinkName[kNameSize];
			   char Magic[8];
			   char UserName[kUserNameSize];
			   char GroupName[kGroupNameSize];
			   char DeviceMajor[8];
			   char DeviceMinor[8];
			   char Prefix[155];
			   };
			   union CRecord
			   {
			   CHeader Header;
			   Byte Padding[kRecordSize];
			   };
			 */

			namespace NLinkFlag {
				const char kOldNormal    = 0;       // Normal disk file, Unix compatible
				const char kNormal       = '0';     // Normal disk file
				const char kHardLink     = '1';     // Link to previously dumped file
				const char kSymLink      = '2';     // Symbolic link
				const char kCharacter    = '3';     // Character special file
				const char kBlock        = '4';     // Block special file
				const char kDirectory    = '5';     // Directory
				const char kFIFO = '6';     // FIFO special file
				const char kContiguous   = '7';     // Contiguous file
				const char kGnu_LongLink = 'K';
				const char kGnu_LongName = 'L';
				const char kSparse       = 'S';
				const char kDumpDir      = 'D'; /* GNUTYPE_DUMPDIR.
									   data: list of files created by the --incremental (-G) option
									   Each file name is preceded by either
									   - 'Y' (file should be in this archive)
									   - 'N' (file is a directory, or is not stored in the archive.)
									   Each file name is terminated by a null + an additional null after
									   the last file name. */
			}
			extern const char * const kLongLink;    //   = "././@LongLink";
			extern const char * const kLongLink2;   //   = "@LongLink";
			namespace NMagic {
				// extern const char * const kUsTar;  //  = "ustar"; // 5 chars
				// extern const char * const kGNUTar; //  = "GNUtar "; // 7 chars and a null
				// extern const char * const kEmpty;  //  = "\0\0\0\0\0\0\0\0"
				extern const char kUsTar_00[8];
			}
		}

		struct CSparseBlock {
			uint64 Offset;
			uint64 Size;
		};

		struct CItem {
			bool   IsSymLink() const;
			bool   IsHardLink() const;
			bool   IsSparse() const;
			uint64 GetUnpackSize() const;
			bool   IsPaxExtendedHeader() const;
			uint32 Get_Combined_Mode() const;
			uint32 Get_FileTypeMode_from_LinkFlag() const;
			bool   IsDir() const;
			bool   IsUstarMagic() const;
			uint64 GetPackSizeAligned() const;

			uint64 PackSize;
			uint64 Size;
			int64  MTime;
			uint32 Mode;
			uint32 UID;
			uint32 GID;
			uint32 DeviceMajor;
			uint32 DeviceMinor;
			AString Name;
			AString LinkName;
			AString User;
			AString Group;
			CRecordVector <CSparseBlock> SparseBlocks;
			char   Magic[8];
			char   LinkFlag;
			bool   DeviceMajorDefined;
			bool   DeviceMinorDefined;
			uint8  Reserve[1]; // @alignment
		};

		struct CItemEx : public CItem {
			uint64 GetDataPosition() const { return HeaderPos + HeaderSize; }
			uint64 GetFullSize() const { return HeaderSize + PackSize; }
			uint64 HeaderPos;
			uint   HeaderSize;
			bool   NameCouldBeReduced;
			bool   LinkNameCouldBeReduced;
			uint8  Reserve[2]; // @alignment
		};
	}
}
//
//#include <Lzx.h> <LzxDecoder.h>
namespace NCompress {
	namespace NLzx {
		const uint kBlockType_NumBits = 3;
		const uint kBlockType_Verbatim = 1;
		const uint kBlockType_Aligned = 2;
		const uint kBlockType_Uncompressed = 3;

		const uint kNumHuffmanBits = 16;
		const uint kNumReps = 3;

		const uint kNumLenSlots = 8;
		const uint kMatchMinLen = 2;
		const uint kNumLenSymbols = 249;
		const uint kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

		const uint kNumAlignLevelBits = 3;
		const uint kNumAlignBits = 3;
		const uint kAlignTableSize = 1 << kNumAlignBits;

		const uint kNumPosSlots = 50;
		const uint kNumPosLenSlots = kNumPosSlots * kNumLenSlots;

		const uint kMainTableSize = 256 + kNumPosLenSlots;
		const uint kLevelTableSize = 20;
		const uint kMaxTableSize = kMainTableSize;

		const uint kNumLevelBits = 4;

		const uint kLevelSym_Zero1 = 17;
		const uint kLevelSym_Zero2 = 18;
		const uint kLevelSym_Same = 19;

		const uint kLevelSym_Zero1_Start = 4;
		const uint kLevelSym_Zero1_NumBits = 4;

		const uint kLevelSym_Zero2_Start = kLevelSym_Zero1_Start + (1 << kLevelSym_Zero1_NumBits);
		const uint kLevelSym_Zero2_NumBits = 5;

		const uint kLevelSym_Same_NumBits = 1;
		const uint kLevelSym_Same_Start = 4;
 
		const uint kNumDictBits_Min = 15;
		const uint kNumDictBits_Max = 21;
		const uint32 kDictSize_Max = (uint32)1 << kNumDictBits_Max;

		const uint kNumLinearPosSlotBits = 17;
		const uint kNumPowerPosSlots = 38;

		class CBitDecoder {
			unsigned _bitPos;
			uint32 _value;
			const Byte * _buf;
			const Byte * _bufLim;
			uint32 _extraSize;
		public:
			void Init(const Byte * data, size_t size);
			size_t GetRem() const;
			bool WasExtraReadError_Fast() const;
			bool WasFinishedOK() const;
			void NormalizeSmall();
			void NormalizeBig();
			uint32 FASTCALL GetValue(uint numBits) const;
			void   FASTCALL MovePos(uint numBits);
			uint32 FASTCALL ReadBitsSmall(uint numBits);
			uint32 FASTCALL ReadBitsBig(uint numBits);
			bool PrepareUncompressed();
			uint32 ReadUInt32();
			void CopyTo(Byte * dest, size_t size);
			bool IsOneDirectByteLeft() const;
			Byte DirectReadByte();
		};

		class CDecoder : public IUnknown, public CMyUnknownImp {
		private:
			CBitDecoder _bitStream;
			Byte * _win;
			uint32 _pos;
			uint32 _winSize;
			bool _overDict;
			bool _isUncompressedBlock;
			bool _skipByte;
			unsigned _numAlignBits;
			uint32 _reps[kNumReps];
			uint32 _numPosLenSlots;
			uint32 _unpackBlockSize;
		public:
			bool KeepHistoryForNext;
			bool NeedAlloc;
		private:
			bool _keepHistory;
			bool _wimMode;
			unsigned _numDictBits;
			uint32 _writePos;
			Byte * _x86_buf;
			uint32 _x86_translationSize;
			uint32 _x86_processedSize;
			Byte * _unpackedData;
			NHuffman::CDecoder <kNumHuffmanBits, kMainTableSize> _mainDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kNumLenSymbols> _lenDecoder;
			NHuffman::CDecoder7b <kAlignTableSize> _alignDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kLevelTableSize, 7> _levelDecoder;
			Byte _mainLevels[kMainTableSize];
			Byte _lenLevels[kNumLenSymbols];

			HRESULT Flush();
			uint32 ReadBits(uint numBits);
			bool ReadTable(Byte * levels, unsigned numSymbols);
			bool ReadTables();
			HRESULT CodeSpec(uint32 size);
			HRESULT SetParams2(unsigned numDictBits);
		public:
			CDecoder(bool wimMode = false);
			~CDecoder();
			MY_UNKNOWN_IMP
			HRESULT SetExternalWindow(Byte * win, unsigned numDictBits)
			{
				NeedAlloc = false;
				_win = win;
				_winSize = (uint32)1 << numDictBits;
				return SetParams2(numDictBits);
			}
			void SetKeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
			HRESULT SetParams_and_Alloc(unsigned numDictBits);
			HRESULT Code(const Byte * inData, size_t inSize, uint32 outSize);
			bool WasBlockFinished() const { return _unpackBlockSize == 0; }
			const Byte * GetUnpackData() const { return _unpackedData; }
			const uint32 GetUnpackSize() const { return _pos - _writePos; }
		};
	}
}
//
//#include <MtCoder.h>
EXTERN_C_BEGIN

typedef struct {
	CThread thread;
	CAutoResetEvent startEvent;
	CAutoResetEvent finishedEvent;
	int stop;
	THREAD_FUNC_TYPE func;
	LPVOID param;
	THREAD_FUNC_RET_TYPE res;
} CLoopThread;

void LoopThread_Construct(CLoopThread * p);
void LoopThread_Close(CLoopThread * p);
WRes LoopThread_Create(CLoopThread * p);
WRes LoopThread_StopAndWait(CLoopThread * p);
WRes LoopThread_StartSubThread(CLoopThread * p);
WRes LoopThread_WaitSubThread(CLoopThread * p);

#ifndef _7ZIP_ST
	#define NUM_MT_CODER_THREADS_MAX 32
#else
	#define NUM_MT_CODER_THREADS_MAX 1
#endif

typedef struct {
	uint64 totalInSize;
	uint64 totalOutSize;
	ICompressProgress * progress;
	SRes res;
	CCriticalSection cs;
	uint64 inSizes[NUM_MT_CODER_THREADS_MAX];
	uint64 outSizes[NUM_MT_CODER_THREADS_MAX];
} CMtProgress;

SRes MtProgress_Set(CMtProgress * p, unsigned index, uint64 inSize, uint64 outSize);

struct _CMtCoder;

typedef struct {
	struct _CMtCoder * mtCoder;
	Byte * outBuf;
	size_t outBufSize;
	Byte * inBuf;
	size_t inBufSize;
	uint   index;
	CLoopThread thread;
	Bool stopReading;
	Bool stopWriting;
	CAutoResetEvent canRead;
	CAutoResetEvent canWrite;
} CMtThread;

typedef struct IMtCoderCallback IMtCoderCallback;
struct IMtCoderCallback {
	SRes (* Code)(const IMtCoderCallback * p, unsigned index, Byte * dest, size_t * destSize, const Byte * src, size_t srcSize, int finished);
};

#define IMtCoderCallback_Code(p, index, dest, destSize, src, srcSize, finished) (p)->Code(p, index, dest, destSize, src, srcSize, finished)

typedef struct _CMtCoder {
	size_t blockSize;
	size_t destBlockSize;
	uint   numThreads;
	ISeqInStream * inStream;
	ISeqOutStream * outStream;
	ICompressProgress * progress;
	ISzAllocPtr alloc;
	IMtCoderCallback * mtCallback;
	CCriticalSection cs;
	SRes res;
	CMtProgress mtProgress;
	CMtThread threads[NUM_MT_CODER_THREADS_MAX];
} CMtCoder;

void MtCoder_Construct(CMtCoder* p);
void MtCoder_Destruct(CMtCoder* p);
SRes MtCoder_Code(CMtCoder * p);

EXTERN_C_END
//
//#include <PercentPrinter.h>
struct CPercentPrinterState {
	CPercentPrinterState();
	void ClearCurState();

	uint64 Completed;
	uint64 Total;
	uint64 Files;
	AString Command;
	UString FileName;
};

class CPercentPrinter : public CPercentPrinterState {
public:
	CPercentPrinter(uint32 tickStep = 200);
	~CPercentPrinter();
	void   ClosePrint(bool needFlush);
	void   Print();

	CStdOutStream * _so;
	bool   NeedFlush;
	uint8  Reserve[3]; // @alignment
	uint   MaxLen;
private:
	void   GetPercents();

	uint32 _tickStep;
	DWORD _prevTick;
	AString _s;
	AString _printedString;
	AString _temp;
	UString _tempU;
	CPercentPrinterState _printedState;
	AString _printedPercents;
};
//
//#include <ExtractMode.h>
namespace NExtract {
	namespace NPathMode {
		enum EEnum {
			kFullPaths,
			kCurPaths,
			kNoPaths,
			kAbsPaths,
			kNoPathsAlt // alt streams must be extracted without name of base file
		};
	}
	namespace NOverwriteMode {
		enum EEnum {
			kAsk,
			kOverwrite,
			kSkip,
			kRename,
			kRenameExisting
		};
	}
}
//
//#include <RarHeader.h> <RarItem.h> <RarVol.h>
namespace NArchive {
	namespace NRar {
		namespace NHeader {
			const uint kMarkerSize = 7;
			const uint kArchiveSolid = 0x1;

			namespace NBlockType {
				enum EBlockType {
					kMarker = 0x72,
					kArchiveHeader,
					kFileHeader,
					kCommentHeader,
					kOldAuthenticity,
					kOldSubBlock,
					kRecoveryRecord,
					kAuthenticity,
					kSubBlock,
					kEndOfArchive
				};
			}
			namespace NArchive {
				const uint16 kVolume  = 1;
				const uint16 kComment = 2;
				const uint16 kLock    = 4;
				const uint16 kSolid   = 8;
				const uint16 kNewVolName = 0x10;   // ('volname.partN.rar')
				const uint16 kAuthenticity  = 0x20;
				const uint16 kRecovery = 0x40;
				const uint16 kBlockEncryption  = 0x80;
				const uint16 kFirstVolume = 0x100;   // (set only by RAR 3.0 and later)
				// const uint16 kEncryptVer = 0x200; // RAR 3.6 : that feature was discarded by origial RAR
				const uint16 kEndOfArc_Flags_NextVol   = 1;
				const uint16 kEndOfArc_Flags_DataCRC   = 2;
				const uint16 kEndOfArc_Flags_RevSpace  = 4;
				const uint16 kEndOfArc_Flags_VolNumber = 8;
				const uint kHeaderSizeMin = 7;
				const uint kArchiveHeaderSize = 13;
				const uint kBlockHeadersAreEncrypted = 0x80;
			}
			namespace NFile {
				const uint kSplitBefore = 1 << 0;
				const uint kSplitAfter  = 1 << 1;
				const uint kEncrypted   = 1 << 2;
				const uint kComment     = 1 << 3;
				const uint kSolid       = 1 << 4;

				const uint kDictBitStart     = 5;
				const uint kNumDictBits  = 3;
				const uint kDictMask = (1 << kNumDictBits) - 1;
				const uint kDictDirectoryValue  = 0x7;

				const uint kSize64Bits    = 1 << 8;
				const uint kUnicodeName   = 1 << 9;
				const uint kSalt  = 1 << 10;
				const uint kOldVersion    = 1 << 11;
				const uint kExtTime       = 1 << 12;
				// const uint kExtFlags      = 1 << 13;
				// const uint kSkipIfUnknown = 1 << 14;

				const uint kLongBlock    = 1 << 15;
				/*
				   struct CBlock {
				   // uint16 HeadCRC;
				   // Byte Type;
				   // uint16 Flags;
				   // uint16 HeadSize;
				   uint32 PackSize;
				   uint32 UnPackSize;
				   Byte HostOS;
				   uint32 FileCRC;
				   uint32 Time;
				   Byte UnPackVersion;
				   Byte Method;
				   uint16 NameSize;
				   uint32 Attributes;
				   };
				 */

				/*
				   struct CBlock32
				   {
				   uint16 HeadCRC;
				   Byte Type;
				   uint16 Flags;
				   uint16 HeadSize;
				   uint32 PackSize;
				   uint32 UnPackSize;
				   Byte HostOS;
				   uint32 FileCRC;
				   uint32 Time;
				   Byte UnPackVersion;
				   Byte Method;
				   uint16 NameSize;
				   uint32 Attributes;
				   uint16 GetRealCRC(const void *aName, uint32 aNameSize,
					  bool anExtraDataDefined = false, Byte *anExtraData = 0) const;
				   };
				   struct CBlock64
				   {
				   uint16 HeadCRC;
				   Byte Type;
				   uint16 Flags;
				   uint16 HeadSize;
				   uint32 PackSizeLow;
				   uint32 UnPackSizeLow;
				   Byte HostOS;
				   uint32 FileCRC;
				   uint32 Time;
				   Byte UnPackVersion;
				   Byte Method;
				   uint16 NameSize;
				   uint32 Attributes;
				   uint32 PackSizeHigh;
				   uint32 UnPackSizeHigh;
				   uint16 GetRealCRC(const void *aName, uint32 aNameSize) const;
				   };
				 */

				const uint kLabelFileAttribute    = 0x08;
				const uint kWinFileDirectoryAttributeMask = 0x10;

				enum CHostOS {
					kHostMSDOS = 0,
					kHostOS2   = 1,
					kHostWin32 = 2,
					kHostUnix  = 3,
					kHostMacOS = 4,
					kHostBeOS  = 5
				};
			}
			namespace NBlock {
				const uint16 kLongBlock = 1 << 15;
				struct CBlock {
					uint16 CRC;
					Byte Type;
					uint16 Flags;
					uint16 HeadSize;
					//  uint32 DataSize;
				};
			}
			/*
			   struct CSubBlock
			   {
			   uint16 HeadCRC;
			   Byte HeadType;
			   uint16 Flags;
			   uint16 HeadSize;
			   uint32 DataSize;
			   uint16 SubType;
			   Byte Level; // Reserved : Must be 0
			   };

			   struct CCommentBlock
			   {
			   uint16 HeadCRC;
			   Byte HeadType;
			   uint16 Flags;
			   uint16 HeadSize;
			   uint16 UnpSize;
			   Byte UnpVer;
			   Byte Method;
			   uint16 CommCRC;
			   };


			   struct CProtectHeader
			   {
			   uint16 HeadCRC;
			   Byte HeadType;
			   uint16 Flags;
			   uint16 HeadSize;
			   uint32 DataSize;
			   Byte Version;
			   uint16 RecSectors;
			   uint32 TotalBlocks;
			   Byte Mark[8];
			   };
			 */
		}
		
		struct CRarTime {
			uint32 DosTime;
			Byte LowSecond;
			Byte SubTime[3];
		};

		struct CItem {
			uint64 Size;
			uint64 PackSize;
			CRarTime CTime;
			CRarTime ATime;
			CRarTime MTime;
			uint32 FileCRC;
			uint32 Attrib;
			uint16 Flags;
			Byte HostOS;
			Byte UnPackVersion;
			Byte Method;
			bool CTimeDefined;
			bool ATimeDefined;
			AString Name;
			UString UnicodeName;
			Byte Salt[8];

			bool Is_Size_Defined() const { return Size != static_cast<uint64>(-1LL); }
			bool IsEncrypted()   const { return (Flags & NHeader::NFile::kEncrypted) != 0; }
			bool IsSolid() const { return (Flags & NHeader::NFile::kSolid) != 0; }
			bool IsCommented()   const { return (Flags & NHeader::NFile::kComment) != 0; }
			bool IsSplitBefore() const { return (Flags & NHeader::NFile::kSplitBefore) != 0; }
			bool IsSplitAfter()  const { return (Flags & NHeader::NFile::kSplitAfter) != 0; }
			bool HasSalt() const { return (Flags & NHeader::NFile::kSalt) != 0; }
			bool HasExtTime()    const { return (Flags & NHeader::NFile::kExtTime) != 0; }
			bool HasUnicodeName() const { return (Flags & NHeader::NFile::kUnicodeName) != 0; }
			bool IsOldVersion()  const { return (Flags & NHeader::NFile::kOldVersion) != 0; }
			uint32 GetDictSize() const { return (Flags >> NHeader::NFile::kDictBitStart) & NHeader::NFile::kDictMask; }
			bool IsDir() const;
			bool IgnoreItem() const;
			uint32 GetWinAttrib() const;

			uint64 Position;
			unsigned MainPartSize;
			uint16 CommentSize;
			uint16 AlignSize;

			// int BaseFileIndex;
			// bool IsAltStream;
			UString GetName() const
			{
				if(( /* IsAltStream || */ HasUnicodeName()) && !UnicodeName.IsEmpty())
					return UnicodeName;
				return MultiByteToUnicodeString(Name, CP_OEMCP);
			}
			void Clear()
			{
				CTimeDefined = false;
				ATimeDefined = false;
				Name.Empty();
				UnicodeName.Empty();
				// IsAltStream = false;
				// BaseFileIndex = -1;
			}
			CItem() 
			{
				Clear();
			}
			uint64 GetFullSize()  const { return MainPartSize + CommentSize + AlignSize + PackSize; }
			//  DWORD GetHeaderWithCommentSize()  const { return MainPartSize + CommentSize; }
			uint64 GetCommentPosition() const { return Position + MainPartSize; }
			uint64 GetDataPosition()    const { return GetCommentPosition() + CommentSize + AlignSize; }
		};

		inline bool IsDigit(wchar_t c)
		{
			return c >= L'0' && c <= L'9';
		}
		class CVolumeName {
			bool _needChangeForNext;
			UString _before;
			UString _changed;
			UString _after;
		public:
			CVolumeName() : _needChangeForNext(true) 
			{
			}
			bool InitName(const UString &name, bool newStyle = true)
			{
				_needChangeForNext = true;
				_after.Empty();
				UString base = name;
				int dotPos = name.ReverseFind_Dot();

				if(dotPos >= 0) {
					const UString ext = name.Ptr(dotPos + 1);
					if(ext.IsEqualTo_Ascii_NoCase("rar")) {
						_after = name.Ptr(dotPos);
						base.DeleteFrom(dotPos);
					}
					else if(ext.IsEqualTo_Ascii_NoCase("exe")) {
						_after = ".rar";
						base.DeleteFrom(dotPos);
					}
					else if(!newStyle) {
						if(ext.IsEqualTo_Ascii_NoCase("000") || ext.IsEqualTo_Ascii_NoCase("001") || ext.IsEqualTo_Ascii_NoCase("r00") || ext.IsEqualTo_Ascii_NoCase("r01")) {
							_changed = ext;
							_before = name.Left(dotPos + 1);
							return true;
						}
					}
				}
				if(newStyle) {
					unsigned i = base.Len();
					for(; i != 0; i--)
						if(!IsDigit(base[i - 1]))
							break;
					if(i != base.Len()) {
						_before = base.Left(i);
						_changed = base.Ptr(i);
						return true;
					}
				}
				_after.Empty();
				_before = base;
				_before += '.';
				_changed = "r00";
				_needChangeForNext = false;
				return true;
			}
			/*
			   void MakeBeforeFirstName()
			   {
			   uint len = _changed.Len();
			   _changed.Empty();
			   for(unsigned i = 0; i < len; i++)
				_changed += L'0';
			   }
			 */

			UString GetNextName()
			{
				if(_needChangeForNext) {
					unsigned i = _changed.Len();
					if(i == 0)
						return UString();
					for(;; ) {
						wchar_t c = _changed[--i];
						if(c == L'9') {
							c = L'0';
							_changed.ReplaceOneCharAtPos(i, c);
							if(i == 0) {
								_changed.InsertAtFront(L'1');
								break;
							}
							continue;
						}
						c++;
						_changed.ReplaceOneCharAtPos(i, c);
						break;
					}
				}
				_needChangeForNext = true;
				return _before + _changed + _after;
			}
		};
	}
}
//
//#include <Rar3Vm.h>
#define RARVM_STANDARD_FILTERS
// #define RARVM_VM_ENABLE

namespace NCompress {
	namespace NRar3 {
		class CMemBitDecoder {
			const Byte * _data;
			uint32 _bitSize;
			uint32 _bitPos;
		public:
			void Init(const Byte * data, uint32 byteSize)
			{
				_data = data;
				_bitSize = (byteSize << 3);
				_bitPos = 0;
			}

			uint32 ReadBits(uint numBits);
			uint32 ReadBit();
			bool Avail() const { return (_bitPos < _bitSize); }
			uint32 ReadEncodedUInt32();
		};

		namespace NVm {
			inline uint32 GetValue32(const void * addr) { return GetUi32(addr); }
			inline void SetValue32(void * addr, uint32 value) { SetUi32(addr, value); }

			const uint kNumRegBits = 3;
			const uint32 kNumRegs = 1 << kNumRegBits;
			const uint32 kNumGpRegs = kNumRegs - 1;
			const uint32 kSpaceSize = 0x40000;
			const uint32 kSpaceMask = kSpaceSize - 1;
			const uint32 kGlobalOffset = 0x3C000;
			const uint32 kGlobalSize = 0x2000;
			const uint32 kFixedGlobalSize = 64;

			namespace NGlobalOffset {
				const uint32 kBlockSize = 0x1C;
				const uint32 kBlockPos  = 0x20;
				const uint32 kExecCount = 0x2C;
				const uint32 kGlobalMemOutSize = 0x30;
			}
#ifdef RARVM_VM_ENABLE
			enum ECommand {
				CMD_MOV,  CMD_CMP,  CMD_ADD,  CMD_SUB,  CMD_JZ,   CMD_JNZ,  CMD_INC,  CMD_DEC,
				CMD_JMP,  CMD_XOR,  CMD_AND,  CMD_OR,   CMD_TEST, CMD_JS,   CMD_JNS,  CMD_JB,
				CMD_JBE,  CMD_JA,   CMD_JAE,  CMD_PUSH, CMD_POP,  CMD_CALL, CMD_RET,  CMD_NOT,
				CMD_SHL,  CMD_SHR,  CMD_SAR,  CMD_NEG,  CMD_PUSHA, CMD_POPA, CMD_PUSHF, CMD_POPF,
				CMD_MOVZX, CMD_MOVSX, CMD_XCHG, CMD_MUL,  CMD_DIV,  CMD_ADC,  CMD_SBB,  CMD_PRINT,

				CMD_MOVB, CMD_CMPB, CMD_ADDB, CMD_SUBB, CMD_INCB, CMD_DECB,
				CMD_XORB, CMD_ANDB, CMD_ORB,  CMD_TESTB, CMD_NEGB,
				CMD_SHLB, CMD_SHRB, CMD_SARB, CMD_MULB
			};

			enum EOpType {OP_TYPE_REG, OP_TYPE_INT, OP_TYPE_REGMEM, OP_TYPE_NONE};

			// Addr in COperand object can link (point) to CVm object!!!

			struct COperand {
				EOpType Type;
				uint32 Data;
				uint32 Base;
				COperand() : Type(OP_TYPE_NONE), Data(0), Base(0) 
				{
				}
			};

			struct CCommand {
				ECommand OpCode;
				bool ByteMode;
				COperand Op1, Op2;
			};
#endif
			struct CBlockRef {
				uint32 Offset;
				uint32 Size;
			};

			class CProgram {
			  #ifdef RARVM_VM_ENABLE
				void ReadProgram(const Byte * code, uint32 codeSize);
			public:
				CRecordVector<CCommand> Commands;
			  #endif
			public:
			  #ifdef RARVM_STANDARD_FILTERS
				int StandardFilterIndex;
			  #endif
				bool IsSupported;
				CRecordVector<Byte> StaticData;
				bool PrepareProgram(const Byte * code, uint32 codeSize);
			};

			struct CProgramInitState {
				uint32 InitR[kNumGpRegs];
				CRecordVector<Byte> GlobalData;
				void AllocateEmptyFixedGlobal()
				{
					GlobalData.ClearAndSetSize(NVm::kFixedGlobalSize);
					memzero(&GlobalData[0], NVm::kFixedGlobalSize);
				}
			};

			class CVm {
				static uint32 GetValue(bool byteMode, const void * addr) { return byteMode ? *(const Byte *)addr : GetUi32(addr); }
				static void SetValue(bool byteMode, void * addr, uint32 value)
				{
					if(byteMode)
						*(Byte *)addr = (Byte)value;
					else
						SetUi32(addr, value);
				}
				uint32 GetFixedGlobalValue32(uint32 globalOffset) { return GetValue(false, &Mem[kGlobalOffset + globalOffset]); }
				void SetBlockSize(uint32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockSize], v); }
				void SetBlockPos(uint32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockPos], v); }
			public:
				static void SetValue(void * addr, uint32 value) { SetValue(false, addr, value); }
			private:
			  #ifdef RARVM_VM_ENABLE
				uint32 GetOperand32(const COperand * op) const;
				void SetOperand32(const COperand * op, uint32 val);
				Byte GetOperand8(const COperand * op) const;
				void SetOperand8(const COperand * op, Byte val);
				uint32 GetOperand(bool byteMode, const COperand * op) const;
				void SetOperand(bool byteMode, const COperand * op, uint32 val);
				bool ExecuteCode(const CProgram * prg);
			  #endif
			  #ifdef RARVM_STANDARD_FILTERS
				void ExecuteStandardFilter(unsigned filterIndex);
			  #endif
				Byte *Mem;
				uint32 R[kNumRegs + 1]; // R[kNumRegs] = 0 always (speed optimization)
				uint32 Flags;
			public:
				CVm();
				~CVm();
				bool Create();
				void SetMemory(uint32 pos, const Byte * data, uint32 dataSize);
				bool Execute(CProgram * prg, const CProgramInitState * initState, CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData);
				const Byte * GetDataPointer(uint32 offset) const { return Mem + offset; }
			};
			//#endif
		}
	}
}
//
//#include <InOutTempBuffer.h>

class CInOutTempBuffer {
public:
	CInOutTempBuffer();
	~CInOutTempBuffer();
	void Create();
	void InitWriting();
	bool Write(const void * data, uint32 size);
	HRESULT WriteToStream(ISequentialOutStream * stream);
	uint64 GetDataSize() const { return _size; }
private:
	NWindows::NFile::NDir::CTempFile _tempFile;
	NWindows::NFile::NIO::COutFile _outFile;
	Byte * _buf;
	size_t _bufPos;
	uint64 _size;
	uint32 _crc;
	bool _tempFileCreated;
	bool WriteToFile(const void * data, uint32 size);
};
/*
	class CSequentialOutTempBufferImp : public ISequentialOutStream, public CMyUnknownImp {
		CInOutTempBuffer *_buf;
	public:
		void Init(CInOutTempBuffer *buffer) { _buf = buffer; }
		MY_UNKNOWN_IMP
		STDMETHOD(Write)(const void *data, uint32 size, uint32 *processedSize);
	};
 */
//
//#include <ExitCode.h>
namespace NExitCode {
	enum EEnum {
		kSuccess       = 0, // Successful operation
		kWarning       = 1, // Non fatal error(s) occurred
		kFatalError    = 2, // A fatal error occurred
		// kCRCError      = 3,     // A CRC error occurred when unpacking
		// kLockedArchive = 4,     // Attempt to modify an archive previously locked
		// kWriteError    = 5,     // Write to disk error
		// kOpenError     = 6,     // Open file error
		kUserError     = 7, // Command line option error
		kMemoryError   = 8, // Not enough memory for operation
		// kCreateFileError = 9,     // Create file error

		kUserBreak     = 255 // User stopped the process
	};
}
//
enum {
	k_HashCalc_Index_Current,
	k_HashCalc_Index_DataSum,
	k_HashCalc_Index_NamesSum,
	k_HashCalc_Index_StreamsSum
};

enum EArcNameMode {
	k_ArcNameMode_Smart,
	k_ArcNameMode_Exact,
	k_ArcNameMode_Add,
};

enum {
	k_OutStream_disabled = 0,
	k_OutStream_stdout = 1,
	k_OutStream_stderr = 2
};

namespace NCoderPropID {
	enum EEnum {
		kDefaultProp = 0,
		kDictionarySize,
		kUsedMemorySize,
		kOrder,
		kBlockSize,
		kPosStateBits,
		kLitContextBits,
		kLitPosBits,
		kNumFastBytes,
		kMatchFinder,
		kMatchFinderCycles,
		kNumPasses,
		kAlgorithm,
		kNumThreads,
		kEndMarker,
		kLevel,
		kReduceSize // estimated size of data that will be compressed. Encoder can use this value to reduce dictionary size.
	};
}
namespace NMethodPropID {
	enum EEnum {
		kID,
		kName,
		kDecoder,
		kEncoder,
		kPackStreams,
		kUnpackStreams,
		kDescription,
		kDecoderIsAssigned,
		kEncoderIsAssigned,
		kDigestSize
	};
}
namespace NFileTimeType {
	enum EEnum {
		kWindows,
		kUnix,
		kDOS
	};
}
namespace NParentType {
	enum {
		kDir = 0,
		kAltStream
	};
};
namespace NUpdateNotifyOp {
	enum {
		kAdd = 0,
		kUpdate,
		kAnalyze,
		kReplicate,
		kRepack,
		kSkip,
		kDelete,
		kHeader
		// kNumDefined
	};
};
namespace NUserAnswerMode {
	enum EEnum {
		kYes,
		kNo,
		kYesAll,
		kNoAll,
		kAutoRenameAll,
		kQuit,
		kEof,
		kError
	};
}
namespace NOverwriteAnswer {
	enum EEnum {
		kYes,
		kYesToAll,
		kNo,
		kNoToAll,
		kAutoRename,
		kCancel
	};
}
namespace NRecursedType { 
	enum EEnum {
		kRecursed,
		kWildcardOnlyRecursed,
		kNonRecursed
	}; 
}
namespace NCommandType { 
	enum EEnum {
		kAdd = 0,
		kUpdate,
		kDelete,
		kTest,
		kExtract,
		kExtractFull,
		kList,
		kBenchmark,
		kInfo,
		kHash,
		kRename
	}; 
}

struct CExtractNtOptions {
	CExtractNtOptions();

	CBoolPair NtSecurity;
	CBoolPair SymLinks;
	CBoolPair HardLinks;
	CBoolPair AltStreams;
	bool   ReplaceColonForAltStream;
	bool   WriteToAltStreamIfColon;
	bool   PreAllocateOutFile;
	uint8  Reserve[1]; // @alignment
};

struct CArchivePath {
	CArchivePath();
	void ParseFromPath(const UString &path, EArcNameMode mode);
	UString GetPathWithoutExt() const { return Prefix + Name; }
	UString GetFinalPath() const;
	UString GetFinalVolPath() const;
	FString GetTempPath() const;

	UString OriginalPath;
	UString Prefix; // path(folder) prefix including slash
	UString Name; // base name
	UString BaseExtension; // archive type extension or "exe" extension
	UString VolExtension; // archive type extension for volumes
	FString TempPrefix; // path(folder) for temp location
	FString TempPostfix;
	bool   Temp;
	uint8  Reserve[3]; // @alignment
};
//
//
//
struct CDirItemsStat {
	CDirItemsStat();
	uint64 Get_NumItems() const;
	uint64 Get_NumDataItems() const;
	uint64 GetTotalBytes() const;
	bool IsEmpty() const;

	uint64 NumDirs;
	uint64 NumFiles;
	uint64 NumAltStreams;
	uint64 FilesSize;
	uint64 AltStreamsSize;
	uint64 NumErrors;
};

struct CDirItem {
	CDirItem() : PhyParent(-1), LogParent(-1), SecureIndex(-1), IsAltStream(false) 
	{
	}
	bool   IsDir() const { return (Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0; }

	uint64 Size;
	FILETIME CTime;
	FILETIME ATime;
	FILETIME MTime;
	UString Name;
#if defined(_WIN32) && !defined(UNDER_CE)
	// UString ShortName;
	CByteBuffer ReparseData;
	CByteBuffer ReparseData2; // fixed (reduced) absolute links
	bool   AreReparseData() const { return ReparseData.Size() != 0 || ReparseData2.Size() != 0; }
#endif
	uint32 Attrib;
	int    PhyParent;
	int    LogParent;
	int    SecureIndex;
	bool   IsAltStream;
	uint8  Reserve[3]; // @alignment
};

class CDirItems {
	UStringVector Prefixes;
	CIntVector PhyParents;
	CIntVector LogParents;

	UString GetPrefixesPath(const CIntVector &parents, int index, const UString &name) const;
	HRESULT EnumerateDir(int phyParent, int logParent, const FString &phyPrefix);
public:
	CObjectVector<CDirItem> Items;
	bool SymLinks;
	bool ScanAltStreams;
	CDirItemsStat Stat;
  #ifndef UNDER_CE
	HRESULT SetLinkInfo(CDirItem &dirItem, const NWindows::NFile::NFind::CFileInfo &fi, const FString &phyPrefix);
  #endif
  #if defined(_WIN32) && !defined(UNDER_CE)
	CUniqBlocks SecureBlocks;
	CByteBuffer TempSecureBuf;
	bool _saclEnabled;
	bool ReadSecure;
	HRESULT AddSecurityItem(const FString &path, int &secureIndex);
  #endif
	IDirItemsCallback * Callback;

	CDirItems();
	void AddDirFileInfo(int phyParent, int logParent, int secureIndex, const NWindows::NFile::NFind::CFileInfo &fi);
	HRESULT AddError(const FString &path, DWORD errorCode);
	HRESULT AddError(const FString &path);
	HRESULT ScanProgress(const FString &path);
	// unsigned GetNumFolders() const { return Prefixes.Size(); }
	FString GetPhyPath(uint index) const;
	UString GetLogPath(uint index) const;
	unsigned AddPrefix(int phyParent, int logParent, const UString &prefix);
	void DeleteLastPrefix();
	HRESULT EnumerateItems2(const FString &phyPrefix, const UString &logPrefix, const FStringVector &filePaths, FStringVector * requestedPaths);
#if defined(_WIN32) && !defined(UNDER_CE)
	void FillFixedReparse();
#endif
	void ReserveDown();
};

struct CArcItem {
	CArcItem() : IsDir(false), IsAltStream(false), SizeDefined(false), MTimeDefined(false), Censored(false), TimeType(-1) 
	{
	}
	uint64 Size;
	FILETIME MTime;
	UString Name;
	uint32 IndexInServer;
	int    TimeType;
	bool   IsDir;
	bool   IsAltStream;
	bool   SizeDefined;
	bool   MTimeDefined;
	bool   Censored;
	uint8  Reserve[3]; // @alignment
};
//#include <StreamBinder.h>
// 
// We don't use probably UNSAFE version:
// reader thread:
//   _canWrite_Event.Set();
//   _readingWasClosed = true
//   _canWrite_Event.Set();
// writer thread:
//   _canWrite_Event.Wait()
//   if(_readingWasClosed)
// Can second call of _canWrite_Event.Set() be executed without memory barrier, if event is already set?
//
class CStreamBinder {
	NWindows::NSynchronization::CAutoResetEvent _canWrite_Event;
	NWindows::NSynchronization::CManualResetEvent _canRead_Event;
	NWindows::NSynchronization::CManualResetEvent _readingWasClosed_Event;
	// bool _readingWasClosed;
	bool _readingWasClosed2;
	// bool WritingWasCut;
	bool _waitWrite;
	uint32 _bufSize;
	const void * _buf;
public:
	uint64 ProcessedSize;

	WRes CreateEvents();
	void CreateStreams(ISequentialInStream ** inStream, ISequentialOutStream ** outStream);
	void ReInit();
	HRESULT Read(void * data, uint32 size, uint32 * processedSize);
	HRESULT Write(const void * data, uint32 size, uint32 * processedSize);
	void CloseRead();
	void CloseWrite();
};
//
//
//
struct CStreamFileProps {
	uint64 Size;
	uint64 VolID;
	uint64 FileID_Low;
	uint64 FileID_High;
	uint32 NumLinks;
	uint32 Attrib;
	FILETIME CTime;
	FILETIME ATime;
	FILETIME MTime;
};
//#include <UpdateAction.h>
namespace NUpdateArchive {
	namespace NPairState {
		const uint kNumValues = 7;
		enum EEnum {
			kNotMasked = 0,
			kOnlyInArchive,
			kOnlyOnDisk,
			kNewInArchive,
			kOldInArchive,
			kSameFiles,
			kUnknowNewerFiles
		};
	}
	namespace NPairAction {
		enum EEnum {
			kIgnore = 0,
			kCopy,
			kCompress,
			kCompressAsAnti
		};
	}

	struct CActionSet {
		NPairAction::EEnum StateActions[NPairState::kNumValues];

		bool FASTCALL IsEqualTo(const CActionSet &a) const;
		bool NeedScanning() const;
	};
	extern const CActionSet k_ActionSet_Add;
	extern const CActionSet k_ActionSet_Update;
	extern const CActionSet k_ActionSet_Fresh;
	extern const CActionSet k_ActionSet_Sync;
	extern const CActionSet k_ActionSet_Delete;
}
//
//#include <Bench.h>
struct CBenchInfo {
	CBenchInfo();
	uint64 GetUsage() const;
	uint64 GetRatingPerUsage(uint64 rating) const;
	uint64 GetSpeed(uint64 numCommands) const;

	uint64 GlobalTime;
	uint64 GlobalFreq;
	uint64 UserTime;
	uint64 UserFreq;
	uint64 UnpackSize;
	uint64 PackSize;
	uint64 NumIterations;
};

struct IBenchCallback {
	virtual HRESULT SetFreq(bool showFreq, uint64 cpuFreq) = 0;
	virtual HRESULT SetEncodeResult(const CBenchInfo &info, bool final) = 0;
	virtual HRESULT SetDecodeResult(const CBenchInfo &info, bool final) = 0;
};

uint64 GetCompressRating(uint32 dictSize, uint64 elapsedTime, uint64 freq, uint64 size);
uint64 GetDecompressRating(uint64 elapsedTime, uint64 freq, uint64 outSize, uint64 inSize, uint64 numIterations);

const uint kBenchMinDicLogSize = 18;

uint64 GetBenchMemoryUsage(uint32 numThreads, uint32 dictionary, bool totalBench = false);

struct IBenchPrintCallback {
	virtual void Print(const char * s) = 0;
	virtual void NewLine() = 0;
	virtual HRESULT CheckBreak() = 0;
};

/*struct IBenchFreqCallback {
	virtual void AddCpuFreq(uint64 freq) = 0;
};*/

AString GetProcessThreadsInfo(const NWindows::NSystem::CProcessAffinity &ti);
void GetSysInfo(AString &s1, AString &s2);
void GetCpuName(AString &s);
void GetCpuFeatures(AString &s);
//
//#include <IsoHeader.h>
namespace NArchive {
	namespace NIso {

		const Byte kVersion = 1;
		extern const char * const kElToritoSpec;
		const uint32 kStartPos = 0x8000;
		const Byte kBootMediaTypeMask = 0xF;

		namespace NVolDescType {
			const Byte kBootRecord = 0;
			const Byte kPrimaryVol = 1;
			const Byte kSupplementaryVol = 2;
			const Byte kVolParttition = 3;
			const Byte kTerminator = 255;
		}
		namespace NFileFlags {
			const Byte kDirectory = 1 << 1;
			const Byte kNonFinalExtent = 1 << 7;
		}
		namespace NBootEntryId {
			const Byte kValidationEntry = 1;
			const Byte kInitialEntryNotBootable = 0;
			const Byte kInitialEntryBootable = 0x88;
			const Byte kMoreHeaders = 0x90;
			const Byte kFinalHeader = 0x91;
			const Byte kExtensionIndicator = 0x44;
		}
		namespace NBootPlatformId {
			const Byte kX86 = 0;
			const Byte kPowerPC = 1;
			const Byte kMac = 2;
		}
		namespace NBootMediaType {
			const Byte kNoEmulation = 0;
			const Byte k1d2Floppy = 1;
			const Byte k1d44Floppy = 2;
			const Byte k2d88Floppy = 3;
			const Byte kHardDisk = 4;
		}
	}
}
//
//#include <IsoItem.h>
namespace NArchive {
	namespace NIso {
		struct CRecordingDateTime {
			Byte Year;
			Byte Month;
			Byte Day;
			Byte Hour;
			Byte Minute;
			Byte Second;
			signed char GmtOffset; // min intervals from -48 (West) to +52 (East) recorded.
			bool GetFileTime(FILETIME &ft) const
			{
				uint64 value;
				bool res = NWindows::NTime::GetSecondsSince1601(Year + 1900, Month, Day, Hour, Minute, Second, value);
				if(res) {
					value -= (int64)((int32)GmtOffset * 15 * 60);
					value *= 10000000;
				}
				ft.dwLowDateTime = (DWORD)value;
				ft.dwHighDateTime = (DWORD)(value >> 32);
				return res;
			}
		};

		enum EPx {
			k_Px_Mode,
			k_Px_Links,
			k_Px_User,
			k_Px_Group,
			k_Px_SerialNumber

			// k_Px_Num
		};
		/*
		   enum ETf {
			   k_Tf_CTime,
			   k_Tf_MTime,
			   k_Tf_ATime,
			   k_Tf_Attrib,
			   k_Tf_Backup,
			   k_Tf_Expiration,
			   k_Tf_Effective
			   // k_Tf_Num
		   };
		 */
		struct CDirRecord {
			uint32 ExtentLocation;
			uint32 Size;
			CRecordingDateTime DateTime;
			Byte FileFlags;
			Byte FileUnitSize;
			Byte InterleaveGapSize;
			Byte ExtendedAttributeRecordLen;
			uint16 VolSequenceNumber;
			CByteBuffer FileId;
			CByteBuffer SystemUse;

			bool AreMultiPartEqualWith(const CDirRecord &a) const
			{
				return FileId == a.FileId && (FileFlags & (~NFileFlags::kNonFinalExtent)) == (a.FileFlags & (~NFileFlags::kNonFinalExtent));
			}
			bool IsDir() const { return (FileFlags & NFileFlags::kDirectory) != 0; }
			bool IsNonFinalExtent() const { return (FileFlags & NFileFlags::kNonFinalExtent) != 0; }
			bool IsSystemItem() const
			{
				if(FileId.Size() != 1)
					return false;
				Byte b = *(const Byte *)FileId;
				return (b == 0 || b == 1);
			}
			const Byte* FindSuspRecord(unsigned skipSize, Byte id0, Byte id1, unsigned &lenRes) const
			{
				lenRes = 0;
				if(SystemUse.Size() < skipSize)
					return 0;
				const Byte * p = (const Byte *)SystemUse + skipSize;
				unsigned rem = (uint)(SystemUse.Size() - skipSize);
				while(rem >= 5) {
					uint len = p[2];
					if(len < 3 || len > rem)
						return 0;
					if(p[0] == id0 && p[1] == id1 && p[3] == 1) {
						if(len < 4)
							return 0;  // Check it
						lenRes = len - 4;
						return p + 4;
					}
					p += len;
					rem -= len;
				}
				return 0;
			}

			const Byte* GetNameCur(bool checkSusp, int skipSize, unsigned &nameLenRes) const
			{
				const Byte * res = NULL;
				uint len = 0;
				if(checkSusp)
					res = FindSuspRecord(skipSize, 'N', 'M', len);
				if(!res || len < 1) {
					res = (const Byte *)FileId;
					len = (uint)FileId.Size();
				}
				else {
					res++;
					len--;
				}
				uint i;
				for(i = 0; i < len; i++)
					if(res[i] == 0)
						break;
				nameLenRes = i;
				return res;
			}

			const bool GetSymLink(int skipSize, AString &link) const
			{
				link.Empty();
				const Byte * p = NULL;
				uint len = 0;
				p = FindSuspRecord(skipSize, 'S', 'L', len);
				if(!p || len < 1)
					return false;

				if(*p != 0)
					return false;

				p++;
				len--;

				while(len != 0) {
					if(len < 2)
						return false;
					unsigned flags = p[0];
					unsigned cl = p[1];
					p += 2;
					len -= 2;

					if(cl > len)
						return false;
					bool needSlash = false;
					if(flags & (1 << 1)) link += "./";
					else if(flags & (1 << 2)) link += "../";
					else if(flags & (1 << 3)) link += '/';
					else
						needSlash = true;
					for(uint i = 0; i < cl; i++) {
						char c = p[i];
						if(c == 0) {
							break;
							// return false;
						}
						link += c;
					}
					p += cl;
					len -= cl;
					if(len == 0)
						break;
					if(needSlash)
						link += '/';
				}
				return true;
			}

			static const bool GetLe32Be32(const Byte * p, uint32 &dest)
			{
				uint32 v1 = GetUi32(p);
				uint32 v2 = GetBe32(p + 4);
				if(v1 == v2) {
					dest = v1;
					return true;
				}
				return false;
			}

			const bool GetPx(int skipSize, unsigned pxType, uint32 &val) const
			{
				val = 0;
				const Byte * p = NULL;
				uint len = 0;
				p = FindSuspRecord(skipSize, 'P', 'X', len);
				if(!p)
					return false;
				// px.Clear();
				if(len < ((uint)pxType + 1) * 8)
					return false;

				return GetLe32Be32(p + pxType * 8, val);
			}

			/*
			   const bool GetTf(int skipSize, unsigned pxType, CRecordingDateTime &t) const
			   {
			   const Byte *p = NULL;
			   uint len = 0;
			   p = FindSuspRecord(skipSize, 'T', 'F', len);
			   if(!p)
				return false;
			   if(len < 1)
				return false;
			   Byte flags = *p++;
			   len--;

			   unsigned step = 7;
			   if(flags & 0x80)
			   {
				step = 17;
				return false;
			   }

			   if((flags & (1 << pxType)) == 0)
				return false;

			   for(unsigned i = 0; i < pxType; i++)
			   {
				if(len < step)
				  return false;
				if(flags & (1 << i))
				{
				  p += step;
				  len -= step;
				}
			   }

			   if(len < step)
				return false;

			   t.Year = p[0];
			   t.Month = p[1];
			   t.Day = p[2];
			   t.Hour = p[3];
			   t.Minute = p[4];
			   t.Second = p[5];
			   t.GmtOffset = (signed char)p[6];

			   return true;
			   }
			 */

			bool CheckSusp(const Byte * p, unsigned &startPos) const
			{
				if(p[0] == 'S' && p[1] == 'P' && p[2] == 0x7 && p[3] == 0x1 && p[4] == 0xBE && p[5] == 0xEF) {
					startPos = p[6];
					return true;
				}
				return false;
			}
			bool CheckSusp(unsigned &startPos) const
			{
				const Byte * p = (const Byte *)SystemUse;
				uint len = (int)SystemUse.Size();
				const uint kMinLen = 7;
				if(len < kMinLen)
					return false;
				if(CheckSusp(p, startPos))
					return true;
				const uint kOffset2 = 14;
				if(len < kOffset2 + kMinLen)
					return false;
				return CheckSusp(p + kOffset2, startPos);
			}
		};
	}
}
//
//#include <DynLimBuf.h>
class CDynLimBuf {
	Byte * _chars;
	size_t _pos;
	size_t _size;
	size_t _sizeLimit;
	bool _error;
	CDynLimBuf(const CDynLimBuf &s);
	// ---------- forbidden functions ----------
	CDynLimBuf & operator+=(wchar_t c);
public:
	CDynLimBuf(size_t limit) throw();
	~CDynLimBuf() 
	{
		SAlloc::F(_chars);
	}
	size_t Len() const { return _pos; }
	bool IsError() const { return _error; }
	void Empty() { _pos = 0; _error = false; }
	operator const Byte *() const { return _chars; }
	// const char *Ptr() const { return _chars; }

	CDynLimBuf & operator+=(char c) throw();
	CDynLimBuf & operator+=(const char * s) throw();
};
//
//#include <MethodProps.h>
struct ICompressSetCoderProperties;

bool   FASTCALL StringToBool(const wchar_t * s, bool &res);
HRESULT FASTCALL PROPVARIANT_to_bool(const PROPVARIANT &prop, bool &dest);
uint   FASTCALL ParseStringToUInt32(const UString &srcString, uint32 &number);
HRESULT FASTCALL ParsePropToUInt32(const UString &name, const PROPVARIANT &prop, uint32 &resValue);
HRESULT ParseMtProp(const UString &name, const PROPVARIANT &prop, uint32 defaultNumThreads, uint32 &numThreads);

struct CProp {
	CProp() : Id(0), IsOptional(false)
	{
	}
	PROPID Id;
	NWindows::NCOM::CPropVariant Value;
	bool IsOptional;
};

struct CProps {
	void   Clear();
	bool   AreThereNonOptionalProps() const;
	void   AddProp32(PROPID propid, uint32 val);
	void   AddPropBool(PROPID propid, bool val);
	void   AddProp_Ascii(PROPID propid, const char * s);
	HRESULT SetCoderProps(ICompressSetCoderProperties * scp, const uint64 * dataSizeReduce) const;

	CObjectVector <CProp> Props;
};

class CMethodProps : public CProps {
public:
	int    GetLevel() const;
	int    Get_NumThreads() const;
	bool   Get_DicSize(uint32 &res) const;
	int    FindProp(PROPID id) const;
	uint32 Get_Lzma_Algo() const;
	uint32 Get_Lzma_DicSize() const;
	bool   Get_Lzma_Eos() const;
	bool   Are_Lzma_Model_Props_Defined() const;
	uint32 Get_Lzma_NumThreads() const;
	uint32 Get_Lzma2_NumThreads(bool &fixedNumber) const;
	uint64 Get_Lzma2_BlockSize() const;
	uint32 Get_BZip2_NumThreads(bool &fixedNumber) const;
	uint32 Get_BZip2_BlockSize() const;
	uint32 Get_Ppmd_MemSize() const;
	void   AddProp_Level(uint32 level);
	void   AddProp_NumThreads(uint32 numThreads);
	void   AddProp_EndMarker_if_NotFound(bool eos);
	HRESULT ParseParamsFromString(const UString &srcString);
	HRESULT ParseParamsFromPROPVARIANT(const UString &realName, const PROPVARIANT &value);
private:
	HRESULT SetParam(const UString &name, const UString &value);
};

class COneMethodInfo : public CMethodProps {
public:
	void Clear();
	bool IsEmpty() const { return MethodName.IsEmpty() && Props.IsEmpty(); }
	HRESULT ParseMethodFromPROPVARIANT(const UString &realName, const PROPVARIANT &value);
	HRESULT ParseMethodFromString(const UString &s);

	AString MethodName;
	UString PropsString;
};
//
#endif // !__7Z_INTERNAL_H
