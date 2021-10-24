// 7Z-UTIL.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;
using namespace NCOM;

// Common/CRC.cpp
struct CCRCTableInit { CCRCTableInit() { CrcGenerateTable(); } } g_CRCTableInit;
//
// XzCrc64Init.cpp
static struct CCrc64Gen { CCrc64Gen() { Crc64GenerateTable(); } } g_Crc64TableInit;
//
// PropId.cpp
// VARTYPE
const Byte k7z_PROPID_To_VARTYPE[kpid_NUM_DEFINED] = {
	VT_EMPTY,
	VT_UI4,
	VT_UI4,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR,
	VT_BOOL,
	VT_UI8,
	VT_UI8,
	VT_UI4,
	VT_FILETIME,
	VT_FILETIME,
	VT_FILETIME,
	VT_BOOL,
	VT_BOOL,
	VT_BOOL,
	VT_BOOL,
	VT_BOOL,
	VT_UI4,
	VT_UI4,
	VT_BSTR,
	VT_BOOL,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR,
	VT_UI8,
	VT_BSTR,
	VT_UI8,
	VT_BSTR,
	VT_UI8,
	VT_UI8,
	VT_BSTR, // or VT_UI8 kpidUnpackVer
	VT_UI4, // or VT_UI8 kpidVolume
	VT_BOOL,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_UI4,
	VT_BOOL,
	VT_BOOL,
	VT_BSTR,
	VT_UI8,
	VT_UI8,
	VT_UI4, // kpidChecksum
	VT_BSTR,
	VT_UI8,
	VT_BSTR, // or VT_UI8 kpidId
	VT_BSTR,
	VT_BSTR,
	VT_UI4,
	VT_UI4,
	VT_BSTR,
	VT_BSTR,
	VT_UI8,
	VT_UI8,
	VT_UI4,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR, // kpidNtSecure
	VT_BOOL,
	VT_BOOL,
	VT_BOOL,
	VT_BOOL,
	VT_BSTR, // SHA-1
	VT_BSTR, // SHA-256
	VT_BSTR,
	VT_UI8,
	VT_UI4,
	VT_UI4,
	VT_BSTR,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_UI8,
	VT_BSTR,
	VT_BSTR,
	VT_BSTR,
	VT_BOOL,
	VT_BOOL,
	VT_BOOL,
	VT_UI8,
	VT_UI8,
	VT_BSTR, // kpidNtReparse
	VT_BSTR,
	VT_UI8,
	VT_UI8,
	VT_BOOL,
	VT_BSTR,
	VT_BSTR
};
//
// DefaultName.cpp
static UString GetDefaultName3(const UString &fileName, const UString &extension, const UString &addSubExtension)
{
	const uint extLen = extension.Len();
	const uint fileNameLen = fileName.Len();
	if(fileNameLen > extLen + 1) {
		const uint dotPos = fileNameLen - (extLen + 1);
		if(fileName[dotPos] == '.')
			if(extension.IsEqualTo_NoCase(fileName.Ptr(dotPos + 1)))
				return fileName.Left(dotPos) + addSubExtension;
	}
	int dotPos = fileName.ReverseFind_Dot();
	if(dotPos > 0)
		return fileName.Left(dotPos) + addSubExtension;
	if(addSubExtension.IsEmpty())
		return fileName + L'~';
	else
		return fileName + addSubExtension;
}

UString GetDefaultName2(const UString &fileName, const UString &extension, const UString &addSubExtension)
{
	UString name = GetDefaultName3(fileName, extension, addSubExtension);
	name.TrimRight();
	return name;
}
//
// FilePathAutoRename.cpp
static bool MakeAutoName(const FString &name, const FString &extension, uint32 value, FString &path)
{
	path = name;
	path.Add_UInt32(value);
	path += extension;
	return NFile::NFind::DoesFileOrDirExist(path);
}

bool AutoRenamePath(FString &path)
{
	int dotPos = path.ReverseFind_Dot();
	int slashPos = path.ReverseFind_PathSepar();
	FString name = path;
	FString extension;
	if(dotPos > slashPos + 1) {
		name.DeleteFrom(dotPos);
		extension = path.Ptr(dotPos);
	}
	name += '_';
	FString temp;
	uint32 left = 1, right = ((uint32)1 << 30);
	while(left != right) {
		uint32 mid = (left + right) / 2;
		if(MakeAutoName(name, extension, mid, temp))
			left = mid + 1;
		else
			right = mid;
	}
	return !MakeAutoName(name, extension, right, path);
}
//
// ListFileUtils.cpp
static const char kQuoteChar = '\"';

static void AddName(UStringVector &strings, UString &s)
{
	s.Trim();
	if(s.Len() >= 2 && s[0] == kQuoteChar && s.Back() == kQuoteChar) {
		s.DeleteBack();
		s.Delete(0);
	}
	if(!s.IsEmpty())
		strings.Add(s);
}

bool ReadNamesFromListFile(CFSTR fileName, UStringVector &strings, UINT codePage)
{
	NWindows::NFile::NIO::CInFile file;
	if(!file.Open(fileName))
		return false;
	uint64 fileSize;
	if(!file.GetLength(fileSize))
		return false;
	if(fileSize >= ((uint32)1 << 31) - 32)
		return false;
	UString u;
	if(oneof2(codePage, MY__CP_UTF16, MY__CP_UTF16BE)) {
		if((fileSize & 1) != 0)
			return false;
		CByteArr buf((size_t)fileSize);
		uint32 processed;
		if(!file.Read(buf, (uint32)fileSize, processed))
			return false;
		if(processed != fileSize)
			return false;
		file.Close();
		unsigned num = (uint)fileSize / 2;
		wchar_t * p = u.GetBuf(num);
		if(codePage == MY__CP_UTF16)
			for(uint i = 0; i < num; i++) {
				wchar_t c = GetUi16(buf + (size_t)i * 2);
				if(c == 0)
					return false;
				p[i] = c;
			}
		else
			for(uint i = 0; i < num; i++) {
				wchar_t c = (wchar_t)GetBe16(buf + (size_t)i * 2);
				if(c == 0)
					return false;
				p[i] = c;
			}
		p[num] = 0;
		u.ReleaseBuf_SetLen(num);
	}
	else {
		AString s;
		char * p = s.GetBuf((uint)fileSize);
		uint32 processed;
		if(!file.Read(p, (uint32)fileSize, processed))
			return false;
		if(processed != fileSize)
			return false;
		file.Close();
		s.ReleaseBuf_CalcLen((uint)processed);
		if(s.Len() != processed)
			return false;
		// #ifdef CP_UTF8
		if(codePage == CP_UTF8) {
			if(!ConvertUTF8ToUnicode(s, u))
				return false;
		}
		else
			// #endif
			MultiByteToUnicodeString2(u, s, codePage);
	}
	const wchar_t kGoodBOM = 0xFEFF;
	const wchar_t kBadBOM  = 0xFFFE;
	UString s;
	unsigned i = 0;
	for(; i < u.Len() && u[i] == kGoodBOM; i++) ;
	for(; i < u.Len(); i++) {
		wchar_t c = u[i];
		if(c == kGoodBOM || c == kBadBOM)
			return false;
		if(c == '\n' || c == 0xD) {
			AddName(strings, s);
			s.Empty();
		}
		else
			s += c;
	}
	AddName(strings, s);
	return true;
}
//
// SortUtils.cpp
static int CompareStrings(const uint * p1, const uint * p2, void * param)
{
	const UStringVector &strings = *(const UStringVector*)param;
	return CompareFileNames(strings[*p1], strings[*p2]);
}

void SortFileNames(const UStringVector &strings, CUIntVector &indices)
{
	const uint numItems = strings.Size();
	indices.ClearAndSetSize(numItems);
	if(numItems) {
		uint * vals = &indices[0];
		for(uint i = 0; i < numItems; i++)
			vals[i] = i;
		indices.Sort(CompareStrings, (void *)&strings);
	}
}
//
// IntToString.cpp
#define CONVERT_INT_TO_STR(charType, tempSize) \
	uchar temp[tempSize]; uint i = 0; \
	while(val >= 10) { temp[i++] = (uchar)('0' + (uint)(val % 10)); val /= 10; } \
	*s++ = (charType)('0' + (uint)val);	\
	while(i != 0) { i--; *s++ = temp[i]; } \
	*s = 0;

void FASTCALL ConvertUInt32ToString(uint32 val, char * s) throw()
{
	CONVERT_INT_TO_STR(char, 16);
}

void FASTCALL ConvertUInt64ToString(uint64 val, char * s) throw()
{
	if(val <= (uint32)0xFFFFFFFF) {
		ConvertUInt32ToString((uint32)val, s);
	}
	else {
		CONVERT_INT_TO_STR(char, 24);
	}
}

void ConvertUInt64ToOct(uint64 val, char * s) throw()
{
	uint64 v = val;
	uint   i;
	for(i = 1;; i++) {
		v >>= 3;
		if(v == 0)
			break;
	}
	s[i] = 0;
	do {
		unsigned t = (uint)(val & 0x7);
		val >>= 3;
		s[--i] = (char)('0' + t);
	} while(i);
}

#define GET_HEX_CHAR(t) ((char)(((t < 10) ? ('0' + t) : ('A' + (t - 10)))))

static inline char GetHexChar(unsigned t) { return GET_HEX_CHAR(t); }

void ConvertUInt32ToHex(uint32 val, char * s) throw()
{
	uint32 v = val;
	uint   i;
	for(i = 1;; i++) {
		v >>= 4;
		if(v == 0)
			break;
	}
	s[i] = 0;
	do {
		uint   t = (uint)(val & 0xF);
		val >>= 4;
		s[--i] = GET_HEX_CHAR(t);
	} while(i);
}

void ConvertUInt64ToHex(uint64 val, char * s) throw()
{
	uint64 v = val;
	uint   i;
	for(i = 1;; i++) {
		v >>= 4;
		if(v == 0)
			break;
	}
	s[i] = 0;
	do {
		uint   t = (uint)(val & 0xF);
		val >>= 4;
		s[--i] = GET_HEX_CHAR(t);
	} while(i);
}

void ConvertUInt32ToHex8Digits(uint32 val, char * s) throw()
{
	s[8] = 0;
	for(int i = 7; i >= 0; i--) {
		uint   t = val & 0xF;
		val >>= 4;
		s[i] = GET_HEX_CHAR(t);;
	}
}
/*
   void ConvertUInt32ToHex8Digits(uint32 val, wchar_t *s)
   {
   s[8] = 0;
   for(int i = 7; i >= 0; i--) {
    unsigned t = val & 0xF;
    val >>= 4;
    s[i] = (wchar_t)(((t < 10) ? ('0' + t) : ('A' + (t - 10))));
   }
   }
 */
void FASTCALL ConvertUInt32ToString(uint32 val, wchar_t * s) throw()
{
	CONVERT_INT_TO_STR(wchar_t, 16);
}

void FASTCALL ConvertUInt64ToString(uint64 val, wchar_t * s) throw()
{
	if(val <= (uint32)0xFFFFFFFF) {
		ConvertUInt32ToString((uint32)val, s);
		return;
	}
	CONVERT_INT_TO_STR(wchar_t, 24);
}

void ConvertInt64ToString(int64 val, char * s) throw()
{
	if(val < 0) {
		*s++ = '-';
		val = -val;
	}
	ConvertUInt64ToString(val, s);
}

void ConvertInt64ToString(int64 val, wchar_t * s) throw()
{
	if(val < 0) {
		*s++ = L'-';
		val = -val;
	}
	ConvertUInt64ToString(val, s);
}

static void ConvertByteToHex2Digits(unsigned v, char * s) throw()
{
	s[0] = GetHexChar(v >> 4);
	s[1] = GetHexChar(v & 0xF);
}

static void ConvertUInt16ToHex4Digits(uint32 val, char * s) throw()
{
	ConvertByteToHex2Digits(val >> 8, s);
	ConvertByteToHex2Digits(val & 0xFF, s + 2);
}

char * RawLeGuidToString(const Byte * g, char * s) throw()
{
	ConvertUInt32ToHex8Digits(GetUi32(g),  s);  s += 8;  *s++ = '-';
	ConvertUInt16ToHex4Digits(GetUi16(g + 4), s);  s += 4;  *s++ = '-';
	ConvertUInt16ToHex4Digits(GetUi16(g + 6), s);  s += 4;  *s++ = '-';
	for(uint i = 0; i < 8; i++) {
		if(i == 2)
			*s++ = '-';
		ConvertByteToHex2Digits(g[8 + i], s);
		s += 2;
	}
	*s = 0;
	return s;
}

char * RawLeGuidToString_Braced(const Byte * g, char * s) throw()
{
	*s++ = '{';
	s = RawLeGuidToString(g, s);
	*s++ = '}';
	*s = 0;
	return s;
}
//
// StringToInt.cpp
static const uint32 k_uint32_max = 0xFFFFFFFF;
static const uint64 k_uint64_max = UINT64_CONST(0xFFFFFFFFFFFFFFFF);
// static const uint64 k_UInt64_max = static_cast<uint64>(-1LL);

#define CONVERT_STRING_TO_UINT_FUNC(uintType, charType, charTypeUnsigned) \
	uintType ConvertStringTo ## uintType(const charType *s, const charType **end) throw() {	\
		ASSIGN_PTR(end, s); \
		uintType res = 0; \
		for(;; s++) { \
			charTypeUnsigned c = (charTypeUnsigned)*s; \
			if(c < '0' || c > '9') { ASSIGN_PTR(end, s); return res; } \
			if(res > (k_ ## uintType ## _max) / 10) return 0; \
			res *= 10; \
			unsigned v = (c - '0');	\
			if(res > (k_ ## uintType ## _max) - v) return 0; \
			res += v; }}

CONVERT_STRING_TO_UINT_FUNC(uint32, char, Byte)
CONVERT_STRING_TO_UINT_FUNC(uint32, wchar_t, wchar_t)
CONVERT_STRING_TO_UINT_FUNC(uint64, char, Byte)
CONVERT_STRING_TO_UINT_FUNC(uint64, wchar_t, wchar_t)

uint32 ConvertStringToUInt32(const char *s, const char **end) throw() { return ConvertStringTouint32(s, end); }
uint32 ConvertStringToUInt32(const wchar_t *s, const wchar_t **end) throw() { return ConvertStringTouint32(s, end); }
uint64 ConvertStringToUInt64(const char *s, const char **end) throw() { return ConvertStringTouint64(s, end); }
uint64 ConvertStringToUInt64(const wchar_t *s, const wchar_t **end) throw() { return ConvertStringTouint64(s, end); }

int32 ConvertStringToInt32(const wchar_t * s, const wchar_t ** end) throw()
{
	ASSIGN_PTR(end, s);
	const wchar_t * s2 = s;
	if(*s == '-')
		s2++;
	if(*s2 == 0)
		return 0;
	const wchar_t * end2;
	uint32 res = ConvertStringToUInt32(s2, &end2);
	if(*s == '-') {
		if(res > ((uint32)1 << (32 - 1)))
			return 0;
	}
	else if((res & ((uint32)1 << (32 - 1))) != 0)
		return 0;
	ASSIGN_PTR(end, end2);
	return (*s == '-') ? -(int32)res : (int32)res;
}

uint32 ConvertOctStringToUInt32(const char * s, const char ** end) throw()
{
	ASSIGN_PTR(end, s);
	uint32 res = 0;
	for(;; s++) {
		unsigned c = (uchar)*s;
		if(c < '0' || c > '7') {
			ASSIGN_PTR(end, s);
			return res;
		}
		if((res & (uint32)7 << (32 - 3)) != 0)
			return 0;
		res <<= 3;
		res |= (uint)(c - '0');
	}
}

uint64 ConvertOctStringToUInt64(const char * s, const char ** end) throw()
{
	ASSIGN_PTR(end, s);
	uint64 res = 0;
	for(;; s++) {
		uint c = (uchar)*s;
		if(c < '0' || c > '7') {
			ASSIGN_PTR(end, s);
			return res;
		}
		if((res & (uint64)7 << (64 - 3)) != 0)
			return 0;
		res <<= 3;
		res |= (uint)(c - '0');
	}
}

uint32 ConvertHexStringToUInt32(const char * s, const char ** end) throw()
{
	ASSIGN_PTR(end, s);
	uint32 res = 0;
	for(;; s++) {
		unsigned c = (Byte)*s;
		unsigned v;
		if(c >= '0' && c <= '9') v = (c - '0');
		else if(c >= 'A' && c <= 'F') v = 10 + (c - 'A');
		else if(c >= 'a' && c <= 'f') v = 10 + (c - 'a');
		else {
			ASSIGN_PTR(end, s);
			return res;
		}
		if((res & (uint32)0xF << (32 - 4)) != 0)
			return 0;
		res <<= 4;
		res |= v;
	}
}

uint64 ConvertHexStringToUInt64(const char * s, const char ** end) throw()
{
	ASSIGN_PTR(end, s);
	uint64 res = 0;
	for(;; s++) {
		unsigned c = (Byte)*s;
		unsigned v;
		if(c >= '0' && c <= '9') v = (c - '0');
		else if(c >= 'A' && c <= 'F') v = 10 + (c - 'A');
		else if(c >= 'a' && c <= 'f') v = 10 + (c - 'a');
		else {
			ASSIGN_PTR(end, s);
			return res;
		}
		if((res & (uint64)0xF << (64 - 4)) != 0)
			return 0;
		res <<= 4;
		res |= v;
	}
}
//
// StringConvert.cpp
static const char k_DefultChar = '_';

#ifdef _WIN32
	/*
	   MultiByteToWideChar(CodePage, DWORD dwFlags,
		LPCSTR lpMultiByteStr, int cbMultiByte,
		LPWSTR lpWideCharStr, int cchWideChar)

	   if(cbMultiByte == 0)
		return: 0. ERR: ERROR_INVALID_PARAMETER

	   if(cchWideChar == 0)
		return: the required buffer size in characters.

	   if(supplied buffer size was not large enough)
		return: 0. ERR: ERROR_INSUFFICIENT_BUFFER
		The number of filled characters in lpWideCharStr can be smaller than cchWideChar (if last character is complex)

	   If there are illegal characters:
		if MB_ERR_INVALID_CHARS is set in dwFlags:
		  - the function stops conversion on illegal character.
		  - Return: 0. ERR: ERROR_NO_UNICODE_TRANSLATION.

		if MB_ERR_INVALID_CHARS is NOT set in dwFlags:
		  before Vista: illegal character is dropped (skipped). WinXP-64: GetLastError() returns 0.
		  in Vista+:    illegal character is not dropped (MSDN). Undocumented: illegal
						character is converted to U+FFFD, which is REPLACEMENT CHARACTER.
	 */
	void FASTCALL MultiByteToUnicodeString2(UString &dest, const AString &src, UINT codePage)
	{
		dest.Empty();
		if(src.IsEmpty())
			return;
		{
			/*
			   wchar_t *d = dest.GetBuf(src.Len());
			   const char *s = (const char *)src;
			   uint i;

			   for(i = 0;;)
			   {
			   Byte c = (Byte)s[i];
			   if(c >= 0x80 || c == 0)
				break;
			   d[i++] = (wchar_t)c;
			   }

			   if(i != src.Len())
			   {
			   uint len = MultiByteToWideChar(codePage, 0, s + i,
				  src.Len() - i, d + i,
				  src.Len() + 1 - i);
			   if(len == 0)
				throw 282228;
			   i += len;
			   }

			   d[i] = 0;
			   dest.ReleaseBuf_SetLen(i);
			 */
			uint len = MultiByteToWideChar(codePage, 0, src, src.Len(), NULL, 0);
			if(len == 0) {
				if(GetLastError() != 0)
					throw 282228;
			}
			else {
				len = MultiByteToWideChar(codePage, 0, src, src.Len(), dest.GetBuf(len), len);
				if(len == 0)
					throw 282228;
				dest.ReleaseBuf_SetEnd(len);
			}
		}
	}
	/*
	   int WideCharToMultiByte(
		  UINT CodePage, DWORD dwFlags,
		  LPCWSTR lpWideCharStr, int cchWideChar,
		  LPSTR lpMultiByteStr, int cbMultiByte,
		  LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);

	   if(lpDefaultChar == NULL),
	   - it uses system default value.

	   if(CodePage == CP_UTF7 || CodePage == CP_UTF8)
	   if(lpDefaultChar != NULL || lpUsedDefaultChar != NULL)
		return: 0. ERR: ERROR_INVALID_PARAMETER.

	   The function operates most efficiently, if(lpDefaultChar == NULL && lpUsedDefaultChar == NULL)

	 */
	static void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
	{
		dest.Empty();
		defaultCharWasUsed = false;
		if(src.IsEmpty())
			return;
		{
			/*
			   unsigned numRequiredBytes = src.Len() * 2;
			   char *d = dest.GetBuf(numRequiredBytes);
			   const wchar_t *s = (const wchar_t *)src;
			   uint i;

			   for(i = 0;;)
			   {
			   wchar_t c = s[i];
			   if(c >= 0x80 || c == 0)
				break;
			   d[i++] = (char)c;
			   }

			   if(i != src.Len())
			   {
			   BOOL defUsed = FALSE;
			   defaultChar = defaultChar;

			   bool isUtf = (codePage == CP_UTF8 || codePage == CP_UTF7);
			   uint len = WideCharToMultiByte(codePage, 0, s + i, src.Len() - i,
				  d + i, numRequiredBytes + 1 - i,
				  (isUtf ? NULL : &defaultChar),
				  (isUtf ? NULL : &defUsed));
			   defaultCharWasUsed = (defUsed != FALSE);
			   if(len == 0)
				throw 282229;
			   i += len;
			   }

			   d[i] = 0;
			   dest.ReleaseBuf_SetLen(i);
			 */

			/*
			   if(codePage != CP_UTF7)
			   {
			   const wchar_t *s = (const wchar_t *)src;
			   uint i;
			   for(i = 0;; i++)
			   {
				wchar_t c = s[i];
				if(c >= 0x80 || c == 0)
				  break;
			   }

			   if(s[i] == 0)
			   {
				char *d = dest.GetBuf(src.Len());
				for(i = 0;;)
				{
				  wchar_t c = s[i];
				  if(c == 0)
					break;
				  d[i++] = (char)c;
				}
				d[i] = 0;
				dest.ReleaseBuf_SetLen(i);
				return;
			   }
			   }
			 */
			uint len = WideCharToMultiByte(codePage, 0, src, src.Len(), NULL, 0, NULL, NULL);
			if(len == 0) {
				if(GetLastError() != 0)
					throw 282228;
			}
			else {
				BOOL defUsed = FALSE;
				bool isUtf = (codePage == CP_UTF8 || codePage == CP_UTF7);
				// defaultChar = defaultChar;
				len = WideCharToMultiByte(codePage, 0, src, src.Len(), dest.GetBuf(len), len, (isUtf ? NULL : &defaultChar), (isUtf ? NULL : &defUsed));
				if(!isUtf)
					defaultCharWasUsed = (defUsed != FALSE);
				if(len == 0)
					throw 282228;
				dest.ReleaseBuf_SetEnd(len);
			}
		}
	}
	/*
	   #ifndef UNDER_CE
	   AString SystemStringToOemString(const CSysString &src)
	   {
	   AString dest;
	   const uint len = src.Len() * 2;
	   CharToOem(src, dest.GetBuf(len));
	   dest.ReleaseBuf_CalcLen(len);
	   return dest;
	   }
	   #endif
	 */
#else
	void FASTCALL MultiByteToUnicodeString2(UString &dest, const AString &src, UINT /* codePage */)
	{
		dest.Empty();
		if(src.IsEmpty())
			return;
		size_t limit = ((size_t)src.Len() + 1) * 2;
		wchar_t * d = dest.GetBuf((uint)limit);
		size_t len = mbstowcs(d, src, limit);
		if(len != (size_t)-1) {
			dest.ReleaseBuf_SetEnd((uint)len);
			return;
		}
		{
			uint i;
			const char * s = (const char *)src;
			for(i = 0;; ) {
				Byte c = (Byte)s[i];
				if(c == 0)
					break;
				d[i++] = (wchar_t)c;
			}
			d[i] = 0;
			dest.ReleaseBuf_SetLen(i);
		}
	}

	static void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT /* codePage */, char defaultChar, bool &defaultCharWasUsed)
	{
		dest.Empty();
		defaultCharWasUsed = false;
		if(src.IsEmpty())
			return;
		size_t limit = ((size_t)src.Len() + 1) * 6;
		char * d = dest.GetBuf((uint)limit);
		size_t len = wcstombs(d, src, limit);
		if(len != (size_t)-1) {
			dest.ReleaseBuf_SetEnd((uint)len);
			return;
		}
		{
			const wchar_t * s = (const wchar_t *)src;
			uint i;
			for(i = 0;; ) {
				wchar_t c = s[i];
				if(c == 0)
					break;
				if(c >= 0x100) {
					c = defaultChar;
					defaultCharWasUsed = true;
				}
				d[i++] = (char)c;
			}
			d[i] = 0;
			dest.ReleaseBuf_SetLen(i);
		}
	}
#endif

UString FASTCALL MultiByteToUnicodeString(const AString &src, UINT codePage)
{
	UString dest;
	MultiByteToUnicodeString2(dest, src, codePage);
	return dest;
}

UString FASTCALL MultiByteToUnicodeString(const char * src, UINT codePage)
{
	return MultiByteToUnicodeString(AString(src), codePage);
}

void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT codePage)
{
	bool defaultCharWasUsed;
	UnicodeStringToMultiByte2(dest, src, codePage, k_DefultChar, defaultCharWasUsed);
}

AString FASTCALL UnicodeStringToMultiByte(const UString &src, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
{
	AString dest;
	UnicodeStringToMultiByte2(dest, src, codePage, defaultChar, defaultCharWasUsed);
	return dest;
}

AString FASTCALL UnicodeStringToMultiByte(const UString &src, UINT codePage)
{
	AString dest;
	bool defaultCharWasUsed;
	UnicodeStringToMultiByte2(dest, src, codePage, k_DefultChar, defaultCharWasUsed);
	return dest;
}
//
// SetProperties.cpp
HRESULT SetProperties(IUnknown * unknown, const CObjectVector<CProperty> & properties)
{
	if(!properties.IsEmpty()) {
		CMyComPtr <ISetProperties> setProperties;
		unknown->QueryInterface(IID_ISetProperties, (void **)&setProperties);
		if(setProperties) {
			UStringVector realNames;
			CPropVariant * values = new CPropVariant[properties.Size()];
			try {
				uint i;
				for(i = 0; i < properties.Size(); i++) {
					const CProperty & property = properties[i];
					CPropVariant propVariant;
					UString name = property.Name;
					if(property.Value.IsEmpty()) {
						if(!name.IsEmpty()) {
							wchar_t c = name.Back();
							if(c == L'-')
								propVariant = false;
							else if(c == L'+')
								propVariant = true;
							if(propVariant.vt != VT_EMPTY)
								name.DeleteBack();
						}
					}
					else
						ParseNumberString(property.Value, propVariant);
					realNames.Add(name);
					values[i] = propVariant;
				}
				CRecordVector <const wchar_t *> names;
				for(i = 0; i < realNames.Size(); i++)
					names.Add((const wchar_t *)realNames[i]);
				RINOK(setProperties->SetProperties(&names.Front(), values, names.Size()));
			}
			catch(...) {
				delete []values;
				throw;
			}
			delete [] values;
		}
	}
	return S_OK;
}
//
// ProgressUtils.cpp
CLocalProgress::CLocalProgress() : ProgressOffset(0), InSize(0), OutSize(0), SendRatio(true), SendProgress(true) 
{
}

void CLocalProgress::Init(IProgress * progress, bool inSizeIsMain)
{
	_ratioProgress.Release();
	_progress = progress;
	_progress.QueryInterface(IID_ICompressProgressInfo, &_ratioProgress);
	_inSizeIsMain = inSizeIsMain;
}

STDMETHODIMP CLocalProgress::SetRatioInfo(const uint64 * inSize, const uint64 * outSize)
{
	uint64 inSize2 = InSize;
	uint64 outSize2 = OutSize;
	if(inSize)
		inSize2 += (*inSize);
	if(outSize)
		outSize2 += (*outSize);
	if(SendRatio && _ratioProgress) {
		RINOK(_ratioProgress->SetRatioInfo(&inSize2, &outSize2));
	}
	if(SendProgress) {
		inSize2 += ProgressOffset;
		outSize2 += ProgressOffset;
		return _progress->SetCompleted(_inSizeIsMain ? &inSize2 : &outSize2);
	}
	return S_OK;
}

HRESULT CLocalProgress::SetCur()
{
	return SetRatioInfo(NULL, NULL);
}
//
// ExtractingFilePath.cpp
#ifdef _WIN32
    bool g_PathTrailReplaceMode = true;
#else
    bool g_PathTrailReplaceMode = false;
#endif

static void ReplaceIncorrectChars(UString &s)
{
	{
		for(uint i = 0; i < s.Len(); i++) {
			wchar_t c = s[i];
			if(
	  #ifdef _WIN32
			    c == ':' || c == '*' || c == '?' || c < 0x20 || c == '<' || c == '>' || c == '|' || c == '"'
			    || c == '/'
			    // || c == 0x202E // RLO
			    ||
	  #endif
			    c == WCHAR_PATH_SEPARATOR)
				s.ReplaceOneCharAtPos(i, '_');
		}
	}

	if(g_PathTrailReplaceMode) {
		/*
		   // if(g_PathTrailReplaceMode == 1)
		   {
		   if(!s.IsEmpty())
		   {
		    wchar_t c = s.Back();
		    if(c == '.' || c == ' ')
		    {
		      // s += (wchar_t)(0x9c); // STRING TERMINATOR
		      s += (wchar_t)'_';
		    }
		   }
		   }
		   else
		 */
		{
			uint i;
			for(i = s.Len(); i != 0; ) {
				wchar_t c = s[i - 1];
				if(c != '.' && c != ' ')
					break;
				i--;
				s.ReplaceOneCharAtPos(i, '_');
				// s.ReplaceOneCharAtPos(i, (c == ' ' ? (wchar_t)(0x2423) : (wchar_t)0x00B7));
			}
			/*
			   if(g_PathTrailReplaceMode > 1 && i != s.Len())
			   {
			   s.DeleteFrom(i);
			   }
			 */
		}
	}
}

#ifdef _WIN32
	// 
	// WinXP-64 doesn't support ':', '\\' and '/' symbols in name of alt stream.
	// But colon in postfix ":$DATA" is allowed.
	// WIN32 functions don't allow empty alt stream name "name:" */
	// 
	void Correct_AltStream_Name(UString &s)
	{
		uint len = s.Len();
		const uint kPostfixSize = 6;
		if(s.Len() >= kPostfixSize && sstreqi_ascii(s.RightPtr(kPostfixSize), ":$DATA"))
			len -= kPostfixSize;
		for(uint i = 0; i < len; i++) {
			wchar_t c = s[i];
			if(c == ':' || c == '\\' || c == '/' || c == 0x202E /*RLO*/)
				s.ReplaceOneCharAtPos(i, '_');
		}
		if(s.IsEmpty())
			s = '_';
	}

	static const uint g_ReservedWithNum_Index = 4;
	static const char * const g_ReservedNames[] = { "CON", "PRN", "AUX", "NUL", "COM", "LPT" };

	static bool IsSupportedName(const UString &name)
	{
		for(uint i = 0; i < SIZEOFARRAY(g_ReservedNames); i++) {
			const char * reservedName = g_ReservedNames[i];
			uint len = sstrlen(reservedName);
			if(name.Len() < len)
				continue;
			if(!name.IsPrefixedBy_Ascii_NoCase(reservedName))
				continue;
			if(i >= g_ReservedWithNum_Index) {
				wchar_t c = name[len];
				if(c < L'0' || c > L'9')
					continue;
				len++;
			}
			for(;; ) {
				wchar_t c = name[len++];
				if(c == 0 || c == '.')
					return false;
				if(c != ' ')
					break;
			}
		}
		return true;
	}

	static void CorrectUnsupportedName(UString &name)
	{
		if(!IsSupportedName(name))
			name.InsertAtFront(L'_');
	}
#endif

static void Correct_PathPart(UString &s)
{
	// "." and ".."
	if(s[0] == '.' && (s[1] == 0 || s[1] == '.' && s[2] == 0))
		s.Empty();
  #ifdef _WIN32
	else
		ReplaceIncorrectChars(s);
  #endif
}

// static const char * const k_EmptyReplaceName = "[]";
static const char k_EmptyReplaceName = '_';

UString Get_Correct_FsFile_Name(const UString &name)
{
	UString res = name;
	Correct_PathPart(res);
  #ifdef _WIN32
	CorrectUnsupportedName(res);
  #endif
	if(res.IsEmpty())
		res = k_EmptyReplaceName;
	return res;
}

void Correct_FsPath(bool absIsAllowed, UStringVector &parts, bool isDir)
{
	unsigned i = 0;
	if(absIsAllowed) {
    #if defined(_WIN32) && !defined(UNDER_CE)
		bool isDrive = false;
    #endif
		if(parts[0].IsEmpty()) {
			i = 1;
      #if defined(_WIN32) && !defined(UNDER_CE)
			if(parts.Size() > 1 && parts[1].IsEmpty()) {
				i = 2;
				if(parts.Size() > 2 && parts[2] == L"?") {
					i = 3;
					if(parts.Size() > 3  && NWindows::NFile::NName::IsDrivePath2(parts[3])) {
						isDrive = true;
						i = 4;
					}
				}
			}
      #endif
		}
    #if defined(_WIN32) && !defined(UNDER_CE)
		else if(NWindows::NFile::NName::IsDrivePath2(parts[0])) {
			isDrive = true;
			i = 1;
		}
		if(isDrive) {
			// we convert "c:name" to "c:\name", if absIsAllowed path.
			UString &ds = parts[i - 1];
			if(ds.Len() > 2) {
				parts.Insert(i, ds.Ptr(2));
				ds.DeleteFrom(2);
			}
		}
    #endif
	}
	for(; i < parts.Size(); ) {
		UString &s = parts[i];
		Correct_PathPart(s);
		if(s.IsEmpty()) {
			if(isDir || i != parts.Size() - 1) {
				parts.Delete(i);
				continue;
			}
			s = k_EmptyReplaceName;
		}
		else {
      #ifdef _WIN32
			CorrectUnsupportedName(s);
      #endif
		}
		i++;
	}
	if(!isDir) {
		if(parts.IsEmpty())
			parts.Add((UString)k_EmptyReplaceName);
		else {
			UString &s = parts.Back();
			if(s.IsEmpty())
				s = k_EmptyReplaceName;
		}
	}
}

UString FASTCALL MakePathFromParts(const UStringVector & parts)
{
	UString s;
	FOR_VECTOR(i, parts) {
		if(i != 0)
			s.Add_PathSepar();
		s += parts[i];
	}
	return s;
}
//
uint FASTCALL GetNumSlashes(const FChar * s)
{
	for(uint numSlashes = 0;; ) {
		FChar c = *s++;
		if(c == 0)
			return numSlashes;
		if(IS_PATH_SEPAR(c))
			numSlashes++;
	}
}