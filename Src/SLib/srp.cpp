// SRP.CPP
// Copyright (c) A.Sobolev 2021
//
/*
 * Secure Remote Password 6a implementation
 * Copyright (c) 2010 Tom Cocagne. All rights reserved.
 * https://github.com/cocagne/csrp
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Tom Cocagne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <slib-internal.h>
#pragma hdrstop
#ifdef WIN32
    #include <Wincrypt.h>
#else
    #include <sys/time.h>
#endif
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

// @debug #define SRP_NETWORK_DEBUG 

// 
// Out: bytes_B, len_B.
// 
// On failure, bytes_B will be set to NULL and len_B will be set to 0
// 
// The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type
// 
/*SrpVerifier * srp_verifier_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username,
    const uchar * bytes_s, int len_s, const uchar * bytes_v, int len_v, const uchar * bytes_A, int len_A,
    const uchar ** bytes_B, int * len_B, const char * n_hex, const char * g_hex);*/
//void srp_verifier_delete(SrpVerifier * ver);
//int srp_verifier_is_authenticated(SrpVerifier * ver);
//const char * srp_verifier_get_username(SrpVerifier * ver);
/* key_length may be null */
//const uchar * srp_verifier_get_session_key(SrpVerifier * ver, int * key_length);
//int srp_verifier_get_session_key_length(SrpVerifier * ver);
// user_M must be exactly SrpVerifier::GetSessionKeyLength() bytes in size 
//void srp_verifier_verify_session(SrpVerifier * ver, const uchar * user_M, const uchar ** bytes_HAMK);

/*******************************************************************************/

//SrpUser * srp_user_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username, const uchar * bytes_password, int len_password, const char * n_hex, const char * g_hex);
//void srp_user_delete(SrpUser * usr);
//const char * srp_user_get_username(SrpUser * usr);
// key_length may be null 
//const uchar * srp_user_get_session_key(SrpUser * usr, int * key_length);
//int srp_user_get_session_key_length(SrpUser * usr);
// Output: username, bytes_A, len_A 
//void srp_user_start_authentication(SrpUser * usr, const char ** username, const uchar ** bytes_A, int * len_A);
// Output: bytes_M, len_M  (len_M may be null and will always be srp_user_get_session_key_length() bytes in size) 
//void srp_user_process_challenge(SrpUser * usr, const uchar * bytes_s, int len_s, const uchar * bytes_B, int len_B, const uchar ** bytes_M, int * len_M);

static int g_initialized = 0;

struct NGConstant {
	BIGNUM * N;
	BIGNUM * g;
};

struct NGHex {
	const char * n_hex;
	const char * g_hex;
};
//
// All constants here were pulled from Appendix A of RFC 5054 
//
static struct NGHex global_Ng_constants[] = {
	{ /* 1024 */
		"EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496"
		"EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8E"
		"F4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA"
		"9AFD5138FE8376435B9FC61D2FC0EB06E3",
		"2"
	},
	{ /* 2048 */
		"AC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4"
		"A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF60"
		"95179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF"
		"747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B907"
		"8717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB37861"
		"60279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DB"
		"FBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73",
		"2"
	},
	{ /* 4096 */
		"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"
		"8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"
		"302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"
		"A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE6"
		"49286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8"
		"FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D"
		"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C"
		"180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"
		"3995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D"
		"04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7D"
		"B3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D226"
		"1AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200C"
		"BBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFC"
		"E0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B26"
		"99C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB"
		"04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2"
		"233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127"
		"D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C934063199"
		"FFFFFFFFFFFFFFFF",
		"5"
	},
	{ /* 8192 */
		"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"
		"8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"
		"302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"
		"A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE6"
		"49286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8"
		"FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D"
		"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C"
		"180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"
		"3995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D"
		"04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7D"
		"B3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D226"
		"1AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200C"
		"BBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFC"
		"E0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B26"
		"99C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB"
		"04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2"
		"233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127"
		"D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C934028492"
		"36C3FAB4D27C7026C1D4DCB2602646DEC9751E763DBA37BDF8FF9406"
		"AD9E530EE5DB382F413001AEB06A53ED9027D831179727B0865A8918"
		"DA3EDBEBCF9B14ED44CE6CBACED4BB1BDB7F1447E6CC254B33205151"
		"2BD7AF426FB8F401378CD2BF5983CA01C64B92ECF032EA15D1721D03"
		"F482D7CE6E74FEF6D55E702F46980C82B5A84031900B1C9E59E7C97F"
		"BEC7E8F323A97A7E36CC88BE0F1D45B7FF585AC54BD407B22B4154AA"
		"CC8F6D7EBF48E1D814CC5ED20F8037E0A79715EEF29BE32806A1D58B"
		"B7C5DA76F550AA3D8A1FBFF0EB19CCB1A313D55CDA56C9EC2EF29632"
		"387FE8D76E3C0468043E8F663F4860EE12BF2D5B0B7474D6E694F91E"
		"6DBE115974A3926F12FEE5E438777CB6A932DF8CD8BEC4D073B931BA"
		"3BC832B68D9DD300741FA7BF8AFC47ED2576F6936BA424663AAB639C"
		"5AE4F5683423B4742BF1C978238F16CBE39D652DE3FDB8BEFC848AD9"
		"22222E04A4037C0713EB57A81A23F0C73473FC646CEA306B4BCBC886"
		"2F8385DDFA9D4B7FA2C087E879683303ED5BDD3A062B3CF5B3A278A6"
		"6D2A13F83F44F82DDF310EE074AB6A364597E899A0255DC164F31CC5"
		"0846851DF9AB48195DED7EA1B1D510BD7EE74D73FAF36BC31ECFA268"
		"359046F4EB879F924009438B481C6CD7889A002ED5EE382BC9190DA6"
		"FC026E479558E4475677E9AA9E3050E2765694DFC81F56E880B96E71"
		"60C980DD98EDD3DFFFFFFFFFFFFFFFFF",
		"13"
	},
	{0, 0} /* null sentinel */
};

static void delete_ng(NGConstant * ng)
{
	if(ng) {
		BN_free(ng->N);
		BN_free(ng->g);
		ng->N = 0;
		ng->g = 0;
		SAlloc::F(ng);
	}
}

union HashCTX {
	SHA_CTX sha;
	SHA256_CTX sha256;
	SHA512_CTX sha512;
};

static NGConstant * new_ng(SlSRP::NGType ng_type, const char * n_hex, const char * g_hex)
{
	NGConstant * ng = static_cast<NGConstant *>(SAlloc::M(sizeof(NGConstant)));
	ng->N = BN_new();
	ng->g = BN_new();
	if(!ng || !ng->N || !ng->g)
		return 0;
	if(ng_type != SlSRP::SRP_NG_CUSTOM) {
		n_hex = global_Ng_constants[ng_type].n_hex;
		g_hex = global_Ng_constants[ng_type].g_hex;
	}
	BN_hex2bn(&ng->N, n_hex);
	BN_hex2bn(&ng->g, g_hex);
	return ng;
}

static int hash_init(SlSRP::HashAlgorithm alg, HashCTX * c)
{
	switch(alg) {
		case SlSRP::SRP_SHA1: return SHA1_Init(&c->sha);
		case SlSRP::SRP_SHA224: return SHA224_Init(&c->sha256);
		case SlSRP::SRP_SHA256: return SHA256_Init(&c->sha256);
		case SlSRP::SRP_SHA384: return SHA384_Init(&c->sha512);
		case SlSRP::SRP_SHA512: return SHA512_Init(&c->sha512);
		default: return -1;
	};
}

static int hash_update(SlSRP::HashAlgorithm alg, HashCTX * c, const void * data, size_t len)
{
	switch(alg) {
		case SlSRP::SRP_SHA1: return SHA1_Update(&c->sha, data, len);
		case SlSRP::SRP_SHA224: return SHA224_Update(&c->sha256, data, len);
		case SlSRP::SRP_SHA256: return SHA256_Update(&c->sha256, data, len);
		case SlSRP::SRP_SHA384: return SHA384_Update(&c->sha512, data, len);
		case SlSRP::SRP_SHA512: return SHA512_Update(&c->sha512, data, len);
		default:
		    return -1;
	};
}

static int hash_final(SlSRP::HashAlgorithm alg, HashCTX * c, uchar * md)
{
	switch(alg) {
		case SlSRP::SRP_SHA1: return SHA1_Final(md, &c->sha);
		case SlSRP::SRP_SHA224: return SHA224_Final(md, &c->sha256);
		case SlSRP::SRP_SHA256: return SHA256_Final(md, &c->sha256);
		case SlSRP::SRP_SHA384: return SHA384_Final(md, &c->sha512);
		case SlSRP::SRP_SHA512: return SHA512_Final(md, &c->sha512);
		default:
		    return -1;
	};
}

static uchar * hash(SlSRP::HashAlgorithm alg, const uchar * d, size_t n, uchar * md)
{
	switch(alg) {
		case SlSRP::SRP_SHA1: return SHA1(d, n, md);
		case SlSRP::SRP_SHA224: return SHA224(d, n, md);
		case SlSRP::SRP_SHA256: return SHA256(d, n, md);
		case SlSRP::SRP_SHA384: return SHA384(d, n, md);
		case SlSRP::SRP_SHA512: return SHA512(d, n, md);
		default: return 0;
	};
}

static int hash_length(SlSRP::HashAlgorithm alg)
{
	switch(alg) {
		case SlSRP::SRP_SHA1: return SHA_DIGEST_LENGTH;
		case SlSRP::SRP_SHA224: return SHA224_DIGEST_LENGTH;
		case SlSRP::SRP_SHA256: return SHA256_DIGEST_LENGTH;
		case SlSRP::SRP_SHA384: return SHA384_DIGEST_LENGTH;
		case SlSRP::SRP_SHA512: return SHA512_DIGEST_LENGTH;
		default: return -1;
	};
}

static BIGNUM * H_nn(SlSRP::HashAlgorithm alg, const BIGNUM * n1, const BIGNUM * n2)
{
	uchar buff[SHA512_DIGEST_LENGTH];
	int len_n1 = BN_num_bytes(n1);
	int len_n2 = BN_num_bytes(n2);
	int nbytes = len_n1 + len_n2;
	uchar * bin = (uchar *)SAlloc::M(nbytes);
	if(!bin)
		return 0;
	BN_bn2bin(n1, bin);
	BN_bn2bin(n2, bin + len_n1);
	hash(alg, bin, nbytes, buff);
	SAlloc::F(bin);
	return BN_bin2bn(buff, hash_length(alg), NULL);
}

static BIGNUM * H_ns(SlSRP::HashAlgorithm alg, const BIGNUM * n, const uchar * bytes, int len_bytes)
{
	uchar buff[SHA512_DIGEST_LENGTH];
	int len_n  = BN_num_bytes(n);
	int nbytes = len_n + len_bytes;
	uchar * bin = (uchar *)SAlloc::M(nbytes);
	if(!bin)
		return 0;
	BN_bn2bin(n, bin);
	memcpy(bin + len_n, bytes, len_bytes);
	hash(alg, bin, nbytes, buff);
	SAlloc::F(bin);
	return BN_bin2bn(buff, hash_length(alg), NULL);
}

static BIGNUM * calculate_x(SlSRP::HashAlgorithm alg, const BIGNUM * salt, const char * username, const uchar * password, int password_len)
{
	uchar ucp_hash[SHA512_DIGEST_LENGTH];
	HashCTX ctx;
	hash_init(alg, &ctx);
	hash_update(alg, &ctx, username, strlen(username));
	hash_update(alg, &ctx, ":", 1);
	hash_update(alg, &ctx, password, password_len);
	hash_final(alg, &ctx, ucp_hash);
	return H_ns(alg, salt, ucp_hash, hash_length(alg));
}

static void update_hash_n(SlSRP::HashAlgorithm alg, HashCTX * ctx, const BIGNUM * n)
{
	ulong len = BN_num_bytes(n);
	uchar * n_bytes = static_cast<uchar *>(SAlloc::M(len));
	if(n_bytes) {
		BN_bn2bin(n, n_bytes);
		hash_update(alg, ctx, n_bytes, len);
		SAlloc::F(n_bytes);
	}
}

static void hash_num(SlSRP::HashAlgorithm alg, const BIGNUM * n, uchar * dest)
{
	int nbytes = BN_num_bytes(n);
	uchar * bin = static_cast<uchar *>(SAlloc::M(nbytes));
	if(bin) {
		BN_bn2bin(n, bin);
		hash(alg, bin, nbytes, dest);
		SAlloc::F(bin);
	}
}

static void calculate_M(SlSRP::HashAlgorithm alg, NGConstant * ng, uchar * dest, const char * I, const BIGNUM * s,
    const BIGNUM * A, const BIGNUM * B, const uchar * K)
{
	uchar H_N[SHA512_DIGEST_LENGTH];
	uchar H_g[SHA512_DIGEST_LENGTH];
	uchar H_I[SHA512_DIGEST_LENGTH];
	uchar H_xor[SHA512_DIGEST_LENGTH];
	HashCTX ctx;
	int i = 0;
	int hash_len = hash_length(alg);
	hash_num(alg, ng->N, H_N);
	hash_num(alg, ng->g, H_g);
	hash(alg, (const uchar *)I, strlen(I), H_I);
	for(i = 0; i < hash_len; i++)
		H_xor[i] = H_N[i] ^ H_g[i];
	hash_init(alg, &ctx);
	hash_update(alg, &ctx, H_xor, hash_len);
	hash_update(alg, &ctx, H_I,   hash_len);
	update_hash_n(alg, &ctx, s);
	update_hash_n(alg, &ctx, A);
	update_hash_n(alg, &ctx, B);
	hash_update(alg, &ctx, K, hash_len);
	hash_final(alg, &ctx, dest);
}

static void calculate_H_AMK(SlSRP::HashAlgorithm alg, uchar * dest, const BIGNUM * A, const uchar * M, const uchar * K)
{
	HashCTX ctx;
	hash_init(alg, &ctx);
	update_hash_n(alg, &ctx, A);
	hash_update(alg, &ctx, M, hash_length(alg) );
	hash_update(alg, &ctx, K, hash_length(alg) );
	hash_final(alg, &ctx, dest);
}

static void init_random()
{
#ifdef WIN32
	HCRYPTPROV wctx;
#else
	FILE   * fp   = 0;
#endif
	uchar buff[64];
	if(g_initialized)
		return;
#ifdef WIN32
	CryptAcquireContext(&wctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptGenRandom(wctx, sizeof(buff), (BYTE*)buff);
	CryptReleaseContext(wctx, 0);
	g_initialized = 1;
#else
	fp = fopen("/dev/urandom", "r");
	if(fp) {
		size_t read = fread(buff, sizeof(buff), 1, fp);
		g_initialized = read == 1;
		fclose(fp);
	}
#endif
	if(g_initialized)
		RAND_seed(buff, sizeof(buff) );
}
//
// Exported Functions
//

void SlSRP::User::VerifySession(const uchar * bytes_HAMK)
{
	if(memcmp(H_AMK, bytes_HAMK, hash_length(HashAlg)) == 0)
		Authenticated = 1;
}

const uchar * SlSRP::User::GetSessionKey(int * pKeyLength) const
{
	ASSIGN_PTR(pKeyLength, hash_length(HashAlg));
	return SessionKey;
}

//void srp_random_seed(const uchar * pRandomData, int dataLength)
/*static*/void SlSRP::RandomSeed(const uchar * pRandomData, int dataLength)
{
	g_initialized = 1;
	if(pRandomData)
		RAND_seed(pRandomData, dataLength);
}

/*static*/int SlSRP::CreateSaltedVerificationKey2(HashAlgorithm alg, NGType ng_type, const char * pUserName,
		const uchar * pPassword, int lenPassword, SBinaryChunk & rS, SBinaryChunk & rV, const char * n_hex, const char * g_hex)
{
	int    ok = 1;
	BIGNUM * s = BN_new();
	BIGNUM * v = BN_new();
	BIGNUM * x = 0;
	BN_CTX * ctx = BN_CTX_new();
	NGConstant * ng  = new_ng(ng_type, n_hex, g_hex);
	THROW(s && v && ctx && ng);
	init_random(); // Only happens once 
	BN_rand(s, 32, -1, 0);
	x = calculate_x(alg, s, pUserName, pPassword, lenPassword);
	THROW(x);
	BN_mod_exp(v, ng->g, x, ng->N, ctx);
	{
		THROW(rS.Ensure(BN_num_bytes(s)));
		THROW(rV.Ensure(BN_num_bytes(v)));
		BN_bn2bin(s, static_cast<uchar *>(rS.Ptr()));
		BN_bn2bin(v, static_cast<uchar *>(rV.Ptr()));
	}
	CATCHZOK
	delete_ng(ng);
	BN_free(s);
	BN_free(v);
	BN_free(x);
	BN_CTX_free(ctx);
	return ok;
}
//
// Out: bytes_s, len_s, bytes_v, len_v
//
// The caller is responsible for freeing the memory allocated for bytes_s and bytes_v
// 
// The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type.
// If provided, they must contain ASCII text of the hexidecimal notation.
// 
#if 0 // {
/*static*/void SlSRP::CreateSaltedVerificationKey(HashAlgorithm alg, NGType ng_type, const char * pUserName,
    const uchar * pPassword, int lenPassword, uchar ** ppBytesS, int * len_s, uchar ** ppBytesV, int * len_v, const char * n_hex, const char * g_hex)
{
	BIGNUM * s   = BN_new();
	BIGNUM * v   = BN_new();
	BIGNUM * x   = 0;
	BN_CTX * ctx = BN_CTX_new();
	NGConstant * ng  = new_ng(ng_type, n_hex, g_hex);
	if(!s || !v || !ctx || !ng)
		goto cleanup_and_exit;
	init_random(); // Only happens once 
	BN_rand(s, 32, -1, 0);
	x = calculate_x(alg, s, pUserName, pPassword, lenPassword);
	if(!x)
		goto cleanup_and_exit;
	BN_mod_exp(v, ng->g, x, ng->N, ctx);
	*len_s = BN_num_bytes(s);
	*len_v = BN_num_bytes(v);
	*ppBytesS = (uchar *)SAlloc::M(*len_s);
	*ppBytesV = (uchar *)SAlloc::M(*len_v);
	if(!ppBytesS || !ppBytesV)
		goto cleanup_and_exit;
	BN_bn2bin(s, *ppBytesS);
	BN_bn2bin(v, *ppBytesV);
cleanup_and_exit:
	delete_ng(ng);
	BN_free(s);
	BN_free(v);
	BN_free(x);
	BN_CTX_free(ctx);
}
#endif // } 0

SlSRP::VerifierBase::VerifierBase(HashAlgorithm alg) : HashAlg(alg), P_Ng(0), Authenticated(0)
{
	memzero(M, sizeof(M));
	memzero(H_AMK, sizeof(H_AMK));
	memzero(SessionKey, sizeof(SessionKey));
}

SlSRP::VerifierBase::~VerifierBase()
{
	delete_ng(P_Ng);
}

SlSRP::Verifier::Verifier(HashAlgorithm alg, NGType ngType, const char * pUserName,
	const SBinaryChunk & rS, const SBinaryChunk & rV, const SBinaryChunk & rA, SBinaryChunk & rB, const char * pNHex, const char * pGHex) :
	VerifierBase(alg), UserName(pUserName)
{
	BIGNUM * p_s = BN_bin2bn(static_cast<const uchar *>(rS.PtrC()), rS.Len(), NULL);
	BIGNUM * p_v = BN_bin2bn(static_cast<const uchar *>(rV.PtrC()), rV.Len(), NULL);
	BIGNUM * p_A = BN_bin2bn(static_cast<const uchar *>(rA.PtrC()), rA.Len(), NULL);
	BIGNUM * p_u = 0;
	BIGNUM * p_B = BN_new();
	BIGNUM * p_S = BN_new();
	BIGNUM * p_b = BN_new();
	BIGNUM * p_k = 0;
	BIGNUM * tmp1 = BN_new();
	BIGNUM * tmp2 = BN_new();
	BN_CTX * ctx  = BN_CTX_new();
	//int ulen = strlen(pUserName) + 1;
	NGConstant * ng = new_ng(ngType, pNHex, pGHex);
	//SrpVerifier * ver = 0;
	//*pLenB   = 0;
	//*ppBytesB = 0;
	rB.Z();
	THROW(p_s && p_v && p_A && p_B && p_S && p_b && tmp1 && tmp2 && ctx && ng);
	//if(!p_s || !p_v || !p_A || !p_B || !p_S || !p_b || !tmp1 || !tmp2 || !ctx || !ng) goto cleanup_and_exit;
	//ver = (SrpVerifier *)SAlloc::M(sizeof(SrpVerifier));
	init_random(); // Only happens once 
	P_Ng = ng;
	//THROW(P_UserName = (char *)SAlloc::M(ulen));
	//memcpy((char*)P_UserName, pUserName, ulen);
	//Authenticated = 0;
	// SRP-6a safety check 
	BN_mod(tmp1, p_A, ng->N, ctx);
	if(!BN_is_zero(tmp1)) {
		//BN_rand(p_b, 256, -1, 0);
		//"263f95a17dae6f22c28d847a9f87d6ab3f6147e2734816697e119a92c2c59ee9" // @debug
#ifdef SRP_NETWORK_DEBUG
		BN_hex2bn(&p_b, "263f95a17dae6f22c28d847a9f87d6ab3f6147e2734816697e119a92c2c59ee9");
#else
		BN_rand(p_b, 256, -1, 0);
#endif
		p_k = H_nn(alg, ng->N, ng->g);
		// B = kv + g^b 
		BN_mul(tmp1, p_k, p_v, ctx);
		BN_mod_exp(tmp2, ng->g, p_b, ng->N, ctx);
		BN_mod_add(p_B, tmp1, tmp2, ng->N, ctx);
		p_u = H_nn(alg, p_A, p_B);
		// S = (A *(v^u)) ^ b 
		BN_mod_exp(tmp1, p_v, p_u, ng->N, ctx);
		BN_mul(tmp2, p_A, tmp1, ctx);
		BN_mod_exp(p_S, tmp2, p_b, ng->N, ctx);
		hash_num(alg, p_S, SessionKey);
		calculate_M(alg, ng, M, UserName.cptr(), p_s, p_A, p_B, SessionKey);
		calculate_H_AMK(alg, H_AMK, p_A, M, SessionKey);
		//*pLenB   = BN_num_bytes(p_B);
		//*ppBytesB = (const uchar *)SAlloc::M(*pLenB);
		//if(!*ppBytesB) {
		if(!rB.Ensure(BN_num_bytes(p_B))) {
			rB.Z();
			//SAlloc::F((void*)P_UserName);
			//*pLenB = 0;
			goto cleanup_and_exit;
		}
		BN_bn2bin(p_B, static_cast<uchar *>(rB.Ptr()));
		BytesB = rB;
		//P_BytesB = *ppBytesB;
	}
cleanup_and_exit:
	CATCH
		rB.Z();
		//SAlloc::F((void*)P_UserName);
	ENDCATCH
	BN_free(p_s);
	BN_free(p_v);
	BN_free(p_A);
	BN_free(p_u);
	BN_free(p_k);
	BN_free(p_B);
	BN_free(p_S);
	BN_free(p_b);
	BN_free(tmp1);
	BN_free(tmp2);
	BN_CTX_free(ctx);
	//return ver;
}

/* Out: bytes_B, len_B.
 *
 * On failure, bytes_B will be set to NULL and len_B will be set to 0
 */
/*SrpVerifier * srp_verifier_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username,
    const uchar * bytes_s, int len_s, const uchar * bytes_v, int len_v, const uchar * bytes_A, int len_A,
    const uchar ** bytes_B, int * len_B, const char * n_hex, const char * g_hex)*/
#if 0 // {
SlSRP::Verifier::Verifier(SlSRP::HashAlgorithm alg, NGType ngType, const char * pUserName,
	const uchar * pBytesS, int lenS, const uchar * pBytesV, int lenV, const uchar * pBytesA, int lenA,
	const uchar ** ppBytesB, int * pLenB, const char * pNHex, const char * pGHex)
{
	BIGNUM * p_s = BN_bin2bn(pBytesS, lenS, NULL);
	BIGNUM * p_v = BN_bin2bn(pBytesV, lenV, NULL);
	BIGNUM * p_A = BN_bin2bn(pBytesA, lenA, NULL);
	BIGNUM * p_u = 0;
	BIGNUM * p_B = BN_new();
	BIGNUM * p_S = BN_new();
	BIGNUM * p_b = BN_new();
	BIGNUM * p_k = 0;
	BIGNUM * tmp1 = BN_new();
	BIGNUM * tmp2 = BN_new();
	BN_CTX * ctx  = BN_CTX_new();
	int ulen = strlen(pUserName) + 1;
	NGConstant * ng = new_ng(ngType, pNHex, pGHex);
	//SrpVerifier * ver = 0;
	*pLenB   = 0;
	*ppBytesB = 0;
	if(!p_s || !p_v || !p_A || !p_B || !p_S || !p_b || !tmp1 || !tmp2 || !ctx || !ng)
		goto cleanup_and_exit;
	//ver = (SrpVerifier *)SAlloc::M(sizeof(SrpVerifier));
	init_random(); /* Only happens once */
	P_UserName = (char*)SAlloc::M(ulen);
	HashAlg = alg;
	P_Ng = ng;
	if(!P_UserName) {
		goto cleanup_and_exit;
	}
	memcpy((char*)P_UserName, pUserName, ulen);
	Authenticated = 0;
	// SRP-6a safety check 
	BN_mod(tmp1, p_A, ng->N, ctx);
	if(!BN_is_zero(tmp1)) {
		BN_rand(p_b, 256, -1, 0);
		p_k = H_nn(alg, ng->N, ng->g);
		// B = kv + g^b 
		BN_mul(tmp1, p_k, p_v, ctx);
		BN_mod_exp(tmp2, ng->g, p_b, ng->N, ctx);
		BN_mod_add(p_B, tmp1, tmp2, ng->N, ctx);
		p_u = H_nn(alg, p_A, p_B);
		// S = (A *(v^u)) ^ b 
		BN_mod_exp(tmp1, p_v, p_u, ng->N, ctx);
		BN_mul(tmp2, p_A, tmp1, ctx);
		BN_mod_exp(p_S, tmp2, p_b, ng->N, ctx);
		hash_num(alg, p_S, SessionKey);
		calculate_M(alg, ng, M, pUserName, p_s, p_A, p_B, SessionKey);
		calculate_H_AMK(alg, H_AMK, p_A, M, SessionKey);
		*pLenB   = BN_num_bytes(p_B);
		*ppBytesB = (const uchar *)SAlloc::M(*pLenB);
		if(!*ppBytesB) {
			SAlloc::F((void*)P_UserName);
			*pLenB = 0;
			goto cleanup_and_exit;
		}
		BN_bn2bin(p_B, (uchar *)*ppBytesB);
		P_BytesB = *ppBytesB;
	}
cleanup_and_exit:
	BN_free(p_s);
	BN_free(p_v);
	BN_free(p_A);
	BN_free(p_u);
	BN_free(p_k);
	BN_free(p_B);
	BN_free(p_S);
	BN_free(p_b);
	BN_free(tmp1);
	BN_free(tmp2);
	BN_CTX_free(ctx);
	//return ver;
}
#endif // } 0

//void srp_verifier_delete(SrpVerifier * ver)
SlSRP::Verifier::~Verifier()
{
	//delete_ng(P_Ng);
	//SAlloc::F((char*)P_UserName);
	//SAlloc::F((uchar *)P_BytesB);
	THISZERO();
}

//int SlSRP::Verifier::IsAuthenticated() const { return Authenticated; }

const char * SlSRP::Verifier::GetName() const
{
	return UserName.cptr();
}

const uchar * SlSRP::Verifier::GetSessionKey(int * pKeyLength) const
{
	ASSIGN_PTR(pKeyLength, hash_length(HashAlg));
	return SessionKey;
}

int SlSRP::Verifier::GetSessionKeyLength() const
{
	return hash_length(HashAlg);
}
//
// user_M must be exactly SHA512_DIGEST_LENGTH bytes in size 
//
//void srp_verifier_verify_session(SrpVerifier * pVer, const uchar * pUserM, const uchar ** ppBytesHAMK)
void SlSRP::Verifier::VerifySession(const uchar * pUserM, const uchar ** ppBytesHAMK)
{
	if(memcmp(M, pUserM, hash_length(HashAlg)) == 0) {
		Authenticated = 1;
		*ppBytesHAMK = H_AMK;
	}
	else
		*ppBytesHAMK = NULL;
}

/*******************************************************************************/

//SrpUser * srp_user_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char * username, const uchar * bytes_password, int len_password,
//const char * n_hex, const char * g_hex)
SlSRP::User::User(SlSRP::HashAlgorithm alg, NGType ng_type, const char * pUserName, const void * pPassword, int passwordLen, const char * pNHex, const char * pGHex) :
	VerifierBase(alg)
{
	//SrpUser  * usr  = (SrpUser *)SAlloc::M(sizeof(SrpUser));
	int ulen = strlen(pUserName) + 1;
	init_random(); // Only happens once 
	//HashAlg = alg;
	P_Ng = new_ng(ng_type, pNHex, pGHex);
	P_a = BN_new();
	P_A = BN_new();
	P_S = BN_new();
	if(!P_Ng || !P_a || !P_A || !P_S)
		goto err_exit;
	P_UserName = (char *)SAlloc::M(ulen);
	P_Password = (uchar *)SAlloc::M(passwordLen);
	PasswordLen = passwordLen;
	if(!P_UserName || !P_Password)
		goto err_exit;
	memcpy(P_UserName, pUserName, ulen);
	memcpy(P_Password, pPassword, passwordLen);
	//authenticated = 0;
	//P_BytesA = 0;
	return;
err_exit:
	BN_free(static_cast<BIGNUM *>(P_a));
	BN_free(static_cast<BIGNUM *>(P_A));
	BN_free(static_cast<BIGNUM *>(P_S));
	SAlloc::F((void*)P_UserName);
	if(P_Password) {
		memzero(P_Password, PasswordLen);
		SAlloc::F(P_Password);
	}
}

//void srp_user_delete(SrpUser * usr)
SlSRP::User::~User()
{
	BN_free(static_cast<BIGNUM *>(P_a));
	BN_free(static_cast<BIGNUM *>(P_A));
	BN_free(static_cast<BIGNUM *>(P_S));
	//delete_ng(P_Ng);
	memzero(P_Password, PasswordLen);
	SAlloc::F(P_UserName);
	SAlloc::F(P_Password);
	//SAlloc::F(P_BytesA);
	THISZERO();
}

const char * SlSRP::User::GetName() const { return P_UserName; }
int SlSRP::User::GetSessionKeyLength() const { return hash_length(HashAlg); }
//
// Output: username, bytes_A, len_A 
//
//void srp_user_start_authentication(SrpUser * pUsr, const char ** ppUserName, const uchar ** ppBytesA, int * pLenA)
//void SlSRP::User::StartAuthentication(char ** ppUserName, uchar ** ppBytesA, int * pLenA)
void SlSRP::User::StartAuthentication(char ** ppUserName, SBinaryChunk & rA)
{
	BN_CTX  * ctx  = BN_CTX_new();
	//BN_rand(static_cast<BIGNUM *>(P_a), 256, -1, 0);
	// "17e3d491cde2cfeb1922d52ed1790f11a372fe6532f63e9e91fb5587de80c353" // @debug
#ifdef SRP_NETWORK_DEBUG
	{
		BIGNUM * p__a = static_cast<BIGNUM *>(P_a);
		BN_hex2bn(&p__a, "17e3d491cde2cfeb1922d52ed1790f11a372fe6532f63e9e91fb5587de80c353");
	}
#else
	BN_rand(static_cast<BIGNUM *>(P_a), 256, -1, 0);
#endif // SRP_NETWORK_DEBUG
	BN_mod_exp(static_cast<BIGNUM *>(P_A), P_Ng->g, static_cast<BIGNUM *>(P_a), P_Ng->N, ctx);
	BN_CTX_free(ctx);
	if(rA.Ensure(BN_num_bytes(static_cast<BIGNUM *>(P_A)))) {
		BN_bn2bin(static_cast<BIGNUM *>(P_A), static_cast<uchar *>(rA.Ptr()));
		BytesA = rA;
		//P_BytesA = *ppBytesA;
		ASSIGN_PTR(ppUserName, P_UserName);
	}
	else {
		rA.Z();
		ASSIGN_PTR(ppUserName, 0);
	}
	//*pLenA   = BN_num_bytes(static_cast<BIGNUM *>(P_A));
	//*ppBytesA = (uchar *)SAlloc::M(*pLenA);
	/*if(!*ppBytesA) {
		*pLenA = 0;
		*ppBytesA = 0;
		*ppUserName = 0;
	}
	else {
		BN_bn2bin(static_cast<BIGNUM *>(P_A), (uchar *)*ppBytesA);
		P_BytesA = *ppBytesA;
		*ppUserName = P_UserName;
	}*/
}
//
// Output: bytes_M. Buffer length is SHA512_DIGEST_LENGTH 
//
void SlSRP::User::ProcessChallenge(const SBinaryChunk & rS, const SBinaryChunk & rB, SBinaryChunk & rM)
{
	BIGNUM * s = BN_bin2bn(static_cast<const uchar *>(rS.PtrC()), rS.Len(), NULL);
	BIGNUM * B = BN_bin2bn(static_cast<const uchar *>(rB.PtrC()), rB.Len(), NULL);
	BIGNUM * u = 0;
	BIGNUM * x = 0;
	BIGNUM * k = 0;
	BIGNUM * v = BN_new();
	BIGNUM * tmp1 = BN_new();
	BIGNUM * tmp2 = BN_new();
	BIGNUM * tmp3 = BN_new();
	BN_CTX * ctx  = BN_CTX_new();
	//*pLenM = 0;
	//*ppBytesM = 0;
	rM.Z();
	THROW(s && B && v && tmp1 && tmp2 && tmp3 && ctx);
	u = H_nn(HashAlg, static_cast<BIGNUM *>(P_A), B);
	THROW(u);
	x = calculate_x(HashAlg, s, P_UserName, P_Password, PasswordLen);
	THROW(x);
	k = H_nn(HashAlg, P_Ng->N, P_Ng->g);
	THROW(k);
	// SRP-6a safety check 
	if(!BN_is_zero(B) && !BN_is_zero(u) ) {
		BN_mod_exp(v, P_Ng->g, x, P_Ng->N, ctx);
		// S = (B - k*(g^x)) ^ (a + ux) 
		BN_mul(tmp1, u, x, ctx);
		BN_add(tmp2, static_cast<BIGNUM *>(P_a), tmp1); // tmp2 = (a + ux)
		BN_mod_exp(tmp1, P_Ng->g, x, P_Ng->N, ctx);
		BN_mul(tmp3, k, tmp1, ctx); // tmp3 = k*(g^x)    
		BN_sub(tmp1, B, tmp3); // tmp1 = (B - K*(g^x)) 
		BN_mod_exp(static_cast<BIGNUM *>(P_S), tmp1, tmp2, P_Ng->N, ctx);
		hash_num(HashAlg, static_cast<BIGNUM *>(P_S), SessionKey);
		calculate_M(HashAlg, P_Ng, M, P_UserName, s, static_cast<BIGNUM *>(P_A), B, SessionKey);
		calculate_H_AMK(HashAlg, H_AMK, static_cast<BIGNUM *>(P_A), M, SessionKey);
		rM.Put(M, hash_length(HashAlg));
		//*ppBytesM = M;
		//ASSIGN_PTR(pLenM, hash_length(HashAlg));
	}
	else {
		rM.Z();
		//*ppBytesM = NULL;
		//ASSIGN_PTR(pLenM, 0);
	}
//cleanup_and_exit:
	CATCH
		;
	ENDCATCH
	BN_free(s);
	BN_free(B);
	BN_free(u);
	BN_free(x);
	BN_free(k);
	BN_free(v);
	BN_free(tmp1);
	BN_free(tmp2);
	BN_free(tmp3);
	BN_CTX_free(ctx);
}

#if 0 // {
//void srp_user_process_challenge(SrpUser * pUsr, const uchar * pBytesS, int lenS, const uchar * pBytesB, int lenB, const uchar ** ppBytesM, int * pLenM)
void SlSRP::User::ProcessChallenge(const uchar * pBytesS, int lenS, const uchar * pBytesB, int lenB, const uchar ** ppBytesM, int * pLenM)
{
	BIGNUM * s = BN_bin2bn(pBytesS, lenS, NULL);
	BIGNUM * B = BN_bin2bn(pBytesB, lenB, NULL);
	BIGNUM * u = 0;
	BIGNUM * x = 0;
	BIGNUM * k = 0;
	BIGNUM * v = BN_new();
	BIGNUM * tmp1 = BN_new();
	BIGNUM * tmp2 = BN_new();
	BIGNUM * tmp3 = BN_new();
	BN_CTX * ctx  = BN_CTX_new();
	*pLenM = 0;
	*ppBytesM = 0;
	if(!s || !B || !v || !tmp1 || !tmp2 || !tmp3 || !ctx)
		goto cleanup_and_exit;
	u = H_nn(HashAlg, static_cast<BIGNUM *>(P_A), B);
	if(!u)
		goto cleanup_and_exit;
	x = calculate_x(HashAlg, s, P_UserName, P_Password, PasswordLen);
	if(!x)
		goto cleanup_and_exit;
	k = H_nn(HashAlg, P_ng->N, P_ng->g);
	if(!k)
		goto cleanup_and_exit;
	// SRP-6a safety check 
	if(!BN_is_zero(B) && !BN_is_zero(u) ) {
		BN_mod_exp(v, P_ng->g, x, P_ng->N, ctx);
		// S = (B - k*(g^x)) ^ (a + ux) 
		BN_mul(tmp1, u, x, ctx);
		BN_add(tmp2, static_cast<BIGNUM *>(P_a), tmp1); // tmp2 = (a + ux)
		BN_mod_exp(tmp1, P_ng->g, x, P_ng->N, ctx);
		BN_mul(tmp3, k, tmp1, ctx); // tmp3 = k*(g^x)    
		BN_sub(tmp1, B, tmp3); // tmp1 = (B - K*(g^x)) 
		BN_mod_exp(static_cast<BIGNUM *>(P_S), tmp1, tmp2, P_ng->N, ctx);
		hash_num(HashAlg, static_cast<BIGNUM *>(P_S), SessionKey);
		calculate_M(HashAlg, P_ng, M, P_UserName, s, static_cast<BIGNUM *>(P_A), B, SessionKey);
		calculate_H_AMK(HashAlg, H_AMK, static_cast<BIGNUM *>(P_A), M, SessionKey);
		*ppBytesM = M;
		ASSIGN_PTR(pLenM, hash_length(HashAlg));
	}
	else {
		*ppBytesM = NULL;
		ASSIGN_PTR(pLenM, 0);
	}
cleanup_and_exit:
	BN_free(s);
	BN_free(B);
	BN_free(u);
	BN_free(x);
	BN_free(k);
	BN_free(v);
	BN_free(tmp1);
	BN_free(tmp2);
	BN_free(tmp3);
	BN_CTX_free(ctx);
}
#endif // } 0
//
// TEST
//
#define NITER          100
#define TEST_HASH      SlSRP::SRP_SHA1
#define TEST_NG        SRP_NG_1024

uint64 get_usec()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (((uint64)t.tv_sec) * 1000000) + t.tv_usec;
}

const char * test_n_hex = "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496"
    "EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8E"
    "F4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA"
    "9AFD5138FE8376435B9FC61D2FC0EB06E3";
const char * test_g_hex = "2";

int SrpTest()
{
	int    ok = 1;
	//uchar * bytes_s = 0;
	//uchar * bytes_v = 0;
	//const uchar * bytes_B = 0;
	//const uchar * bytes_M    = 0;
	const uchar * bytes_HAMK = 0;
	SBinaryChunk __s; // S
	SBinaryChunk __v; // V
	SBinaryChunk __b; // B
	SBinaryChunk __m; // M
	SBinaryChunk __a; // A
	//int len_s   = 0;
	//int len_v   = 0;
	//int len_B   = 0;
	//int len_M   = 0;
	uint64 duration;
	const char * username = "testuser";
	const char * password = "password";
	const char * n_hex = 0;
	const char * g_hex = 0;
	SlSRP::HashAlgorithm alg = TEST_HASH;
	SlSRP::NGType ng_type = SlSRP::SRP_NG_8192;    //TEST_NG;
	if(ng_type == SlSRP::SRP_NG_CUSTOM) {
		n_hex = test_n_hex;
		g_hex = test_g_hex;
	}
	const int pw_len = sstrleni(password);
	//SlSRP::CreateSaltedVerificationKey(alg, ng_type, username, (const uchar *)password, pw_len, &bytes_s, &len_s, &bytes_v, &len_v, n_hex, g_hex);
	SlSRP::CreateSaltedVerificationKey2(alg, ng_type, username, (const uchar *)password, pw_len, __s, __v, n_hex, g_hex);
	uint64 start = get_usec();
	for(uint i = 0; i < NITER; i++) {
		char * p_auth_username = 0;
		SlSRP::User usr(alg, ng_type, username, (const uchar *)password, pw_len, n_hex, g_hex);
		usr.StartAuthentication(&p_auth_username, __a);
		// User -> Host: (username, bytes_A) 
		SlSRP::Verifier ver(alg, ng_type, username, __s, __v, __a, __b, n_hex, g_hex);
		if(!__b.Len()) {
			printf("Verifier SRP-6a safety check violated!\n");
			ok = 0;
		}
		else {
			// Host -> User: (bytes_s, bytes_B) 
			usr.ProcessChallenge(__s, __b, __m);
			if(!__m.Len()) {
				printf("User SRP-6a safety check violation!\n");
				ok = 0;
			}
			else {
				// User -> Host: (bytes_M) 
				ver.VerifySession(static_cast<const uchar *>(__m.PtrC()), &bytes_HAMK);
				if(!bytes_HAMK) {
					printf("User authentication failed!\n");
					ok = 0;
				}
				else {
					// Host -> User: (HAMK) 
					usr.VerifySession(bytes_HAMK);
					if(!usr.IsAuthenticated()) {
						printf("Server authentication failed!\n");
						ok = 0;
					}
				}
			}
		}
	}
	duration = get_usec() - start;
	printf("Usec per call: %d\n", (int)(duration / NITER));
	//SAlloc::F(bytes_s);
	//SAlloc::F(bytes_v);
	return 0;
}
