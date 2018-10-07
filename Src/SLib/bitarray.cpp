// BITARRAY.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2004, 2006, 2007, 2008, 2010, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

SLAPI BitArray::BitArray() : Count(0)
{
	Size = 64;
	P_Buf = (char *)SAlloc::M(Size);
	resetbitstring(P_Buf, Size);
}

SLAPI BitArray::BitArray(const BitArray & s) : Count(0)
{
	SBaseBuffer::Init();
	Copy(s);
}

SLAPI BitArray::~BitArray()
{
	SBaseBuffer::Destroy();
}

BitArray & FASTCALL BitArray::operator = (const BitArray & s)
{
	Copy(s);
	return *this;
}

int FASTCALL BitArray::Copy(const BitArray & s)
{
	size_t new_size = s.Size;
	P_Buf = (char *)SAlloc::R(P_Buf, new_size);
	if(P_Buf) {
		Size = new_size;
		memcpy(P_Buf, s.P_Buf, Size);
		Count = s.Count;
		return 1;
	}
	else
		return 0;
}

int SLAPI BitArray::Init(const void * pBits, size_t count)
{
	int    ok = -1;
	if(count && pBits) {
		size_t new_size = ((count + 31) / 32) * 4;
		if(Size < new_size) {
			P_Buf = (char *)SAlloc::R(P_Buf, new_size);
			Size = new_size;
			resetbitstring(P_Buf, Size);
		}
		Count = count;
		memcpy(P_Buf, pBits, new_size);
		ok = 1;
	}
	return ok;
}

void SLAPI BitArray::Clear()
{
	Zero();
	Count = 0;
}

size_t SLAPI BitArray::getCount() const
{
	return Count;
}

int FASTCALL BitArray::IsEqual(const BitArray & rS) const
{
	int    eq = 0;
	const  size_t c = getCount();
	if(c == rS.getCount()) {
		eq = 1;
		for(uint i = 0; eq && i < c; i++) {
			if(getbit32(P_Buf, Size, i) != getbit32(rS.P_Buf, rS.Size, i))
				eq = 0;
		}
	}
	return eq;
}

size_t FASTCALL BitArray::getCountVal(int val) const
{
	size_t i;
	size_t c = (Count / 32);
	uint   r = 0;
	for(i = 0; i < c; i++) {
		//
		// Трюк с подсчетом ненулевых битов посредством обнуления последнего единичного бита X & (X-1) {
		//
		/* @v7.8.3
		uint   w = PTR32(P_Buf)[i];
		uint   popc = 0;
		for(; w != 0; w &= (w-1))
			popc++;
		*/
		//
		// @v7.8.3
		// Вариант с использованием функции из ASMLIB. Корректность протестирована, но
		// остаются вопросы насчет зависаний системы при использовании каких-то функций из
		// этой библиотеки.
		// uint   popc = A_popcount(PTR32(P_Buf)[i]);
		//
		uint   popc = popcount32(PTR32(P_Buf)[i]); // @v7.8.3 @proof
		// }
		if(val)
			r += popc;
		else
			r += (32-popc);
	}
	{
		i = (c * 32);
		c = Count;
		for(; i < c; i++) {
			if(get(i)) {
				if(val)
					r++;
			}
			else {
				if(!val)
					r++;
			}
		}
	}
	return r;
}

size_t FASTCALL BitArray::findFirst(int val, size_t start) const
{
	size_t p = start;
	while(p < Count) {
		int    v = getbit32(P_Buf, Size, p++);
		if((v && val) || (!v && !val))
			return p;
	}
	return 0;
}

int FASTCALL BitArray::get(size_t pos) const
{
	return (pos < Count) ? getbit32(P_Buf, Size, pos) : -1;
}

uint32 FASTCALL BitArray::getN(size_t pos, uint count) const
{
	return getbits(P_Buf, Size, pos, count);
}

int FASTCALL BitArray::operator [] (size_t pos) const
{
	return get(pos);
}

int FASTCALL BitArray::set(size_t pos, int val)
{
	if(pos < Count) {
		if(val)
			setbit32(P_Buf, Size, pos);
		else
			resetbit32(P_Buf, Size, pos);
		return 1;
	}
	else
		return 0;
}

int FASTCALL BitArray::atInsert(size_t pos, int val)
{
	if(pos <= Count) {
		size_t new_size = ((Count + 32) / 32) * 4;
		if(new_size > Size) {
			P_Buf = (char *)SAlloc::R(P_Buf, new_size);
			Size = new_size;
		}
		if(pos < Count)
			insbit(P_Buf, Size, pos);
		Count++;
		set(pos, val);
		return 1;
	}
	else
		return 0;
}

int FASTCALL BitArray::insertN(int val, size_t N)
{
	if(N == 0)
		return 0;
	else {
		size_t s = (((Count + N - 1) + 32) / 32) * 4;
		if(s > Size) {
			P_Buf = (char *)SAlloc::R(P_Buf, s);
			Size = s;
		}
		s = Count; // prev value of BitArray::Count
		Count += N;
		if(val) {
			for(size_t i = s; i < Count; i++)
				setbit32(P_Buf, Size, i);
		}
		else {
			for(size_t i = s; i < Count; i++)
				resetbit32(P_Buf, Size, i);
		}
		return 1;
	}
}

int FASTCALL BitArray::insert(int val)
{
	return atInsert(Count, val);
}

int FASTCALL BitArray::atFree(size_t pos)
{
	if(pos < Count) {
		delbit(P_Buf, Size, pos);
		Count--;
		return 1;
	}
	else
		return 0;
}

size_t SLAPI BitArray::getBufSize() const
{
	return ((Count + 31) / 32) * 4;
}

int SLAPI BitArray::getBuf(void * pBits, size_t maxLen) const
{
	int    ok = -1;
	if(pBits && maxLen) {
		size_t  buflen = getBufSize();
		if(maxLen > buflen) {
			memcpy(pBits, P_Buf, buflen);
			memzero(((char *)pBits) + buflen, maxLen - buflen);
		}
		else
			memcpy(pBits, P_Buf, maxLen);
		ok = 1;
	}
	return ok;
}

#if SLTEST_RUNNING // {

SLTEST_R(BitArray)
{
	int    ok = 1;
	uint   total = 1001;
	uint   count_set = 0;
	uint   count_reset = 0;
	uint   i;
	BitArray list;
	SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
	p_rng->Set(177);
	//
	// Pattern используется для контроля совпадения битов с тем, что должно быть
	// Предполагается, что LongArray работает правильно
	//
	LongArray pattern;
	//
	// Тестирование добавления битов в конец списка
	//
	for(i = 0; i < total; i++) {
		int s = p_rng->Get() % 2;
		list.insert(s);
		pattern.add(s);
		if(s) {
			count_set++;
			THROW(SLTEST_CHECK_NZ(list.get(i)));
		}
		else {
			count_reset++;
			THROW(SLTEST_CHECK_Z(list.get(i)));
		}
	}
	THROW(SLTEST_CHECK_EQ(total, list.getCount()));
	THROW(SLTEST_CHECK_EQ(total, pattern.getCount()));
	THROW(SLTEST_CHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLTEST_CHECK_EQ(list[i], pattern.get(i)));
	}
	THROW(SLTEST_CHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLTEST_CHECK_EQ(count_reset, list.getCountVal(0)));
	//
	// Тестирование добавления битов в случайном порядке
	//
	for(i = 0; i < total; i++) {
		int    s = p_rng->Get() % 2;
		uint   pos = p_rng->Get() % list.getCount();
		THROW(SLTEST_CHECK_NZ(list.atInsert(pos, s)));
		THROW(SLTEST_CHECK_NZ(pattern.atInsert(pos, (void *)&s)));
		if(s) {
			count_set++;
			THROW(SLTEST_CHECK_NZ(list.get(pos)));
		}
		else {
			count_reset++;
			THROW(SLTEST_CHECK_Z(list.get(pos)));
		}
	}
	total *= 2;
	THROW(SLTEST_CHECK_EQ(total, list.getCount()));
	THROW(SLTEST_CHECK_EQ(total, pattern.getCount()));
	THROW(SLTEST_CHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLTEST_CHECK_EQ(list[i], pattern.get(i)));
	}
	THROW(SLTEST_CHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLTEST_CHECK_EQ(count_reset, list.getCountVal(0)));
	//
	// Тестирование удаления битов в случайном порядке
	//
	for(i = 0; i < total; i++) {
		uint   pos = p_rng->Get() % list.getCount();
		int    s = list.get(pos);
		THROW(SLTEST_CHECK_EQ(s, pattern.get(pos)));
		THROW(SLTEST_CHECK_NZ(list.atFree(pos)));
		THROW(SLTEST_CHECK_NZ(pattern.atFree(pos)));
		if(s) {
			count_set--;
		}
		else {
			count_reset--;
		}
	}
	total = 0;
	THROW(SLTEST_CHECK_EQ(total, list.getCount()));
	THROW(SLTEST_CHECK_EQ(total, pattern.getCount()));
	THROW(SLTEST_CHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLTEST_CHECK_EQ(list[i], pattern.get(i)));
	}
	THROW(SLTEST_CHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLTEST_CHECK_EQ(count_reset, list.getCountVal(0)));
	//
	// Тестирование массированного добавления битов в конец списка (порциями случайного размера)
	//
	uint   cc = 0;
	for(i = 0; i < 103; i++) {
		int    s = p_rng->Get() % 2;
		uint   n = p_rng->Get() % 64;
		uint   c = list.getCount();
		list.insertN(s, n);
		for(uint j = 0; j < n; j++) {
			pattern.add(s);
			if(s) {
				count_set++;
				THROW(SLTEST_CHECK_NZ(list.get(c+j)));
			}
			else {
				count_reset++;
				THROW(SLTEST_CHECK_Z(list.get(c+j)));
			}
		}
		cc += n;
	}
	total = cc;
	THROW(SLTEST_CHECK_EQ(total, list.getCount()));
	THROW(SLTEST_CHECK_EQ(total, pattern.getCount()));
	THROW(SLTEST_CHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLTEST_CHECK_EQ(list[i], pattern.get(i)));
	}
	THROW(SLTEST_CHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLTEST_CHECK_EQ(count_reset, list.getCountVal(0)));
	//
	// Тест функций bitscanforward и bitscanreverse
	//
	{
		uint32 idx;
		SLTEST_CHECK_Z(bitscanforward(&idx, 0));
		SLTEST_CHECK_EQ(idx, 0);
		SLTEST_CHECK_NZ(bitscanforward(&idx, 0x01));
		SLTEST_CHECK_EQ(idx, 0);
		SLTEST_CHECK_NZ(bitscanforward(&idx, 0x10));
		SLTEST_CHECK_EQ(idx, 4);
		SLTEST_CHECK_NZ(bitscanforward(&idx, 0x80000800));
		SLTEST_CHECK_EQ(idx, 11);

		SLTEST_CHECK_Z(bitscanreverse(&idx, 0));
		SLTEST_CHECK_EQ(idx, 0);
		SLTEST_CHECK_NZ(bitscanreverse(&idx, 0x01));
		SLTEST_CHECK_EQ(idx, 0);
		SLTEST_CHECK_NZ(bitscanreverse(&idx, 0x10));
		SLTEST_CHECK_EQ(idx, 4);
		SLTEST_CHECK_NZ(bitscanreverse(&idx, 0x80000800));
		SLTEST_CHECK_EQ(idx, 31);
	}
	CATCHZOK
	delete p_rng;
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
