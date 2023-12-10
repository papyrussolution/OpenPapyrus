// TEST-MATH.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop

bool FASTCALL Helper_IsPrime(ulong val, int test);

SLTEST_R(smath)
{
	{
		// 12.0/5.0 == 2.4
		SLCHECK_EQ(idivroundn(12, 5), 2);
		SLCHECK_EQ(idivroundn(12L, 5L), 2L);
		SLCHECK_EQ(idivroundn(12LL, 5LL), 2LL);
		SLCHECK_EQ(idivroundup(12, 5), 3);
		SLCHECK_EQ(idivroundup(12L, 5L), 3L);
		SLCHECK_EQ(idivroundup(12LL, 5LL), 3LL);

		// 13.0/5.0 == 2.6
		SLCHECK_EQ(idivroundn(13, 5), 3);
		SLCHECK_EQ(idivroundn(13L, 5L), 3L);
		SLCHECK_EQ(idivroundn(13LL, 5LL), 3LL);
		SLCHECK_EQ(idivroundup(13, 5), 3);
		SLCHECK_EQ(idivroundup(13L, 5L), 3L);
		SLCHECK_EQ(idivroundup(13LL, 5LL), 3LL);

		// 8.0/4.0 == 2.0
		SLCHECK_EQ(idivroundn(8, 4), 2);
		SLCHECK_EQ(idivroundn(8L, 4L), 2L);
		SLCHECK_EQ(idivroundn(8LL, 4LL), 2LL);
		SLCHECK_EQ(idivroundup(8, 4), 2);
		SLCHECK_EQ(idivroundup(8L, 4L), 2L);
		SLCHECK_EQ(idivroundup(8LL, 4LL), 2LL);

		// 9.0/4.0 == 2.25
		SLCHECK_EQ(idivroundn(9, 4), 2);
		SLCHECK_EQ(idivroundn(9L, 4L), 2L);
		SLCHECK_EQ(idivroundn(9LL, 4LL), 2LL);
		SLCHECK_EQ(idivroundup(9, 4), 3);
		SLCHECK_EQ(idivroundup(9L, 4L), 3L);
		SLCHECK_EQ(idivroundup(9LL, 4LL), 3LL);

		// 10.0/4.0 == 2.5
		SLCHECK_EQ(idivroundn(10, 4), 3);
		SLCHECK_EQ(idivroundn(10L, 4L), 3L);
		SLCHECK_EQ(idivroundn(10LL, 4LL), 3LL);
		SLCHECK_EQ(idivroundup(10, 4), 3);
		SLCHECK_EQ(idivroundup(10L, 4L), 3L);
		SLCHECK_EQ(idivroundup(10LL, 4LL), 3LL);
	}
	{
		struct rational_test_param {
			ulong  num;
			ulong  den;
			ulong  max_num;
			ulong  max_den;
			ulong  exp_num;
			ulong  exp_den;
			const  char * name;
		};

		static const struct rational_test_param test_parameters[] = {
			{  1230,   10,   100,   20,  100,    1, "Exceeds bounds, semi-convergent term > 1/2 last term" },
			{ 34567,  100,   120,   20,  120,    1, "Exceeds bounds, semi-convergent term < 1/2 last term" },
			{     1,   30,   100,   10,    0,    1, "Closest to zero" },
			{     1,   19,   100,   10,    1,   10, "Closest to smallest non-zero" },
			{    27,   32,    16,   16,   11,   13, "Use convergent" },
			{  1155, 7735,   255,  255,   33,  221, "Exact answer" },
			{    87,   32,    70,   32,   68,   25, "Semiconvergent, numerator limit" },
			{ 14533, 4626, 15000, 2400, 7433, 2366, "Semiconvergent, denominator limit" },
		};
		for(uint i = 0; i < SIZEOFARRAY(test_parameters); i++) {
			const rational_test_param & r_param = test_parameters[i];
			ulong n = 0;
			ulong d = 0;
			RationalBestApproximation(r_param.num, r_param.den, r_param.max_num, r_param.max_den, &n, &d);
			SLCHECK_EQ(n, r_param.exp_num);
			SLCHECK_EQ(d, r_param.exp_den);
		}
	}
	{
		//
		// fsplitintofractions
		//
		for(double ip = 0.0; ip <= +117.0; ip += 1.0) {
			for(uint fragmentation = 2; fragmentation <= 1000; fragmentation++) {
				for(uint j = 0; j < fragmentation; j++) {
					double value = ip + static_cast<double>(j) / static_cast<double>(fragmentation);
					double intpart = -1.0;
					double numerator = -1.0;
					double denominator = -1.0;
					int r = fsplitintofractions(value, fragmentation, 1E-5, &intpart, &numerator, &denominator);
					SLCHECK_NZ(r);
					SLCHECK_EQ(intpart, fint(fabs(value)));
					SLCHECK_EQ(denominator, static_cast<double>(fragmentation));
					SLCHECK_EQ(numerator, static_cast<double>(j));
					// Проверка нулевых указателей в качестве результирующих параметров
					SLCHECK_NZ(fsplitintofractions(value, fragmentation, 1E-5, 0, 0, 0));
				}
			}
		}
		{
			double intpart = -1.0;
			double numerator = -1.0;
			double denominator = -1.0;
			SLCHECK_Z(fsplitintofractions(1.1, 0, 1E-5, &intpart, &numerator, &denominator));
			SLCHECK_EQ(intpart, 1.0);
			SLCHECK_EQ(denominator, 0.0);
			SLCHECK_EQ(numerator, 0.0);
			SLCHECK_Z(fsplitintofractions(-0.2, 10000000, 1E-5, &intpart, &numerator, &denominator));
			SLCHECK_EQ(intpart, 0.0);
			SLCHECK_EQ(denominator, 0.0);
			SLCHECK_EQ(numerator, 0.0);
		}
	}
	{
		//
		// fpow10i
		//
		for(int i = -9; i < +9; i++) {
			SLCHECK_EQ(fpow10i(i), pow(10.0, i));
		}
	}
	{
		struct Pow10i_TestEntry {
			int    power;     // Testing Pow10(power)
			uint64 significand; // Raw bits of the expected value
			int    radix;     // significand is adjusted by 2^radix
		};
		//
		// The logic in pow10_helper.cc is so simple that theoretically we don't even
		// need a test. However, we're paranoid and believe that there may be
		// compilers that don't round floating-point literals correctly, even though
		// it is specified by the standard. We check various edge cases, just to be sure.
		//
		constexpr Pow10i_TestEntry pow10i_test_entries[] = {
			// Subnormals
			{-323, 0x2, -1074},
			{-322, 0x14, -1074},
			{-321, 0xca, -1074},
			{-320, 0x7e8, -1074},
			{-319, 0x4f10, -1074},
			{-318, 0x316a2, -1074},
			{-317, 0x1ee257, -1074},
			{-316, 0x134d761, -1074},
			{-315, 0xc1069cd, -1074},
			{-314, 0x78a42205, -1074},
			{-313, 0x4b6695433, -1074},
			{-312, 0x2f201d49fb, -1074},
			{-311, 0x1d74124e3d1, -1074},
			{-310, 0x12688b70e62b, -1074},
			{-309, 0xb8157268fdaf, -1074},
			{-308, 0x730d67819e8d2, -1074},
			// Values that are very close to rounding the other way.
			// Comment shows difference of significand from the true value.
			{-307, 0x11fa182c40c60d, -1072}, // -.4588
			{-290, 0x18f2b061aea072, -1016}, //  .4854
			{-276, 0x11BA03F5B21000, -969}, //  .4709
			{-259, 0x1899C2F6732210, -913}, //  .4830
			{-252, 0x1D53844EE47DD1, -890}, // -.4743
			{-227, 0x1E5297287C2F45, -807}, // -.4708
			{-198, 0x1322E220A5B17E, -710}, // -.4714
			{-195, 0x12B010D3E1CF56, -700}, //  .4928
			{-192, 0x123FF06EEA847A, -690}, //  .4968
			{-163, 0x1708D0F84D3DE7, -594}, // -.4977
			{-145, 0x13FAAC3E3FA1F3, -534}, // -.4785
			{-111, 0x133D4032C2C7F5, -421}, //  .4774
			{-106, 0x1D5B561574765B, -405}, // -.4869
			{-104, 0x16EF5B40C2FC77, -398}, // -.4741
			{-88, 0x197683DF2F268D, -345}, // -.4738
			{-86, 0x13E497065CD61F, -338}, //  .4736
			{-76, 0x17288E1271F513, -305}, // -.4761
			{-63, 0x1A53FC9631D10D, -262}, //  .4929
			{-30, 0x14484BFEEBC2A0, -152}, //  .4758
			{-21, 0x12E3B40A0E9B4F, -122}, // -.4916
			{-5, 0x14F8B588E368F1, -69}, //  .4829
			{23, 0x152D02C7E14AF6, 24}, // -.5000 (exactly, round-to-even)
			{29, 0x1431E0FAE6D721, 44}, // -.4870
			{34, 0x1ED09BEAD87C03, 60}, // -.4721
			{70, 0x172EBAD6DDC73D, 180}, //  .4733
			{105, 0x1BE7ABD3781ECA, 296}, // -.4850
			{126, 0x17A2ECC414A03F, 366}, // -.4999
			{130, 0x1CDA62055B2D9E, 379}, //  .4855
			{165, 0x115D847AD00087, 496}, // -.4913
			{172, 0x14B378469B6732, 519}, //  .4818
			{187, 0x1262DFEEBBB0F9, 569}, // -.4805
			{210, 0x18557F31326BBB, 645}, // -.4992
			{212, 0x1302CB5E6F642A, 652}, // -.4838
			{215, 0x1290BA9A38C7D1, 662}, // -.4881
			{236, 0x1F736F9B3494E9, 731}, //  .4707
			{244, 0x176EC98994F489, 758}, //  .4924
			{250, 0x1658E3AB795204, 778}, // -.4963
			{252, 0x117571DDF6C814, 785}, //  .4873
			{254, 0x1B4781EAD1989E, 791}, // -.4887
			{260, 0x1A03FDE214CAF1, 811}, //  .4784
			{284, 0x1585041B2C477F, 891}, //  .4798
			{304, 0x1D2A1BE4048F90, 957}, // -.4987
			// Out-of-range values
			{-324, 0x0, 0},
			{-325, 0x0, 0},
			{-326, 0x0, 0},
			{309, 1, 2000},
			{310, 1, 2000},
			{311, 1, 2000},
		};
		for(const Pow10i_TestEntry & r_entry : pow10i_test_entries) {
			SLCHECK_EQ(fpow10i(r_entry.power), ldexp(r_entry.significand, r_entry.radix));
			/*
			EXPECT_EQ(Pow10(test_case.power), std::ldexp(test_case.significand, test_case.radix))
				<< absl::StrFormat("Failure for Pow10(%d): %a vs %a", test_case.power,
				Pow10(test_case.power), std::ldexp(test_case.significand, test_case.radix));
			*/
		}
	}
	{
		//
		// @v11.8.5 Тестирование некоторых целочисленных функций:
		//   CheckOverflowMul, log10i_floor, log10i_ceil
		//
		auto & r_rg = SLS.GetTLA().Rg;
		bool debug_mark = false;
		for(uint i = 0; CurrentStatus && i < 100000; i++) {
			const int32 r1 = static_cast<int32>(r_rg.GetUniformIntPos((i & 1) ? UINT_MAX : (UINT_MAX>>6)));
			const int32 r2 = static_cast<int32>(r_rg.GetUniformIntPos((i & 1) ? UINT_MAX : (UINT_MAX>>5)));
			const bool cor = CheckOverflowMul(r1, r2);
			const int64 r64 = static_cast<int64>(r1) * static_cast<int64>(r2);
			SLCHECK_NZ(!cor || HiDWord(r64) == 0);
			if(!CurrentStatus)
				debug_mark = false;
			{
				uint lf = log10i_floor(static_cast<uint32>(r1));
				uint lc = log10i_ceil(static_cast<uint32>(r1));
				SLCHECK_NZ(lf <= lc);
				const double p = fpow10i(lc);
				SLCHECK_NZ(lf != lc || p == static_cast<double>(r1));
				SLCHECK_NZ(oneof2((lc-lf), 0, 1));
				if(!CurrentStatus)
					debug_mark = false;
			}
			{
				uint lf = log10i_floor(static_cast<uint32>(r2));
				uint lc = log10i_ceil(static_cast<uint32>(r2));
				SLCHECK_NZ(lf <= lc);
				const double p = fpow10i(lc);
				SLCHECK_NZ(lf != lc || p == static_cast<double>(r2));
				SLCHECK_NZ(oneof2((lc-lf), 0, 1));
				if(!CurrentStatus)
					debug_mark = false;
			}
			{
				uint lf = log10i_floor(static_cast<uint64>(r64));
				uint lc = log10i_ceil(static_cast<uint64>(r64));
				SLCHECK_NZ(lf <= lc);
				const double p = fpow10i(lc);
				SLCHECK_NZ(lf != lc || p == static_cast<double>(r64));
				SLCHECK_NZ(oneof2((lc-lf), 0, 1));
				if(!CurrentStatus)
					debug_mark = false;
			}
		}
	}
	{
		//
		// round
		//
		struct A {
			int    R;  // точность округления //
			double T;  // тестовое значение
			double E0; // ожидаемый результат функции round(x, 0)
			double E1; // ожидаемый результат функции roundnev(x, 0)
		};
		A a[] = {
			{0, 0., 0., 0.},
			{0, -1., -1., -1.},
			{0, -5.00000000103, -5., -5.},
			{0, -4.99999981, -5., -5.},
			{0, -2.5, -3., -2.},
			{0, -17.5, -18., -18.},
			{0, -1023.499999, -1023., -1023.},
			{0, 5.00000000103, 5., 5.},
			{0, 4.99999981, 5., 5.},
			{0, 2.5, 3., 2.},
			{0, 17.5, 18., 18.},
			{0, 1023.499999, 1023., 1023.},
			{2, 72.055, 72.06, 72.06 }
		};
		uint i;
		int  j;
		for(i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
			SLCHECK_EQ(round   (a[i].T, a[i].R), a[i].E0);
			SLCHECK_EQ(roundnev(a[i].T, a[i].R), a[i].E1);
		}
		for(j = -11; j < +13; j++) {
			double p_ = fpow10i(j);
			for(i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
				if(a[i].R == 0) { // Для ненулевого начального округления возникает мелкая ошибка
					double v = a[i].T / p_;
					SLCHECK_EQ(round(v, j),    a[i].E0 / p_);
					SLCHECK_EQ(roundnev(v, j), a[i].E1 / p_);
				}
			}
		}
		if(CurrentStatus) {
			SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
			for(i = 0; i < 1000; i++) {
				double b, v, r;
				for(j = -11; j < +13; j++) {
					b = p_rng->GetReal();
					if(i % 2)
						b = _chgsign(b);
					v = round(b, j);
					SLCHECK_LT(fabs(v - b), fpow10i(-j)/2.0);
					r = fabs(b * fpow10i(j));
					SLCHECK_CRANGE(fabs(v), floor(r)/fpow10i(j), ceil(r)/fpow10i(j));
				}
			}
			delete p_rng;
		}
	}
	{
		// 
		// Prime numbers
		//
		size_t prime_tab_count = 0;
		const ushort * p_first_prime_numbers = GetPrimeTab(&prime_tab_count);
		ulong last_tabbed_prime = p_first_prime_numbers[prime_tab_count-1];
		for(ulong i = 0; i < last_tabbed_prime; i++) {
			const bool isp = Helper_IsPrime(i, 1);
			bool is_tabbed_prime = false;
			if(i && i < prime_tab_count) {
				SLCHECK_LT((long)p_first_prime_numbers[i-1], (long)p_first_prime_numbers[i]);
			}
			for(uint j = 0; !is_tabbed_prime && j < prime_tab_count; j++) {
				const ushort tv = p_first_prime_numbers[j];
				if(tv == i)
					is_tabbed_prime = true;
				else if(tv > i)
					break;
			}
			SLCHECK_EQ(is_tabbed_prime, isp);
			SLCHECK_EQ(Helper_IsPrime(i, 0), isp);
		}
		{
			struct TestSValue {
        		uint64 V;
        		uint32 S;
			} _test_row[] = {
        		{ 0ULL,                  1 },
        		{ 0x00000000000000ffULL, 1 },
        		{ 0x000000000000ffffULL, 2 },
        		{ 0x0000000000007f00ULL, 2 },
        		{ 0x0000000000ffffffULL, 3 },
        		{ 0x000000000070ff00ULL, 3 },
        		{ 0x00000000ffffffffULL, 4 },
        		{ 0x00000000af4f00ffULL, 4 },
        		{ 0x000000ffffffffffULL, 5 },
        		{ 0x0000003f00007b00ULL, 5 },
        		{ 0x0000ffffffffffffULL, 6 },
        		{ 0x00000ffff12fbeefULL, 6 },
        		{ 0x00ffffffffffffffULL, 7 },
        		{ 0x0070ff220001ff00ULL, 7 },
        		{ 0xffffffffffffffffULL, 8 },
        		{ 0x7000000000000000ULL, 8 },
        		{ 0x70af81b39ec62da5ULL, 8 }
			};
			uint8 buffer[32];
			uint32 sz;
			uint64 value;
			for(uint i = 0; i < SIZEOFARRAY(_test_row); i++) {
				memzero(buffer, sizeof(buffer));
				value = _test_row[i].V;
				sz = sshrinkuint64(value, buffer);
				SLCHECK_NZ(ismemzero(buffer+sz, sizeof(buffer)-sz));
				SLCHECK_EQ(sz, _test_row[i].S);
				SLCHECK_EQ(sexpanduint64(buffer, sz), value);
			}
		}
	}
	return CurrentStatus;
}
#if 0 // @v11.2.6 {
SLTEST_R(fpow10i)
{
	for(int i = -9; i < +9; i++) {
		SLCHECK_EQ(fpow10i(i), pow(10.0, i));
	}
	return CurrentStatus;
}

SLTEST_R(round)
{
	struct A {
		int    R;  // точность округления //
		double T;  // тестовое значение
		double E0; // ожидаемый результат функции round(x, 0)
		double E1; // ожидаемый результат функции roundnev(x, 0)
	};
	A a[] = {
		{0, 0., 0., 0.},
		{0, -1., -1., -1.},
		{0, -5.00000000103, -5., -5.},
		{0, -4.99999981, -5., -5.},
		{0, -2.5, -3., -2.},
		{0, -17.5, -18., -18.},
		{0, -1023.499999, -1023., -1023.},
		{0, 5.00000000103, 5., 5.},
		{0, 4.99999981, 5., 5.},
		{0, 2.5, 3., 2.},
		{0, 17.5, 18., 18.},
		{0, 1023.499999, 1023., 1023.},
		{2, 72.055, 72.06, 72.06 }
	};
	uint i;
	int  j;
	for(i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
		SLCHECK_EQ(round   (a[i].T, a[i].R), a[i].E0);
		SLCHECK_EQ(roundnev(a[i].T, a[i].R), a[i].E1);
	}
	for(j = -11; j < +13; j++) {
		double p_ = fpow10i(j);
		for(i = 0; i < sizeof(a) / sizeof(a[0]); i++) {
			if(a[i].R == 0) { // Для ненулевого начального округления возникает мелкая ошибка
				double v = a[i].T / p_;
				SLCHECK_EQ(round(v, j),    a[i].E0 / p_);
				SLCHECK_EQ(roundnev(v, j), a[i].E1 / p_);
			}
		}
	}
	if(CurrentStatus) {
		SRng * p_rng = SRng::CreateInstance(SRng::algMT, 0);
		for(i = 0; i < 1000; i++) {
			double b, v, r;
			for(j = -11; j < +13; j++) {
				b = p_rng->GetReal();
				if(i % 2)
					b = _chgsign(b);
				v = round(b, j);
				SLCHECK_LT(fabs(v - b), fpow10i(-j)/2.0);
				r = fabs(b * fpow10i(j));
				SLCHECK_CRANGE(fabs(v), floor(r)/fpow10i(j), ceil(r)/fpow10i(j));
			}
		}
		delete p_rng;
	}
	return CurrentStatus;
}

SLTEST_R(Prime)
{
	ulong last_tabbed_prime = FirstPrimeNumbers[SIZEOFARRAY(FirstPrimeNumbers)-1];
	for(ulong i = 0; i < last_tabbed_prime; i++) {
		const bool isp = Helper_IsPrime(i, 1);
		bool is_tabbed_prime = false;
		if(i && i < SIZEOFARRAY(FirstPrimeNumbers)) {
			SLCHECK_LT((long)FirstPrimeNumbers[i-1], (long)FirstPrimeNumbers[i]);
		}
		for(uint j = 0; !is_tabbed_prime && j < SIZEOFARRAY(FirstPrimeNumbers); j++) {
			const ushort tv = FirstPrimeNumbers[j];
			if(tv == i)
				is_tabbed_prime = true;
			else if(tv > i)
				break;
		}
		SLCHECK_EQ(is_tabbed_prime, isp);
		SLCHECK_EQ(Helper_IsPrime(i, 0), isp);
	}
	{
		struct TestSValue {
        	uint64 V;
        	uint32 S;
		} _test_row[] = {
        	{ 0ULL,                  1 },
        	{ 0x00000000000000ffULL, 1 },
        	{ 0x000000000000ffffULL, 2 },
        	{ 0x0000000000007f00ULL, 2 },
        	{ 0x0000000000ffffffULL, 3 },
        	{ 0x000000000070ff00ULL, 3 },
        	{ 0x00000000ffffffffULL, 4 },
        	{ 0x00000000af4f00ffULL, 4 },
        	{ 0x000000ffffffffffULL, 5 },
        	{ 0x0000003f00007b00ULL, 5 },
        	{ 0x0000ffffffffffffULL, 6 },
        	{ 0x00000ffff12fbeefULL, 6 },
        	{ 0x00ffffffffffffffULL, 7 },
        	{ 0x0070ff220001ff00ULL, 7 },
        	{ 0xffffffffffffffffULL, 8 },
        	{ 0x7000000000000000ULL, 8 },
        	{ 0x70af81b39ec62da5ULL, 8 }
		};
		uint8 buffer[32];
		uint32 sz;
		uint64 value;
		for(uint i = 0; i < SIZEOFARRAY(_test_row); i++) {
			memzero(buffer, sizeof(buffer));
			value = _test_row[i].V;
			sz = sshrinkuint64(value, buffer);
			SLCHECK_NZ(ismemzero(buffer+sz, sizeof(buffer)-sz));
			SLCHECK_EQ(sz, _test_row[i].S);
			SLCHECK_EQ(sexpanduint64(buffer, sz), value);
		}
	}
	return CurrentStatus;
}
#endif // } 0 @v11.2.6

#define TEST_TOL0  (2.0*SMathConst::Epsilon)
#define TEST_TOL1  (16.0*SMathConst::Epsilon)
#define TEST_TOL2  (256.0*SMathConst::Epsilon)
#define TEST_TOL3  (2048.0*SMathConst::Epsilon)
#define TEST_TOL4  (16384.0*SMathConst::Epsilon)
#define TEST_TOL5  (131072.0*SMathConst::Epsilon)
#define TEST_TOL6  (1048576.0*SMathConst::Epsilon)
#define TEST_SQRT_TOL0 (2.0*GSL_SQRT_DBL_EPSILON)
#define TEST_SNGL  (1.0e-06)

static int TestSf(STestCase * pCase, SMathResult & r, double val_in, double tol, int status, const char * desc)
{
	return BIN(pCase->_check_math_result(r, val_in, tol, desc) && pCase->_check_nz(status, desc));
}

static int TestSfSgn(STestCase * pCase, SMathResult & r, double sgn, double val_in, double tol, double expect_sgn, int status, const char * desc)
{
	SMathResult local_r;
	local_r.V = sgn;
	local_r.E = 0.0;
	return BIN(
		pCase->_check_math_result(r, val_in, tol, desc) &&
		pCase->_check_math_result(local_r, expect_sgn, 0.0, desc) &&
		pCase->_check_nz(status, desc));
}

#define TEST_SF(func, args, val_in, tol, expect_return) \
	{ int status = func args; TestSf(this, r, val_in, tol, status, "\n\t" #func #args); }
#define TEST_SF_SGN(stat, func, args, val_in, tol, expect_sgn, expect_return) \
	{ int status = func args; stat += TestSfSgn(this, r, sgn, val_in, tol, expect_sgn, status, "\n\t" #func #args); }
/*
#define TEST_SF_2(stat, func, args, val1, tol1, val2, tol2, expect_return) \
	{ int status = func args; stat += test_sf_2(r1, val1, tol1, r2, val2, tol2, status, expect_return, #func #args); }
*/

int flngamma_sgn(double x, SMathResult * result_lg, double * sgn);
int fgamma(double x, SMathResult * result);
int gammastar(double x, SMathResult * result);
int gamma_inc_P_e(double a, double x, SMathResult * result);
int gamma_inc_Q_e(const double a, const double x, SMathResult * result);
int gamma_inc_e(double a, double x, SMathResult * result);

SLTEST_R(SMathGamma)
{
	SMathResult r;
	//SMathResult r1, r2;
	double sgn;
	int    s = 0;
	{
		//
		// Тест линейной аппроксимации методом наименьших квадратов (LssLin)
		//
		static const double norris_x[] = { 
			  0.2, 337.4, 118.2, 884.6,  10.1, 226.5, 666.3, 996.3, 448.6, 777.0, 558.2,   0.4,  0.6, 775.5, 666.9, 338.0, 447.5, 11.6, 
			556.0, 228.1, 995.8, 887.6, 120.2,   0.3,   0.3, 556.8, 339.1, 887.2, 999.0, 779.0, 11.1, 118.3, 229.2, 669.1, 448.9,  0.5 
		};
		static const double norris_y[] = { 
			  0.1, 338.8, 118.1, 888.0,   9.2, 228.1, 668.5, 998.5, 449.1, 778.9, 559.2,   0.3,  0.1, 778.1, 668.8, 339.3, 448.9, 10.8, 
			557.7, 228.3, 998.0, 888.8, 119.6,   0.3,   0.6, 557.6, 339.3, 888.0, 998.5, 778.9, 10.2, 117.6, 228.9, 668.4, 449.2,  0.2
		};
		assert(SIZEOFARRAY(norris_x) == SIZEOFARRAY(norris_y));
		const size_t norris_n = /*36*/SIZEOFARRAY(norris_x);
		const double expected_c0 = -0.262323073774029;
		const double expected_c1 =  1.00211681802045; 
		const double expected_cov00 = pow(0.232818234301152, 2.0);
		const double expected_cov01 = -7.74327536339570e-05;  // computed from octave 
		const double expected_cov11 = pow(0.429796848199937E-03, 2.0);
		const double expected_sumsq = 26.6173985294224;
		{
			LssLin lss;
			lss.Solve_Simple(norris_n, norris_x, norris_y);
			SLCHECK_EQ_TOL(lss.A, expected_c0, 1e-10);
			SLCHECK_EQ_TOL(lss.B, expected_c1, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov00, expected_cov00, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov01, expected_cov01, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov11, expected_cov11, 1e-10);
			SLCHECK_EQ_TOL(lss.SumSq, expected_sumsq, 1e-10);
		}
		/*{
			LssLin lss;
			lss.Solve_SSE(norris_n, norris_x, norris_y);
			SLCHECK_EQ_TOL(lss.A, expected_c0, 1e-10);
			SLCHECK_EQ_TOL(lss.B, expected_c1, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov00, expected_cov00, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov01, expected_cov01, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov11, expected_cov11, 1e-10);
			SLCHECK_EQ_TOL(lss.SumSq, expected_sumsq, 1e-10);
		}*/
		{
			LVect nv_x;
			LVect nv_y;
			nv_x.init(norris_n, norris_x);
			nv_y.init(norris_n, norris_y);
			LssLin lss;
			lss.Solve(nv_x, nv_y);
			SLCHECK_EQ_TOL(lss.A, expected_c0, 1e-10);
			SLCHECK_EQ_TOL(lss.B, expected_c1, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov00, expected_cov00, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov01, expected_cov01, 1e-10);
			SLCHECK_EQ_TOL(lss.Cov11, expected_cov11, 1e-10);
			SLCHECK_EQ_TOL(lss.SumSq, expected_sumsq, 1e-10);
		}
	}
	TEST_SF(flngamma, (-0.1, &r),       2.368961332728788655,  TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-1.0/256.0, &r), 5.547444766967471595,  TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (1.0e-08, &r),    18.420680738180208905, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (0.1, &r),        2.252712651734205,     TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (1.0 + 1.0/256.0, &r), -0.0022422226599611501448, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (2.0 + 1.0/256.0, &r), 0.0016564177556961728692,  TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (100.0, &r),           359.1342053695753,         TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-1.0-1.0/65536.0, &r), 11.090348438090047844,    TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-1.0-1.0/268435456.0, &r), 19.408121054103474300, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-100.5, &r), -364.9009683094273518, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flngamma, (-100-1.0/65536.0, &r), -352.6490910117097874, TEST_TOL0, GSL_SUCCESS);

	TEST_SF_SGN(s, flngamma_sgn, (0.7, &r, &sgn), 0.26086724653166651439, TEST_TOL1, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (0.1, &r, &sgn), 2.2527126517342059599, TEST_TOL0, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-0.1, &r, &sgn), 2.368961332728788655, TEST_TOL0, -1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-1.0-1.0/65536.0, &r, &sgn), 11.090348438090047844, TEST_TOL0, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-2.0-1.0/256.0, &r, &sgn), 4.848447725860607213, TEST_TOL0, -1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-2.0-1.0/65536.0, &r, &sgn), 10.397193628164674967, TEST_TOL0, -1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-3.0-1.0/8.0, &r, &sgn), 0.15431112768404182427, TEST_TOL2, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, flngamma_sgn, (-100.5, &r, &sgn), -364.9009683094273518, TEST_TOL0, -1.0, GSL_SUCCESS);

	TEST_SF(fgamma, (1.0 + 1.0/4096.0, &r), 0.9998591371459403421, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (1.0 + 1.0/32.0, &r),   0.9829010992836269148, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (2.0 + 1.0/256.0, &r),  1.0016577903733583299, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (9.0, &r),              40320.0,               TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (10.0, &r),             362880.0,              TEST_TOL0, GSL_SUCCESS);
	// @v10.3.12 (не проходит тест и я не знаю почему) TEST_SF(fgamma, (100.0, &r),            9.332621544394415268e+155, TEST_TOL2, GSL_SUCCESS);
	// @v10.3.12 (не проходит тест и я не знаю почему) TEST_SF(fgamma, (170.0, &r),            4.269068009004705275e+304, TEST_TOL2, GSL_SUCCESS);
	// @v10.3.12 (не проходит тест и я не знаю почему) TEST_SF(fgamma, (171.0, &r),            7.257415615307998967e+306, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(fgamma, (-10.5, &r), -2.640121820547716316e-07, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fgamma, (-11.25, &r), 6.027393816261931672e-08, TEST_TOL0, GSL_SUCCESS); /* exp()... not my fault */
	TEST_SF(fgamma, (-1.0+1.0/65536.0, &r), -65536.42280587818970, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(gammastar, (1.0e-08, &r), 3989.423555759890865, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gammastar, (1.0e-05, &r), 126.17168469882690233, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (0.001, &r), 12.708492464364073506, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (1.5, &r), 1.0563442442685598666, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (3.0, &r), 1.0280645179187893045, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (9.0, &r), 1.0092984264218189715, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (11.0, &r), 1.0076024283104962850, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (100.0, &r), 1.0008336778720121418, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (1.0e+05, &r), 1.0000008333336805529, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammastar, (1.0e+20, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
#if 0 // {
	TEST_SF(gammainv_e, (1.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (2.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (3.0, &r), 0.5, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (4.0, &r), 1.0/6.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (10.0, &r), 1.0/362880.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (100.0, &r), 1.0715102881254669232e-156, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gammainv_e, (0.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-1.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-2.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-3.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-4.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-10.5, &r), -1.0/2.640121820547716316e-07, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-11.25, &r), 1.0/6.027393816261931672e-08, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gammainv_e, (-1.0+1.0/65536.0, &r), -1.0/65536.42280587818970, TEST_TOL1, GSL_SUCCESS);

	TEST_SF_2(s, lngamma_complex_e, (5.0, 2.0, &r1, &r2),
		2.7487017561338026749, TEST_TOL0,
		3.0738434100497007915, TEST_TOL0,
		GSL_SUCCESS);
	TEST_SF_2(s, lngamma_complex_e, (100.0, 100.0, &r1, &r2),
		315.07804459949331323, TEST_TOL1,
		2.0821801804113110099, TEST_TOL3,
		GSL_SUCCESS);
	TEST_SF_2(s, lngamma_complex_e, (100.0, -1000.0, &r1, &r2),
		-882.3920483010362817000, TEST_TOL1,
		-2.1169293725678813270, TEST_TOL3,
		GSL_SUCCESS);
	TEST_SF_2(s, lngamma_complex_e, (-100.0, -1.0, &r1, &r2),
		-365.0362469529239516000, TEST_TOL1,
		-3.0393820262864361140, TEST_TOL1,
		GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   1.0/1048576.0, &r), 1.7148961854776073928e-67 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   1.0/1024.0, &r), 2.1738891788497900281e-37 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   1.0, &r), 2.7557319223985890653e-07 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   5.0, &r), 2.6911444554673721340     , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (10,   500.0, &r), 2.6911444554673721340e+20 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (100,  100.0, &r), 1.0715102881254669232e+42 , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (1000, 200.0, &r), 2.6628790558154746898e-267, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(taylorcoeff_e, (1000, 500.0, &r), 2.3193170139740855074e+131, TEST_TOL1, GSL_SUCCESS);

	TEST_SF(fact_e, (0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fact_e, (1, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fact_e, (7, &r), 5040.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(fact_e, (33, &r), 8.683317618811886496e+36, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(doublefact_e, (0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(doublefact_e, (1, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(doublefact_e, (7, &r), 105.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(doublefact_e, (33, &r), 6.332659870762850625e+18, TEST_TOL0, GSL_SUCCESS);
#endif // } 0

	TEST_SF(flnfact, (0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flnfact, (1, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flnfact, (7, &r), 8.525161361065414300, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(flnfact, (33, &r), 85.05446701758151741, TEST_TOL0, GSL_SUCCESS);
#if 0 // {
	TEST_SF(lndoublefact_e, (0, &r), 0.0 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (7, &r), 4.653960350157523371 , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (33, &r), 43.292252022541719660, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (34, &r), 45.288575519655959140, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (1034, &r), 3075.6383796271197707, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lndoublefact_e, (1035, &r), 3078.8839081731809169, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(lnchoose_e, (7,3, &r), 3.555348061489413680, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnchoose_e, (5,2, &r), 2.302585092994045684, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(choose_e, (7,3, &r), 35.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (7,4, &r), 35.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (5,2, &r), 10.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (5,3, &r), 10.0, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(choose_e, (500,495, &r), 255244687600.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(choose_e, (500,5, &r), 255244687600.0, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(choose_e, (500,200, &r), 5.054949849935532221e+144, TEST_TOL5, GSL_SUCCESS);
	TEST_SF(choose_e, (500,300, &r), 5.054949849935532221e+144, TEST_TOL5, GSL_SUCCESS);

	TEST_SF(lnpoch_e, (5, 0.0, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (5, 1.0/65536.0, &r), 0.000022981557571259389129, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (5, 1.0/256.0, &r),   0.005884960217985189004,    TEST_TOL2, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (7,3, &r), 6.222576268071368616, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnpoch_e, (5,2, &r), 3.401197381662155375, TEST_TOL0, GSL_SUCCESS);

	TEST_SF_SGN(s, lnpoch_sgn_e, (5.0, 0.0, &r, &sgn), 0.0, TEST_TOL1, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, lnpoch_sgn_e, (-4.5, 0.25, &r, &sgn), 0.7430116475119920117, TEST_TOL1, 1.0, GSL_SUCCESS);
	TEST_SF_SGN(s, lnpoch_sgn_e, (-4.5, 1.25, &r, &sgn), 2.1899306304483174731, TEST_TOL1, -1.0, GSL_SUCCESS);

	TEST_SF(poch_e, (5, 0.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(poch_e, (7,3, &r), 504.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(poch_e, (5,2, &r), 30.0 , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(poch_e, (5,1.0/256.0, &r), 1.0059023106151364982, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(pochrel_e, (5,0, &r), 1.506117668431800472, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (7,3, &r), 503.0/3.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(pochrel_e, (5,2, &r), 29.0/2.0, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (5,0.01, &r), 1.5186393661368275330, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,0.01, &r), 1.8584945633829063516, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,-1.0/8.0, &r), 1.0883319303552135488, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,-1.0/256.0, &r), 1.7678268037726177453, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(pochrel_e, (-5.5,-11.0, &r), 0.09090909090939652475, TEST_TOL0, GSL_SUCCESS);
#endif // } 0

	TEST_SF(gamma_inc_P_e, (1e-100, 0.001, &r), 1.0, TEST_TOL0, GSL_SUCCESS) ;
	TEST_SF(gamma_inc_P_e, (0.001, 0.001, &r), 0.9936876467088602902, TEST_TOL0, GSL_SUCCESS) ;
	TEST_SF(gamma_inc_P_e, (0.001, 1.0, &r), 0.9997803916424144436, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (0.001, 10.0, &r), 0.9999999958306921828, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1.0, 0.001, &r), 0.0009995001666250083319, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1.0, 1.01, &r), 0.6357810204284766802, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1.0, 10.0, &r), 0.9999546000702375151, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (10.0, 10.01, &r), 0.5433207586693410570, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (10.0, 20.0, &r), 0.9950045876916924128, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1000.0, 1000.1, &r), 0.5054666401440661753, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (1000.0, 2000.0, &r), 1.0, TEST_TOL0, GSL_SUCCESS);
	/* Test for failure of the Gautschi recurrence (now fixed) for x = a - 2 */
	TEST_SF(gamma_inc_P_e, (34.0, 32.0, &r), 0.3849626436463866776322932129, TEST_TOL2, GSL_SUCCESS);
	/* and the next test is gamma_inc_P(37,35-20*eps) */
	TEST_SF(gamma_inc_P_e, (37.0, 3.499999999999999289e+01, &r), 0.3898035054195570860969333039, TEST_TOL2, GSL_SUCCESS);

	/* Regression test Martin Jansche <jansche@ling.ohio-state.edu> BUG#12 */
	TEST_SF(gamma_inc_P_e, (10, 1e-16, &r), 2.755731922398588814734648067e-167, TEST_TOL2, GSL_SUCCESS);

	TEST_SF(gamma_inc_Q_e, (0.0, 0.001, &r), 0.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 0.001, &r), 0.006312353291139709793, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 1.0, &r), 0.00021960835758555639171, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 2.0, &r), 0.00004897691783098147880, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (0.001, 5.0, &r), 1.1509813397308608541e-06, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0, 0.001, &r), 0.9990004998333749917, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0, 1.01, &r), 0.3642189795715233198, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0, 10.0, &r), 0.00004539992976248485154, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (10.0, 10.01, &r), 0.4566792413306589430, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (10.0, 100.0, &r), 1.1253473960842733885e-31, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1000.0, 1000.1, &r), 0.4945333598559338247, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1000.0, 2000.0, &r), 6.847349459614753180e-136, TEST_TOL2, GSL_SUCCESS);

	/* designed to trap the a-x=1 problem */
	TEST_SF(gamma_inc_Q_e, (100,  99.0, &r), 0.5266956696005394, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (200, 199.0, &r), 0.5188414119121281, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (100,  99.0, &r), 0.4733043303994607, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_P_e, (200, 199.0, &r), 0.4811585880878718, TEST_TOL2, GSL_SUCCESS);

	/* Test for x86 cancellation problems */
	TEST_SF(gamma_inc_P_e, (5670, 4574, &r),  3.063972328743934e-55, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (5670, 4574, &r), 1.0000000000000000, TEST_TOL2, GSL_SUCCESS);


	/* test suggested by Michel Lespinasse [gsl-discuss Sat, 13 Nov 2004] */
	TEST_SF(gamma_inc_Q_e, (1.0e+06-1.0, 1.0e+06-2.0, &r), 0.50026596175224547004, TEST_TOL3, GSL_SUCCESS);

	/* tests in asymptotic regime related to Lespinasse test */
	TEST_SF(gamma_inc_Q_e, (1.0e+06+2.0, 1.0e+06+1.0, &r), 0.50026596135330304336, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0e+06, 1.0e+06-2.0, &r), 0.50066490399940144811, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_Q_e, (1.0e+07, 1.0e+07-2.0, &r), 0.50021026104978614908, TEST_TOL2, GSL_SUCCESS);

	/* non-normalized "Q" function */
	TEST_SF(gamma_inc_e, (-1.0/1048576.0, 1.0/1048576.0, &r), 13.285819596290624271, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 1.0/1048576.0, &r), 13.381275128625328858, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   1.0/1048576.0, &r), 1.0485617142715768655e+06, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.00001,0.001, &r), 6.3317681434563592142, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.0001,0.001, &r), 6.3338276439767189385, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 0.001, &r), 6.3544709102510843793, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   0.001, &r), 59.763880515942196981, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   0.001, &r), 992.66896046923884234, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-3.5,   0.001, &r), 9.0224404490639003706e+09, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.5,  0.001, &r), 3.0083661558184815656e+30, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 0.1,   &r), 1.8249109609418620068, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   0.1,   &r), 3.4017693366916154163, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.0,  0.1,   &r), 8.9490757483586989181e+08, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.5,  0.1,   &r), 2.6967403834226421766e+09, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 1.0,   &r), 0.21928612679072766340, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   1.0,   &r), 0.17814771178156069019, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   1.0,   &r), 0.14849550677592204792, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-2.5,   1.0,   &r), 0.096556648631275160264, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   10.0,  &r), 3.8302404656316087616e-07, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.001, 10.0,  &r), 4.1470562324807320961e-06, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-0.5,   10.0,  &r), 1.2609042613241570681e-06, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-1.0,   10.0,  &r), 3.8302404656316087616e-07, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-10.5,  10.0,  &r), 6.8404927328441566785e-17, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-100.0, 10.0,  &r), 4.1238327669858313997e-107, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (-200.0, 10.0,  &r), 2.1614091830529343423e-207, TEST_TOL2, GSL_SUCCESS);

	TEST_SF(gamma_inc_e, (  0.0,     0.001, &r), 6.3315393641361493320, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.001,   0.001, &r), 6.3087159394864007261, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  1.0,     0.001, &r), 0.99900049983337499167, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, ( 10.0,     0.001, &r), 362880.0, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.0,     1.0,   &r), 0.21938393439552027368, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.001,   1.0,   &r), 0.21948181320730279613, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  1.0,     1.0,   &r), 0.36787944117144232160, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, ( 10.0,     1.0,   &r), 362879.95956592242045, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (100.0,     1.0,   &r), 9.3326215443944152682e+155, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.0,   100.0, &r), 3.6835977616820321802e-46, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  0.001, 100.0, &r), 3.7006367674063550631e-46, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (  1.0,   100.0, &r), 3.7200759760208359630e-44, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, ( 10.0,   100.0, &r), 4.0836606309106112723e-26, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(gamma_inc_e, (100.0,   100.0, &r), 4.5421981208626694294e+155, TEST_TOL1, GSL_SUCCESS);

#if 0 // {
	TEST_SF(lnbeta_e, (1.0e-8, 1.0e-8, &r),  19.113827924512310617, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 0.01, &r),  18.420681743788563403, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 1.0, &r),  18.420680743952365472, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 10.0, &r),  18.420680715662683009, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0e-8, 1000.0, &r),  18.420680669107656949, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 0.1, &r), 2.9813614810376273949, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 1.0, &r),  2.3025850929940456840, TEST_TOL1, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 100.0, &r),  1.7926462324527931217, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (0.1, 1000, &r),  1.5619821298353164928, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0, 1.00025, &r),  -0.0002499687552073570, TEST_TOL4, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0, 1.01, &r),  -0.009950330853168082848, TEST_TOL3, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (1.0, 1000.0, &r),  -6.907755278982137052, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (100.0, 100.0, &r),  -139.66525908670663927, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (100.0, 1000.0, &r),  -336.4348576477366051, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(lnbeta_e, (100.0, 1.0e+8, &r),  -1482.9339185256447309, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0,   1.0, &r), 1.0                  , TEST_TOL0, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0, 1.001, &r), 0.9990009990009990010, TEST_TOL0, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0,   5.0, &r), 0.2                  , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(beta_e, (1.0,  100.0, &r), 0.01                 , TEST_TOL1, GSL_SUCCESS);
	TEST_SF(beta_e, (10.0, 100.0, &r), 2.3455339739604649879e-15, TEST_TOL2, GSL_SUCCESS);

	/* Test negative arguments */
	TEST_SF(beta_e, (2.5, -0.1, &r), -11.43621278354402041480, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (2.5, -1.1, &r), 14.555179906328753255202, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-0.25, -0.1, &r), -13.238937960945229110, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-1.25, -0.1, &r), -14.298052997820847439, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, -99.1, &r), -1.005181917797644630375787297e60, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, 99.3, &r), 0.0004474258199579694011200969001, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (100.1, -99.3, &r), 1.328660939628876472028853747, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, 1.2, &r), 0.00365530364287960795444856281, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (100.1, -1.2, &r), 1203.895236907821059270698160, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.1, -1.2, &r), -3236.073671884748847700283841, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (-100.001, 0.0099, &r), -853.946649365611147996495177, TEST_TOL2, GSL_SUCCESS);

	/* Other test cases */
	TEST_SF(beta_e, (1e-32, 1.5, &r), 1e32, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_e, (1e-6, 0.5, &r), 1000001.386293677092419390336, TEST_TOL2, GSL_SUCCESS);

	TEST_SF(beta_e, (-1.5, 0.5, &r), 0.0, TEST_TOL0, GSL_SUCCESS);

	TEST_SF(beta_inc_e, (1.0, 1.0, 0.0, &r), 0.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (1.0, 1.0, 1.0, &r), 1.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (0.1, 0.1, 1.0, &r), 1.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  1.0, 0.5, &r), 0.5, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 0.1,  1.0, 0.5, &r), 0.9330329915368074160, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (10.0,  1.0, 0.5, &r), 0.0009765625000000000000, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (50.0,  1.0, 0.5, &r), 8.881784197001252323e-16, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  0.1, 0.5, &r), 0.06696700846319258402, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0, 10.0, 0.5, &r), 0.99902343750000000000, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0, 50.0, 0.5, &r), 0.99999999999999911180, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  1.0, 0.1, &r), 0.10, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  2.0, 0.1, &r), 0.19, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 1.0,  2.0, 0.9, &r), 0.99, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (50.0, 60.0, 0.5, &r), 0.8309072939016694143, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (90.0, 90.0, 0.5, &r), 0.5, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, ( 500.0,  500.0, 0.6, &r), 0.9999999999157549630, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (5000.0, 5000.0, 0.4, &r), 4.518543727260666383e-91, TEST_TOL5, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (5000.0, 5000.0, 0.6, &r), 1.0, TEST_TOL2, GSL_SUCCESS);
	TEST_SF(beta_inc_e, (5000.0, 2000.0, 0.6, &r), 8.445388773903332659e-89, TEST_TOL5, GSL_SUCCESS);
#endif // } 0
	return s;
}

SLTEST_R(DebtGammaProb)
{
	double eta = 328.0342;
	double K = 0.0813;
	double sum = 0.0;
	for(uint x = 1; x < 1000; x++) {
		double prob = (GammaIncompleteP(K, x/eta) - GammaIncompleteP(K, (x-1)/eta));
		sum += prob;
	}
	SLCHECK_EQ_TOL(sum, 1.0, 0.01);
	return CurrentStatus;
}

SLTEST_R(SDecimal)
{
	SString temp_buf;
	SLCHECK_EQ(SDecimal().ToStr(0, temp_buf), "0");
	SLCHECK_NZ(SDecimal() == SDecimal("0"));
	SLCHECK_EQ(SDecimal(1, 0).ToStr(0, temp_buf), "1");
	SLCHECK_NZ(SDecimal(1, 0) == SDecimal("1"));
	SLCHECK_EQ(SDecimal(7, -1).ToStr(0, temp_buf), "0.7");
	SLCHECK_NZ(SDecimal(7, -1) == SDecimal("0.7"));
	SLCHECK_EQ(SDecimal(127, -1).ToStr(0, temp_buf), "12.7");
	SLCHECK_NZ(SDecimal(127, -1) == SDecimal("\t 12.7"));
	SLCHECK_EQ(SDecimal(127, +1).ToStr(0, temp_buf), "1270");
	SLCHECK_NZ(SDecimal(127, +1) == SDecimal("1270"));
	SLCHECK_EQ(SDecimal(-127, +1).ToStr(0, temp_buf), "-1270");
	SLCHECK_NZ(SDecimal(-127, +1) == SDecimal("\t\t\t\t\t\t-1270  "));
	SLCHECK_EQ(SDecimal(-127, -5).ToStr(0, temp_buf), "-0.00127");
	SLCHECK_NZ(SDecimal(-127, -5) == SDecimal(temp_buf.Z().CatCharN(' ', 1024).Cat("-0.00127")));

	SLCHECK_NZ(SDecimal(127, -6) == SDecimal(" 000.000127"));
	SLCHECK_NZ(SDecimal(1234567890123456789, 0) == SDecimal("1234567890123456789"));

	SLCHECK_NZ(SDecimal(56, -6) == SDecimal("56e-6"));
	SLCHECK_NZ(SDecimal(561, -6) == SDecimal("56.1E-5"));
	{
		SDecimal r;
		SDecimal r2;
		r.Div(SDecimal(1, 0), SDecimal(4, 0));
		SLCHECK_EQ(r.ToStr(0, temp_buf), "0.25");
		SLCHECK_NZ(r2.FromStr(temp_buf));
		SLCHECK_NZ(r == r2);

		r.Div(SDecimal(1, 0), SDecimal(3, 0));
		r.ToStr(0, temp_buf);
		r.Div(SDecimal(1, 0), SDecimal(6, 0));
		r.ToStr(0, temp_buf);
		r2.Mul(r, SDecimal(6, 0));
		r2.ToStr(0, temp_buf);
	}
	{
		SRandGenerator & r_rng = SLS.GetTLA().Rg;
		TSStack <SDecimal> stk;
		SDecimal sum;
		double fsum = 0.0;
		for(uint i = 0; i < 100000; i++) {
			uint32 m32 = r_rng.GetUniformIntPos(INT32_MAX);
			int exp = -static_cast<int>(log10i_floor(m32));
			int64 mant = (i & 1) ? -static_cast<int32>(m32) : static_cast<int32>(m32);
			SDecimal r(mant, exp);
			stk.push(r);
			sum.Add(sum, r);
			//
			fsum += (double)mant * fpow10i(exp);
		}
		{
			SDecimal decr;
			while(stk.pop(decr)) {
				sum.Sub(sum, decr);
				//
				fsum -= decr.GetReal();
			}
			SLCHECK_NZ(sum.IsZero());
		}
	}
	{
		struct TestEntry {
			int64  N;
			int16  Dp;
			const char * P_Txt;
		};
		const TestEntry entries[] = {
			{ 17171717171717LL, -7, "1717171.7171717" },
			{ 1LL, 0, "1" },
			{ 3LL, 0, "3" },
			{ 1LL, -1, "0.1" },
			{ 1LL, -2, "0.01" },
			{ 5LL, -1, "0.5" },
			{ 3LL, -1, "0.3" },
			{ 71LL, -2, "0.71" },
			{ 3333333333333LL,   -12, "3.333333333333" },
			{ 6666666666666LL,   -12, "6.666666666666" },
			{ 777777777777777LL, -14, "7.77777777777777" },
			{ 1LL, +1, "10" },
			{ 1LL, +2, "100" },
			{ 5LL, +1, "50" },
			{ 0LL, 0, "0" },
			{ 314159265359LL, -11, "3.14159265359" },
		};
		SLCHECK_NZ(SDecimal().IsZero());
		SLCHECK_NZ(!SDecimal(1, 1).IsZero());
		for(uint i = 0; i < SIZEOFARRAY(entries); i++) {
			const TestEntry & r_te = entries[i];
			SLCHECK_EQ(SDecimal(r_te.N, r_te.Dp).ToStr(0, temp_buf), r_te.P_Txt);
			SLCHECK_NZ(SDecimal(r_te.P_Txt) == SDecimal(r_te.N, r_te.Dp));
			{
				SDecimal r(r_te.N, r_te.Dp);
				{
					uint64 ued = r.ToUed_NonConst(48);
					SDecimal r2;
					r2.FromUed(ued, 48);
					SLCHECK_NZ(r.IsEq(r2));
				}
			}
		}
		{
			SDecimal r;
			SDecimal r2;
			r.Add(SDecimal(0, 0), SDecimal(1, 0));
			SLCHECK_EQ(r.GetReal(), 1.0);
			r2.Sub(r, SDecimal(1, 0));
			SLCHECK_EQ(r2.GetReal(), 0.0);
			SLCHECK_NZ(r2.IsZero());
			//
			r.Add(SDecimal(0, 0), SDecimal(1, -5));
			SLCHECK_EQ(r.GetReal(), 0.00001);
			r2.Sub(r, SDecimal(1, -5));
			SLCHECK_EQ(r2.GetReal(), 0.0);
			SLCHECK_NZ(r2.IsZero());
			//
			r.Add(SDecimal(1, 0), SDecimal(1, -1));
			SLCHECK_EQ(r.GetReal(), 1.1);
			r2.Sub(r, SDecimal(1, -1));
			SLCHECK_EQ(r2.GetReal(), 1.0);
			//
			r.Add(SDecimal(703, 6), SDecimal(1, -1));
			SLCHECK_EQ(r.GetReal(), 703000000.1);
			r2.Sub(r, SDecimal(1, -1));
			SLCHECK_EQ(r2.GetReal(), 703000000.0);
			//
			r.Mul(SDecimal(1, 0), SDecimal(17, -3));
			SLCHECK_EQ(r.GetReal(), 0.017);
			//
			r.Mul(SDecimal(2, 0), SDecimal(17, -3));
			SLCHECK_EQ(r.GetReal(), 0.034);
			//
			r.Mul(SDecimal(1, 1), SDecimal(17, -3));
			SLCHECK_EQ(r.GetReal(), 0.17);
			//
			{
				r.Z();
				for(uint i = 0; i < 1000; i++) {
					r.Add(r, SDecimal(1, -6));
				}
				SLCHECK_EQ(r.GetReal(), 0.001);
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(STimeSeries)
{
	int    ok = 1;
	SString temp_buf;
	SString src_file_name;
	SString test_file_name;
	SLS.QueryPath("testroot", src_file_name);
	src_file_name.SetLastSlash().Cat("data").SetLastSlash().Cat("ts-eurusd.csv");
	SLS.QueryPath("testroot", test_file_name);
	test_file_name.SetLastSlash().Cat("out").SetLastSlash().Cat("ts-eurusd.out");
	SFile f_in(src_file_name, SFile::mRead);
	if(f_in.IsValid()) {
		SString line_buf;
		StringSet ss_in(",");
		STimeSeries ts;

		LDATETIME dtm;
		double open = 0.0;
		double close = 0.0;
		long   tick_vol = 0;
		long   real_vol = 0;
		long   spread = 0;

		uint   vecidx_open = 0;
		uint   vecidx_close = 0;
		uint   vecidx_ticvol = 0;
		uint   vecidx_realvol = 0;
		uint   vecidx_spread = 0;
		//THROW(ts.AddValueVec("open", T_DOUBLE, 0, &vecidx_open));
		THROW(SLCHECK_NZ(ts.AddValueVec("open", T_INT32, 5, &vecidx_open)));
		//THROW(ts.AddValueVec("close", T_DOUBLE, 0, &vecidx_close));
		THROW(SLCHECK_NZ(ts.AddValueVec("close", T_INT32, 5, &vecidx_close)));
		THROW(SLCHECK_NZ(ts.AddValueVec("tick_volume", T_INT32, 0, &vecidx_ticvol)));
		THROW(SLCHECK_NZ(ts.AddValueVec("real_volume", T_INT32, 0, &vecidx_realvol)));
		THROW(SLCHECK_NZ(ts.AddValueVec("spread", T_INT32, 0, &vecidx_spread)));
		{
			uint8 sign[8];
			size_t actual_size = 0;
			if(f_in.Read(sign, 4, &actual_size) && actual_size == 4) {
				if(sign[0] == 0xEF && sign[1] == 0xBB && sign[2] == 0xBF)
					f_in.Seek(3);
				else
					f_in.Seek(0);
			}
		}
		while(f_in.ReadLine(line_buf)) {
			line_buf.Chomp().Strip();
			if(line_buf.NotEmpty()) {
				ss_in.setBuf(line_buf);
				dtm.Z();
				open = 0.0;
				close = 0.0;
				tick_vol = 0;
				real_vol = 0;
				spread = 0;
				for(uint ssp = 0, fldn = 0; ss_in.get(&ssp, temp_buf); fldn++) {
					switch(fldn) {
						case 0: strtodate(temp_buf, DATF_YMD, &dtm.d); break;
						case 1: strtotime(temp_buf, TIMF_HMS, &dtm.t); break;
						case 2: open = temp_buf.ToReal(); break;
						case 3: close = temp_buf.ToReal(); break;
						case 4: tick_vol = temp_buf.ToLong(); break;
						case 5: real_vol = temp_buf.ToLong(); break;
						case 6: spread = temp_buf.ToLong(); break;
					}
				}
				if(checkdate(&dtm) && close > 0.0) {
					SUniTime ut;
					ut.Set(dtm, SUniTime::indMin);
					uint   item_idx = 0;
					THROW(SLCHECK_NZ(ts.AddItem(ut, &item_idx)));
					THROW(SLCHECK_NZ(ts.SetValue(item_idx, vecidx_open, open)));
					THROW(SLCHECK_NZ(ts.SetValue(item_idx, vecidx_close, close)));
					THROW(SLCHECK_NZ(ts.SetValue(item_idx, vecidx_ticvol, tick_vol)));
					THROW(SLCHECK_NZ(ts.SetValue(item_idx, vecidx_realvol, real_vol)));
					THROW(SLCHECK_NZ(ts.SetValue(item_idx, vecidx_spread, spread)));
				}
			}
		}
		{
			//
			STimeSeries dts;
			SBuffer sbuf; // serialize buf
			SBuffer cbuf; // compress buf
			SBuffer dbuf; // decompress buf
			SSerializeContext sctx;
			THROW(SLCHECK_NZ(ts.Serialize(+1, sbuf, &sctx)));
			{
				{
					SCompressor c(SCompressor::tZLib);
					THROW(SLCHECK_NZ(c.CompressBlock(sbuf.GetBuf(sbuf.GetRdOffs()), sbuf.GetAvailableSize(), cbuf, 0, 0)));
				}
				{
					SCompressor c(SCompressor::tZLib);
					THROW(SLCHECK_NZ(c.DecompressBlock(cbuf.GetBuf(cbuf.GetRdOffs()), cbuf.GetAvailableSize(), dbuf)));
				}
				SLCHECK_EQ(sbuf.GetAvailableSize(), dbuf.GetAvailableSize());
				SLCHECK_Z(memcmp(sbuf.GetBuf(sbuf.GetRdOffs()), dbuf.GetBuf(dbuf.GetRdOffs()), sbuf.GetAvailableSize()));
				THROW(SLCHECK_NZ(dts.Serialize(-1, dbuf, &sctx)));
			}
			{
				SFile f_out(test_file_name, SFile::mWrite);
				THROW(SLCHECK_NZ(f_out.IsValid()));
				for(uint i = 0; i < dts.GetCount(); i++) {
					SUniTime ut;
					dts.GetTime(i, &ut);
					ut.Get(dtm);
					THROW(SLCHECK_NZ(dts.GetValue(i, vecidx_open, &open)));
					THROW(SLCHECK_NZ(dts.GetValue(i, vecidx_close, &close)));
					THROW(SLCHECK_NZ(dts.GetValue(i, vecidx_ticvol, &tick_vol)));
					THROW(SLCHECK_NZ(dts.GetValue(i, vecidx_realvol, &real_vol)));
					THROW(SLCHECK_NZ(dts.GetValue(i, vecidx_spread, &spread)));
					line_buf.Z().Cat(dtm.d, DATF_ANSI|DATF_CENTURY).Comma().Cat(dtm.t, TIMF_HM).Comma().
						Cat(open, MKSFMTD(0, 5, 0)).Comma().Cat(close, MKSFMTD(0, 5, 0)).Comma().Cat(tick_vol).Comma().Cat(real_vol).Comma().Cat(spread).CR();
					THROW(SLCHECK_NZ(f_out.WriteLine(line_buf)));
				}
				f_out.Close();
				SLCHECK_LT(0, SFile::Compare(src_file_name, test_file_name, 0));
			}
		}
	}
	CATCHZOK
	return ok;
}
