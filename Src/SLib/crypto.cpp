// CRYPTO.CPP
// Copyright (c) A.Sobolev 1996, 2003, 2010, 2016, 2019, 2020, 2021
// @threadsafe
//
#include <slib-internal.h>
#pragma hdrstop
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/pem.h>

static int getkey(int key[], ulong * addendum)
{
	key[0] = 5;
	key[1] = 4;
	key[2] = 19;
	key[3] = 2;
	key[4] = 171;
	key[5] = 1;
	key[6] = 88;
	*addendum = 0x11230244UL;
	return 7;
}

static ulong mix(ulong v)
{
	int i;
	int key[16];
	ulong addendum;
	int c = getkey(key, &addendum);
	v += addendum;
	for(i = 0; i < c; i++)
		if(key[i] % 3)
			v = _lrotr(v, key[i]);
		else
			v = ~_lrotl(v, key[i]);
	return v;
}

static ulong unmix(ulong v)
{
	int i;
	int key[16];
	ulong addendum;
	int c = getkey(key, &addendum);
	for(i = c-1; i >= 0; i--)
		if(key[i] % 3)
			v = _lrotl(v, key[i]);
		else
			v = _lrotr(~v, key[i]);
	v -= addendum;
	return v;
}

void * encrypt(void * pBuf, size_t len)
{
	len = ((len + 3) >> 2);
	for(size_t i = 0; i < len; i++)
		((ulong *)pBuf)[i] = mix(((ulong *)pBuf)[i]);
	return pBuf;
}

void * decrypt(void * pBuf, size_t len)
{
	len >>= 2;
	for(size_t i = 0; i < len; i++)
		static_cast<ulong *>(pBuf)[i] = unmix(static_cast<const ulong *>(pBuf)[i]);
	return pBuf;
}

ulong _checksum__(const char * buf, size_t len)
{
	ulong  r = 0xc22cc22cUL;
	size_t i;
	for(i = 0; i < len; i++)
		reinterpret_cast<uchar *>(&r)[i % 4] += (uchar)((uint)buf[i] ^ (uint)((uchar *)&r)[3 - (i % 4)]);
	return r;
}
//
//
//
SSecretTagPool::SSecretTagPool()
{
}

int SSecretTagPool::GeneratePrivateKey(uint bitCount)
{
	int    ok = 1;
	//
	STempBuffer temp_buf(4096);
	size_t priv_key_len = 0;
	{
		EVP_PKEY * p_pkey = 0;
		EVP_PKEY_CTX * p_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, 0);
		EVP_PKEY_keygen_init(p_ctx);
		EVP_PKEY_keygen(p_ctx, &p_pkey);
		//
		EVP_PKEY_get_raw_private_key(p_pkey, static_cast<uchar *>(temp_buf.vptr()), &priv_key_len);
	}
	{
		BIGNUM * bn = BN_new();
		BN_set_word(bn, RSA_F4);
		//
		RSA * rsa = RSA_new();
		{
			uint8 seed_buf[1024];
			RAND_seed(seed_buf, sizeof(seed_buf));
		}
		RSA_generate_key_ex(rsa/* pointer to the RSA structure */, 2048/* number of bits for the key - 2048 is a good value */, 
			bn/* exponent allocated earlier */, NULL/* callback - can be NULL if progress isn't needed */);
		//
		/*{
			EVP_PKEY * pkey = EVP_PKEY_new();
			EVP_PKEY_assign_RSA(pkey, rsa);
			size_t raw_key_size = 0;
			if(EVP_PKEY_get_raw_private_key(pkey, static_cast<uchar *>(temp_buf.vptr()), &raw_key_size)) {
			}
			EVP_PKEY_free(pkey);
		}*/
		{
			BIO * p_priv_key_buf = BIO_new(BIO_s_mem());
			PEM_write_bio_RSAPrivateKey(p_priv_key_buf, rsa, 0, 0, 0, 0, 0);
			char * p_priv_key_data = 0;
			long   priv_key_size = BIO_get_mem_data(p_priv_key_buf, &p_priv_key_data);
			BIO_free_all(p_priv_key_buf);
		}
		//
		RSA_free(rsa);
		BN_free(bn);
	}
	return ok;
}

SlCrypto::CipherProperties::CipherProperties() : BlockSize(0), KeySize(0), IvSize(0), AadSize(0)
{
}

SlCrypto::Key::Key() : EndP(0)
{
	KEY.Init();
	IV.Init();
	AAD.Init();
}

SlCrypto::Key & SlCrypto::Key::Z()
{
	KEY.Init();
	IV.Init();
	AAD.Init();
	EndP = 0;
	return *this;
}

const SBaseBuffer & SlCrypto::Key::GetKey() const { return KEY; }
const SBaseBuffer & SlCrypto::Key::GetIV() const { return IV; }
const SBaseBuffer & SlCrypto::Key::GetAAD() const { return AAD; }

int SlCrypto::Key::SetKey(const void * pData, size_t size)
{
	int    ok = 1;
	if((EndP + size) > sizeof(Bin)) {
		ok = SLS.SetError(SLERR_CRYPTO_KEYBUFOVRFLW); // SLERR_CRYPTO_KEYBUFOVRFLW  Ошибка SLCRYPTO - переполнение внутреннего буфера ключа
	}
	else {
		memcpy(Bin+EndP, pData, size);
		KEY.P_Buf = reinterpret_cast<char *>(Bin+EndP);
		KEY.Size = size;
		EndP += size;
	}
	return ok;
}
		
int SlCrypto::Key::SetIV(const void * pData, size_t size)
{
	int    ok = 1;
	if((EndP + size) > sizeof(Bin)) {
		ok = SLS.SetError(SLERR_CRYPTO_KEYBUFOVRFLW); // SLERR_CRYPTO_KEYBUFOVRFLW  Ошибка SLCRYPTO - переполнение внутреннего буфера ключа
	}
	else {
		memcpy(Bin+EndP, pData, size);
		IV.P_Buf = reinterpret_cast<char *>(Bin+EndP);
		IV.Size = size;
		EndP += size;
	}
	return ok;
}
		
int SlCrypto::Key::SetAAD(const void * pData, size_t size)
{
	int    ok = 1;
	if((EndP + size) > sizeof(Bin)) {
		ok = SLS.SetError(SLERR_CRYPTO_KEYBUFOVRFLW); // SLERR_CRYPTO_KEYBUFOVRFLW  Ошибка SLCRYPTO - переполнение внутреннего буфера ключа
	}
	else {
		memcpy(Bin+EndP, pData, size);
		AAD.P_Buf = reinterpret_cast<char *>(Bin+EndP);
		AAD.Size = size;
		EndP += size;
	}
	return ok;
}

SlCrypto::SlCrypto(int alg, uint kbl, int algModif) : P_Ctx(0), P_Cphr(0), Alg(alg), KeyBitLen(kbl), AlgModif(algModif), State(0)
{
	const EVP_CIPHER * p_cphr = 0;
	switch(Alg) {
		case algAes:
			switch(KeyBitLen) {
				case kbl128:
					switch(AlgModif) {
						case algmodCbc: p_cphr = EVP_aes_128_cbc(); break;
						case algmodEcb: p_cphr = EVP_aes_128_ecb(); break;
						case algmodOcb: p_cphr = EVP_aes_128_ocb(); break;
						case algmodOfb: p_cphr = EVP_aes_128_ofb(); break;
						case algmodCcm: p_cphr = EVP_aes_128_ccm(); break;
						case algmodGcm: p_cphr = EVP_aes_128_gcm(); break;
						case algmodCtr: p_cphr = EVP_aes_128_ctr(); break;
						case algmodXts: p_cphr = EVP_aes_128_xts(); break;
						case algmodWrap: p_cphr = EVP_aes_128_wrap(); break;
						case algmodWrapPad: p_cphr = EVP_aes_128_wrap_pad(); break;
						case algmodCfb1: p_cphr = EVP_aes_128_cfb1(); break;
						case algmodCfb8: p_cphr = EVP_aes_128_cfb8(); break;
						case algmodCfb128: p_cphr = EVP_aes_128_cfb128(); break;
					}
					break;
				case kbl192:
					switch(AlgModif) {
						case algmodCbc: p_cphr = EVP_aes_192_cbc(); break;
						case algmodEcb: p_cphr = EVP_aes_192_ecb(); break;
						case algmodOcb: p_cphr = EVP_aes_192_ocb(); break;
						case algmodOfb: p_cphr = EVP_aes_192_ofb(); break;
						case algmodCcm: p_cphr = EVP_aes_192_ccm(); break;
						case algmodGcm: p_cphr = EVP_aes_192_gcm(); break;
						case algmodCtr: p_cphr = EVP_aes_192_ctr(); break;
						//case algmodXts: p_cphr = EVP_aes_192_xts(); break;
						case algmodWrap: p_cphr = EVP_aes_192_wrap(); break;
						case algmodWrapPad: p_cphr = EVP_aes_192_wrap_pad(); break;
						case algmodCfb1: p_cphr = EVP_aes_192_cfb1(); break;
						case algmodCfb8: p_cphr = EVP_aes_192_cfb8(); break;
						case algmodCfb128: p_cphr = EVP_aes_192_cfb128(); break;
					}
					break;
				case kbl256:
					switch(AlgModif) {
						case algmodCbc: p_cphr = EVP_aes_256_cbc(); break;
						case algmodEcb: p_cphr = EVP_aes_256_ecb(); break;
						case algmodOcb: p_cphr = EVP_aes_256_ocb(); break;
						case algmodOfb: p_cphr = EVP_aes_256_ofb(); break;
						case algmodCcm: p_cphr = EVP_aes_256_ccm(); break;
						case algmodGcm: p_cphr = EVP_aes_256_gcm(); break;
						case algmodCtr: p_cphr = EVP_aes_256_ctr(); break;
						case algmodXts: p_cphr = EVP_aes_256_xts(); break;
						case algmodWrap: p_cphr = EVP_aes_256_wrap(); break;
						case algmodWrapPad: p_cphr = EVP_aes_256_wrap_pad(); break;
						case algmodCfb1: p_cphr = EVP_aes_256_cfb1(); break;
						case algmodCfb8: p_cphr = EVP_aes_256_cfb8(); break;
						case algmodCfb128: p_cphr = EVP_aes_256_cfb128(); break;
					}
					break;
				default:
					break;
			}
			break;
		case algDes:
			switch(AlgModif) {
				case algmodCbc: p_cphr = EVP_des_cbc(); break;
				case algmodEcb: p_cphr = EVP_des_ecb(); break;
				case algmodOfb: p_cphr = EVP_des_ofb(); break;
				case algmodCfb1: p_cphr = EVP_des_cfb1(); break;
				case algmodCfb8: p_cphr = EVP_des_cfb8(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_des_cfb64(); break;
			}
			break;
		case algDes_Ede:
			switch(AlgModif) {
				case algmodCbc: p_cphr = EVP_des_ede_cbc(); break;
				case algmodEcb: p_cphr = EVP_des_ede_ecb(); break;
				case algmodOfb: p_cphr = EVP_des_ede_ofb(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_des_ede_cfb64(); break;
				default: p_cphr = EVP_des_ede(); break;
			}
			break;
		case algDes_Ede3:
			switch(AlgModif) {
				case algmodCbc: p_cphr = EVP_des_ede3_cbc(); break;
				case algmodEcb: p_cphr = EVP_des_ede3_ecb(); break;
				case algmodOfb: p_cphr = EVP_des_ede3_ofb(); break;
				case algmodWrap: p_cphr = EVP_des_ede3_wrap(); break;
				case algmodCfb1: p_cphr = EVP_des_ede3_cfb1(); break;
				case algmodCfb8: p_cphr = EVP_des_ede3_cfb8(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_des_ede3_cfb64(); break;
				default: p_cphr = EVP_des_ede3(); break;
			}
			break;
		case algDesx:
			switch(AlgModif) {
				case algmodCbc: p_cphr = EVP_desx_cbc(); break;
				default: p_cphr = EVP_desx_cbc(); break;
			}
			break;
		case algIdea:
			switch(AlgModif) {
				case algmodEcb: p_cphr = EVP_idea_ecb(); break;
				case algmodCbc: p_cphr = EVP_idea_cbc(); break;
				case algmodOfb: p_cphr = EVP_idea_ofb(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_idea_cfb64(); break;
			}
			break;
		case algCamellia:
			switch(KeyBitLen) {
				case kbl128:
					switch(AlgModif) {
						case algmodEcb: p_cphr = EVP_camellia_128_ecb(); break;
						case algmodCbc: p_cphr = EVP_camellia_128_cbc(); break;
						case algmodOfb: p_cphr = EVP_camellia_128_ofb(); break;
						case algmodCtr: p_cphr = EVP_camellia_128_ctr(); break;
						case algmodCfb1: p_cphr = EVP_camellia_128_cfb1(); break;
						case algmodCfb8: p_cphr = EVP_camellia_128_cfb8(); break;
						case algmodCfb:
						case algmodCfb128: p_cphr = EVP_camellia_128_cfb128(); break;
					}
					break;
				case kbl192:
					switch(AlgModif) {
						case algmodEcb: p_cphr = EVP_camellia_192_ecb(); break;
						case algmodCbc: p_cphr = EVP_camellia_192_cbc(); break;
						case algmodOfb: p_cphr = EVP_camellia_192_ofb(); break;
						case algmodCtr: p_cphr = EVP_camellia_192_ctr(); break;
						case algmodCfb1: p_cphr = EVP_camellia_192_cfb1(); break;
						case algmodCfb8: p_cphr = EVP_camellia_192_cfb8(); break;
						case algmodCfb:
						case algmodCfb128: p_cphr = EVP_camellia_192_cfb128(); break;
					}
					break;
				case kbl256:
					switch(AlgModif) {
						case algmodEcb: p_cphr = EVP_camellia_256_ecb(); break;
						case algmodCbc: p_cphr = EVP_camellia_256_cbc(); break;
						case algmodOfb: p_cphr = EVP_camellia_256_ofb(); break;
						case algmodCtr: p_cphr = EVP_camellia_256_ctr(); break;
						case algmodCfb1: p_cphr = EVP_camellia_256_cfb1(); break;
						case algmodCfb8: p_cphr = EVP_camellia_256_cfb8(); break;
						case algmodCfb:
						case algmodCfb128: p_cphr = EVP_camellia_256_cfb128(); break;
					}
					break;
			}
			break;
		case algBlowfish:
			switch(AlgModif) {
				case algmodEcb: p_cphr = EVP_bf_ecb(); break;
				case algmodCbc: p_cphr = EVP_bf_cbc(); break;
				case algmodOfb: p_cphr = EVP_bf_ofb(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_bf_cfb64(); break;
			}
			break;
		case algChaCha20:
			switch(AlgModif) {
				case algmodPoly1305: p_cphr = EVP_chacha20_poly1305(); break;
				default: p_cphr = EVP_chacha20(); break;
			}
			break;
	}
	if(p_cphr) {
		P_Cphr = p_cphr;
		P_Ctx = EVP_CIPHER_CTX_new();
		Cp.BlockSize = static_cast<uint>(EVP_CIPHER_block_size(p_cphr));
		Cp.KeySize = static_cast<uint>(EVP_CIPHER_key_length(p_cphr));
		Cp.IvSize = static_cast<uint>(EVP_CIPHER_iv_length(p_cphr));
		if(EVP_EncryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), p_cphr, NULL, NULL, NULL)) { // Set cipher type and mode 
			Cp.BlockSize = static_cast<uint>(EVP_CIPHER_CTX_block_size(static_cast<EVP_CIPHER_CTX *>(P_Ctx)));
			Cp.KeySize = static_cast<uint>(EVP_CIPHER_CTX_key_length(static_cast<EVP_CIPHER_CTX *>(P_Ctx)));
			Cp.IvSize = static_cast<uint>(EVP_CIPHER_CTX_iv_length(static_cast<EVP_CIPHER_CTX *>(P_Ctx)));
		}
		else {
			EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX *>(P_Ctx));
			P_Ctx = 0;
			State |= stError;
		}
	}
	else {
		State |= stError;
	}
}
	
SlCrypto::~SlCrypto()
{
	if(P_Ctx) {
		EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX *>(P_Ctx));
		P_Ctx = 0;
	}
}
	
const  SlCrypto::CipherProperties & SlCrypto::GetCipherProperties() const { return Cp; }

int SlCrypto::SetupKey(SlCrypto::Key & rK, const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen, const void * pAad, size_t aadLen)
{
	rK.Z();
	int    ok = 1;
	assert(P_Ctx && P_Cphr);
	THROW_S(P_Ctx && P_Cphr, SLERR_CRYPTO_INVPARAM); // SLERR_CRYPTO_INVPARAM Ошибка SLCRYPTO - недопустимый параметр функции
	if(pKey) {
		THROW_S(keyByteLen == Cp.KeySize, SLERR_CRYPTO_INVKEYSIZE); // SLERR_CRYPTO_INVKEYSIZE Ошибка SLCRYPTO - недопустимая длина ключа шифрования
		THROW(rK.SetKey(pKey, keyByteLen));
	}
	if(pIv) {
		THROW_S(ivLen == Cp.IvSize, SLERR_CRYPTO_INVIVSIZE); // SLERR_CRYPTO_INVIVSIZE  Ошибка SLCRYPTO - недопустимая длина IV
		THROW(rK.SetIV(pIv, ivLen));
	}
	if(pAad) {
		THROW(rK.SetAAD(pAad, aadLen));
	}
	CATCHZOK
	return ok;
}

int SlCrypto::SetupKey(SlCrypto::Key & rK, const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen)
	{ return SetupKey(rK, pKey, keyByteLen, pIv, ivLen, 0, 0); }
int SlCrypto::SetupKey(SlCrypto::Key & rK, const void * pKey, size_t keyByteLen)
	{ return SetupKey(rK, pKey, keyByteLen, 0, 0, 0, 0); }

int SlCrypto::SetupKey(SlCrypto::Key & rK, const char * pPassword)
{
	rK.Z();
	int    ok = 1;
	const  uchar * p_salt = 0;
	uchar  key[EVP_MAX_KEY_LENGTH];
	uchar  iv[EVP_MAX_IV_LENGTH];
	int    result_key_len = 0;
	THROW(P_Ctx && P_Cphr);
	result_key_len = EVP_BytesToKey(static_cast<const EVP_CIPHER *>(P_Cphr), EVP_md5(), p_salt, 
		reinterpret_cast<const uchar *>(pPassword), sstrlen(pPassword), 1/*count*/, key, iv);
	THROW(result_key_len);
	assert(result_key_len == static_cast<int>(Cp.KeySize));
	if(Cp.KeySize) {
		rK.SetKey(key, result_key_len);
	}
	if(Cp.IvSize) {
		rK.SetIV(iv, Cp.IvSize);
	}
	CATCHZOK
	return ok;
}

#if 0 // {
	int    SlCrypto::SetupEncrypt(const char * pPassword)
	{
		int    ok = 1;
		const  EVP_MD * p_dgst = 0;
		const  uchar * p_salt = 0;
		uchar  key[EVP_MAX_KEY_LENGTH];
		uchar  iv[EVP_MAX_IV_LENGTH];
		int    result_key_len = 0;
		THROW(P_Ctx && P_Cphr);
		p_dgst = EVP_md5();
		result_key_len = EVP_BytesToKey(static_cast<const EVP_CIPHER *>(P_Cphr), p_dgst, p_salt, 
			reinterpret_cast<const uchar *>(pPassword), sstrlen(pPassword), 1/*count*/, key, iv);
		THROW(result_key_len);
		assert(result_key_len == static_cast<int>(Cp.KeySize));
		THROW(EVP_EncryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<const EVP_CIPHER *>(P_Cphr), 0, key, iv)); // Initialise key and IV 
		State &= ~stInitDecr;
		State |= stInitEncr;
		CATCHZOK
		return ok;
	}
	
	int    SlCrypto::SetupEncrypt(const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen, const void * pAad, size_t aadLen)
	{
		int    ok = 1;
		THROW(P_Ctx && P_Cphr);
		THROW(EVP_EncryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<const EVP_CIPHER *>(P_Cphr), NULL, NULL, NULL)); // Set cipher type and mode 
		THROW(!Cp.KeySize || !keyByteLen || keyByteLen == Cp.KeySize);
		THROW(!ivLen || Cp.IvSize == ivLen);
		THROW(EVP_EncryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), 0, 0, static_cast<const uint8 *>(pKey), static_cast<const uint8 *>(pIv))); // Initialise key and IV 
		State &= ~stInitDecr;
		State |= stInitEncr;
		CATCHZOK
		return ok;
	}
	
	int    SlCrypto::SetupDecrypt(const char * pPassword)
	{
		int    ok = 1;
		const  EVP_MD * p_dgst = 0;
		const  uchar * p_salt = 0;
		uchar  key[EVP_MAX_KEY_LENGTH];
		uchar  iv[EVP_MAX_IV_LENGTH];
		int    result_key_len = 0;
		THROW(P_Ctx && P_Cphr);
		p_dgst = EVP_md5();
		result_key_len = EVP_BytesToKey(static_cast<const EVP_CIPHER *>(P_Cphr), p_dgst, p_salt, 
			reinterpret_cast<const uchar *>(pPassword), sstrlen(pPassword), 1/*count*/, key, iv);
		THROW(result_key_len);
		assert(result_key_len == static_cast<int>(Cp.KeySize));
		THROW(EVP_DecryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<const EVP_CIPHER *>(P_Cphr), 0, key, iv)); // Initialise key and IV 
		State |= stInitDecr;
		State &= ~stInitEncr;
		CATCHZOK
		return ok;
	}
	
	int SlCrypto::SetupDecrypt(const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen, const void * pAad, size_t aadLen)
	{
		int    ok = 1;
		THROW(P_Ctx && P_Cphr);
		THROW(EVP_DecryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<const EVP_CIPHER *>(P_Cphr), NULL, NULL, NULL)); // Set cipher type and mode 
		THROW(!Cp.KeySize || !keyByteLen || keyByteLen == Cp.KeySize);
		THROW(Cp.IvSize == ivLen);
		THROW(EVP_DecryptInit_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), 0, 0, static_cast<const uint8 *>(pKey), static_cast<const uint8 *>(pIv))); // Initialise key and IV 
		State |= stInitDecr;
		State &= ~stInitEncr;
		CATCHZOK
		return ok;
	}

	int SlCrypto::Encrypt(const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen)
	{
		int    ok = 1;
		int    outl = 0;
		THROW(P_Ctx && P_Cphr);
		THROW(State & stInitEncr);
		if(pData && dataLen) {
			THROW(EVP_EncryptUpdate(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<uchar *>(pResult), &outl, static_cast<const uchar *>(pData), static_cast<int>(dataLen)));
		}
		else {
			THROW(EVP_EncryptFinal_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<uchar *>(pResult), &outl));
		}
		CATCHZOK
		ASSIGN_PTR(pActualResultLen, outl);
		return ok;
	}
	
	int SlCrypto::Decrypt(const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen)
	{
		int    ok = 1;
		int    outl = 0;
		THROW(P_Ctx && P_Cphr);
		THROW(State & stInitDecr);
		if(pData && dataLen) {
			THROW(EVP_DecryptUpdate(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<uchar *>(pResult), &outl, static_cast<const uchar *>(pData), static_cast<int>(dataLen)));
		}
		else {
			THROW(EVP_DecryptFinal_ex(static_cast<EVP_CIPHER_CTX *>(P_Ctx), static_cast<uchar *>(pResult), &outl));
		}
		CATCHZOK
		ASSIGN_PTR(pActualResultLen, outl);
		return ok;
	}
#endif // } 0

int SlCrypto::Encrypt_(const SlCrypto::Key * pKey, const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen)
{
	assert(pKey);
	int    ok = 1;
	int    outl = 0;
	if(pKey) {
		int    final_outl = 0;
		EVP_CIPHER_CTX * p_ctx = static_cast<EVP_CIPHER_CTX *>(P_Ctx);
		const SBaseBuffer & r_key = pKey->GetKey();
		const SBaseBuffer & r_iv = pKey->GetIV();
		const SBaseBuffer & r_aad = pKey->GetAAD();
		THROW_S(p_ctx && P_Cphr, SLERR_CRYPTO_INVPARAM);
		//THROW(EVP_EncryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), NULL, NULL, NULL)); // Set cipher type and mode 
		THROW_S(!Cp.KeySize || !r_key.Size || r_key.Size == Cp.KeySize, SLERR_CRYPTO_INVKEYSIZE);
		THROW_S(!r_iv.Size || r_iv.Size == Cp.IvSize, SLERR_CRYPTO_INVIVSIZE);
		THROW_S(EVP_EncryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), 0, 
			reinterpret_cast<const uint8 *>(r_key.P_Buf), reinterpret_cast<const uint8 *>(r_iv.P_Buf)), SLERR_CRYPTO_OPENSSL); // Initialise key and IV 
		State &= ~stInitDecr;
		State |= stInitEncr;
		if(pResult) {
			//THROW(State & stInitEncr);
			if(pData && dataLen) {
				if(r_aad.Size) {
					THROW_S(EVP_EncryptUpdate(p_ctx, 0, &outl, reinterpret_cast<const uchar *>(r_aad.P_Buf), static_cast<int>(r_aad.Size)), SLERR_CRYPTO_OPENSSL);
				}
				THROW_S(EVP_EncryptUpdate(p_ctx, static_cast<uchar *>(pResult), &outl, static_cast<const uchar *>(pData), static_cast<int>(dataLen)), SLERR_CRYPTO_OPENSSL);
				THROW_S(EVP_EncryptFinal_ex(p_ctx, static_cast<uchar *>(pResult)+outl, &final_outl), SLERR_CRYPTO_OPENSSL);
				outl += final_outl;
			}
			else {
				//THROW_S(EVP_EncryptFinal_ex(p_ctx, static_cast<uchar *>(pResult), &outl), SLERR_CRYPTO_OPENSSL);
			}
		}
	}
	else
		ok = 0;
	CATCHZOK
	ASSIGN_PTR(pActualResultLen, outl);
	return ok;
}

int SlCrypto::Decrypt_(const SlCrypto::Key * pKey, const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen)
{
	/*
		ctx     = EVP_CIPHER_CTX_new();
		//Get the cipher.
		cipher  = EVP_aes_128_gcm ();
		#define     GCM_IV      "000000000000"
		#define     GCM_ADD     "0000"
		#define     TAG_SIZE    16
		#define     ENC_SIZE    64
		//Encrypt the data first.
		//Set the cipher and context only.
		retv    = EVP_EncryptInit (ctx, cipher, NULL, NULL);

		//Set the nonce and tag sizes.
		//Set IV length. [Optional for GCM].
		retv    = EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_SET_IVLEN, strlen((const char *)GCM_IV), NULL);
		retv    = EVP_EncryptInit (ctx, NULL, (const unsigned char *)keybuf, (const unsigned char *)GCM_IV); //Now initialize the context with key and IV. 
		retv    = EVP_EncryptUpdate(ctx, NULL, (int *)&enclen, (const unsigned char *)GCM_ADD, strlen(GCM_ADD)); //Add Additional associated data (AAD). [Optional for GCM]
		retv    = EVP_EncryptUpdate(ctx, (unsigned char *)encm, (int *)&enclen, (const unsigned char *)msg, _tcslen (msg) *sizeof(Char)); //Now encrypt the data.
		retv    = EVP_EncryptFinal(ctx, (unsigned char *)encm + enclen, (int *)&enclen2); //Finalize.
		enclen  += enclen2;
		retv    = EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, (unsigned char *)encm + enclen); //Append authentication tag at the end.
		//DECRYPTION PART
		//Now Decryption of the data.
		//Then decrypt the data.
		retv    = EVP_DecryptInit(ctx, cipher, NULL, NULL); //Set just cipher.
		//
		retv    = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, strlen((const char *)GCM_IV), NULL); //Set Nonce size.
		retv    = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, (unsigned char *)encm + enclen); //Set Tag from the data.
		retv    = EVP_DecryptInit (ctx, NULL, (const unsigned char*)keybuf, (const unsigned char *)GCM_IV); //Set key and IV (nonce).
		retv    = EVP_DecryptUpdate (ctx, NULL, (int *)&declen, (const unsigned char *)GCM_ADD, strlen((const char *)GCM_ADD)); //Add Additional associated data (AAD).
		retv    = EVP_DecryptUpdate (ctx, decm, (int *)&declen, (const unsigned char *)encm, enclen); //Decrypt the data.
		retv    = EVP_DecryptFinal (ctx, (unsigned char*)decm + declen, (int *)&declen2); //Finalize.
	*/
	assert(pKey);
	int    ok = 1;
	int    outl = 0;
	EVP_CIPHER_CTX * p_ctx = static_cast<EVP_CIPHER_CTX *>(P_Ctx);
	THROW(p_ctx && P_Cphr);
	if(pKey) {
		int    final_outl = 0;
		const SBaseBuffer & r_key = pKey->GetKey();
		const SBaseBuffer & r_iv = pKey->GetIV();
		const SBaseBuffer & r_aad = pKey->GetAAD();
		//THROW(EVP_DecryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), NULL, NULL, NULL)); // Set cipher type and mode 
		THROW(!r_key.Size || !r_key.Size || r_key.Size == Cp.KeySize);
		THROW(r_iv.Size == Cp.IvSize);
		THROW(EVP_DecryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), 0, 
			reinterpret_cast<const uint8 *>(r_key.P_Buf), reinterpret_cast<const uint8 *>(r_iv.P_Buf))); // Initialise key and IV 
		State |= stInitDecr;
		State &= ~stInitEncr;
		if(pResult) {
			THROW(State & stInitDecr);
			if(pData && dataLen) {
				if(r_aad.Size) {
					THROW(EVP_DecryptUpdate(p_ctx, 0, &outl, reinterpret_cast<const uchar *>(r_aad.P_Buf), static_cast<int>(r_aad.Size)));
				}
				THROW(EVP_DecryptUpdate(p_ctx, static_cast<uchar *>(pResult), &outl, static_cast<const uchar *>(pData), static_cast<int>(dataLen)));
				THROW(EVP_DecryptFinal/*_ex*/(p_ctx, static_cast<uchar *>(pResult)+outl, &final_outl));
				outl += final_outl; // ?
			}
			else {
				//THROW(EVP_DecryptFinal_ex(p_ctx, static_cast<uchar *>(pResult), &outl));
			}
		}
	}
	else
		ok = 0;
	CATCHZOK
	ASSIGN_PTR(pActualResultLen, outl);
	return ok;
}
