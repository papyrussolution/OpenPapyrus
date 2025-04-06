// TEST-HASH.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025
//
#include <pp.h>
#pragma hdrstop
#include <xxhash.h>

#ifdef TEST_ZLIB_IMPLEMENTATION
	#include <zlib.h>
#endif
#define BOBJEN_HASHSTATE 1
#define BOBJEN_HASHLEN   1
#define BOBJEN_MAXPAIR  60
#define BOBJEN_MAXLEN   70
//
// Декларации для тестирования идентичности lzma-варианта (для замещения его собственным)
//
struct lzma_sha256_state {
	uint32 state[8]; /// Internal state
	uint64 size; /// Size of the message excluding padding
};

struct lzma_check_state {
	union {
		uint8  u8[64];
		uint32 u32[16];
		uint64 u64[8];
	} buffer;
	union {
		uint32 crc32;
		uint64 crc64;
		lzma_sha256_state sha256;
	} state;
};

static void __lzma_sha256_init(lzma_check_state * check);
static void __lzma_sha256_update(const uint8 * buf, size_t size, lzma_check_state * check);
static void __lzma_sha256_finish(lzma_check_state * check);
//
// }
//
//#define DISPLAY(...) slfprintf_stderr(__VA_ARGS__)
//#define DISPLAYLEVEL(l, ...) do { if(g_displayLevel>=l) DISPLAY(__VA_ARGS__); } while(0)
//static int g_displayLevel = 2;

static int Helper_Test_Crypto_Vec(STestCase & rCase, const SString & rInFileName, const char * pSetName, int alg, int kbl, int algmod)
{
	int    ok = 1;
	TSCollection <BdtTestItem> data_set;
	STempBuffer result_buf(SKILOBYTE(512));
	{
		BdtTestItem::ReadBdtTestData(rInFileName, pSetName, data_set);
		SlCrypto cs(alg, kbl, algmod);
		SlCrypto::Key key;
		for(uint i = 0; i < data_set.getCount(); i++) {
			const BdtTestItem * p_item = data_set.at(i);
			const size_t src_size = p_item->In.GetLen();
			const void * p_src_buf = p_item->In.GetBufC();
			const size_t key_size = p_item->Key.GetLen();
			const void * p_key_buf = p_item->Key.GetBufC();
			const size_t pattern_size = p_item->Out.GetLen();
			const void * p_pattern_buf = p_item->Out.GetBufC();
			size_t total_size = 0;
			rCase.SLCHECK_NZ(cs.SetupKey(key, p_key_buf, key_size));
			size_t actual_size = 0;
			rCase.SLCHECK_NZ(cs.Encrypt_(&key, p_src_buf, src_size, result_buf.vptr(total_size), result_buf.GetSize()-total_size, &actual_size));
			total_size += actual_size;
			rCase.SLCHECK_LE(static_cast<ulong>(pattern_size), static_cast<ulong>(total_size));
			rCase.SLCHECK_Z(memcmp(result_buf.vptr(), p_pattern_buf, pattern_size));
		}
		{
			const char * p_password = "test_crypto_password";
			const size_t pattern_buf_size = SKILOBYTE(4096);
			const size_t pattern_work_size = pattern_buf_size /*- 13*/;
			SlCrypto::Key key;
			size_t total_encr_size = 0;
			size_t work_offs = 0;
			size_t total_decr_size = 0;
			size_t actual_size = 0;
			STempBuffer dest_buf(pattern_buf_size + SKILOBYTE(512)); // @v11.1.12 (+ SKILOBYTE(512)) из-за 'padding' результат может быть длиннее, чем оригинал
			STempBuffer pattern_buf(pattern_buf_size);
			STempBuffer result_buf(pattern_buf_size + SKILOBYTE(512)); // with ensuring
			SObfuscateBuffer(result_buf.vptr(), result_buf.GetSize());
			SObfuscateBuffer(pattern_buf.vptr(), pattern_buf.GetSize());
			{
				SlCrypto cs(alg, kbl, algmod);
				rCase.SLCHECK_NZ(cs.SetupKey(key, p_password));
				work_offs = 0;
				actual_size = 0;
				rCase.SLCHECK_NZ(cs.Encrypt_(&key, pattern_buf.vptr(total_encr_size), pattern_work_size, result_buf.vptr(work_offs), result_buf.GetSize()-work_offs, &actual_size));
				work_offs += actual_size;
				total_encr_size += actual_size; // @v11.1.12 @fix pattern_work_size-->actual_size
			}
			{
				SlCrypto cs(alg, kbl, algmod);
				rCase.SLCHECK_NZ(cs.SetupKey(key, p_password));
				work_offs = 0;
				actual_size = 0;
				rCase.SLCHECK_NZ(cs.Decrypt_(&key, result_buf.vptr(total_decr_size), total_encr_size, dest_buf.vptr(work_offs), dest_buf.GetSize()-work_offs, &actual_size));
				work_offs += actual_size;
				total_decr_size += total_encr_size;
			}
			rCase.SLCHECK_Z(memcmp(dest_buf.vcptr(), pattern_buf.vcptr(), pattern_work_size));
		}
	}
	return ok;
}

template <typename T> void TestSlHashFunc(STestCase & rTc, T (STDCALL * hashFunc)(SlHash::State * pS, const void * pData, size_t dataLen),
	const SString & rDataTrasformPath, const char * pTestVecFileName, const char * pTestVec)
{
	SString in_file_name;
	TSCollection <BdtTestItem> data_set;
	(in_file_name = rDataTrasformPath).Cat(pTestVecFileName);
	data_set.freeAll();
	BdtTestItem::ReadBdtTestData(in_file_name, pTestVec, data_set);
	for(uint i = 0; i < data_set.getCount(); i++) {
		const BdtTestItem * p_item = data_set.at(i);
		const size_t src_size = p_item->In.GetLen();
		const void * p_src_buf = p_item->In.GetBufC();
		const uint8 * p_pattern_buf = static_cast<const uint8 *>(p_item->Out.GetBufC());
		const T s1 = hashFunc(0, p_src_buf, src_size);
		rTc.SLCHECK_Z(memcmp(&s1, p_pattern_buf, sizeof(s1)));
		if(src_size > 10) {
			SlHash::State st;
			size_t total_sz = 0;
			uint32 r = 0;
			for(size_t ps = 1; total_sz < src_size; ps++) {
				hashFunc(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
				total_sz += ps;
			}
			T s2 = hashFunc(&st, 0, 0); // finalize
			rTc.SLCHECK_Z(memcmp(&s2, p_pattern_buf, sizeof(s2)));
			rTc.SLCHECK_Z(memcmp(&s1, &s2, sizeof(s2)));
		}
	}
}

SLTEST_R(BDT)
{
	int    ok = 1;
#ifdef _LOCAL_USE_SSL // {
	OpenSSL_add_all_ciphers();
#endif
	{
		SString data_trasform_path;
		SLS.QueryPath("testroot", data_trasform_path);
		data_trasform_path.SetLastDSlash().Cat("data/DataTransform").SetLastDSlash();
		SString in_file_name;
		TSCollection <BdtTestItem> data_set;
		{
			(in_file_name = data_trasform_path).Cat("aes.vec");
			Helper_Test_Crypto_Vec(*this, in_file_name, "AES-128", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
			Helper_Test_Crypto_Vec(*this, in_file_name, "AES-192", SlCrypto::algAes, SlCrypto::kbl192, SlCrypto::algmodEcb);
			Helper_Test_Crypto_Vec(*this, in_file_name, "AES-256", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodEcb);
		}
		// @v12.3.1 {
		{
			{
				const char * p_data = "123456789";
				uint64 h = SlHash::CRC64(0, p_data, sstrlen(p_data));
				SLCHECK_EQ(h, 0xe9c6d914c4b8d9caULL);
			}
			{
				const char * p_data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed "
						"do eiusmod tempor incididunt ut labore et dolore magna "
						"aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
						"ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis "
						"aute irure dolor in reprehenderit in voluptate velit esse "
						"cillum dolore eu fugiat nulla pariatur. Excepteur sint "
						"occaecat cupidatat non proident, sunt in culpa qui officia "
						"deserunt mollit anim id est laborum.";
				uint64 h = SlHash::CRC64(0, p_data, sstrlen(p_data)+1); // +1 потому что тестовый случай включает завершающий '\0'
				SLCHECK_EQ(h, 0xc7794709e69683b3ULL);
			}
		}
		// } @v12.3.1 
		{
			(in_file_name = data_trasform_path).Cat("crc24.vec");
			data_set.freeAll();
			BdtTestItem::ReadBdtTestData(in_file_name, "CRC24", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const uint32 pattern_value = PTR32C(p_item->Out.GetBufC())[0] & 0x00ffffff;
				const void * p_src_buf = p_item->In.GetBufC();
				SLCHECK_EQ(SlHash::CRC24(0, p_src_buf, src_size), pattern_value);
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::CRC24(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					SlHash::CRC24(&st, 0, 0); // finalize
					SLCHECK_EQ(st.GetResult32(), pattern_value);
				}
			}
		}
		{
			(in_file_name = data_trasform_path).Cat("crc32.vec");
			data_set.freeAll();
			BdtTestItem::ReadBdtTestData(in_file_name, "CRC32", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const void * p_src_buf = p_item->In.GetBufC();
				uint32 pattern_value = sbswap32(PTR32C(p_item->Out.GetBufC())[0]);
				SLCHECK_EQ(SlHash::CRC32(0, p_src_buf, src_size), pattern_value);
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::CRC32(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					SlHash::CRC32(&st, 0, 0); // finalize
					SLCHECK_EQ(st.GetResult32(), pattern_value);
				}
			}
		}
		{
			(in_file_name = data_trasform_path).Cat("adler32.vec");
			data_set.freeAll();
			BdtTestItem::ReadBdtTestData(in_file_name, "Adler32", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const void * p_src_buf = p_item->In.GetBufC();
				uint32 pattern_value = sbswap32(PTR32C(p_item->Out.GetBufC())[0]);
				SLCHECK_EQ(SlHash::Adler32(0, p_src_buf, src_size), pattern_value);
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::Adler32(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					SlHash::Adler32(&st, 0, 0); // finalize
					SLCHECK_EQ(st.GetResult32(), pattern_value);
				}
			}
		}
		{
			//
			// С алгоритмами adler32 и crc32 не получается использовать TestSlHashFunc из-за того,
			// что тестовые векторы хранят результаты в перевернутом виде (see above)
			//
			TestSlHashFunc<binary128>(*this, SlHash::Md5,     data_trasform_path, "md5.vec", "MD5");
			TestSlHashFunc<binary160>(*this, SlHash::Sha1,    data_trasform_path, "sha1.vec", "SHA-160");
			TestSlHashFunc<binary256>(*this, SlHash::Sha256,  data_trasform_path, "sha2_32.vec", "SHA-256");
			TestSlHashFunc<binary512>(*this, SlHash::Sha512,  data_trasform_path, "sha2_64.vec", "SHA-512");
		}
		/*{ // MD5
			(in_file_name = data_trasform_path).Cat("md5.vec");
			data_set.freeAll();
			ReadBdtTestData(in_file_name, "MD5", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const void * p_src_buf = p_item->In.GetBufC();
				//uint8 result_buf[128];
				const uint8 * p_pattern_buf = static_cast<const uint8 *>(p_item->Out.GetBufC());
				{
					binary128 md5 = SlHash::Md5(0, p_src_buf, src_size);
					SLCHECK_Z(memcmp(&md5, p_pattern_buf, sizeof(md5)));
				}
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					uint32 r = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::Md5(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					binary128 md5 = SlHash::Md5(&st, 0, 0); // finalize
					SLCHECK_Z(memcmp(&md5, p_pattern_buf, sizeof(md5)));
				}
			}
		}*/
		/*{ // SHA-1
			(in_file_name = data_trasform_path).Cat("sha1.vec");
			data_set.freeAll();
			ReadBdtTestData(in_file_name, "SHA-160", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const void * p_src_buf = p_item->In.GetBufC();
				//uint8 result_buf[128];
				const uint8 * p_pattern_buf = static_cast<const uint8 *>(p_item->Out.GetBufC());
				{
					binary160 s1 = SlHash::Sha1(0, p_src_buf, src_size);
					SLCHECK_Z(memcmp(&s1, p_pattern_buf, 20));
				}
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					uint32 r = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::Sha1(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					binary160 s1 = SlHash::Sha1(&st, 0, 0); // finalize
					SLCHECK_Z(memcmp(&s1, p_pattern_buf, 20));
				}
			}
		}*/
		{ // SHA-1 Дополнительный тест по образцам библиотеки jbig2dec для элиминации локальной реализации
			static const char * jbig2dec_test_data[] = {
				"abc",
				"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
				"A million repetitions of 'a'"
			};
			static const char * jbig2dec_test_results[] = {
				"A9993E364706816ABA3E25717850C26C9CD0D89D",
				"84983E441C3BD26EBAAE4AA1F95129E5E54670F1",
				"34AA973CD4C4DAA4F61EEB2BDBAD27316534016F"
			};	
			binary160 s1;
			SString hex;
			assert(SIZEOFARRAY(jbig2dec_test_data) == SIZEOFARRAY(jbig2dec_test_results));
			for(uint i = 0; i < SIZEOFARRAY(jbig2dec_test_data); i++) {
				if(i == 2) { // A million repetitions of 'a'
					SlHash::State st;
					for(uint j = 0; j < 1000000; j++)
						SlHash::Sha1(&st, &"a", 1);
					s1 = SlHash::Sha1(&st, 0, 0); // finalize
				}
				else {
					const char * p_data = jbig2dec_test_data[i];
					s1 = SlHash::Sha1(0, p_data, strlen(p_data));
				}
				hex.Z();
				for(uint j = 0; j < sizeof(s1); j++)
					hex.CatHexUpper(reinterpret_cast<const uint8 *>(&s1)[j]);
				{
					//char hex_outp[256];
					//InnerBlock::digest_to_hex(reinterpret_cast<const uint8 *>(&s1), hex_outp);
					//SLCHECK_NZ(sstreq(jbig2dec_test_results[i], hex_outp));
				}
				SLCHECK_NZ(hex.IsEq(jbig2dec_test_results[i]));
			}
		}
		{
			//static void __lzma_sha256_init(lzma_check_state * check);
			//static void __lzma_sha256_update(const uint8 * buf, size_t size, lzma_check_state * check);
			//static void __lzma_sha256_finish(lzma_check_state * check);
			SBinaryChunk bc;
			for(uint i = 0; i < 100; i++) {
				const size_t len = SLS.GetTLA().Rg.GetUniformInt(SMEGABYTE(8));
				bc.Randomize(len);
				assert(bc.Len() == len);
				binary256 h1 = SlHash::Sha256(0, bc.PtrC(), bc.Len());
				{
					lzma_check_state lzmas;
					__lzma_sha256_init(&lzmas);
					__lzma_sha256_update(PTR8C(bc.PtrC()), bc.Len(), &lzmas);
					__lzma_sha256_finish(&lzmas);
					SLCHECK_Z(memcmp(&h1, &lzmas.buffer, sizeof(h1)));
				}
			}
		}
		/*{ // SHA-256
			(in_file_name = data_trasform_path).Cat("sha2_32.vec");
			data_set.freeAll();
			ReadBdtTestData(in_file_name, "SHA-256", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const void * p_src_buf = p_item->In.GetBufC();
				const uint8 * p_pattern_buf = static_cast<const uint8 *>(p_item->Out.GetBufC());
				const binary256 s1 = SlHash::Sha256(0, p_src_buf, src_size);
				SLCHECK_Z(memcmp(&s1, p_pattern_buf, sizeof(s1)));
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					uint32 r = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::Sha256(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					binary256 s2 = SlHash::Sha256(&st, 0, 0); // finalize
					SLCHECK_Z(memcmp(&s2, p_pattern_buf, sizeof(s2)));
					SLCHECK_Z(memcmp(&s1, &s2, sizeof(s2)));
				}
			}
		}*/
		/*{ // SHA-512
			(in_file_name = data_trasform_path).Cat("sha2_64.vec");
			data_set.freeAll();
			ReadBdtTestData(in_file_name, "SHA-512", data_set);
			for(uint i = 0; i < data_set.getCount(); i++) {
				const BdtTestItem * p_item = data_set.at(i);
				const size_t src_size = p_item->In.GetLen();
				const void * p_src_buf = p_item->In.GetBufC();
				const uint8 * p_pattern_buf = static_cast<const uint8 *>(p_item->Out.GetBufC());
				const binary512 s1 = SlHash::Sha512(0, p_src_buf, src_size);
				SLCHECK_Z(memcmp(&s1, p_pattern_buf, sizeof(s1)));
				if(src_size > 10) {
					SlHash::State st;
					size_t total_sz = 0;
					uint32 r = 0;
					for(size_t ps = 1; total_sz < src_size; ps++) {
						SlHash::Sha512(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
						total_sz += ps;
					}
					binary512 s2 = SlHash::Sha512(&st, 0, 0); // finalize
					SLCHECK_Z(memcmp(&s2, p_pattern_buf, sizeof(s2)));
					SLCHECK_Z(memcmp(&s1, &s2, sizeof(s2)));
				}
			}
		}*/
	}
	{
		// CRC32
		SString in_file_name(MakeInputFilePath("crc32.vec"));
		TSCollection <BdtTestItem> data_set;
		THROW(SLCHECK_NZ(BdtTestItem::ReadBdtTestData(in_file_name, "CRC32", data_set)));
		{
			SBdtFunct fu(SBdtFunct::Crc32);
			for(uint i = 0; i < data_set.getCount(); i++) {
				BdtTestItem * p_item = data_set.at(i);
				if(p_item) {
					SLCHECK_EQ(p_item->Out.GetLen(), sizeof(uint32));
					uint32 pattern_value = SMem::GetBe32(p_item->Out.GetBuf());
					//uint32 pattern_value = PTR32(p_item->Out.GetBuf())[0];
					//PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
					//PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
					//PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
					//
					{
						uint32 result;
						size_t out_offs = 0;
						size_t _o;
						THROW(SLCHECK_NZ(fu.Init()));
						THROW(SLCHECK_NZ(fu.Update(p_item->In.GetBuf(), p_item->In.GetLen(), &result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLCHECK_NZ(fu.Finish(&result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLCHECK_EQ(out_offs, sizeof(result)));
						SLCHECK_Z(memcmp(&result, &pattern_value, p_item->Out.GetLen()));
					}
					// @v11.9.0 {
					{
						uint32 result = SlHash::CRC32(0, p_item->In.GetBuf(), p_item->In.GetLen());
						SLCHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
					// } @v11.9.0 
					{
						//
						// Проверка старой реализации (которой всю жизнь пользуемся)
						//
						SCRC32 cc;
						uint32 result = cc.Calc(0, p_item->In.GetBuf(), p_item->In.GetLen());
						SLCHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
#ifdef TEST_ZLIB_IMPLEMENTATION
					{
						//
						// Проверка реализации zlib
						//
						uint32 result = crc32_z(0, (const uint8 *)p_item->In.GetBuf(), p_item->In.GetLen());
						SLCHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
#endif
				}
			}
		}
	}
	{
		// ADLER32
		SString in_file_name(MakeInputFilePath("adler32.vec"));
		TSCollection <BdtTestItem> data_set;
		THROW(SLCHECK_NZ(BdtTestItem::ReadBdtTestData(in_file_name, "Adler32", data_set)));
		{
			SBdtFunct fu(SBdtFunct::Adler32);
			for(uint i = 0; i < data_set.getCount(); i++) {
				BdtTestItem * p_item = data_set.at(i);
				if(p_item) {
					SLCHECK_EQ(p_item->Out.GetLen(), sizeof(uint32));
					uint32 pattern_value = SMem::GetBe32(p_item->Out.GetBuf());
					//uint32 pattern_value = PTR32(p_item->Out.GetBuf())[0];
					//PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
					//PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
					//PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
					//
					{
						uint32 result;
						size_t out_offs = 0;
						size_t _o;
						THROW(SLCHECK_NZ(fu.Init()));
						THROW(SLCHECK_NZ(fu.Update(p_item->In.GetBuf(), p_item->In.GetLen(), &result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLCHECK_NZ(fu.Finish(&result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLCHECK_EQ(out_offs, sizeof(result)));
						SLCHECK_Z(memcmp(&result, &pattern_value, p_item->Out.GetLen()));
					}
#ifdef TEST_ZLIB_IMPLEMENTATION
					{
						//
						// Проверка реализации zlib
						//
						uint32 result = adler32_z(0, 0, 0);
						result = adler32_z(result, (const uint8 *)p_item->In.GetBuf(), p_item->In.GetLen());
						SLCHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
#endif
				}
			}
		}
	}
	{
#ifdef _LOCAL_USE_SSL // {
		// IDEA
		SString in_file_name = MakeInputFilePath("idea.vec");
		TSCollection <BdtTestItem> data_set;
		THROW(SLCHECK_NZ(ReadBdtTestData(in_file_name, "IDEA", data_set)));
		{
			SBdtFunct fu_direct(SBdtFunct::IdeaCrypt);
			SBdtFunct fu_inverse(SBdtFunct::IdeaDecrypt);
			for(uint i = 0; i < data_set.getCount(); i++) {
				BdtTestItem * p_item = data_set.at(i);
				if(p_item && (p_item->Flags & p_item->fIn) && (p_item->Flags & p_item->fOut) && (p_item->Flags & p_item->fKey)) {
					THROW(SLCHECK_EQ(p_item->Key.GetLen(), 16));
					THROW(SLCHECK_EQ(p_item->In.GetLen(), p_item->Out.GetLen()));
					//
					{
						STempBuffer result(MAX(p_item->Out.GetLen(), 1024)+16);
						STempBuffer result_decrypt(MAX(p_item->Out.GetLen(), 1024)+16);
						size_t out_offs = 0;
						size_t _o;
						size_t crypt_size = 0;
						THROW(SLCHECK_NZ(result.IsValid()));
						THROW(SLCHECK_NZ(result_decrypt.IsValid()));
						memzero(result, result.GetSize());
						THROW(SLCHECK_NZ(fu_direct.SetParam(SBdtFunct::paramKey, p_item->Key.GetBuf(), p_item->Key.GetLen())));
						THROW(SLCHECK_NZ(fu_direct.Init()));
						THROW(SLCHECK_NZ(fu_direct.Update(p_item->In.GetBuf(), p_item->In.GetLen(), result, result.GetSize(), &_o)));
						out_offs += _o;
						THROW(SLCHECK_NZ(fu_direct.Finish(result+out_offs, result.GetSize()-out_offs, &_o)));
						out_offs += _o;
						crypt_size = out_offs;
						//THROW(SLCHECK_EQ(out_offs, p_item->In.GetLen()));
						{
							int   lr = 0;
							SLCHECK_Z(lr = memcmp(result, p_item->Out.GetBuf(), p_item->Out.GetLen()));
							SLCHECK_Z(lr);
						}
						//
						out_offs = 0;
						memzero(result_decrypt, result.GetSize());
						THROW(SLCHECK_NZ(fu_inverse.SetParam(SBdtFunct::paramKey, p_item->Key.GetBuf(), p_item->Key.GetLen())));
						THROW(SLCHECK_NZ(fu_inverse.Init()));
						THROW(SLCHECK_NZ(fu_inverse.Update(result, crypt_size, result_decrypt, result_decrypt.GetSize(), &_o)));
						out_offs += _o;
						// Для финализации здесь надо подложить последний блок
						THROW(SLCHECK_NZ(fu_inverse.Finish(result_decrypt+out_offs-_o, _o, &_o)));
						out_offs += _o;
						//THROW(SLCHECK_EQ(out_offs, p_item->In.GetLen()));
						SLCHECK_Z(memcmp(result_decrypt, p_item->In.GetBuf(), p_item->In.GetLen()));
					}
				}
			}
		}
#endif // } _LOCAL_USE_SSL 
	}
	CATCHZOK
	return CurrentStatus;
}

SLTEST_R(CalcCheckDigit)
{
	int    ok = 1;
	SString in_file_name(MakeInputFilePath("CalcCheckDigit.txt"));
	SString line_buf, left, right;
	SFile f_inp;
	THROW(SLCHECK_NZ(f_inp.Open(in_file_name, SFile::mRead)));
	while(f_inp.ReadLine(line_buf, SFile::rlfChomp)) {
		if(line_buf.Divide(':', left, right) > 0) {
			right.Strip();
			if(left.IsEqiAscii("upc") || left.IsEqiAscii("ean")) {
				SLCHECK_NZ(isdec(SCalcCheckDigit(SCHKDIGALG_BARCODE, right, right.Len()-1)));
				SLCHECK_EQ(SCalcCheckDigit(SCHKDIGALG_BARCODE|SCHKDIGALG_TEST, right, right.Len()), 1);
			}
			else if(left.IsEqiAscii("inn")) {
				SLCHECK_EQ(SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, right, right.Len()), 1);
			}
			else if(left.IsEqiAscii("luhn")) {
				SLCHECK_NZ(isdec(SCalcCheckDigit(SCHKDIGALG_LUHN, right, right.Len()-1)));
				SLCHECK_EQ(SCalcCheckDigit(SCHKDIGALG_LUHN|SCHKDIGALG_TEST, right, right.Len()), 1);
			}
		}
	}
	CATCHZOK
	return CurrentStatus;
}

class XXHashTest {
private:
	STestCase & R_Case;
public:
	XXHashTest(STestCase & rCase) : R_Case(rCase)
	{
	}
	/*void BMK_checkResult32(uint32 r1, uint32 r2)
	{
		static int nbTests = 1;
		if(r1!=r2) {
			DISPLAY("\rError: 32-bit hash test %i: Internal sanity check failed!\n", nbTests);
			DISPLAY("\rGot 0x%08X, expected 0x%08X.\n", r1, r2);
			DISPLAY("\rNote: If you modified the hash functions, make sure to either update the values\nor temporarily comment out the tests in BMK_sanityCheck.\n");
			exit(1);
		}
		nbTests++;
	}*/
	/*void BMK_checkResult64(uint64 r1, uint64 r2)
	{
		static int nbTests = 1;
		if(r1!=r2) {
			DISPLAY("\rError: 64-bit hash test %i: Internal sanity check failed!\n", nbTests);
			DISPLAY("\rGot 0x%08X%08XULL, expected 0x%08X%08XULL.\n", (uint32)(r1>>32), (uint32)r1, (uint32)(r2>>32), (uint32)r2);
			DISPLAY("\rNote: If you modified the hash functions, make sure to either update the values\nor temporarily comment out the tests in BMK_sanityCheck.\n");
			exit(1);
		}
		nbTests++;
	}*/
	/*void BMK_checkResult128(XXH128_hash_t r1, XXH128_hash_t r2)
	{
		static int nbTests = 1;
		if((r1.low64 != r2.low64) || (r1.high64 != r2.high64)) {
			DISPLAY("\rError: 128-bit hash test %i: Internal sanity check failed.\n", nbTests);
			DISPLAY("\rGot { 0x%08X%08XULL, 0x%08X%08XULL }, expected { 0x%08X%08XULL, %08X%08XULL } \n",
				(uint32)(r1.low64>>32), (uint32)r1.low64, (uint32)(r1.high64>>32), (uint32)r1.high64, (uint32)(r2.low64>>32), (uint32)r2.low64, (uint32)(r2.high64>>32), (uint32)r2.high64);
			DISPLAY("\rNote: If you modified the hash functions, make sure to either update the values\nor temporarily comment out the tests in BMK_sanityCheck.\n");
			exit(1);
		}
		nbTests++;
	}*/
	void BMK_testXXH32(const void * sequence, size_t len, uint32 seed, uint32 Nresult)
	{
		XXH32_state_t state;
		//BMK_checkResult32(XXH32(sequence, len, seed), Nresult);
		R_Case.SLCHECK_EQ(XXH32(sequence, len, seed), Nresult);
		XXH32_reset(&state, seed);
		XXH32_update(&state, sequence, len);
		//BMK_checkResult32(XXH32_digest(&state), Nresult);
		R_Case.SLCHECK_EQ(XXH32_digest(&state), Nresult);
		XXH32_reset(&state, seed);
		for(size_t pos = 0; pos < len; pos++)
			XXH32_update(&state, ((const char *)sequence)+pos, 1);
		//BMK_checkResult32(XXH32_digest(&state), Nresult);
		R_Case.SLCHECK_EQ(XXH32_digest(&state), Nresult);
	}
	void BMK_testXXH64(const void * data, size_t len, uint64 seed, uint64 Nresult)
	{
		XXH64_state_t state;
		//BMK_checkResult64(XXH64(data, len, seed), Nresult);
		R_Case.SLCHECK_EQ(XXH64(data, len, seed), Nresult);
		XXH64_reset(&state, seed);
		XXH64_update(&state, data, len);
		//BMK_checkResult64(XXH64_digest(&state), Nresult);
		R_Case.SLCHECK_EQ(XXH64(data, len, seed), Nresult);
		XXH64_reset(&state, seed);
		for(size_t pos = 0; pos < len; pos++)
			XXH64_update(&state, ((const char *)data)+pos, 1);
		//BMK_checkResult64(XXH64_digest(&state), Nresult);
		R_Case.SLCHECK_EQ(XXH64_digest(&state), Nresult);
	}
	void BMK_testXXH3(const void * data, size_t len, uint64 seed, uint64 Nresult)
	{
		{   
			uint64 const Dresult = XXH3_64bits_withSeed(data, len, seed);
			//BMK_checkResult64(Dresult, Nresult);
			R_Case.SLCHECK_EQ(Dresult, Nresult);
		}
		// check that the no-seed variant produces same result as seed==0 
		if(seed == 0) {
			uint64 const Dresult = XXH3_64bits(data, len);
			//BMK_checkResult64(Dresult, Nresult);
			R_Case.SLCHECK_EQ(Dresult, Nresult);
		}
		// streaming API test 
		{   
			XXH3_state_t state;
			// single ingestion 
			XXH3_64bits_reset_withSeed(&state, seed);
			XXH3_64bits_update(&state, data, len);
			//BMK_checkResult64(XXH3_64bits_digest(&state), Nresult);
			R_Case.SLCHECK_EQ(XXH3_64bits_digest(&state), Nresult);
			if(len > 3) {
				// 2 ingestions 
				XXH3_64bits_reset_withSeed(&state, seed);
				XXH3_64bits_update(&state, data, 3);
				XXH3_64bits_update(&state, (const char *)data+3, len-3);
				//BMK_checkResult64(XXH3_64bits_digest(&state), Nresult);
				R_Case.SLCHECK_EQ(XXH3_64bits_digest(&state), Nresult);
			}
			// byte by byte ingestion 
			{   
				XXH3_64bits_reset_withSeed(&state, seed);
				for(size_t pos = 0; pos < len; pos++)
					XXH3_64bits_update(&state, ((const char *)data)+pos, 1);
				//BMK_checkResult64(XXH3_64bits_digest(&state), Nresult);
				R_Case.SLCHECK_EQ(XXH3_64bits_digest(&state), Nresult);
			}   
		}
	}
	void BMK_testXXH3_withSecret(const void * data, size_t len, const void * secret, size_t secretSize, uint64 Nresult)
	{
		{   
			uint64 const Dresult = XXH3_64bits_withSecret(data, len, secret, secretSize);
			//BMK_checkResult64(Dresult, Nresult);
			R_Case.SLCHECK_EQ(Dresult, Nresult);
		}
		// streaming API test 
		{   
			XXH3_state_t state;
			XXH3_64bits_reset_withSecret(&state, secret, secretSize);
			XXH3_64bits_update(&state, data, len);
			//BMK_checkResult64(XXH3_64bits_digest(&state), Nresult);
			R_Case.SLCHECK_EQ(XXH3_64bits_digest(&state), Nresult);
			// byte by byte ingestion 
			{   
				XXH3_64bits_reset_withSecret(&state, secret, secretSize);
				for(size_t pos = 0; pos<len; pos++)
					XXH3_64bits_update(&state, ((const char *)data)+pos, 1);
				//BMK_checkResult64(XXH3_64bits_digest(&state), Nresult);
				R_Case.SLCHECK_EQ(XXH3_64bits_digest(&state), Nresult);
			}   
		}
	}
	void BMK_testXXH128(const void * data, size_t len, uint64 seed, XXH128_hash_t Nresult)
	{
		{   
			XXH128_hash_t const Dresult = XXH3_128bits_withSeed(data, len, seed);
			//BMK_checkResult128(Dresult, Nresult);
			R_Case.SLCHECK_EQMEM(&Dresult, &Nresult, 16);
		}
		// check that XXH128() is identical to XXH3_128bits_withSeed() 
		{   
			XXH128_hash_t const Dresult2 = XXH128(data, len, seed);
			//BMK_checkResult128(Dresult2, Nresult);
			R_Case.SLCHECK_EQMEM(&Dresult2, &Nresult, 16);
		}
		// check that the no-seed variant produces same result as seed==0 
		if(seed == 0) {
			XXH128_hash_t const Dresult = XXH3_128bits(data, len);
			//BMK_checkResult128(Dresult, Nresult);
			R_Case.SLCHECK_EQMEM(&Dresult, &Nresult, 16);
		}
		// streaming API test 
		{   
			XXH3_state_t state;
			// single ingestion 
			XXH3_128bits_reset_withSeed(&state, seed);
			XXH3_128bits_update(&state, data, len);
			//BMK_checkResult128(XXH3_128bits_digest(&state), Nresult);
			{
				XXH128_hash_t digest128 = XXH3_128bits_digest(&state);
				R_Case.SLCHECK_EQMEM(&digest128, &Nresult, 16);
			}
			if(len > 3) {
				// 2 ingestions 
				XXH3_128bits_reset_withSeed(&state, seed);
				XXH3_128bits_update(&state, data, 3);
				XXH3_128bits_update(&state, (const char *)data+3, len-3);
				//BMK_checkResult128(XXH3_128bits_digest(&state), Nresult);
				{
					XXH128_hash_t digest128 = XXH3_128bits_digest(&state);
					R_Case.SLCHECK_EQMEM(&digest128, &Nresult, 16);
				}
			}
			// byte by byte ingestion 
			{   
				XXH3_128bits_reset_withSeed(&state, seed);
				for(size_t pos = 0; pos < len; pos++)
					XXH3_128bits_update(&state, ((const char *)data)+pos, 1);
				//BMK_checkResult128(XXH3_128bits_digest(&state), Nresult);
				{
					XXH128_hash_t digest128 = XXH3_128bits_digest(&state);
					R_Case.SLCHECK_EQMEM(&digest128, &Nresult, 16);
				}
			}   
		}
	}
	void Test()
	{
		const uint32 prime = SlConst::MagicHashPrime32/*2654435761U*/;
		const uint64 prime64 = 11400714785074694797ULL;
		//#define SANITY_BUFFER_SIZE 2243
		BYTE sanityBuffer[2243];
		uint64 byteGen = prime;
		for(int i = 0; i < SIZEOFARRAY(sanityBuffer); i++) {
			sanityBuffer[i] = (BYTE)(byteGen>>56);
			byteGen *= prime64;
		}
		BMK_testXXH32(NULL,          0, 0,     0x02CC5D05);
		BMK_testXXH32(NULL,          0, prime, 0x36B78AE7);
		BMK_testXXH32(sanityBuffer,  1, 0,     0xCF65B03E);
		BMK_testXXH32(sanityBuffer,  1, prime, 0xB4545AA4);
		BMK_testXXH32(sanityBuffer, 14, 0,     0x1208E7E2);
		BMK_testXXH32(sanityBuffer, 14, prime, 0x6AF1D1FE);
		BMK_testXXH32(sanityBuffer, 222, 0,     0x5BD11DBD);
		BMK_testXXH32(sanityBuffer, 222, prime, 0x58803C5F);

		BMK_testXXH64(NULL,  0, 0,     0xEF46DB3751D8E999ULL);
		BMK_testXXH64(NULL,  0, prime, 0xAC75FDA2929B17EFULL);
		BMK_testXXH64(sanityBuffer,  1, 0,     0xE934A84ADB052768ULL);
		BMK_testXXH64(sanityBuffer,  1, prime, 0x5014607643A9B4C3ULL);
		BMK_testXXH64(sanityBuffer,  4, 0,     0x9136A0DCA57457EEULL);
		BMK_testXXH64(sanityBuffer, 14, 0,     0x8282DCC4994E35C8ULL);
		BMK_testXXH64(sanityBuffer, 14, prime, 0xC3BD6BF63DEB6DF0ULL);
		BMK_testXXH64(sanityBuffer, 222, 0,     0xB641AE8CB691C174ULL);
		BMK_testXXH64(sanityBuffer, 222, prime, 0x20CB8AB7AE10C14AULL);

		BMK_testXXH3(NULL,           0, 0,       0); /* zero-length hash is always 0 */
		BMK_testXXH3(NULL,           0, prime64, 0);
		BMK_testXXH3(sanityBuffer,   1, 0,       0x7198D737CFE7F386ULL); /* 1 -  3 */
		BMK_testXXH3(sanityBuffer,   1, prime64, 0xB70252DB7161C2BDULL); /* 1 -  3 */
		BMK_testXXH3(sanityBuffer,   6, 0,       0x22CBF5F3E1F6257CULL); /* 4 -  8 */
		BMK_testXXH3(sanityBuffer,   6, prime64, 0x6398631C12AB94CEULL); /* 4 -  8 */
		BMK_testXXH3(sanityBuffer,  12, 0,       0xD5361CCEEBB5A0CCULL); /* 9 - 16 */
		BMK_testXXH3(sanityBuffer,  12, prime64, 0xC4C125E75A808C3DULL); /* 9 - 16 */
		BMK_testXXH3(sanityBuffer,  24, 0,       0x46796F3F78B20F6BULL); /* 17 - 32 */
		BMK_testXXH3(sanityBuffer,  24, prime64, 0x60171A7CD0A44C10ULL); /* 17 - 32 */
		BMK_testXXH3(sanityBuffer,  48, 0,       0xD8D4D3590D136E11ULL); /* 33 - 64 */
		BMK_testXXH3(sanityBuffer,  48, prime64, 0x05441F2AEC2A1296ULL); /* 33 - 64 */
		BMK_testXXH3(sanityBuffer,  80, 0,       0xA1DC8ADB3145B86AULL); /* 65 - 96 */
		BMK_testXXH3(sanityBuffer,  80, prime64, 0xC9D55256965B7093ULL); /* 65 - 96 */
		BMK_testXXH3(sanityBuffer, 112, 0,       0xE43E5717A61D3759ULL); /* 97 -128 */
		BMK_testXXH3(sanityBuffer, 112, prime64, 0x5A5F89A3FECE44A5ULL); /* 97 -128 */
		BMK_testXXH3(sanityBuffer, 195, 0,       0x6F747739CBAC22A5ULL); /* 129-240 */
		BMK_testXXH3(sanityBuffer, 195, prime64, 0x33368E23C7F95810ULL); /* 129-240 */

		BMK_testXXH3(sanityBuffer, 403, 0,       0x4834389B15D981E8ULL); /* one block, last stripe is overlapping */
		BMK_testXXH3(sanityBuffer, 403, prime64, 0x85CE5DFFC7B07C87ULL); /* one block, last stripe is overlapping */
		BMK_testXXH3(sanityBuffer, 512, 0,       0x6A1B982631F059A8ULL); /* one block, finishing at stripe boundary */
		BMK_testXXH3(sanityBuffer, 512, prime64, 0x10086868CF0ADC99ULL); /* one block, finishing at stripe boundary */
		BMK_testXXH3(sanityBuffer, 2048, 0,       0xEFEFD4449323CDD4ULL); /* 2 blocks, finishing at block boundary */
		BMK_testXXH3(sanityBuffer, 2048, prime64, 0x01C85E405ECA3F6EULL); /* 2 blocks, finishing at block boundary */
		BMK_testXXH3(sanityBuffer, 2240, 0,       0x998C0437486672C7ULL); /* 3 blocks, finishing at stripe boundary */
		BMK_testXXH3(sanityBuffer, 2240, prime64, 0x4ED38056B87ABC7FULL); /* 3 blocks, finishing at stripe boundary */
		BMK_testXXH3(sanityBuffer, 2243, 0,       0xA559D20581D742D3ULL); /* 3 blocks, last stripe is overlapping */
		BMK_testXXH3(sanityBuffer, 2243, prime64, 0x96E051AB57F21FC8ULL); /* 3 blocks, last stripe is overlapping */
		{   
			const void * const secret = sanityBuffer + 7;
			const size_t secretSize = XXH3_SECRET_SIZE_MIN + 11;
			BMK_testXXH3_withSecret(NULL,           0, secret, secretSize, 0); /* zero-length hash is always 0 */
			BMK_testXXH3_withSecret(sanityBuffer,   1, secret, secretSize, 0x7F69735D618DB3F0ULL); /* 1 -  3 */
			BMK_testXXH3_withSecret(sanityBuffer,   6, secret, secretSize, 0xBFCC7CB1B3554DCEULL); /* 6 -  8 */
			BMK_testXXH3_withSecret(sanityBuffer,  12, secret, secretSize, 0x8C50DC90AC9206FCULL); /* 9 - 16 */
			BMK_testXXH3_withSecret(sanityBuffer,  24, secret, secretSize, 0x1CD2C2EE9B9A0928ULL); /* 17 - 32 */
			BMK_testXXH3_withSecret(sanityBuffer,  48, secret, secretSize, 0xA785256D9D65D514ULL); /* 33 - 64 */
			BMK_testXXH3_withSecret(sanityBuffer,  80, secret, secretSize, 0x6F3053360D21BBB7ULL); /* 65 - 96 */
			BMK_testXXH3_withSecret(sanityBuffer, 112, secret, secretSize, 0x560E82D25684154CULL); /* 97 -128 */
			BMK_testXXH3_withSecret(sanityBuffer, 195, secret, secretSize, 0xBA5BDDBC5A767B11ULL); /* 129-240 */

			BMK_testXXH3_withSecret(sanityBuffer, 403, secret, secretSize, 0xFC3911BBA656DB58ULL); /* one block, last stripe is overlapping */
			BMK_testXXH3_withSecret(sanityBuffer, 512, secret, secretSize, 0x306137DD875741F1ULL); /* one block, finishing at stripe boundary */
			BMK_testXXH3_withSecret(sanityBuffer, 2048, secret, secretSize, 0x2836B83880AD3C0CULL); /* > one block, at least one scrambling */
			BMK_testXXH3_withSecret(sanityBuffer, 2243, secret, secretSize, 0x3446E248A00CB44AULL); /* > one block, at least one scrambling, last stripe unaligned */
		}
		{
			XXH128_hash_t const expected = { 0, 0 };
			BMK_testXXH128(NULL, 0, 0,     expected); /* zero-length hash is { seed, -seed } by default */
		}
		{   
			XXH128_hash_t const expected = { 0, 0 };
			BMK_testXXH128(NULL, 0, prime, expected);}
		{   
			XXH128_hash_t const expected = { 0x7198D737CFE7F386ULL, 0x3EE70EA338F3F1E8ULL };
			BMK_testXXH128(sanityBuffer,   1, 0,     expected); /* 1-3 */
		}
		{   
			XXH128_hash_t const expected = { 0x8E05996EC27C0F46ULL, 0x90DFC659A8BDCC0CULL };
			BMK_testXXH128(sanityBuffer,   1, prime, expected); /* 1-3 */
		}
		{   
			XXH128_hash_t const expected = { 0x22CBF5F3E1F6257CULL, 0xD4E6C2B94FFC3BFAULL };
			BMK_testXXH128(sanityBuffer,   6, 0,     expected); /* 4-8 */
		}
		{   
			XXH128_hash_t const expected = { 0x97B28D3079F8541FULL, 0xEFC0B954298E6555ULL };
			BMK_testXXH128(sanityBuffer,   6, prime, expected); /* 4-8 */
		}
		{   
			XXH128_hash_t const expected = { 0x0E0CD01F05AC2F0DULL, 0x2B55C95951070D4BULL };
			BMK_testXXH128(sanityBuffer,  12, 0,     expected); /* 9-16 */
		}
		{   
			XXH128_hash_t const expected = { 0xA9DE561CA04CDF37ULL, 0x609E31FDC00A43C9ULL };
			BMK_testXXH128(sanityBuffer,  12, prime, expected); /* 9-16 */
		}
		{   
			XXH128_hash_t const expected = { 0x46796F3F78B20F6BULL, 0x58FF55C3926C13FAULL };
			BMK_testXXH128(sanityBuffer,  24, 0,     expected); /* 17-32 */
		}
		{   
			XXH128_hash_t const expected = { 0x30D5C4E9EB415C55ULL, 0x8868344B3A4645D0ULL };
			BMK_testXXH128(sanityBuffer,  24, prime, expected); /* 17-32 */
		}
		{   
			XXH128_hash_t const expected = { 0xD8D4D3590D136E11ULL, 0x5527A42843020A62ULL };
			BMK_testXXH128(sanityBuffer,  48, 0,     expected); /* 33-64 */
		}
		{   
			XXH128_hash_t const expected = { 0x1D8834E1A5407A1CULL, 0x44375B9FB060F541ULL };
			BMK_testXXH128(sanityBuffer,  48, prime, expected); /* 33-64 */
		}
		{   
			XXH128_hash_t const expected = { 0x4B9B448ED8DFD3DDULL, 0xE805A6D1A43D70E5ULL };
			BMK_testXXH128(sanityBuffer,  81, 0,     expected); /* 65-96 */
		}
		{   
			XXH128_hash_t const expected = { 0xD2D6B075945617BAULL, 0xE58BE5736F6E7550ULL };
			BMK_testXXH128(sanityBuffer,  81, prime, expected); /* 65-96 */
		}
		{   
			XXH128_hash_t const expected = { 0xC5A9F97B29EFA44EULL, 0x254DB7BE881E125CULL };
			BMK_testXXH128(sanityBuffer, 103, 0,     expected); /* 97-128 */
		}
		{   
			XXH128_hash_t const expected = { 0xFA2086367CDB177FULL, 0x0AEDEA68C988B0C0ULL };
			BMK_testXXH128(sanityBuffer, 103, prime, expected); /* 97-128 */
		}
		{   
			XXH128_hash_t const expected = { 0xC3142FDDD9102A3FULL, 0x06F1747E77185F97ULL };
			BMK_testXXH128(sanityBuffer, 192, 0,     expected); /* 129-240 */
		}
		{   
			XXH128_hash_t const expected = { 0xA89F07B35987540FULL, 0xCF1B35FB2C557F54ULL };
			BMK_testXXH128(sanityBuffer, 192, prime, expected); /* 129-240 */
		}
		{   
			XXH128_hash_t const expected = { 0xA61AC4EB3295F86BULL, 0x33FA7B7598C28A07ULL };
			BMK_testXXH128(sanityBuffer, 222, 0,     expected); /* 129-240 */
		}
		{   
			XXH128_hash_t const expected = { 0x54135EB88AD8B75EULL, 0xBC45CE6AE50BCF53ULL };
			BMK_testXXH128(sanityBuffer, 222, prime, expected); /* 129-240 */
		}
		{   
			XXH128_hash_t const expected = { 0xB0C48E6D18E9D084ULL, 0xB16FC17E992FF45DULL };
			BMK_testXXH128(sanityBuffer, 403, 0,     expected); /* one block, last stripe is overlapping */
		}
		{   
			XXH128_hash_t const expected = { 0x0A1D320C9520871DULL, 0xCE11CB376EC93252ULL };
			BMK_testXXH128(sanityBuffer, 403, prime64, expected); /* one block, last stripe is overlapping */
		}
		{   
			XXH128_hash_t const expected = { 0xA03428558AC97327ULL, 0x4ECF51281BA406F7ULL };
			BMK_testXXH128(sanityBuffer, 512, 0,     expected); /* one block, finishing at stripe boundary */
		}
		{   
			XXH128_hash_t const expected = { 0xAF67A482D6C893F2ULL, 0x1382D92F25B84D90ULL };
			BMK_testXXH128(sanityBuffer, 512, prime64, expected); /* one block, finishing at stripe boundary */
		}
		{   
			XXH128_hash_t const expected = { 0x21901B416B3B9863ULL, 0x212AF8E6326F01E0ULL };
			BMK_testXXH128(sanityBuffer, 2048, 0,     expected); /* two blocks, finishing at block boundary */
		}
		{   
			XXH128_hash_t const expected = { 0xBDBB2282577DADECULL, 0xF78CDDC2C9A9A692ULL };
			BMK_testXXH128(sanityBuffer, 2048, prime, expected); /* two blocks, finishing at block boundary */
		}
		{   
			XXH128_hash_t const expected = { 0x00AD52FA9385B6FEULL, 0xC705BAD3356CE302ULL };
			BMK_testXXH128(sanityBuffer, 2240, 0,     expected); /* two blocks, ends at stripe boundary */
		}
		{   
			XXH128_hash_t const expected = { 0x10FD0072EC68BFAAULL, 0xE1312F3458817F15ULL };
			BMK_testXXH128(sanityBuffer, 2240, prime, expected); /* two blocks, ends at stripe boundary */
		}
		{   
			XXH128_hash_t const expected = { 0x970C91411533862CULL, 0x4BBD06FF7BFF0AB1ULL };
			BMK_testXXH128(sanityBuffer, 2237, 0,     expected); /* two blocks, ends at stripe boundary */
		}
		{   
			XXH128_hash_t const expected = { 0xD80282846D814431ULL, 0x14EBB157B84D9785ULL };
			BMK_testXXH128(sanityBuffer, 2237, prime, expected); /* two blocks, ends at stripe boundary */
		}
		//DISPLAYLEVEL(3, "\r%70s\r", ""); /* Clean display line */
		//DISPLAYLEVEL(3, "Sanity check -- all tests ok\n");
	}
};
//
// Descr: Реализация 32-разрядного murmur2 из nginx (seed assumed 0)
//
static uint32 SlEqualityTest_ngx_murmur_hash2(const uchar * data, size_t len)
{
	uint32 k;
	uint32 h = (0U ^ static_cast<uint32>(len));
	while(len >= 4) {
		k  = data[0];
		k |= data[1] << 8;
		k |= data[2] << 16;
		k |= data[3] << 24;
		k *= 0x5bd1e995;
		k ^= k >> 24;
		k *= 0x5bd1e995;
		h *= 0x5bd1e995;
		h ^= k;
		data += 4;
		len -= 4;
	}
	switch(len) {
		case 3: h ^= data[2] << 16;
		// @fallthrough
		case 2: h ^= data[1] << 8;
		// @fallthrough
		case 1:
		    h ^= data[0];
		    h *= 0x5bd1e995;
	}
	h ^= h >> 13;
	h *= 0x5bd1e995;
	h ^= h >> 15;
	return h;
}
//
// Descr: Реализация 32-разрядного murmur3 из языка Gravity.
//   Приведена для сравнения, поскольку реализация аналогичного SlHash::Murmur3_32 взята из иного источника.
//
static uint32 SlEqualityTest_gravity_murmur3_32(const char * key, uint32 len, uint32 seed) 
{
	static const uint32 c1 = SlConst::MagicMurmurC1;
	static const uint32 c2 = SlConst::MagicMurmurC2;
	static const uint32 r1 = 15;
	static const uint32 r2 = 13;
	static const uint32 m = 5;
	static const uint32 n = 0xe6546b64;
	uint32 hash = seed;
	const int nblocks = len / 4;
	const uint32 * blocks = (const uint32 *)key;
	for(int i = 0; i < nblocks; i++) {
		uint32 k = blocks[i];
		k *= c1;
		k = /*_rotl*/SBits::Rotl(k, r1);
		k *= c2;
		hash ^= k;
		hash = /*_rotl*/SBits::Rotl(hash, r2) * m + n;
	}
	const uint8 * tail = (const uint8 *)(key + nblocks * 4);
	uint32 k1 = 0;
	switch(len & 3) {
		case 3:
			k1 ^= tail[2] << 16;
		case 2:
			k1 ^= tail[1] << 8;
		case 1:
			k1 ^= tail[0];
			k1 *= c1;
			k1 = /*_rotl*/SBits::Rotl(k1, r1);
			k1 *= c2;
			hash ^= k1;
	}
	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);
	return hash;
}

SLTEST_R(HashFunction)
{
	{
		SString in_file_name(MakeInputFilePath("botan-validate.dat"));
		STestDataArray td;
		SString key_buf, val_buf;
		STempBuffer bin_buf(0);
		SCRC32 _c;
		THROW(td.ReadBotanTestSequence(1, in_file_name, "CRC32"));
		for(uint i = 0; i < td.GetCount(); i++) {
			const STestDataArray::Item & r_item = td.Get(i);
			if(r_item.Count >= 2) {
				size_t real_bin_size = 0;
				uint32 valid_val = 0;
				td.GetDataByPos(r_item.ValPos[STestDataArray::pIn], key_buf);
				td.GetDataByPos(r_item.ValPos[STestDataArray::pOut], val_buf);
				THROW(bin_buf.Alloc(NZOR(key_buf.Len(), 1) * 2));
				THROW(key_buf.DecodeHex(0, bin_buf, bin_buf.GetSize(), &real_bin_size));
				{
					const size_t additive_chunk = 16;
					ulong val_additive = 0;
					ulong val = _c.Calc(0, (const uint8 *)(const char *)bin_buf, real_bin_size);
					if(real_bin_size > additive_chunk) {
						//
						// Тест аддитивности расчета CRC32
						//
						size_t tail_size = real_bin_size;
						while(tail_size > additive_chunk) {
							val_additive = _c.Calc(val_additive, ((const uint8 *)(const char *)bin_buf)+real_bin_size-tail_size, additive_chunk);
							tail_size -= additive_chunk;
						}
						val_additive = _c.Calc(val_additive, ((const uint8 *)(const char *)bin_buf)+real_bin_size-tail_size, tail_size);
						SLCHECK_EQ(val_additive, val);
					}
					THROW(bin_buf.Alloc(NZOR(val_buf.Len(), 1) * 2));
					THROW(val_buf.DecodeHex(1, bin_buf, bin_buf.GetSize(), &real_bin_size));
					THROW(SLCHECK_EQ(real_bin_size, static_cast<size_t>(4U)));
					SLCHECK_EQ(*reinterpret_cast<const ulong *>(bin_buf.cptr()), val);
				}
			}
		}
	}
	{
		const char * p_key = "abcdefghijklmnopqrstuvwxyz1234567890";
		const size_t key_size = sstrlen(p_key);
		uint32 h;
		//SString msg;
		SLCHECK_EQ(h = SlHash::RS(p_key, key_size), 4097835502U);
		//SetInfo(msg.Z().CatEq("RSHash", h), -1);
		SLCHECK_EQ(h = SlHash::JS(p_key, key_size), 1651003062U);
		//SetInfo(msg.Z().CatEq("JSHash", h), -1);
		SLCHECK_EQ(h = SlHash::PJW(p_key, key_size), 126631744U);
		//SetInfo(msg.Z().CatEq("PJWHash", h), -1);
		SLCHECK_EQ(h = SlHash::ELF(p_key, key_size), 126631744U);
		//SetInfo(msg.Z().CatEq("ELFHash", h), -1);
		SLCHECK_EQ(h = SlHash::BKDR(p_key, key_size), 3153586616U);
		//SetInfo(msg.Z().CatEq("BKDRHash", h), -1);
		SLCHECK_EQ(h = SlHash::SDBM(p_key, key_size), 3449571336U);
		//SetInfo(msg.Z().CatEq("SDBMHash", h), -1);
		SLCHECK_EQ(h = SlHash::DJB(p_key, key_size), 729241521U);
		//SetInfo(msg.Z().CatEq("DJBHash", h), -1);
		SLCHECK_EQ(h = SlHash::DEK(p_key, key_size), 2923964919U);
		//SetInfo(msg.Z().CatEq("DEKHash", h), -1);
		SLCHECK_EQ(h = SlHash::BP(p_key, key_size), 1726880944U);
		//SetInfo(msg.Z().CatEq("BPHash", h), -1);
		SLCHECK_EQ(h = SlHash::FNV(p_key, key_size), 3243095106U);
		//SetInfo(msg.Z().CatEq("FNVHash", h), -1);
		SLCHECK_EQ(h = SlHash::AP(p_key, key_size), 882643939U);
		//SetInfo(msg.Z().CatEq("APHash", h), -1);
	}
	{
		XXHashTest xxhash_test(*this);
		xxhash_test.Test();
	}
	{
		SString out_buf;
		uint8  buf[BOBJEN_MAXLEN+20], * b;
		uint32 len, h, i, j, ref;
		uint8 q[] = "This is the time for all good men to come to the aid of their country...";
		uint8 qq[] = "xThis is the time for all good men to come to the aid of their country...";
		uint8 qqq[] = "xxThis is the time for all good men to come to the aid of their country...";
		uint8 qqqq[] = "xxxThis is the time for all good men to come to the aid of their country...";
		uint8 * p;
		SetInfo((out_buf = "Endianness.  These lines should all be the same (for values filled in):").CR());
		SetInfo(out_buf.Printf("%.8x                            %.8x                            %.8x\n",
		    SlHash::BobJencHash_Word((const uint32 *)q, (sizeof(q)-1)/4, 13),
		    SlHash::BobJencHash_Word((const uint32 *)q, (sizeof(q)-5)/4, 13),
		    SlHash::BobJencHash_Word((const uint32 *)q, (sizeof(q)-9)/4, 13)));
		p = q;
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    SlHash::BobJencHash_Little(p, sizeof(q)-1, 13), SlHash::BobJencHash_Little(p, sizeof(q)-2, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-3, 13), SlHash::BobJencHash_Little(p, sizeof(q)-4, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-5, 13), SlHash::BobJencHash_Little(p, sizeof(q)-6, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-7, 13), SlHash::BobJencHash_Little(p, sizeof(q)-8, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-9, 13), SlHash::BobJencHash_Little(p, sizeof(q)-10, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-11, 13), SlHash::BobJencHash_Little(p, sizeof(q)-12, 13)));
		p = &qq[1];
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    SlHash::BobJencHash_Little(p, sizeof(q)-1, 13), SlHash::BobJencHash_Little(p, sizeof(q)-2, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-3, 13), SlHash::BobJencHash_Little(p, sizeof(q)-4, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-5, 13), SlHash::BobJencHash_Little(p, sizeof(q)-6, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-7, 13), SlHash::BobJencHash_Little(p, sizeof(q)-8, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-9, 13), SlHash::BobJencHash_Little(p, sizeof(q)-10, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-11, 13), SlHash::BobJencHash_Little(p, sizeof(q)-12, 13)));
		p = &qqq[2];
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    SlHash::BobJencHash_Little(p, sizeof(q)-1, 13), SlHash::BobJencHash_Little(p, sizeof(q)-2, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-3, 13), SlHash::BobJencHash_Little(p, sizeof(q)-4, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-5, 13), SlHash::BobJencHash_Little(p, sizeof(q)-6, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-7, 13), SlHash::BobJencHash_Little(p, sizeof(q)-8, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-9, 13), SlHash::BobJencHash_Little(p, sizeof(q)-10, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-11, 13), SlHash::BobJencHash_Little(p, sizeof(q)-12, 13)));
		p = &qqqq[3];
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    SlHash::BobJencHash_Little(p, sizeof(q)-1, 13), SlHash::BobJencHash_Little(p, sizeof(q)-2, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-3, 13), SlHash::BobJencHash_Little(p, sizeof(q)-4, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-5, 13), SlHash::BobJencHash_Little(p, sizeof(q)-6, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-7, 13), SlHash::BobJencHash_Little(p, sizeof(q)-8, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-9, 13), SlHash::BobJencHash_Little(p, sizeof(q)-10, 13),
		    SlHash::BobJencHash_Little(p, sizeof(q)-11, 13), SlHash::BobJencHash_Little(p, sizeof(q)-12, 13)));
		SetInfo(out_buf.Z().CR());
		//
		// check that BobJencHash_Little2 and BobJencHash_Little produce the same results
		//
		i = 47; j = 0;
		SLCHECK_EQ((SlHash::BobJencHash_Little2(q, sizeof(q), &i, &j), i), SlHash::BobJencHash_Little(q, sizeof(q), 47));
		//
		// check that hashword2 and BobJencHash_Word produce the same results
		//
		len = 0xdeadbeef;
		i = 47, j = 0;
		SLCHECK_EQ((SlHash::BobJencHash_Word2(&len, 1, &i, &j), i), SlHash::BobJencHash_Word(&len, 1, 47));
		//
		// check BobJencHash_Little doesn't read before or after the ends of the string
		//
		for(h = 0, b = buf+1; h<8; ++h, ++b) {
			for(i = 0; i < BOBJEN_MAXLEN; ++i) {
				len = i;
				for(j = 0; j<i; ++j)
					*(b+j) = 0;
				//
				// these should all be equal
				//
				ref = SlHash::BobJencHash_Little(b, len, (uint32)1);
				*(b+i) = (uint8)~0;
				*(b-1) = (uint8)~0;
				SLCHECK_EQ(SlHash::BobJencHash_Little(b, len, (uint32)1), ref);
				SLCHECK_EQ(SlHash::BobJencHash_Little(b, len, (uint32)1), ref);
			}
		}
		{
			//
			// check for problems with nulls
			//
			const size_t cnt = 8;
			uint32 hash_list[32];
			uint8 buf[1];
			uint32 h, state[BOBJEN_HASHSTATE];
			buf[0] = ~0;
			for(i = 0; i < BOBJEN_HASHSTATE; ++i)
				state[i] = 1;
			for(i = 0, h = 0; i < cnt; ++i) {
				h = SlHash::BobJencHash_Little(buf, 0, h);
				hash_list[i] = h;
			}
			//
			// These should all be different
			//
			for(i = 0; i < cnt; i++) {
				for(j = i+1; j < cnt; j++) {
					SLCHECK_NZ(hash_list[i] - hash_list[j]);
				}
			}
		}
		{
			//
			// На одних данных BobJencHash_Little и BobJencHash_Word должны выдавать один результат
			//
			const char * p_data = "The procedure mandelbrot determines whether a point [x, y] in the complex domain is part of the famous Mandelbrot set by determining whether it leaves a certain radius after a given number of iterations";
			size_t len = sstrlen(p_data);
			len = (len >> 2) << 2; // Усекаем размер до кратного 4.
			SLCHECK_EQ(SlHash::BobJencHash_Little(p_data, len, 0xdeadbeef), SlHash::BobJencHash_Word((uint32 *)p_data, len / 4, 0xdeadbeef));
		}
		{
			uint32 b = 0;
			uint32 c = 0;
			SlHash::BobJencHash_Little2("", 0, &c, &b);
			SLCHECK_EQ(c, 0xdeadbeefU);
			SLCHECK_EQ(b, 0xdeadbeefU);
			b = 0xdeadbeef;
			c = 0;
			SlHash::BobJencHash_Little2("", 0, &c, &b);
			SLCHECK_EQ(c, 0xbd5b7ddeU);
			SLCHECK_EQ(b, 0xdeadbeefU);
			b = 0xdeadbeef;
			c = 0xdeadbeef;
			SlHash::BobJencHash_Little2("", 0, &c, &b);
			SLCHECK_EQ(c, 0x9c093ccdU);
			SLCHECK_EQ(b, 0xbd5b7ddeU);
			b = 0;
			c = 0;
			SlHash::BobJencHash_Little2("Four score and seven years ago", 30, &c, &b);
			SLCHECK_EQ(c, 0x17770551U);
			SLCHECK_EQ(b, 0xce7226e6U);
			b = 1;
			c = 0;
			SlHash::BobJencHash_Little2("Four score and seven years ago", 30, &c, &b);
			SLCHECK_EQ(c, 0xe3607caeU);
			SLCHECK_EQ(b, 0xbd371de4U);
			b = 0;
			c = 1;
			SlHash::BobJencHash_Little2("Four score and seven years ago", 30, &c, &b);
			SLCHECK_EQ(c, 0xcd628161U);
			SLCHECK_EQ(b, 0x6cbea4b3U);
			SLCHECK_EQ(SlHash::BobJencHash_Little("Four score and seven years ago", 30, 0), 0x17770551U);
			SLCHECK_EQ(SlHash::BobJencHash_Little("Four score and seven years ago", 30, 1), 0xcd628161U);
		}
	}
	{
		const char * p_data = "Hello, world!";
		SLCHECK_EQ(SlEqualityTest_ngx_murmur_hash2(reinterpret_cast<const uchar *>(p_data), sstrlen(p_data)), SlHash::Murmur2_32(p_data, sstrlen(p_data), 0));
		p_data = "xxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		SLCHECK_EQ(SlEqualityTest_ngx_murmur_hash2(reinterpret_cast<const uchar *>(p_data), sstrlen(p_data)), SlHash::Murmur2_32(p_data, sstrlen(p_data), 0));
		p_data = "";
		SLCHECK_EQ(SlEqualityTest_ngx_murmur_hash2(reinterpret_cast<const uchar *>(p_data), sstrlen(p_data)), SlHash::Murmur2_32(p_data, sstrlen(p_data), 0));
	}
	{
		//
		// murmur3 test
		//
		const char * p_data = "Hello, world!";
		uint32 rh;
		SLCHECK_EQ((rh = SlHash::Murmur3_32(p_data, sstrlen(p_data), 1234U)), 0xfaf6cdb3U);
		SLCHECK_EQ(SlEqualityTest_gravity_murmur3_32(p_data, sstrlen(p_data), 1234), rh);

		SLCHECK_EQ((rh = SlHash::Murmur3_32(p_data, sstrlen(p_data), 4321U)), 0xbf505788U);
		SLCHECK_EQ(SlEqualityTest_gravity_murmur3_32(p_data, sstrlen(p_data), 4321), rh);
		p_data = "xxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		SLCHECK_EQ((rh = SlHash::Murmur3_32(p_data, sstrlen(p_data), 1234U)), 0x8905ac28U);
		SLCHECK_EQ(SlEqualityTest_gravity_murmur3_32(p_data, sstrlen(p_data), 1234), rh);
		p_data = "";
		SLCHECK_EQ((rh = SlHash::Murmur3_32(p_data, sstrlen(p_data), 1234U)), 0x0f2cc00bU);
		SLCHECK_EQ(SlEqualityTest_gravity_murmur3_32(p_data, sstrlen(p_data), 1234), rh);
		//
		{
			//uint32 h128[4];
			binary128 _h128;
			p_data = "Hello, world!";
			_h128 = SlHash::Murmur3_128x32(p_data, sstrlen(p_data), 123);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0x61c9129eU);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0x5a1aacd7U);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0xa4162162U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0x9e37c886U);
			_h128 = SlHash::Murmur3_128x32(p_data, sstrlen(p_data), 321);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0xd5fbdcb3U);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0xc26c4193U);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0x045880c5U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0xa7170f0fU);
			p_data = "xxxxxxxxxxxxxxxxxxxxxxxxxxxx";
			_h128 = SlHash::Murmur3_128x32(p_data, sstrlen(p_data), 123);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0x5e40bab2U);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0x78825a16U);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0x4cf929d3U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0x1fec6047U);
			p_data = "";
			_h128 = SlHash::Murmur3_128x32(p_data, sstrlen(p_data), 123);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0xfedc5245U);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0x26f3e799U);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0x26f3e799U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0x26f3e799U);

			p_data = "Hello, world!";
			_h128 = SlHash::Murmur3_128x64(p_data, sstrlen(p_data), 123);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0x8743acadU);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0x421c8c73U);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0xd373c3f5U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0xf19732fdU);
			_h128 = SlHash::Murmur3_128x64(p_data, sstrlen(p_data), 321);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0xf86d4004U);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0xca47f42bU);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0xb9546c79U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0x79200aeeU);
			p_data = "xxxxxxxxxxxxxxxxxxxxxxxxxxxx";
			_h128 = SlHash::Murmur3_128x64(p_data, sstrlen(p_data), 123);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0xbecf7e04U);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0xdbcf7463U);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0x7751664eU);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0xf66e73e0U);
			p_data = "";
			_h128 = SlHash::Murmur3_128x64(p_data, sstrlen(p_data), 123);
			SLCHECK_EQ(PTR32C(_h128.D)[0], 0x4cd95970U);
			SLCHECK_EQ(PTR32C(_h128.D)[1], 0x81679d1aU);
			SLCHECK_EQ(PTR32C(_h128.D)[2], 0xbd92f878U);
			SLCHECK_EQ(PTR32C(_h128.D)[3], 0x4bace33dU);
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
//
// check that every input bit changes every output bit half the time
//
static void HashFunc_BobJenkins_driver2()
{
	uint8 qa[BOBJEN_MAXLEN+1], qb[BOBJEN_MAXLEN+2], * a = &qa[0], * b = &qb[1];
	uint32 c[BOBJEN_HASHSTATE], d[BOBJEN_HASHSTATE], i = 0, j = 0, k, l, m = 0, z;
	uint32 e[BOBJEN_HASHSTATE], f[BOBJEN_HASHSTATE], g[BOBJEN_HASHSTATE], h[BOBJEN_HASHSTATE];
	uint32 x[BOBJEN_HASHSTATE], y[BOBJEN_HASHSTATE];
	uint32 hlen;

	printf("No more than %d trials should ever be needed \n", BOBJEN_MAXPAIR/2);
	for(hlen = 0; hlen < BOBJEN_MAXLEN; ++hlen) {
		z = 0;
		for(i = 0; i<hlen; ++i) { /*----------------------- for each input byte, */
			for(j = 0; j<8; ++j) { /*------------------------ for each input bit, */
				for(m = 1; m<8; ++m) { /*------------ for serveral possible initvals, */
					for(l = 0; l < BOBJEN_HASHSTATE; ++l)
						e[l] = f[l] = g[l] = h[l] = x[l] = y[l] = ~((uint32)0);

					/*---- check that every output bit is affected by that input bit */
					for(k = 0; k < BOBJEN_MAXPAIR; k += 2) {
						uint32 finished = 1;
						/* keys have one bit different */
						for(l = 0; l<hlen+1; ++l) {
							a[l] = b[l] = (uint8)0;
						}
						/* have a and b be two keys differing in only one bit */
						a[i] ^= (k << j);
						a[i] ^= (k >> (8-j));
						c[0] = SlHash::BobJencHash_Little(a, hlen, m);
						b[i] ^= ((k+1) << j);
						b[i] ^= ((k+1) >> (8-j));
						d[0] = SlHash::BobJencHash_Little(b, hlen, m);
						/* check every bit is 1, 0, set, and not set at least once */
						for(l = 0; l < BOBJEN_HASHSTATE; ++l) {
							e[l] &= (c[l]^d[l]);
							f[l] &= ~(c[l]^d[l]);
							g[l] &= c[l];
							h[l] &= ~c[l];
							x[l] &= d[l];
							y[l] &= ~d[l];
							if(e[l]|f[l]|g[l]|h[l]|x[l]|y[l])
								finished = 0;
						}
						if(finished)
							break;
					}
					if(k > z)
						z = k;
					if(k == BOBJEN_MAXPAIR) {
						printf("Some bit didn't change: ");
						printf("%.8x %.8x %.8x %.8x %.8x %.8x  ", e[0], f[0], g[0], h[0], x[0], y[0]);
						printf("i %d j %d m %d len %d\n", i, j, m, hlen);
					}
					if(z == BOBJEN_MAXPAIR)
						goto done;
				}
			}
		}
done:
		if(z < BOBJEN_MAXPAIR) {
			printf("Mix success  %2d bytes  %2d initvals  ", i, m);
			printf("required  %d  trials\n", z/2);
		}
	}
	printf("\n");
}
//
// Реализация sh256 из LZMA. Для тестирования идентичности с целью замены на собственную имплементацию.
//
// Rotate a uint32_t. GCC can optimize this to a rotate instruction at least on x86.
//static inline uint32_t rotr_32(uint32_t num, unsigned amount) { return (num >> amount) | (num << (32 - amount)); }

#ifdef SL_BIGENDIAN
	#ifndef conv32be
		#define conv32be(num) ((uint32_t)(num))
	#endif
	#ifndef conv64be
		#define conv64be(num) ((uint64_t)(num))
	#endif
#else
	#ifndef conv32be
		#define conv32be(num) sbswap32(num)
	#endif
	#ifndef conv64be
		#define conv64be(num) sbswap64(num)
	#endif
#endif

#define LZMA_blk0(i) (W[i] = conv32be(data[i]))
#define LZMA_blk2(i) (W[i & 15] += LZMA_s1(W[(i - 2) & 15]) + W[(i - 7) & 15] + LZMA_s0(W[(i - 15) & 15]))
#define LZMA_Ch(x, y, z) (z ^ (x & (y ^ z)))
#define LZMA_Maj(x, y, z) ((x & (y ^ z)) + (y & z))
#define LZMA_a(i) T[(0 - i) & 7]
#define LZMA_b(i) T[(1 - i) & 7]
#define LZMA_c(i) T[(2 - i) & 7]
#define LZMA_d(i) T[(3 - i) & 7]
#define LZMA_e(i) T[(4 - i) & 7]
#define LZMA_f(i) T[(5 - i) & 7]
#define LZMA_g(i) T[(6 - i) & 7]
#define LZMA_h(i) T[(7 - i) & 7]

#define LZMA_R(i, j, blk) LZMA_h(i) += LZMA_S1(LZMA_e(i)) + LZMA_Ch(LZMA_e(i), LZMA_f(i), LZMA_g(i)) + SHA256_K[i + j] + blk; LZMA_d(i) += LZMA_h(i); LZMA_h(i) += LZMA_S0(LZMA_a(i)) + LZMA_Maj(LZMA_a(i), LZMA_b(i), LZMA_c(i))
#define LZMA_R0(i) LZMA_R(i, 0, LZMA_blk0(i))
#define LZMA_R2(i) LZMA_R(i, j, LZMA_blk2(i))

#define LZMA_S0(x) SBits::Rotr(x ^ SBits::Rotr(x ^ SBits::Rotr(x, 9), 11), 2)
#define LZMA_S1(x) SBits::Rotr(x ^ SBits::Rotr(x ^ SBits::Rotr(x, 14), 5), 6)
#define LZMA_s0(x) (SBits::Rotr(x ^ SBits::Rotr(x, 11), 7) ^ (x >> 3))
#define LZMA_s1(x) (SBits::Rotr(x ^ SBits::Rotr(x, 2), 17) ^ (x >> 10))

static void process(lzma_check_state * check)
{
	//LZMA_SHA256_transform(check->state.sha256.state, check->buffer.u32);
	//static void LZMA_SHA256_transform(uint32 state[8], const uint32 data[16])
	uint32 * state = check->state.sha256.state;
	const uint32 * data = check->buffer.u32;
	{
		static const uint32 SHA256_K[64] = {
			0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
			0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
			0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
			0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
			0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
			0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
			0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
			0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
			0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
			0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
			0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
			0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
			0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
			0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
			0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
			0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
		};
		uint32_t W[16];
		uint32_t T[8];
		// Copy state[] to working vars.
		memcpy(T, state, sizeof(T));
		// The first 16 operations unrolled
		LZMA_R0(0); 
		LZMA_R0(1); 
		LZMA_R0(2); 
		LZMA_R0(3);
		LZMA_R0(4); 
		LZMA_R0(5); 
		LZMA_R0(6); 
		LZMA_R0(7);
		LZMA_R0(8); 
		LZMA_R0(9); 
		LZMA_R0(10); 
		LZMA_R0(11);
		LZMA_R0(12); 
		LZMA_R0(13); 
		LZMA_R0(14); 
		LZMA_R0(15);
		// The remaining 48 operations partially unrolled
		for(unsigned int j = 16; j < 64; j += 16) {
			LZMA_R2(0); 
			LZMA_R2(1); 
			LZMA_R2(2); 
			LZMA_R2(3);
			LZMA_R2(4); 
			LZMA_R2(5); 
			LZMA_R2(6); 
			LZMA_R2(7);
			LZMA_R2(8); 
			LZMA_R2(9); 
			LZMA_R2(10); 
			LZMA_R2(11);
			LZMA_R2(12); 
			LZMA_R2(13); 
			LZMA_R2(14); 
			LZMA_R2(15);
		}
		// Add the working vars back into state[].
		state[0] += LZMA_a(0);
		state[1] += LZMA_b(0);
		state[2] += LZMA_c(0);
		state[3] += LZMA_d(0);
		state[4] += LZMA_e(0);
		state[5] += LZMA_f(0);
		state[6] += LZMA_g(0);
		state[7] += LZMA_h(0);
	}
}

static void __lzma_sha256_init(lzma_check_state * check)
{
	static const uint32_t s[8] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19, };
	memcpy(check->state.sha256.state, s, sizeof(s));
	check->state.sha256.size = 0;
}

static void __lzma_sha256_update(const uint8 * buf, size_t size, lzma_check_state * check)
{
	// Copy the input data into a properly aligned temporary buffer.
	// This way we can be called with arbitrarily sized buffers
	// (no need to be multiple of 64 bytes), and the code works also
	// on architectures that don't allow unaligned memory access.
	while(size > 0) {
		const size_t copy_start = static_cast<size_t>(check->state.sha256.size & 0x3F);
		size_t copy_size = 64 - copy_start;
		if(copy_size > size)
			copy_size = size;
		memcpy(check->buffer.u8 + copy_start, buf, copy_size);
		buf += copy_size;
		size -= copy_size;
		check->state.sha256.size += copy_size;
		if((check->state.sha256.size & 0x3F) == 0)
			process(check);
	}
}

static void __lzma_sha256_finish(lzma_check_state * check)
{
	// Add padding as described in RFC 3174 (it describes SHA-1 but
	// the same padding style is used for SHA-256 too).
	size_t pos = static_cast<size_t>(check->state.sha256.size & 0x3F);
	check->buffer.u8[pos++] = 0x80;
	while(pos != 64 - 8) {
		if(pos == 64) {
			process(check);
			pos = 0;
		}
		check->buffer.u8[pos++] = 0x00;
	}
	// Convert the message size from bytes to bits.
	check->state.sha256.size *= 8;
	check->buffer.u64[(64 - 8) / 8] = conv64be(check->state.sha256.size);
	process(check);
	for(size_t i = 0; i < 8; ++i)
		check->buffer.u32[i] = conv32be(check->state.sha256.state[i]);
}
//
//
//
// test_fnv - FNV (Fowler/Noll/Vo) test suite
//
// these test vectors are used as part o the FNV test suite
//
struct test_vector {
	const void * buf; // start of test vector buffer
	size_t len; // length of test vector
};

struct fnv0_32_test_vector {
	struct test_vector * test; // test vector buffer to hash
	uint32 fnv0_32; // expected FNV-0 32 bit hash value
};

struct fnv1_32_test_vector {
	struct test_vector * test; // test vector buffer to hash
	uint32 fnv1_32; /* expected FNV-1 32 bit hash value */
};

struct fnv1a_32_test_vector {
	struct test_vector * test; // test vector buffer to hash
	uint32 fnv1a_32; /* expected FNV-1a 32 bit hash value */
};

struct fnv0_64_test_vector {
	struct test_vector * test; // test vector buffer to hash
	uint64 fnv0_64; /* expected FNV-0 64 bit hash value */
};

struct fnv1_64_test_vector {
	struct test_vector * test; // test vector buffer to hash
	uint64 fnv1_64; // expected FNV-1 64 bit hash value
};

struct fnv1a_64_test_vector {
	struct test_vector * test; // test vector buffer to hash
	uint64 fnv1a_64; // expected FNV-1a 64 bit hash value
};

#define TEST(x) {x, (sizeof(x)-1)} /* TEST macro does not include trailing NUL byte in the test vector */
#define TEST0(x) {x, sizeof(x)} /* TEST0 macro includes the trailing NUL byte in the test vector */
/* REPEAT500 - repeat a string 500 times */
#define R500(x) R100(x) R100(x) R100(x) R100(x) R100(x)
#define R100(x) R10(x) R10(x) R10(x) R10(x) R10(x) R10(x) R10(x) R10(x) R10(x) R10(x)
#define R10(x) x x x x x x x x x x

/*
 * FNV test vectors
 *
 * NOTE: A NULL pointer marks beyond the end of the test vectors.
 *
 * NOTE: The order of the _fnv_test_str[] test vectors is 1-to-1 with:
 *
 *	struct fnv0_32_test_vector fnv0_32_vector[];
 *	struct fnv1_32_test_vector fnv1_32_vector[];
 *	struct fnv1a_32_test_vector fnv1a_32_vector[];
 *	struct fnv0_64_test_vector fnv0_64_vector[];
 *	struct fnv1_64_test_vector fnv1_64_vector[];
 *	struct fnv1a_64_test_vector fnv1a_64_vector[];
 *
 * IMPORTANT NOTE:
 *
 *	If you change the _fnv_test_str[] array, you need
 *	to also change ALL of the above fnv*_vector arrays!!!
 *	To rebuild, try:
 *		make vector.c
 *	and then fold the results into the source file.
 *	Of course, you better make sure that the vaules
 *	produced by the above command are valid, otherwise
 * 	you will be testing against invalid vectors!
 */
static struct test_vector _fnv_test_str[] = {
	TEST(""),
	TEST("a"),
	TEST("b"),
	TEST("c"),
	TEST("d"),
	TEST("e"),
	TEST("f"),
	TEST("fo"),
	TEST("foo"),
	TEST("foob"),
	TEST("fooba"),
	TEST("foobar"),
	TEST0(""),
	TEST0("a"),
	TEST0("b"),
	TEST0("c"),
	TEST0("d"),
	TEST0("e"),
	TEST0("f"),
	TEST0("fo"),
	TEST0("foo"),
	TEST0("foob"),
	TEST0("fooba"),
	TEST0("foobar"),
	TEST("ch"),
	TEST("cho"),
	TEST("chon"),
	TEST("chong"),
	TEST("chongo"),
	TEST("chongo "),
	TEST("chongo w"),
	TEST("chongo wa"),
	TEST("chongo was"),
	TEST("chongo was "),
	TEST("chongo was h"),
	TEST("chongo was he"),
	TEST("chongo was her"),
	TEST("chongo was here"),
	TEST("chongo was here!"),
	TEST("chongo was here!\n"),
	TEST0("ch"),
	TEST0("cho"),
	TEST0("chon"),
	TEST0("chong"),
	TEST0("chongo"),
	TEST0("chongo "),
	TEST0("chongo w"),
	TEST0("chongo wa"),
	TEST0("chongo was"),
	TEST0("chongo was "),
	TEST0("chongo was h"),
	TEST0("chongo was he"),
	TEST0("chongo was her"),
	TEST0("chongo was here"),
	TEST0("chongo was here!"),
	TEST0("chongo was here!\n"),
	TEST("cu"),
	TEST("cur"),
	TEST("curd"),
	TEST("curds"),
	TEST("curds "),
	TEST("curds a"),
	TEST("curds an"),
	TEST("curds and"),
	TEST("curds and "),
	TEST("curds and w"),
	TEST("curds and wh"),
	TEST("curds and whe"),
	TEST("curds and whey"),
	TEST("curds and whey\n"),
	TEST0("cu"),
	TEST0("cur"),
	TEST0("curd"),
	TEST0("curds"),
	TEST0("curds "),
	TEST0("curds a"),
	TEST0("curds an"),
	TEST0("curds and"),
	TEST0("curds and "),
	TEST0("curds and w"),
	TEST0("curds and wh"),
	TEST0("curds and whe"),
	TEST0("curds and whey"),
	TEST0("curds and whey\n"),
	TEST("hi"), TEST0("hi"),
	TEST("hello"), TEST0("hello"),
	TEST("\xff\x00\x00\x01"), TEST("\x01\x00\x00\xff"),
	TEST("\xff\x00\x00\x02"), TEST("\x02\x00\x00\xff"),
	TEST("\xff\x00\x00\x03"), TEST("\x03\x00\x00\xff"),
	TEST("\xff\x00\x00\x04"), TEST("\x04\x00\x00\xff"),
	TEST("\x40\x51\x4e\x44"), TEST("\x44\x4e\x51\x40"),
	TEST("\x40\x51\x4e\x4a"), TEST("\x4a\x4e\x51\x40"),
	TEST("\x40\x51\x4e\x54"), TEST("\x54\x4e\x51\x40"),
	TEST("127.0.0.1"), TEST0("127.0.0.1"),
	TEST("127.0.0.2"), TEST0("127.0.0.2"),
	TEST("127.0.0.3"), TEST0("127.0.0.3"),
	TEST("64.81.78.68"), TEST0("64.81.78.68"),
	TEST("64.81.78.74"), TEST0("64.81.78.74"),
	TEST("64.81.78.84"), TEST0("64.81.78.84"),
	TEST("feedface"), TEST0("feedface"),
	TEST("feedfacedaffdeed"), TEST0("feedfacedaffdeed"),
	TEST("feedfacedeadbeef"), TEST0("feedfacedeadbeef"),
	TEST("line 1\nline 2\nline 3"),
	TEST("chongo <Landon Curt Noll> /\\../\\"),
	TEST0("chongo <Landon Curt Noll> /\\../\\"),
	TEST("chongo (Landon Curt Noll) /\\../\\"),
	TEST0("chongo (Landon Curt Noll) /\\../\\"),
	TEST("http://antwrp.gsfc.nasa.gov/apod/astropix.html"),
	TEST("http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash"),
	TEST("http://epod.usra.edu/"),
	TEST("http://exoplanet.eu/"),
	TEST("http://hvo.wr.usgs.gov/cam3/"),
	TEST("http://hvo.wr.usgs.gov/cams/HMcam/"),
	TEST("http://hvo.wr.usgs.gov/kilauea/update/deformation.html"),
	TEST("http://hvo.wr.usgs.gov/kilauea/update/images.html"),
	TEST("http://hvo.wr.usgs.gov/kilauea/update/maps.html"),
	TEST("http://hvo.wr.usgs.gov/volcanowatch/current_issue.html"),
	TEST("http://neo.jpl.nasa.gov/risk/"),
	TEST("http://norvig.com/21-days.html"),
	TEST("http://primes.utm.edu/curios/home.php"),
	TEST("http://slashdot.org/"),
	TEST("http://tux.wr.usgs.gov/Maps/155.25-19.5.html"),
	TEST("http://volcano.wr.usgs.gov/kilaueastatus.php"),
	TEST("http://www.avo.alaska.edu/activity/Redoubt.php"),
	TEST("http://www.dilbert.com/fast/"),
	TEST("http://www.fourmilab.ch/gravitation/orbits/"),
	TEST("http://www.fpoa.net/"),
	TEST("http://www.ioccc.org/index.html"),
	TEST("http://www.isthe.com/cgi-bin/number.cgi"),
	TEST("http://www.isthe.com/chongo/bio.html"),
	TEST("http://www.isthe.com/chongo/index.html"),
	TEST("http://www.isthe.com/chongo/src/calc/lucas-calc"),
	TEST("http://www.isthe.com/chongo/tech/astro/venus2004.html"),
	TEST("http://www.isthe.com/chongo/tech/astro/vita.html"),
	TEST("http://www.isthe.com/chongo/tech/comp/c/expert.html"),
	TEST("http://www.isthe.com/chongo/tech/comp/calc/index.html"),
	TEST("http://www.isthe.com/chongo/tech/comp/fnv/index.html"),
	TEST("http://www.isthe.com/chongo/tech/math/number/howhigh.html"),
	TEST("http://www.isthe.com/chongo/tech/math/number/number.html"),
	TEST("http://www.isthe.com/chongo/tech/math/prime/mersenne.html"),
	TEST("http://www.isthe.com/chongo/tech/math/prime/mersenne.html#largest"),
	TEST("http://www.lavarnd.org/cgi-bin/corpspeak.cgi"),
	TEST("http://www.lavarnd.org/cgi-bin/haiku.cgi"),
	TEST("http://www.lavarnd.org/cgi-bin/rand-none.cgi"),
	TEST("http://www.lavarnd.org/cgi-bin/randdist.cgi"),
	TEST("http://www.lavarnd.org/index.html"),
	TEST("http://www.lavarnd.org/what/nist-test.html"),
	TEST("http://www.macosxhints.com/"),
	TEST("http://www.mellis.com/"),
	TEST("http://www.nature.nps.gov/air/webcams/parks/havoso2alert/havoalert.cfm"),
	TEST("http://www.nature.nps.gov/air/webcams/parks/havoso2alert/timelines_24.cfm"),
	TEST("http://www.paulnoll.com/"),
	TEST("http://www.pepysdiary.com/"),
	TEST("http://www.sciencenews.org/index/home/activity/view"),
	TEST("http://www.skyandtelescope.com/"),
	TEST("http://www.sput.nl/~rob/sirius.html"),
	TEST("http://www.systemexperts.com/"),
	TEST("http://www.tq-international.com/phpBB3/index.php"),
	TEST("http://www.travelquesttours.com/index.htm"),
	TEST("http://www.wunderground.com/global/stations/89606.html"),
	TEST(R10("21701")),
	TEST(R10("M21701")),
	TEST(R10("2^21701-1")),
	TEST(R10("\x54\xc5")),
	TEST(R10("\xc5\x54")),
	TEST(R10("23209")),
	TEST(R10("M23209")),
	TEST(R10("2^23209-1")),
	TEST(R10("\x5a\xa9")),
	TEST(R10("\xa9\x5a")),
	TEST(R10("391581216093")),
	TEST(R10("391581*2^216093-1")),
	TEST(R10("\x05\xf9\x9d\x03\x4c\x81")),
	TEST(R10("FEDCBA9876543210")),
	TEST(R10("\xfe\xdc\xba\x98\x76\x54\x32\x10")),
	TEST(R10("EFCDAB8967452301")),
	TEST(R10("\xef\xcd\xab\x89\x67\x45\x23\x01")),
	TEST(R10("0123456789ABCDEF")),
	TEST(R10("\x01\x23\x45\x67\x89\xab\xcd\xef")),
	TEST(R10("1032547698BADCFE")),
	TEST(R10("\x10\x32\x54\x76\x98\xba\xdc\xfe")),
	TEST(R500("\x00")),
	TEST(R500("\x07")),
	TEST(R500("~")),
	TEST(R500("\x7f")),
	{NULL, 0} /* MUST BE LAST */
};

/* FNV-0 32 bit test vectors */
static const struct fnv0_32_test_vector fnv0_32_vector[] = {
	{ &_fnv_test_str[0], (uint32)0x00000000UL },
	{ &_fnv_test_str[1], (uint32)0x00000061UL },
	{ &_fnv_test_str[2], (uint32)0x00000062UL },
	{ &_fnv_test_str[3], (uint32)0x00000063UL },
	{ &_fnv_test_str[4], (uint32)0x00000064UL },
	{ &_fnv_test_str[5], (uint32)0x00000065UL },
	{ &_fnv_test_str[6], (uint32)0x00000066UL },
	{ &_fnv_test_str[7], (uint32)0x6600a0fdUL },
	{ &_fnv_test_str[8], (uint32)0x8ffd6e28UL },
	{ &_fnv_test_str[9], (uint32)0xd3f4689aUL },
	{ &_fnv_test_str[10], (uint32)0x43c0aa0fUL },
	{ &_fnv_test_str[11], (uint32)0xb74bb5efUL },
	{ &_fnv_test_str[12], (uint32)0x00000000UL },
	{ &_fnv_test_str[13], (uint32)0x610098b3UL },
	{ &_fnv_test_str[14], (uint32)0x62009a46UL },
	{ &_fnv_test_str[15], (uint32)0x63009bd9UL },
	{ &_fnv_test_str[16], (uint32)0x64009d6cUL },
	{ &_fnv_test_str[17], (uint32)0x65009effUL },
	{ &_fnv_test_str[18], (uint32)0x6600a092UL },
	{ &_fnv_test_str[19], (uint32)0x8ffd6e47UL },
	{ &_fnv_test_str[20], (uint32)0xd3f468f8UL },
	{ &_fnv_test_str[21], (uint32)0x43c0aa6eUL },
	{ &_fnv_test_str[22], (uint32)0xb74bb59dUL },
	{ &_fnv_test_str[23], (uint32)0x7b2f673dUL },
	{ &_fnv_test_str[24], (uint32)0x63009bb1UL },
	{ &_fnv_test_str[25], (uint32)0x8af517ccUL },
	{ &_fnv_test_str[26], (uint32)0x8bd4764aUL },
	{ &_fnv_test_str[27], (uint32)0x69763619UL },
	{ &_fnv_test_str[28], (uint32)0x1e172934UL },
	{ &_fnv_test_str[29], (uint32)0x9275dcfcUL },
	{ &_fnv_test_str[30], (uint32)0x8b8ae0c3UL },
	{ &_fnv_test_str[31], (uint32)0x6e9fd298UL },
	{ &_fnv_test_str[32], (uint32)0xbd98853bUL },
	{ &_fnv_test_str[33], (uint32)0xb219bbc1UL },
	{ &_fnv_test_str[34], (uint32)0x1f8290bbUL },
	{ &_fnv_test_str[35], (uint32)0x5589d604UL },
	{ &_fnv_test_str[36], (uint32)0xabfbe83eUL },
	{ &_fnv_test_str[37], (uint32)0xfb8e99ffUL },
	{ &_fnv_test_str[38], (uint32)0x007c6c4cUL },
	{ &_fnv_test_str[39], (uint32)0x0fde7baeUL },
	{ &_fnv_test_str[40], (uint32)0x8af517a3UL },
	{ &_fnv_test_str[41], (uint32)0x8bd47624UL },
	{ &_fnv_test_str[42], (uint32)0x6976367eUL },
	{ &_fnv_test_str[43], (uint32)0x1e17295bUL },
	{ &_fnv_test_str[44], (uint32)0x9275dcdcUL },
	{ &_fnv_test_str[45], (uint32)0x8b8ae0b4UL },
	{ &_fnv_test_str[46], (uint32)0x6e9fd2f9UL },
	{ &_fnv_test_str[47], (uint32)0xbd988548UL },
	{ &_fnv_test_str[48], (uint32)0xb219bbe1UL },
	{ &_fnv_test_str[49], (uint32)0x1f8290d3UL },
	{ &_fnv_test_str[50], (uint32)0x5589d661UL },
	{ &_fnv_test_str[51], (uint32)0xabfbe84cUL },
	{ &_fnv_test_str[52], (uint32)0xfb8e999aUL },
	{ &_fnv_test_str[53], (uint32)0x007c6c6dUL },
	{ &_fnv_test_str[54], (uint32)0x0fde7ba4UL },
	{ &_fnv_test_str[55], (uint32)0xa93cb2eaUL },
	{ &_fnv_test_str[56], (uint32)0x63009bacUL },
	{ &_fnv_test_str[57], (uint32)0x85f50fb6UL },
	{ &_fnv_test_str[58], (uint32)0x96c7bbe6UL },
	{ &_fnv_test_str[59], (uint32)0x426ccb61UL },
	{ &_fnv_test_str[60], (uint32)0xf2442993UL },
	{ &_fnv_test_str[61], (uint32)0xf44d7208UL },
	{ &_fnv_test_str[62], (uint32)0x9dea82f6UL },
	{ &_fnv_test_str[63], (uint32)0x8e2c2926UL },
	{ &_fnv_test_str[64], (uint32)0xf584c6f2UL },
	{ &_fnv_test_str[65], (uint32)0x72052e81UL },
	{ &_fnv_test_str[66], (uint32)0xff28357bUL },
	{ &_fnv_test_str[67], (uint32)0x274c30c4UL },
	{ &_fnv_test_str[68], (uint32)0xa0f0c4f5UL },
	{ &_fnv_test_str[69], (uint32)0x50060da5UL },
	{ &_fnv_test_str[70], (uint32)0x85f50fc4UL },
	{ &_fnv_test_str[71], (uint32)0x96c7bb82UL },
	{ &_fnv_test_str[72], (uint32)0x426ccb12UL },
	{ &_fnv_test_str[73], (uint32)0xf24429b3UL },
	{ &_fnv_test_str[74], (uint32)0xf44d7269UL },
	{ &_fnv_test_str[75], (uint32)0x9dea8298UL },
	{ &_fnv_test_str[76], (uint32)0x8e2c2942UL },
	{ &_fnv_test_str[77], (uint32)0xf584c6d2UL },
	{ &_fnv_test_str[78], (uint32)0x72052ef6UL },
	{ &_fnv_test_str[79], (uint32)0xff283513UL },
	{ &_fnv_test_str[80], (uint32)0x274c30a1UL },
	{ &_fnv_test_str[81], (uint32)0xa0f0c48cUL },
	{ &_fnv_test_str[82], (uint32)0x50060dafUL },
	{ &_fnv_test_str[83], (uint32)0x9e877abfUL },
	{ &_fnv_test_str[84], (uint32)0x6800a3d1UL },
	{ &_fnv_test_str[85], (uint32)0x8a01e203UL },
	{ &_fnv_test_str[86], (uint32)0xec6d6be8UL },
	{ &_fnv_test_str[87], (uint32)0x1840de38UL },
	{ &_fnv_test_str[88], (uint32)0xa7cc97b4UL },
	{ &_fnv_test_str[89], (uint32)0x3ee6b3b4UL },
	{ &_fnv_test_str[90], (uint32)0xa7cc97b7UL },
	{ &_fnv_test_str[91], (uint32)0x7dcd6669UL },
	{ &_fnv_test_str[92], (uint32)0xa7cc97b6UL },
	{ &_fnv_test_str[93], (uint32)0xbcb4191eUL },
	{ &_fnv_test_str[94], (uint32)0xa7cc97b1UL },
	{ &_fnv_test_str[95], (uint32)0xfb9acdd3UL },
	{ &_fnv_test_str[96], (uint32)0x89380433UL },
	{ &_fnv_test_str[97], (uint32)0x8acd2855UL },
	{ &_fnv_test_str[98], (uint32)0x8938043dUL },
	{ &_fnv_test_str[99], (uint32)0xcaeed493UL },
	{ &_fnv_test_str[100], (uint32)0x89380423UL },
	{ &_fnv_test_str[101], (uint32)0x59382a25UL },
	{ &_fnv_test_str[102], (uint32)0x567f75d7UL },
	{ &_fnv_test_str[103], (uint32)0x01a68175UL },
	{ &_fnv_test_str[104], (uint32)0x567f75d4UL },
	{ &_fnv_test_str[105], (uint32)0xfea67cbcUL },
	{ &_fnv_test_str[106], (uint32)0x567f75d5UL },
	{ &_fnv_test_str[107], (uint32)0xffa67e4fUL },
	{ &_fnv_test_str[108], (uint32)0xd131b668UL },
	{ &_fnv_test_str[109], (uint32)0xb94225b8UL },
	{ &_fnv_test_str[110], (uint32)0xd231b7d7UL },
	{ &_fnv_test_str[111], (uint32)0xbb446775UL },
	{ &_fnv_test_str[112], (uint32)0xdf31cc6eUL },
	{ &_fnv_test_str[113], (uint32)0xc964d12aUL },
	{ &_fnv_test_str[114], (uint32)0x23af8f9fUL },
	{ &_fnv_test_str[115], (uint32)0xcc5f174dUL },
	{ &_fnv_test_str[116], (uint32)0x96b29b8cUL },
	{ &_fnv_test_str[117], (uint32)0xc72add64UL },
	{ &_fnv_test_str[118], (uint32)0x528fb7efUL },
	{ &_fnv_test_str[119], (uint32)0xe73e8d3dUL },
	{ &_fnv_test_str[120], (uint32)0x876386feUL },
	{ &_fnv_test_str[121], (uint32)0x811c9dc5UL },
	{ &_fnv_test_str[122], (uint32)0x050c5d1fUL },
	{ &_fnv_test_str[123], (uint32)0x14bf7238UL },
	{ &_fnv_test_str[124], (uint32)0xe160ce28UL },
	{ &_fnv_test_str[125], (uint32)0x89dc5a75UL },
	{ &_fnv_test_str[126], (uint32)0xd89b69a0UL },
	{ &_fnv_test_str[127], (uint32)0x94471a88UL },
	{ &_fnv_test_str[128], (uint32)0xe78db65fUL },
	{ &_fnv_test_str[129], (uint32)0x0c3009a2UL },
	{ &_fnv_test_str[130], (uint32)0x122dff03UL },
	{ &_fnv_test_str[131], (uint32)0xb4cd8875UL },
	{ &_fnv_test_str[132], (uint32)0xf4dba725UL },
	{ &_fnv_test_str[133], (uint32)0x41a16560UL },
	{ &_fnv_test_str[134], (uint32)0x9c0f941fUL },
	{ &_fnv_test_str[135], (uint32)0x451a5348UL },
	{ &_fnv_test_str[136], (uint32)0x3f1d1d89UL },
	{ &_fnv_test_str[137], (uint32)0x1b91b57aUL },
	{ &_fnv_test_str[138], (uint32)0x3e99b577UL },
	{ &_fnv_test_str[139], (uint32)0x4c9de07aUL },
	{ &_fnv_test_str[140], (uint32)0x1ddf7572UL },
	{ &_fnv_test_str[141], (uint32)0x64e81976UL },
	{ &_fnv_test_str[142], (uint32)0x1106a888UL },
	{ &_fnv_test_str[143], (uint32)0xa498d8e5UL },
	{ &_fnv_test_str[144], (uint32)0x3c03d2e3UL },
	{ &_fnv_test_str[145], (uint32)0x26568b28UL },
	{ &_fnv_test_str[146], (uint32)0x70d7fb42UL },
	{ &_fnv_test_str[147], (uint32)0xd3ae1d22UL },
	{ &_fnv_test_str[148], (uint32)0xac8ea5f4UL },
	{ &_fnv_test_str[149], (uint32)0x4d0abd60UL },
	{ &_fnv_test_str[150], (uint32)0x48f5e086UL },
	{ &_fnv_test_str[151], (uint32)0xa8f6241bUL },
	{ &_fnv_test_str[152], (uint32)0x572f864fUL },
	{ &_fnv_test_str[153], (uint32)0xa5340803UL },
	{ &_fnv_test_str[154], (uint32)0x22881aa8UL },
	{ &_fnv_test_str[155], (uint32)0xc2e2f5a2UL },
	{ &_fnv_test_str[156], (uint32)0xebf5aec7UL },
	{ &_fnv_test_str[157], (uint32)0x3cdbfb85UL },
	{ &_fnv_test_str[158], (uint32)0xbb859704UL },
	{ &_fnv_test_str[159], (uint32)0xc956fe11UL },
	{ &_fnv_test_str[160], (uint32)0x8f11a7c9UL },
	{ &_fnv_test_str[161], (uint32)0x36c48ecfUL },
	{ &_fnv_test_str[162], (uint32)0x24bfa27eUL },
	{ &_fnv_test_str[163], (uint32)0xf2596ad1UL },
	{ &_fnv_test_str[164], (uint32)0xf14a9b45UL },
	{ &_fnv_test_str[165], (uint32)0x7d45835aUL },
	{ &_fnv_test_str[166], (uint32)0x6e49334dUL },
	{ &_fnv_test_str[167], (uint32)0x71767337UL },
	{ &_fnv_test_str[168], (uint32)0x858a1a8aUL },
	{ &_fnv_test_str[169], (uint32)0x16e75ac2UL },
	{ &_fnv_test_str[170], (uint32)0x409f99dfUL },
	{ &_fnv_test_str[171], (uint32)0x6d6652ddUL },
	{ &_fnv_test_str[172], (uint32)0x2761a9ffUL },
	{ &_fnv_test_str[173], (uint32)0x41f0d616UL },
	{ &_fnv_test_str[174], (uint32)0x0e2d0d0fUL },
	{ &_fnv_test_str[175], (uint32)0x06adc8fdUL },
	{ &_fnv_test_str[176], (uint32)0x60e0d4b9UL },
	{ &_fnv_test_str[177], (uint32)0x5ddc79d3UL },
	{ &_fnv_test_str[178], (uint32)0x1e6d0b46UL },
	{ &_fnv_test_str[179], (uint32)0x1d1514d8UL },
	{ &_fnv_test_str[180], (uint32)0xb1903a4eUL },
	{ &_fnv_test_str[181], (uint32)0x8200c318UL },
	{ &_fnv_test_str[182], (uint32)0x15e22888UL },
	{ &_fnv_test_str[183], (uint32)0x57591760UL },
	{ &_fnv_test_str[184], (uint32)0x02462efcUL },
	{ &_fnv_test_str[185], (uint32)0x7651ec44UL },
	{ &_fnv_test_str[186], (uint32)0x7c24e9d4UL },
	{ &_fnv_test_str[187], (uint32)0x1952a034UL },
	{ &_fnv_test_str[188], (uint32)0xd4c46864UL },
	{ &_fnv_test_str[189], (uint32)0xcb57cde0UL },
	{ &_fnv_test_str[190], (uint32)0x71136a70UL },
	{ &_fnv_test_str[191], (uint32)0x0618fb40UL },
	{ &_fnv_test_str[192], (uint32)0x69a24fc0UL },
	{ &_fnv_test_str[193], (uint32)0x6a9be510UL },
	{ &_fnv_test_str[194], (uint32)0xe0477040UL },
	{ &_fnv_test_str[195], (uint32)0x85aa94b0UL },
	{ &_fnv_test_str[196], (uint32)0xc6d76240UL },
	{ &_fnv_test_str[197], (uint32)0xa9f09e40UL },
	{ &_fnv_test_str[198], (uint32)0xa0291540UL },
	{ &_fnv_test_str[199], (uint32)0x00000000UL },
	{ &_fnv_test_str[200], (uint32)0x2e672aa4UL },
	{ &_fnv_test_str[201], (uint32)0x84b1aa48UL },
	{ &_fnv_test_str[202], (uint32)0xfc24ba24UL },
	{ NULL, 0 }
};

/* FNV-1 32 bit test vectors */
static struct fnv1_32_test_vector fnv1_32_vector[] = {
	{ &_fnv_test_str[0], (uint32)0x811c9dc5UL },
	{ &_fnv_test_str[1], (uint32)0x050c5d7eUL },
	{ &_fnv_test_str[2], (uint32)0x050c5d7dUL },
	{ &_fnv_test_str[3], (uint32)0x050c5d7cUL },
	{ &_fnv_test_str[4], (uint32)0x050c5d7bUL },
	{ &_fnv_test_str[5], (uint32)0x050c5d7aUL },
	{ &_fnv_test_str[6], (uint32)0x050c5d79UL },
	{ &_fnv_test_str[7], (uint32)0x6b772514UL },
	{ &_fnv_test_str[8], (uint32)0x408f5e13UL },
	{ &_fnv_test_str[9], (uint32)0xb4b1178bUL },
	{ &_fnv_test_str[10], (uint32)0xfdc80fb0UL },
	{ &_fnv_test_str[11], (uint32)0x31f0b262UL },
	{ &_fnv_test_str[12], (uint32)0x050c5d1fUL },
	{ &_fnv_test_str[13], (uint32)0x70772d5aUL },
	{ &_fnv_test_str[14], (uint32)0x6f772bc7UL },
	{ &_fnv_test_str[15], (uint32)0x6e772a34UL },
	{ &_fnv_test_str[16], (uint32)0x6d7728a1UL },
	{ &_fnv_test_str[17], (uint32)0x6c77270eUL },
	{ &_fnv_test_str[18], (uint32)0x6b77257bUL },
	{ &_fnv_test_str[19], (uint32)0x408f5e7cUL },
	{ &_fnv_test_str[20], (uint32)0xb4b117e9UL },
	{ &_fnv_test_str[21], (uint32)0xfdc80fd1UL },
	{ &_fnv_test_str[22], (uint32)0x31f0b210UL },
	{ &_fnv_test_str[23], (uint32)0xffe8d046UL },
	{ &_fnv_test_str[24], (uint32)0x6e772a5cUL },
	{ &_fnv_test_str[25], (uint32)0x4197aebbUL },
	{ &_fnv_test_str[26], (uint32)0xfcc8100fUL },
	{ &_fnv_test_str[27], (uint32)0xfdf147faUL },
	{ &_fnv_test_str[28], (uint32)0xbcd44ee1UL },
	{ &_fnv_test_str[29], (uint32)0x23382c13UL },
	{ &_fnv_test_str[30], (uint32)0x846d619eUL },
	{ &_fnv_test_str[31], (uint32)0x1630abdbUL },
	{ &_fnv_test_str[32], (uint32)0xc99e89b2UL },
	{ &_fnv_test_str[33], (uint32)0x1692c316UL },
	{ &_fnv_test_str[34], (uint32)0x9f091bcaUL },
	{ &_fnv_test_str[35], (uint32)0x2556be9bUL },
	{ &_fnv_test_str[36], (uint32)0x628e0e73UL },
	{ &_fnv_test_str[37], (uint32)0x98a0bf6cUL },
	{ &_fnv_test_str[38], (uint32)0xb10d5725UL },
	{ &_fnv_test_str[39], (uint32)0xdd002f35UL },
	{ &_fnv_test_str[40], (uint32)0x4197aed4UL },
	{ &_fnv_test_str[41], (uint32)0xfcc81061UL },
	{ &_fnv_test_str[42], (uint32)0xfdf1479dUL },
	{ &_fnv_test_str[43], (uint32)0xbcd44e8eUL },
	{ &_fnv_test_str[44], (uint32)0x23382c33UL },
	{ &_fnv_test_str[45], (uint32)0x846d61e9UL },
	{ &_fnv_test_str[46], (uint32)0x1630abbaUL },
	{ &_fnv_test_str[47], (uint32)0xc99e89c1UL },
	{ &_fnv_test_str[48], (uint32)0x1692c336UL },
	{ &_fnv_test_str[49], (uint32)0x9f091ba2UL },
	{ &_fnv_test_str[50], (uint32)0x2556befeUL },
	{ &_fnv_test_str[51], (uint32)0x628e0e01UL },
	{ &_fnv_test_str[52], (uint32)0x98a0bf09UL },
	{ &_fnv_test_str[53], (uint32)0xb10d5704UL },
	{ &_fnv_test_str[54], (uint32)0xdd002f3fUL },
	{ &_fnv_test_str[55], (uint32)0x1c4a506fUL },
	{ &_fnv_test_str[56], (uint32)0x6e772a41UL },
	{ &_fnv_test_str[57], (uint32)0x26978421UL },
	{ &_fnv_test_str[58], (uint32)0xe184ff97UL },
	{ &_fnv_test_str[59], (uint32)0x9b5e5ac6UL },
	{ &_fnv_test_str[60], (uint32)0x5b88e592UL },
	{ &_fnv_test_str[61], (uint32)0xaa8164b7UL },
	{ &_fnv_test_str[62], (uint32)0x20b18c7bUL },
	{ &_fnv_test_str[63], (uint32)0xf28025c5UL },
	{ &_fnv_test_str[64], (uint32)0x84bb753fUL },
	{ &_fnv_test_str[65], (uint32)0x3219925aUL },
	{ &_fnv_test_str[66], (uint32)0x384163c6UL },
	{ &_fnv_test_str[67], (uint32)0x54f010d7UL },
	{ &_fnv_test_str[68], (uint32)0x8cea820cUL },
	{ &_fnv_test_str[69], (uint32)0xe12ab8eeUL },
	{ &_fnv_test_str[70], (uint32)0x26978453UL },
	{ &_fnv_test_str[71], (uint32)0xe184fff3UL },
	{ &_fnv_test_str[72], (uint32)0x9b5e5ab5UL },
	{ &_fnv_test_str[73], (uint32)0x5b88e5b2UL },
	{ &_fnv_test_str[74], (uint32)0xaa8164d6UL },
	{ &_fnv_test_str[75], (uint32)0x20b18c15UL },
	{ &_fnv_test_str[76], (uint32)0xf28025a1UL },
	{ &_fnv_test_str[77], (uint32)0x84bb751fUL },
	{ &_fnv_test_str[78], (uint32)0x3219922dUL },
	{ &_fnv_test_str[79], (uint32)0x384163aeUL },
	{ &_fnv_test_str[80], (uint32)0x54f010b2UL },
	{ &_fnv_test_str[81], (uint32)0x8cea8275UL },
	{ &_fnv_test_str[82], (uint32)0xe12ab8e4UL },
	{ &_fnv_test_str[83], (uint32)0x64411eaaUL },
	{ &_fnv_test_str[84], (uint32)0x6977223cUL },
	{ &_fnv_test_str[85], (uint32)0x428ae474UL },
	{ &_fnv_test_str[86], (uint32)0xb6fa7167UL },
	{ &_fnv_test_str[87], (uint32)0x73408525UL },
	{ &_fnv_test_str[88], (uint32)0xb78320a1UL },
	{ &_fnv_test_str[89], (uint32)0x0caf4135UL },
	{ &_fnv_test_str[90], (uint32)0xb78320a2UL },
	{ &_fnv_test_str[91], (uint32)0xcdc88e80UL },
	{ &_fnv_test_str[92], (uint32)0xb78320a3UL },
	{ &_fnv_test_str[93], (uint32)0x8ee1dbcbUL },
	{ &_fnv_test_str[94], (uint32)0xb78320a4UL },
	{ &_fnv_test_str[95], (uint32)0x4ffb2716UL },
	{ &_fnv_test_str[96], (uint32)0x860632aaUL },
	{ &_fnv_test_str[97], (uint32)0xcc2c5c64UL },
	{ &_fnv_test_str[98], (uint32)0x860632a4UL },
	{ &_fnv_test_str[99], (uint32)0x2a7ec4a6UL },
	{ &_fnv_test_str[100], (uint32)0x860632baUL },
	{ &_fnv_test_str[101], (uint32)0xfefe8e14UL },
	{ &_fnv_test_str[102], (uint32)0x0a3cffd8UL },
	{ &_fnv_test_str[103], (uint32)0xf606c108UL },
	{ &_fnv_test_str[104], (uint32)0x0a3cffdbUL },
	{ &_fnv_test_str[105], (uint32)0xf906c5c1UL },
	{ &_fnv_test_str[106], (uint32)0x0a3cffdaUL },
	{ &_fnv_test_str[107], (uint32)0xf806c42eUL },
	{ &_fnv_test_str[108], (uint32)0xc07167d7UL },
	{ &_fnv_test_str[109], (uint32)0xc9867775UL },
	{ &_fnv_test_str[110], (uint32)0xbf716668UL },
	{ &_fnv_test_str[111], (uint32)0xc78435b8UL },
	{ &_fnv_test_str[112], (uint32)0xc6717155UL },
	{ &_fnv_test_str[113], (uint32)0xb99568cfUL },
	{ &_fnv_test_str[114], (uint32)0x7662e0d6UL },
	{ &_fnv_test_str[115], (uint32)0x33a7f0e2UL },
	{ &_fnv_test_str[116], (uint32)0xc2732f95UL },
	{ &_fnv_test_str[117], (uint32)0xb053e78fUL },
	{ &_fnv_test_str[118], (uint32)0x3a19c02aUL },
	{ &_fnv_test_str[119], (uint32)0xa089821eUL },
	{ &_fnv_test_str[120], (uint32)0x31ae8f83UL },
	{ &_fnv_test_str[121], (uint32)0x995fa9c4UL },
	{ &_fnv_test_str[122], (uint32)0x35983f8cUL },
	{ &_fnv_test_str[123], (uint32)0x5036a251UL },
	{ &_fnv_test_str[124], (uint32)0x97018583UL },
	{ &_fnv_test_str[125], (uint32)0xb4448d60UL },
	{ &_fnv_test_str[126], (uint32)0x025dfe59UL },
	{ &_fnv_test_str[127], (uint32)0xc5eab3afUL },
	{ &_fnv_test_str[128], (uint32)0x7d21ba1eUL },
	{ &_fnv_test_str[129], (uint32)0x7704cddbUL },
	{ &_fnv_test_str[130], (uint32)0xd0071bfeUL },
	{ &_fnv_test_str[131], (uint32)0x0ff3774cUL },
	{ &_fnv_test_str[132], (uint32)0xb0fea0eaUL },
	{ &_fnv_test_str[133], (uint32)0x58177303UL },
	{ &_fnv_test_str[134], (uint32)0x4f599cdaUL },
	{ &_fnv_test_str[135], (uint32)0x3e590a47UL },
	{ &_fnv_test_str[136], (uint32)0x965595f8UL },
	{ &_fnv_test_str[137], (uint32)0xc37f178dUL },
	{ &_fnv_test_str[138], (uint32)0x9711dd26UL },
	{ &_fnv_test_str[139], (uint32)0x23c99b7fUL },
	{ &_fnv_test_str[140], (uint32)0x6e568b17UL },
	{ &_fnv_test_str[141], (uint32)0x43f0245bUL },
	{ &_fnv_test_str[142], (uint32)0xbcb7a001UL },
	{ &_fnv_test_str[143], (uint32)0x12e6dffeUL },
	{ &_fnv_test_str[144], (uint32)0x0792f2d6UL },
	{ &_fnv_test_str[145], (uint32)0xb966936bUL },
	{ &_fnv_test_str[146], (uint32)0x46439ac5UL },
	{ &_fnv_test_str[147], (uint32)0x728d49afUL },
	{ &_fnv_test_str[148], (uint32)0xd33745c9UL },
	{ &_fnv_test_str[149], (uint32)0xbc382a57UL },
	{ &_fnv_test_str[150], (uint32)0x4bda1d31UL },
	{ &_fnv_test_str[151], (uint32)0xce35ccaeUL },
	{ &_fnv_test_str[152], (uint32)0x3b6eed94UL },
	{ &_fnv_test_str[153], (uint32)0x445c9c58UL },
	{ &_fnv_test_str[154], (uint32)0x3db8bf9dUL },
	{ &_fnv_test_str[155], (uint32)0x2dee116dUL },
	{ &_fnv_test_str[156], (uint32)0xc18738daUL },
	{ &_fnv_test_str[157], (uint32)0x5b156176UL },
	{ &_fnv_test_str[158], (uint32)0x2aa7d593UL },
	{ &_fnv_test_str[159], (uint32)0xb2409658UL },
	{ &_fnv_test_str[160], (uint32)0xe1489528UL },
	{ &_fnv_test_str[161], (uint32)0xfe1ee07eUL },
	{ &_fnv_test_str[162], (uint32)0xe8842315UL },
	{ &_fnv_test_str[163], (uint32)0x3a6a63a2UL },
	{ &_fnv_test_str[164], (uint32)0x06d2c18cUL },
	{ &_fnv_test_str[165], (uint32)0xf8ef7225UL },
	{ &_fnv_test_str[166], (uint32)0x843d3300UL },
	{ &_fnv_test_str[167], (uint32)0xbb24f7aeUL },
	{ &_fnv_test_str[168], (uint32)0x878c0ec9UL },
	{ &_fnv_test_str[169], (uint32)0xb557810fUL },
	{ &_fnv_test_str[170], (uint32)0x57423246UL },
	{ &_fnv_test_str[171], (uint32)0x87f7505eUL },
	{ &_fnv_test_str[172], (uint32)0xbb809f20UL },
	{ &_fnv_test_str[173], (uint32)0x8932abb5UL },
	{ &_fnv_test_str[174], (uint32)0x0a9b3aa0UL },
	{ &_fnv_test_str[175], (uint32)0xb8682a24UL },
	{ &_fnv_test_str[176], (uint32)0xa7ac1c56UL },
	{ &_fnv_test_str[177], (uint32)0x11409252UL },
	{ &_fnv_test_str[178], (uint32)0xa987f517UL },
	{ &_fnv_test_str[179], (uint32)0xf309e7edUL },
	{ &_fnv_test_str[180], (uint32)0xc9e8f417UL },
	{ &_fnv_test_str[181], (uint32)0x7f447bddUL },
	{ &_fnv_test_str[182], (uint32)0xb929adc5UL },
	{ &_fnv_test_str[183], (uint32)0x57022879UL },
	{ &_fnv_test_str[184], (uint32)0xdcfd2c49UL },
	{ &_fnv_test_str[185], (uint32)0x6edafff5UL },
	{ &_fnv_test_str[186], (uint32)0xf04fb1f1UL },
	{ &_fnv_test_str[187], (uint32)0xfb7de8b9UL },
	{ &_fnv_test_str[188], (uint32)0xc5f1d7e9UL },
	{ &_fnv_test_str[189], (uint32)0x32c1f439UL },
	{ &_fnv_test_str[190], (uint32)0x7fd3eb7dUL },
	{ &_fnv_test_str[191], (uint32)0x81597da5UL },
	{ &_fnv_test_str[192], (uint32)0x05eb7a25UL },
	{ &_fnv_test_str[193], (uint32)0x9c0fa1b5UL },
	{ &_fnv_test_str[194], (uint32)0x53ccb1c5UL },
	{ &_fnv_test_str[195], (uint32)0xfabece15UL },
	{ &_fnv_test_str[196], (uint32)0x4ad745a5UL },
	{ &_fnv_test_str[197], (uint32)0xe5bdc495UL },
	{ &_fnv_test_str[198], (uint32)0x23b3c0a5UL },
	{ &_fnv_test_str[199], (uint32)0xfa823dd5UL },
	{ &_fnv_test_str[200], (uint32)0x0c6c58b9UL },
	{ &_fnv_test_str[201], (uint32)0xe2dbccd5UL },
	{ &_fnv_test_str[202], (uint32)0xdb7f50f9UL },
	{ NULL, 0 }
};

/* FNV-1a 32 bit test vectors */
static struct fnv1a_32_test_vector fnv1a_32_vector[] = {
	{ &_fnv_test_str[0], (uint32)0x811c9dc5UL },
	{ &_fnv_test_str[1], (uint32)0xe40c292cUL },
	{ &_fnv_test_str[2], (uint32)0xe70c2de5UL },
	{ &_fnv_test_str[3], (uint32)0xe60c2c52UL },
	{ &_fnv_test_str[4], (uint32)0xe10c2473UL },
	{ &_fnv_test_str[5], (uint32)0xe00c22e0UL },
	{ &_fnv_test_str[6], (uint32)0xe30c2799UL },
	{ &_fnv_test_str[7], (uint32)0x6222e842UL },
	{ &_fnv_test_str[8], (uint32)0xa9f37ed7UL },
	{ &_fnv_test_str[9], (uint32)0x3f5076efUL },
	{ &_fnv_test_str[10], (uint32)0x39aaa18aUL },
	{ &_fnv_test_str[11], (uint32)0xbf9cf968UL },
	{ &_fnv_test_str[12], (uint32)0x050c5d1fUL },
	{ &_fnv_test_str[13], (uint32)0x2b24d044UL },
	{ &_fnv_test_str[14], (uint32)0x9d2c3f7fUL },
	{ &_fnv_test_str[15], (uint32)0x7729c516UL },
	{ &_fnv_test_str[16], (uint32)0xb91d6109UL },
	{ &_fnv_test_str[17], (uint32)0x931ae6a0UL },
	{ &_fnv_test_str[18], (uint32)0x052255dbUL },
	{ &_fnv_test_str[19], (uint32)0xbef39fe6UL },
	{ &_fnv_test_str[20], (uint32)0x6150ac75UL },
	{ &_fnv_test_str[21], (uint32)0x9aab3a3dUL },
	{ &_fnv_test_str[22], (uint32)0x519c4c3eUL },
	{ &_fnv_test_str[23], (uint32)0x0c1c9eb8UL },
	{ &_fnv_test_str[24], (uint32)0x5f299f4eUL },
	{ &_fnv_test_str[25], (uint32)0xef8580f3UL },
	{ &_fnv_test_str[26], (uint32)0xac297727UL },
	{ &_fnv_test_str[27], (uint32)0x4546b9c0UL },
	{ &_fnv_test_str[28], (uint32)0xbd564e7dUL },
	{ &_fnv_test_str[29], (uint32)0x6bdd5c67UL },
	{ &_fnv_test_str[30], (uint32)0xdd77ed30UL },
	{ &_fnv_test_str[31], (uint32)0xf4ca9683UL },
	{ &_fnv_test_str[32], (uint32)0x4aeb9bd0UL },
	{ &_fnv_test_str[33], (uint32)0xe0e67ad0UL },
	{ &_fnv_test_str[34], (uint32)0xc2d32fa8UL },
	{ &_fnv_test_str[35], (uint32)0x7f743fb7UL },
	{ &_fnv_test_str[36], (uint32)0x6900631fUL },
	{ &_fnv_test_str[37], (uint32)0xc59c990eUL },
	{ &_fnv_test_str[38], (uint32)0x448524fdUL },
	{ &_fnv_test_str[39], (uint32)0xd49930d5UL },
	{ &_fnv_test_str[40], (uint32)0x1c85c7caUL },
	{ &_fnv_test_str[41], (uint32)0x0229fe89UL },
	{ &_fnv_test_str[42], (uint32)0x2c469265UL },
	{ &_fnv_test_str[43], (uint32)0xce566940UL },
	{ &_fnv_test_str[44], (uint32)0x8bdd8ec7UL },
	{ &_fnv_test_str[45], (uint32)0x34787625UL },
	{ &_fnv_test_str[46], (uint32)0xd3ca6290UL },
	{ &_fnv_test_str[47], (uint32)0xddeaf039UL },
	{ &_fnv_test_str[48], (uint32)0xc0e64870UL },
	{ &_fnv_test_str[49], (uint32)0xdad35570UL },
	{ &_fnv_test_str[50], (uint32)0x5a740578UL },
	{ &_fnv_test_str[51], (uint32)0x5b004d15UL },
	{ &_fnv_test_str[52], (uint32)0x6a9c09cdUL },
	{ &_fnv_test_str[53], (uint32)0x2384f10aUL },
	{ &_fnv_test_str[54], (uint32)0xda993a47UL },
	{ &_fnv_test_str[55], (uint32)0x8227df4fUL },
	{ &_fnv_test_str[56], (uint32)0x4c298165UL },
	{ &_fnv_test_str[57], (uint32)0xfc563735UL },
	{ &_fnv_test_str[58], (uint32)0x8cb91483UL },
	{ &_fnv_test_str[59], (uint32)0x775bf5d0UL },
	{ &_fnv_test_str[60], (uint32)0xd5c428d0UL },
	{ &_fnv_test_str[61], (uint32)0x34cc0ea3UL },
	{ &_fnv_test_str[62], (uint32)0xea3b4cb7UL },
	{ &_fnv_test_str[63], (uint32)0x8e59f029UL },
	{ &_fnv_test_str[64], (uint32)0x2094de2bUL },
	{ &_fnv_test_str[65], (uint32)0xa65a0ad4UL },
	{ &_fnv_test_str[66], (uint32)0x9bbee5f4UL },
	{ &_fnv_test_str[67], (uint32)0xbe836343UL },
	{ &_fnv_test_str[68], (uint32)0x22d5344eUL },
	{ &_fnv_test_str[69], (uint32)0x19a1470cUL },
	{ &_fnv_test_str[70], (uint32)0x4a56b1ffUL },
	{ &_fnv_test_str[71], (uint32)0x70b8e86fUL },
	{ &_fnv_test_str[72], (uint32)0x0a5b4a39UL },
	{ &_fnv_test_str[73], (uint32)0xb5c3f670UL },
	{ &_fnv_test_str[74], (uint32)0x53cc3f70UL },
	{ &_fnv_test_str[75], (uint32)0xc03b0a99UL },
	{ &_fnv_test_str[76], (uint32)0x7259c415UL },
	{ &_fnv_test_str[77], (uint32)0x4095108bUL },
	{ &_fnv_test_str[78], (uint32)0x7559bdb1UL },
	{ &_fnv_test_str[79], (uint32)0xb3bf0bbcUL },
	{ &_fnv_test_str[80], (uint32)0x2183ff1cUL },
	{ &_fnv_test_str[81], (uint32)0x2bd54279UL },
	{ &_fnv_test_str[82], (uint32)0x23a156caUL },
	{ &_fnv_test_str[83], (uint32)0x64e2d7e4UL },
	{ &_fnv_test_str[84], (uint32)0x683af69aUL },
	{ &_fnv_test_str[85], (uint32)0xaed2346eUL },
	{ &_fnv_test_str[86], (uint32)0x4f9f2cabUL },
	{ &_fnv_test_str[87], (uint32)0x02935131UL },
	{ &_fnv_test_str[88], (uint32)0xc48fb86dUL },
	{ &_fnv_test_str[89], (uint32)0x2269f369UL },
	{ &_fnv_test_str[90], (uint32)0xc18fb3b4UL },
	{ &_fnv_test_str[91], (uint32)0x50ef1236UL },
	{ &_fnv_test_str[92], (uint32)0xc28fb547UL },
	{ &_fnv_test_str[93], (uint32)0x96c3bf47UL },
	{ &_fnv_test_str[94], (uint32)0xbf8fb08eUL },
	{ &_fnv_test_str[95], (uint32)0xf3e4d49cUL },
	{ &_fnv_test_str[96], (uint32)0x32179058UL },
	{ &_fnv_test_str[97], (uint32)0x280bfee6UL },
	{ &_fnv_test_str[98], (uint32)0x30178d32UL },
	{ &_fnv_test_str[99], (uint32)0x21addaf8UL },
	{ &_fnv_test_str[100], (uint32)0x4217a988UL },
	{ &_fnv_test_str[101], (uint32)0x772633d6UL },
	{ &_fnv_test_str[102], (uint32)0x08a3d11eUL },
	{ &_fnv_test_str[103], (uint32)0xb7e2323aUL },
	{ &_fnv_test_str[104], (uint32)0x07a3cf8bUL },
	{ &_fnv_test_str[105], (uint32)0x91dfb7d1UL },
	{ &_fnv_test_str[106], (uint32)0x06a3cdf8UL },
	{ &_fnv_test_str[107], (uint32)0x6bdd3d68UL },
	{ &_fnv_test_str[108], (uint32)0x1d5636a7UL },
	{ &_fnv_test_str[109], (uint32)0xd5b808e5UL },
	{ &_fnv_test_str[110], (uint32)0x1353e852UL },
	{ &_fnv_test_str[111], (uint32)0xbf16b916UL },
	{ &_fnv_test_str[112], (uint32)0xa55b89edUL },
	{ &_fnv_test_str[113], (uint32)0x3c1a2017UL },
	{ &_fnv_test_str[114], (uint32)0x0588b13cUL },
	{ &_fnv_test_str[115], (uint32)0xf22f0174UL },
	{ &_fnv_test_str[116], (uint32)0xe83641e1UL },
	{ &_fnv_test_str[117], (uint32)0x6e69b533UL },
	{ &_fnv_test_str[118], (uint32)0xf1760448UL },
	{ &_fnv_test_str[119], (uint32)0x64c8bd58UL },
	{ &_fnv_test_str[120], (uint32)0x97b4ea23UL },
	{ &_fnv_test_str[121], (uint32)0x9a4e92e6UL },
	{ &_fnv_test_str[122], (uint32)0xcfb14012UL },
	{ &_fnv_test_str[123], (uint32)0xf01b2511UL },
	{ &_fnv_test_str[124], (uint32)0x0bbb59c3UL },
	{ &_fnv_test_str[125], (uint32)0xce524afaUL },
	{ &_fnv_test_str[126], (uint32)0xdd16ef45UL },
	{ &_fnv_test_str[127], (uint32)0x60648bb3UL },
	{ &_fnv_test_str[128], (uint32)0x7fa4bcfcUL },
	{ &_fnv_test_str[129], (uint32)0x5053ae17UL },
	{ &_fnv_test_str[130], (uint32)0xc9302890UL },
	{ &_fnv_test_str[131], (uint32)0x956ded32UL },
	{ &_fnv_test_str[132], (uint32)0x9136db84UL },
	{ &_fnv_test_str[133], (uint32)0xdf9d3323UL },
	{ &_fnv_test_str[134], (uint32)0x32bb6cd0UL },
	{ &_fnv_test_str[135], (uint32)0xc8f8385bUL },
	{ &_fnv_test_str[136], (uint32)0xeb08bfbaUL },
	{ &_fnv_test_str[137], (uint32)0x62cc8e3dUL },
	{ &_fnv_test_str[138], (uint32)0xc3e20f5cUL },
	{ &_fnv_test_str[139], (uint32)0x39e97f17UL },
	{ &_fnv_test_str[140], (uint32)0x7837b203UL },
	{ &_fnv_test_str[141], (uint32)0x319e877bUL },
	{ &_fnv_test_str[142], (uint32)0xd3e63f89UL },
	{ &_fnv_test_str[143], (uint32)0x29b50b38UL },
	{ &_fnv_test_str[144], (uint32)0x5ed678b8UL },
	{ &_fnv_test_str[145], (uint32)0xb0d5b793UL },
	{ &_fnv_test_str[146], (uint32)0x52450be5UL },
	{ &_fnv_test_str[147], (uint32)0xfa72d767UL },
	{ &_fnv_test_str[148], (uint32)0x95066709UL },
	{ &_fnv_test_str[149], (uint32)0x7f52e123UL },
	{ &_fnv_test_str[150], (uint32)0x76966481UL },
	{ &_fnv_test_str[151], (uint32)0x063258b0UL },
	{ &_fnv_test_str[152], (uint32)0x2ded6e8aUL },
	{ &_fnv_test_str[153], (uint32)0xb07d7c52UL },
	{ &_fnv_test_str[154], (uint32)0xd0c71b71UL },
	{ &_fnv_test_str[155], (uint32)0xf684f1bdUL },
	{ &_fnv_test_str[156], (uint32)0x868ecfa8UL },
	{ &_fnv_test_str[157], (uint32)0xf794f684UL },
	{ &_fnv_test_str[158], (uint32)0xd19701c3UL },
	{ &_fnv_test_str[159], (uint32)0x346e171eUL },
	{ &_fnv_test_str[160], (uint32)0x91f8f676UL },
	{ &_fnv_test_str[161], (uint32)0x0bf58848UL },
	{ &_fnv_test_str[162], (uint32)0x6317b6d1UL },
	{ &_fnv_test_str[163], (uint32)0xafad4c54UL },
	{ &_fnv_test_str[164], (uint32)0x0f25681eUL },
	{ &_fnv_test_str[165], (uint32)0x91b18d49UL },
	{ &_fnv_test_str[166], (uint32)0x7d61c12eUL },
	{ &_fnv_test_str[167], (uint32)0x5147d25cUL },
	{ &_fnv_test_str[168], (uint32)0x9a8b6805UL },
	{ &_fnv_test_str[169], (uint32)0x4cd2a447UL },
	{ &_fnv_test_str[170], (uint32)0x1e549b14UL },
	{ &_fnv_test_str[171], (uint32)0x2fe1b574UL },
	{ &_fnv_test_str[172], (uint32)0xcf0cd31eUL },
	{ &_fnv_test_str[173], (uint32)0x6c471669UL },
	{ &_fnv_test_str[174], (uint32)0x0e5eef1eUL },
	{ &_fnv_test_str[175], (uint32)0x2bed3602UL },
	{ &_fnv_test_str[176], (uint32)0xb26249e0UL },
	{ &_fnv_test_str[177], (uint32)0x2c9b86a4UL },
	{ &_fnv_test_str[178], (uint32)0xe415e2bbUL },
	{ &_fnv_test_str[179], (uint32)0x18a98d1dUL },
	{ &_fnv_test_str[180], (uint32)0xb7df8b7bUL },
	{ &_fnv_test_str[181], (uint32)0x241e9075UL },
	{ &_fnv_test_str[182], (uint32)0x063f70ddUL },
	{ &_fnv_test_str[183], (uint32)0x0295aed9UL },
	{ &_fnv_test_str[184], (uint32)0x56a7f781UL },
	{ &_fnv_test_str[185], (uint32)0x253bc645UL },
	{ &_fnv_test_str[186], (uint32)0x46610921UL },
	{ &_fnv_test_str[187], (uint32)0x7c1577f9UL },
	{ &_fnv_test_str[188], (uint32)0x512b2851UL },
	{ &_fnv_test_str[189], (uint32)0x76823999UL },
	{ &_fnv_test_str[190], (uint32)0xc0586935UL },
	{ &_fnv_test_str[191], (uint32)0xf3415c85UL },
	{ &_fnv_test_str[192], (uint32)0x0ae4ff65UL },
	{ &_fnv_test_str[193], (uint32)0x58b79725UL },
	{ &_fnv_test_str[194], (uint32)0xdea43aa5UL },
	{ &_fnv_test_str[195], (uint32)0x2bb3be35UL },
	{ &_fnv_test_str[196], (uint32)0xea777a45UL },
	{ &_fnv_test_str[197], (uint32)0x8f21c305UL },
	{ &_fnv_test_str[198], (uint32)0x5c9d0865UL },
	{ &_fnv_test_str[199], (uint32)0xfa823dd5UL },
	{ &_fnv_test_str[200], (uint32)0x21a27271UL },
	{ &_fnv_test_str[201], (uint32)0x83c5c6d5UL },
	{ &_fnv_test_str[202], (uint32)0x813b0881UL },
	{ NULL, 0 }
};

/* FNV-0 64 bit test vectors */
static struct fnv0_64_test_vector fnv0_64_vector[] = {
	{ &_fnv_test_str[0], (uint64)0x0000000000000000ULL },
	{ &_fnv_test_str[1], (uint64)0x0000000000000061ULL },
	{ &_fnv_test_str[2], (uint64)0x0000000000000062ULL },
	{ &_fnv_test_str[3], (uint64)0x0000000000000063ULL },
	{ &_fnv_test_str[4], (uint64)0x0000000000000064ULL },
	{ &_fnv_test_str[5], (uint64)0x0000000000000065ULL },
	{ &_fnv_test_str[6], (uint64)0x0000000000000066ULL },
	{ &_fnv_test_str[7], (uint64)0x000066000000ad3dULL },
	{ &_fnv_test_str[8], (uint64)0x015a8f0001265ec8ULL },
	{ &_fnv_test_str[9], (uint64)0x733fc501f4330dbaULL },
	{ &_fnv_test_str[10], (uint64)0x08697c51f2c0536fULL },
	{ &_fnv_test_str[11], (uint64)0x0b91ae3f7ccdc5efULL },
	{ &_fnv_test_str[12], (uint64)0x0000000000000000ULL },
	{ &_fnv_test_str[13], (uint64)0x000061000000a4d3ULL },
	{ &_fnv_test_str[14], (uint64)0x000062000000a686ULL },
	{ &_fnv_test_str[15], (uint64)0x000063000000a839ULL },
	{ &_fnv_test_str[16], (uint64)0x000064000000a9ecULL },
	{ &_fnv_test_str[17], (uint64)0x000065000000ab9fULL },
	{ &_fnv_test_str[18], (uint64)0x000066000000ad52ULL },
	{ &_fnv_test_str[19], (uint64)0x015a8f0001265ea7ULL },
	{ &_fnv_test_str[20], (uint64)0x733fc501f4330dd8ULL },
	{ &_fnv_test_str[21], (uint64)0x08697c51f2c0530eULL },
	{ &_fnv_test_str[22], (uint64)0x0b91ae3f7ccdc59dULL },
	{ &_fnv_test_str[23], (uint64)0x765104e111a7551dULL },
	{ &_fnv_test_str[24], (uint64)0x000063000000a851ULL },
	{ &_fnv_test_str[25], (uint64)0x01508a00011e01ccULL },
	{ &_fnv_test_str[26], (uint64)0x59dc4a01e5fd0dcaULL },
	{ &_fnv_test_str[27], (uint64)0xae5f8b39ccfe6e59ULL },
	{ &_fnv_test_str[28], (uint64)0x4ac7ec3754558154ULL },
	{ &_fnv_test_str[29], (uint64)0x6737b6044d4ac19cULL },
	{ &_fnv_test_str[30], (uint64)0xae6be54f5606fc63ULL },
	{ &_fnv_test_str[31], (uint64)0x685308cf2ddedc58ULL },
	{ &_fnv_test_str[32], (uint64)0x23f4500af1b069fbULL },
	{ &_fnv_test_str[33], (uint64)0xc88dfd98aec415a1ULL },
	{ &_fnv_test_str[34], (uint64)0x8d5b8b70f730c0fbULL },
	{ &_fnv_test_str[35], (uint64)0x634eebf407d7eae4ULL },
	{ &_fnv_test_str[36], (uint64)0x9705d3a953e4211eULL },
	{ &_fnv_test_str[37], (uint64)0x8307c6b98ca4459fULL },
	{ &_fnv_test_str[38], (uint64)0x4a7c4c49fb224d0cULL },
	{ &_fnv_test_str[39], (uint64)0xb382adb5bb48eb6eULL },
	{ &_fnv_test_str[40], (uint64)0x01508a00011e01a3ULL },
	{ &_fnv_test_str[41], (uint64)0x59dc4a01e5fd0da4ULL },
	{ &_fnv_test_str[42], (uint64)0xae5f8b39ccfe6e3eULL },
	{ &_fnv_test_str[43], (uint64)0x4ac7ec375455813bULL },
	{ &_fnv_test_str[44], (uint64)0x6737b6044d4ac1bcULL },
	{ &_fnv_test_str[45], (uint64)0xae6be54f5606fc14ULL },
	{ &_fnv_test_str[46], (uint64)0x685308cf2ddedc39ULL },
	{ &_fnv_test_str[47], (uint64)0x23f4500af1b06988ULL },
	{ &_fnv_test_str[48], (uint64)0xc88dfd98aec41581ULL },
	{ &_fnv_test_str[49], (uint64)0x8d5b8b70f730c093ULL },
	{ &_fnv_test_str[50], (uint64)0x634eebf407d7ea81ULL },
	{ &_fnv_test_str[51], (uint64)0x9705d3a953e4216cULL },
	{ &_fnv_test_str[52], (uint64)0x8307c6b98ca445faULL },
	{ &_fnv_test_str[53], (uint64)0x4a7c4c49fb224d2dULL },
	{ &_fnv_test_str[54], (uint64)0xb382adb5bb48eb64ULL },
	{ &_fnv_test_str[55], (uint64)0x4ff899cd3ce80beaULL },
	{ &_fnv_test_str[56], (uint64)0x000063000000a84cULL },
	{ &_fnv_test_str[57], (uint64)0x01508500011df956ULL },
	{ &_fnv_test_str[58], (uint64)0x59cb5501e5eead46ULL },
	{ &_fnv_test_str[59], (uint64)0x832eb839b4906d81ULL },
	{ &_fnv_test_str[60], (uint64)0x78d08b0dd16a1213ULL },
	{ &_fnv_test_str[61], (uint64)0xb46e5b7ad73cb628ULL },
	{ &_fnv_test_str[62], (uint64)0xd43b99bbbc298596ULL },
	{ &_fnv_test_str[63], (uint64)0xcacbd000ba8dfd86ULL },
	{ &_fnv_test_str[64], (uint64)0x264ff73cff45ca92ULL },
	{ &_fnv_test_str[65], (uint64)0x5fabaea5c3973661ULL },
	{ &_fnv_test_str[66], (uint64)0x27f024ab59f166bbULL },
	{ &_fnv_test_str[67], (uint64)0xce750a29d5318fa4ULL },
	{ &_fnv_test_str[68], (uint64)0x026fe915433713d5ULL },
	{ &_fnv_test_str[69], (uint64)0x5b3ce4213696b2e5ULL },
	{ &_fnv_test_str[70], (uint64)0x01508500011df924ULL },
	{ &_fnv_test_str[71], (uint64)0x59cb5501e5eead22ULL },
	{ &_fnv_test_str[72], (uint64)0x832eb839b4906df2ULL },
	{ &_fnv_test_str[73], (uint64)0x78d08b0dd16a1233ULL },
	{ &_fnv_test_str[74], (uint64)0xb46e5b7ad73cb649ULL },
	{ &_fnv_test_str[75], (uint64)0xd43b99bbbc2985f8ULL },
	{ &_fnv_test_str[76], (uint64)0xcacbd000ba8dfde2ULL },
	{ &_fnv_test_str[77], (uint64)0x264ff73cff45cab2ULL },
	{ &_fnv_test_str[78], (uint64)0x5fabaea5c3973616ULL },
	{ &_fnv_test_str[79], (uint64)0x27f024ab59f166d3ULL },
	{ &_fnv_test_str[80], (uint64)0xce750a29d5318fc1ULL },
	{ &_fnv_test_str[81], (uint64)0x026fe915433713acULL },
	{ &_fnv_test_str[82], (uint64)0x5b3ce4213696b2efULL },
	{ &_fnv_test_str[83], (uint64)0x9f2a896fc211fb1fULL },
	{ &_fnv_test_str[84], (uint64)0x000068000000b0d1ULL },
	{ &_fnv_test_str[85], (uint64)0x01618900012c7323ULL },
	{ &_fnv_test_str[86], (uint64)0x3fa86e63bc7d03c8ULL },
	{ &_fnv_test_str[87], (uint64)0xa8375b79486d6cd8ULL },
	{ &_fnv_test_str[88], (uint64)0xa0d18504e316ac54ULL },
	{ &_fnv_test_str[89], (uint64)0x08a97b0004e7fe54ULL },
	{ &_fnv_test_str[90], (uint64)0xa0d18504e316ac57ULL },
	{ &_fnv_test_str[91], (uint64)0x1152f60009cffda9ULL },
	{ &_fnv_test_str[92], (uint64)0xa0d18504e316ac56ULL },
	{ &_fnv_test_str[93], (uint64)0x19fc71000eb7fcfeULL },
	{ &_fnv_test_str[94], (uint64)0xa0d18504e316ac51ULL },
	{ &_fnv_test_str[95], (uint64)0x22a5ec00139ffa53ULL },
	{ &_fnv_test_str[96], (uint64)0x29bed00139779a33ULL },
	{ &_fnv_test_str[97], (uint64)0x4dbc81014e3c19f5ULL },
	{ &_fnv_test_str[98], (uint64)0x29bed00139779a3dULL },
	{ &_fnv_test_str[99], (uint64)0x81a72b016b9f7573ULL },
	{ &_fnv_test_str[100], (uint64)0x29bed00139779a23ULL },
	{ &_fnv_test_str[101], (uint64)0xd85411019cbbce45ULL },
	{ &_fnv_test_str[102], (uint64)0xf548616b8621d657ULL },
	{ &_fnv_test_str[103], (uint64)0xebd3e0b4eb7f35d5ULL },
	{ &_fnv_test_str[104], (uint64)0xf548616b8621d654ULL },
	{ &_fnv_test_str[105], (uint64)0xebd3ddb4eb7f30bcULL },
	{ &_fnv_test_str[106], (uint64)0xf548616b8621d655ULL },
	{ &_fnv_test_str[107], (uint64)0xebd3deb4eb7f326fULL },
	{ &_fnv_test_str[108], (uint64)0x581cb60340ab0968ULL },
	{ &_fnv_test_str[109], (uint64)0x63d2af86e2a0fbb8ULL },
	{ &_fnv_test_str[110], (uint64)0x581cb70340ab0b37ULL },
	{ &_fnv_test_str[111], (uint64)0x63d63186e2a40e75ULL },
	{ &_fnv_test_str[112], (uint64)0x581cc40340ab212eULL },
	{ &_fnv_test_str[113], (uint64)0x64023f86e2c9612aULL },
	{ &_fnv_test_str[114], (uint64)0xdbda6a26c33c909fULL },
	{ &_fnv_test_str[115], (uint64)0xd0b2feddbfe9be2dULL },
	{ &_fnv_test_str[116], (uint64)0x9c9eae3f5d037decULL },
	{ &_fnv_test_str[117], (uint64)0x252001ab0ceef804ULL },
	{ &_fnv_test_str[118], (uint64)0x4456a56f9e05cfefULL },
	{ &_fnv_test_str[119], (uint64)0x250b0ba983e0531dULL },
	{ &_fnv_test_str[120], (uint64)0x52b007213b27b33eULL },
	{ &_fnv_test_str[121], (uint64)0xcbf29ce484222325ULL },
	{ &_fnv_test_str[122], (uint64)0xaf63bd4c8601b7dfULL },
	{ &_fnv_test_str[123], (uint64)0x128599ccddae09f8ULL },
	{ &_fnv_test_str[124], (uint64)0x270e4f1caebaf068ULL },
	{ &_fnv_test_str[125], (uint64)0x01517d497446a395ULL },
	{ &_fnv_test_str[126], (uint64)0x9af5a29a89450b40ULL },
	{ &_fnv_test_str[127], (uint64)0xb502f6c063ba72e8ULL },
	{ &_fnv_test_str[128], (uint64)0xacf41561498ca7dfULL },
	{ &_fnv_test_str[129], (uint64)0x6be8c2423a351542ULL },
	{ &_fnv_test_str[130], (uint64)0xd04f1f6da96ce4a3ULL },
	{ &_fnv_test_str[131], (uint64)0x69eb9a8f282c7235ULL },
	{ &_fnv_test_str[132], (uint64)0x6a7e5a418f77cfc5ULL },
	{ &_fnv_test_str[133], (uint64)0xbcaf568ddc2ecba0ULL },
	{ &_fnv_test_str[134], (uint64)0xb03b5cc4c38f8b1fULL },
	{ &_fnv_test_str[135], (uint64)0xf89a9f51432db828ULL },
	{ &_fnv_test_str[136], (uint64)0x549e856be6103429ULL },
	{ &_fnv_test_str[137], (uint64)0x3cf50d224d29377aULL },
	{ &_fnv_test_str[138], (uint64)0xdb762df418c10c37ULL },
	{ &_fnv_test_str[139], (uint64)0xfeeb4226b0e9a6baULL },
	{ &_fnv_test_str[140], (uint64)0x7004a4cd9310c052ULL },
	{ &_fnv_test_str[141], (uint64)0xd1c727d7f5329276ULL },
	{ &_fnv_test_str[142], (uint64)0xbe313796596ce908ULL },
	{ &_fnv_test_str[143], (uint64)0x768f67ede090fcc5ULL },
	{ &_fnv_test_str[144], (uint64)0xa81563cc9db9bfc3ULL },
	{ &_fnv_test_str[145], (uint64)0x47194043c55197a8ULL },
	{ &_fnv_test_str[146], (uint64)0xc99d81864aebab02ULL },
	{ &_fnv_test_str[147], (uint64)0xcc1f161b235ea4a2ULL },
	{ &_fnv_test_str[148], (uint64)0xaadab0c420ecd434ULL },
	{ &_fnv_test_str[149], (uint64)0x6b3c034d6f44d740ULL },
	{ &_fnv_test_str[150], (uint64)0x73a45e850602cbc6ULL },
	{ &_fnv_test_str[151], (uint64)0x72360f04f0cd227bULL },
	{ &_fnv_test_str[152], (uint64)0xa9ca80be384a778fULL },
	{ &_fnv_test_str[153], (uint64)0xd4085e66906889e3ULL },
	{ &_fnv_test_str[154], (uint64)0x93aa8b2748efdbc8ULL },
	{ &_fnv_test_str[155], (uint64)0x6f8cd678407436a2ULL },
	{ &_fnv_test_str[156], (uint64)0xf39a43d4dc8be4c7ULL },
	{ &_fnv_test_str[157], (uint64)0xd7f5cec91125d245ULL },
	{ &_fnv_test_str[158], (uint64)0x691d7b73be18adc4ULL },
	{ &_fnv_test_str[159], (uint64)0xf4361e01caf6b691ULL },
	{ &_fnv_test_str[160], (uint64)0xde7d8264f64be089ULL },
	{ &_fnv_test_str[161], (uint64)0xa34ff43e5545c06fULL },
	{ &_fnv_test_str[162], (uint64)0x181f0b8e908a2bdeULL },
	{ &_fnv_test_str[163], (uint64)0x28a965b78ddbc071ULL },
	{ &_fnv_test_str[164], (uint64)0xead9cea0e3cc6ae5ULL },
	{ &_fnv_test_str[165], (uint64)0x0b6743153b43ebbaULL },
	{ &_fnv_test_str[166], (uint64)0xa7aa3f012c74528dULL },
	{ &_fnv_test_str[167], (uint64)0x2d5d8ad7f9dffeb7ULL },
	{ &_fnv_test_str[168], (uint64)0x00750fb6e19624eaULL },
	{ &_fnv_test_str[169], (uint64)0x01c125a4e6c76c82ULL },
	{ &_fnv_test_str[170], (uint64)0x3fde3afac0722f1fULL },
	{ &_fnv_test_str[171], (uint64)0xd7c3eaf4abaa379dULL },
	{ &_fnv_test_str[172], (uint64)0xd2217e1c923c9f3fULL },
	{ &_fnv_test_str[173], (uint64)0x82d0a2e3b725caf6ULL },
	{ &_fnv_test_str[174], (uint64)0x0a10bee8eeb72e4fULL },
	{ &_fnv_test_str[175], (uint64)0xc530e8723e72c6fdULL },
	{ &_fnv_test_str[176], (uint64)0xd8d34dcd2e7bad99ULL },
	{ &_fnv_test_str[177], (uint64)0xecf77466e9a2baf3ULL },
	{ &_fnv_test_str[178], (uint64)0xde3d2ddb043b9666ULL },
	{ &_fnv_test_str[179], (uint64)0xd1cc824e1a8157d8ULL },
	{ &_fnv_test_str[180], (uint64)0x7d5c68ecbc90512eULL },
	{ &_fnv_test_str[181], (uint64)0x2f7c691b1d7c76d8ULL },
	{ &_fnv_test_str[182], (uint64)0x5d88c2bad3a46bc8ULL },
	{ &_fnv_test_str[183], (uint64)0xdf107320276647a0ULL },
	{ &_fnv_test_str[184], (uint64)0x0f78f22e7e70e9bcULL },
	{ &_fnv_test_str[185], (uint64)0x8c67be5c80f67d04ULL },
	{ &_fnv_test_str[186], (uint64)0x07c1adfa4d019194ULL },
	{ &_fnv_test_str[187], (uint64)0xce1312420c5b1af4ULL },
	{ &_fnv_test_str[188], (uint64)0x043a41b2dc53ab24ULL },
	{ &_fnv_test_str[189], (uint64)0x0b038eebf7340860ULL },
	{ &_fnv_test_str[190], (uint64)0x1bcd837353fb69b0ULL },
	{ &_fnv_test_str[191], (uint64)0x46f992fc59eff180ULL },
	{ &_fnv_test_str[192], (uint64)0x497678ee29ae79c0ULL },
	{ &_fnv_test_str[193], (uint64)0xb10a62280ddd4450ULL },
	{ &_fnv_test_str[194], (uint64)0x35eb228db4d68140ULL },
	{ &_fnv_test_str[195], (uint64)0x8b350e86d9470870ULL },
	{ &_fnv_test_str[196], (uint64)0x4e1fbdb2812e9540ULL },
	{ &_fnv_test_str[197], (uint64)0x051e080df69a0600ULL },
	{ &_fnv_test_str[198], (uint64)0x45e1e8ae54dadb40ULL },
	{ &_fnv_test_str[199], (uint64)0x0000000000000000ULL },
	{ &_fnv_test_str[200], (uint64)0xcd73806290557064ULL },
	{ &_fnv_test_str[201], (uint64)0x2613a37bbe0317c8ULL },
	{ &_fnv_test_str[202], (uint64)0x1480e21fcf2ae5e4ULL },
	{ NULL, (uint64)0 }
};

/* FNV-1 64 bit test vectors */
static struct fnv1_64_test_vector fnv1_64_vector[] = {
	{ &_fnv_test_str[0], (uint64)0xcbf29ce484222325ULL },
	{ &_fnv_test_str[1], (uint64)0xaf63bd4c8601b7beULL },
	{ &_fnv_test_str[2], (uint64)0xaf63bd4c8601b7bdULL },
	{ &_fnv_test_str[3], (uint64)0xaf63bd4c8601b7bcULL },
	{ &_fnv_test_str[4], (uint64)0xaf63bd4c8601b7bbULL },
	{ &_fnv_test_str[5], (uint64)0xaf63bd4c8601b7baULL },
	{ &_fnv_test_str[6], (uint64)0xaf63bd4c8601b7b9ULL },
	{ &_fnv_test_str[7], (uint64)0x08326207b4eb2f34ULL },
	{ &_fnv_test_str[8], (uint64)0xd8cbc7186ba13533ULL },
	{ &_fnv_test_str[9], (uint64)0x0378817ee2ed65cbULL },
	{ &_fnv_test_str[10], (uint64)0xd329d59b9963f790ULL },
	{ &_fnv_test_str[11], (uint64)0x340d8765a4dda9c2ULL },
	{ &_fnv_test_str[12], (uint64)0xaf63bd4c8601b7dfULL },
	{ &_fnv_test_str[13], (uint64)0x08326707b4eb37daULL },
	{ &_fnv_test_str[14], (uint64)0x08326607b4eb3627ULL },
	{ &_fnv_test_str[15], (uint64)0x08326507b4eb3474ULL },
	{ &_fnv_test_str[16], (uint64)0x08326407b4eb32c1ULL },
	{ &_fnv_test_str[17], (uint64)0x08326307b4eb310eULL },
	{ &_fnv_test_str[18], (uint64)0x08326207b4eb2f5bULL },
	{ &_fnv_test_str[19], (uint64)0xd8cbc7186ba1355cULL },
	{ &_fnv_test_str[20], (uint64)0x0378817ee2ed65a9ULL },
	{ &_fnv_test_str[21], (uint64)0xd329d59b9963f7f1ULL },
	{ &_fnv_test_str[22], (uint64)0x340d8765a4dda9b0ULL },
	{ &_fnv_test_str[23], (uint64)0x50a6d3b724a774a6ULL },
	{ &_fnv_test_str[24], (uint64)0x08326507b4eb341cULL },
	{ &_fnv_test_str[25], (uint64)0xd8d5c8186ba98bfbULL },
	{ &_fnv_test_str[26], (uint64)0x1ccefc7ef118dbefULL },
	{ &_fnv_test_str[27], (uint64)0x0c92fab3ad3db77aULL },
	{ &_fnv_test_str[28], (uint64)0x9b77794f5fdec421ULL },
	{ &_fnv_test_str[29], (uint64)0x0ac742dfe7874433ULL },
	{ &_fnv_test_str[30], (uint64)0xd7dad5766ad8e2deULL },
	{ &_fnv_test_str[31], (uint64)0xa1bb96378e897f5bULL },
	{ &_fnv_test_str[32], (uint64)0x5b3f9b6733a367d2ULL },
	{ &_fnv_test_str[33], (uint64)0xb07ce25cbea969f6ULL },
	{ &_fnv_test_str[34], (uint64)0x8d9e9997f9df0d6aULL },
	{ &_fnv_test_str[35], (uint64)0x838c673d9603cb7bULL },
	{ &_fnv_test_str[36], (uint64)0x8b5ee8a5e872c273ULL },
	{ &_fnv_test_str[37], (uint64)0x4507c4e9fb00690cULL },
	{ &_fnv_test_str[38], (uint64)0x4c9ca59581b27f45ULL },
	{ &_fnv_test_str[39], (uint64)0xe0aca20b624e4235ULL },
	{ &_fnv_test_str[40], (uint64)0xd8d5c8186ba98b94ULL },
	{ &_fnv_test_str[41], (uint64)0x1ccefc7ef118db81ULL },
	{ &_fnv_test_str[42], (uint64)0x0c92fab3ad3db71dULL },
	{ &_fnv_test_str[43], (uint64)0x9b77794f5fdec44eULL },
	{ &_fnv_test_str[44], (uint64)0x0ac742dfe7874413ULL },
	{ &_fnv_test_str[45], (uint64)0xd7dad5766ad8e2a9ULL },
	{ &_fnv_test_str[46], (uint64)0xa1bb96378e897f3aULL },
	{ &_fnv_test_str[47], (uint64)0x5b3f9b6733a367a1ULL },
	{ &_fnv_test_str[48], (uint64)0xb07ce25cbea969d6ULL },
	{ &_fnv_test_str[49], (uint64)0x8d9e9997f9df0d02ULL },
	{ &_fnv_test_str[50], (uint64)0x838c673d9603cb1eULL },
	{ &_fnv_test_str[51], (uint64)0x8b5ee8a5e872c201ULL },
	{ &_fnv_test_str[52], (uint64)0x4507c4e9fb006969ULL },
	{ &_fnv_test_str[53], (uint64)0x4c9ca59581b27f64ULL },
	{ &_fnv_test_str[54], (uint64)0xe0aca20b624e423fULL },
	{ &_fnv_test_str[55], (uint64)0x13998e580afa800fULL },
	{ &_fnv_test_str[56], (uint64)0x08326507b4eb3401ULL },
	{ &_fnv_test_str[57], (uint64)0xd8d5ad186ba95dc1ULL },
	{ &_fnv_test_str[58], (uint64)0x1c72e17ef0ca4e97ULL },
	{ &_fnv_test_str[59], (uint64)0x2183c1b327c38ae6ULL },
	{ &_fnv_test_str[60], (uint64)0xb66d096c914504f2ULL },
	{ &_fnv_test_str[61], (uint64)0x404bf57ad8476757ULL },
	{ &_fnv_test_str[62], (uint64)0x887976bd815498bbULL },
	{ &_fnv_test_str[63], (uint64)0x3afd7f02c2bf85a5ULL },
	{ &_fnv_test_str[64], (uint64)0xfc4476b0eb70177fULL },
	{ &_fnv_test_str[65], (uint64)0x186d2da00f77ecbaULL },
	{ &_fnv_test_str[66], (uint64)0xf97140fa48c74066ULL },
	{ &_fnv_test_str[67], (uint64)0xa2b1cf49aa926d37ULL },
	{ &_fnv_test_str[68], (uint64)0x0690712cd6cf940cULL },
	{ &_fnv_test_str[69], (uint64)0xf7045b3102b8906eULL },
	{ &_fnv_test_str[70], (uint64)0xd8d5ad186ba95db3ULL },
	{ &_fnv_test_str[71], (uint64)0x1c72e17ef0ca4ef3ULL },
	{ &_fnv_test_str[72], (uint64)0x2183c1b327c38a95ULL },
	{ &_fnv_test_str[73], (uint64)0xb66d096c914504d2ULL },
	{ &_fnv_test_str[74], (uint64)0x404bf57ad8476736ULL },
	{ &_fnv_test_str[75], (uint64)0x887976bd815498d5ULL },
	{ &_fnv_test_str[76], (uint64)0x3afd7f02c2bf85c1ULL },
	{ &_fnv_test_str[77], (uint64)0xfc4476b0eb70175fULL },
	{ &_fnv_test_str[78], (uint64)0x186d2da00f77eccdULL },
	{ &_fnv_test_str[79], (uint64)0xf97140fa48c7400eULL },
	{ &_fnv_test_str[80], (uint64)0xa2b1cf49aa926d52ULL },
	{ &_fnv_test_str[81], (uint64)0x0690712cd6cf9475ULL },
	{ &_fnv_test_str[82], (uint64)0xf7045b3102b89064ULL },
	{ &_fnv_test_str[83], (uint64)0x74f762479f9d6aeaULL },
	{ &_fnv_test_str[84], (uint64)0x08326007b4eb2b9cULL },
	{ &_fnv_test_str[85], (uint64)0xd8c4c9186b9b1a14ULL },
	{ &_fnv_test_str[86], (uint64)0x7b495389bdbdd4c7ULL },
	{ &_fnv_test_str[87], (uint64)0x3b6dba0d69908e25ULL },
	{ &_fnv_test_str[88], (uint64)0xd6b2b17bf4b71261ULL },
	{ &_fnv_test_str[89], (uint64)0x447bfb7f98e615b5ULL },
	{ &_fnv_test_str[90], (uint64)0xd6b2b17bf4b71262ULL },
	{ &_fnv_test_str[91], (uint64)0x3bd2807f93fe1660ULL },
	{ &_fnv_test_str[92], (uint64)0xd6b2b17bf4b71263ULL },
	{ &_fnv_test_str[93], (uint64)0x3329057f8f16170bULL },
	{ &_fnv_test_str[94], (uint64)0xd6b2b17bf4b71264ULL },
	{ &_fnv_test_str[95], (uint64)0x2a7f8a7f8a2e19b6ULL },
	{ &_fnv_test_str[96], (uint64)0x23d3767e64b2f98aULL },
	{ &_fnv_test_str[97], (uint64)0xff768d7e4f9d86a4ULL },
	{ &_fnv_test_str[98], (uint64)0x23d3767e64b2f984ULL },
	{ &_fnv_test_str[99], (uint64)0xccd1837e334e4aa6ULL },
	{ &_fnv_test_str[100], (uint64)0x23d3767e64b2f99aULL },
	{ &_fnv_test_str[101], (uint64)0x7691fd7e028f6754ULL },
	{ &_fnv_test_str[102], (uint64)0x34ad3b1041204318ULL },
	{ &_fnv_test_str[103], (uint64)0xa29e749ea9d201c8ULL },
	{ &_fnv_test_str[104], (uint64)0x34ad3b104120431bULL },
	{ &_fnv_test_str[105], (uint64)0xa29e779ea9d206e1ULL },
	{ &_fnv_test_str[106], (uint64)0x34ad3b104120431aULL },
	{ &_fnv_test_str[107], (uint64)0xa29e769ea9d2052eULL },
	{ &_fnv_test_str[108], (uint64)0x02a17ebca4aa3497ULL },
	{ &_fnv_test_str[109], (uint64)0x229ef18bcd375c95ULL },
	{ &_fnv_test_str[110], (uint64)0x02a17dbca4aa32c8ULL },
	{ &_fnv_test_str[111], (uint64)0x229b6f8bcd3449d8ULL },
	{ &_fnv_test_str[112], (uint64)0x02a184bca4aa3ed5ULL },
	{ &_fnv_test_str[113], (uint64)0x22b3618bcd48c3efULL },
	{ &_fnv_test_str[114], (uint64)0x5c2c346706186f36ULL },
	{ &_fnv_test_str[115], (uint64)0xb78c410f5b84f8c2ULL },
	{ &_fnv_test_str[116], (uint64)0xed9478212b267395ULL },
	{ &_fnv_test_str[117], (uint64)0xd9bbb55c5256662fULL },
	{ &_fnv_test_str[118], (uint64)0x8c54f0203249438aULL },
	{ &_fnv_test_str[119], (uint64)0xbd9790b5727dc37eULL },
	{ &_fnv_test_str[120], (uint64)0xa64e5f36c9e2b0e3ULL },
	{ &_fnv_test_str[121], (uint64)0x8fd0680da3088a04ULL },
	{ &_fnv_test_str[122], (uint64)0x67aad32c078284ccULL },
	{ &_fnv_test_str[123], (uint64)0xb37d55d81c57b331ULL },
	{ &_fnv_test_str[124], (uint64)0x55ac0f3829057c43ULL },
	{ &_fnv_test_str[125], (uint64)0xcb27f4b8e1b6cc20ULL },
	{ &_fnv_test_str[126], (uint64)0x26caf88bcbef2d19ULL },
	{ &_fnv_test_str[127], (uint64)0x8e6e063b97e61b8fULL },
	{ &_fnv_test_str[128], (uint64)0xb42750f7f3b7c37eULL },
	{ &_fnv_test_str[129], (uint64)0xf3c6ba64cf7ca99bULL },
	{ &_fnv_test_str[130], (uint64)0xebfb69b427ea80feULL },
	{ &_fnv_test_str[131], (uint64)0x39b50c3ed970f46cULL },
	{ &_fnv_test_str[132], (uint64)0x5b9b177aa3eb3e8aULL },
	{ &_fnv_test_str[133], (uint64)0x6510063ecf4ec903ULL },
	{ &_fnv_test_str[134], (uint64)0x2b3bbd2c00797c7aULL },
	{ &_fnv_test_str[135], (uint64)0xf1d6204ff5cb4aa7ULL },
	{ &_fnv_test_str[136], (uint64)0x4836e27ccf099f38ULL },
	{ &_fnv_test_str[137], (uint64)0x82efbb0dd073b44dULL },
	{ &_fnv_test_str[138], (uint64)0x4a80c282ffd7d4c6ULL },
	{ &_fnv_test_str[139], (uint64)0x305d1a9c9ee43bdfULL },
	{ &_fnv_test_str[140], (uint64)0x15c366948ffc6997ULL },
	{ &_fnv_test_str[141], (uint64)0x80153ae218916e7bULL },
	{ &_fnv_test_str[142], (uint64)0xfa23e2bdf9e2a9e1ULL },
	{ &_fnv_test_str[143], (uint64)0xd47e8d8a2333c6deULL },
	{ &_fnv_test_str[144], (uint64)0x7e128095f688b056ULL },
	{ &_fnv_test_str[145], (uint64)0x2f5356890efcedabULL },
	{ &_fnv_test_str[146], (uint64)0x95c2b383014f55c5ULL },
	{ &_fnv_test_str[147], (uint64)0x4727a5339ce6070fULL },
	{ &_fnv_test_str[148], (uint64)0xb0555ecd575108e9ULL },
	{ &_fnv_test_str[149], (uint64)0x48d785770bb4af37ULL },
	{ &_fnv_test_str[150], (uint64)0x09d4701c12af02b1ULL },
	{ &_fnv_test_str[151], (uint64)0x79f031e78f3cf62eULL },
	{ &_fnv_test_str[152], (uint64)0x52a1ee85db1b5a94ULL },
	{ &_fnv_test_str[153], (uint64)0x6bd95b2eb37fa6b8ULL },
	{ &_fnv_test_str[154], (uint64)0x74971b7077aef85dULL },
	{ &_fnv_test_str[155], (uint64)0xb4e4fae2ffcc1aadULL },
	{ &_fnv_test_str[156], (uint64)0x2bd48bd898b8f63aULL },
	{ &_fnv_test_str[157], (uint64)0xe9966ac1556257f6ULL },
	{ &_fnv_test_str[158], (uint64)0x92a3d1cd078ba293ULL },
	{ &_fnv_test_str[159], (uint64)0xf81175a482e20ab8ULL },
	{ &_fnv_test_str[160], (uint64)0x5bbb3de722e73048ULL },
	{ &_fnv_test_str[161], (uint64)0x6b4f363492b9f2beULL },
	{ &_fnv_test_str[162], (uint64)0xc2d559df73d59875ULL },
	{ &_fnv_test_str[163], (uint64)0xf75f62284bc7a8c2ULL },
	{ &_fnv_test_str[164], (uint64)0xda8dd8e116a9f1ccULL },
	{ &_fnv_test_str[165], (uint64)0xbdc1e6ab76057885ULL },
	{ &_fnv_test_str[166], (uint64)0xfec6a4238a1224a0ULL },
	{ &_fnv_test_str[167], (uint64)0xc03f40f3223e290eULL },
	{ &_fnv_test_str[168], (uint64)0x1ed21673466ffda9ULL },
	{ &_fnv_test_str[169], (uint64)0xdf70f906bb0dd2afULL },
	{ &_fnv_test_str[170], (uint64)0xf3dcda369f2af666ULL },
	{ &_fnv_test_str[171], (uint64)0x9ebb11573cdcebdeULL },
	{ &_fnv_test_str[172], (uint64)0x81c72d9077fedca0ULL },
	{ &_fnv_test_str[173], (uint64)0x0ec074a31be5fb15ULL },
	{ &_fnv_test_str[174], (uint64)0x2a8b3280b6c48f20ULL },
	{ &_fnv_test_str[175], (uint64)0xfd31777513309344ULL },
	{ &_fnv_test_str[176], (uint64)0x194534a86ad006b6ULL },
	{ &_fnv_test_str[177], (uint64)0x3be6fdf46e0cfe12ULL },
	{ &_fnv_test_str[178], (uint64)0x017cc137a07eb057ULL },
	{ &_fnv_test_str[179], (uint64)0x9428fc6e7d26b54dULL },
	{ &_fnv_test_str[180], (uint64)0x9aaa2e3603ef8ad7ULL },
	{ &_fnv_test_str[181], (uint64)0x82c6d3f3a0ccdf7dULL },
	{ &_fnv_test_str[182], (uint64)0xc86eeea00cf09b65ULL },
	{ &_fnv_test_str[183], (uint64)0x705f8189dbb58299ULL },
	{ &_fnv_test_str[184], (uint64)0x415a7f554391ca69ULL },
	{ &_fnv_test_str[185], (uint64)0xcfe3d49fa2bdc555ULL },
	{ &_fnv_test_str[186], (uint64)0xf0f9c56039b25191ULL },
	{ &_fnv_test_str[187], (uint64)0x7075cb6abd1d32d9ULL },
	{ &_fnv_test_str[188], (uint64)0x43c94e2c8b277509ULL },
	{ &_fnv_test_str[189], (uint64)0x3cbfd4e4ea670359ULL },
	{ &_fnv_test_str[190], (uint64)0xc05887810f4d019dULL },
	{ &_fnv_test_str[191], (uint64)0x14468ff93ac22dc5ULL },
	{ &_fnv_test_str[192], (uint64)0xebed699589d99c05ULL },
	{ &_fnv_test_str[193], (uint64)0x6d99f6df321ca5d5ULL },
	{ &_fnv_test_str[194], (uint64)0x0cd410d08c36d625ULL },
	{ &_fnv_test_str[195], (uint64)0xef1b2a2c86831d35ULL },
	{ &_fnv_test_str[196], (uint64)0x3b349c4d69ee5f05ULL },
	{ &_fnv_test_str[197], (uint64)0x55248ce88f45f035ULL },
	{ &_fnv_test_str[198], (uint64)0xaa69ca6a18a4c885ULL },
	{ &_fnv_test_str[199], (uint64)0x1fe3fce62bd816b5ULL },
	{ &_fnv_test_str[200], (uint64)0x0289a488a8df69d9ULL },
	{ &_fnv_test_str[201], (uint64)0x15e96e1613df98b5ULL },
	{ &_fnv_test_str[202], (uint64)0xe6be57375ad89b99ULL },
	{ NULL, (uint64)0 }
};

/* FNV-1a 64 bit test vectors */
static struct fnv1a_64_test_vector fnv1a_64_vector[] = {
	{ &_fnv_test_str[0], (uint64)0xcbf29ce484222325ULL },
	{ &_fnv_test_str[1], (uint64)0xaf63dc4c8601ec8cULL },
	{ &_fnv_test_str[2], (uint64)0xaf63df4c8601f1a5ULL },
	{ &_fnv_test_str[3], (uint64)0xaf63de4c8601eff2ULL },
	{ &_fnv_test_str[4], (uint64)0xaf63d94c8601e773ULL },
	{ &_fnv_test_str[5], (uint64)0xaf63d84c8601e5c0ULL },
	{ &_fnv_test_str[6], (uint64)0xaf63db4c8601ead9ULL },
	{ &_fnv_test_str[7], (uint64)0x08985907b541d342ULL },
	{ &_fnv_test_str[8], (uint64)0xdcb27518fed9d577ULL },
	{ &_fnv_test_str[9], (uint64)0xdd120e790c2512afULL },
	{ &_fnv_test_str[10], (uint64)0xcac165afa2fef40aULL },
	{ &_fnv_test_str[11], (uint64)0x85944171f73967e8ULL },
	{ &_fnv_test_str[12], (uint64)0xaf63bd4c8601b7dfULL },
	{ &_fnv_test_str[13], (uint64)0x089be207b544f1e4ULL },
	{ &_fnv_test_str[14], (uint64)0x08a61407b54d9b5fULL },
	{ &_fnv_test_str[15], (uint64)0x08a2ae07b54ab836ULL },
	{ &_fnv_test_str[16], (uint64)0x0891b007b53c4869ULL },
	{ &_fnv_test_str[17], (uint64)0x088e4a07b5396540ULL },
	{ &_fnv_test_str[18], (uint64)0x08987c07b5420ebbULL },
	{ &_fnv_test_str[19], (uint64)0xdcb28a18fed9f926ULL },
	{ &_fnv_test_str[20], (uint64)0xdd1270790c25b935ULL },
	{ &_fnv_test_str[21], (uint64)0xcac146afa2febf5dULL },
	{ &_fnv_test_str[22], (uint64)0x8593d371f738acfeULL },
	{ &_fnv_test_str[23], (uint64)0x34531ca7168b8f38ULL },
	{ &_fnv_test_str[24], (uint64)0x08a25607b54a22aeULL },
	{ &_fnv_test_str[25], (uint64)0xf5faf0190cf90df3ULL },
	{ &_fnv_test_str[26], (uint64)0xf27397910b3221c7ULL },
	{ &_fnv_test_str[27], (uint64)0x2c8c2b76062f22e0ULL },
	{ &_fnv_test_str[28], (uint64)0xe150688c8217b8fdULL },
	{ &_fnv_test_str[29], (uint64)0xf35a83c10e4f1f87ULL },
	{ &_fnv_test_str[30], (uint64)0xd1edd10b507344d0ULL },
	{ &_fnv_test_str[31], (uint64)0x2a5ee739b3ddb8c3ULL },
	{ &_fnv_test_str[32], (uint64)0xdcfb970ca1c0d310ULL },
	{ &_fnv_test_str[33], (uint64)0x4054da76daa6da90ULL },
	{ &_fnv_test_str[34], (uint64)0xf70a2ff589861368ULL },
	{ &_fnv_test_str[35], (uint64)0x4c628b38aed25f17ULL },
	{ &_fnv_test_str[36], (uint64)0x9dd1f6510f78189fULL },
	{ &_fnv_test_str[37], (uint64)0xa3de85bd491270ceULL },
	{ &_fnv_test_str[38], (uint64)0x858e2fa32a55e61dULL },
	{ &_fnv_test_str[39], (uint64)0x46810940eff5f915ULL },
	{ &_fnv_test_str[40], (uint64)0xf5fadd190cf8edaaULL },
	{ &_fnv_test_str[41], (uint64)0xf273ed910b32b3e9ULL },
	{ &_fnv_test_str[42], (uint64)0x2c8c5276062f6525ULL },
	{ &_fnv_test_str[43], (uint64)0xe150b98c821842a0ULL },
	{ &_fnv_test_str[44], (uint64)0xf35aa3c10e4f55e7ULL },
	{ &_fnv_test_str[45], (uint64)0xd1ed680b50729265ULL },
	{ &_fnv_test_str[46], (uint64)0x2a5f0639b3dded70ULL },
	{ &_fnv_test_str[47], (uint64)0xdcfbaa0ca1c0f359ULL },
	{ &_fnv_test_str[48], (uint64)0x4054ba76daa6a430ULL },
	{ &_fnv_test_str[49], (uint64)0xf709c7f5898562b0ULL },
	{ &_fnv_test_str[50], (uint64)0x4c62e638aed2f9b8ULL },
	{ &_fnv_test_str[51], (uint64)0x9dd1a8510f779415ULL },
	{ &_fnv_test_str[52], (uint64)0xa3de2abd4911d62dULL },
	{ &_fnv_test_str[53], (uint64)0x858e0ea32a55ae0aULL },
	{ &_fnv_test_str[54], (uint64)0x46810f40eff60347ULL },
	{ &_fnv_test_str[55], (uint64)0xc33bce57bef63eafULL },
	{ &_fnv_test_str[56], (uint64)0x08a24307b54a0265ULL },
	{ &_fnv_test_str[57], (uint64)0xf5b9fd190cc18d15ULL },
	{ &_fnv_test_str[58], (uint64)0x4c968290ace35703ULL },
	{ &_fnv_test_str[59], (uint64)0x07174bd5c64d9350ULL },
	{ &_fnv_test_str[60], (uint64)0x5a294c3ff5d18750ULL },
	{ &_fnv_test_str[61], (uint64)0x05b3c1aeb308b843ULL },
	{ &_fnv_test_str[62], (uint64)0xb92a48da37d0f477ULL },
	{ &_fnv_test_str[63], (uint64)0x73cdddccd80ebc49ULL },
	{ &_fnv_test_str[64], (uint64)0xd58c4c13210a266bULL },
	{ &_fnv_test_str[65], (uint64)0xe78b6081243ec194ULL },
	{ &_fnv_test_str[66], (uint64)0xb096f77096a39f34ULL },
	{ &_fnv_test_str[67], (uint64)0xb425c54ff807b6a3ULL },
	{ &_fnv_test_str[68], (uint64)0x23e520e2751bb46eULL },
	{ &_fnv_test_str[69], (uint64)0x1a0b44ccfe1385ecULL },
	{ &_fnv_test_str[70], (uint64)0xf5ba4b190cc2119fULL },
	{ &_fnv_test_str[71], (uint64)0x4c962690ace2baafULL },
	{ &_fnv_test_str[72], (uint64)0x0716ded5c64cda19ULL },
	{ &_fnv_test_str[73], (uint64)0x5a292c3ff5d150f0ULL },
	{ &_fnv_test_str[74], (uint64)0x05b3e0aeb308ecf0ULL },
	{ &_fnv_test_str[75], (uint64)0xb92a5eda37d119d9ULL },
	{ &_fnv_test_str[76], (uint64)0x73ce41ccd80f6635ULL },
	{ &_fnv_test_str[77], (uint64)0xd58c2c132109f00bULL },
	{ &_fnv_test_str[78], (uint64)0xe78baf81243f47d1ULL },
	{ &_fnv_test_str[79], (uint64)0xb0968f7096a2ee7cULL },
	{ &_fnv_test_str[80], (uint64)0xb425a84ff807855cULL },
	{ &_fnv_test_str[81], (uint64)0x23e4e9e2751b56f9ULL },
	{ &_fnv_test_str[82], (uint64)0x1a0b4eccfe1396eaULL },
	{ &_fnv_test_str[83], (uint64)0x54abd453bb2c9004ULL },
	{ &_fnv_test_str[84], (uint64)0x08ba5f07b55ec3daULL },
	{ &_fnv_test_str[85], (uint64)0x337354193006cb6eULL },
	{ &_fnv_test_str[86], (uint64)0xa430d84680aabd0bULL },
	{ &_fnv_test_str[87], (uint64)0xa9bc8acca21f39b1ULL },
	{ &_fnv_test_str[88], (uint64)0x6961196491cc682dULL },
	{ &_fnv_test_str[89], (uint64)0xad2bb1774799dfe9ULL },
	{ &_fnv_test_str[90], (uint64)0x6961166491cc6314ULL },
	{ &_fnv_test_str[91], (uint64)0x8d1bb3904a3b1236ULL },
	{ &_fnv_test_str[92], (uint64)0x6961176491cc64c7ULL },
	{ &_fnv_test_str[93], (uint64)0xed205d87f40434c7ULL },
	{ &_fnv_test_str[94], (uint64)0x6961146491cc5faeULL },
	{ &_fnv_test_str[95], (uint64)0xcd3baf5e44f8ad9cULL },
	{ &_fnv_test_str[96], (uint64)0xe3b36596127cd6d8ULL },
	{ &_fnv_test_str[97], (uint64)0xf77f1072c8e8a646ULL },
	{ &_fnv_test_str[98], (uint64)0xe3b36396127cd372ULL },
	{ &_fnv_test_str[99], (uint64)0x6067dce9932ad458ULL },
	{ &_fnv_test_str[100], (uint64)0xe3b37596127cf208ULL },
	{ &_fnv_test_str[101], (uint64)0x4b7b10fa9fe83936ULL },
	{ &_fnv_test_str[102], (uint64)0xaabafe7104d914beULL },
	{ &_fnv_test_str[103], (uint64)0xf4d3180b3cde3edaULL },
	{ &_fnv_test_str[104], (uint64)0xaabafd7104d9130bULL },
	{ &_fnv_test_str[105], (uint64)0xf4cfb20b3cdb5bb1ULL },
	{ &_fnv_test_str[106], (uint64)0xaabafc7104d91158ULL },
	{ &_fnv_test_str[107], (uint64)0xf4cc4c0b3cd87888ULL },
	{ &_fnv_test_str[108], (uint64)0xe729bac5d2a8d3a7ULL },
	{ &_fnv_test_str[109], (uint64)0x74bc0524f4dfa4c5ULL },
	{ &_fnv_test_str[110], (uint64)0xe72630c5d2a5b352ULL },
	{ &_fnv_test_str[111], (uint64)0x6b983224ef8fb456ULL },
	{ &_fnv_test_str[112], (uint64)0xe73042c5d2ae266dULL },
	{ &_fnv_test_str[113], (uint64)0x8527e324fdeb4b37ULL },
	{ &_fnv_test_str[114], (uint64)0x0a83c86fee952abcULL },
	{ &_fnv_test_str[115], (uint64)0x7318523267779d74ULL },
	{ &_fnv_test_str[116], (uint64)0x3e66d3d56b8caca1ULL },
	{ &_fnv_test_str[117], (uint64)0x956694a5c0095593ULL },
	{ &_fnv_test_str[118], (uint64)0xcac54572bb1a6fc8ULL },
	{ &_fnv_test_str[119], (uint64)0xa7a4c9f3edebf0d8ULL },
	{ &_fnv_test_str[120], (uint64)0x7829851fac17b143ULL },
	{ &_fnv_test_str[121], (uint64)0x2c8f4c9af81bcf06ULL },
	{ &_fnv_test_str[122], (uint64)0xd34e31539740c732ULL },
	{ &_fnv_test_str[123], (uint64)0x3605a2ac253d2db1ULL },
	{ &_fnv_test_str[124], (uint64)0x08c11b8346f4a3c3ULL },
	{ &_fnv_test_str[125], (uint64)0x6be396289ce8a6daULL },
	{ &_fnv_test_str[126], (uint64)0xd9b957fb7fe794c5ULL },
	{ &_fnv_test_str[127], (uint64)0x05be33da04560a93ULL },
	{ &_fnv_test_str[128], (uint64)0x0957f1577ba9747cULL },
	{ &_fnv_test_str[129], (uint64)0xda2cc3acc24fba57ULL },
	{ &_fnv_test_str[130], (uint64)0x74136f185b29e7f0ULL },
	{ &_fnv_test_str[131], (uint64)0xb2f2b4590edb93b2ULL },
	{ &_fnv_test_str[132], (uint64)0xb3608fce8b86ae04ULL },
	{ &_fnv_test_str[133], (uint64)0x4a3a865079359063ULL },
	{ &_fnv_test_str[134], (uint64)0x5b3a7ef496880a50ULL },
	{ &_fnv_test_str[135], (uint64)0x48fae3163854c23bULL },
	{ &_fnv_test_str[136], (uint64)0x07aaa640476e0b9aULL },
	{ &_fnv_test_str[137], (uint64)0x2f653656383a687dULL },
	{ &_fnv_test_str[138], (uint64)0xa1031f8e7599d79cULL },
	{ &_fnv_test_str[139], (uint64)0xa31908178ff92477ULL },
	{ &_fnv_test_str[140], (uint64)0x097edf3c14c3fb83ULL },
	{ &_fnv_test_str[141], (uint64)0xb51ca83feaa0971bULL },
	{ &_fnv_test_str[142], (uint64)0xdd3c0d96d784f2e9ULL },
	{ &_fnv_test_str[143], (uint64)0x86cd26a9ea767d78ULL },
	{ &_fnv_test_str[144], (uint64)0xe6b215ff54a30c18ULL },
	{ &_fnv_test_str[145], (uint64)0xec5b06a1c5531093ULL },
	{ &_fnv_test_str[146], (uint64)0x45665a929f9ec5e5ULL },
	{ &_fnv_test_str[147], (uint64)0x8c7609b4a9f10907ULL },
	{ &_fnv_test_str[148], (uint64)0x89aac3a491f0d729ULL },
	{ &_fnv_test_str[149], (uint64)0x32ce6b26e0f4a403ULL },
	{ &_fnv_test_str[150], (uint64)0x614ab44e02b53e01ULL },
	{ &_fnv_test_str[151], (uint64)0xfa6472eb6eef3290ULL },
	{ &_fnv_test_str[152], (uint64)0x9e5d75eb1948eb6aULL },
	{ &_fnv_test_str[153], (uint64)0xb6d12ad4a8671852ULL },
	{ &_fnv_test_str[154], (uint64)0x88826f56eba07af1ULL },
	{ &_fnv_test_str[155], (uint64)0x44535bf2645bc0fdULL },
	{ &_fnv_test_str[156], (uint64)0x169388ffc21e3728ULL },
	{ &_fnv_test_str[157], (uint64)0xf68aac9e396d8224ULL },
	{ &_fnv_test_str[158], (uint64)0x8e87d7e7472b3883ULL },
	{ &_fnv_test_str[159], (uint64)0x295c26caa8b423deULL },
	{ &_fnv_test_str[160], (uint64)0x322c814292e72176ULL },
	{ &_fnv_test_str[161], (uint64)0x8a06550eb8af7268ULL },
	{ &_fnv_test_str[162], (uint64)0xef86d60e661bcf71ULL },
	{ &_fnv_test_str[163], (uint64)0x9e5426c87f30ee54ULL },
	{ &_fnv_test_str[164], (uint64)0xf1ea8aa826fd047eULL },
	{ &_fnv_test_str[165], (uint64)0x0babaf9a642cb769ULL },
	{ &_fnv_test_str[166], (uint64)0x4b3341d4068d012eULL },
	{ &_fnv_test_str[167], (uint64)0xd15605cbc30a335cULL },
	{ &_fnv_test_str[168], (uint64)0x5b21060aed8412e5ULL },
	{ &_fnv_test_str[169], (uint64)0x45e2cda1ce6f4227ULL },
	{ &_fnv_test_str[170], (uint64)0x50ae3745033ad7d4ULL },
	{ &_fnv_test_str[171], (uint64)0xaa4588ced46bf414ULL },
	{ &_fnv_test_str[172], (uint64)0xc1b0056c4a95467eULL },
	{ &_fnv_test_str[173], (uint64)0x56576a71de8b4089ULL },
	{ &_fnv_test_str[174], (uint64)0xbf20965fa6dc927eULL },
	{ &_fnv_test_str[175], (uint64)0x569f8383c2040882ULL },
	{ &_fnv_test_str[176], (uint64)0xe1e772fba08feca0ULL },
	{ &_fnv_test_str[177], (uint64)0x4ced94af97138ac4ULL },
	{ &_fnv_test_str[178], (uint64)0xc4112ffb337a82fbULL },
	{ &_fnv_test_str[179], (uint64)0xd64a4fd41de38b7dULL },
	{ &_fnv_test_str[180], (uint64)0x4cfc32329edebcbbULL },
	{ &_fnv_test_str[181], (uint64)0x0803564445050395ULL },
	{ &_fnv_test_str[182], (uint64)0xaa1574ecf4642ffdULL },
	{ &_fnv_test_str[183], (uint64)0x694bc4e54cc315f9ULL },
	{ &_fnv_test_str[184], (uint64)0xa3d7cb273b011721ULL },
	{ &_fnv_test_str[185], (uint64)0x577c2f8b6115bfa5ULL },
	{ &_fnv_test_str[186], (uint64)0xb7ec8c1a769fb4c1ULL },
	{ &_fnv_test_str[187], (uint64)0x5d5cfce63359ab19ULL },
	{ &_fnv_test_str[188], (uint64)0x33b96c3cd65b5f71ULL },
	{ &_fnv_test_str[189], (uint64)0xd845097780602bb9ULL },
	{ &_fnv_test_str[190], (uint64)0x84d47645d02da3d5ULL },
	{ &_fnv_test_str[191], (uint64)0x83544f33b58773a5ULL },
	{ &_fnv_test_str[192], (uint64)0x9175cbb2160836c5ULL },
	{ &_fnv_test_str[193], (uint64)0xc71b3bc175e72bc5ULL },
	{ &_fnv_test_str[194], (uint64)0x636806ac222ec985ULL },
	{ &_fnv_test_str[195], (uint64)0xb6ef0e6950f52ed5ULL },
	{ &_fnv_test_str[196], (uint64)0xead3d8a0f3dfdaa5ULL },
	{ &_fnv_test_str[197], (uint64)0x922908fe9a861ba5ULL },
	{ &_fnv_test_str[198], (uint64)0x6d4821de275fd5c5ULL },
	{ &_fnv_test_str[199], (uint64)0x1fe3fce62bd816b5ULL },
	{ &_fnv_test_str[200], (uint64)0xc23e9fccd6f70591ULL },
	{ &_fnv_test_str[201], (uint64)0xc1af12bdfe16b5b5ULL },
	{ &_fnv_test_str[202], (uint64)0x39e9f18f2f85e221ULL },
	{ NULL, (uint64)0 }
};

//#include <..\SLib\fnv-503\fnv.h>

extern uint32 fnv_32_buf(const void * buf, size_t len, uint32 hashval);
extern uint32 fnv_32a_buf(const void * buf, size_t len, uint32 hashval);
extern uint64 fnv_64_buf(const void * buf, size_t len, uint64 hashval);
extern uint64 fnv_64a_buf(const void * buf, size_t len, uint64 hashval);

SLTEST_R(HashFunctionFnv) // @construction
{
	{
		for(uint i = 0; i < SIZEOFARRAY(fnv0_32_vector); i++) {
			const fnv0_32_test_vector & r_entry = fnv0_32_vector[i];
			if(r_entry.test) {
				uint32 hval = fnv_32_buf(r_entry.test->buf, r_entry.test->len, SlConst::FnvHash0Init32);
				SLCHECK_EQ(hval, r_entry.fnv0_32);
			}
		}
	}
	{
		for(uint i = 0; i < SIZEOFARRAY(fnv1_32_vector); i++) {
			const fnv1_32_test_vector & r_entry = fnv1_32_vector[i];
			if(r_entry.test) {
				uint32 hval = fnv_32_buf(r_entry.test->buf, r_entry.test->len, SlConst::FnvHash1Init32);
				SLCHECK_EQ(hval, r_entry.fnv1_32);
			}
		}
	}
	{
		for(uint i = 0; i < SIZEOFARRAY(fnv1a_32_vector); i++) {
			const fnv1a_32_test_vector & r_entry = fnv1a_32_vector[i];
			if(r_entry.test) {
				uint32 hval = fnv_32a_buf(r_entry.test->buf, r_entry.test->len, SlConst::FnvHash1Init32);
				SLCHECK_EQ(hval, r_entry.fnv1a_32);
			}
		}
	}
	{
		for(uint i = 0; i < SIZEOFARRAY(fnv0_64_vector); i++) {
			const fnv0_64_test_vector & r_entry = fnv0_64_vector[i];
			if(r_entry.test) {
				uint64 hval = fnv_64_buf(r_entry.test->buf, r_entry.test->len, SlConst::FnvHash0Init64);
				SLCHECK_EQ(hval, r_entry.fnv0_64);
			}
		}
	}
	{
		for(uint i = 0; i < SIZEOFARRAY(fnv1_64_vector); i++) {
			const fnv1_64_test_vector & r_entry = fnv1_64_vector[i];
			if(r_entry.test) {
				uint64 hval = fnv_64_buf(r_entry.test->buf, r_entry.test->len, SlConst::FnvHash1Init64);
				SLCHECK_EQ(hval, r_entry.fnv1_64);
			}
		}
	}
	{
		for(uint i = 0; i < SIZEOFARRAY(fnv1a_64_vector); i++) {
			const fnv1a_64_test_vector & r_entry = fnv1a_64_vector[i];
			if(r_entry.test) {
				uint64 hval = fnv_64a_buf(r_entry.test->buf, r_entry.test->len, SlConst::FnvHash1Init64);
				SLCHECK_EQ(hval, r_entry.fnv1a_64);
			}
		}
	}
	return CurrentStatus;
}