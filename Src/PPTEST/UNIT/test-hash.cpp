// TEST-HASH.CPP
// Copyright (c) A.Sobolev 2023
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
				//uint32 pattern_value = PTR32C(p_item->Out.GetBufC())[0];
				//PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
				//PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
				//PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
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
				//uint32 pattern_value = PTR32C(p_item->Out.GetBufC())[0];
				//PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
				//PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
				//PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
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
					uint32 pattern_value = PTR32(p_item->Out.GetBuf())[0];
					PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
					PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
					PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
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
					{
						//
						// Проверка старой реализации (которой всю жизнь пользуемся)
						//
						SCRC32 cc;
						uint32 result = cc.Calc(0, (const uint8 *)p_item->In.GetBuf(), p_item->In.GetLen());
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
					uint32 pattern_value = PTR32(p_item->Out.GetBuf())[0];
					PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
					PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
					PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
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
	uint32 h = 0 ^ len;
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
					THROW(SLCHECK_EQ(real_bin_size, 4U));
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
