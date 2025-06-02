// BIST.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// @threadsafe
// Реализация стандартных типов данных семейства SType
//
#include <slib-internal.h>
#pragma hdrstop
// @todo библиотека DB не должна включаться в slib #include <db.h> // DBRowId

class SVoid : public DataType {
public:
	SVoid() : DataType(0) {}
	int    base() const { return BTS_VOID; }
};

class SInt : public DataType {
public:
	explicit SInt(uint32 sz/*= 2*/); 
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_INT; }
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

typedef SInt SAutoinc;

class SInt64 : public DataType {
public:
	SInt64(); 
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_INT64_; }
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SUInt64 : public DataType { // @v11.9.2 @construction
public:
	SUInt64(); 
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_INT64_; } // @fixme Возможно, следует завести новый базовый тип (хотя, Оккам напряжется)
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SBool : public DataType {
public:
	explicit SBool(uint32 sz = 4);
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_BOOL; }
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SUInt : public DataType {
public:
 	explicit SUInt(uint32 sz = 2);
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_INT; }
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SFloat : public DataType {
public:
	explicit SFloat(uint32 sz = 8);
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_REAL; }
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SDec : public DataType {
public:
	SDec(size_t sz = 8, size_t prec = 2);
	virtual uint32 size() const;
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_REAL; }
	virtual void   tobase(const void *, void *) const;
	virtual int    baseto(void *, const void *) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
};

class SMoney : public SDec {
public:
	explicit SMoney(size_t sz = 8) : SDec(sz, 2) {}
};

class SDate : public DataType {
public:
	SDate();
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_DATE; }
	virtual void   tobase(const void * s, void * b) const { memmove(b, s, size()); }
	virtual int    baseto(void * s, const void * b) const{ memmove(s, b, size()); return 1; }
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class STime : public DataType {
public:
	STime();
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_TIME; }
	virtual void   tobase(const void * s, void * b) const { memmove(b, s, size()); }
	virtual int    baseto(void * s, const void * b) const { memmove(s, b, size()); return 1; }
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SDateTime : public DataType {
public:
	SDateTime();
	virtual int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_DATETIME; }
	virtual void   tobase(const void * s, void * b) const { memmove(b, s, size()); }
	virtual int    baseto(void * s, const void * b) const{ memmove(s, b, size()); return 1; }
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SChar : public DataType {
public:
	explicit SChar(uint32 sz = 0);
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_STRING; }
	void   tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); }
	int    baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   minval(void *) const;
	void   maxval(void *) const;
};

class SZString : public DataType {
public:
	explicit SZString(uint32 = 0);
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_STRING; }
	void   tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); }
	int    baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   minval(void *) const;
	void   maxval(void *) const;
	virtual int Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

typedef SZString SNote;

class SWcString : public DataType {
public:
	//
	// Размер указывается в байтах (не символах)
	//
	explicit SWcString(uint32 = sizeof(wchar_t));
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_STRING; }
	void   tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); }
	int    baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   minval(void *) const;
	void   maxval(void *) const;
};

class SLString : public DataType {
public:
	explicit SLString(uint32 = 0);
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	virtual int    base() const { return BTS_STRING; }
	void   tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); }
	int    baseto(void * s, const void * b) const { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }
	void   minval(void *) const;
	void   maxval(void *) const;
};

class SVariant : public DataType {
public:
	SVariant() : DataType(sizeof(VARIANT)) {}
};

class SRaw : public DataType {
public:
	explicit SRaw(uint32 sz = 0);
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	int    base() const;
	void   tobase(const void * s, void * b) const;
	int    baseto(void * s, const void * b) const;
	void   minval(void *) const;
	void   maxval(void *) const;
};

class SIPoint2 : public DataType {
public:
	explicit SIPoint2(uint32 sz = 4);
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	int    base() const;
	void   tobase(const void * s, void * b) const;
	int    baseto(void * s, const void * b) const;
	void   minval(void *) const;
	void   maxval(void *) const;
	int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SFPoint2 : public DataType {
public:
	explicit SFPoint2(uint32 sz = 8);
	int    comp(const void *, const void *) const;
	char * tostr(const void *, long, char *) const;
	int    fromstr(void *, long, const char *) const;
	int    base() const;
	void   tobase(const void * s, void * b) const;
	int    baseto(void * s, const void * b) const;
	void   minval(void *) const;
	void   maxval(void *) const;
	int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SGuid : public DataType {
public:
	SGuid();
	// @baseused int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual SString & ToStr(const void * pData, long format, SString & rBuf) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const;
	virtual void   tobase(const void * s, void * b) const;
	virtual int    baseto(void * s, const void * b) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	// @baseused virtual int Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};

class SDataType_Color : public DataType {
public:
	SDataType_Color();
	// @baseused int    comp(const void *, const void *) const;
	virtual char * tostr(const void *, long, char *) const;
	virtual SString & ToStr(const void * pData, long format, SString & rBuf) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const;
	virtual void   tobase(const void * s, void * b) const;
	virtual int    baseto(void * s, const void * b) const;
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
};
//
// Функция регистрации встроенных типов
//
void RegisterBIST()
{
	RegisterSType(S_VOID,        &SVoid());
	RegisterSType(S_CHAR,     	 &SChar());
	RegisterSType(S_INT,      	 &SInt(4));
	// @v11.9.9 (S_BOOL is S_LOGICAL) RegisterSType(S_BOOL,     	 &SBool());
	RegisterSType(S_UINT,     	 &SUInt());
	RegisterSType(S_LOGICAL,  	 &SBool());
	RegisterSType(S_FLOAT,    	 &SFloat());
	RegisterSType(S_DEC,      	 &SDec());
	RegisterSType(S_MONEY,    	 &SMoney());
	RegisterSType(S_DATE,     	 &SDate());
	RegisterSType(S_DATETIME, 	 &SDateTime());
	RegisterSType(S_TIME,     	 &STime());
	RegisterSType(S_ZSTRING,  	 &SZString());
	RegisterSType(S_AUTOINC,  	 &SAutoinc(4));
	RegisterSType(S_NOTE,     	 &SNote());
	RegisterSType(S_LSTRING,  	 &SLString());
	RegisterSType(S_VARIANT,  	 &SVariant());
	RegisterSType(S_WCHAR,    	 &SWcString());
	RegisterSType(S_WZSTRING, 	 &SWcString());
	RegisterSType(S_RAW,      	 &SRaw());
	RegisterSType(S_IPOINT2,  	 &SIPoint2());
	RegisterSType(S_FPOINT2,  	 &SFPoint2());
	RegisterSType(S_UUID_,    	 &SGuid());
	RegisterSType(S_INT64,       &SInt64());
	RegisterSType(S_UINT64,      &SUInt64());
	RegisterSType(S_COLOR_RGBA,  &SDataType_Color());
}
//
// SChar
//
SChar::SChar(uint32 sz) : DataType(sz)
{
}

int SChar::comp(const void * i1, const void * i2) const
{
	return S ? strncmp(static_cast<const char *>(i1), static_cast<const char *>(i2), S) : strcmp(static_cast<const char *>(i1), static_cast<const char *>(i2));
}

char * SChar::tostr(const void * d, long fmt, char * buf) const
{
	return strfmt(strnzcpy(buf, static_cast<const char *>(d), S), fmt, buf);
}

int SChar::fromstr(void * d, long, const char * buf) const
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

void SChar::minval(void * d) const
{
	if(S)
		memset(d, ' ', S);
	else
		*static_cast<char *>(d) = 0;
}

void SChar::maxval(void * d) const
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
SWcString::SWcString(uint32 sz) : DataType(ALIGNSIZE(sz, 1))
{
}

int SWcString::comp(const void * i1, const void * i2) const
{
	return S ? wcsncmp(static_cast<const wchar_t *>(i1), static_cast<const wchar_t *>(i2), S/2) : wcscmp(static_cast<const wchar_t *>(i1), static_cast<const wchar_t *>(i2));
}

char * SWcString::tostr(const void * d, long fmt, char * buf) const
{
	WideCharToMultiByte(CP_OEMCP, 0, static_cast<const wchar_t *>(d), -1, buf, 254, 0, 0);
	return buf;
}

int SWcString::fromstr(void * d, long, const char * buf) const
{
	MultiByteToWideChar(CP_OEMCP, 0, buf, -1, static_cast<wchar_t *>(d), 254);
	return 1;
}

void SWcString::minval(void * d) const
{
	static_cast<wchar_t *>(d)[0] = 0;
}

void SWcString::maxval(void * d) const
{
	static_cast<wchar_t *>(d)[0] = static_cast<wchar_t>(MAXSHORT);
	static_cast<wchar_t *>(d)[1] = 0;
}
//
// SZString
//
SZString::SZString(uint32 sz) : DataType(sz)
{
}

#pragma option -K

int SZString::comp(const void * i1, const void * i2) const
{
	return S ? strncmp(static_cast<const char *>(i1), static_cast<const char *>(i2), S) : strcmp(static_cast<const char *>(i1), static_cast<const char *>(i2));
}

#pragma option -K.

char * SZString::tostr(const void * d, long fmt, char * buf) const
{
	return strfmt(strnzcpy(buf, static_cast<const char *>(d), S), fmt, buf);
}

/* @construction SString & SZString::ToStr(const void * pData, long format, SString & rBuf) const
{
	rBuf.Z();
	if(S > 0)
		rBuf.CatN(static_cast<const char *>(pData), S);
	else
		rBuf.Cat(static_cast<const char *>(pData));
}*/

int SZString::fromstr(void * d, long, const char * buf) const
{
	strnzcpy(static_cast<char *>(d), buf, S);
	return 1;
}

void SZString::minval(void * d) const
{
	static_cast<char *>(d)[0] = 0;
}

void SZString::maxval(void * d) const
{
	memset(d, 255, S-1);
	PTR8(d)[S-1] = 0;
	// ((char *) d)[0] = (char)254;
	// ((char *) d)[1] = 0;
}

int SZString::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		size_t sz = size();
		size_t len = sstrlen(static_cast<const char *>(pData));
		if(!len) {
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
SLString::SLString(uint32 sz) : DataType(sz)
{
}

#pragma option -K

int SLString::comp(const void * i1, const void * i2) const
{
	const size_t l1 = *static_cast<const char *>(i1);
	const size_t l2 = *static_cast<const char *>(i2);
	return strncmp(static_cast<const char *>(i1)+1, static_cast<const char *>(i2)+1, MIN(l1, l2));
}

#pragma option -K.

char * SLString::tostr(const void * d, long fmt, char * buf) const
{
	size_t l = *static_cast<const char *>(d);
	return strfmt(strnzcpy(buf, static_cast<const char *>(d)+1, MIN(S, l+1)), fmt, buf);
}

int SLString::fromstr(void * d, long, const char * buf) const
{
	size_t l = sstrlen(buf);
	*static_cast<char *>(d) = static_cast<char>(l);
	strncpy(static_cast<char *>(d)+1, buf, l);
	return 1;
}

void SLString::minval(void * d) const
{
	PTR8(d)[0] = 0;
}

void SLString::maxval(void * d) const
{
	PTR8(d)[0] = 0;
}
//
// SInt
//
static int64 FASTCALL _tolong(const void * d, int sz)
{
	switch(sz) {
		case 4: return static_cast<int64>(*static_cast<const long *>(d));
		case 2: return static_cast<int64>(*static_cast<const int16 *>(d));
		case 1: return static_cast<int64>(*static_cast<const int8 *>(d));
		case 8: return *static_cast<const int64 *>(d);
	}
	return 0L;
}

static void FASTCALL _longto(int64 v, void * d, int sz)
{
	switch(sz) {
		case 4: *static_cast<int32 *>(d) = static_cast<int32>(v); break;
		case 2: *static_cast<int16 *>(d) = static_cast<int16>(v); break;
		case 1: *static_cast<int8 *>(d) = static_cast<int8>(v); break;
		case 8: *static_cast<int64 *>(d) = v; break;
	}
}

SInt::SInt(uint32 sz) : DataType(sz)
{
}

int SInt::comp(const void * i1, const void * i2) const
{
	if(S == 0) {
		const int64 l1 = *static_cast<const int64 *>(i1);
		const int64 l2 = *static_cast<const int64 *>(i2);
		return CMPSIGN(l1, l2);
	}
	else {
		const int64 l1 = _tolong(i1, S);
		const int64 l2 = _tolong(i2, S);
		return CMPSIGN(l1, l2);
	}
}

char * SInt::tostr(const void * d, long fmt, char * buf) const
{
	if(S == 8)
		return int64fmt(*static_cast<const int64 *>(d), fmt, buf);
	else
		return intfmt(static_cast<int32>(_tolong(d, S)), fmt, buf);
}

int SInt::fromstr(void * d, long, const char * buf) const
{
	long   lv;
	int    r = strtolong(buf, &lv);
	_longto(lv, d, S);
	return r;
}

void SInt::tobase(const void * d, void * baseData) const
{
	*static_cast<int32 *>(baseData) = static_cast<int32>(_tolong(d, S));
}

int SInt::baseto(void * d, const void * baseData) const
{
	return (_longto(*static_cast<const int32 *>(baseData), d, S), 1);
}

void SInt::minval(void * d) const
{
	switch(S) {
		case 2: *static_cast<int16 *>(d) = SHRT_MIN;  break;
		case 4: *static_cast<int32 *>(d) = LONG_MIN;  break;
		case 1: *static_cast<int8  *>(d) = SCHAR_MIN; break;
		case 8: *static_cast<int64 *>(d) = LLONG_MIN; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

void SInt::maxval(void * d) const
{
	switch(S) {
		case 2: *static_cast<int16 *>(d) = SHRT_MAX; break;
		case 4: *static_cast<int32 *>(d) = LONG_MAX; break;
		case 1: *static_cast<int8  *>(d) = SCHAR_MAX; break;
		case 8: *static_cast<int64 *>(d) = LLONG_MAX; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

int SInt::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - zero, 2 - full
		if(S == 8) {
			const int64 v64 = *static_cast<const int64 *>(pData);
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
			const int64 v = _tolong(pData, S);
			if(v == 0)
				spec = 1;
			else {
				if(S > 1 && _abs64(v) <= 0x7fL) {
					*pInd = 2;
					int8 _v = (int8)v;
					rBuf.Write(&_v, sizeof(_v));
				}
				else if(S > 2 && _abs64(v) <= 0x7fffL) {
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
SUInt64::SUInt64() : DataType(8) // @v11.9.2 @construction
{
}

int SUInt64::comp(const void * i1, const void * i2) const
{
	const uint64 l1 = *static_cast<const uint64 *>(i1);
	const uint64 l2 = *static_cast<const uint64 *>(i2);
	return CMPSIGN(l1, l2);
}

char * SUInt64::tostr(const void * d, long fmt, char * buf) const
{
	return uint64fmt(*static_cast<const uint64 *>(d), fmt, buf);
}

int SUInt64::fromstr(void * d, long, const char * buf) const
{
	// @todo Нужен вариант функции преобразования строки в целое с диагностикой ошибки
	uint64 lv = satou64(buf);
	_longto(lv, d, S);
	return 1;
}

void SUInt64::tobase(const void * d, void * baseData) const { *static_cast<int64 *>(baseData) = _tolong(d, S); }
int  SUInt64::baseto(void * d, const void * baseData) const { return (_longto(*static_cast<const int64 *>(baseData), d, S), 1); }
void SUInt64::minval(void * d) const { *static_cast<uint64 *>(d) = 0; }
void SUInt64::maxval(void * d) const { *static_cast<uint64 *>(d) = ULLONG_MAX; }

int SUInt64::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - zero, 2 - full
		const uint64 v64 = *static_cast<const uint64 *>(pData);
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
			if(*pInd == 2) {
				int8 _v;
				rBuf.Read(&_v, sizeof(_v));
				*static_cast<int64 *>(pData) = _v;
			}
			else if(*pInd == 3) {
				int16 _v;
				rBuf.Read(_v);
				*static_cast<int64 *>(pData) = _v;
			}
			else if(*pInd == 4) {
				int32 _v;
				rBuf.Read(_v);
				*static_cast<int64 *>(pData) = _v;
			}
			else {
				THROW_S(0, SLERR_SRLZ_INVDATAIND);
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SInt64::SInt64() : DataType(8)
{
}

int SInt64::comp(const void * i1, const void * i2) const
{
	const int64 l1 = *static_cast<const int64 *>(i1);
	const int64 l2 = *static_cast<const int64 *>(i2);
	return CMPSIGN(l1, l2);
}

char * SInt64::tostr(const void * d, long fmt, char * buf) const
{
	return int64fmt(*static_cast<const int64 *>(d), fmt, buf);
}

int SInt64::fromstr(void * d, long, const char * buf) const
{
	// @todo Перестроить на 64-разрядные функции
	long   lv;
	int    r = strtolong(buf, &lv);
	_longto(lv, d, S);
	return r;
}

void SInt64::tobase(const void * d, void * baseData) const { *static_cast<int64 *>(baseData) = _tolong(d, S); }
int  SInt64::baseto(void * d, const void * baseData) const { return (_longto(*static_cast<const int64 *>(baseData), d, S), 1); }
void SInt64::minval(void * d) const { *static_cast<int64 *>(d) = LLONG_MIN; }
void SInt64::maxval(void * d) const { *static_cast<int64 *>(d) = LLONG_MAX; }

int SInt64::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - zero, 2 - full
		const int64 v64 = *static_cast<const int64 *>(pData);
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
			if(*pInd == 2) {
				int8 _v;
				rBuf.Read(&_v, sizeof(_v));
				*static_cast<int64 *>(pData) = _v;
			}
			else if(*pInd == 3) {
				int16 _v;
				rBuf.Read(_v);
				*static_cast<int64 *>(pData) = _v;
			}
			else if(*pInd == 4) {
				int32 _v;
				rBuf.Read(_v);
				*static_cast<int64 *>(pData) = _v;
			}
			else {
				THROW_S(0, SLERR_SRLZ_INVDATAIND);
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SBool::SBool(uint32 sz) : DataType(sz)
{
}

int SBool::comp(const void * p1, const void * p2) const
{
	const long   l1 = BIN(_tolong(p1, S));
	const long   l2 = BIN(_tolong(p2, S));
	return CMPSIGN(l1, l2);
}

char * SBool::tostr(const void * d, long fmt, char * buf) const
{
	const bool b = LOGIC(_tolong(d, S));
	return strcpy(buf, STextConst::GetBool(b));
}

int SBool::fromstr(void * d, long fmt, const char * buf) const
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

void SBool::tobase(const void * pData, void * pBaseData) const
{
	*static_cast<int32 *>(pBaseData) = static_cast<int32>(_tolong(pData, S));
}

int SBool::baseto(void * pData, const void * pBaseData) const
{
	return (_longto(*static_cast<const int32 *>(pBaseData), pData, S), 1);
}

void SBool::minval(void * pData) const
{
	switch(S) {
		case 2: *static_cast<int16 *>(pData) = 0;  break;
		case 4: *static_cast<int32 *>(pData) = 0;  break;
		case 1: *static_cast<int8  *>(pData) = 0; break;
		case 8: *static_cast<int64 *>(pData) = 0; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

void SBool::maxval(void * pData) const
{
	switch(S) {
		case 2: *static_cast<int16 *>(pData) = 1; break;
		case 4: *static_cast<int32 *>(pData) = 1; break;
		case 1: *static_cast<int8  *>(pData) = 1; break;
		case 8: *static_cast<int64 *>(pData) = 1; break;
		default: ; // assert(INVALID_DATA_SIZE);
	}
}

int SBool::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - false, 2 - true
		if(S == 8) {
			const int64 v64 = *static_cast<const int64 *>(pData);
			if(v64 == 0)
				spec = 1;
			else
				spec = 2;
		}
		else {
			const long v = static_cast<long>(_tolong(pData, S));
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

SUInt::SUInt(uint32 sz) : DataType(sz)
{
}

int SUInt::comp(const void * i1, const void * i2) const
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

char * SUInt::tostr(const void * d, long fmt, char * buf) const
{
	if(S == 8)
		return uint64fmt(*static_cast<const uint64 *>(d), fmt, buf);
	else
		return uintfmt(_toulong(d, S), fmt, buf);
}

int SUInt::fromstr(void * d, long, const char * buf) const
{
	ulong  lv;
	int    r = strtouint(buf, &lv);
	_ulongto(lv, d, S);
	return r;
}

void SUInt::tobase(const void * d, void * baseData) const { *static_cast<long *>(baseData) = static_cast<long>(_toulong(d, S)); }
int  SUInt::baseto(void * d, const void * baseData) const { return (_ulongto((ulong)*(long *)baseData, d, S), 1); }
void SUInt::minval(void * d) const { _ulongto(0L, d, S); }

void SUInt::maxval(void * d) const
{
	switch(S) {
		case 1: *static_cast<uint8 *>(d)  = 255; break;
		case 2: *static_cast<uint16 *>(d) = USHRT_MAX; break;
		case 4: *static_cast<ulong *>(d)  = ULONG_MAX; break;
		default: ; //CHECK(INVALID_DATA_SIZE);
	}
}

int SUInt::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		int    spec = 0; // 1 - zero, 2 - full
		if(S == 8) {
			uint64 v64 = *static_cast<const uint64 *>(pData);
			if(v64 == 0)
				spec = 1;
			else if(v64 <= 0xffUL) {
				*pInd = 2;
				uint8 _v = static_cast<uint8>(v64);
				rBuf.Write(&_v, sizeof(_v));
			}
			else if(v64 <= 0xffffUL) {
				*pInd = 3;
				uint16 _v = static_cast<uint16>(v64);
				rBuf.Write(_v);
			}
			else if(v64 <= 0xffffffffUL) {
				*pInd = 4;
				uint32 _v = static_cast<uint32>(v64);
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
				*static_cast<uint64 *>(pData) = _v;
			}
			else if(*pInd == 3) {
				uint16 _v;
				rBuf.Read(_v);
				*static_cast<uint64 *>(pData) = _v;
			}
			else if(*pInd == 4) {
				uint32 _v;
				rBuf.Read(_v);
				*static_cast<uint64 *>(pData) = _v;
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
SFloat::SFloat(uint32 sz) : DataType(sz)
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

static void __ldblto(double v, void * d, int s)
{
	switch(s) {
		case 8: *(double *)d = v; break;
		case 4: *(float *)d = (float)v; break;
		case 10: *(long double *)d = v; break;
		default: break;
	}
}

int SFloat::comp(const void * i1, const void * i2) const
{
	const LDBL v1 = __toldbl(i1, S);
	const LDBL v2 = __toldbl(i2, S);
	return CMPSIGN(v1, v2);
}

char * SFloat::tostr(const void * d, long fmt, char * buf) const
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

int SFloat::fromstr(void * d, long, const char * buf) const
{
	double v;
	int r = strtodoub(buf, &v);
	__ldblto(v, d, S);
	return r;
}

void SFloat::tobase(const void * d, void * baseData) const { *static_cast<double *>(baseData) = __toldbl(d, S); }
int  SFloat::baseto(void * d, const void * baseData) const { return (__ldblto(*static_cast<const double *>(baseData), d, S), 1); }

//static const float max_flt = (float)MAXFLOAT;

void SFloat::minval(void * d) const
{
	switch(S) {
		case  8: *static_cast<double *>(d) = -SMathConst::Max; break;
		case  4: *static_cast<float *>(d)  = -SMathConst::Max_f; break;
		case 10: *static_cast<LDBL *>(d)   = -SMathConst::Max; break;
		// default: assert(INVALID_DATA_SIZE);
	}
}

void SFloat::maxval(void * d) const
{
	switch(S) {
		case  8: *static_cast<double *>(d) = SMathConst::Max; break;
		case  4: *static_cast<float *>(d)  = SMathConst::Max_f; break;
		case 10: *static_cast<LDBL *>(d)   = SMathConst::Max; break;
		// default: assert(INVALID_DATA_SIZE);
	}
}

int SFloat::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
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
// SDec
//
//#define _L (S & 0x00ff)
//#define _D (S >> 8)

SDec::SDec(size_t sz, size_t prec) : DataType(GETSSIZE(MKSTYPED(S_DEC, sz, prec)))
{
}

uint32 SDec::size() const { return (S & 0x00ff); }

int SDec::comp(const void * i1, const void * i2) const
{
	return deccmp(static_cast<const char *>(i1), static_cast<const char *>(i2), (int16)(S & 0x00ff));
}

char * SDec::tostr(const void * d, long fmt, char * buf) const
{
	long f;
	if(SFMTFLAG(fmt) & COMF_SQL)
		f = MKSFMTD(0, (S >> 8), 0);
	else if(fmt == 0)
		f = MKSFMTD(0, (S >> 8), 0);
	else
		f = fmt;
	return realfmt(dectobin(static_cast<const char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8)), f, buf);
}

int SDec::fromstr(void * d, long, const char * buf) const
{
	double v;
	int    r = strtodoub(buf, &v);
	dectodec(v, static_cast<char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8));
	return r;
}

void SDec::tobase(const void * d, void * baseData) const
{
	*static_cast<double *>(baseData) = dectobin(static_cast<const char *>(d), (int16)(S & 0x00ff), (int16)(S >> 8));
}

int SDec::baseto(void * d, const void * baseData) const
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

void SDec::minval(void * d) const { _bound(d, S, 1); }
void SDec::maxval(void * d) const { _bound(d, S, 0); }
//
// SDate
//
SDate::SDate() : DataType(4) {}
char * SDate::tostr(const void * v, long f, char * b) const { return datefmt(v, f, b); }
int  SDate::fromstr(void * v, long f, const char * b) const { return strtodate(b, f, v); }
void SDate::minval(void * d) const { return encodedate(0, 0, 0, d); }
void SDate::maxval(void * d) const { return encodedate(1, 1, 3000, d); }
int  SDate::comp(const void * i1, const void * i2) const { return CMPSIGN(*static_cast<const ulong *>(i1), *static_cast<const ulong *>(i2)); }

int SDate::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
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
STime::STime() : DataType(4) {}
char * STime::tostr(const void * v, long f, char * b) const { return timefmt(*(LTIME *)v, f, b); }
int STime::fromstr(void * v, long f, const char * b) const { return strtotime(b, f, (LTIME *)v); }
void STime::minval(void * d) const { (((LTIME *)d)->v = 0UL); }
void STime::maxval(void * d) const { ((LTIME *)d)->v = ULONG_MAX; }
int STime::comp(const void * i1, const void * i2) const { return CMPSIGN(*(ulong *)i1, *(ulong *)i2); }

int STime::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
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
SDateTime::SDateTime() : DataType(sizeof(LDATETIME))
{
}

char * SDateTime::tostr(const void * v, long f, char * b) const
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

int SDateTime::fromstr(void * v, long f, const char * b) const
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

void SDateTime::minval(void * d) const
{
	*static_cast<LDATETIME *>(d) = ZERODATETIME;
}

void SDateTime::maxval(void * d) const
{
	LDATETIME * p_ldt = static_cast<LDATETIME *>(d);
	if(p_ldt) {
		p_ldt->d.encode(1, 1, 3000);
		p_ldt->t = MAXTIME;
	}
}

int SDateTime::comp(const void * i1, const void * i2) const
{
	return cmp(*static_cast<const LDATETIME *>(i1), *static_cast<const LDATETIME *>(i2));
}

int SDateTime::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
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
SRaw::SRaw(uint32 sz) : DataType(sz)
{
}

int SRaw::comp(const void * p1, const void *p2) const
{
	return memcmp(p1, p2, S);
}

char * SRaw::tostr(const void * pData, long, char * pStr) const
{
	SString temp_buf;
	temp_buf.EncodeMime64(pData, S).CopyTo(pStr, 256);
	return pStr;
}

int SRaw::fromstr(void * pData, long, const char * pStr) const
{
	SString temp_buf(pStr);
	size_t real_len = S;
	return temp_buf.DecodeMime64(pData, S, &real_len);
}

int  SRaw::base() const { return BTS_STRING; }
void SRaw::tobase(const void * s, void * b) const { tostr(s, 0L, static_cast<char *>(b)); }
int  SRaw::baseto(void * s, const void * b) const  { fromstr(s, 0L, static_cast<const char *>(b)); return 1; }

void SRaw::minval(void * pData) const
{
	memzero(pData, S);
}

void SRaw::maxval(void * pData) const
{
	memset(pData, 0xff, S);
}
//
//
//
SIPoint2::SIPoint2(uint32 sz) : DataType(sz)
{
	assert(sz == 4);
}

int SIPoint2::comp(const void * i1, const void * i2) const
{
	SPoint2F p1 = *static_cast<const SPoint2S *>(i1);
	SPoint2F p2 = *static_cast<const SPoint2S *>(i2);
	return CMPSIGN(p1.Hypot(), p2.Hypot());
}

char * SIPoint2::tostr(const void * s, long fmt, char * pBuf) const
{
	const SPoint2S * p_pnt = static_cast<const SPoint2S *>(s);
	char * p = pBuf;
	itoa(p_pnt->x, p, 10);
	p += sstrlen(p);
	*p++ = ',';
	itoa(p_pnt->y, p, 10);
	return pBuf;
}

int SIPoint2::fromstr(void * s, long, const char * pStr) const
{
	SPoint2R p;
	p.Set(0.0);
	if(p.FromStr(pStr)) {
		static_cast<SPoint2S *>(s)->Set((int)p.x, (int)p.y);
		return 1;
	}
	else
		return 0;
}

int SIPoint2::base() const
{
	return BTS_POINT2;
}

void SIPoint2::tobase(const void * s, void * b) const
{
	SPoint2R * p_rp = static_cast<SPoint2R *>(b);
	p_rp->x = static_cast<const SPoint2S *>(s)->x;
	p_rp->y = static_cast<const SPoint2S *>(s)->y;
}

int SIPoint2::baseto(void * s, const void * b) const
{
	static_cast<SPoint2S *>(s)->Set((int)static_cast<const SPoint2R *>(b)->x, (int)static_cast<const SPoint2R *>(b)->y);
	return 1;
}

void SIPoint2::minval(void * s) const
{
	static_cast<SPoint2S *>(s)->Z();
}

void SIPoint2::maxval(void * s) const
{
	static_cast<SPoint2S *>(s)->Set(0x7fff, 0x7fff);
}

int SIPoint2::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SPoint2S & r_p = *static_cast<SPoint2S *>(pData);
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
SFPoint2::SFPoint2(uint32 sz) : DataType(sz)
{
	assert(sz == sizeof(SPoint2F));
}

int SFPoint2::comp(const void * i1, const void * i2) const
{
	return CMPSIGN(static_cast<const SPoint2F *>(i1)->Hypot(), static_cast<const SPoint2F *>(i2)->Hypot());
}

char * SFPoint2::tostr(const void * pData, long f, char * pBuf) const
{
	const SPoint2F * p_pnt = static_cast<const SPoint2F *>(pData);
	char * p = pBuf;
	realfmt(p_pnt->x, MKSFMTD(0, 5, NMBF_NOTRAILZ), p);
	p += sstrlen(p);
	*p++ = ',';
	realfmt(p_pnt->y, MKSFMTD(0, 5, NMBF_NOTRAILZ), p);
	return pBuf;
}

int SFPoint2::fromstr(void * pData, long, const char * pStr) const
{
	SPoint2R p;
	p.Set(0.0);
	if(p.FromStr(pStr)) {
		static_cast<SPoint2F *>(pData)->Set((float)p.x, (float)p.y);
		return 1;
	}
	else
		return 0;
}

int SFPoint2::base() const
{
	return BTS_POINT2;
}

void SFPoint2::tobase(const void * pData, void * pBase) const
{
	SPoint2R * p_rp = static_cast<SPoint2R *>(pBase);
	p_rp->x = static_cast<const SPoint2F *>(pData)->x;
	p_rp->y = static_cast<const SPoint2F *>(pData)->y;
}

int SFPoint2::baseto(void * pData, const void * pBase) const
{
	static_cast<SPoint2F *>(pData)->Set((float)((const SPoint2R *)pBase)->x, (float)((const SPoint2R *)pBase)->y);
	return 1;
}

void SFPoint2::minval(void * pData) const
{
	static_cast<SPoint2F *>(pData)->Set(0.0f, 0.0f);
}

void SFPoint2::maxval(void * pData) const
{
	static_cast<SPoint2F *>(pData)->Set(32000.0f, 32000.0f);
}

int SFPoint2::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SPoint2F & r_p = *static_cast<SPoint2F *>(pData);
	if(dir > 0) {
		if(r_p.x == r_p.y) {
			if(r_p.x == 0.0f) {
				*pInd = 1;
			}
			else {
				*pInd = 2;
				THROW(rBuf.WriteFloat(r_p.x));
			}
		}
		else {
			THROW(rBuf.WriteFloat(r_p.x));
			THROW(rBuf.WriteFloat(r_p.y));
		}
	}
	else if(dir < 0) {
		if(*pInd == 1)
			r_p.SetZero();
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
SGuid::SGuid() : DataType(sizeof(S_GUID))
{
}

char * SGuid::tostr(const void * pData, long f, char * pBuf) const
{
	SString temp_buf;
	static_cast<const S_GUID *>(pData)->ToStr(S_GUID::fmtIDL, temp_buf);
	temp_buf.CopyTo(pBuf, 0);
	return pBuf;
}

SString & SGuid::ToStr(const void * pData, long format, SString & rBuf) const
{
	rBuf.Z();
	static_cast<const S_GUID *>(pData)->ToStr(S_GUID::fmtIDL, rBuf);
	return rBuf;
}

int  SGuid::fromstr(void * pData, long f, const char * pStr) const { return static_cast<S_GUID *>(pData)->FromStr(pStr); }
int  SGuid::base() const { return BTS_STRING; }
void SGuid::tobase(const void * pData, void * pBase) const { tostr(pData, 0L, static_cast<char *>(pBase)); }
int  SGuid::baseto(void * pData, const void * pBase) const { fromstr(pData, 0L, static_cast<const char *>(pBase)); return 1; }
void SGuid::minval(void * pData) const { memzero(pData, sizeof(S_GUID)); }
void SGuid::maxval(void * pData) const { memset(pData, 0xff, sizeof(S_GUID)); }
//
//
//
SDataType_Color::SDataType_Color() : DataType(sizeof(SColorBase))
{
}
// @baseused int    comp(const void *, const void *) const;
char * SDataType_Color::tostr(const void * pData, long format, char * pBuf) const
{
	SString temp_buf;
	static_cast<const SColorBase *>(pData)->ToStr(temp_buf, /*format*/0);
	temp_buf.CopyTo(pBuf, 0);
	return pBuf;
}

SString & SDataType_Color::ToStr(const void * pData, long format, SString & rBuf) const
{
	rBuf.Z();
	static_cast<const SColorBase *>(pData)->ToStr(rBuf, /*format*/0);
	return rBuf;
}

int SDataType_Color::fromstr(void * pData, long fmt, const char * pStr) const { return static_cast<SColorBase *>(pData)->FromStr(pStr); }
int SDataType_Color::base() const { return BTS_STRING; }
void SDataType_Color::tobase(const void * pData, void * pBase) const { tostr(pData, 0L, static_cast<char *>(pBase)); }
int SDataType_Color::baseto(void * pData, const void * pBase) const { fromstr(pData, 0L, static_cast<const char *>(pBase)); return 1; }
void SDataType_Color::minval(void * pData) const { memzero(pData, sizeof(SColorBase)); }
void SDataType_Color::maxval(void * pData) const { memset(pData, 0xff, sizeof(SColorBase)); }

int SDataType_Color::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SColorBase & r_c = *static_cast<SColorBase *>(pData);
	if(dir > 0) {
		if(r_c.IsEmpty()) {
			*pInd = 1;
		}
		else if(r_c.B == r_c.G && r_c.G == r_c.R) { // Все компоненты равны
			THROW(rBuf.Write(r_c.B));
			if(r_c.Alpha == 255) {
				*pInd = 2;
			}
			else if(r_c.Alpha == 0) {
				*pInd = 3;
			}
			else {
				THROW(rBuf.Write(r_c.Alpha));
				*pInd = 4;
			}
		}
		else {
			THROW(rBuf.Write(r_c.B));
			THROW(rBuf.Write(r_c.G));
			THROW(rBuf.Write(r_c.R));
			if(r_c.Alpha == 255) {
				*pInd = 5;
			}
			else if(r_c.Alpha == 0) {
				*pInd = 6;
			}
			else {
				THROW(rBuf.Write(r_c.Alpha));
			}
		}
	}
	else if(dir < 0) {
		if(*pInd == 1) {
			r_c.Z();
		}
		else if(oneof3(*pInd, 2, 3, 4)) {
			uint8 byte = 0;
			THROW(rBuf.Read(byte));
			r_c.R = byte;
			r_c.G = byte;
			r_c.B = byte;
			if(*pInd == 2) {
				r_c.Alpha = 255;
			}
			else if(*pInd == 3) {
				r_c.Alpha = 0;
			}
			else if(*pInd == 4) {
				THROW(rBuf.Read(r_c.Alpha));
			}
		}
		else {
			THROW(rBuf.Read(r_c.B));
			THROW(rBuf.Read(r_c.G));
			THROW(rBuf.Read(r_c.R));
			if(*pInd == 5)
				r_c.Alpha = 255;
			else if(*pInd == 6)
				r_c.Alpha = 0;
			else {
				THROW(rBuf.Read(r_c.Alpha));
			}
		}
	}
	CATCHZOK
	return ok;
}
