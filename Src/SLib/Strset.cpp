// STRSET.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

size_t FASTCALL SnapUpSize(size_t i); // @prototype

int FASTCALL StringSet::Alloc(size_t sz)
{
	int    ok = 1;
	if(sz == 0)
		Z();
	else if(sz > Size) {
		size_t new_size = SnapUpSize(sz);
		char * p = 0;
		if((7 * Size) < (8 * DataLen)) { // Assume probability of a non-moving realloc is 0.125
			// If L is close to Size in size then use realloc to reduce the memory defragmentation
			p = static_cast<char *>(SAlloc::R(P_Buf, new_size));
		}
		else {
			// If L is not close to Size then avoid the penalty of copying
			// the extra bytes that are allocated, but not considered part of the string
			p = static_cast<char *>(SAlloc::M(new_size));
			if(!p)
				p = static_cast<char *>(SAlloc::R(P_Buf, new_size));
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

int StringSet::Init(const char * pDelim, size_t prealloc)
{
	setDelim(pDelim);
	P_Buf   = 0;
	DataLen = 0;
	Size    = 0;
	return prealloc ? Alloc(prealloc) : 1;
}

StringSet::StringSet(const char * pDelim/*, size_t prealloc*/) : P_SaIdx(0)
{
	Init(pDelim, 0/*prealloc*/);
}

StringSet::StringSet(char delim, const char * pBuf) : P_SaIdx(0)
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
		setBuf(pBuf, sstrlen(pBuf)+1);
}

StringSet::StringSet(const StringSet & rS) : P_SaIdx(0)
{
	Init(0, 0);
	copy(rS);
}

StringSet::~StringSet()
{
	if(P_Buf) // @speedcritical
		SAlloc::F(P_Buf);
	ZDELETE(P_SaIdx); // @v11.8.3
}

bool FASTCALL StringSet::IsEqPermutation(const StringSet & rS) const
{
	bool   eq = true;   
	if(sstreq(Delim, rS.Delim)) {
		const uint _c1 = getCount();
		const uint _c2 = rS.getCount();
		if(_c1 == _c2) {
			SString & r_temp_buf1 = SLS.AcquireRvlStr();
			SString & r_temp_buf2 = SLS.AcquireRvlStr();
			uint   p1 = 0;
			uint   p2 = 0;
			uint   preserve_p1 = p1;
			uint   preserve_p2 = p2;
			int    r1 = 0;
			int    r2 = 0;
			while(eq) {
				preserve_p1 = p1;
				r1 = get(&p1, r_temp_buf1);
				preserve_p2 = p2;
				r2 = rS.get(&p2, r_temp_buf2);
				assert(r1 == r2);
				if(r1 != 0 && r2 != 0) {
					if(r_temp_buf1 != r_temp_buf2) {
						eq = false;
						//
						// Мы дошли до пары элементов в this и rS, которые не равны.
						// Теперь мы, начиная с позиции preserve_p1 в this и preserve_p2 в rS,
						// перебирая каждый элемент, находим соответствие в сравниваемом контейнере.
						// Если для какого-то элемента соответствие не найдено, то общий результат false
						//
						{
							eq = true;
							{
								for(uint ssp1 = preserve_p1; eq && get(&ssp1, r_temp_buf1);) {
									bool found = false;
									for(uint ssp2 = preserve_p2; !found && rS.get(&ssp2, r_temp_buf2);) {
										if(r_temp_buf1 == r_temp_buf2)
											found = true;
									}
									if(!found)
										eq = false;
								}
							}
							if(eq) {
								for(uint ssp2 = preserve_p2; eq && rS.get(&ssp2, r_temp_buf2);) {
									bool found = false;
									for(uint ssp1 = preserve_p1; !found && get(&ssp1, r_temp_buf1);) {
										if(r_temp_buf1 == r_temp_buf2)
											found = true;
									}
									if(!found)
										eq = false;
								}
							}
						}
						break; // !exit-loop while(eq)
					}
				}
				else
					break;
			}
		}
		else
			eq = false;
	}
	else
		eq = false;
	return eq;
}

bool FASTCALL StringSet::IsEq(const StringSet & rS) const
{
	bool   eq = true;   
	if(sstreq(Delim, rS.Delim)) {
		const uint _c1 = getCount();
		const uint _c2 = rS.getCount();
		if(_c1 == _c2) {
			SString & r_temp_buf1 = SLS.AcquireRvlStr();
			SString & r_temp_buf2 = SLS.AcquireRvlStr();
			uint   p1 = 0;
			uint   p2 = 0;
			int    r1 = 0;
			int    r2 = 0;
			while(eq) {
				r1 = get(&p1, r_temp_buf1);
				r2 = rS.get(&p2, r_temp_buf2);
				assert(r1 == r2);
				if(r1 != 0 && r2 != 0) {
					if(r_temp_buf1 != r_temp_buf2)
						eq = false;
				}
				else
					break;
			}
			assert(LOGIC(r1) == LOGIC(r2)); // Мы выше сравнили количество элементов. Если результаты получения очередной
				// подстроки отличаются для разных экземпляров, то значит у нас тяжелая ошибка в коде.
		}
		else
			eq = false;
	}
	else
		eq = false;
	return eq;
}

StringSet & FASTCALL StringSet::operator = (const StringSet & rS)
{
	copy(rS);
	return *this;
}

int FASTCALL StringSet::copy(const StringSet & rS)
{
	int    ok = 1;
	Z();
	setDelim(rS.Delim);
	if(Alloc(rS.DataLen)) {
		memcpy(P_Buf, rS.P_Buf, rS.DataLen);
		DataLen = rS.DataLen;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL StringSet::Write(SBuffer & rBuf) const
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	r_temp_buf = Delim;
	const  uint32 data_len = DataLen; // sizeof(data_len) == 4
	rBuf.Write(r_temp_buf);
	rBuf.Write(&data_len, sizeof(data_len));
	rBuf.Write(P_Buf, data_len);
	return 1;
}

int FASTCALL StringSet::Read(SBuffer & rBuf)
{
	int    ok = 1;
	SString & r_delim = SLS.AcquireRvlStr();
	uint32 data_len = 0;
	rBuf.Read(r_delim);
	rBuf.Read(&data_len, sizeof(data_len));
	Z();
	if(Init(r_delim, data_len)) {
		rBuf.Read(P_Buf, data_len);
		DataLen = data_len;
	}
	else
		ok = 0;
	return ok;
}

int StringSet::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	//
	// Note: Формат сериализации, применяемый данной функцией должен в точности совпадать
	//   с форматом, применяемым функциями StringSet::Write и StringSet::Read.
	//
	return (dir > 0) ? Write(rBuf) : ((dir < 0) ? Read(rBuf) : -1);
}

int StringSet::Write(SFile & rFile, long) const
{
	int    ok = 1;
	THROW(rFile.IsValid());
	{
		SBuffer buf;
		const  SString temp_buf(Delim);
		const  uint32 data_len = DataLen; // sizeof(data_len) == 4
		THROW(buf.Write(temp_buf));
		THROW(buf.Write(&data_len, sizeof(data_len)));
		THROW(rFile.Write(buf));
		if(DataLen)
			THROW(rFile.Write(P_Buf, data_len));
	}
	CATCHZOK
	return ok;
}

int StringSet::Read(SFile & rFile, long)
{
	int    ok = 1;
	THROW(rFile.IsValid());
	{
		SBuffer buf;
		SString delim(Delim);
		uint32 data_len = 0;
		THROW(rFile.Read(buf));
		buf.Read(delim);
		buf.Read(&data_len, sizeof(data_len));
		Z();
		THROW(Init(delim, data_len));
		if(data_len)
			THROW(rFile.Read(P_Buf, data_len));
		DataLen = data_len;
	}
	CATCHZOK
	return ok;
}

StringSet & FASTCALL StringSet::operator + (const char * s)
{
	add(s);
	return *this;
}

StringSet & FASTCALL StringSet::operator += (const char * s)
{
	add(s);
	return *this;
}

int FASTCALL StringSet::setBuf(const SString & rBuf)
{
	return setBuf(rBuf, rBuf.Len()+1);
}

int StringSet::setBuf(const void * b, size_t len)
{
	int    ok = 1;
	Z();
	if(len) {
		assert(b);
		assert(PTR8C(b)[len-1] == 0);
		if(Alloc(len)) {
			memcpy(P_Buf, b, len);
			DataLen = len;
			if(Delim[0]) {
				const char * p = static_cast<const char *>(smemchr(P_Buf, 0, len)); // @v11.7.0 memchr-->smemchr
				if(p)
					DataLen = p-P_Buf+1;
			}
			else {
				for(const char * p = P_Buf; (p = static_cast<const char *>(smemchr(p, 0, len))) != 0; p++) // @v11.7.0 memchr-->smemchr
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

void StringSet::destroy()
{
	ZFREE(P_Buf);
	ZDELETE(P_SaIdx); // @v11.8.3
	Size = 0;
	DataLen = 0;
}

/* @v11.8.3 
void StringSet::clear()
{
	DataLen = 0;
	ZDELETE(P_SaIdx); // @v11.8.3
}*/

StringSet & StringSet::Z()
{
	DataLen = 0;
	ZDELETE(P_SaIdx); // @v11.8.3
	return *this;
}

void StringSet::sort()
{
	// @v12.3.5 const uint org_count = getCount(); // @v11.4.8 see considerations at StringSet::sortAndUndup()
	// @v12.3.5 if(org_count > 1) { // @v11.4.8 
	if(IsCountGreaterThan(1)) { // @v12.3.5
		StrAssocArray temp_list;
		SString str;
		uint   i;
		long   id = 0;
		for(i = 0; get(&i, str);)
			temp_list.AddFast(++id, str); // @v11.2.4 Add-->AddFast
		temp_list.SortByText();
		Z();
		for(i = 0; i < temp_list.getCount(); i++)
			add(temp_list.Get(i).Txt);
	}
	// @v12.3.5 assert(getCount() == org_count); // @v11.4.8 дорогая проверка (из-за getCount()), но в релизе ее не будет 
}

void StringSet::shuffle()
{
	// @v12.3.5 const uint org_count = getCount(); // @v11.4.8 see considerations at StringSet::sortAndUndup()
	// @v12.3.5 if(org_count > 1) {
	if(IsCountGreaterThan(1)) { // @v12.3.5
		StrAssocArray temp_list;
		SString str;
		uint   i;
		long   id = 0;
		for(i = 0; get(&i, str);)
			temp_list.AddFast(++id, str);
		temp_list.Shuffle();
		Z();
		for(i = 0; i < temp_list.getCount(); i++)
			add(temp_list.Get(i).Txt);
	}
	// @v12.3.5 assert(getCount() == org_count); // дорогая проверка (из-за getCount()), но в релизе ее не будет 
}

void StringSet::sortAndUndup()
{
	// @v11.4.8 Соображения о производительности: 
	// Эта функция достаточно дорогая - она преобразует сет в строковый массив StrAssocArray, затем сортирует этот массив
	// и результат сбрасывает в оригинальный сет.
	// Для того чтобы не делать холостых действий сначала проверяем количество элементов в сете: если их 0 или 1 то просто
	// ничего не делаем. 
	// @v12.3.5 дальнейший текст более на актуален {
	//   Увы, функция getCount() тоже дорогая - она пробегает все элементы сета, то есть, если
	//   количество элементов значительное то мы общую производительность ухудшаем. Но в качестве утешения замечу, что 
	//   подсчитав предварительно количество элеметов мы заполнили кэш процессора данными из сета и последующий перебор 
	//   в цикле #ref01 будет значительно быстрее.
	// }
	//
	// @v12.3.5 const uint org_count = getCount(); // @v11.4.8 
	// @v12.3.5 if(org_count > 1) { // @v11.4.8
	if(IsCountGreaterThan(1)) { // @v12.3.5
		StrAssocArray temp_list;
		SString str;
		uint   i;
		long   id = 0;
		for(i = 0; get(&i, str);) { // #ref01
			temp_list.Add(++id, str);
		}
		temp_list.SortByText();
		Z();
		str.Z();
		for(i = 0; i < temp_list.getCount(); i++) {
			const char * p_item = temp_list.Get(i).Txt;
			if(!i || str != p_item) {
				add(p_item);
			}
			str = p_item;
		}
	}
}

int StringSet::reverse()
{
	int    ok = 1;
	SString temp_buf;
	StringSet temp_ss = *this;
	temp_ss.Z();
	LongArray pos_list;
	uint prev_pos = 0;
	uint pos = 0;
	while((prev_pos = pos), get(&pos, temp_buf)) {
		THROW(pos_list.add(static_cast<long>(prev_pos)));
	}
	{
		const uint plc = pos_list.getCount();
		if(plc > 1) {
			pos_list.reverse(0, plc);
			for(uint i = 0; i < plc; i++) {
				get(static_cast<uint>(pos_list.get(i)), temp_buf);
				THROW(temp_ss.add(temp_buf));
			}
			*this = temp_ss;
		}
	}
	CATCHZOK
	return ok;
}

void FASTCALL StringSet::setDelim(const char * pDelim)
{
	Delim[0] = 0;
	if(pDelim) {
		const size_t dl = sstrlen(pDelim);
		assert(dl < sizeof(Delim));
		memcpy(Delim, pDelim, dl+1);
		//STRNSCPY(Delim, pDelim);
	}
}

uint StringSet::getDelimLen() const { return Delim[0] ? sstrlen(Delim) : 1; }
bool StringSet::isZeroDelim() const { return Delim[0] == 0; }

int FASTCALL StringSet::add(const StringSet & rS)
{
	int    ok = 1;
	SString & r_temp_buf = SLS.AcquireRvlStr();
	for(uint ssp = 0; rS.get(&ssp, r_temp_buf);) {
		THROW(add(r_temp_buf));
	}
	CATCHZOK
	return ok;
}

int FASTCALL StringSet::add(const char * pStr)
{
	return add(pStr, static_cast<uint *>(0));
}

int StringSet::add(const char * pStr, uint * pPos)
{
	int    ok = 1;
	char   temp_buf[32];
	if(!pStr) {
		temp_buf[0] = 0;
		pStr = temp_buf;
	}
	const size_t delim_len = DataLen ? (Delim[0] ? sstrlen(Delim) : 1) : (Delim[0] ? 1 : 2);
	const size_t add_len   = sstrlen(pStr);
	const size_t new_len   = DataLen + add_len + delim_len;
	uint   p;
	if(new_len <= Size || Alloc(new_len)) {
		if(DataLen == 0) {
			p = 0;
			memcpy(P_Buf + p, pStr, add_len+1);
			if(Delim[0] == 0)
				P_Buf[add_len+1] = 0;
		}
		else {
			p = DataLen - 1;
			if(Delim[0]) {
				strcpy(P_Buf + p, Delim);
				p += delim_len;
				memcpy(P_Buf + p, pStr, add_len+1);
			}
			else {
				memcpy(P_Buf + p, pStr, add_len+1);
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

bool StringSet::search(const char * pPattern, uint * pPos, int ignoreCase) const
{
	uint   pos = DEREFPTRORZ(pPos);
	SString & r_temp_buf = SLS.AcquireRvlStr();
	for(uint prev_pos = pos; get(&pos, r_temp_buf); prev_pos = pos) {
		if(r_temp_buf.Cmp(pPattern, ignoreCase) == 0) {
			ASSIGN_PTR(pPos, prev_pos);
			return true;
		}
	}
	return false;
}

bool StringSet::searchNcAscii(const char * pPattern, uint * pPos, uint * pNextPos) const
{
	uint   pos = DEREFPTRORZ(pPos);
	SString temp_buf;
	for(uint prev_pos = pos; get(&pos, temp_buf); prev_pos = pos) {
		if(temp_buf.IsEqiAscii(pPattern)) {
			ASSIGN_PTR(pPos, prev_pos);
			ASSIGN_PTR(pNextPos, pos);
			return true;
		}
	}
	ASSIGN_PTR(pPos, 0);
	ASSIGN_PTR(pNextPos, pos);
	return false;
}

bool StringSet::searchNcUtf8(const char * pPattern, uint * pPos, uint * pNextPos) const
{
	uint   pos = DEREFPTRORZ(pPos);
	SString temp_buf;
	for(uint prev_pos = pos; get(&pos, temp_buf); prev_pos = pos) {
		if(temp_buf.IsEqiUtf8(pPattern)) {
			ASSIGN_PTR(pPos, prev_pos);
			ASSIGN_PTR(pNextPos, pos);
			return true;
		}
	}
	ASSIGN_PTR(pPos, 0);
	ASSIGN_PTR(pNextPos, pos);
	return false;
}

bool StringSet::search(const char * pPattern, CompFunc fcmp, uint * pPos, uint * pNextPos) const
{
	bool   ok = false;
	uint   p = DEREFPTRORZ(pPos);
	uint   next_pos = p+1;
	const  uint fix_delim_len = Delim[0] ? sstrlen(Delim) : 1;
	SString temp_buf;
	while(!ok && p < DataLen) {
		uint  delim_len = fix_delim_len;
		uint  len = 0;
		const char * c = P_Buf + p;
		if(Delim[0]) {
			const char * p_end = (fix_delim_len == 1) ? sstrchr(c, Delim[0]) : strstr(c, Delim);
			if(p_end)
				len = (uint)(p_end - c);
			else { // Конец буфера данных (в конце разделителя нет)
				delim_len = 1;
				len = sstrlen(c);
			}
		}
		else if(*c)
			len = sstrlen(c);
		else
			c = 0;
		if(c) {
			if(Delim[0]) {
				temp_buf.CopyFromN(P_Buf+p, len);
				if(fcmp) {
					if(fcmp(temp_buf.cptr(), pPattern, 0) == 0)
						ok = true;
				}
				else if(temp_buf.Cmp(pPattern, 0) == 0)
					ok = true;
			}
			else if(fcmp) {
				if(fcmp(c, pPattern, 0) == 0)
					ok = true;
			}
			else if(sstreq(c, pPattern))
				ok = true;
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
	if(pos < DataLen) {
		if(Delim[0]) {
			const char * c = strstr(P_Buf + pos, Delim);
			if(c != 0)
				len = static_cast<uint>(c - (P_Buf + pos));
			else
				len = sstrlen(P_Buf + pos);
		}
		else
			len = sstrlen(P_Buf + pos);
	}
	return len;
}

bool StringSet::get(uint * pPos, char * pStr, size_t maxlen) const
{
	bool   ok = true;
	const  char * c = 0;
	uint   p = *pPos;
	uint   len = 0;
	uint   delim_len = 0;
	if(p < DataLen) {
		if(Delim[0]) {
			c = strstr(P_Buf + p, Delim);
			if(c) {
				delim_len = sstrlen(Delim);
				len = static_cast<uint>(c - (P_Buf + p));
			}
			else {
				delim_len = 1;
				len = sstrlen(P_Buf + p);
			}
			c = P_Buf + p;
		}
		else {
			delim_len = 1;
			c = P_Buf + p;
			if(*c)
				len = sstrlen(c);
			else {
				c = 0;
				ok = false;
			}
		}
	}
	else
		ok = false;
	p += (len + delim_len);
	if(pStr) {
		if(maxlen)
			len = MIN(len, maxlen-1);
		if(c) {
			memcpy(pStr, c, len);
			pStr[len] = 0;
		}
		else
			pStr[0] = 0;
	}
	*pPos = p;
	return ok;
}

bool StringSet::get(uint * pPos, SString & s) const
{
	bool   ok = true;
	const  char * c = 0;
	uint   p = *pPos;
	uint   len = 0;
	uint   delim_len = 0;
	if(p < DataLen) {
		assert(P_Buf);
		const size_t _dlen = strlen(Delim); // strlen чуть быстрее чем sstrlen (точно известно что аргумент не нулевой)
		if(_dlen) {
			c = (_dlen == 1) ? sstrchr(P_Buf + p, Delim[0]) : strstr(P_Buf + p, Delim);
			if(c) {
				delim_len = _dlen;
				len = static_cast<uint>(c - (P_Buf + p));
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
				ok = false;
			}
		}
	}
	else
		ok = false;
	s.CopyFromN(c, len);
	*pPos = p + len + delim_len;
	return ok;
}

bool StringSet::getByIdx(uint idx, SString & rBuf) const
{
	bool   ok = false;
	for(uint i = 0, sp = 0; !ok && get(&sp, rBuf); i++) {
		if(i == idx)
			ok = true;
	}
	if(!ok)
		rBuf.Z();
	return ok;
}

bool StringSet::get(uint pos, SString & s) const
{
	return get(&pos, s);
}

bool StringSet::getnz(uint pos, SString & rBuf) const
{
	if(pos)
		return get(&pos, rBuf);
	else {
		rBuf.Z();
		return false; // @v11.2.4 @fix true-->false (я не уверен в последствиях, но по логике, здесь должно быть false)
	}
}

uint StringSet::getCount() const
{
	uint   count = 0;
	uint   p = 0;
	while(get(&p, 0, 0))
		count++;
	return count;
}

bool FASTCALL StringSet::IsCountGreaterThan(uint t) const
{
	bool   result = false;
	uint   count = 0;
	uint   p = 0;
	while(!result && get(&p, 0, 0)) {
		count++;
		if(count > t)
			result = true;
	}
	return result;
}

const char * StringSet::getBuf() const { return P_Buf; }
size_t StringSet::getSize() const { return Size; }

int StringSet::BuildSa()
{
	int   ok = 1;
	const size_t sz = getDataLen();
	if(sz) {
		THROW(SETIFZ(P_SaIdx, new SaIndex()));
		THROW(P_SaIdx->SetTextOuter(P_Buf, sz));
		THROW(P_SaIdx->Build());
	}
	else
		ok = -1;
	CATCH
		ZDELETE(P_SaIdx);
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
SStrGroup::SStrGroup()
{
	Pool.add("$"); // zero index - is empty string
}

SStrGroup::SStrGroup(const SStrGroup & rS) : Pool(rS.Pool) // @v10.3.4
{
}

size_t SStrGroup::GetPoolDataLen() const { return Pool.getDataLen(); }
size_t SStrGroup::GetPoolSize() const { return Pool.getSize(); }
SStrGroup & FASTCALL SStrGroup::operator = (const SStrGroup & rS) { return CopyS(rS); }
bool   SStrGroup::GetS(uint pos, SString & rStr) const { return Pool.getnz(pos, rStr); }
int    FASTCALL SStrGroup::WriteS(SBuffer & rBuf) const { return Pool.Write(rBuf); }
int    FASTCALL SStrGroup::ReadS(SBuffer & rBuf) { return Pool.Read(rBuf); }
int    SStrGroup::SerializeS(int dir, SBuffer & rBuf, SSerializeContext * pCtx) { return Pool.Serialize(dir, rBuf, pCtx); }

SStrGroup & FASTCALL SStrGroup::CopyS(const SStrGroup & rS)
{
	Pool.copy(rS.Pool);
	return *this;
}

void SStrGroup::ClearS()
{
	Pool.Z().add("$"); // zero index - is empty string
}

void SStrGroup::DestroyS()
{
	Pool.destroy();
	Pool.add("$"); // zero index - is empty string
}

int SStrGroup::AddS(const char * pStr, uint * pPos)
{
	uint   pos = 0;
	int    ok = isempty(pStr) ? 1 : Pool.add(pStr, &pos);
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SStrGroup::AddS(const char * pStr, ulong * pPos)
{
	uint   pos = 0;
	int    ok = isempty(pStr) ? 1 : Pool.add(pStr, &pos);
	ASSIGN_PTR(pPos, pos);
	return ok;
}

void * SStrGroup::Pack_Start() const
{
	StringSet * p_handle = new StringSet;
	CALLPTRMEMB(p_handle, add("$"));
	return p_handle;
}

int SStrGroup::Pack_Finish(void * pHandle)
{
	int    ok = 1;
	StringSet * p_handle = static_cast<StringSet *>(pHandle);
	if(p_handle) {
		Pool = *p_handle;
		delete p_handle;
	}
	else
		ok = 0;
	return ok;
}

int SStrGroup::Pack_Replace(void * pHandle, uint & rPos) const
{
	int    ok = 1;
	uint   new_pos = 0; // @v10.5.7 =rPos --> =0
	StringSet * p_handle = static_cast<StringSet *>(pHandle);
	if(p_handle) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		Pool.getnz(rPos, r_temp_buf);
		if(r_temp_buf.NotEmpty())
			p_handle->add(r_temp_buf, &new_pos);
	}
	else
		ok = 0;
	rPos = new_pos;
	return ok;
}
