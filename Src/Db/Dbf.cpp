// DBF.CPP
// Copyright (c) A. Sobolev 1993-2001, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2018
//
#include <db.h>
#pragma hdrstop
#include <dbf.h>
//
//
//
#ifndef _WIN32_WCE // @v5.1.7 AHTOXA
int DBFF::GetSType(TYPEID * pTyp, long * pFmt) const
{
	// Тип поля (C,L,N,M,F,D)
	int    ok = 1;
	TYPEID typ = 0;
	long   fmt = 0;
	switch(ftype) {
		case 'C':
			typ = MKSTYPE(S_ZSTRING, fsize+1);
			fmt = MKSFMT(fsize, 0);
			SETSFMTFLAG(fmt, SFMTFLAG(fmt) | ALIGN_LEFT);
			break;
		case 'L':
			// @v8.1.2 typ = MKSTYPE(S_INT, 2);
			typ = T_BOOL; // @v8.1.2
			fmt = MKSFMT(fsize, ALIGN_LEFT);
			break;
		case 'N':
			if(fprec) {
				typ = MKSTYPE(S_FLOAT, 8);
				fmt = MKSFMTD(fsize, fprec, ALIGN_RIGHT);
			}
			else {
				typ = MKSTYPE(S_INT, 4);
				fmt = MKSFMT(fsize, ALIGN_RIGHT);
			}
			break;
		case 'M':
			typ = (TYPEID)MKSTYPE(S_ZSTRING, 128);
			break;
		case 'F':
			typ = MKSTYPE(S_FLOAT, 8);
			fmt = MKSFMTD(fsize, fprec, ALIGN_RIGHT);
			break;
		case 'D':
			typ = MKSTYPE(S_DATE, 4);
			fmt = MKSFMT(8, DATF_DMY|ALIGN_LEFT);
			break;
		default:
			ok = 0;
			break;
	}
	ASSIGN_PTR(pTyp, typ);
	ASSIGN_PTR(pFmt, fmt);
	return ok;
}
#endif
//
//
//
SLAPI DBFCreateFld::DBFCreateFld()
{
	THISZERO();
}

void SLAPI DBFCreateFld::Init(const char * pName, int typ, uint sz, uint prec)
{
	STRNSCPY(Name, pName);
	Type = typ;
	Size = sz;
	Prec = prec;
}
//
// DbfRecord
//
SLAPI DbfRecord::DbfRecord(const DbfTable * pTbl) : P_Tbl(pTbl)
{
	P_Buffer = new char[BufSize = P_Tbl->getRecSize()];
	empty();
}

SLAPI DbfRecord::~DbfRecord()
{
	delete P_Buffer;
}

SCodepage SLAPI DbfRecord::getCodePage() const
{
	return P_Tbl ? P_Tbl->getCodePage() : cpUndef;
}

int SLAPI DbfRecord::empty()
{
	memzero(P_Buffer, BufSize);
	P_Buffer[0] = ' ';
	return 1;
}

#ifndef _WIN32_WCE // {

int SLAPI DbfRecord::put(const SdRecord & rRec)
{
	int    ok = 1;
	int    fld_no = 0;
	SdbField fld;
	for(uint i = 0; i < rRec.GetCount(); i++) {
		THROW(rRec.GetFieldByPos(i, &fld));
		if(getFieldNumber(fld.Name, &fld_no))
			THROW(put(fld_no, fld.T.Typ, rRec.GetDataC(i)));
	}
	CATCHZOK
	return ok;
}

int SLAPI DbfRecord::get(SdRecord & rRec) const
{
	int    ok = 1;
	int    fld_no = 0;
	SdbField fld;
	const  uint c = rRec.GetCount();
	for(uint i = 0; i < c; i++) {
		THROW(rRec.GetFieldByPos(i, &fld));
		if(getFieldNumber(fld.Name, &fld_no))
			THROW(get(fld_no, fld.T.Typ, rRec.GetData(i)));
	}
	CATCHZOK
	return ok;
}

#endif // } !_WIN32_WCE

int SLAPI DbfRecord::getFieldName(uint fldN, char * pFldName, size_t bufLen) const
{
	return P_Tbl ? P_Tbl->getFieldName(fldN, pFldName, bufLen) : 0;
}

int FASTCALL DbfRecord::getFieldNumber(const char * pFldName, int * pFldN /* 1.. */) const
{
	return P_Tbl ? P_Tbl->getFieldNumber(pFldName, pFldN) : 0;
}

int FASTCALL DbfRecord::getFieldType(uint fldN, int * pType)
{
	DBFF   fld;
	if(P_Tbl && P_Tbl->getField(fldN, &fld)) {
		ASSIGN_PTR(pType, fld.ftype);
		return 1;
	}
	else {
		ASSIGN_PTR(pType, 0);
		return 0;
	}
}

#ifndef _WIN32_WCE // @v5.1.7 AHTOXA
int SLAPI DbfRecord::put(int fldN, TYPEID typ, const void * pData)
{
	char   temp_buf[256];
	int    bt = stbase(typ);
	if(bt == BTS_STRING) {
		sttobase(typ, pData, temp_buf);
		put(fldN, temp_buf);
	}
	else if(bt == BTS_INT) {
		long base_long;
		sttobase(typ, pData, &base_long);
		put(fldN, base_long);
	}
	else if(bt == BTS_REAL) {
		double base_real;
		sttobase(typ, pData, &base_real);
		put(fldN, base_real);
	}
	else if(bt == BTS_DATE) {
		LDATE base_date;
		sttobase(typ, pData, &base_date);
		put(fldN, base_date);
	}
	// @v8.1.2 {
	else if(bt == BTS_BOOL) {
		long base_long = 0;
		sttobase(typ, pData, &base_long);
		put(fldN, base_long ? 'T' : 'F');
	}
	// } @v8.1.2
	else {
		put(fldN, sttostr(typ, pData, TIMF_HMS, temp_buf));
	}
	return 1;
}
#endif

int SLAPI DbfRecord::put(int fld, const char * data)
{
	DBFF   f;
	if(P_Tbl->getField(fld, &f)) {
		if(f.ftype == 'T' && f.fsize == 8) {
			int   d = 1, m = 1, y = 2000;
			long  first_date = 0x00256859, date = 0L;
			char  buf[128];
			LDATE dt, f_dt;
			encodedate(d, m, y, &f_dt);
			STRNSCPY(buf, data);
			d = atoi(buf + 6);
			buf[6] = 0;
			m = atoi(buf + 4);
			buf[4] = 0;
			y = atoi(buf);
			encodedate(d, m, y, &dt);
			memcpy(P_Buffer + f.offset + 4, &date, 4);
			if(dt != ZERODATE)
				date = first_date + diffdate(dt, f_dt);
			memcpy(P_Buffer + f.offset, &date, 4);
		}
		else {
			int    len = strlen(data);
			if(len > f.fsize)
				len = f.fsize;
			memcpy(P_Buffer + f.offset, data, len);
			memset(P_Buffer + f.offset + len, ' ', f.fsize - len);
		}
		return 1;
	}
	else
		return 0;
}

int SLAPI DbfRecord::put(int fld, double data)
{
	DBFF f;
	if(P_Tbl->getField(fld, &f)) {
		char buf[256];
		if(f.ftype == 'I') {
			long   ldata = (long)data;
			memzero(buf, sizeof(buf));
			if(f.fsize == 4)
				memcpy(buf, &ldata, 4);
		}
		else if(f.ftype == 'L') {
			buf[0] = (data == 0) ? 'F' : 'T';
			buf[1] = 0;
		}
		else {
			realfmt(data, MKSFMTD(f.fsize, f.fprec, ALIGN_RIGHT), buf);
			if(strlen(buf) > f.fsize)
				memset(buf, '*', f.fsize);
		}
		memcpy(P_Buffer + f.offset, buf, f.fsize);
		return 1;
	}
	else
		return 0;
}

int SLAPI DbfRecord::put(int fld, float data) { return put(fld, (double)data); }
int SLAPI DbfRecord::put(int fld, long data) { return put(fld, (double)data); }
int SLAPI DbfRecord::put(int fld, int data) { return put(fld, (double)data); }

int SLAPI DbfRecord::put(int fld, const DBFDate * data)
{
	//char   tmp[32];
	//sprintf(tmp, "%04d%02d%02d", data->year, data->month, data->day);
	//return put(fld, tmp);
	SString temp_buf;
	return put(fld, temp_buf.CatLongZ((long)data->year, 4).CatLongZ((long)data->month, 2).CatLongZ((long)data->day, 2));
}

int SLAPI DbfRecord::put(int fld, LDATE dt)
{
	DBFDate dbf_dt;
	dbf_dt.year  = dt.year();
	dbf_dt.month = dt.month();
	dbf_dt.day   = dt.day();
	return put(fld, &dbf_dt);
}

char * FASTCALL DbfRecord::getFldBuf(int fldN, char * buf) const
{
	DBFF   f;
	if(P_Tbl->getField(fldN, &f)) {
		memcpy(buf, P_Buffer + f.offset, f.fsize);
		buf[f.fsize] = '\0';
		return buf;
	}
	else {
		buf[0] = 0;
		return 0;
	}
}

int SLAPI DbfRecord::get(int fldN, char * data, int skipEmpty) const
{
	int    ok = getFldBuf(fldN, data) ? 1 : 0;
	if(ok && skipEmpty) {
		if(*strip(data) == 0)
			ok = 0;
	}
	return ok;
}

int SLAPI DbfRecord::get(int fldN, SString & rBuf, int skipEmpty) const
{
	char   temp_buf[1024];
	int    ok = getFldBuf(fldN, temp_buf) ? 1 : 0;
	rBuf = temp_buf;
	if(ok && skipEmpty) {
		if(!rBuf.NotEmptyS())
			ok = 0;
	}
	return ok;
}

int SLAPI DbfRecord::get(int fldN, double & data) const
{
	char   tmp[256];
	if(getFldBuf(fldN, tmp) == 0) {
		data = 0;
		return 0;
	}
	data = atof(tmp);
	return 1;
}

int SLAPI DbfRecord::get(int fldN, float & data) const
{
	char   tmp[256];
	if(getFldBuf(fldN, tmp) == 0) {
		data = 0;
		return 0;
	}
	data = (float)atof(tmp);
	return 1;
}

int SLAPI DbfRecord::get(int fldN, long & data) const
{
	int    ok = 1;
	char   tmp[256];
	if(getFldBuf(fldN, tmp) == 0) {
		data = 0;
		ok = 0;
	}
	else
		data = atol(tmp);
	return ok;
}

int SLAPI DbfRecord::get(int fldN, int64 & data) const
{
	char   tmp[256];
	if(getFldBuf(fldN, tmp) == 0) {
		data = 0;
		return 0;
	}
	data = _atoi64(tmp);
	return 1;
}

int SLAPI DbfRecord::get(int fldN, int & data) const
{
	char   tmp[256];
	if(getFldBuf(fldN, tmp) == 0) {
		data = 0;
		return 0;
	}
	else {
		data = atoi(tmp);
		return 1;
	}
}

int SLAPI DbfRecord::get(int fldN, DBFDate * data) const
{
	char   tmp[256];
	if(getFldBuf(fldN, tmp) == 0) {
		memzero(data, sizeof(*data));
		return 0;
	}
	else {
		tmp[8] = 0;
		data->day   = atoi(tmp+6); tmp[6] = 0;
		data->month = atoi(tmp+4); tmp[4] = 0;
		data->year  = atoi(tmp);
		return 1;
	}
}

int SLAPI DbfRecord::get(int fldN, LDATE & data) const
{
	DBFDate d;
	if(get(fldN, &d)) {
		encodedate((int)d.day, (int)d.month, (int)d.year, &data);
		return 1;
	}
	return 0;
}

int SLAPI DbfRecord::get(int fldN, LTIME & data) const
{
	char   str[256];
	if(get(fldN, str)) {
		strtotime(str, 0, &data);
		return 1;
	}
	return 0;
}

#ifndef _WIN32_WCE // @v5.1.7 AHTOXA
int SLAPI DbfRecord::get(int fldN, TYPEID typ, void * pBuf) const
{
	int    ok = 1;
	union {
		char   Str[4096]; // @v8.1.2 [512]-->[4096]
		long   Lv;
		double Rv;
		LDATE  Dv;
		LTIME  Tv;
	} temp_buf;
	switch(stbase(typ)) {
		case BTS_STRING:
			ok = get(fldN, temp_buf.Str);
			strip(temp_buf.Str);
			break;
		case BTS_INT:
			ok = get(fldN, temp_buf.Lv);
			break;
		case BTS_REAL:
			ok = get(fldN, temp_buf.Rv);
			break;
		case BTS_DATE:
			ok = get(fldN, temp_buf.Dv);
			break;
		case BTS_TIME:
			ok = get(fldN, temp_buf.Tv);
			break;
		// @v8.1.2 {
		case BTS_BOOL:
			ok = get(fldN, temp_buf.Str);
			break;
		// } @v8.1.2
		default:
			ok = 0;
			break;
	}
	if(ok)
		stbaseto(typ, pBuf, &temp_buf);
	return ok;
}
#endif
//
// DbfTable
//
int SLAPI DbfTable::initBuffer()
{
	BufSize = 1024 * 8;
	P_Buffer  = (char *)SAlloc::M(BufSize);
	if(P_Buffer == 0)
		BufSize = 0;
	return (BufSize != 0);
}

int SLAPI DbfTable::releaseBuffer()
{
	ZFREE(P_Buffer);
	return 1;
}

SLAPI DbfTable::DbfTable(const char * pName) : P_Buffer(0), Opened(0), Mod(0), Current(0),
	P_Flds(0), NumFlds(0), Stream(0), BFirst(0), BLast(0), P_Name(newStr(pName))
{
	open();
}

SLAPI DbfTable::~DbfTable()
{
	close();
	delete P_Name;
}

int SLAPI DbfTable::open()
{
	int    ok = 0;
	if(Opened)
		ok = 1;
	else {
#ifndef _WIN32_WCE // @v5.0.4 AHTOXA
		THROW(fileExists(P_Name));
#endif
		THROW_S_S(Stream = fopen(P_Name, "r+b"), SLERR_OPENFAULT, P_Name);
		Mod = 0;
		THROW(initBuffer());
		THROW(getHeader());
		THROW(getFields());
		Opened = 1;
		top();
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI DbfTable::close()
{
	if(Opened) {
		flush();
		SFile::ZClose(&Stream);
		Opened  = 0;
		Current = 0;
		ZDELETE(P_Flds);
		NumFlds = 0;
		BFirst  = BLast = 0;
		releaseBuffer();
	}
	return 1;
}

int SLAPI DbfTable::flush()
{
	if(Mod) {
		//flushBuffer();
		if(Mod) {
			putHeader();
			putFields();
		}
	}
	return 1;
}

struct DbfCpEntry {
	SCodepage Cp;
	uint8  LdId;
};

static const DbfCpEntry __DbfCpEntries[] = {
	{ cpANSI, 0x57},
	{ cpOEM,  0x00},
	{ cp437,  0x01},
	{ cp737,  0x6A},
	{ cp850,  0x02},
	{ cp852,  0x64},
	{ cp857,  0x6B},
	{ cp861,  0x67},
	{ cp865,  0x66},
	{ cp866,  0x65},
	{ cp932,  0x7B},
	{ cp936,  0x7A},
	{ cp950,  0x78},
	{ cp1250, 0xC8},
	{ cp1251, 0xC9},
	{ cp1252, 0x03},
	{ cp1253, 0xCB},
	{ cp1254, 0xCA},
	{ cp1255, 0xC8},
	{ cp1256, 0x7E}
};

static SCodepage LdIdToCp(uint8 ldid)
{
	SCodepage cp = cpUndef;
	for(uint i = 0; cp == cpUndef && i < SIZEOFARRAY(__DbfCpEntries); i++) {
		if(ldid == __DbfCpEntries[i].LdId)
			cp = __DbfCpEntries[i].Cp;
	}
	return cp;
}

static uint8 CpToLdId(SCodepage cp)
{
	uint8  ldid = 0;
	for(uint i = 0; !ldid && i < SIZEOFARRAY(__DbfCpEntries); i++) {
		if(cp == __DbfCpEntries[i].Cp)
			ldid = __DbfCpEntries[i].LdId;
	}
	/*
	switch(cp) {
		case cpANSI: ldid = 0x57; break;
		case cpOEM:  ldid = 0x00; break;
		case cp437:  ldid = 0x01; break;
		case cp737:  ldid = 0x6A; break;
		case cp850:  ldid = 0x02; break;
		case cp852:  ldid = 0x64; break;
		case cp857:  ldid = 0x6B; break;
		case cp861:  ldid = 0x67; break;
		case cp865:  ldid = 0x66; break;
		case cp866:  ldid = 0x65; break;
		case cp932:  ldid = 0x7B; break;
		case cp936:  ldid = 0x7A; break;
		case cp950:  ldid = 0x78; break;
		case cp1250: ldid = 0xC8; break;
		case cp1251: ldid = 0xC9; break;
		case cp1252: ldid = 0x03; break;
		case cp1253: ldid = 0xCB; break;
		case cp1254: ldid = 0xCA; break;
		case cp1255: ldid = 0xC8; break;
		case cp1256: ldid = 0x7E; break;
	}
	*/
	return ldid;
}

SCodepage SLAPI DbfTable::getCodePage() const { return LdIdToCp(Head.LdID); }
ulong  SLAPI DbfTable::getNumRecs() const { return Head.NumRecs; }
size_t SLAPI DbfTable::getRecSize() const { return Head.RecSize; }
uint   SLAPI DbfTable::getNumFields() const { return NumFlds; }

int SLAPI DbfTable::create(int aNumFlds, const DBFCreateFld * pFldDescr, SCodepage cp, int infoByte)
{
	int    ok = 1;
	int    d = 0, m = 0, y = 0;
	LDATE  cur_dt = getcurdate_();
	// @v9.3.5 {
	if(!checkdate(cur_dt, 0))
		cur_dt = encodedate(1, 1, 2016);
	// } @v9.3.5
	close();
	FILE * f = fopen(P_Name, "wb");
	THROW_S_S(f, SLERR_OPENFAULT, P_Name);
	MEMSZERO(Head);
	{
		Head.Info = (infoByte >= 0 && infoByte <= 0x7f) ? infoByte : 3;
		const size_t dbc_path_size = (Head.Info == 0x30) ? 263 : 0;
		decodedate(&d, &m, &y, &cur_dt);
		Head.Year   = y % 100;
		Head.Month  = (int8)m;
		Head.Day    = (int8)d;
		Head.LdID   = CpToLdId(cp);
		Head.NumRecs = 0L;
		Head.HeadSize = (uint16)(sizeof(DBFH) + aNumFlds * sizeof(DBFF) + 1 + dbc_path_size);
		Head.RecSize  = 1;
		memzero(P_Flds = new DBFF[aNumFlds], aNumFlds * sizeof(DBFF));
		unsigned offset = 1;
		for(int i = 0; i < aNumFlds; i++) {
			strncpy(P_Flds[i].fname, pFldDescr[i].Name, 10);
			P_Flds[i].ftype = pFldDescr[i].Type;
			Head.RecSize += (P_Flds[i].fsize = pFldDescr[i].Size);
			P_Flds[i].fprec = pFldDescr[i].Prec;
			P_Flds[i].offset = offset;
			offset += P_Flds[i].fsize;
		}
		fseek(f, 0L, SEEK_SET);
		THROW_S_S(fwrite(&Head, sizeof(DBFH), 1, f) == 1, SLERR_WRITEFAULT, P_Name);
		//mod = 1;
		THROW_S_S(fwrite(P_Flds, sizeof(DBFF), aNumFlds, f) == aNumFlds, SLERR_WRITEFAULT, P_Name);
		THROW_S_S(fputc('\x0D', f) != EOF, SLERR_WRITEFAULT, P_Name);
		// @v8.4.3 {
		if(dbc_path_size) {
			uint8 dbc_path[512];
			memzero(dbc_path, sizeof(dbc_path));
			THROW_S_S(fwrite(dbc_path, dbc_path_size, 1, f) == 1, SLERR_WRITEFAULT, P_Name);
		}
		// } @v8.4.3
	}
	ZDELETE(P_Flds);
	CATCHZOK
	SFile::ZClose(&f);
	return ok;
}

int SLAPI DbfTable::getPosition(ulong * pPos) const
{
	if(Opened) {
		ASSIGN_PTR(pPos, Current);
		return 1;
	}
	else {
		ASSIGN_PTR(pPos, 0);
		return 0;
	}
}

int SLAPI DbfTable::getHeader()
{
	int    ok = 1;
	long   pos = ftell(Stream);
	SLS.SetAddedMsgString(P_Name);
	fseek(Stream, 0L, SEEK_SET);
	THROW_S_S(fread(&Head, sizeof(DBFH), 1, Stream), SLERR_READFAULT, P_Name);
	{
		const size_t dbc_path_size = (Head.Info == 0x30) ? 263 : 0;
		THROW_S_S(Head.Month >= 1 && Head.Month <= 12, SLERR_DBF_INVHEADER, P_Name);
		THROW_S_S(Head.Day >= 1 && Head.Day <= 31, SLERR_DBF_INVHEADER, P_Name);
		THROW_S_S(Head.HeadSize >= sizeof(Head) && Head.HeadSize <= 8192, SLERR_DBF_INVHEADER, P_Name);
		THROW_S_S(Head.RecSize <= 16*1024, SLERR_DBF_INVHEADER, P_Name);
		NumFlds = (Head.HeadSize - 1 - sizeof(DBFH) - dbc_path_size) / sizeof(DBFF);
		THROW_S_S(NumFlds <= 1024, SLERR_DBF_INVHEADER, P_Name);
		fseek(Stream, pos, SEEK_SET);
	}
	CATCHZOK
	return ok;
}

int SLAPI DbfTable::getFields()
{
	int    ok = 1;
	long   pos = ftell(Stream);
	ZDELETE(P_Flds);
	THROW_S(P_Flds = new DBFF[NumFlds], SLERR_NOMEM);
	fseek(Stream, sizeof(DBFH), SEEK_SET);
	THROW_S_S(fread(P_Flds, sizeof(DBFF), NumFlds, Stream) == NumFlds, SLERR_READFAULT, P_Name);
	int    offs = 1;
	for(uint i = 0; i < NumFlds; i++) {
		P_Flds[i].offset = offs;
		offs += P_Flds[i].fsize;
	}
	fseek(Stream, pos, SEEK_SET);
	CATCHZOK
	return ok;
}

int SLAPI DbfTable::putHeader()
{
	int    ok = 1;
	long   pos = ftell(Stream);
	fseek(Stream, 0L, SEEK_SET);
	THROW_S_S(fwrite(&Head, sizeof(DBFH), 1, Stream) == 1, SLERR_WRITEFAULT, P_Name);
	fseek(Stream, pos, SEEK_SET);
	CATCHZOK
	return ok;
}

int SLAPI DbfTable::putFields()
{
	int    ok = 1;
	long   pos = ftell(Stream);
	fseek(Stream, sizeof(DBFH), SEEK_SET);
	THROW_S_S(fwrite(P_Flds, sizeof(DBFF), NumFlds, Stream) == NumFlds, SLERR_WRITEFAULT, P_Name);
	THROW_S_S(fputc('\x0D', Stream) != EOF, SLERR_WRITEFAULT, P_Name);
	CATCHZOK
	fseek(Stream, pos, SEEK_SET);
	return ok;
}

int SLAPI DbfTable::getFieldName(uint fldNumber, char * pFldName, size_t bufLen) const
{
	if(fldNumber > 0 && fldNumber <= NumFlds) {
		char   temp_buf[32];
		strnzcpy(temp_buf, P_Flds[fldNumber-1].fname, 11);
		strip(temp_buf);
		strnzcpy(pFldName, temp_buf, bufLen);
		return 1;
	}
	ASSIGN_PTR(pFldName, 0);
	return 0;
}

int FASTCALL DbfTable::getFieldNumber(const char * pFldName, int * pFldNumber) const
{
	const  DBFF * p_fld = P_Flds;
	for(uint i = 0; i < NumFlds; i++) {
		if(strnicmp(pFldName, p_fld->fname, 11) == 0) {
			ASSIGN_PTR(pFldNumber, i+1);
			return 1;
		}
		p_fld++;
	}
	ASSIGN_PTR(pFldNumber, 0);
	return 0;
}

int FASTCALL DbfTable::getField(uint fldN, DBFF * pField) const
{
	if(fldN > 0 && fldN <= NumFlds) {
		ASSIGN_PTR(pField, P_Flds[fldN-1]);
		return 1;
	}
	else
		return 0;
}

int FASTCALL DbfTable::goToRec(ulong recNo)
{
	if(!Opened || recNo < 1 || recNo > Head.NumRecs)
		return 0;
	else {
		fseek(Stream, Head.HeadSize + (recNo - 1) * Head.RecSize, SEEK_SET);
		Current = recNo;
		return 1;
	}
}

int SLAPI DbfTable::top()
{
	if(!Opened)
		return 0;
	else {
		fseek(Stream, Head.HeadSize, SEEK_SET);
		Current = 1L;
		return 1;
	}
}

int SLAPI DbfTable::bottom()
{
	return goToRec(Head.NumRecs);
}

int SLAPI DbfTable::next()
{
	if(!Opened || Current >= Head.NumRecs)
		return 0;
	else {
		fseek(Stream, Head.RecSize, SEEK_CUR);
		Current++;
		return 1;
	}
}

int SLAPI DbfTable::prev()
{
	if(!Opened || Current <= 1)
		return 0;
	else {
		fseek(Stream, -Head.RecSize, SEEK_CUR);
		Current--;
		return 1;
	}
}

DbfRecord * SLAPI DbfTable::makeRec() const
{
	return new DbfRecord(this);
}

int SLAPI DbfTable::isDeletedRec()
{
	long   p = ftell(Stream);
	int8   s[4];
	int    r = ((fread(s, 1, 1, Stream) == 1) && s[0] == '*') ? 1 : SLS.SetError(SLERR_READFAULT, P_Name);
	fseek(Stream, p, SEEK_SET);
	return r;
}

int FASTCALL DbfTable::getRec(DbfRecord * pRec)
{
	long   p = ftell(Stream);
	int    ok = (fread(pRec->P_Buffer, Head.RecSize, 1, Stream) == 1) ? 1 : SLS.SetError(SLERR_READFAULT, P_Name);
	fseek(Stream, p, SEEK_SET);
	return ok;
}

int FASTCALL DbfTable::updateRec(DbfRecord * pRec)
{
	int    ok = 1;
	long   p = ftell(Stream);
	if(fwrite(pRec->P_Buffer, Head.RecSize, 1, Stream) == 1)
		Mod = 1;
	else
		ok = SLS.SetError(SLERR_WRITEFAULT, P_Name);
	fseek(Stream, p, SEEK_SET);
	return ok;
}

int SLAPI DbfTable::deleteRec()
{
	int    ok = 1;
	char   c = '*';
	if(fwrite(&c, 1, 1, Stream) == 1)
		Mod = 1;
	else
		ok = SLS.SetError(SLERR_WRITEFAULT, P_Name);
	return ok;
}

int FASTCALL DbfTable::appendRec(DbfRecord * pRec)
{
	int    ok = 1;
	long   p = Head.HeadSize + Head.NumRecs * Head.RecSize;
	fseek(Stream, p, SEEK_SET);
	THROW_S_S(fwrite(pRec->P_Buffer, Head.RecSize, 1, Stream) == 1, SLERR_WRITEFAULT, P_Name);
	THROW_S_S(fputc('\x1A', Stream) != EOF, SLERR_WRITEFAULT, P_Name);
	Current = ++Head.NumRecs;
	Mod = 1;
	CATCHZOK
	fseek(Stream, p, SEEK_SET);
	return ok;
}

ulong SLAPI DbfTable::recPerBuffer() const
{
	return DbfTable::BufSize / Head.RecSize;
}

int SLAPI DbfTable::flushBuffer()
{
	int    ok = 1;
	THROW_S_S(Opened, SLERR_DBF_NOTOPENED, P_Name);
	long pos = ftell(Stream);
	fseek(Stream, Head.HeadSize + (BFirst - 1) * Head.RecSize, SEEK_SET);
	THROW_S_S(fwrite(P_Buffer, (size_t)(Head.RecSize * (BLast - BFirst + 1)), 1, Stream) == 1, SLERR_WRITEFAULT, P_Name);
	if(BLast == Head.NumRecs)
		fputc('\x1A', Stream);
	fseek(Stream, pos, SEEK_SET);
	CATCHZOK
	return ok;
}

int SLAPI DbfTable::loadBuffer(long beg, long end)
{
	int    ok = 1;
	THROW_S_S(Opened, SLERR_DBF_NOTOPENED, P_Name);
	long pos = ftell(Stream);
	fseek(Stream, Head.HeadSize + (beg - 1) * Head.RecSize, SEEK_SET);
	THROW_S_S(fread(P_Buffer, (size_t)(Head.RecSize * (end - beg + 1)), 1, Stream) == 1, SLERR_READFAULT, P_Name);
	fseek(Stream, pos, SEEK_SET);
	BFirst = beg;
	BLast  = end;
	CATCHZOK
	return ok;
}

int SLAPI DbfTable::loadBuffer(ulong recNo)
{
	int    ok = 1;
	THROW_S_S(Opened, SLERR_DBF_NOTOPENED, P_Name);
	THROW_S_S(recNo < 1 || recNo > Head.NumRecs, SLERR_DBF_INVRECNO, P_Name);
	if(recNo < BFirst || recNo > BLast) {
		THROW(flushBuffer());
		ulong  recperbuf = recPerBuffer();
		if(recNo < recperbuf) {
			BFirst = 1;
			BLast  = MIN(recperbuf, Head.NumRecs);
		}
		else
			if((Head.NumRecs - recNo) < recperbuf) {
				BFirst = Head.NumRecs - recperbuf + 1;
				BLast  = Head.NumRecs;
			}
			else {
				BFirst = recNo - recperbuf / 2;
				BLast  = MIN(BFirst + recperbuf - 1, Head.NumRecs);
			}
		THROW(loadBuffer(BFirst, BLast));
	}
	CATCHZOK
	return ok;
}
