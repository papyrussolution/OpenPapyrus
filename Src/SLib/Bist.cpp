// BIST.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
// @threadsafe
// Реализация стандартных типов данных семейства SType
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
// @todo библиотека DB не должна включаться в slib #include <db.h> // DBRowId

class SVoid : public DataType {
public:
	SLAPI  SVoid() : DataType(0) {}
	int    SLAPI base() const { return BTS_VOID; }
};

class SInt : public DataType {
public:
	explicit SLAPI SInt(uint32 sz = 2);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_INT; }
	int    SLAPI tobase(const void *, void *) const;
	int    SLAPI baseto(void *, const void *) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

typedef SInt SAutoinc;

class SBool : public DataType {
public:
	explicit SLAPI SBool(uint32 sz = 4);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_BOOL; }
	int    SLAPI tobase(const void *, void *) const;
	int    SLAPI baseto(void *, const void *) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SUInt : public DataType {
public:
 	explicit SLAPI SUInt(uint32 sz = 2);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_INT; }
	int    SLAPI tobase(const void *, void *) const;
	int    SLAPI baseto(void *, const void *) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SFloat : public DataType {
public:
	explicit SLAPI SFloat(uint32 sz = 8);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_REAL; }
	int    SLAPI tobase(const void *, void *) const;
	int    SLAPI baseto(void *, const void *) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SDecimal : public DataType {
public:
	SLAPI  SDecimal(size_t sz = 8, size_t prec = 2);
	uint32 SLAPI size() const; // @v10.2.1 size_t-->uint32
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_REAL; }
	int    SLAPI tobase(const void *, void *) const;
	int    SLAPI baseto(void *, const void *) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
};

class SMoney : public SDecimal {
public:
	explicit SLAPI SMoney(size_t sz = 8) : SDecimal(sz, 2) {}
};

class SDate : public DataType {
public:
	SLAPI  SDate();
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_DATE; }
	int    SLAPI tobase(const void * s, void * b) const { memmove(b, s, size()); return 1; }
	int    SLAPI baseto(void * s, const void * b) const{ memmove(s, b, size()); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	virtual int SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class STime : public DataType {
public:
	SLAPI  STime();
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_TIME; }
	int    SLAPI tobase(const void * s, void * b) const { memmove(b, s, size()); return 1; }
	int    SLAPI baseto(void * s, const void * b) const { memmove(s, b, size()); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	virtual int SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SDateTime : public DataType {
public:
	SLAPI  SDateTime();
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_DATETIME; }
	int    SLAPI tobase(const void * s, void * b) const { memmove(b, s, size()); return 1; }
	int    SLAPI baseto(void * s, const void * b) const{ memmove(s, b, size()); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	virtual int SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SChar : public DataType {
public:
	explicit SLAPI SChar(uint32 sz = 0);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_STRING; }
	int    SLAPI tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); return 1; }
	int    SLAPI baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
};

class SZString : public DataType {
public:
	explicit SLAPI SZString(uint32 = 0);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_STRING; }
	int    SLAPI tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); return 1; }
	int    SLAPI baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	virtual int SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

typedef SZString SNote;

class SWcString : public DataType {
public:
	//
	// Размер указывается в байтах (не символах)
	//
	explicit SLAPI SWcString(uint32 = sizeof(wchar_t));
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_STRING; }
	int    SLAPI tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); return 1; }
	int    SLAPI baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
};

class SLString : public DataType {
public:
	explicit SLAPI SLString(uint32 = 0);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const { return BTS_STRING; }
	int    SLAPI tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); return 1; }
	int    SLAPI baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
};

class SVariant : public DataType {
public:
	SLAPI  SVariant() : DataType(sizeof(VARIANT)) {}
};

class SRaw : public DataType {
public:
	explicit SLAPI SRaw(uint32 sz = 0);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const;
	int    SLAPI tobase(const void * s, void * b) const;
	int    SLAPI baseto(void * s, const void * b) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
};

class SIPoint2 : public DataType {
public:
	explicit SLAPI SIPoint2(uint32 sz = 4);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const;
	int    SLAPI tobase(const void * s, void * b) const;
	int    SLAPI baseto(void * s, const void * b) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SFPoint2 : public DataType {
public:
	explicit SLAPI SFPoint2(uint32 sz = 8);
	int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const;
	int    SLAPI tobase(const void * s, void * b) const;
	int    SLAPI baseto(void * s, const void * b) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SGuid : public DataType {
public:
	SLAPI  SGuid();
	// @baseused int    SLAPI comp(const void *, const void *) const;
	char * SLAPI tostr(const void *, long, char *) const;
	int    SLAPI fromstr(void *, long, const char *) const;
	int    SLAPI base() const;
	int    SLAPI tobase(const void * s, void * b) const;
	int    SLAPI baseto(void * s, const void * b) const;
	void   SLAPI minval(void *) const;
	void   SLAPI maxval(void *) const;
	// @baseused int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};
//
// Функция регистрации встроенных типов
//
void SLAPI RegisterBIST()
{
	RegisterSType(S_VOID,     &SVoid());
	RegisterSType(S_CHAR,     &SChar());
	RegisterSType(S_INT,      &SInt());
	RegisterSType(S_BOOL,     &SBool());
	RegisterSType(S_UINT,     &SUInt());
	RegisterSType(S_LOGICAL,  &SBool());
	RegisterSType(S_FLOAT,    &SFloat());
	RegisterSType(S_DEC,      &SDecimal());
	RegisterSType(S_MONEY,    &SMoney());
	RegisterSType(S_DATE,     &SDate());
	RegisterSType(S_DATETIME, &SDateTime());
	RegisterSType(S_TIME,     &STime());
	RegisterSType(S_ZSTRING,  &SZString());
	RegisterSType(S_AUTOINC,  &SAutoinc());
	RegisterSType(S_NOTE,     &SNote());
	RegisterSType(S_LSTRING,  &SLString());
	RegisterSType(S_VARIANT,  &SVariant());
	RegisterSType(S_WCHAR,    &SWcString());
	RegisterSType(S_WZSTRING, &SWcString());
	RegisterSType(S_RAW,      &SRaw());
	RegisterSType(S_IPOINT2,  &SIPoint2());
	RegisterSType(S_FPOINT2,  &SFPoint2());
	RegisterSType(S_UUID_,    &SGuid());
}
//
// SChar
//
SLAPI SChar::SChar(uint32 sz) : DataType(sz)
{
}

int SLAPI SChar::comp(const void * i1, const void * i2) const
{
	return S ? strncmp(static_cast<const char *>(i1), static_cast<const char *>(i2), S) : strcmp(static_cast<const char *>(i1), static_cast<const char *>(i2));
}

char * SLAPI SChar::tostr(const void * d, long fmt, char * buf) const
{
	return strfmt(strnzcpy(buf, static_cast<const char *>(d), S), fmt, buf);
}

int SLAPI SChar::fromstr(void * d, long, const char * buf) const
{
	if(S) {
		memset(d, ' ', S);
		const size_t bl = sstrlen(buf);
		memmove(static_cast<char *>(d), buf, MIN(bl, S));
		return 1;
	}
	else {
		strcpy(static_cast<char *>(d), buf);
		return 0;
	}
}

void SLAPI SChar::minval(void * d) const
{
	if(S)
		memset(d, ' ', S);
	else
		*static_cast<char *>(d) = 0;
}

void SLAPI SChar::maxval(void * d) const
{
	if(S)
		memset(d, 254, S);
	else {
		*static_cast<char *>(d) = (char)254;
		static_cast<char *>(d)[1] = 0;
	}
}
//
//
//
SLAPI SWcString::SWcString(uint32 sz) : DataType(ALIGNSIZE(sz, 1))
{
}

int SLAPI SWcString::comp(const void * i1, const void * i2) const
{
	return S ? wcsncmp(static_cast<const wchar_t *>(i1), static_cast<const wchar_t *>(i2), S/2) : wcscmp(static_cast<const wchar_t *>(i1), static_cast<const wchar_t *>(i2));
}

char * SLAPI SWcString::tostr(const void * d, long fmt, char * buf) const
{
	WideCharToMultiByte(CP_OEMCP, 0, static_cast<const wchar_t *>(d), -1, buf, 254, 0, 0);
	return buf;
}

int SLAPI SWcString::fromstr(void * d, long, const char * buf) const
{
	MultiByteToWideChar(CP_OEMCP, 0, buf, -1, static_cast<wchar_t *>(d), 254);
	return 1;
}

void SLAPI SWcString::minval(void * d) const
{
	static_cast<wchar_t *>(d)[0] = 0;
}

void SLAPI SWcString::maxval(void * d) const
{
	static_cast<wchar_t *>(d)[0] = static_cast<wchar_t>(MAXSHORT);
	static_cast<wchar_t *>(d)[1] = 0;
}
//
// SZString
//
SLAPI SZString::SZString(uint32 sz) : DataType(sz)
{
}

#pragma option -K

int SLAPI SZString::comp(const void * i1, const void * i2) const
{
	return S ? strncmp(static_cast<const char *>(i1), static_cast<const char *>(i2), S) : strcmp(static_cast<const char *>(i1), static_cast<const char *>(i2));
}

#pragma option -K.

char * SLAPI SZString::tostr(const void * d, long fmt, char * buf) const
{
	return strfmt(strnzcpy(buf, static_cast<const char *>(d), S), fmt, buf);
}

int SLAPI SZString::fromstr(void * d, long, const char * buf) const
{
	strnzcpy(static_cast<char *>(d), buf, S);
	return 1;
}

void SLAPI SZString::minval(void * d) const
{
	static_cast<char *>(d)[0] = 0;
}

void SLAPI SZString::maxval(void * d) const
{
	memset(d, 255, S-1);
	PTR8(d)[S-1] = 0;
	// ((char *) d)[0] = (char)254;
	// ((char *) d)[1] = 0;
}

int SLAPI SZString::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		size_t sz = size();
		size_t len = sstrlen(static_cast<const char *>(pData));
		if(len == 0) {
			*pInd = 1;
		}
		else if(len <= 0x000000ffU) {
			*pInd = 3;
			uint8 _len = static_cast<uint8>(len);
			rBuf.Write(&_len, sizeof(_len));
			rBuf.Write(pData, _len);
		}
		else if(len <= 0x0000ffffU) {
			*pInd = 0;
			uint16 _len = static_cast<uint16>(len);
			rBuf.Write(_len);
			rBuf.Write(pData, _len);
		}
		else {
			*pInd = 2;
			uint32 _len = static_cast<uint32>(len);
			rBuf.Write(_len);
			rBuf.Write(pData, _len);
		}
	}
	else if(dir < 0) {
		size_t sz = size();
		if(*pInd == 1) {
			PTR16(pData)[0] = 0;
		}
		else if(*pInd == 0) {
			uint16 _len;
			rBuf.Read(_len);
			rBuf.Read(pData, _len);
			PTR8(pData)[_len] = 0;
		}
		else if(*pInd == 2) {
			uint32 _len;
			rBuf.Read(_len);
			rBuf.Read(pData, _len);
			PTR8(pData)[_len] = 0;
		}
		else if(*pInd == 3) {
			uint8 _len;
			rBuf.Read(&_len, sizeof(_len));
			rBuf.Read(pData, _len);
			PTR8(pData)[_len] = 0;
		}
		else {
			THROW_S(0, SLERR_SRLZ_INVDATAIND);
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI SLString::SLString(uint32 sz) : DataType(sz)
{
}

#pragma option -K

int SLAPI SLString::comp(const void * i1, const void * i2) const
{
	const size_t l1 = *static_cast<const char *>(i1);
	const size_t l2 = *static_cast<const char *>(i2);
	return strncmp(static_cast<const char *>(i1)+1, static_cast<const char *>(i2)+1, MIN(l1, l2));
}

#pragma option -K.

char * SLAPI SLString::tostr(const void * d, long fmt, char * buf) const
{
	size_t l = *static_cast<const char *>(d);
	return strfmt(strnzcpy(buf, ((char *)d)+1, MIN(S, l+1)), fmt, buf);
}

int SLAPI SLString::fromstr(void * d, long, const char * buf) const
{
	size_t l = sstrlen(buf);
	(*(char *)d) = (char)l;
	strncpy(((char *)d)+1, buf, l);
	return 1;
}

void SLAPI SLString::minval(void * d) const
{
	((char *)d)[0] = 0;
}

void SLAPI SLString::maxval(void * d) const
{
	((char *)d)[0] = 0;
}
//
// SInt
//
static long FASTCALL _tolong(const void * d, int sz)
{
	switch(sz) {
		case 4: return *static_cast<const long *>(d);
		case 2: return (long)*static_cast<const int16 *>(d);
		case 1: return (long)*static_cast<const int8 *>(d);
		case 8: return (long)*static_cast<const int64 *>(d);
	}
	return 0L;
}

static void FASTCALL _longto(long v, void * d, int sz)
{
	switch(sz) {
		case 4: *(int32 *)d = v; break;
		case 2: *(int16 *)d = (int16)v; break;
		case 1: *(int8 *)d = (int8)v; break;
		case 8: *(int64 *)d = (int64)v; break;
	}
}

SLAPI SInt::SInt(uint32 sz) : DataType(sz)
{
}

int SLAPI SInt::comp(const void * i1, const void * i2) const
{
	if(S == 0) {
		const int64 l1 = *(int64 *)i1;
		const int64 l2 = *(int64 *)i2;
		return CMPSIGN(l1, l2);
	}
	else {
		const long   l1 = _tolong(i1, S);
		const long   l2 = _tolong(i2, S);
		return CMPSIGN(l1, l2);
	}
}

char * SLAPI SInt::tostr(const void * d, long fmt, char * buf) const
{
	if(S == 8)
		return int64fmt(*(int64 *)d, fmt, buf);
	else
		return intfmt(_tolong(d, S), fmt, buf);
}

int SLAPI SInt::fromstr(void * d, long, const char * buf) const
{
	long   lv;
	int    r = strtolong(buf, &lv);
	_longto(lv, d, S);
	return r;
}

int SLAPI SInt::tobase(const void * d, void * baseData) const
{
	return ((*(int32 *)baseData = _tolong(d, S)), 1);
}

int SLAPI SInt::baseto(void * d, const void * baseData) const
{
	return (_longto(*(int32 *)baseData, d, S), 1);
}

void SLAPI SInt::minval(void * d) const
{
	switch(S) {
		case 2: *(int16 *)d = SHRT_MIN;  break;
		case 4: *(int32 *)d = LONG_MIN;  break;
		case 1: *(int8  *)d = SCHAR_MIN; break;
		case 8: *(int64 *)d = LLONG_MIN; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

void SLAPI SInt::maxval(void * d) const
{
	switch(S) {
		case 2: *(int16 *)d = SHRT_MAX; break;
		case 4: *(int32 *)d  = LONG_MAX; break;
		case 1: *(int8  *)d  = SCHAR_MAX; break;
		case 8: *(int64 *)d  = LLONG_MAX; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

int SLAPI SInt::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - zero, 2 - full
		if(S == 8) {
			const int64 v64 = *(int64 *)pData;
			if(v64 == 0)
				spec = 1;
			else {
				const int64 _a = _abs64(v64);
				if(_a <= 0x7fL) {
					*pInd = 2;
					int8 _v = (int8)v64;
					rBuf.Write(&_v, sizeof(_v));
				}
				else if(_a <= 0x7fffL) {
					*pInd = 3;
					int16 _v = (int16)v64;
					rBuf.Write(_v);
				}
				else if(_a <= 0x7fffffffL) {
					*pInd = 4;
					int32 _v = (int32)v64;
					rBuf.Write(_v);
				}
				else
					spec = 2;
			}
		}
		else {
			const long v = _tolong(pData, S);
			if(v == 0)
				spec = 1;
			else {
				if(S > 1 && labs(v) <= 0x7fL) {
					*pInd = 2;
					int8 _v = (int8)v;
					rBuf.Write(&_v, sizeof(_v));
				}
				else if(S > 2 && labs(v) <= 0x7fffL) {
					*pInd = 3;
					int16 _v = (int16)v;
					rBuf.Write(_v);
				}
				else
					spec = 2;
			}
		}
		if(spec == 1)
			*pInd = 1;
		else if(spec == 2) {
			*pInd = 0;
			rBuf.Write(pData, S);
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else if(*pInd == 0) {
			rBuf.Read(pData, S);
		}
		else {
			if(S == 8) {
				if(*pInd == 2) {
					int8 _v;
					rBuf.Read(&_v, sizeof(_v));
					*(int64 *)pData = _v;
				}
				else if(*pInd == 3) {
					int16 _v;
					rBuf.Read(_v);
					*(int64 *)pData = _v;
				}
				else if(*pInd == 4) {
					int32 _v;
					rBuf.Read(_v);
					*(int64 *)pData = _v;
				}
				else {
					THROW_S(0, SLERR_SRLZ_INVDATAIND);
				}
			}
			else {
				long v = 0;
				if(*pInd == 2) {
					int8 _v;
					rBuf.Read(&_v, sizeof(_v));
					v = _v;
				}
				else if(*pInd == 3) {
					int16 _v;
					rBuf.Read(_v);
					v = _v;
				}
				else {
					THROW_S(0, SLERR_SRLZ_INVDATAIND);
				}
				_longto(v, pData, S);
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI SBool::SBool(uint32 sz) : DataType(sz)
{
}

int SLAPI SBool::comp(const void * p1, const void * p2) const
{
	const long   l1 = BIN(_tolong(p1, S));
	const long   l2 = BIN(_tolong(p2, S));
	return CMPSIGN(l1, l2);
}

char * SLAPI SBool::tostr(const void * d, long fmt, char * buf) const
{
	const long b = BIN(_tolong(d, S));
	return strcpy(buf, b ? "true" : "false");
}

int SLAPI SBool::fromstr(void * d, long fmt, const char * buf) const
{
	int  ok = 1;
	long b = 0;
	if(sstreqi_ascii(buf, "true") || sstreqi_ascii(buf, "yes") || sstreqi_ascii(buf, "t"))
		b = 1;
	else if(sstreqi_ascii(buf, "false") || sstreqi_ascii(buf, "no") || sstreqi_ascii(buf, "f"))
		b = 0;
	else {
		char * p_end = 0;
		b = BIN(strtol(buf, &p_end, 10));
	}
	_longto(b, d, S);
	return ok;
}

int SLAPI SBool::tobase(const void * pData, void * pBaseData) const
{
	return ((*(int32 *)pBaseData = _tolong(pData, S)), 1);
}

int SLAPI SBool::baseto(void * pData, const void * pBaseData) const
{
	return (_longto(*(int32 *)pBaseData, pData, S), 1);
}

void SLAPI SBool::minval(void * pData) const
{
	switch(S) {
		case 2: *(int16 *)pData = 0;  break;
		case 4: *(int32 *)pData = 0;  break;
		case 1: *(int8  *)pData = 0; break;
		case 8: *(int64 *)pData = 0; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

void SLAPI SBool::maxval(void * pData) const
{
	switch(S) {
		case 2: *(int16 *)pData = 1; break;
		case 4: *(int32 *)pData = 1; break;
		case 1: *(int8  *)pData = 1; break;
		case 8: *(int64 *)pData = 1; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

int SLAPI SBool::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - false, 2 - true
		if(S == 8) {
			const int64 v64 = *(int64 *)pData;
			if(v64 == 0)
				spec = 1;
			else
				spec = 2;
		}
		else {
			const long v = _tolong(pData, S);
			if(v == 0)
				spec = 1;
			else
				spec = 2;
		}
		*pInd = spec;
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else if(*pInd == 2) {
			if(S == 8)
				*static_cast<int64 *>(pData) = 1;
			else
				_longto(1, pData, S);
		}
		else {
			THROW_S(0, SLERR_SRLZ_INVDATAIND);
		}
	}
	CATCHZOK
	return ok;
}
//
// STUInt
//
#pragma warn -rvl

static ulong FASTCALL _toulong(const void * d, int sz)
{
	switch(sz) {
		case 4: return *static_cast<const ulong *>(d);
		case 2: return (ulong)*static_cast<const uint16 *>(d);
		case 1: return (ulong)*static_cast<const uchar *>(d);
		case 8: return (ulong)*static_cast<const uint64 *>(d);
	}
	return 0;
}

static void FASTCALL _ulongto(ulong ul, void * d, int sz)
{
	switch(sz) {
		case 4: *static_cast<ulong  *>(d) = ul; break;
		case 2: *static_cast<uint16 *>(d) = (uint16)ul; break;
		case 1: *static_cast<uint8  *>(d) = (uint8)ul; break;
		case 8: *static_cast<uint64 *>(d) = (uint64)ul; break;
	}
}

#pragma warn +rvl

SLAPI SUInt::SUInt(uint32 sz) : DataType(sz)
{
}

int SLAPI SUInt::comp(const void * i1, const void * i2) const
{
	if(S == 8) {
		const uint64 v1 = *static_cast<const uint64 *>(i1);
		const uint64 v2 = *static_cast<const uint64 *>(i2);
		return CMPSIGN(v1, v2);
	}
	else {
		const ulong  v1 = _toulong(i1, S);
		const ulong  v2 = _toulong(i2, S);
		return CMPSIGN(v1, v2);
	}
}

char * SLAPI SUInt::tostr(const void * d, long fmt, char * buf) const
{
	if(S == 8)
		return uint64fmt(*(uint64 *)d, fmt, buf);
	else
		return uintfmt(_toulong(d, S), fmt, buf);
}

int SLAPI SUInt::fromstr(void * d, long, const char * buf) const
{
	ulong  lv;
	int    r = strtoulong(buf, &lv);
	_ulongto(lv, d, S);
	return r;
}

int SLAPI SUInt::tobase(const void * d, void * baseData) const { return ((*(long *)baseData = (long)_toulong(d, S)), 1); }
int SLAPI SUInt::baseto(void * d, const void * baseData) const { return (_ulongto((ulong)*(long *)baseData, d, S), 1); }
void SLAPI SUInt::minval(void * d) const { _ulongto(0L, d, S); }

void SLAPI SUInt::maxval(void * d) const
{
	switch(S) {
		case 1: *static_cast<uint8 *>(d)  = 255; break;
		case 2: *static_cast<uint16 *>(d) = USHRT_MAX; break;
		case 4: *static_cast<ulong *>(d)  = ULONG_MAX; break;
		default: ; //CHECK(INVALID_DATA_SIZE);
	}
}

int SLAPI SUInt::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - zero, 2 - full
		if(S == 8) {
			uint64 v64 = *(uint64 *)pData;
			if(v64 == 0)
				spec = 1;
			else if(v64 <= 0xffUL) {
				*pInd = 2;
				uint8 _v = (uint8)v64;
				rBuf.Write(&_v, sizeof(_v));
			}
			else if(v64 <= 0xffffUL) {
				*pInd = 3;
				uint16 _v = (uint16)v64;
				rBuf.Write(_v);
			}
			else if(v64 <= 0xffffffffUL) {
				*pInd = 4;
				uint32 _v = (uint32)v64;
				rBuf.Write(_v);
			}
			else
				spec = 2;
		}
		else {
			ulong v = _toulong(pData, S);
			if(v == 0)
				spec = 1;
			else if(S > 1 && v <= 0xffUL) {
				*pInd = 2;
				uint8 _v = static_cast<uint8>(v);
				rBuf.Write(&_v, sizeof(_v));
			}
			else if(S > 2 && v <= 0xffffUL) {
				*pInd = 3;
				uint16 _v = static_cast<uint16>(v);
				rBuf.Write(_v);
			}
			else
				spec = 2;
		}
		if(spec == 1)
			*pInd = 1;
		else if(spec == 2) {
			*pInd = 0;
			rBuf.Write(pData, S);
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else if(*pInd == 0) {
			rBuf.Read(pData, S);
		}
		else if(S == 8) {
			if(*pInd == 2) {
				uint8 _v;
				rBuf.Read(&_v, sizeof(_v));
				*(uint64 *)pData = _v;
			}
			else if(*pInd == 3) {
				uint16 _v;
				rBuf.Read(_v);
				*(uint64 *)pData = _v;
			}
			else if(*pInd == 4) {
				uint32 _v;
				rBuf.Read(_v);
				*(uint64 *)pData = _v;
			}
			else {
				THROW_S(0, SLERR_SRLZ_INVDATAIND);
			}
		}
		else {
			ulong v = 0;
			if(*pInd == 2) {
				uint8 _v;
				rBuf.Read(&_v, sizeof(_v));
				v = _v;
			}
			else if(*pInd == 3) {
				uint16 _v;
				rBuf.Read(_v);
				v = _v;
			}
			else {
				THROW_S(0, SLERR_SRLZ_INVDATAIND);
			}
			_ulongto(v, pData, S);
		}
	}
	CATCHZOK
	return ok;
}
//
// SFloat
//
SLAPI SFloat::SFloat(uint32 sz) : DataType(sz)
{
}

#pragma warn -rvl

static double FASTCALL __toldbl(const void * d, int s)
{
	switch(s) {
		case 8: return *static_cast<const double *>(d);
		case 4: return (double)*static_cast<const float *>(d);
		case 10: return (double)*static_cast<const long double *>(d);
	}
	return 0.0;
}

#pragma warn +rvl

static void SLAPI __ldblto(double v, void * d, int s)
{
	switch(s) {
		case 8: *(double *)d = v; break;
		case 4: *(float *)d = (float)v; break;
		case 10: *(long double *)d = v; break;
		default: break;
	}
}

int SLAPI SFloat::comp(const void * i1, const void * i2) const
{
	const LDBL v1 = __toldbl(i1, S);
	const LDBL v2 = __toldbl(i2, S);
	return CMPSIGN(v1, v2);
}

char * SLAPI SFloat::tostr(const void * d, long fmt, char * buf) const
{
	long f;
	if(SFMTFLAG(fmt) & COMF_SQL)
		f = MKSFMTD(0, 6, NMBF_NOTRAILZ);
	else if(fmt == 0)
		f = MKSFMTD(0, 6, NMBF_NOTRAILZ);
	else
		f = fmt;
	return realfmt(__toldbl(d, S), f, buf);
}

int SLAPI SFloat::fromstr(void * d, long, const char * buf) const
{
	double v;
	int r = strtodoub(buf, &v);
	__ldblto(v, d, S);
	return r;
}

int SLAPI SFloat::tobase(const void * d, void * baseData) const
	{ return ((*(double *)baseData = __toldbl(d, S)), 1); }
int SLAPI SFloat::baseto(void * d, const void * baseData) const
	{ return (__ldblto(*(double *)baseData, d, S), 1); }

//static const double min_dbl = MINDOUBLE;
//static const float min_flt = MINFLOAT;
static const float max_flt = (float)MAXFLOAT;

void SLAPI SFloat::minval(void * d) const
{
	switch(S) {
		case  8: *static_cast<double *>(d) = -SMathConst::Max; break;
		case  4: *static_cast<float *>(d)  = -max_flt; break;
		case 10: *static_cast<LDBL *>(d)   = -SMathConst::Max; break;
		// default: assert(INVALID_DATA_SIZE);
	}
}

void SLAPI SFloat::maxval(void * d) const
{
	switch(S) {
		case  8: *static_cast<double *>(d) = SMathConst::Max; break;
		case  4: *static_cast<float *>(d)  = max_flt; break;
		case 10: *static_cast<LDBL *>(d)   = SMathConst::Max; break;
		// default: assert(INVALID_DATA_SIZE);
	}
}

int SLAPI SFloat::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		double v = __toldbl(pData, S);
		if(v == 0.0) {
			*pInd = 1;
		}
		else {
			long   lv = static_cast<long>(v);
			if((double)lv == v) {
				//
				// Целочисленное значение
				//
				if(labs(lv) <= 0x7f) {
					*pInd = 2;
					int8 _v = (int8)lv;
					rBuf.Write(&_v, sizeof(_v));
				}
				else if(labs(lv) <= 0x7fff) {
					*pInd = 3;
					int16 _v = (int16)lv;
					rBuf.Write(_v);
				}
				else {
					*pInd = 4;
					rBuf.Write(lv);
				}
			}
			else if(S == sizeof(double)) {
				//
				// Если преобразование (double)-->(float) не приводит к потере
				// значимости, то в потоке сохраняем значение как float
				//
				float fv = static_cast<float>(v);
				double dv = fv;
				if(dv == v) {
					*pInd = 5;
					rBuf.WriteFloat(fv);
				}
				else {
					*pInd = 0;
					rBuf.Write(pData, S);
				}
			}
			else {
				*pInd = 0;
				rBuf.Write(pData, S);
			}
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else {
			if(*pInd == 0) {
				rBuf.Read(pData, S);
			}
			else {
				double v = 0.0;
				if(*pInd == 2) {
					int8 _v;
					rBuf.Read(&_v, sizeof(_v));
					v = _v;
				}
				else if(*pInd == 3) {
					int16 _v;
					rBuf.Read(_v);
					v = _v;
				}
				else if(*pInd == 4) {
					long _v;
					rBuf.Read(_v);
					v = _v;
				}
				else if(*pInd == 5) {
					float _v;
					rBuf.Read(_v);
					v = _v;
				}
				else {
					THROW_S(0, SLERR_SRLZ_INVDATAIND);
				}
				__ldblto(v, pData, S);
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// SDecimal
//
//#define _L (S & 0x00ff)
//#define _D (S >> 8)

SLAPI SDecimal::SDecimal(size_t sz, size_t prec) : DataType(GETSSIZE(MKSTYPED(S_DEC, sz, prec)))
{
}

uint32 SLAPI SDecimal::size() const // @v10.2.1
{
	return (S & 0x00ff);
}

int SLAPI SDecimal::comp(const void * i1, const void * i2) const
{
	return deccmp(static_cast<const char *>(i1), static_cast<const char *>(i2), (int16)(S & 0x00ff));
}

char * SLAPI SDecimal::tostr(const void * d, long fmt, char * buf) const
{
	long f;
	if(SFMTFLAG(fmt) & COMF_SQL)
		f = MKSFMTD(0, (S >> 8), 0);
	else if(fmt == 0)
		f = MKSFMTD(0, (S >> 8), 0);
	else
		f = fmt;
	return realfmt(dectobin(static_cast<const char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8)), f, buf); // @v9.8.4 @fix fmt-->f
}

int SLAPI SDecimal::fromstr(void * d, long, const char * buf) const
{
	double v;
	int    r = strtodoub(buf, &v);
	dectodec(v, static_cast<char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8));
	return r;
}

int SLAPI SDecimal::tobase(const void * d, void * baseData) const
{
	return ((*static_cast<double *>(baseData) = dectobin(static_cast<const char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8))), 1);
}

int SLAPI SDecimal::baseto(void * d, const void * baseData) const
{
	return (dectodec(*static_cast<const double *>(baseData), static_cast<char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8)), 1);
}

static void FASTCALL _bound(void * d, int s, int sign)
{
	int    sz = (s & 0x00ff);
	int    dec = (s >> 8);
	LDBL   v = (pow(10.0, sz * 2 - 1) - 1) / pow(10.0, dec);
	dectodec(sign ? -v : v, static_cast<char *>(d), sz, dec);
}

void SLAPI SDecimal::minval(void * d) const { _bound(d, S, 1); }
void SLAPI SDecimal::maxval(void * d) const { _bound(d, S, 0); }
//
// SDate
//
SLAPI SDate::SDate() : DataType(4) {}
char * SLAPI SDate::tostr(const void * v, long f, char * b) const { return datefmt(v, f, b); }
int  SLAPI SDate::fromstr(void * v, long f, const char * b) const { return strtodate(b, f, v); }
void SLAPI SDate::minval(void * d) const { return encodedate(0, 0, 0, d); }
void SLAPI SDate::maxval(void * d) const { return encodedate(1, 1, 3000, d); }
int  SLAPI SDate::comp(const void * i1, const void * i2) const { return CMPSIGN(*static_cast<const ulong *>(i1), *static_cast<const ulong *>(i2)); }

int SLAPI SDate::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		LDATE v = *static_cast<const LDATE *>(pData);
		if(v == ZERODATE) {
			*pInd = 1;
		}
		else {
			LDATE supp_date = pCtx ? pCtx->GetSupportingDate() : ZERODATE;
			if(supp_date) {
				long   d = diffdate(v, supp_date);
				//
				// Если есть опорная дата и разница в днях между сохраняемой датой
				// и опорной датой умещается в 1 или 2 байта, то воспользуемся этим.
				//
				if(labs(d) <= 0x7f) {
					*pInd = 2;
					int8 _v = static_cast<int8>(d);
					rBuf.Write(&_v, sizeof(_v));
				}
				else if(labs(d) <= 0x7fff) {
					*pInd = 3;
					int16 _v = static_cast<int16>(d);
					rBuf.Write(_v);
				}
				else {
					*pInd = 0;
					rBuf.Write(pData, S);
				}
			}
			else {
				*pInd = 0;
				rBuf.Write(pData, S);
			}
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else {
			if(*pInd == 0) {
				rBuf.Read(pData, S);
			}
			else {
				LDATE  v = ZERODATE;
				LDATE  supp_date = pCtx ? pCtx->GetSupportingDate() : ZERODATE;
				THROW_S(supp_date, SLERR_SRLZ_UNDEFSUPPDATE);
				if(*pInd == 2) {
					int8 _v;
					rBuf.Read(&_v, sizeof(_v));
					v = plusdate(supp_date, _v);
				}
				else if(*pInd == 3) {
					int16 _v;
					rBuf.Read(_v);
					v = plusdate(supp_date, _v);
				}
				else {
					THROW_S(0, SLERR_SRLZ_INVDATAIND);
				}
				*static_cast<LDATE *>(pData) = v;
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// STime
//
SLAPI STime::STime() : DataType(4) {}
char * SLAPI STime::tostr(const void * v, long f, char * b) const { return timefmt(*(LTIME *)v, f, b); }
int SLAPI STime::fromstr(void * v, long f, const char * b) const { return strtotime(b, f, (LTIME *)v); }
void SLAPI STime::minval(void * d) const { (((LTIME *)d)->v = 0UL); }
void SLAPI STime::maxval(void * d) const { ((LTIME *)d)->v = ULONG_MAX; }
int SLAPI STime::comp(const void * i1, const void * i2) const { return CMPSIGN(*(ulong *)i1, *(ulong *)i2); }

int SLAPI STime::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		LTIME  v = *static_cast<const LTIME *>(pData);
		if(v == ZEROTIME) {
			*pInd = 1;
		}
		else {
			if(v.hs() != 0) {
				*pInd = 0;
				rBuf.Write(pData, S);
			}
			else if(v.sec() != 0) {
				ulong t = (ulong)v.totalsec();
				if(t <= 0xffffU) {
					*pInd = 2;
					uint16 _v = (uint16)t;
					rBuf.Write(_v);
				}
				else {
					*pInd = 0;
					rBuf.Write(pData, S);
				}
			}
			else if(v.minut() != 0) {
				*pInd = 3;
				uint16 _v = static_cast<uint16>((uint)v.hour() << 8) | ((uint)v.minut());
				rBuf.Write(_v);
			}
			else {
				*pInd = 4;
				uint8 _v = static_cast<uint8>(v.hour());
				rBuf.Write(&_v, sizeof(_v));
			}
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else {
			if(*pInd == 0) {
				rBuf.Read(pData, S);
			}
			else {
				LTIME v = ZEROTIME;
				if(*pInd == 2) {
					uint16 _v;
					rBuf.Read(_v);
					v.settotalsec((long)_v);
				}
				else if(*pInd == 3) {
					uint16 _v = 0;
					rBuf.Read(_v);
					v = encodetime((int)(_v >> 8), (int)(_v & 0x00ff), 0, 0);
				}
				else if(*pInd == 4) {
					uint8 _v;
					rBuf.Read(&_v, sizeof(_v));
					v = encodetime((int)_v, 0, 0, 0);
				}
				else {
					THROW_S(0, SLERR_SRLZ_INVDATAIND);
				}
				*static_cast<LTIME *>(pData) = v;
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// SDateTime
//
SLAPI SDateTime::SDateTime() : DataType(sizeof(LDATETIME))
{
}

char * SLAPI SDateTime::tostr(const void * v, long f, char * b) const
{
	const LDATETIME * p_dtm = static_cast<const LDATETIME *>(v);
	char   t[256];
	char * p = t;
	datefmt(&p_dtm->d, MKSFMT(0, SFMTFLAG(f)), p);
	p += sstrlen(p);
	*p++ = ' ';
	timefmt(p_dtm->t, MKSFMT(0, TIMF_HMS), p);
	_commfmt(f, t);
	strcpy(b, t);
	return b;
}

int SLAPI SDateTime::fromstr(void * v, long f, const char * b) const
{
	int    ret = 0;
	const  char * s = sstrchr(b, ' ');
	LDATETIME * ldt = static_cast<LDATETIME *>(v);
	if(!s++) // @todo V769 The 's' pointer in the 's ++' expression could be nullptr. In such case, resulting value will be senseless and it should not be used. Bist.cpp 1406
		ret = SLERR_INVFORMAT;
	else {
		ret = strtodate(b, f, &ldt->d);
		SETIFZ(ret, strtotime(s, f, &ldt->t));
	}
	return ret;
}

void SLAPI SDateTime::minval(void * d) const
{
	*static_cast<LDATETIME *>(d) = ZERODATETIME;
}

void SLAPI SDateTime::maxval(void * d) const
{
	LDATETIME * ldt = static_cast<LDATETIME *>(d);
	ldt->d.encode(1, 1, 3000);
	ldt->t = MAXTIME;
}

int SLAPI SDateTime::comp(const void * i1, const void * i2) const
{
	return cmp(*static_cast<const LDATETIME *>(i1), *static_cast<const LDATETIME *>(i2));
}

int SLAPI SDateTime::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		if(ismemzero(pData, S)) {
			*pInd = 1;
		}
		else {
			uint8 ind_d = 0;
			uint8 ind_t = 0;
			LDATETIME v = *static_cast<const LDATETIME *>(pData);
			SDate _td;
			STime _tt;
			THROW(_td.Serialize(dir, &v.d, &ind_d, rBuf, pCtx));
			THROW(_tt.Serialize(dir, &v.t, &ind_t, rBuf, pCtx));
			*pInd = (100 + ind_d * 10 + ind_t);
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			memzero(pData, S);
		}
		else if(*pInd == 0) {
			rBuf.Read(pData, S);
		}
		else {
			uint8 ind_d = ((*pInd) - 100) / 10;
			uint8 ind_t = ((*pInd) - 100) % 10;
			LDATETIME v;
			SDate _td;
			STime _tt;
			THROW(_td.Serialize(dir, &v.d, &ind_d, rBuf, pCtx));
			THROW(_tt.Serialize(dir, &v.t, &ind_t, rBuf, pCtx));
			*static_cast<LDATETIME *>(pData) = v;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI SRaw::SRaw(uint32 sz) : DataType(sz)
{
}

int SLAPI SRaw::comp(const void * p1, const void *p2) const
{
	return memcmp(p1, p2, S);
}

char * SLAPI SRaw::tostr(const void * pData, long, char * pStr) const
{
	SString temp_buf;
	temp_buf.EncodeMime64(pData, S).CopyTo(pStr, 256);
	return pStr;
}

int SLAPI SRaw::fromstr(void * pData, long, const char * pStr) const
{
	SString temp_buf = pStr;
	size_t real_len = S;
	return temp_buf.DecodeMime64(pData, S, &real_len);
}

int SLAPI SRaw::base() const
	{ return BTS_STRING; }
int SLAPI SRaw::tobase(const void * s, void * b) const
	{ tostr(s, 0L, static_cast<char *>(b)); return 1; }
int SLAPI SRaw::baseto(void * s, const void * b) const
	{ fromstr(s, 0L, static_cast<const char *>(b)); return 1; }

void SLAPI SRaw::minval(void * pData) const
{
	memzero(pData, S);
}

void SLAPI SRaw::maxval(void * pData) const
{
	memset(pData, 0xff, S);
}
//
//
//
SLAPI SIPoint2::SIPoint2(uint32 sz) : DataType(sz)
{
	assert(sz == 4);
}

int SLAPI SIPoint2::comp(const void * i1, const void * i2) const
{
	FPoint p1 = *static_cast<const TPoint *>(i1);
	FPoint p2 = *static_cast<const TPoint *>(i2);
	return CMPSIGN(p1.Hypot(), p2.Hypot());
}

char * SLAPI SIPoint2::tostr(const void * s, long fmt, char * pBuf) const
{
	const TPoint * p_pnt = static_cast<const TPoint *>(s);
	char * p = pBuf;
	itoa(p_pnt->x, p, 10);
	p += sstrlen(p);
	*p++ = ',';
	itoa(p_pnt->y, p, 10);
	return pBuf;
}

int SLAPI SIPoint2::fromstr(void * s, long, const char * pStr) const
{
	RPoint p;
	p.Set(0.0);
	if(p.FromStr(pStr)) {
		static_cast<TPoint *>(s)->Set((int)p.x, (int)p.y);
		return 1;
	}
	else
		return 0;
}

int SLAPI SIPoint2::base() const
{
	return BTS_POINT2;
}

int SLAPI SIPoint2::tobase(const void * s, void * b) const
{
	RPoint * p_rp = static_cast<RPoint *>(b);
	p_rp->x = static_cast<const TPoint *>(s)->x;
	p_rp->y = static_cast<const TPoint *>(s)->y;
	return 1;
}

int SLAPI SIPoint2::baseto(void * s, const void * b) const
{
	static_cast<TPoint *>(s)->Set((int)static_cast<const RPoint *>(b)->x, (int)static_cast<const RPoint *>(b)->y);
	return 1;
}

void SLAPI SIPoint2::minval(void * s) const
{
	static_cast<TPoint *>(s)->Z();
}

void SLAPI SIPoint2::maxval(void * s) const
{
	static_cast<TPoint *>(s)->Set(0x7fff, 0x7fff);
}

int SLAPI SIPoint2::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	TPoint & r_p = *static_cast<TPoint *>(pData);
	if(dir > 0) {
		if(r_p.x == r_p.y) {
			if(r_p.x == 0) {
				*pInd = 1;
			}
			else {
				*pInd = 2;
				THROW(rBuf.Write(r_p.x));
			}
		}
		else {
			THROW(rBuf.Write(r_p.x));
			THROW(rBuf.Write(r_p.y));
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			r_p.x = r_p.y = 0;
		}
		else if(*pInd == 2) {
			THROW(rBuf.Read(r_p.x));
			r_p.y = r_p.x;
		}
		else {
			THROW(rBuf.Read(r_p.x));
			THROW(rBuf.Read(r_p.y));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI SFPoint2::SFPoint2(uint32 sz) : DataType(sz)
{
	assert(sz == sizeof(FPoint));
}

int SLAPI SFPoint2::comp(const void * i1, const void * i2) const
{
	return CMPSIGN(static_cast<const FPoint *>(i1)->Hypot(), static_cast<const FPoint *>(i2)->Hypot());
}

char * SLAPI SFPoint2::tostr(const void * pData, long f, char * pBuf) const
{
	const FPoint * p_pnt = static_cast<const FPoint *>(pData);
	char * p = pBuf;
	realfmt(p_pnt->X, MKSFMTD(0, 5, NMBF_NOTRAILZ), p);
	p += sstrlen(p);
	*p++ = ',';
	realfmt(p_pnt->Y, MKSFMTD(0, 5, NMBF_NOTRAILZ), p);
	return pBuf;
}

int SLAPI SFPoint2::fromstr(void * pData, long, const char * pStr) const
{
	RPoint p;
	p.Set(0.0);
	if(p.FromStr(pStr)) {
		static_cast<FPoint *>(pData)->Set((float)p.x, (float)p.y);
		return 1;
	}
	else
		return 0;
}

int SLAPI SFPoint2::base() const
{
	return BTS_POINT2;
}

int SLAPI SFPoint2::tobase(const void * pData, void * pBase) const
{
	RPoint * p_rp = static_cast<RPoint *>(pBase);
	p_rp->x = static_cast<const FPoint *>(pData)->X;
	p_rp->y = static_cast<const FPoint *>(pData)->Y;
	return 1;
}

int SLAPI SFPoint2::baseto(void * pData, const void * pBase) const
{
	static_cast<FPoint *>(pData)->Set((float)((const RPoint *)pBase)->x, (float)((const RPoint *)pBase)->y);
	return 1;
}

void SLAPI SFPoint2::minval(void * pData) const
{
	static_cast<FPoint *>(pData)->Set(0.0f, 0.0f);
}

void SLAPI SFPoint2::maxval(void * pData) const
{
	static_cast<FPoint *>(pData)->Set(32000.0f, 32000.0f);
}

int SLAPI SFPoint2::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	FPoint & r_p = *(FPoint *)pData;
	if(dir > 0) {
		if(r_p.X == r_p.Y) {
			if(r_p.X == 0.0f) {
				*pInd = 1;
			}
			else {
				*pInd = 2;
				THROW(rBuf.WriteFloat(r_p.X));
			}
		}
		else {
			THROW(rBuf.WriteFloat(r_p.X));
			THROW(rBuf.WriteFloat(r_p.Y));
		}
	}
	else if(dir < 0) {
		if(*pInd == 1)
			r_p.SetZero();
		else if(*pInd == 2) {
			THROW(rBuf.Read(r_p.X));
			r_p.Y = r_p.X;
		}
		else {
			THROW(rBuf.Read(r_p.X));
			THROW(rBuf.Read(r_p.Y));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI  SGuid::SGuid() : DataType(sizeof(S_GUID))
{
}

char * SLAPI SGuid::tostr(const void * pData, long f, char * pBuf) const
{
	SString temp_buf;
	static_cast<const S_GUID *>(pData)->ToStr(S_GUID::fmtIDL, temp_buf);
	temp_buf.CopyTo(pBuf, 0);
	return pBuf;
}

int SLAPI SGuid::fromstr(void * pData, long f, const char * pStr) const
{
	return static_cast<S_GUID *>(pData)->FromStr(pStr);
}

int SLAPI SGuid::base() const
	{ return BTS_STRING; }
int SLAPI SGuid::tobase(const void * pData, void * pBase) const
	{ tostr(pData, 0L, static_cast<char *>(pBase)); return 1; }
int SLAPI SGuid::baseto(void * pData, const void * pBase) const
	{ fromstr(pData, 0L, static_cast<const char *>(pBase)); return 1; }

void SLAPI SGuid::minval(void * pData) const
{
	memzero(pData, sizeof(S_GUID));
}

void SLAPI SGuid::maxval(void * pData) const
{
	memset(pData, 0xff, sizeof(S_GUID));
}
