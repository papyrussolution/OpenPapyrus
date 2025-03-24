// STYPE.CPP
// Copyright (c) Sobolev A. 1995, 1996, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2019, 2020, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

#define getVptr(thisPtr) (*(void **)(thisPtr))

static void * P_TypeTable[SlConst::TypeTabSize]; // @global @threadsafe

#define INVALID_TYPEID 0

void FASTCALL RegisterSType(TYPEID id, const DataType * typ)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	void * vptr = getVptr(typ);
	assert(id < SlConst::TypeTabSize && (!P_TypeTable[GETSTYPE(id)] || P_TypeTable[GETSTYPE(id)] == vptr));
	P_TypeTable[GETSTYPE(id)] = vptr;
	LEAVE_CRITICAL_SECTION
}

DataType & FASTCALL stype(TYPEID t, DataType * pT)
{
	assert(P_TypeTable[GETSTYPE(t)] != 0);
	pT->S = GETSSIZE(t);
	getVptr(pT) = P_TypeTable[GETSTYPE(t)];
	return *pT;
}

int    FASTCALL btnumber(int bt) { return oneof3(bt, BTS_INT, BTS_REAL, BTS_INT64_); }

uint32 FASTCALL stsize(TYPEID t) 
{ 
	if(GETSTYPE(t) == S_LVAR) { // @v12.2.6 аварийный случай - для этого типа у нас нет манипулятора (и не будет, ибо экзотика)
		return GETSSIZE(t);
	}
	else {
		DataType _t; 
		return stype(t, &_t).size(); 
	}
}

int    FASTCALL stcomp(TYPEID t, const void * i1, const void * i2) { DataType _t; return stype(t, &_t).comp(i1, i2); }
char * FASTCALL sttostr(TYPEID t, const void * d, long f, char * b) { DataType _t; return stype(t, &_t).tostr(d, f, b); }
int    FASTCALL stfromstr(TYPEID t, void * d, long f, const char * b) { DataType _t; return stype(t, &_t).fromstr(d, f, b); }
int    FASTCALL stbase(TYPEID t) { DataType _t; return stype(t, &_t).base(); }
void   FASTCALL sttobase(TYPEID t, const void * data, void * baseData) { DataType _t; stype(t, &_t).tobase(data, baseData); }
int    FASTCALL stbaseto(TYPEID t, void * data, const void * baseData) { DataType _t; return stype(t, &_t).baseto(data, baseData); }
void   FASTCALL stdestroy(TYPEID t, void * d) { DataType _t; stype(t, &_t).destroy(d); }
int    FASTCALL stisnumber(TYPEID t) { DataType _t; return stype(t, &_t).isnumber(); }
void   FASTCALL stminval(TYPEID t, void * d) { DataType _t; stype(t, &_t).minval(d); }
void   FASTCALL stmaxval(TYPEID t, void * d) { DataType _t; stype(t, &_t).maxval(d); }
//
//
//
uint32 DataType::size() const { return S; }
int    DataType::comp(const void * i1, const void * i2) const { return memcmp(i1, i2, size()); }
char * DataType::tostr(const void *, long, char * b) const { return &(b[0] = 0); }

/*virtual*/SString & DataType::ToStr(const void * pData, long format, SString & rBuf) const
{
	rBuf.Z();
	char    temp_buf[4096];
	tostr(pData, format, temp_buf);
	rBuf = temp_buf;
	return rBuf;
}

int    DataType::fromstr(void *, long, const char *) const { return 0; }
int    DataType::base() const { return BTS_VOID; }
void   DataType::tobase(const void * data, void * baseData) const {}
int    DataType::baseto(void * data, const void * baseData) const { return 0; }
void   FASTCALL DataType::destroy(void *) const {}
void   DataType::minval(void *) const { }
void   DataType::maxval(void *) const { }
/*
TYPEID DataType::recornsymb(const char *) const { return 0; }
int    DataType::getsymb(int style, char * pBuf, size_t bufLen) const
{
	strnzcpy(pBuf, "unknown", bufLen);
	return -1;
}
*/

int DataType::Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		size_t sz = size();
		if(ismemzero(pData, sz)) {
			*pInd = 1;
		}
		else {
			*pInd = 0;
			rBuf.Write(pData, sz);
		}
	}
	else if(dir < 0) {
		size_t sz = size();
		if(*pInd == 1) {
			memzero(pData, sz);
		}
		else {
			rBuf.Read(pData, sz);
		}
	}
	return ok;
}
//
//
//
#pragma warn -rvl

TYPEID FASTCALL bt2st(int baseType)
{
	switch(baseType) {
		case BTS_STRING: return MKSTYPE(S_ZSTRING, 0);
		case BTS_INT: return MKSTYPE(S_INT, 4);
		case BTS_REAL: return MKSTYPE(S_FLOAT, 8);
		case BTS_DATE: return MKSTYPE(S_DATE, 4);
		case BTS_TIME: return MKSTYPE(S_TIME, 4);
		case BTS_BOOL: return MKSTYPE(S_BOOL, 4);
		default: return 0;
	}
}

#pragma warn +rvl

int FASTCALL stcast(TYPEID st, TYPEID dt, const void * sd, void * dd, long f)
{
	DataType _t;
	if(st == dt) {
		memmove(dd, sd, stype(dt, &_t).size());
		return 1;
	}
	else {
		char   str_base[8192];
		long   int_base;
		double real_base;
		LDATE  date_base;
		LTIME  time_base;
		const  int sbt = stype(st, &_t).base();
		if(GETSTYPE(dt) == S_LOGICAL) {
			int    cvt = 1;
			if(sbt == BTS_INT) {
				stype(st, &_t).tobase(sd, &int_base);
				int_base = BIN(int_base);
			}
			else if(sbt == BTS_REAL) {
				stype(st, &_t).tobase(sd, &real_base);
				int_base = BIN(real_base);
			}
			else if(sbt == BTS_DATE) {
				stype(st, &_t).tobase(sd, &date_base);
				int_base = BIN(date_base);
			}
			else if(sbt == BTS_TIME) {
				stype(st, &_t).tobase(sd, &time_base);
				int_base = BIN(time_base);
			}
			else if(sbt == BTS_STRING) {
				stype(st, &_t).tobase(sd, &str_base);
				stype(dt, &_t).fromstr(&int_base, 0, str_base);
			}
			else if(sbt == BTS_BOOL) {
				stype(st, &_t).tobase(sd, &int_base);
			}
			else
				cvt = 0;
			if(cvt)
				return stype(dt, &_t).baseto(dd, &int_base);
		}
		const int dbt = stype(dt, &_t).base();
		if(sbt == BTS_STRING) {
			stype(st, &_t).tobase(sd, str_base);
			return stype(dt, &_t).fromstr(dd, f, str_base);
		}
		else if(dbt == BTS_STRING) {
			stype(st, &_t).tostr(sd, f, str_base);
			return stype(dt, &_t).baseto(dd, str_base);
		}
		else {
			if(sbt == BTS_INT) {
				stype(st, &_t).tobase(sd, &int_base);
				if(dbt == BTS_INT)
					return stype(dt, &_t).baseto(dd, &int_base);
				else if(dbt == BTS_REAL)
					return stype(dt, &_t).baseto(dd, &(real_base = (double)int_base));
			}
			if(sbt == BTS_REAL) {
				stype(st, &_t).tobase(sd, &real_base);
				if(dbt == BTS_INT)
					return stype(dt, &_t).baseto(dd, &(int_base = (long)real_base));
				else if(dbt == BTS_REAL)
					return stype(dt, &_t).baseto(dd, &real_base);
			}
			return 0;
		}
	}
}
//
//
//
IMPL_INVARIANT_C(STypEx)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(oneof5(Mod, modNone, modArray, modPtr, modRef, modLink), pInvP);
	//S_ASSERT_P((Flags & fOf && Link) || (!(Flags & fOf) && !Link), pInvP);
	//S_ASSERT_P(!(Flags & fStruct) || Link, pInvP);
	S_ASSERT_P((Mod == modArray && Dim) || (Mod != modArray && !Dim), pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

void STypEx::Init()
{
	THISZERO();
}

bool STypEx::IsPure() const { return (Mod == 0 && !(Flags & (fStruct|fOf))); }

bool FASTCALL STypEx::IsEq(const STypEx & s) const
{
	if(Typ == s.Typ && Dim == s.Dim && Link == s.Link && Mod == s.Mod) {
		if((Flags & (fStruct | fOf)) == (s.Flags & (fStruct | fOf)))
			return true;
	}
	return false;
}

bool FASTCALL STypEx::IsZStr(size_t * pLen) const
{
	if(IsPure() && GETSTYPE(Typ) == S_ZSTRING) {
		ASSIGN_PTR(pLen, GETSSIZE(Typ));
		return true;
	}
	else
		return false;
}

bool FASTCALL STypEx::IsWZStr(size_t * pLen) const
{
	if(IsPure() && GETSTYPE(Typ) == S_WZSTRING) {
		ASSIGN_PTR(pLen, GETSSIZE(Typ)/sizeof(wchar_t));
		return true;
	}
	else
		return false;
}

bool FASTCALL STypEx::IsRaw(size_t * pLen) const
{
	if(IsPure() && GETSTYPE(Typ) == S_RAW) {
		ASSIGN_PTR(pLen, GETSSIZE(Typ));
		return true;
	}
	else
		return false;
}

TYPEID STypEx::GetDbFieldType() const
{
	TYPEID t = 0;
	if(Flags & fOf) {
		if(Mod == modPtr)
			t = MKSTYPE(S_INT, 4);
		else if(Mod == modRef)
			t = MKSTYPE(S_INT, 4);
	}
	else {
		if(Mod == modNone)
			t = Typ;
		else if(Mod == modPtr)
			t = MKSTYPE(S_INT, 4);
		else if(Mod == modRef)
			t = MKSTYPE(S_INT, 4);
		else if(Mod == modLink)
			t = MKSTYPE(S_INT, 4);
		else if(Mod == modArray)
			t = 0;
	}
	return t;
}

size_t STypEx::GetBinSize() const
{
	size_t s = 0;
	if(Flags & fOf) {
		if(Mod == modPtr)
			s = sizeof(uint32);
		else if(Mod == modRef)
			s = sizeof(uint32);
	}
	else {
		if(Mod == modNone)
			s = stsize(Typ);
		else if(Mod == modPtr)
			s = sizeof(uint32);
		else if(Mod == modRef)
			s = sizeof(uint32);
		else if(Mod == modLink)
			s = sizeof(int32);
		else if(Mod == modArray)
			s = (stsize(Typ) * Dim);
	}
	// assert(s != 0);
	return s;
}
//
//
//
TYPEID FASTCALL IsSTypeName(SStrScan & rScan)
{
	struct TypeName {
		TYPEID Typ;
		const  char * P_Names;
		const  char * P_NativeName;
	};

	static const TypeName TypeNameList[] = {
		{0, "undefined,undef", "Не определенный"},
		{MKSTYPE(S_ZSTRING, 0), "string,zstring,char,C",  "Строка"},
		{MKSTYPE(S_INT64, 8),   "int64,integer,long long,__int64", "Целое"}, // @v10.6.3
		{MKSTYPE(S_INT, 8),     "int64,integer,long long,__int64", "Целое"}, // @v10.6.3
		{MKSTYPE(S_UINT, 8),    "uint64,unsigned long long", "Целое без знака"}, // @v10.6.3
		{MKSTYPE(S_INT, 1),     "int8",                   "Целое"},
		{MKSTYPE(S_INT, 2),     "int16,short",            "Целое"},
		{MKSTYPE(S_INT, 4),     "int32,integer,long,int", "Целое"},
		{MKSTYPE(S_UINT, 1),    "uint8,uchar",            "Целое без знака"},
		{MKSTYPE(S_UINT, 2),    "uint16,unsigned short",  "Целое без знака"},
		{MKSTYPE(S_UINT, 4),    "uint32,ulong,uint,unsigned long,unsigned int", "Целое без знака"},
		{MKSTYPE(S_FLOAT, 4),   "float",                "Число"},
		{MKSTYPE(S_FLOAT, 8),   "real,double,N",        "Число"},
		{MKSTYPE(S_FLOAT, 10),  "long double",          "Число"},
		{T_DATE,                "date,LDATE,D",         "Дата"},
		{T_TIME,                "time,LTIME,T",         "Время"},
		{T_DATETIME,            "datetime,LDATETIME",   "Дата-Время"},
		{T_VARIANT,             "variant,VARIANT",      "VB variant"},
		{T_IPOINT2,             "ipoint2",              "Точка 2D"},
		{T_FPOINT2,             "fpoint2",              "Точка 2D"},
		{T_BOOL,                "boolean,bool,logical", "Логический"}
	};
	rScan.Skip();
	const SString text(rScan);
	SString temp_buf;
	for(uint i = 0; i < SIZEOFARRAY(TypeNameList); i++) {
		StringSet ss(',', TypeNameList[i].P_Names);
		for(uint p = 0; ss.get(&p, temp_buf);) {
			if(text.HasPrefixNC(temp_buf)) {
				uchar next2 = static_cast<uchar>(text[temp_buf.Len()]);
				if(!isalnum(next2) && next2 != '_') {
					rScan.SetLen(temp_buf.Len());
					return TypeNameList[i].Typ;
				}
			}
		}
	}
	rScan.SetLen(0);
	return 0;
}
//
// ARG(style IN):
//   0 - c++
//   1 - DL600
//
SString & GetBinaryTypeString(TYPEID typ, int style, SString & rBuf, const char * pFldName, uint indent)
{
	rBuf.Z();
	size_t sz = stsize(typ);
	size_t dim = 0;
	SString type_text;
	switch(GETSTYPE(typ)) {
		case S_AUTOINC:
		case S_INT:
			if(sz == 1)
				type_text = "int8";
			else if(sz == 2)
				type_text = "int16";
			else if(sz == 4)
				type_text = "int32";
			else if(sz == 8)
				type_text = "int64";
			else
				(type_text = "int_invalid_size_").Cat(sz);
			break;
		case S_INT64: type_text = "int64"; break;
		case S_UINT64: type_text = "uint64"; break; // @v11.9.2
		case S_LOGICAL: type_text = "bool"; break;
		case S_UINT:
			if(sz == 1)
				type_text = "uint8";
			else if(sz == 2)
				type_text = "uint16";
			else if(sz == 4)
				type_text = "uint32";
			else if(sz == 8)
				type_text = "uint64";
			else
				(type_text = "uint_invalid_size_").Cat(sz);
			break;
		case S_FLOAT:
			if(sz == 4)
				type_text = "float";
			else if(sz == 8)
				type_text = "double";
			else if(sz == 10)
				type_text = "long double";
			else
				(type_text = "float_invalid_size_").Cat(sz);
			break;
		case S_DATE:
			if(style == 0)
				type_text = "LDATE";
			else if(style == 1)
				type_text = "date";
			break;
		case S_TIME:
			if(style == 0)
				type_text = "LTIME";
			else if(style == 1)
				type_text = "time";
			break;
		case S_DATETIME:
			if(style == 0)
				type_text = "LDATETIME";
			else if(style == 1)
				type_text = "datetime";
			break;
		case S_MONEY:
		case S_NUMERIC:
			if(style == 0)
				type_text = "char";
			else if(style == 1)
				type_text = "money";
			dim = sz;
			break;
		case S_LSTRING:
			if(style == 0)
				type_text = "char";
			else if(style == 1)
				type_text = "lstring";
			dim = sz;
			break;
		case S_CHAR:
		case S_ZSTRING:
			if(style == 0)
				type_text = "char";
			else if(style == 1)
				type_text = "string";
			dim = sz;
			break;
		case S_NOTE:
			if(style == 0)
				type_text = "char";
			else if(style == 1)
				type_text = "note";
			dim = sz;
			break;
		case S_WCHAR:
			type_text = "wchar_t";
			dim = sz / sizeof(wchar_t);
			break;
		case S_WZSTRING:
			type_text = "wchar_t";
			dim = sz / sizeof(wchar_t);
			break;
		case S_RAW:
			type_text = "uint8";
			dim = sz;
			break;
		case S_INTRANGE: type_text = "IntRange"; break;
		case S_REALRANGE: type_text = "RealRange"; break;
		case S_DATERANGE: type_text = "DateRange"; break;
		case S_VARIANT: type_text = "VARIANT"; break;
		case S_VOID: type_text = "void"; break;
		case S_BLOB:
		case S_CLOB:
			if(sz > 32)
				(type_text = "TSLob").Space().CatChar('<').Cat(sz).CatChar('>');
			else
				type_text = "SLob";
			break;
		case S_IPOINT2:
			if(style == 0)
				type_text = "SPoint2S";
			else if(style == 1)
				type_text = "ipoint2";
			break;
		case S_FPOINT2:
			if(style == 0)
				type_text = "SPoint2F";
			else if(style == 1)
				type_text = "fpoint2";
			break;
		case S_UUID_:
			type_text = "S_GUID";
			break;
		case S_LVAR: // @v12.2.6 исключительный случай. Этот тип практически не применяется. Сейчас он понадобился для ситуативной цели.
			type_text = "LVAR";
			dim = sz;
			break;
		default:
			type_text = "unknown";
			break;
	}
	rBuf = type_text;
	if(pFldName) {
		if(indent <= rBuf.Len() && pFldName[0])
			indent = rBuf.Len()+1;
		rBuf.Align(indent, ADJ_LEFT).Cat(pFldName);
		if(dim > 1)
			rBuf.CatChar('[').Cat(dim).CatChar(']');
	}
	return rBuf;
}

SString & FASTCALL GetBaseTypeString(int baseTyp, long flags, SString & rBuf)
{
	switch(baseTyp) {
		case BTS_STRING: rBuf = (flags & BTSF_NATIVE) ? "Строка" : "string"; break;
		case BTS_INT: rBuf = (flags & BTSF_NATIVE) ? "Целое число" : "integer"; break;
		case BTS_INT64_: rBuf = (flags & BTSF_NATIVE) ? "Целое число" : "integer"; break;
		case BTS_REAL: rBuf = (flags & BTSF_NATIVE) ? "Число" : "real"; break;
		case BTS_DATE: rBuf = (flags & BTSF_NATIVE) ? "Дата" : "date"; break;
		case BTS_TIME: rBuf = (flags & BTSF_NATIVE) ? "Время" : "time"; break;
		case BTS_POINT2: rBuf = (flags & BTSF_NATIVE) ? "Точка 2D" : "point2d"; break;
		case BTS_BOOL: rBuf = (flags & BTSF_NATIVE) ? "Логический" : "bool"; break;
		default: rBuf = (flags & BTSF_NATIVE) ? "Не определенный" : "undefined";
	}
	if(flags & BTSF_NATIVE) {
		rBuf.Transf((flags & BTSF_OEM) ? CTRANSF_UTF8_TO_INNER : CTRANSF_UTF8_TO_OUTER);
	}
	return rBuf;
}
