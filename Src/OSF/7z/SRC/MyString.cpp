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
		if(MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
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

/* @sobolev (replaced with sstreqi_ascii) bool StringsAreEqualNoCase_Ascii__(const char * s1, const char * s2) throw()
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

bool StringsAreEqualNoCase_Ascii__(const wchar_t * s1, const wchar_t * s2) throw()
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

bool StringsAreEqualNoCase_Ascii__(const wchar_t * s1, const char * s2) throw()
{
	for(;; ) {
		wchar_t c1 = *s1++;
		char c2 = *s2++;
		if(c1 != (uchar)c2 && (c1 > 0x7F || MyCharLower_Ascii(c1) != (uchar)MyCharLower_Ascii(c2)))
			return false;
		if(c1 == 0)
			return true;
	}
}*/

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
		uchar c2 = (uchar)(*s2++); 
		if(c2 == 0) 
			return true;
		wchar_t c1 = *s1++; 
		if(c1 != c2) 
			return false;
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
	_chars = MY_STRING_NEW_char(len+1);
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

AString &AString::operator = (char c)
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

AString & AString::operator = (const char * s)
{
	uint len = sstrlen(s);
	if(len > _limit) {
		char * newBuf = MY_STRING_NEW_char(len+1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	sstrcpy(_chars, s);
	return *this;
}

AString &AString::operator = (const AString &s)
{
	if(!(&s == this)) {
		uint len = s._len;
		if(len > _limit) {
			char * newBuf = MY_STRING_NEW_char(len+1);
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
		char * newBuf = MY_STRING_NEW_char(len+1);
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
    char *newBuf = MY_STRING_NEW_char(len+1);
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
		char * newBuf = MY_STRING_NEW_char(len+1);
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
	_chars = MY_STRING_NEW_wchar_t(len+1);
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
	chars[0] = static_cast<uchar>(c);
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

UString &UString::operator = (const wchar_t * s)
{
	uint len = sstrlen(s);
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	wmemcpy(_chars, s, len + 1);
	return *this;
}

UString &UString::operator = (const UString &s)
{
	if(&s == this)
		return *this;
	uint len = s._len;
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
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
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
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
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
		MY_STRING_DELETE(_chars);
		_chars = newBuf;
		_limit = len;
	}
	_len = len;
	// if(s)
	wmemcpy(_chars, s, len + 1);
}

UString &UString::operator = (const char * s)
{
	uint len = sstrlen(s);
	if(len > _limit) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
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
	_chars = MY_STRING_NEW_wchar_t(len+1);
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

/*UString2 &UString2::operator = (wchar_t c)
{
	if(1 > _len) {
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
}*/

UString2 & UString2::operator = (const wchar_t * s)
{
	uint len = sstrlen(s);
	if(len > _len) {
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
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
		wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
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

UString2 & UString2::operator = (const UString2 &s)
{
	if(!(&s == this)) {
		uint len = s._len;
		if(len > _len) {
			wchar_t * newBuf = MY_STRING_NEW_wchar_t(len+1);
			if(_chars)
				MY_STRING_DELETE(_chars);
			_chars = newBuf;
		}
		_len = len;
		sstrcpy(_chars, s._chars);
	}
	return *this;
}

bool operator == (const UString2 &s1, const UString2 &s2)
	{ return s1.Len() == s2.Len() && (s1.IsEmpty() || wcscmp(s1.GetRawPtr(), s2.GetRawPtr()) == 0); }
bool operator == (const UString2 &s1, const wchar_t * s2)
	{ return s1.IsEmpty() ? (*s2 == 0) : (wcscmp(s1.GetRawPtr(), s2) == 0); }
bool operator == (const wchar_t * s1, const UString2 &s2)
	{ return s2.IsEmpty() ? (*s1 == 0) : (wcscmp(s1, s2.GetRawPtr()) == 0); }

// ----------------------------------------

/*int MyStringCompareNoCase(const char *s1, const char *s2)
{
	return MyStringCompareNoCase(MultiByteToUnicodeString(s1), MultiByteToUnicodeString(s2));
}*/
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
//
// UTFConvert.cpp
#ifdef _WIN32
	#define _WCHART_IS_16BIT 1
#endif
/*
   _UTF8_START(n) - is a base value for start byte (head), if there are (n) additional bytes after start byte

   n : _UTF8_START(n) : Bits of code point

   0 : 0x80 :    : unused
   1 : 0xC0 : 11 :
   2 : 0xE0 : 16 : Basic Multilingual Plane
   3 : 0xF0 : 21 : Unicode space
   3 : 0xF8 : 26 :
   5 : 0xFC : 31 : UCS-4
   6 : 0xFE : 36 : We can use it, if we want to encode any 32-bit value
   7 : 0xFF :
 */
#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))
#define _UTF8_HEAD_PARSE2(n) if(c < _UTF8_START((n) + 1)) { numBytes = (n); c -= _UTF8_START(n); }
#define _UTF8_HEAD_PARSE _UTF8_HEAD_PARSE2(1) else _UTF8_HEAD_PARSE2(2) else _UTF8_HEAD_PARSE2(3) else _UTF8_HEAD_PARSE2(4) else _UTF8_HEAD_PARSE2(5) \
// else _UTF8_HEAD_PARSE2(6)

bool FASTCALL CheckUTF8(const char * src, bool allowReduced) throw()
{
	for(;; ) {
		Byte c = *src++;
		if(c == 0)
			return true;
		if(c < 0x80)
			continue;
		if(c < 0xC0) // (c < 0xC0 + 2) // if we support only optimal encoding chars
			return false;
		unsigned numBytes;
		_UTF8_HEAD_PARSE
		else
			return false;
		uint32 val = c;
		do {
			Byte c2 = *src++;
			if(c2 < 0x80 || c2 >= 0xC0)
				return allowReduced && c2 == 0;
			val <<= 6;
			val |= (c2 - 0x80);
		} while(--numBytes);
		if(val >= 0x110000)
			return false;
	}
}

#define _ERROR_UTF8 { if(dest) dest[destPos] = (wchar_t)0xFFFD; destPos++; ok = false; continue; }

static bool Utf8_To_Utf16(wchar_t * dest, size_t * destLen, const char * src, const char * srcLim) throw()
{
	size_t destPos = 0;
	bool ok = true;
	for(;; ) {
		Byte c;
		if(src == srcLim) {
			*destLen = destPos;
			return ok;
		}
		c = *src++;
		if(c < 0x80) {
			if(dest)
				dest[destPos] = (wchar_t)c;
			destPos++;
			continue;
		}
		if(c < 0xC0)
			_ERROR_UTF8
			unsigned numBytes;
		_UTF8_HEAD_PARSE
		else
			_ERROR_UTF8
		uint32 val = c;
		do {
			Byte c2;
			if(src == srcLim)
				break;
			c2 = *src;
			if(c2 < 0x80 || c2 >= 0xC0)
				break;
			src++;
			val <<= 6;
			val |= (c2 - 0x80);
		} while(--numBytes);
		if(numBytes != 0)
			_ERROR_UTF8
			if(val < 0x10000) {
				if(dest)
					dest[destPos] = (wchar_t)val;
				destPos++;
			}
			else {
				val -= 0x10000;
				if(val >= 0x100000)
					_ERROR_UTF8
					if(dest) {
						dest[destPos + 0] = (wchar_t)(0xD800 + (val >> 10));
						dest[destPos + 1] = (wchar_t)(0xDC00 + (val & 0x3FF));
					}
				destPos += 2;
			}
	}
}

#define _UTF8_RANGE(n) (((uint32)1) << ((n) * 5 + 6))
#define _UTF8_HEAD(n, val) ((char)(_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((char)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t Utf16_To_Utf8_Calc(const wchar_t * src, const wchar_t * srcLim)
{
	size_t size = srcLim - src;
	for(;; ) {
		if(src == srcLim)
			return size;
		uint32 val = *src++;
		if(val < 0x80)
			continue;
		if(val < _UTF8_RANGE(1)) {
			size++;
			continue;
		}
		if(val >= 0xD800 && val < 0xDC00 && src != srcLim) {
			uint32 c2 = *src;
			if(c2 >= 0xDC00 && c2 < 0xE000) {
				src++;
				size += 2;
				continue;
			}
		}
    #ifdef _WCHART_IS_16BIT
		size += 2;
    #else
		if(val < _UTF8_RANGE(2)) size += 2;
		else if(val < _UTF8_RANGE(3)) size += 3;
		else if(val < _UTF8_RANGE(4)) size += 4;
		else if(val < _UTF8_RANGE(5)) size += 5;
		else size += 6;
    #endif
	}
}

static char * Utf16_To_Utf8(char * dest, const wchar_t * src, const wchar_t * srcLim)
{
	for(;; ) {
		if(src == srcLim)
			return dest;
		uint32 val = *src++;
		if(val < 0x80) {
			*dest++ = (char)val;
			continue;
		}
		if(val < _UTF8_RANGE(1)) {
			dest[0] = _UTF8_HEAD(1, val);
			dest[1] = _UTF8_CHAR(0, val);
			dest += 2;
			continue;
		}
		if(val >= 0xD800 && val < 0xDC00 && src != srcLim) {
			uint32 c2 = *src;
			if(c2 >= 0xDC00 && c2 < 0xE000) {
				src++;
				val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
				dest[0] = _UTF8_HEAD(3, val);
				dest[1] = _UTF8_CHAR(2, val);
				dest[2] = _UTF8_CHAR(1, val);
				dest[3] = _UTF8_CHAR(0, val);
				dest += 4;
				continue;
			}
		}
    #ifndef _WCHART_IS_16BIT
		if(val < _UTF8_RANGE(2))
    #endif
		{
			dest[0] = _UTF8_HEAD(2, val);
			dest[1] = _UTF8_CHAR(1, val);
			dest[2] = _UTF8_CHAR(0, val);
			dest += 3;
			continue;
		}

    #ifndef _WCHART_IS_16BIT
		uint32 b;
		unsigned numBits;
		if(val < _UTF8_RANGE(3)) {
			numBits = 6 * 3; b = _UTF8_HEAD(3, val);
		}
		else if(val < _UTF8_RANGE(4)) {
			numBits = 6 * 4; b = _UTF8_HEAD(4, val);
		}
		else if(val < _UTF8_RANGE(5)) {
			numBits = 6 * 5; b = _UTF8_HEAD(5, val);
		}
		else {                           
			numBits = 6 * 6; b = _UTF8_START(6); 
		}
		*dest++ = (Byte)b;
		do {
			numBits -= 6;
			*dest++ = (char)(0x80 + ((val >> numBits) & 0x3F));
		} while(numBits != 0);
    #endif
	}
}

bool FASTCALL ConvertUTF8ToUnicode(const AString &src, UString &dest)
{
	dest.Empty();
	size_t destLen = 0;
	Utf8_To_Utf16(NULL, &destLen, src, src.Ptr(src.Len()));
	bool res = Utf8_To_Utf16(dest.GetBuf((uint)destLen), &destLen, src, src.Ptr(src.Len()));
	dest.ReleaseBuf_SetEnd((uint)destLen);
	return res;
}

void FASTCALL ConvertUnicodeToUTF8(const UString &src, AString &dest)
{
	dest.Empty();
	size_t destLen = Utf16_To_Utf8_Calc(src, src.Ptr(src.Len()));
	Utf16_To_Utf8(dest.GetBuf((uint)destLen), src, src.Ptr(src.Len()));
	dest.ReleaseBuf_SetEnd((uint)destLen);
}
