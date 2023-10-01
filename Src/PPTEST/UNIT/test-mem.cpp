// TEST-MEM.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

#define MEMPAIR_COUNT 1000
#define MEMBLK_SIZE   SKILOBYTE(36)
#define MEMBLK_EXT_SIZE 32
#define BYTES_SIZE    SKILOBYTE(5) // Величина должна превышать размер страницы памяти

//#include <asmlib.h> // @v7.0.12

SLTEST_R(Endian)
{
	int    ok = 1;
	const uint64 k_initial_number{0x0123456789abcdef};
	const uint64 k64value{k_initial_number};
	const uint   k32value_u{0x01234567U};
	const ulong  k32value_l{0x01234567UL};
	const uint16 k16value{0x0123};
#if defined(SL_BIGENDIAN)
	const uint64_t k_initial_in_network_order{k_initial_number};
	const uint64_t k64value_le{0xefcdab8967452301};
	const uint32_t k32value_le{0x67452301};
	const uint16_t k16value_le{0x2301};

	const uint64_t k64value_be{kInitialNumber};
	const uint32_t k32value_be{k32value_u};
	const uint16_t k16value_be{k16value};
#elif defined(SL_LITTLEENDIAN)
	const uint64_t k_initial_in_network_order{0xefcdab8967452301};
	const uint64_t k64value_le{k_initial_number};
	const uint32_t k32value_le{k32value_u};
	const uint16_t k16value_le{k16value};

	const uint64_t k64value_be{0xefcdab8967452301};
	const uint32_t k32value_be{0x67452301};
	const uint16_t k16value_be{0x2301};

	assert(k64value_be == sbswap64(k64value));
	assert(k32value_be == sbswap32(k32value_u));
	assert(k32value_be == sbswap32(k32value_l));
	assert(k16value_be == sbswap16(k16value));
	assert(k64value_be == sbswap64_fallback(k64value));
	assert(k32value_be == sbswap32_fallback(k32value_u));
	assert(k32value_be == sbswap32_fallback(k32value_l));
	assert(k16value_be == sbswap16_fallback(k16value));
#endif
	return ok;
}

struct SlTestFixtureMEMMOVO {
public:
	SlTestFixtureMEMMOVO()
	{
		assert(BYTES_SIZE % 4 == 0);
		P_Bytes = static_cast<char *>(SAlloc::M(BYTES_SIZE));
		assert(P_Bytes);
		P_Pattern = static_cast<char *>(SAlloc::M(BYTES_SIZE));
		assert(P_Pattern);
		{
			SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
			p_rng->Set(7619);
			for(size_t i = 0; i < BYTES_SIZE/sizeof(uint32); i++) {
				((uint32 *)P_Bytes)[i] = ((uint32 *)P_Pattern)[i] = p_rng->Get();
			}
			delete p_rng;
		}
	}
	~SlTestFixtureMEMMOVO()
	{
		SAlloc::F(P_Bytes);
		SAlloc::F(P_Pattern);
	}
	char * P_Bytes;
	char * P_Pattern;
};

SLTEST_FIXTURE(MEMMOVO, SlTestFixtureMEMMOVO)
{
	int    ok = 1;
	char * b = (char *)F.P_Bytes;
	const  size_t bs = BYTES_SIZE;
	int    bm = -1;
	if(pBenchmark == 0)
		bm = 0;
	else if(sstreqi_ascii(pBenchmark, "memmovo"))
		bm = 1;
	else if(sstreqi_ascii(pBenchmark, "memmove"))
		bm = 2;
	else if(sstreqi_ascii(pBenchmark, "A_memmove"))
		bm = 3;
	else if(sstreqi_ascii(pBenchmark, "char[0]=0")) // @v11.4.11
		bm = 4;
	else if(sstreqi_ascii(pBenchmark, "PTR32(char)[0]=0")) // @v11.4.11
		bm = 5;
	else if(sstreqi_ascii(pBenchmark, "PTR64(char)[0]=0")) // @v11.4.11
		bm = 6;
	else
		SetInfo("invalid benchmark");
	if(bm >= 0) {
		if(oneof3(bm, 4, 5, 6)) {
			const uint iter_count = 4000000;
			STempBuffer zbuf(iter_count*8);
			LongArray idx_list;
			{
				SRandGenerator & r_rg = SLS.GetTLA().Rg;
				for(uint i = 0; i < iter_count; i++) {
					long idx = static_cast<long>(r_rg.GetUniformInt(iter_count-8));
					assert(idx >= 0 && idx < (iter_count-8));
					idx_list.add(idx);
				}
			}
			if(bm == 4) {
				assert(idx_list.getCount() == iter_count);
				for(uint i = 0; i < iter_count; i++) {
					const long idx = idx_list.get(i);
					static_cast<char *>(zbuf.vptr())[idx] = 0;
				}
			}
			else if(bm == 5) {
				assert(idx_list.getCount() == iter_count);
				for(uint i = 0; i < iter_count; i++) {
					const long idx = idx_list.get(i);
					PTR32(zbuf.vptr())[idx] = 0;
				}
			}
			else if(bm == 6) {
				assert(idx_list.getCount() == iter_count);
				for(uint i = 0; i < iter_count; i++) {
					const long idx = idx_list.get(i);
					PTR64(zbuf.vptr())[idx] = 0;
				}
			}
		}
		else {
			size_t s;
			for(s = 1; s <= bs/4; s++) {
				const size_t start = bs/4;
				const size_t zone  = bs/2;
				for(size_t offs = start; offs < (start+zone-s); offs++) {
					const size_t src  = offs;
					const size_t dest = bs-offs-s;
					assert(src >= start && src < start+zone);
					assert(dest >= start && dest < start+zone);
					if(bm == 0) {
						//
						// Тестируем A_memmove
						// Она должна скопировать блок длиной s из одной части F.P_Bytes в другую не задев
						// сопредельные участки памяти. Результат копирования сравниваем с F.P_Pattern
						//
						//memmovo(F.P_Bytes+dest, F.P_Bytes+src, s);
						A_memmove(F.P_Bytes+dest, F.P_Bytes+src, s);
						THROW(SLCHECK_Z(memcmp(F.P_Bytes+dest, F.P_Pattern+src, s)));
						THROW(SLCHECK_Z(memcmp(F.P_Bytes, F.P_Pattern, dest)));
						THROW(SLCHECK_Z(memcmp(F.P_Bytes+dest+s, F.P_Pattern+dest+s, BYTES_SIZE-dest-s)));
						//
						// Стандартная процедура копирования для восстановления эквивалентности P_Bytes и P_Pattern
						// Закладываемся на то, что memmove работает правильно.
						// В случае бенчмарка этот вызов "разбавляет" кэш за счет обращения к отличному от F.P_Bytes
						// блоку памяти.
						//
#undef memmove
						memmove(F.P_Pattern+dest, F.P_Pattern+src, s);
					}
					else if(bm == 1) {
						// @v11.6.5 memmovo(F.P_Bytes+dest, F.P_Bytes+src, s);
						// @v11.6.5 memmovo(F.P_Pattern+dest, F.P_Pattern+src, s);
					}
					else if(bm == 2) {
						memmove(F.P_Bytes+dest, F.P_Bytes+src, s);
						memmove(F.P_Pattern+dest, F.P_Pattern+src, s);
					}
					else if(bm == 3) {
						A_memmove(F.P_Bytes+dest, F.P_Bytes+src, s);
						A_memmove(F.P_Pattern+dest, F.P_Pattern+src, s);
					}
				}
			}
			//
			// Тестирование A_memset и ismemzero
			//
			if(bm == 0) {
				for(s = 1; s <= bs/4; s++) {
					const size_t start = bs/4;
					const size_t zone  = bs/2;
					for(size_t offs = start; offs < (start+zone-s); offs++) {
						const size_t src  = offs;
						const size_t dest = bs-offs-s;
						assert(src >= start && src < start+zone);
						assert(dest >= start && dest < start+zone);
						//
						// Тестируем A_memset
						// Она должна обнулить заданный участок памяти не задев сопредельные участки.
						//
						A_memset(F.P_Bytes+dest, 0, s);
						THROW(SLCHECK_NZ(ismemzero(F.P_Bytes+dest, s)));
						THROW(SLCHECK_Z(memcmp(F.P_Bytes, F.P_Pattern, dest)));
						THROW(SLCHECK_Z(memcmp(F.P_Bytes+dest+s, F.P_Pattern+dest+s, BYTES_SIZE-dest-s)));
						//
						// Возвращаем назад содержимое F.P_Bytes[dest..s-1]
						//
						memmove(F.P_Bytes+dest, F.P_Pattern+dest, s);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
void * AVX_memset(void * dest, int val, size_t numbytes);

static bool Test_memset_fill_or_check(bool check, void * ptr, uint8 byte, size_t size, size_t leftSentinelSize, size_t rightSentinelSize, uint8 sentinelByte)
{
	size_t offs = 0;
	bool   check_result = true;
	assert(ptr != 0);
	assert(size > 0);
	assert(byte != sentinelByte);
	assert(leftSentinelSize <= 16 && rightSentinelSize <= 16);
	if(ptr && size && byte != sentinelByte) {
		if(leftSentinelSize) {
			if(check) {
				for(size_t i = 0; i < leftSentinelSize; i++) {
					if(PTR8(ptr)[i+offs] != sentinelByte)
						check_result = false;
					assert(check_result);
				}
			}
			else {
				for(size_t i = 0; i < leftSentinelSize; i++)
					PTR8(ptr)[i+offs] = sentinelByte;
			}
			offs += leftSentinelSize;
		}
		if(check_result) {
			for(size_t i = 0; i < size; i++) {
				if(check) {
					if(PTR8(ptr)[i+offs] != byte)
						check_result = false;
				}
				else {
					PTR8(ptr)[i+offs] = byte;
				}
			}
			assert(check_result);
			offs += size;
			if(check_result && rightSentinelSize) {
				if(check) {
					for(size_t i = 0; i < rightSentinelSize; i++) {
						if(PTR8(ptr)[i+offs] != sentinelByte)
							check_result = false;
						assert(check_result);
					}
				}
				else {
					for(size_t i = 0; i < rightSentinelSize; i++)
						PTR8(ptr)[i+offs] = sentinelByte;
				}
				offs += rightSentinelSize;
			}
		}
	}
	else
		check_result = false;
	assert(offs == (leftSentinelSize+size+rightSentinelSize));
	return check_result;
}

static bool Test_memset_helper(void * (*func)(void *, int, size_t), uint8 * pBuffer, size_t bufferSize)
{
	bool ok = true;
	const uint8 sentinel_byte = 0x0B;
	const uint8 pre_fill_byte = 0xff;
	const uint8 memset_byte = 0x01;
	const size_t left_sentinel_size = 5;
	const size_t right_sentinel_size = 5;
	assert(pre_fill_byte != memset_byte);
	for(size_t offs = 0; offs <= 128 && (offs + left_sentinel_size + right_sentinel_size) < bufferSize; offs++) {
		for(size_t test_size = 1; ((offs + left_sentinel_size + test_size + right_sentinel_size) <= bufferSize); test_size++) {
			int r1 = Test_memset_fill_or_check(false, pBuffer+offs, pre_fill_byte, test_size, left_sentinel_size, right_sentinel_size, sentinel_byte);
			assert(r1);
			THROW(r1);
			void * p_memset_result = func(pBuffer+offs+left_sentinel_size, memset_byte, test_size);
			assert(p_memset_result == pBuffer+offs+left_sentinel_size);
			THROW(p_memset_result == pBuffer+offs+left_sentinel_size);
			int r2 = Test_memset_fill_or_check(true, pBuffer+offs, memset_byte, test_size, left_sentinel_size, right_sentinel_size, sentinel_byte);
			assert(r2);
			THROW(r2);
		}
	}
	CATCHZOK
	return ok;
}

static void Profile_memset_helper(void * (*func)(void *, int, size_t), uint8 * pBuffer, size_t bufferSize)
{
	const uint8 memset_byte = 0xff;
	volatile void * p_memset_result = 0;
	for(size_t offs = 0; offs <= 64 && offs < bufferSize; offs++) {
		for(size_t test_size = 1; ((offs + test_size) <= bufferSize); test_size++) {
			p_memset_result = func(pBuffer+offs, memset_byte, test_size);
		}
	}
}

#undef memset

SLTEST_R(memset)
{
	//A_memset;AVR_memset;memset
	int    bm = -1;
	if(pBenchmark == 0) 
		bm = 0;
	else if(sstreqi_ascii(pBenchmark, "memset"))
		bm = 1;
	else if(sstreqi_ascii(pBenchmark, "A_memset"))
		bm = 2;
	else if(sstreqi_ascii(pBenchmark, "AVR_memset"))      
		bm = 3;
	if(bm == 0) {
		const size_t buffer_size = SKILOBYTE(32);
		STempBuffer buffer(buffer_size);
		SLCHECK_NZ(Test_memset_helper(memset, static_cast<uint8 *>(buffer.vptr()), buffer_size));
		SLCHECK_NZ(Test_memset_helper(A_memset, static_cast<uint8 *>(buffer.vptr()), buffer_size));
		SLCHECK_NZ(Test_memset_helper(AVX_memset, static_cast<uint8 *>(buffer.vptr()), buffer_size));
	}
	else {
		const size_t buffer_size = SKILOBYTE(64);
		STempBuffer buffer(buffer_size);
		if(bm == 1) {
			Profile_memset_helper(memset, static_cast<uint8 *>(buffer.vptr()), buffer_size);
		}
		if(bm == 2) {
			Profile_memset_helper(A_memset, static_cast<uint8 *>(buffer.vptr()), buffer_size);
		}
		if(bm == 3) {
			Profile_memset_helper(AVX_memset, static_cast<uint8 *>(buffer.vptr()), buffer_size);
		}
	}
	return CurrentStatus;
}
