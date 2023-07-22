// TEST-BITS.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование битовых операций
//
#include <pp.h>
#pragma hdrstop

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
			THROW(SLCHECK_NZ(list.get(i)));
		}
		else {
			count_reset++;
			THROW(SLCHECK_Z(list.get(i)));
		}
	}
	THROW(SLCHECK_EQ(total, list.getCount()));
	THROW(SLCHECK_EQ(total, pattern.getCount()));
	THROW(SLCHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLCHECK_EQ(list[i], (int)pattern.get(i)));
	}
	THROW(SLCHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLCHECK_EQ(count_reset, list.getCountVal(0)));
	//
	// Тестирование добавления битов в случайном порядке
	//
	for(i = 0; i < total; i++) {
		int    s = p_rng->Get() % 2;
		uint   pos = p_rng->Get() % list.getCount();
		THROW(SLCHECK_NZ(list.atInsert(pos, s)));
		THROW(SLCHECK_NZ(pattern.atInsert(pos, (void *)&s)));
		if(s) {
			count_set++;
			THROW(SLCHECK_NZ(list.get(pos)));
		}
		else {
			count_reset++;
			THROW(SLCHECK_Z(list.get(pos)));
		}
	}
	total *= 2;
	THROW(SLCHECK_EQ(total, list.getCount()));
	THROW(SLCHECK_EQ(total, pattern.getCount()));
	THROW(SLCHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLCHECK_EQ(list[i], (int)pattern.get(i)));
	}
	THROW(SLCHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLCHECK_EQ(count_reset, list.getCountVal(0)));
	//
	// Тестирование удаления битов в случайном порядке
	//
	for(i = 0; i < total; i++) {
		uint   pos = p_rng->Get() % list.getCount();
		int    s = list.get(pos);
		THROW(SLCHECK_EQ(s, (int)pattern.get(pos)));
		THROW(SLCHECK_NZ(list.atFree(pos)));
		THROW(SLCHECK_NZ(pattern.atFree(pos)));
		if(s) {
			count_set--;
		}
		else {
			count_reset--;
		}
	}
	total = 0;
	THROW(SLCHECK_EQ(total, list.getCount()));
	THROW(SLCHECK_EQ(total, pattern.getCount()));
	THROW(SLCHECK_EQ(total, count_set + count_reset));
	for(i = 0; i < total; i++) {
		THROW(SLCHECK_EQ(list[i], (int)pattern.get(i)));
	}
	THROW(SLCHECK_EQ(count_set,   list.getCountVal(1)));
	THROW(SLCHECK_EQ(count_reset, list.getCountVal(0)));
	{
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
					THROW(SLCHECK_NZ(list.get(c+j)));
				}
				else {
					count_reset++;
					THROW(SLCHECK_Z(list.get(c+j)));
				}
			}
			cc += n;
		}
		total = cc;
		THROW(SLCHECK_EQ(total, list.getCount()));
		THROW(SLCHECK_EQ(total, pattern.getCount()));
		THROW(SLCHECK_EQ(total, count_set + count_reset));
		for(i = 0; i < total; i++) {
			THROW(SLCHECK_EQ(list[i], (int)pattern.get(i)));
		}
		THROW(SLCHECK_EQ(count_set,   list.getCountVal(1)));
		THROW(SLCHECK_EQ(count_reset, list.getCountVal(0)));
		//
		// Тест функций bitscanforward и bitscanreverse
		//
		{
			uint32 idx;
			SLCHECK_Z(bitscanforward(&idx, 0));
			SLCHECK_EQ(idx, 0U);
			SLCHECK_NZ(bitscanforward(&idx, 0x01));
			SLCHECK_EQ(idx, 0U);
			SLCHECK_NZ(bitscanforward(&idx, 0x10));
			SLCHECK_EQ(idx, 4U);
			SLCHECK_NZ(bitscanforward(&idx, 0x80000800));
			SLCHECK_EQ(idx, 11U);

			SLCHECK_Z(bitscanreverse(&idx, 0));
			SLCHECK_EQ(idx, 0U);
			SLCHECK_NZ(bitscanreverse(&idx, 0x01));
			SLCHECK_EQ(idx, 0U);
			SLCHECK_NZ(bitscanreverse(&idx, 0x10));
			SLCHECK_EQ(idx, 4U);
			SLCHECK_NZ(bitscanreverse(&idx, 0x80000800));
			SLCHECK_EQ(idx, 31U);
		}
	}
	CATCHZOK
	delete p_rng;
	return CurrentStatus;
}

SLTEST_R(BitOps)
{
	uint clz;
	uint ctz;
	uint pop;
	SRandGenerator & r_rg = SLS.GetTLA().Rg;
	SLCHECK_EQ(SBits::Clz_fallback(0U), (uint)(sizeof(uint) << 3));
	SLCHECK_EQ(SBits::Ctz_fallback(0U), (uint)(sizeof(uint) << 3));
	SLCHECK_EQ(SBits::Cpop_fallback(0U), 0U);
	SLCHECK_EQ(SBits::Clz_fallback(0ULL), (uint)(sizeof(uint64) << 3));
	SLCHECK_EQ(SBits::Ctz_fallback(0ULL), (uint)(sizeof(uint64) << 3));
	SLCHECK_EQ(SBits::Cpop_fallback(0ULL), 0U);

	SLCHECK_EQ(SBits::Clz_fallback((uint)-1), 0U);
	SLCHECK_EQ(SBits::Ctz_fallback((uint)-1), 0U);
	SLCHECK_EQ(SBits::Cpop_fallback((uint)-1), (uint)(sizeof(uint) << 3));
	SLCHECK_EQ(SBits::Clz_fallback((uint64)-1LL), 0U);
	SLCHECK_EQ(SBits::Ctz_fallback((uint64)-1LL), 0U);
	SLCHECK_EQ(SBits::Cpop_fallback((uint64)-1LL), (uint)(sizeof(uint64) << 3));
	bool debug_mark = false;
	for(uint i = 0; i < 100000; i++) {
		const uint32 v1 = r_rg.GetUniformIntPos(MAXUINT32);
		assert(v1 != 0); // GetUniformIntPos specification
		{
			const uint32 v = v1;
			constexpr uint cbits = sizeof(v) << 3;
			clz = SBits::Clz_fallback(v);
			ctz = SBits::Ctz_fallback(v);
			pop = SBits::Cpop_fallback(v);
			const uint s1 = clz+ctz+pop;
			SLCHECK_LE((clz+ctz+pop), cbits);
			
			clz = SBits::Clz(v);
			ctz = SBits::Ctz(v);
			pop = SBits::Cpop(v);
			SLCHECK_EQ((clz+ctz+pop), s1);
			SLCHECK_LE((clz+ctz+pop), cbits);
			{
				// 
				// Циклический сдвиг на случайную (меньше числа бит в значении) величину.
				// -- Rotl и Rotr обратимы
				//
				uint nb = r_rg.GetUniformIntPos(cbits);
				const uint32 __v = SBits::Rotl(v, nb);
				SLCHECK_EQ(SBits::Rotl_fallback(v, nb), __v);
				SLCHECK_EQ(SBits::Rotr(__v, nb), v);
				SLCHECK_EQ(SBits::Rotr_fallback(__v, nb), v);
				// Чаще всего после вращения результат не равен аргументу,
				// но бывают исключения для периодических двоичных величин.
				// Поэтому сформулируем инвариант v != __v с дополнительным
				// ограничением, что все установленные биты идут спложной полосой (s1 == cbits)
				// и при этом не все биты в числен установлены (clz != 0 || ctz != 0)
				SLCHECK_NZ(!(s1 == cbits && (clz != 0 || ctz != 0)) || (v != __v));
				if(!CurrentStatus)
					debug_mark = true;
			}
		}
		const uint32 v2 = r_rg.GetUniformIntPos(MAXUINT32);
		assert(v2 != 0); // GetUniformIntPos specification
		{
			const uint64 v = (static_cast<uint64>(v1) << 32) | static_cast<uint64>(v2);
			constexpr uint cbits = sizeof(v) << 3;
			clz = SBits::Clz_fallback(v);
			ctz = SBits::Ctz_fallback(v);
			pop = SBits::Cpop_fallback(v);
			const uint s1 = clz+ctz+pop;
			SLCHECK_LE((clz+ctz+pop), cbits);
			clz = SBits::Clz(v);
			ctz = SBits::Ctz(v);
			pop = SBits::Cpop(v);
			SLCHECK_LE((clz+ctz+pop), cbits);
			{
				// 
				// Циклический сдвиг на случайную (меньше числа бит в значении) величину.
				// -- Rotl и Rotr обратимы
				//
				uint nb = r_rg.GetUniformIntPos(cbits);
				const uint64 __v = SBits::Rotl(v, nb);
				SLCHECK_EQ(SBits::Rotl_fallback(v, nb), __v);
				SLCHECK_EQ(SBits::Rotr(__v, nb), v);
				SLCHECK_EQ(SBits::Rotr_fallback(__v, nb), v);
				SLCHECK_NZ(!(s1 == cbits && (clz != 0 || ctz != 0)) || (v != __v));
				if(!CurrentStatus)
					debug_mark = true;
			}
		}
	}
	{
		uint32 v = 1U;
		for(uint i = 1; i <= (sizeof(v) << 3); i++) {
			const uint32 v2 = SBits::Rotl(v, 1);
			SLCHECK_EQ(SBits::Rotl_fallback(v, 1), v2);
			SLCHECK_EQ(SBits::Rotr(v2, 1), v);
			SLCHECK_EQ(SBits::Rotr_fallback(v2, 1), v);
			if(i == (sizeof(v) << 3)) {
				SLCHECK_LT(v2, v);
			}
			else {
				SLCHECK_LT(v, v2);
			}
			v = v2;
			clz = SBits::Clz(v);
			ctz = SBits::Ctz(v);
			pop = SBits::Cpop(v);
			SLCHECK_NZ(clz-ctz); // Так как мы вращаем единственный бит и общее число бит четное, то инвариант годный
			SLCHECK_EQ(pop, 1U);
			SLCHECK_EQ(clz+ctz+pop, (sizeof(v) << 3));
			clz = SBits::Clz_fallback(v);
			ctz = SBits::Ctz_fallback(v);
			pop = SBits::Cpop_fallback(v);
			SLCHECK_NZ(clz-ctz); // Так как мы вращаем единственный бит и общее число бит четное, то инвариант годный
			SLCHECK_EQ(pop, 1U);
			SLCHECK_EQ(clz+ctz+pop, (sizeof(v) << 3));
		}
		SLCHECK_EQ(v, 1U);
	}
	{
		uint64 v = 1ULL;
		for(uint i = 1; i <= (sizeof(v) << 3); i++) {
			uint64 v2 = SBits::Rotl(v, 1);
			SLCHECK_EQ(SBits::Rotl_fallback(v, 1), v2);
			SLCHECK_EQ(SBits::Rotr(v2, 1), v);
			SLCHECK_EQ(SBits::Rotr_fallback(v2, 1), v);
			if(i == (sizeof(v) << 3)) {
				SLCHECK_LT(v2, v);
			}
			else {
				SLCHECK_LT(v, v2);
			}
			v = v2;
			clz = SBits::Clz(v);
			ctz = SBits::Ctz(v);
			pop = SBits::Cpop(v);
			SLCHECK_NZ(clz-ctz); // Так как мы вращаем единственный бит и общее число бит четное, то инвариант годный
			SLCHECK_EQ(pop, 1U);
			SLCHECK_EQ(clz+ctz+pop, (sizeof(v) << 3));
			clz = SBits::Clz_fallback(v);
			ctz = SBits::Ctz_fallback(v);
			pop = SBits::Cpop_fallback(v);
			SLCHECK_NZ(clz-ctz); // Так как мы вращаем единственный бит и общее число бит четное, то инвариант годный
			SLCHECK_EQ(pop, 1U);
			SLCHECK_EQ(clz+ctz+pop, (sizeof(v) << 3));
		}
		SLCHECK_EQ(v, 1ULL);
	}
	{
		uint16 v = 1;
		constexpr uint bits = (sizeof(v) << 3);
		for(uint i = 1; i <= bits; i++) {
			clz = SBits::Clz(v);
			ctz = SBits::Ctz(v);
			SLCHECK_NZ(clz-ctz);
			SLCHECK_EQ(clz, bits-i);
			SLCHECK_EQ(ctz, i-1);
			SLCHECK_EQ(SBits::Cpop(v), 1U);
			v <<= 1;
		}
	}
	{
		uint8 v = 1;
		constexpr uint bits = (sizeof(v) << 3);
		for(uint i = 1; i <= bits; i++) {
			clz = SBits::Clz(v);
			ctz = SBits::Ctz(v);
			SLCHECK_NZ(clz-ctz);
			SLCHECK_EQ(clz, bits-i);
			SLCHECK_EQ(ctz, i-1);
			SLCHECK_EQ(SBits::Cpop(v), 1U);
			v <<= 1;
		}
	}
	return CurrentStatus;
}