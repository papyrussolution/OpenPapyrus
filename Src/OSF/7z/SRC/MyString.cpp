// Common/MyString.cpp

#include <7z-internal.h>
#pragma hdrstop

#define MY_STRING_NEW(_T_, _size_) new _T_[_size_]
// #define MY_STRING_NEW(_T_, _size_) ((_T_ *)my_new((size_t)(_size_) * sizeof(_T_)))

/*
   inline const char* MyStringGetNextCharPointer(const char *p) throw()
   {
   #if defined(_WIN32) && !defined(UNDER_CE)
   return CharNextA(p);
   #else
   return p + 1;
   #endif
   }
 */

#define MY_STRING_NEW_char(_size_) MY_STRING_NEW(char, _size_)
#define MY_STRING_NEW_wchar_t(_size_) MY_STRING_NEW(wchar_t, _size_)

int FASTCALL FindCharPosInString(const char * s, char c)
{
	for(const char * p = s;; p++) {
		if(*p == c)
			return (int)(p - s);
		if(*p == 0)
			return -1;
		// MyStringGetNextCharPointer(p);
	}
}

int FASTCALL FindCharPosInString(const wchar_t * s, wchar_t c)
{
	for(const wchar_t * p = s;; p++) {
		if(*p == c)
			return (int)(p - s);
		if(*p == 0)
			return -1;
	}
}
/*
   void MyStringUpper_Ascii(char *s) throw()
   {
   for(;;)
   {
    char c = *s;
    if(c == 0)
      return;
   *s++ = MyCharUpper_Ascii(c);
   }
   }

   void MyStringUpper_Ascii(wchar_t *s) throw()
   {
   for(;;)
   {
    wchar_t c = *s;
    if(c == 0)
      return;
   *s++ = MyCharUpper_Ascii(c);
   }
   }
 */
void MyStringLower_Ascii(char * s) throw()
{
	for(;; ) {
		char c = *s;
		if(c == 0)
			return;
		*s++ = MyCharLower_Ascii(c);
	}
}

void MyStringLower_Ascii(wchar_t * s) throw()
{
	for(;; ) {
		wchar_t c = *s;
		if(c == 0)
			return;
		*s++ = MyCharLower_Ascii(c);
	}
}

#ifdef _WIN32

#ifdef _UNICODE

// wchar_t * MyStringUpper(wchar_t *s) { return CharUpperW(s); }
// wchar_t * MyStringLower(wchar_t *s) { return CharLowerW(s); }
// for WinCE - FString - char
// const char *MyStringGetPrevCharPointer(const char * /* base */, const char *p) { return p - 1; }

#else

// const char * MyStringGetPrevCharPointer(const char *base, const char *p) throw() { return CharPrevA(base, p); }
// char * MyStringUpper(char *s) { return CharUpperA(s); }
// char * MyStringLower(char *s) { return CharLowerA(s); }

wchar_t MyCharUpper_WIN(wchar_t c) throw()
{
	wchar_t * res = CharUpperW((LPWSTR)(UINT_PTR)(uint)c);
	if(res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
		return (wchar_t)(uint)(UINT_PTR)res;
	const int kBufSize = 4;
	char s[kBufSize + 1];
	int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufSize, 0, 0);
	if(numChars == 0 || numChars > kBufSize)
		return c;
	s[numChars] = 0;
	::CharUpperA(s);
	::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
	return c;
}

/*
   wchar_t MyCharLower_WIN(wchar_t c)
   {
   wchar_t *res = CharLowerW((LPWSTR)(UINT_PTR)(uint)c);
   if(res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)(uint)(UINT_PTR)res;
   const int kBufSize = 4;
   char s[kBufSize + 1];
   int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufSize, 0, 0);
   if(numChars == 0 || numChars > kBufSize)
    return c;
   s[numChars] = 0;
   ::CharLowerA(s);
   ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
   return c;
   }
 */

/*
   wchar_t * MyStringUpper(wchar_t *s)
   {
   if(s == 0)
    return 0;
   wchar_t *res = CharUpperW(s);
   if(res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
   AString a = UnicodeStringToMultiByte(s);
   a.MakeUpper();
   sstrcpy(s, (const wchar_t *)MultiByteToUnicodeString(a));
   return s;
   }
 */

/*
   wchar_t * MyStringLower(wchar_t *s)
   {
   if(s == 0)
    return 0;
   wchar_t *res = CharLowerW(s);
   if(res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
   AString a = UnicodeStringToMultiByte(s);
   a.MakeLower();
   sstrcpy(s, (const wchar_t *)MultiByteToUnicodeString(a));
   return s;
   }
 */

#endif

#endif

bool IsString1PrefixedByString2(const char * s1, const char * s2) throw()
{
	for(;; ) {
		uchar c2 = (uchar)*s2++; if(c2 == 0) return true;
		uchar c1 = (uchar)*s1++; if(c1 != c2) return false;
	}
}

bool StringsAreEqualNoCase(const wchar_t * s1, const wchar_t * s2) throw()
{
	for(;; ) {
		wchar_t c1 = *s1++;
		wchar_t c2 = *s2++;
		if(c1 != c2 && MyCharUpper(c1) != MyCharUpper(c2)) return false;
		if(c1 == 0) return true;
	}
}

// ---------- ASCII ----------

bool AString::IsPrefixedBy_Ascii_NoCase(const char * s) const throw()
{
	const char * s1 = _chars;
	for(;; ) {
		char c2 = *s++;
		if(c2 == 0)
			return true;
		char c1 = *s1++;
		if(MyCharLower_Ascii(c1) !=
		    MyCharLower_Ascii(c2))
			return false;
	}
}

bool UString::IsPrefixedBy_Ascii_NoCase(const char * s) const throw()
{
	const wchar_t * s1 = _chars;
	for(;; ) {
		char c2 = *s++;
		if(c2 == 0)
			return true;
		wchar_t c1 = *s1++;
		if(MyCharLower_Ascii(c1) != (uchar)MyCharLower_Ascii(c2))
			return false;
	}
}

bool StringsAreEqual_Ascii(const wchar_t * u, const char * a) throw()
{
	for(;; ) {
		uchar c = *a;
		if(c != *u)
			return false;
		if(c == 0)
			return true;
		a++;
		u++;
	}
}

bool StringsAreEqualNoCase_Ascii(const char * s1, const char * s2) throw()
{
	for(;; ) {
		char c1 = *s1++;
		char c2 = *s2++;
		if(c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
			return false;
		if(c1 == 0)
			return true;
	}
}

bool StringsAreEqualNoCase_Ascii(const wchar_t * s1, const wchar_t * s2) throw()
{
	for(;; ) {
		wchar_t c1 = *s1++;
		wchar_t c2 = *s2++;
		if(c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
			return false;
		if(c1 == 0)
			return true;
	}
}

bool StringsAreEqualNoCase_Ascii(const wchar_t * s1, const char * s2) throw()
{
	for(;; ) {
		wchar_t c1 = *s1++;
		char c2 = *s2++;
		if(c1 != (uchar)c2 && (c1 > 0x7F || MyCharLower_Ascii(c1) != (uchar)MyCharLower_Ascii(c2)))
			return false;
		if(c1 == 0)
			return true;
	}
}

bool IsString1PrefixedByString2(const wchar_t * s1, const wchar_t * s2) throw()
{
	for(;; ) {
		wchar_t c2 = *s2++; if(c2 == 0) return true;
		wchar_t c1 = *s1++; if(c1 != c2) return false;
	}
}

bool IsString1PrefixedByString2(const wchar_t * s1, const char * s2) throw()
{
	for(;; ) {
		uchar c2 = (uchar)(*s2++); if(c2 == 0) return true;
		wchar_t c1 = *s1++; if(c1 != c2) return false;
	}
}

bool IsString1PrefixedByString2_NoCase_Ascii(const wchar_t * s1, const char * s2) throw()
{
	for(;; ) {
		char c2 = *s2++; if(c2 == 0) return true;
		wchar_t c1 = *s1++;
		if(c1 != (uchar)c2 && MyCharLower_Ascii(c1) != (uchar)MyCharLower_Ascii(c2))
			return false;
	}
}

bool IsString1PrefixedByString2_NoCase(const wchar_t * s1, const wchar_t * s2) throw()
{
	for(;; ) {
		wchar_t c2 = *s2++; if(c2 == 0) return true;
		wchar_t c1 = *s1++;
		if(c1 != c2 && MyCharUpper(c1) != MyCharUpper(c2))
			return false;
	}
}

// NTFS order: uses upper case
int MyStringCompareNoCase(const wchar_t * s1, const wchar_t * s2) throw()
{
	for(;; ) {
		wchar_t c1 = *s1++;
		wchar_t c2 = *s2++;
		if(c1 != c2) {
			wchar_t u1 = MyCharUpper(c1);
			wchar_t u2 = MyCharUpper(c2);
			if(u1 < u2) return -1;
			if(u1 > u2) return 1;
		}
		if(c1 == 0) return 0;
	}
}

/*
   int MyStringCompareNoCase_N(const wchar_t *s1, const wchar_t *s2, unsigned num)
   {
   for(; num != 0; num--)
   {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if(c1 != c2)
    {
      wchar_t u1 = MyCharUpper(c1);
      wchar_t u2 = MyCharUpper(c2);
      if(u1 < u2) return -1;
      if(u1 > u2) return 1;
    }
    if(c1 == 0) return 0;
   }
   return 0;
   }
 */

// ---------- AString ----------

void AString::InsertSpace(unsigned &index, uint size)
{
	Grow(size);
	MoveItems(index + size, index);
}

#define k_Alloc_Len_Limit 0x40000000

void AString::ReAlloc(unsigned newLimit)
{
	if(newLimit < _len || newLimit >= k_Alloc_Len_Limit) throw 20130220;
	// MY_STRING_REALLOC(_chars, char, newLimit + 1, _len + 1);
	char * newBuf = MY_STRING_NEW_char(newLimit + 1);
	memcpy(newBuf, _chars, (size_t)(_len + 1));
	MY_STRING_DELETE(_chars);
	_chars = newBuf;
	_limit = newLimit;
}

void AString::ReAlloc2(unsigned newLimit)
{
	if(newLimit >= k_Alloc_Len_Limit) throw 20130220;
	// MY_STRING_REALLOC(_chars, char, newLimit + 1, 0);
	char * newBuf = MY_STRING_NEW_char(newLimit + 1);
	newBuf[0] = 0;
	MY_STRING_DELETE(_chars);
	_chars = newBuf;
	_limit = newLimit;
}

void AString::SetStartLen(unsigned len)
{
	_chars = 0;
	_chars = MY_STRING_NEW_char(len + 1);
	_len = len;
	_limit = len;
}

void AString::Grow_1()
{
	unsigned next = _len;
	next += next / 2;
	next += 16;
	next &= ~(uint)15;
	ReAlloc(next - 1);
}

void AString::Grow(unsigned n)
{
	unsigned freeSize = _limit - _len;
	if(n > freeSize) {
		unsigned next = _len + n;
		next += next / 2;
		next += 16;
		next &= ~(uint)15;
		ReAlloc(next - 1);
	}
}

AString::AString(unsigned num, const char * s)
{
	uint len = sstrlen(s);
	if(num > len)
		num = len;
	SetStartLen(num);
	memcpy(_chars, s, num);
	_chars[num] = 0;
}

AString::AString(unsigned num, const AString &s)
{
	if(num > s._len)
		num = s._len;
	SetStartLen(num);
	memcpy(_chars, s._chars, num);
	_chars[num] = 0;
}

AString::AString(const AString &s, char c)
{
	SetStartLen(s.Len() + 1);
	char * chars = _chars;
	uint len = s.Len();
	memcpy(chars, s, len);
	chars[len] = c;
	chars[(size_t)len + 1] = 0;
}

AString::AString(const char * s1, unsigned num1, const char * s2, unsigned num2)
{
	SetStartLen(num1 + num2);
	char * chars = _chars;
	memcpy(chars, s1, num1);
	memcpy(chars + num1, s2, num2 + 1);
}

AString operator + (const AString &s1, const AString &s2) { return AString(s1, s1.Len(), s2, s2.Len()); }
AString operator + (const AString &s1, const char    * s2) { return AString(s1, s1.Len(), s2, sstrlen(s2)); }
AString operator + (const char    * s1, const AString &s2) { return AString(s1, sstrlen(s1), s2, s2.Len()); }

static const uint kStartStringCapacity = 4;

AString::AString()
{
	_chars = MY_STRING_NEW_char(kStartStringCapacity);
	_len = 0;
	_limit = kStartStringCapacity - 1;
	_chars[0] = 0;
}

AString::AString(char c)
{
	SetStartLen(1);
	char * chars = _chars;
	chars[0] = c;
	chars[1] = 0;
}

AString::AString(const char * s)
{
	SetStartLen(sstrlen(s));
	sstrcpy(_chars, s);
}

AString::AString(const AString &s)
{
	SetStartLen(s._len);
	sstrcpy(_chars, s._chars);
}

AString &AString::operator=(char c)
{
	if(1 > _limit) {
		char * newBuf = MY_STRING_NEW_char(1 + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = 1;
	}
	_len = 1;
	char * chars = _chars;
	chars[0] = c;
	chars[1] = 0;
	return *this;
}

AString & AString::operator=(const char * s)
{
	uint len = sstrlen(s);
	if(len > _limit) {
		char * newBuf = MY_STRING_NEW_char(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	sstrcpy(_chars, s);
	return *this;
}

AString &AString::operator=(const AString &s)
{
	if(!(&s == this)) {
		uint len = s._len;
		if(len > _limit) {
			char * newBuf = MY_STRING_NEW_char(len + 1);
			MY_STRING_DELETE(_chars);
			_chars = newBuf;
			_limit = len;
		}
		_len = len;
		sstrcpy(_chars, s._chars);
	}
	return *this;
}

char * FASTCALL AString::GetBuf(unsigned minLen)
{
	if(minLen > _limit)
		ReAlloc2(minLen);
	return _chars;
}

char * FASTCALL AString::GetBuf_SetEnd(unsigned minLen)
{
	if(minLen > _limit)
		ReAlloc2(minLen);
	char * chars = _chars;
	chars[minLen] = 0;
	_len = minLen;
	return chars;
}

void FASTCALL AString::ReleaseBuf_SetLen(unsigned newLen) { _len = newLen; }

void FASTCALL AString::ReleaseBuf_SetEnd(unsigned newLen) 
{ 
	_len = newLen; 
	_chars[newLen] = 0; 
}

void FASTCALL AString::ReleaseBuf_CalcLen(unsigned maxLen)
{
	char * chars = _chars;
	chars[maxLen] = 0;
	_len = sstrlen(chars);
}

void FASTCALL AString::SetFromWStr_if_Ascii(const wchar_t * s)
{
	uint len = 0;
	{
		for(;; len++) {
			wchar_t c = s[len];
			if(c == 0)
				break;
			if(c >= 0x80)
				return;
		}
	}
	if(len > _limit) {
		char * newBuf = MY_STRING_NEW_char(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	char * dest = _chars;
	uint i;
	for(i = 0; i < len; i++)
		dest[i] = (char)s[i];
	dest[i] = 0;
}

/*
   void AString::SetFromBstr_if_Ascii(BSTR s)
   {
   uint len = ::SysStringLen(s);
   {
    for(unsigned i = 0; i < len; i++)
      if(s[i] <= 0 || s[i] >= 0x80)
        return;
   }
   if(len > _limit)
   {
    char *newBuf = MY_STRING_NEW_char(len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
   }
   _len = len;
   char *dest = _chars;
   uint i;
   for(i = 0; i < len; i++)
    dest[i] = (char)s[i];
   dest[i] = 0;
   }
 */

AString & FASTCALL AString::operator += (char c)
{
	if(_limit == _len)
		Grow_1();
	uint len = _len;
	char * chars = _chars;
	chars[len++] = c;
	chars[len] = 0;
	_len = len;
	return *this;
}

void AString::Add_Space() { operator += (' '); }
void AString::Add_Space_if_NotEmpty() { if(!IsEmpty()) Add_Space(); }
void AString::Add_LF() { operator += ('\n'); }

AString & FASTCALL AString::operator += (const char * s)
{
	uint len = sstrlen(s);
	Grow(len);
	sstrcpy(_chars + _len, s);
	_len += len;
	return *this;
}

void AString::Add_OptSpaced(const char * s)
{
	Add_Space_if_NotEmpty();
	(*this) += s;
}

AString & FASTCALL AString::operator += (const AString &s)
{
	Grow(s._len);
	sstrcpy(_chars + _len, s._chars);
	_len += s._len;
	return *this;
}

void AString::Add_UInt32(uint32 v)
{
	char sz[16];
	ConvertUInt32ToString(v, sz);
	(*this) += sz;
}

void AString::SetFrom(const char * s, unsigned len) // no check
{
	if(len > _limit) {
		char * newBuf = MY_STRING_NEW_char(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	if(len != 0)
		memcpy(_chars, s, len);
	_chars[len] = 0;
	_len = len;
}

void AString::SetFrom_CalcLen(const char * s, unsigned len) // no check
{
	uint i;
	for(i = 0; i < len; i++)
		if(s[i] == 0)
			break;
	SetFrom(s, i);
}

int AString::Find(const char * s, unsigned startIndex) const throw()
{
	const char * fs = strstr(_chars + startIndex, s);
	if(!fs)
		return -1;
	return (int)(fs - _chars);

	/*
	   if(s[0] == 0)
	   return startIndex;
	   uint len = sstrlen(s);
	   const char *p = _chars + startIndex;
	   for(;; p++)
	   {
	   const char c = *p;
	   if(c != s[0])
	   {
	    if(c == 0)
	      return -1;
	    continue;
	   }
	   uint i;
	   for(i = 1; i < len; i++)
	    if(p[i] != s[i])
	      break;
	   if(i == len)
	    return (int)(p - _chars);
	   }
	 */
}

bool AString::IsAscii() const
{
	uint   len = Len();
	const char * s = _chars;
	for(uint   i = 0; i < len; i++)
		if((uchar)s[i] >= 0x80)
			return false;
	return true;
}

int FASTCALL AString::Find(char c) const { return FindCharPosInString(_chars, c); }

int AString::Find(char c, unsigned startIndex) const
{
	int pos = FindCharPosInString(_chars + startIndex, c);
	return pos < 0 ? -1 : (int)startIndex + pos;
}

int AString::ReverseFind(char c) const throw()
{
	if(_len == 0)
		return -1;
	const char * p = _chars + _len - 1;
	for(;; ) {
		if(*p == c)
			return (int)(p - _chars);
		if(p == _chars)
			return -1;
		p--; // p = GetPrevCharPointer(_chars, p);
	}
}

int AString::ReverseFind_PathSepar() const throw()
{
	if(_len == 0)
		return -1;
	const char * p = _chars + _len - 1;
	for(;; ) {
		char c = *p;
		if(IS_PATH_SEPAR(c))
			return (int)(p - _chars);
		if(p == _chars)
			return -1;
		p--;
	}
}

void AString::TrimLeft() throw()
{
	const char * p = _chars;
	for(;; p++) {
		char c = *p;
		if(c != ' ' && c != '\n' && c != '\t')
			break;
	}
	unsigned pos = (uint)(p - _chars);
	if(pos != 0) {
		MoveItems(0, pos);
		_len -= pos;
	}
}

void AString::TrimRight() throw()
{
	const char * p = _chars;
	uint i;
	for(i = _len; i != 0; i--) {
		char c = p[(size_t)i - 1];
		if(c != ' ' && c != '\n' && c != '\t')
			break;
	}
	if(i != _len) {
		_chars[i] = 0;
		_len = i;
	}
}

void AString::InsertAtFront(char c)
{
	if(_limit == _len)
		Grow_1();
	MoveItems(1, 0);
	_chars[0] = c;
	_len++;
}

/*
   void AString::Insert(uint index, char c)
   {
   InsertSpace(index, 1);
   _chars[index] = c;
   _len++;
   }
 */

void AString::Insert(uint index, const char * s)
{
	unsigned num = sstrlen(s);
	if(num != 0) {
		InsertSpace(index, num);
		memcpy(_chars + index, s, num);
		_len += num;
	}
}

void AString::Insert(uint index, const AString &s)
{
	unsigned num = s.Len();
	if(num != 0) {
		InsertSpace(index, num);
		memcpy(_chars + index, s, num);
		_len += num;
	}
}

void AString::RemoveChar(char ch) throw()
{
	char * src = _chars;
	for(;; ) {
		char c = *src++;
		if(c == 0)
			return;
		if(c == ch)
			break;
	}
	char * dest = src - 1;
	for(;; ) {
		char c = *src++;
		if(c == 0)
			break;
		if(c != ch)
			*dest++ = c;
	}

	*dest = 0;
	_len = (uint)(dest - _chars);
}

// !!!!!!!!!!!!!!! test it if newChar = '\0'
void AString::Replace(char oldChar, char newChar) throw()
{
	if(oldChar == newChar)
		return;  // 0;
	// unsigned number = 0;
	int pos = 0;
	char * chars = _chars;
	while((uint)pos < _len) {
		pos = Find(oldChar, pos);
		if(pos < 0)
			break;
		chars[(uint)pos] = newChar;
		pos++;
		// number++;
	}
	return; //  number;
}

void AString::Replace(const AString &oldString, const AString &newString)
{
	if(oldString.IsEmpty())
		return;  // 0;
	if(oldString == newString)
		return;  // 0;
	unsigned oldLen = oldString.Len();
	unsigned newLen = newString.Len();
	// unsigned number = 0;
	int pos = 0;
	while((uint)pos < _len) {
		pos = Find(oldString, pos);
		if(pos < 0)
			break;
		Delete(pos, oldLen);
		Insert(pos, newString);
		pos += newLen;
		// number++;
	}
	// return number;
}

void AString::Delete(uint index) throw()
{
	MoveItems(index, index + 1);
	_len--;
}

void AString::Delete(uint index, unsigned count) throw()
{
	if(index + count > _len)
		count = _len - index;
	if(count > 0) {
		MoveItems(index, index + count);
		_len -= count;
	}
}

void AString::DeleteFrontal(unsigned num) throw()
{
	if(num != 0) {
		MoveItems(0, num);
		_len -= num;
	}
}

void AString::DeleteBack() { _chars[--_len] = 0; }

void FASTCALL AString::DeleteFrom(uint index)
{
	if(index < _len) {
		_len = index;
		_chars[index] = 0;
	}
}
/*
   AString operator+(const AString &s1, const AString &s2)
   {
   AString result(s1);
   result += s2;
   return result;
   }

   AString operator+(const AString &s, const char *chars)
   {
   AString result(s);
   result += chars;
   return result;
   }

   AString operator+(const char *chars, const AString &s)
   {
   AString result(chars);
   result += s;
   return result;
   }

   AString operator+(const AString &s, char c)
   {
   AString result(s);
   result += c;
   return result;
   }
 */

/*
   AString operator+(char c, const AString &s)
   {
   AString result(c);
   result += s;
   return result;
   }
 */

// ---------- UString ----------

void UString::MoveItems(unsigned dest, unsigned src)
{
	memmove(_chars + dest, _chars + src, (size_t)(_len - src + 1) * sizeof(wchar_t));
}

void UString::InsertSpace(uint index, uint size)
{
	Grow(size);
	MoveItems(index + size, index);
}

void UString::ReAlloc(unsigned newLimit)
{
	if(newLimit < _len || newLimit >= k_Alloc_Len_Limit) throw 20130221;
	// MY_STRING_REALLOC(_chars, wchar_t, newLimit + 1, _len + 1);
	wchar_t * newBuf = MY_STRING_NEW_wchar_t(newLimit + 1);
	wmemcpy(newBuf, _chars, _len + 1);
	MY_STRING_DELETE(_chars);
	_chars = newBuf;
	_limit = newLimit;
}

void UString::ReAlloc2(unsigned newLimit)
{
	if(newLimit >= k_Alloc_Len_Limit) throw 20130221;
	// MY_STRING_REALLOC(_chars, wchar_t, newLimit + 1, 0);
	wchar_t * newBuf = MY_STRING_NEW_wchar_t(newLimit + 1);
	newBuf[0] = 0;
	MY_STRING_DELETE(_chars);
	_chars = newBuf;
	_limit = newLimit;
}

void UString::SetStartLen(unsigned len)
{
	_chars = 0;
	_chars = MY_STRING_NEW_wchar_t(len + 1);
	_len = len;
	_limit = len;
}

void UString::Grow_1()
{
	unsigned next = _len;
	next += next / 2;
	next += 16;
	next &= ~(uint)15;
	ReAlloc(next - 1);
}

void UString::Grow(unsigned n)
{
	unsigned freeSize = _limit - _len;
	if(n > freeSize) {
		unsigned next = _len + n;
		next += next / 2;
		next += 16;
		next &= ~(uint)15;
		ReAlloc(next - 1);
	}
}

UString::UString(unsigned num, const wchar_t * s)
{
	uint len = sstrlen(s);
	SETMIN(num, len);
	SetStartLen(num);
	wmemcpy(_chars, s, num);
	_chars[num] = 0;
}

UString::UString(unsigned num, const UString &s)
{
	SETMIN(num, s._len);
	SetStartLen(num);
	wmemcpy(_chars, s._chars, num);
	_chars[num] = 0;
}

UString::UString(const UString &s, wchar_t c)
{
	SetStartLen(s.Len() + 1);
	wchar_t * chars = _chars;
	uint len = s.Len();
	wmemcpy(chars, s, len);
	chars[len] = c;
	chars[(size_t)len + 1] = 0;
}

UString::UString(const wchar_t * s1, unsigned num1, const wchar_t * s2, unsigned num2)
{
	SetStartLen(num1 + num2);
	wchar_t * chars = _chars;
	wmemcpy(chars, s1, num1);
	wmemcpy(chars + num1, s2, num2 + 1);
}

UString operator + (const UString &s1, const UString &s2) { return UString(s1, s1.Len(), s2, s2.Len()); }
UString operator + (const UString &s1, const wchar_t * s2) { return UString(s1, s1.Len(), s2, sstrlen(s2)); }
UString operator + (const wchar_t * s1, const UString &s2) { return UString(s1, sstrlen(s1), s2, s2.Len()); }

UString::UString() : _chars(MY_STRING_NEW_wchar_t(kStartStringCapacity)), _len(0), _limit(kStartStringCapacity - 1)
{
	_chars[0] = 0;
}

UString::UString(wchar_t c)
{
	SetStartLen(1);
	wchar_t * chars = _chars;
	chars[0] = c;
	chars[1] = 0;
}

UString::UString(char c)
{
	SetStartLen(1);
	wchar_t * chars = _chars;
	chars[0] = (uchar)c;
	chars[1] = 0;
}

UString::UString(const wchar_t * s)
{
	uint len = sstrlen(s);
	SetStartLen(len);
	wmemcpy(_chars, s, len + 1);
}

UString::UString(const char * s)
{
	uint len = sstrlen(s);
	SetStartLen(len);
	wchar_t * chars = _chars;
	for(uint i = 0; i < len; i++)
		chars[i] = (uchar)s[i];
	chars[len] = 0;
}

UString::UString(const UString &s)
{
	SetStartLen(s._len);
	wmemcpy(_chars, s._chars, s._len + 1);
}

UString::~UString() 
{
	MY_STRING_DELETE(_chars);
}

wchar_t * FASTCALL UString::GetBuf(unsigned minLen)
{
	if(minLen > _limit)
		ReAlloc2(minLen);
	return _chars;
}

wchar_t * FASTCALL UString::GetBuf_SetEnd(unsigned minLen)
{
	if(minLen > _limit)
		ReAlloc2(minLen);
	wchar_t * chars = _chars;
	chars[minLen] = 0;
	_len = minLen;
	return chars;
}

UString & FASTCALL UString::operator += (wchar_t c)
{
	if(_limit == _len)
		Grow_1();
	uint len = _len;
	wchar_t * chars = _chars;
	chars[len++] = c;
	chars[len] = 0;
	_len = len;
	return *this;
}

UString & UString::operator = (wchar_t c)
{
	if(1 > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(1 + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = 1;
	}
	_len = 1;
	wchar_t * chars = _chars;
	chars[0] = c;
	chars[1] = 0;
	return *this;
}

UString &UString::operator=(const wchar_t * s)
{
	uint len = sstrlen(s);
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	wmemcpy(_chars, s, len + 1);
	return *this;
}

UString &UString::operator=(const UString &s)
{
	if(&s == this)
		return *this;
	uint len = s._len;
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	wmemcpy(_chars, s._chars, len + 1);
	return *this;
}

void UString::SetFrom(const wchar_t * s, unsigned len) // no check
{
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	if(len != 0)
		wmemcpy(_chars, s, len);
	_chars[len] = 0;
	_len = len;
}

void UString::SetFromBstr(BSTR s)
{
	uint len = ::SysStringLen(s);
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	// if(s)
	wmemcpy(_chars, s, len + 1);
}

UString &UString::operator=(const char * s)
{
	uint len = sstrlen(s);
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	wchar_t * chars = _chars;
	for(uint i = 0; i < len; i++)
		chars[i] = (uchar)s[i];
	chars[len] = 0;
	_len = len;
	return *this;
}

void UString::Add_Space() 
{
	operator+=(L' ');
}

void UString::Add_Space_if_NotEmpty() 
{
	if(!IsEmpty()) 
		Add_Space();
}

void UString::Add_LF()
{
	if(_limit == _len)
		Grow_1();
	uint len = _len;
	wchar_t * chars = _chars;
	chars[len++] = L'\n';
	chars[len] = 0;
	_len = len;
}

UString &UString::operator+=(const wchar_t * s)
{
	uint len = sstrlen(s);
	Grow(len);
	wmemcpy(_chars + _len, s, len + 1);
	_len += len;
	return *this;
}

UString &UString::operator+=(const UString &s)
{
	Grow(s._len);
	wmemcpy(_chars + _len, s._chars, s._len + 1);
	_len += s._len;
	return *this;
}

UString &UString::operator+=(const char * s)
{
	uint len = sstrlen(s);
	Grow(len);
	wchar_t * chars = _chars + _len;
	for(uint i = 0; i < len; i++)
		chars[i] = (uchar)s[i];
	chars[len] = 0;
	_len += len;
	return *this;
}

bool UString::IsAscii() const
{
	uint   len = Len();
	const wchar_t * s = _chars;
	for(uint i = 0; i < len; i++)
		if(s[i] >= 0x80)
			return false;
	return true;
}

void UString::Add_UInt32(uint32 v)
{
	char sz[16];
	ConvertUInt32ToString(v, sz);
	(*this) += sz;
}

int UString::Find(const wchar_t * s, unsigned startIndex) const throw()
{
	const wchar_t * fs = wcsstr(_chars + startIndex, s);
	return fs ? (int)(fs - _chars) : -1;
	/*
	   if(s[0] == 0)
	   return startIndex;
	   uint len = sstrlen(s);
	   const wchar_t *p = _chars + startIndex;
	   for(;; p++)
	   {
	   const wchar_t c = *p;
	   if(c != s[0])
	   {
	    if(c == 0)
	      return -1;
	    continue;
	   }
	   uint i;
	   for(i = 1; i < len; i++)
	    if(p[i] != s[i])
	      break;
	   if(i == len)
	    return (int)(p - _chars);
	   }
	 */
}

int UString::ReverseFind(wchar_t c) const throw()
{
	if(_len == 0)
		return -1;
	const wchar_t * p = _chars + _len - 1;
	for(;; ) {
		if(*p == c)
			return (int)(p - _chars);
		if(p == _chars)
			return -1;
		p--;
	}
}

int UString::ReverseFind_PathSepar() const throw()
{
	if(_len == 0)
		return -1;
	const wchar_t * p = _chars + _len - 1;
	for(;; ) {
		wchar_t c = *p;
		if(IS_PATH_SEPAR(c))
			return (int)(p - _chars);
		if(p == _chars)
			return -1;
		p--;
	}
}

void UString::TrimLeft() throw()
{
	const wchar_t * p = _chars;
	for(;; p++) {
		wchar_t c = *p;
		if(c != ' ' && c != '\n' && c != '\t')
			break;
	}
	unsigned pos = (uint)(p - _chars);
	if(pos != 0) {
		MoveItems(0, pos);
		_len -= pos;
	}
}

void UString::TrimRight() throw()
{
	const wchar_t * p = _chars;
	uint i;
	for(i = _len; i != 0; i--) {
		wchar_t c = p[(size_t)i - 1];
		if(c != ' ' && c != '\n' && c != '\t')
			break;
	}
	if(i != _len) {
		_chars[i] = 0;
		_len = i;
	}
}

void UString::InsertAtFront(wchar_t c)
{
	if(_limit == _len)
		Grow_1();
	MoveItems(1, 0);
	_chars[0] = c;
	_len++;
}

/*
   void UString::Insert(uint index, wchar_t c)
   {
   InsertSpace(index, 1);
   _chars[index] = c;
   _len++;
   }
 */

void UString::Insert(uint index, const wchar_t * s)
{
	unsigned num = sstrlen(s);
	if(num != 0) {
		InsertSpace(index, num);
		wmemcpy(_chars + index, s, num);
		_len += num;
	}
}

void UString::Insert(uint index, const UString &s)
{
	unsigned num = s.Len();
	if(num != 0) {
		InsertSpace(index, num);
		wmemcpy(_chars + index, s, num);
		_len += num;
	}
}

void UString::RemoveChar(wchar_t ch) throw()
{
	wchar_t * src = _chars;

	for(;; ) {
		wchar_t c = *src++;
		if(c == 0)
			return;
		if(c == ch)
			break;
	}
	wchar_t * dest = src - 1;
	for(;; ) {
		wchar_t c = *src++;
		if(c == 0)
			break;
		if(c != ch)
			*dest++ = c;
	}
	*dest = 0;
	_len = (uint)(dest - _chars);
}

// !!!!!!!!!!!!!!! test it if newChar = '\0'
void UString::Replace(wchar_t oldChar, wchar_t newChar) throw()
{
	if(oldChar == newChar)
		return;  // 0;
	// unsigned number = 0;
	int pos = 0;
	wchar_t * chars = _chars;
	while((uint)pos < _len) {
		pos = Find(oldChar, pos);
		if(pos < 0)
			break;
		chars[(uint)pos] = newChar;
		pos++;
		// number++;
	}
	return; //  number;
}

void UString::Replace(const UString &oldString, const UString &newString)
{
	if(oldString.IsEmpty())
		return;  // 0;
	if(oldString == newString)
		return;  // 0;
	unsigned oldLen = oldString.Len();
	unsigned newLen = newString.Len();
	// unsigned number = 0;
	int pos = 0;
	while((uint)pos < _len) {
		pos = Find(oldString, pos);
		if(pos < 0)
			break;
		Delete(pos, oldLen);
		Insert(pos, newString);
		pos += newLen;
		// number++;
	}
	// return number;
}

void UString::Delete(uint index) throw()
{
	MoveItems(index, index + 1);
	_len--;
}

void UString::Delete(uint index, unsigned count) throw()
{
	if(index + count > _len)
		count = _len - index;
	if(count > 0) {
		MoveItems(index, index + count);
		_len -= count;
	}
}

void UString::DeleteFrontal(unsigned num) throw()
{
	if(num != 0) {
		MoveItems(0, num);
		_len -= num;
	}
}

// ---------- UString2 ----------

void UString2::ReAlloc2(unsigned newLimit)
{
	if(newLimit >= k_Alloc_Len_Limit) throw 20130221;
	// MY_STRING_REALLOC(_chars, wchar_t, newLimit + 1, 0);
	_chars = MY_STRING_NEW_wchar_t(newLimit + 1);
}

void UString2::SetStartLen(unsigned len)
{
	_chars = 0;
	_chars = MY_STRING_NEW_wchar_t(len + 1);
	_len = len;
}

/*
   UString2::UString2(wchar_t c)
   {
   SetStartLen(1);
   wchar_t *chars = _chars;
   chars[0] = c;
   chars[1] = 0;
   }
 */

UString2::UString2(const wchar_t * s)
{
	uint len = sstrlen(s);
	SetStartLen(len);
	wmemcpy(_chars, s, len + 1);
}

UString2::UString2(const UString2 &s) : _chars(NULL), _len(0)
{
	if(s._chars) {
		SetStartLen(s._len);
		wmemcpy(_chars, s._chars, s._len + 1);
	}
}

/*
   UString2 &UString2::operator=(wchar_t c)
   {
   if(1 > _len)
   {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t(1 + 1);
    if(_chars)
      MY_STRING_DELETE(_chars);
    _chars = newBuf;
   }
   _len = 1;
   wchar_t *chars = _chars;
   chars[0] = c;
   chars[1] = 0;
   return *this;
   }
 */

UString2 & UString2::operator=(const wchar_t * s)
{
	uint len = sstrlen(s);
	if(len > _len) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		if(_chars)
			MY_STRING_DELETE(_chars);
		_chars = newBuf;
	}
	_len = len;
	sstrcpy(_chars, s);
	return *this;
}

void UString2::SetFromAscii(const char * s)
{
	uint len = sstrlen(s);
	if(len > _len) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
		if(_chars)
			MY_STRING_DELETE(_chars);
		_chars = newBuf;
	}
	wchar_t * chars = _chars;
	for(uint i = 0; i < len; i++)
		chars[i] = (uchar)s[i];
	chars[len] = 0;
	_len = len;
}

UString2 & UString2::operator=(const UString2 &s)
{
	if(!(&s == this)) {
		uint len = s._len;
		if(len > _len) {
			wchar_t * newBuf = MY_STRING_NEW_wchar_t(len + 1);
			if(_chars)
				MY_STRING_DELETE(_chars);
			_chars = newBuf;
		}
		_len = len;
		sstrcpy(_chars, s._chars);
	}
	return *this;
}

bool operator==(const UString2 &s1, const UString2 &s2)
	{ return s1.Len() == s2.Len() && (s1.IsEmpty() || wcscmp(s1.GetRawPtr(), s2.GetRawPtr()) == 0); }
bool operator==(const UString2 &s1, const wchar_t * s2)
	{ return s1.IsEmpty() ? (*s2 == 0) : (wcscmp(s1.GetRawPtr(), s2) == 0); }
bool operator==(const wchar_t * s1, const UString2 &s2)
	{ return s2.IsEmpty() ? (*s1 == 0) : (wcscmp(s1, s2.GetRawPtr()) == 0); }

// ----------------------------------------

/*
   int MyStringCompareNoCase(const char *s1, const char *s2)
   {
   return MyStringCompareNoCase(MultiByteToUnicodeString(s1), MultiByteToUnicodeString(s2));
   }
 */

static inline UINT GetCurrentCodePage()
{
  #if defined(UNDER_CE) || !defined(_WIN32)
	return CP_ACP;
  #else
	return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  #endif
}

#ifdef USE_UNICODE_FSTRING
	#ifndef _UNICODE
		AString fs2fas(CFSTR s) { return UnicodeStringToMultiByte(s, GetCurrentCodePage()); }
		FString fas2fs(const char * s) { return MultiByteToUnicodeString(s, GetCurrentCodePage()); }
		FString fas2fs(const AString &s) { return MultiByteToUnicodeString(s, GetCurrentCodePage()); }
	#endif
#else
UString fs2us(const FChar * s) { return MultiByteToUnicodeString(s, GetCurrentCodePage()); }
UString fs2us(const FString &s) { return MultiByteToUnicodeString(s, GetCurrentCodePage()); }
FString us2fs(const wchar_t * s) { return UnicodeStringToMultiByte(s, GetCurrentCodePage()); }
#endif
