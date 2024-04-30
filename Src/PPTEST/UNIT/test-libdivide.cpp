// TEST-LIBDIVIDE.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование библиотеки libdivide
//
#include <pp.h>
#pragma hdrstop

#include <libdivide.h>
#include "..\OSF\libdivide\constant_fast_div.h"
#include "..\OSF\libdivide\test\DivideTest.h"

using namespace libdivide;

#if defined(_MSC_VER)
	#pragma warning(disable : 4146)
#endif
#define MIN_RANGE (UINT16_MAX/4U)
#define LOOP_STEP 3
#define ABS(a)   MAX(-(a), a)

#define LOOP_START(denom) MIN(((denom*2)+LOOP_STEP), ((denom/2)+LOOP_STEP))
#define LOOP_END(type, denom, range_max) MIN(MAX((type)MIN_RANGE, ABS(denom)*4), range_max-(LOOP_STEP*2))
#define ASSERT_EQUAL(type, numer, denom, libdiv_result, native_result, format_spec) \
	if(libdiv_result!=native_result) { \
		fprintf(stderr, "Division fail: " #type ", %" format_spec "/%" format_spec ". Native: %" format_spec  ", Libdivide %" format_spec "\n", \
		    numer, (type)denom, native_result, libdiv_result); \
	}
#define TEST_ONE(type, numer, denom, divider, format_spec, OPERATION) \
	type libdiv_result = OPERATION(numer, divider); \
	type native_result = numer / denom; \
	ASSERT_EQUAL(type, numer, denom, libdiv_result, native_result, format_spec)

#define TEST_BODY(type, range_max, denom, divider, format_spec, OPERATION) \
	/* We need to be careful to have a wide enough range AND increment!=1 or else GCC figures out */ \
	/* this is a constant range and applies all sorts of optimizations */ \
	{ \
		type loop = (type)LOOP_START(denom); \
		const type end = (type)LOOP_END(type, denom, range_max); \
		const type step = MAX(LOOP_STEP, (end-loop)/(2<<12)); \
		printf("Testing " #type ", %" format_spec " from %" format_spec " to %" format_spec ", step %" format_spec "\n", (type)denom,  loop,  end, step); \
		for(; loop < end; loop += step) { \
			TEST_ONE(type, loop, denom, divider, format_spec, OPERATION) \
		} \
	}

// This is simply a regression test for #96: that the following all compile (and don't crash).
static void test_primitives_compile() 
{
	libdivide::divider<short> d0(1);
	libdivide::divider<int> d1(1);
	libdivide::divider<long> d2(1);
	libdivide::divider<long long> d3(1);
	libdivide::divider<unsigned short> u0(1);
	libdivide::divider<unsigned int> u1(1);
	libdivide::divider<unsigned long> u2(1);
	libdivide::divider<unsigned long long> u3(1);
	// These should fail to compile.
	// libdivide::divider<float> f0(1);
	// libdivide::divider<double> f1(1);
}

static void wait_for_threads(std::vector<std::thread> &test_threads) 
{
	for(auto &t : test_threads) {
		t.join();
	}
}

static uint8_t get_max_threads() { return (uint8_t)std::max(1U, std::thread::hardware_concurrency()); }

template <typename _IntT> void launch_test_thread(std::vector<std::thread> &test_threads) 
{
	static uint8_t max_threads = get_max_threads();
	if(max_threads == test_threads.size()) {
		wait_for_threads(test_threads);
		test_threads.clear();
	}
	test_threads.emplace_back(run_test<_IntT>);
}

SLTEST_R(libdivide)
{
	// bencmarks: libdivide;machine
	if(isempty(pBenchmark)) {
		{ //test_u16() 
			#define U16_DENOM 953 // Prime
				struct libdivide_u16_t divider = libdivide_u16_gen(U16_DENOM);
			#define OP_U16_DO(numer, divider) libdivide_u16_do(numer, &divider)
				TEST_BODY(uint16_t, UINT16_MAX, U16_DENOM, divider, PRIu16, OP_U16_DO)
			#define CONSTANT_OP_U16(numer, denom) FAST_DIV16U(numer, denom)
				printf("Constant division ");
				TEST_BODY(uint16_t, UINT16_MAX, U16_DENOM, U16_DENOM, PRIu16, CONSTANT_OP_U16)
		}
		{ // test_s16() 
			int16_t denom = (int16_t)-4003; // Prime
			struct libdivide_s16_t divider = libdivide_s16_gen(denom);
		#define OP_S16(numer, divider) libdivide_s16_do(numer, &divider)
			TEST_BODY(int16_t, INT16_MAX, denom, divider, PRId16, OP_S16)

		#define CONSTANT_OP_S16(numer, denom) FAST_DIV16(numer, denom)
			printf("Constant division ");
			TEST_BODY(int16_t, INT16_MAX, 4003, 4003, PRId16, CONSTANT_OP_S16)

		#define CONSTANT_OP_NEG_S16(numer, denom) FAST_DIV16_NEG(numer, denom)
			printf("Constant division ");
			TEST_BODY(int16_t, INT16_MAX, -4003, 4003, PRId16, CONSTANT_OP_NEG_S16)
		}
		{ // test_u32() 
			uint32_t denom = ((uint32_t)2 << 21) - 19; // Prime - see https://primes.utm.edu/lists/2small/0bit.html
			struct libdivide_u32_t divider = libdivide_u32_gen(denom);
		#define OP_U32(numer, divider) libdivide_u32_do(numer, &divider)
			TEST_BODY(uint32_t, UINT32_MAX, denom, divider, PRIu32, OP_U32)
		}
		{ // test_s32() 
			int32_t denom = -(((int32_t)2 << 21) - 55); // Prime - see https://primes.utm.edu/lists/2small/0bit.html
			struct libdivide_s32_t divider = libdivide_s32_gen(denom);
		#define OP_S32(numer, divider) libdivide_s32_do(numer, &divider)
			TEST_BODY(int32_t, INT32_MAX, denom, divider, PRId32, OP_S32)
		}
		{ // test_u64() 
			uint64_t denom = ((uint64_t)2 << 29) - 43; // Prime - see https://primes.utm.edu/lists/2small/0bit.html
			struct libdivide_u64_t divider = libdivide_u64_gen(denom);
		#define OP_U64(numer, divider) libdivide_u64_do(numer, &divider)
			TEST_BODY(uint64_t, (UINT64_MAX/2) /* For speed */, denom, divider, PRIu64, OP_U64)
		}
		{ // test_s64() 
			int64_t denom =  -(((int64_t)2 << 29) - 121);// Prime - see https://primes.utm.edu/lists/2small/0bit.html
			struct libdivide_s64_t divider = libdivide_s64_gen(denom);
		#define OP_S64(numer, divider) libdivide_s64_do(numer, &divider)
			TEST_BODY(int64_t, INT64_MAX, denom, divider, PRId64, OP_S64)
		}
		{
			test_primitives_compile();
			std::string vecTypes = "";
		#if defined(LIBDIVIDE_SSE2) || defined(USE_SSE2)
			vecTypes += "sse2 ";
		#endif
		#if defined(LIBDIVIDE_AVX2)
			vecTypes += "avx2 ";
		#endif
		#if defined(LIBDIVIDE_AVX512)
			vecTypes += "avx512 ";
		#endif
		#if defined(LIBDIVIDE_NEON)
			vecTypes += "neon ";
		#endif
			if(vecTypes.empty()) {
				vecTypes = "none ";
			}
			vecTypes.back() = '\n'; // trailing space
			std::cout << "Testing with SIMD ISAs: " << vecTypes;
			// Run tests in threads.
			std::vector<std::thread> test_threads;
			launch_test_thread<int16_t>(test_threads);
			launch_test_thread<uint16_t>(test_threads);
			launch_test_thread<int32_t>(test_threads);
			launch_test_thread<uint32_t>(test_threads);
			launch_test_thread<int64_t>(test_threads);
			launch_test_thread<uint64_t>(test_threads);
			wait_for_threads(test_threads);
			std::cout << "\nAll tests passed successfully!" << std::endl;
		}
	}
	else {
		constexpr uint round_count = 10000000;
		constexpr uint denom_list[] = { 3, 10, 100, 1000, 3600, 37, 2001, 2, 4, 64, 999999999 };
		static int64 check_sum_libdivide;
		static int64 check_sum_machine;
		static int64 check_sum_empty;
		if(sstreqi_ascii(pBenchmark, "empty")) {
			check_sum_empty = 0;
			for(uint di = 0; di < SIZEOFARRAY(denom_list); di++) {
				for(uint i = 0; i < round_count; i++) {
					int64 r = i;
					check_sum_empty += r;
				}
			}
		}
		else if(sstreqi_ascii(pBenchmark, "libdivide")) {
			check_sum_libdivide = 0;
			for(uint di = 0; di < SIZEOFARRAY(denom_list); di++) {
				struct libdivide_s64_t divider = libdivide_s64_gen(denom_list[di]);
				for(uint i = 0; i < round_count; i++) {
					int64 r = libdivide_s64_do((int64)i+1, &divider);
					check_sum_libdivide += r;
				}
			}
		}
		else if(sstreqi_ascii(pBenchmark, "machine")) {
			check_sum_machine = 0;
			for(uint di = 0; di < SIZEOFARRAY(denom_list); di++) {
				int64 denom = denom_list[di];
				for(uint i = 0; i < round_count; i++) {
					int64 r = ((int64)(i+1) / denom);
					check_sum_machine += r;
				}
			}
		}
		if(check_sum_libdivide != 0 && check_sum_machine != 0) {
			assert(check_sum_libdivide == check_sum_machine);
		}
	}
	return CurrentStatus;
}
