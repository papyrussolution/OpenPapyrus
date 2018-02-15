// STRSET.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

size_t FASTCALL SnapUpSize(size_t i); // @prototype

int FASTCALL StringSet::Alloc(size_t sz)
{
	int    ok = 1;
	if(sz == 0) {
		clear();
	}
	else if(sz > Size) {
		size_t new_size = SnapUpSize(sz);
		char * p = 0;
		if((7 * Size) < (8 * DataLen)) { // Assume probability of a non-moving realloc is 0.125
			// If L is close to Size in size then use realloc to reduce the memory defragmentation
			p = (char *)SAlloc::R(P_Buf, new_size);
		}
		else {
			// If L is not close to Size then avoid the penalty of copying
			// the extra bytes that are allocated, but not considered part of the string
			p = (char *)SAlloc::M(new_size);
			if(!p)
				p = (char *)SAlloc::R(P_Buf, new_size);
			else {
				if(DataLen)
					memcpy(p, P_Buf, DataLen);
				SAlloc::F(P_Buf);
			}
		}
		if(p) {
			Size = new_size;
			P_Buf = p;
		}
		else
			ok = 0;
	}
	return ok;
}

int SLAPI StringSet::Init(const char * pDelim, size_t prealloc)
{
	int    ok = 1;
	setDelim(pDelim);
	P_Buf   = 0;
	DataLen = 0;
	Size    = 0;
	if(prealloc) {
		ok = Alloc(prealloc);
	}
	return ok;
}

SLAPI StringSet::StringSet(const char * pDelim/*, size_t prealloc*/)
{
	Init(pDelim, 0/*prealloc*/);
}

SLAPI StringSet::StringSet(char delim, const char * pBuf)
{
	if(delim) {
		char   delim_str[16];
		delim_str[0] = delim;
		delim_str[1] = 0;
		Init(delim_str, 0);
	}
	else
		Init(0, 0);
	if(pBuf)
		setBuf(pBuf, strlen(pBuf)+1);
}

SLAPI StringSet::StringSet(const StringSet & rS)
{
	Init(0, 0);
	copy(rS);
}

SLAPI StringSet::~StringSet()
{
	if(P_Buf) // @speedcritical
		SAlloc::F(P_Buf);
}

StringSet & FASTCALL StringSet::operator = (const StringSet & rS)
{
	copy(rS);
	return *this;
}

int FASTCALL StringSet::copy(const StringSet & rS)
{
	int    ok = 1;
	clear();
	setDelim(rS.Delim);
	if(Alloc(rS.DataLen)) { // @v9.2.0 @fix rS.Size-->rS.DataLen
		memcpy(P_Buf, rS.P_Buf, rS.DataLen);
		DataLen = rS.DataLen;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL StringSet::Write(SBuffer & rBuf) const
{
	SString temp_buf = Delim;
	rBuf.Write(temp_buf);
	rBuf.Write(&DataLen, sizeof(DataLen));
	rBuf.Write(P_Buf, DataLen);
	return 1;
}

int FASTCALL StringSet::Read(SBuffer & rBuf)
{
	int    ok = 1;
	SString delim;
	uint32 data_len = 0;
	rBuf.Read(delim);
	rBuf.Read(&data_len, sizeof(data_len));
	clear();
	if(Init(delim, data_len)) {
		rBuf.Read(P_Buf, data_len);
		DataLen = data_len;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI StringSet::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	//
	// Note: Формат сериализации, применяемый данной функцией должен в точности совпадать
	//   с форматом, применяемым функциями StringSet::Write и StringSet::Read.
	//
	return (dir > 0) ? Write(rBuf) : ((dir < 0) ? Read(rBuf) : -1);
}

#ifndef _WIN32_WCE // {

int SLAPI StringSet::Write(SFile & rFile, long) const
{
	int    ok = 1;
	THROW(rFile.IsValid());
	{
		SBuffer buf;
		SString temp_buf = Delim;
		THROW(buf.Write(temp_buf));
		THROW(buf.Write(&DataLen, sizeof(DataLen)));
		THROW(rFile.Write(buf));
		if(DataLen)
			THROW(rFile.Write(P_Buf, DataLen));
	}
	CATCHZOK
	return ok;
}

int SLAPI StringSet::Read(SFile & rFile, long)
{
	int    ok = 1;
	THROW(rFile.IsValid());
	{
		SBuffer buf;
		SString delim = Delim;
		uint32 data_len = 0;
		THROW(rFile.Read(buf));
		buf.Read(delim);
		buf.Read(&data_len, sizeof(data_len));

		clear();
		THROW(Init(delim, data_len));
		if(data_len)
			THROW(rFile.Read(P_Buf, data_len));
		DataLen = data_len;
	}
	CATCHZOK
	return ok;
}

#endif // } _WIN32_WCE

StringSet & FASTCALL StringSet::operator + (const char * s)
{
	add(s, 0);
	return *this;
}

StringSet & FASTCALL StringSet::operator += (const char * s)
{
	add(s, 0);
	return *this;
}

int FASTCALL StringSet::setBuf(const SString & rBuf)
{
	return setBuf(rBuf, rBuf.Len()+1);
}

int SLAPI StringSet::setBuf(const void * b, size_t len)
{
	int    ok = 1;
	clear();
	if(len) {
		assert(b);
		assert(PTR8(b)[len-1] == 0);
		if(Alloc(len)) {
			memcpy(P_Buf, b, len);
			DataLen = len;
			if(Delim[0]) {
				const char * p = (const char *)memchr(P_Buf, 0, len);
				if(p)
					DataLen = p-P_Buf+1;
			}
			else {
				for(const char * p = P_Buf; (p = (const char *)memchr(p, 0, len)) != 0; p++)
					if(p[1] == 0) {
						DataLen = p-P_Buf+2;
						break;
					}
			}
		}
		else
			ok = 0;
	}
	return ok;
}

void SLAPI StringSet::destroy()
{
	ZFREE(P_Buf);
	Size = 0;
	DataLen = 0;
}

void SLAPI StringSet::clear(/*int dontFreeBuf*/)
{
	DataLen = 0;
}

StringSet & SLAPI StringSet::Z()
{
	DataLen = 0;
	return *this;
}

void SLAPI StringSet::sort()
{
	StrAssocArray temp_list;
	SString str;
	uint   i;
	long   id = 0;
	for(i = 0; get(&i, str);)
		temp_list.Add(++id, str);
	temp_list.SortByText();
	clear();
	for(i = 0; i < temp_list.getCount(); i++)
		add(temp_list.Get(i).Txt);
}

void SLAPI StringSet::sortAndUndup()
{
	StrAssocArray temp_list;
	SString str;
	uint   i;
	long   id = 0;
	for(i = 0; get(&i, str);)
		temp_list.Add(++id, str);
	temp_list.SortByText();
	clear();
	str.Z();
	for(i = 0; i < temp_list.getCount(); i++) {
		const char * p_item = temp_list.Get(i).Txt;
		if(!i || str != p_item) {
			add(p_item);
		}
		str = p_item;
	}
}

int SLAPI StringSet::reverse()
{
	int    ok = 1;
	SString temp_buf;
	StringSet temp_ss;
	temp_ss = *this;
	temp_ss.clear();
	LongArray pos_list;
	uint prev_pos = 0;
	uint pos = 0;
	while((prev_pos = pos), get(&pos, temp_buf)) {
		pos_list.add((long)prev_pos);
	}
	const uint plc = pos_list.getCount();
	if(plc > 1) {
		pos_list.reverse(0, plc);
		for(uint i = 0; i < plc; i++) {
			pos = (uint)pos_list.get(i);
			get(pos, temp_buf);
			temp_ss.add(temp_buf);
		}
		*this = temp_ss;
	}
	return ok;
}

void FASTCALL StringSet::setDelim(const char * pDelim)
{
	Delim[0] = 0;
	if(pDelim) {
		size_t dl = strlen(pDelim);
		assert(dl < sizeof(Delim));
		memcpy(Delim, pDelim, dl+1);
		//STRNSCPY(Delim, pDelim);
	}
}

uint SLAPI StringSet::getDelimLen() const
{
	return Delim[0] ? strlen(Delim) : 1;
}

int FASTCALL StringSet::add(const StringSet & rS)
{
	int    ok = 1;
	SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.5
	for(uint ssp = 0; rS.get(&ssp, r_temp_buf);) {
		THROW(add(r_temp_buf));
	}
	CATCHZOK
	return ok;
}

int FASTCALL StringSet::add(const char * pStr)
{
	return add(pStr, 0);
}

int FASTCALL StringSet::add(const char * str, uint * pPos)
{
	int    ok = 1;
	char   temp_buf[32];
	if(str == 0) {
		temp_buf[0] = 0;
		str = temp_buf;
	}
	const size_t delim_len = DataLen ? (Delim[0] ? strlen(Delim) : 1) : (Delim[0] ? 1 : 2);
	const size_t add_len   = strlen(str);
	const size_t new_len   = DataLen + add_len + delim_len;
	uint   p;
	if(new_len <= Size || Alloc(new_len)) { // @v8.2.9 (new_len <= Size ||) ради ускорения //
		if(DataLen == 0) {
			p = 0;
			memcpy(P_Buf + p, str, add_len+1);
			if(Delim[0] == 0)
				P_Buf[add_len+1] = 0;
		}
		else {
			p = DataLen - 1;
			if(Delim[0]) {
				strcpy(P_Buf + p, Delim);
				p += delim_len;
				memcpy(P_Buf + p, str, add_len+1);
			}
			else {
				memcpy(P_Buf + p, str, add_len+1);
				P_Buf[new_len-1] = 0;
			}
		}
		ASSIGN_PTR(pPos, p);
		DataLen = new_len;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI StringSet::search(const char * pPattern, uint * pPos, int ignoreCase) const
{
	uint   pos = DEREFPTRORZ(pPos);
	SString temp_buf;
	for(uint prev_pos = pos; get(&pos, temp_buf) > 0; prev_pos = pos) {
		if(temp_buf.Cmp(pPattern, ignoreCase) == 0) {
			ASSIGN_PTR(pPos, prev_pos);
			return 1;
		}
	}
	return 0;
}

int SLAPI StringSet::search(const char * pPattern, CompFunc fcmp, uint * pPos, uint * pNextPos) const
{
	int    ok = 0;
	uint   p = DEREFPTRORZ(pPos);
	uint   next_pos = p+1;
	const  uint fix_delim_len = Delim[0] ? strlen(Delim) : 1;
	SString temp_buf;
	while(!ok && p < DataLen) {
		uint  delim_len = fix_delim_len;
		uint  len = 0;
		const char * c = P_Buf + p;
		if(Delim[0]) {
			const char * p_end = (fix_delim_len == 1) ? strchr(c, Delim[0]) : strstr(c, Delim);
			if(p_end)
				len = (uint)(p_end - c);
			else { // Конец буфера данных (в конце разделителя нет)
				delim_len = 1;
				len = strlen(c);
			}
		}
		else if(*c)
			len = strlen(c);
		else
			c = 0;
		if(c) {
			if(Delim[0]) {
				temp_buf.CopyFromN(P_Buf+p, len);
				if(fcmp) {
					if(fcmp(temp_buf.cptr(), pPattern, 0) == 0)
						ok = 1;
				}
				else if(temp_buf.Cmp(pPattern, 0) == 0)
					ok = 1;
			}
			else if(fcmp) {
				if(fcmp(c, pPattern, 0) == 0)
					ok = 1;
			}
			else if(strcmp(c, pPattern) == 0)
				ok = 1;
			if(ok)
				next_pos = (p + len + delim_len);
			else
				p += (len + delim_len);
		}
		else
			break; // Конец буфера данных - больше ничего нет
	}
	ASSIGN_PTR(pPos, p);
	ASSIGN_PTR(pNextPos, next_pos);
	return ok;
}

size_t FASTCALL StringSet::getLen(uint pos) const
{
	uint   len = 0;
	if(pos < DataLen)
		if(Delim[0]) {
			const char * c = strstr(P_Buf + pos, Delim);
			if(c != 0)
				len = (uint)(c - (P_Buf + pos));
			else
				len = strlen(P_Buf + pos);
		}
		else
			len = strlen(P_Buf + pos);
	return len;
}

int FASTCALL StringSet::get(uint * pos, char * str, size_t maxlen) const
{
	int    ok = 1;
	const  char * c = 0;
	uint   p = *pos, len = 0, delim_len = 0;
	if(p < DataLen) {
		if(Delim[0]) {
			if((c = strstr(P_Buf + p, Delim)) != 0) {
				delim_len = strlen(Delim);
				len = (uint)(c - (P_Buf + p));
			}
			else {
				delim_len = 1;
				len = strlen(P_Buf + p);
			}
			c = P_Buf + p;
		}
		else {
			delim_len = 1;
			c = P_Buf + p;
			if(*c)
				len = strlen(c);
			else {
				c = 0;
				ok = 0;
			}
		}
	}
	else
		ok = 0;
	p += (len + delim_len);
	if(str) {
		if(maxlen)
			len = MIN(len, maxlen-1);
		if(c) {
			memcpy(str, c, len);
			str[len] = 0;
		}
		else
			str[0] = 0;
	}
	*pos = p;
	return ok;
}

int FASTCALL StringSet::get(uint * pPos, SString & s) const
{
	int    ok = 1;
	const  char * c = 0;
	uint   p = *pPos, len = 0, delim_len = 0;
	if(p < DataLen) {
		if(Delim[0]) {
			if((c = strstr(P_Buf + p, Delim)) != 0) {
				delim_len = strlen(Delim);
				len = (uint)(c - (P_Buf + p));
			}
			else {
				delim_len = 1;
				len = strlen(P_Buf + p);
			}
			c = P_Buf + p;
		}
		else {
			delim_len = 1;
			c = P_Buf + p;
			if(*c)
				len = strlen(c);
			else {
				c = 0;
				ok = 0;
			}
		}
	}
	else
		ok = 0;
	s.CopyFromN(c, len);
	*pPos = p + len + delim_len;
	return ok;
}

int SLAPI StringSet::get(uint pos, SString & s) const
{
	return get(&pos, s);
}

int SLAPI StringSet::getnz(uint pos, SString & rBuf) const
{
	if(pos)
		return get(&pos, rBuf);
	else {
		rBuf.Z();
		return 1;
	}
}

uint SLAPI StringSet::getCount() const
{
	uint   p = 0, count = 0;
	while(get(&p, 0, 0))
		count++;
	return count;
}

char * SLAPI StringSet::getBuf() const { return P_Buf; }
size_t SLAPI StringSet::getSize() const { return Size; }
//
//
//
SLAPI SStrGroup::SStrGroup()
{
	Pool.add("$"); // zero index - is empty string
}

size_t SLAPI SStrGroup::GetPoolDataLen() const
{
	return Pool.getDataLen();
}

size_t SLAPI SStrGroup::GetPoolSize() const
{
	return Pool.getSize();
}

SStrGroup & FASTCALL SStrGroup::operator = (const SStrGroup & rS)
{
	return CopyS(rS);
}

SStrGroup & FASTCALL SStrGroup::CopyS(const SStrGroup & rS)
{
	Pool.copy(rS.Pool);
	return *this;
}

void SLAPI SStrGroup::ClearS()
{
	Pool.clear();
	Pool.add("$"); // zero index - is empty string
}

void SLAPI SStrGroup::DestroyS()
{
	Pool.destroy();
	Pool.add("$"); // zero index - is empty string
}

int SLAPI SStrGroup::AddS(const char * pStr, uint * pPos)
{
	int    ok = 1;
	uint   pos = 0;
	if(!isempty(pStr)) {
		Pool.add(pStr, &pos);
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI SStrGroup::AddS(const char * pStr, uint32 * pPos)
{
	int    ok = 1;
	uint   pos = 0;
	if(!isempty(pStr)) {
		Pool.add(pStr, &pos);
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI SStrGroup::GetS(uint pos, SString & rStr) const
{
	return Pool.getnz(pos, rStr);
}

void * SLAPI SStrGroup::Pack_Start() const
{
	StringSet * p_handle = new StringSet;
	CALLPTRMEMB(p_handle, add("$", 0));
	return p_handle;
}

int SLAPI SStrGroup::Pack_Finish(void * pHandle)
{
	int    ok = 1;
	StringSet * p_handle = (StringSet *)pHandle;
	if(p_handle) {
		Pool = *p_handle;
		delete p_handle;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI SStrGroup::Pack_Replace(void * pHandle, uint & rPos) const
{
	int    ok = 1;
	StringSet * p_handle = (StringSet *)pHandle;
	if(p_handle) {
		uint   new_pos = rPos;
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.5
		Pool.getnz(rPos, r_temp_buf);
		if(r_temp_buf.NotEmpty()) {
			p_handle->add(r_temp_buf, &new_pos);
		}
		rPos = new_pos;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SStrGroup::WriteS(SBuffer & rBuf) const
{
	return Pool.Write(rBuf);
}

int FASTCALL SStrGroup::ReadS(SBuffer & rBuf)
{
	return Pool.Read(rBuf);
}

int SLAPI SStrGroup::SerializeS(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	return Pool.Serialize(dir, rBuf, pCtx);
}
