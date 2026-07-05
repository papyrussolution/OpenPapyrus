// CRYPTO.CPP
// Copyright (c) A.Sobolev 1996, 2003, 2010, 2016, 2019, 2020, 2021, 2022, 2023, 2026
// @threadsafe
//
#include <slib-internal.h>
#pragma hdrstop
#include <ued.h>
#include <slib-ossl.h>
#include <argon2.h> // @v12.6.2
 
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
	int    i;
	int    key[16];
	ulong  addendum;
	const  int c = getkey(key, &addendum);
	v += addendum;
	for(i = 0; i < c; i++) {
		if(key[i] % 3)
			v = /*_lrotr*/SBits::Rotr(v, key[i]);
		else
			v = ~/*_lrotl*/SBits::Rotl(v, key[i]);
	}
	return v;
}

static ulong unmix(ulong v)
{
	int    i;
	int    key[16];
	ulong  addendum;
	const  int c = getkey(key, &addendum);
	for(i = c-1; i >= 0; i--) {
		if(key[i] % 3)
			v = /*_lrotl*/SBits::Rotl(v, key[i]);
		else
			v = /*_lrotr*/SBits::Rotr(~v, key[i]);
	}
	v -= addendum;
	return v;
}

void * __D2M(void * pBuf, size_t len)
{
	len = ((len + 3) >> 2);
	for(size_t i = 0; i < len; i++) {
		static_cast<ulong *>(pBuf)[i] = mix(static_cast<ulong *>(pBuf)[i]);
	}
	return pBuf;
}

void * __M2D(void * pBuf, size_t len)
{
	len >>= 2;
	for(size_t i = 0; i < len; i++) {
		static_cast<ulong *>(pBuf)[i] = unmix(static_cast<const ulong *>(pBuf)[i]);
	}
	return pBuf;
}

ulong _checksum__(const char * buf, size_t len)
{
	ulong  r = 0xc22cc22cUL;
	for(size_t i = 0; i < len; i++) {
		reinterpret_cast<uchar *>(&r)[i % 4] += (uchar)((uint)buf[i] ^ (uint)((uchar *)&r)[3 - (i % 4)]);
	}
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

SJson * SSecretTagPool::GetJson(uint tag) const
{
	SJson * p_js = 0;
	SBinaryChunk bch;
	if(Get(tag, &bch)) {
		SString temp_buf;
		p_js = SJson::Parse(bch.ToRawStr(temp_buf));
		if(p_js && !p_js->IsValid())
			ZDELETE(p_js);
	}
	return p_js;
}
//
// 
// 
SVaultPool::SVaultPool() : SBinarySet(), P_KeyRef(0)
{
}

SVaultPool::~SVaultPool()
{
	SlCrypto::ResetEncapsultedKey(P_KeyRef);
	P_KeyRef = 0;
}

const struct SVaultPool_ConstBlock {
	SVaultPool_ConstBlock() :
		//Salt(GUID{0xa948b40, 0xb597, 0x48da, { 0x93, 0x57, 0x71, 0xda, 0xac, 0x60, 0x54, 0x0 }}),
		UedSymmCipher(UED_SYMMETRICCIPHER_AES256GCM), DerivedKeyLen(32), SaltSize(16)
	{
		KdfP.UedKdf = UED_KEYDERIVATIONFUNCTION_ARGON2ID;
		KdfP.MemCost = SKILOBYTE(64);
		KdfP.TimeCost = 3;
		KdfP.Parallelism = 1;
	}
	void   SetupKeyVerificationBlock(SlCrypto::KeyVerificationBlock & rBlk) const
	{
		rBlk.UedSymmCipher = UedSymmCipher;
		rBlk.KdfP = KdfP;
		assert(SaltSize > 0 && SaltSize <= sizeof(rBlk.Salt));
		memrandomize(rBlk.Salt, SaltSize);
		//memcpy(rBlk.Salt, &Salt, sizeof(Salt));
		rBlk.SaltSize = SaltSize;
	}
	SlCrypto::KdfParam KdfP;
	const  uint64 UedSymmCipher;
	const  uint DerivedKeyLen; 
	const  uint SaltSize;
	//const  S_GUID Salt;
} SVaultPool_CBlk;

uint64 SVaultPool::GetKeyRef() const { return reinterpret_cast<uint64>(P_KeyRef); }

uint64 SVaultPool::GetUedSymmCipher() const 
{ 
	// @todo Следующая реализация верна только в случае, если пул пустой. Если же в нем есть сегмент с блоком верификации ключа, то алгоритм надо брать от туда!
	return SVaultPool_CBlk.UedSymmCipher; 
}

int SVaultPool::SetupPrimaryPassword(const char * pPw, size_t pwLen)
{
	int    ok = 1;
	SlCrypto::ResetEncapsultedKey(P_KeyRef);
	P_KeyRef = 0;
	THROW(!isempty(pPw) && pwLen); // @todo @err
	{
		SlCrypto::KeyVerificationBlock kv_blk;
		SlCrypto::Key key;
		SVaultPool_CBlk.SetupKeyVerificationBlock(kv_blk);
		{
			int   mkr = SlCrypto::MakeDerivedKey(kv_blk.KdfP, key, pPw, pwLen, SVaultPool_CBlk.DerivedKeyLen, kv_blk.Salt, kv_blk.SaltSize);
			THROW(mkr);
			const ::SBaseBuffer & r_key_buf = key.GetKey();
			int   r1 = SlCrypto::MakeKeyVerificationBlock(kv_blk, r_key_buf.P_Buf, r_key_buf.Size);
			THROW(r1);
			THROW(Put(tagKeyVerification, &kv_blk, sizeof(kv_blk)));
			{
				void * p_key_ref = SlCrypto::EncapsulateKey(r_key_buf.P_Buf, r_key_buf.Size);
				THROW(p_key_ref);
				P_KeyRef = p_key_ref;
			}
		}
	}
	CATCH
		Put(tagKeyVerification, 0, 0);
		ok = 0;
	ENDCATCH
	return ok;
}

int SVaultPool::CheckInPrimaryPassword(const char * pPw, size_t pwLen)
{
	int    ok = 1;
	SlCrypto::ResetEncapsultedKey(P_KeyRef);
	P_KeyRef = 0;
	THROW(!isempty(pPw) && pwLen); // @todo @err
	{
		uint32 kv_blk_size = 0;
		const SlCrypto::KeyVerificationBlock * p_kv_blk = static_cast<const SlCrypto::KeyVerificationBlock *>(GetPtr(tagKeyVerification, &kv_blk_size));
		THROW(p_kv_blk && kv_blk_size == sizeof(SlCrypto::KeyVerificationBlock) && p_kv_blk->Signature == SlConst::SlCryptoKvbSignature); // @todo @err
		{
			SlCrypto::KeyVerificationBlock kv_blk_(*p_kv_blk);
			SlCrypto::Key key;
			int   mkr = SlCrypto::MakeDerivedKey(kv_blk_.KdfP, key, pPw, pwLen, SVaultPool_CBlk.DerivedKeyLen, kv_blk_.Salt, kv_blk_.SaltSize);
			THROW(mkr);
			const ::SBaseBuffer & r_key_buf = key.GetKey();
			THROW(SlCrypto::VerifyKey(kv_blk_, r_key_buf.P_Buf, r_key_buf.Size));
			{
				void * p_key_ref = SlCrypto::EncapsulateKey(r_key_buf.P_Buf, r_key_buf.Size);
				THROW(p_key_ref);
				P_KeyRef = p_key_ref;
			}
		}
	}
	CATCHZOK
	return ok;
}

static constexpr uint32 SVaultPool_IdOffset = 4000;

/*static*/uint32 SVaultPool::GetMinInternalId() { return SVaultPool_IdOffset+1; }
/*static*/uint32 SVaultPool::MakeInternalId(uint32 outerId) { return (outerId && outerId <= (MAXUINT32-SVaultPool_IdOffset)) ? (outerId+SVaultPool_IdOffset) : 0; }
/*static*/uint32 SVaultPool::MakeOuterId(uint32 internalId) { return (internalId && internalId > SVaultPool_IdOffset) ? (internalId-SVaultPool_IdOffset) : 0; }
//
// 
// 
SlCrypto::KeyVerificationBlock::KeyVerificationBlock() : Signature(SlConst::SlCryptoKvbSignature), UedSymmCipher(0), KdfP(),
	SaltSize(0), VBSize(0)
{
	memzero(Salt, sizeof(Salt));
	memzero(VB, sizeof(VB));
}

SlCrypto::KeyVerificationBlock::~KeyVerificationBlock()
{
	Signature = 0;
	UedSymmCipher = 0;
	memzero(Salt, sizeof(Salt));
	memzero(VB, sizeof(VB));
}

static constexpr GUID VG[] = {
	{0xbc954aa1, 0xe602, 0x4f3f, { 0x83, 0xa3, 0x47, 0x35, 0x78, 0x5a, 0x2, 0xf9 }},
	{0xe3db81e3, 0x5d4c, 0x42e8, { 0xa0, 0x33, 0x7c, 0x90, 0x13, 0x81, 0x11, 0xf7 }},
	{0x32aec6c0, 0x33a1, 0x4471, { 0xba, 0xd3, 0xb9, 0xa4, 0xaa, 0x3, 0x14, 0x14 }},
	{0x72e334ae, 0x7d75, 0x4ecc, { 0x80, 0x5b, 0x94, 0x6a, 0x35, 0x26, 0xf2, 0x35 }}
};

/*static*/int SlCrypto::MakeKeyVerificationBlock(KeyVerificationBlock & rBlk, const void * pKey, size_t keyLen)
{
	int   ok = 0;
	THROW(pKey && keyLen);
	THROW(UED::BelongsToMeta(rBlk.UedSymmCipher, UED_META_SYMMETRICCIPHER)); // @todo @err
	{
		SlCrypto crypto(rBlk.UedSymmCipher);
		THROW(crypto.IsValid());
		const  SlCrypto::CipherProperties & r_cp = crypto.GetCipherProperties();
		const  size_t block_size = (r_cp.BlockSize > 1) ? r_cp.BlockSize : sizeof(S_GUID);
		THROW(block_size <= sizeof(VG));
		THROW(r_cp.KeySize == keyLen); // @todo @err
		{
			SlCrypto::Key key;
			SBinaryChunk data;
			SBinaryChunk encrypted;
			SBinaryChunk tag;
			SBinaryChunk aad;
			SBinaryChunk iv;
			THROW(key.SetKey(pKey, keyLen));
			if(r_cp.IvSize) {
				// Генерируем случайный IV
				iv.Randomize(r_cp.IvSize);
				THROW(crypto.SetKey_IV(key, iv.PtrC(), iv.Len()));
			}
			if(r_cp.AadSize) {
				aad.Randomize(r_cp.AadSize);
				THROW(crypto.SetKey_AAD(key, aad.PtrC(), aad.Len()));
			}
			if(r_cp.TagSize)
				tag.Ensure(r_cp.TagSize);
			data.Z().Cat(VG, block_size);
			THROW(crypto.Encrypt(key, data.PtrC(), data.Len(), encrypted, &tag));
			{
				const  size_t total_enc_data_len = encrypted.Len() + tag.Len() + aad.Len() + iv.Len();
				THROW(total_enc_data_len <= sizeof(rBlk.VB)); // @todo @err
				{
					assert(aad.Len() == r_cp.AadSize);
					assert(iv.Len() == r_cp.IvSize);
					assert(tag.Len() == r_cp.TagSize);
					SBinaryChunk temp_chunk;
					temp_chunk.Cat(aad);
					temp_chunk.Cat(iv);
					temp_chunk.Cat(encrypted);
					temp_chunk.Cat(tag);
					memcpy(rBlk.VB, temp_chunk.PtrC(), temp_chunk.Len());
					rBlk.VBSize = static_cast<uint32>(temp_chunk.Len());
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int SlCrypto::VerifyKey(KeyVerificationBlock & rBlk, const void * pKey, size_t keyLen)
{
	int   ok = 0;
	THROW(pKey && keyLen);
	THROW(UED::BelongsToMeta(rBlk.UedSymmCipher, UED_META_SYMMETRICCIPHER)); // @todo @err
	{
		SlCrypto crypto(rBlk.UedSymmCipher);
		THROW(crypto.IsValid());
		const  SlCrypto::CipherProperties & r_cp = crypto.GetCipherProperties();
		const  size_t block_size = (r_cp.BlockSize > 1) ? r_cp.BlockSize : sizeof(S_GUID);
		THROW(block_size <= sizeof(VG));
		THROW(r_cp.KeySize == keyLen); // @todo @err
		THROW((r_cp.AadSize + r_cp.IvSize + r_cp.TagSize + block_size) == rBlk.VBSize); // @todo @err
		{
			SlCrypto::Key key;
			SBinaryChunk data;
			SBinaryChunk decrypted;
			SBinaryChunk tag;
			SBinaryChunk tag_pattern;
			//SBinaryChunk aad;
			//SBinaryChunk iv;
			THROW(key.SetKey(pKey, keyLen));
			{
				ssize_t encrypted_data_len = rBlk.VBSize - (r_cp.AadSize + r_cp.IvSize + r_cp.TagSize);
				size_t vb_offs = 0;
				THROW(encrypted_data_len > 0);
				if(r_cp.AadSize) {
					key.SetAAD(rBlk.VB+vb_offs, r_cp.AadSize);
					vb_offs += r_cp.AadSize;
				}
				if(r_cp.IvSize) {
					key.SetIV(rBlk.VB+vb_offs, r_cp.IvSize);
					vb_offs += r_cp.IvSize;
				}
				data.Cat(rBlk.VB+vb_offs, encrypted_data_len);
				vb_offs += encrypted_data_len;
				if(r_cp.TagSize) {
					tag.Cat(rBlk.VB+vb_offs, r_cp.TagSize);
					tag_pattern.Cat(rBlk.VB+vb_offs, r_cp.TagSize);
				}
			}
			THROW_S(crypto.Decrypt(key, data.PtrC(), data.Len(), decrypted, &tag), SLERR_CRYPTO_INVCRYPTOKEY);
			THROW_S(decrypted.IsEq(VG, decrypted.Len()), SLERR_CRYPTO_INVCRYPTOKEY);
			THROW_S(tag == tag_pattern, SLERR_CRYPTO_INVCRYPTOKEY);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
class SlEncapsulatedKey {
public:
	class Internal {
	public:
		Internal()
		{
			B.Init();
		}
		~Internal()
		{
			if(B.P_Buf) {
				SAlloc::F_secure(B.P_Buf, B.Size);
			}
		}
		SBaseBuffer B;
	};
	SlEncapsulatedKey()
	{
		memrandomize(V, sizeof(V));
	}
	~SlEncapsulatedKey()
	{
		memzero(V, sizeof(V));
	}
	uint   V[20];
};

/*static*/void * SlCrypto::EncapsulateKey(const void * pKey, size_t keySize) // @v12.6.4
{
	void * p_result = 0;
	SlCrypto::Key key;
	if(pKey && keySize) {
		//const SBaseBuffer & r_dk = key.GetKey();
		//assert(r_dk.P_Buf && r_dk.Size == drvKeySize);
		//if(r_dk.P_Buf && r_dk.Size == drvKeySize) {
		{
			TSClassWrapper <SlEncapsulatedKey> cls_ek;
			const  uint h_ek = SLS.CreateGlobalObject(cls_ek);
			SlEncapsulatedKey * p_ek = h_ek ? static_cast<SlEncapsulatedKey *>(SLS.GetGlobalObject(h_ek)) : 0;
			if(p_ek) {
				constexpr uint vdim = SIZEOFARRAY(p_ek->V);
				const  uint r1 = SLS.GetTLA().Rg.GetUniformIntPos(8); // Спонтанная разница между номинальной размерностью массива и реальным числом элементов в нем
				const  uint vc = vdim - r1; // Реальное число элементов в массиве
				p_ek->V[vdim-1] = vc; // Последний номинальный элемент массива содержит vc
				const  uint vtp = SLS.GetSessUuid().Data[1] % vc; // Индекс элемента, в котором будет храниться реальный ключ, в остальных - обманки
				for(uint i = 0; i < vc; i++) {
					TSClassWrapper <SlEncapsulatedKey::Internal> cls;
					const  uint h = SLS.CreateGlobalObject(cls);
					SlEncapsulatedKey::Internal * p_ = h ? static_cast<SlEncapsulatedKey::Internal *>(SLS.GetGlobalObject(h)) : 0;
					if(p_) {
						p_ek->V[i] = h;
						p_->B.Size = keySize;
						p_->B.P_Buf = static_cast<char *>(SAlloc::M_secure(p_->B.Size));
						if(i == vtp) {
							memcpy(p_->B.P_Buf, pKey, p_->B.Size);
						}
						else {
							SVector kts(1);
							kts.insertChunk(keySize, pKey);
							kts.shuffle();
							assert(kts.getCount() == p_->B.Size); // @paranoic
							memcpy(p_->B.P_Buf, kts.dataPtr(), kts.getCount());
						}
					}
				}
				p_result = new uint;
				if(p_result) {
					PTR32(p_result)[0] = h_ek;
				}
			}
		}
	}
	return p_result;
}

/*static*/bool SlCrypto::GetEncapsulatedKey(void * pEK, SBaseBuffer & rResult)
{
	rResult.Init();
	bool   ok = false;
	if(pEK) {
		uint   h_ek = PTR32(pEK)[0];
		const  SlEncapsulatedKey * p_ek = h_ek ? static_cast<SlEncapsulatedKey *>(SLS.GetGlobalObject(h_ek)) : 0;
		if(p_ek) {
			constexpr uint vdim = SIZEOFARRAY(p_ek->V);
			const  uint vc = p_ek->V[vdim-1];
			const  uint vtp = SLS.GetSessUuid().Data[1] % vc;
			const  uint h = p_ek->V[vtp];
			SlEncapsulatedKey::Internal * p_ = h ? static_cast<SlEncapsulatedKey::Internal *>(SLS.GetGlobalObject(h)) : 0;
			if(p_) {
				rResult.Copy(p_->B);
				ok = true;
			}
		}
	}
	return ok;
}

/*static*/void SlCrypto::ResetEncapsultedKey(void * pEK) // @v12.6.4 @construction
{
	if(pEK) {
		uint   h_ek = PTR32(pEK)[0];
		SlEncapsulatedKey * p_ek = h_ek ? static_cast<SlEncapsulatedKey *>(SLS.GetGlobalObject(h_ek)) : 0;
		if(p_ek) {
			constexpr uint vdim = SIZEOFARRAY(p_ek->V);
			const  uint vc = p_ek->V[vdim-1];
			for(uint i = 0; i < vc; i++) {
				const  uint h = p_ek->V[i];
				SLS.DestroyGlobalObject(h);
			}
			SLS.DestroyGlobalObject(h_ek);
		}
		delete pEK;
	}
}
//
//
//
SlCrypto::CipherProperties::CipherProperties() : BlockSize(0), KeySize(0), IvSize(0), AadSize(0), TagSize(0)
{
}

SlCrypto::KdfParam::KdfParam() : UedKdf(0), MemCost(0), TimeCost(0), Parallelism(0)
{
	memzero(Reserve, sizeof(Reserve));
}

SlCrypto::Key::Key() : Signature(SlConst::SlCryptoKeySignature), EndP(0)
{
	KEY.Init();
	IV.Init();
	AAD.Init();
}

SlCrypto::Key::~Key()
{
	Signature = 0;
	memrandomize(Bin, sizeof(Bin)); // @v12.6.4
}

bool SlCrypto::Key::IsConsistent() const { return (this != 0 && Signature == SlConst::SlCryptoKeySignature); }

SlCrypto::Key & SlCrypto::Key::Z()
{
	KEY.Init();
	IV.Init();
	AAD.Init();
	EndP = 0;
	memrandomize(Bin, sizeof(Bin)); // @v12.6.4
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
		EndP += static_cast<uint32>(size);
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
		EndP += static_cast<uint32>(size);
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
		EndP += static_cast<uint32>(size);
	}
	return ok;
}

SlCrypto::SlCrypto(uint64 uedCipher) : // @v12.6.3 @construction
	P_Ctx(0), P_Cphr(0), Alg(0), KeyBitLen(0), AlgModif(0), State(0)
{
	int    alg = 0;
	uint   kbl = 0;
	int    alg_modif = 0;
	if(UED::BelongsToMeta(uedCipher, UED_META_SYMMETRICCIPHER)) {
		switch(uedCipher) {
			case UED_SYMMETRICCIPHER_CAMELLIA128: 
				alg = algCamellia;
				break;
			case UED_SYMMETRICCIPHER_CAMELLIA256: 
				alg = algCamellia;
				break;
			case UED_SYMMETRICCIPHER_EGOST2814789CNT: 
				break;
			case UED_SYMMETRICCIPHER_SEED: 
				break;
			case UED_SYMMETRICCIPHER_AES128GCM: 
				alg = algAes;
				alg_modif = algmodGcm;
				kbl = kbl128;
				break;
			case UED_SYMMETRICCIPHER_AES256GCM: 
				alg = algAes;
				alg_modif = algmodGcm;
				kbl = kbl256;
				break;
			case UED_SYMMETRICCIPHER_AES128CCM: 
				alg = algAes;
				alg_modif = algmodCcm;
				kbl = kbl128;
				break;
			case UED_SYMMETRICCIPHER_AES256CCM: 
				alg = algAes;
				alg_modif = algmodCcm;
				kbl = kbl256;
				break;
			/*
			case UED_SYMMETRICCIPHER_AES128CCM8: 
				alg = algAes;
				kbl = kbl128;
				break;
			case UED_SYMMETRICCIPHER_AES256CCM8: 
				alg = algAes;
				kbl = kbl256;
				break;
			*/
			case UED_SYMMETRICCIPHER_CHACHA20POLY1305: 
				alg = algChaCha20;
				alg_modif = algmodPoly1305;
				break;
		}
		Helper_Construct(alg, kbl, alg_modif, 0);
	}
	else {
		State |= stError;
	}
}

SlCrypto::SlCrypto(int alg, uint kbl, int algModif) : P_Ctx(0), P_Cphr(0), Alg(alg), KeyBitLen(kbl), AlgModif(algModif), State(0)
{
	Helper_Construct(alg, kbl, algModif, 0);
}

int SlCrypto::Helper_Construct(int alg, uint kbl, int algModif, uint tagSize)
{
	int    ok = 1;
	const  EVP_CIPHER * p_cphr = 0;
	switch(alg) {
		case algAes:
			switch(kbl) {
				case kbl128:
					switch(algModif) {
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
					switch(algModif) {
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
					switch(algModif) {
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
			switch(algModif) {
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
			switch(algModif) {
				case algmodCbc: p_cphr = EVP_des_ede_cbc(); break;
				case algmodEcb: p_cphr = EVP_des_ede_ecb(); break;
				case algmodOfb: p_cphr = EVP_des_ede_ofb(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_des_ede_cfb64(); break;
				default: p_cphr = EVP_des_ede(); break;
			}
			break;
		case algDes_Ede3:
			switch(algModif) {
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
			switch(algModif) {
				case algmodCbc: p_cphr = EVP_desx_cbc(); break;
				default: p_cphr = EVP_desx_cbc(); break;
			}
			break;
		case algIdea:
			switch(algModif) {
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
					switch(algModif) {
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
					switch(algModif) {
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
					switch(algModif) {
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
			switch(algModif) {
				case algmodEcb: p_cphr = EVP_bf_ecb(); break;
				case algmodCbc: p_cphr = EVP_bf_cbc(); break;
				case algmodOfb: p_cphr = EVP_bf_ofb(); break;
				case algmodCfb: 
				case algmodCfb64: p_cphr = EVP_bf_cfb64(); break;
			}
			break;
		case algChaCha20:
			switch(algModif) {
				case algmodPoly1305: p_cphr = EVP_chacha20_poly1305(); break;
				default: p_cphr = EVP_chacha20(); break;
			}
			break;
	}
	if(p_cphr) {
		Alg = alg;
		KeyBitLen = kbl;
		AlgModif = algModif;
		P_Cphr = p_cphr;
		P_Ctx = EVP_CIPHER_CTX_new();
		EVP_CIPHER_CTX * p_ctx = static_cast<EVP_CIPHER_CTX *>(P_Ctx);
		Cp.BlockSize = static_cast<uint>(EVP_CIPHER_block_size(p_cphr));
		Cp.KeySize = static_cast<uint>(EVP_CIPHER_key_length(p_cphr));
		Cp.IvSize = static_cast<uint>(EVP_CIPHER_iv_length(p_cphr));
		// @v12.6.6 Cp.TagSize = (EVP_CIPHER_get_flags(p_cphr) & EVP_CIPH_FLAG_AEAD_CIPHER) ? EVP_CIPHER_CTX_get_tag_length(p_ctx) : 0;
		if(EVP_EncryptInit_ex(p_ctx, p_cphr, NULL, NULL, NULL)) { // Set cipher type and mode 
			Cp.BlockSize = static_cast<uint>(EVP_CIPHER_CTX_block_size(p_ctx));
			Cp.KeySize = static_cast<uint>(EVP_CIPHER_CTX_key_length(p_ctx));
			Cp.IvSize = static_cast<uint>(EVP_CIPHER_CTX_iv_length(p_ctx));
			Cp.TagSize = (EVP_CIPHER_get_flags(p_cphr) & EVP_CIPH_FLAG_AEAD_CIPHER) ? EVP_CIPHER_CTX_get_tag_length(p_ctx) : 0; // @v12.6.6 
		}
		else {
			EVP_CIPHER_CTX_free(p_ctx);
			P_Ctx = 0;
			State |= stError;
			ok = 0;
		}
	}
	else {
		State |= stError;
		ok = 0;
	}
	return ok;
}


SlCrypto::~SlCrypto()
{
	if(P_Ctx) {
		EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX *>(P_Ctx));
		P_Ctx = 0;
	}
}

bool SlCrypto::IsIvSizeConfigurable() const { return oneof4(AlgModif, algmodCcm, algmodGcm, algmodCtr, algmodPoly1305); }
	
const  SlCrypto::CipherProperties & SlCrypto::GetCipherProperties() const { return Cp; }

/*static*/int SlCrypto::MakeDerivedKey(const KdfParam & rParam, SlCrypto::Key & rK, const void * pData, size_t size, size_t destKeySize, const void * pSalt, size_t saltSize) // @v12.6.4
{
	int    ok = 1;
	const  size_t result_key_size = destKeySize;
	SString temp_buf;
	KdfParam ap(rParam);
	THROW(pData && size);
	THROW(result_key_size);
	THROW(rK.GetKey().P_Buf == 0); // Если не так, то значит ключ уже установлен
	if(!ap.MemCost) {
		ap.MemCost = SKILOBYTE(4); // = 4MiB // Предлагают использовать 64mb, но я пока остановлюсь на 4
	}
	if(!ap.TimeCost) {
		ap.TimeCost = 3;
	}
	if(!ap.Parallelism) {
		ap.Parallelism = 1;
	}
	{
		uint8  key_buf[256];
		assert(result_key_size < sizeof(key_buf));
		uint8  default_argon2_salt[32];
		size_t default_argon2_salt_size = 0;
		if(!pSalt || !saltSize) {
			reinterpret_cast<uint64 *>(default_argon2_salt)[0] = SlConst::FnvHash1Init64;
			reinterpret_cast<uint64 *>(default_argon2_salt)[1] = SlConst::CrcPoly_64_Lzma;
			default_argon2_salt_size = sizeof(uint64) * 2;
			pSalt = default_argon2_salt;
			saltSize = default_argon2_salt_size;
		}
		int    ar = 0;
		switch(ap.UedKdf) {
			case UED_KEYDERIVATIONFUNCTION_ARGON2D:
				ar = argon2d_hash_raw(ap.TimeCost, ap.MemCost, ap.Parallelism, pData, size, pSalt, saltSize, key_buf, result_key_size);
				break;
			case UED_KEYDERIVATIONFUNCTION_ARGON2I:
				ar = argon2i_hash_raw(ap.TimeCost, ap.MemCost, ap.Parallelism, pData, size, pSalt, saltSize, key_buf, result_key_size);
				break;
			case UED_KEYDERIVATIONFUNCTION_ARGON2ID:
				ar = argon2id_hash_raw(ap.TimeCost, ap.MemCost, ap.Parallelism, pData, size, pSalt, saltSize, key_buf, result_key_size);
				break;
			default:
				CALLEXCEPT_S_S(SLERR_CRYPTO_UNSUPPORTEDKDF, temp_buf.Z().CatHex(ap.UedKdf));
				break;
		}
		THROW_S(ar == ARGON2_OK, SLERR_CRYPTO_KDF_FAULT); // ? ERR_SUCCESS : ERR_ARGON2;
		THROW(rK.SetKey(key_buf, result_key_size));
		memzero(key_buf, sizeof(key_buf));
	}
	CATCHZOK
	return ok;
}

int SlCrypto::SetKey_IV(SlCrypto::Key & rK, const void * pData, size_t size) // @v12.6.3
{
	int   ok = 1;
	THROW(rK.GetIV().P_Buf == 0); // Если не так, то значит IV уже установлен
	if(pData) {
		THROW(size);
		if(!IsIvSizeConfigurable()) {
			THROW_S(size == Cp.IvSize, SLERR_CRYPTO_INVIVSIZE); // SLERR_CRYPTO_INVIVSIZE  Ошибка SLCRYPTO - недопустимая длина IV
		}
		THROW(rK.SetIV(pData, size));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SlCrypto::SetKey_AAD(SlCrypto::Key & rK, const void * pData, size_t size) // @v12.6.3
{
	int   ok = 1;
	THROW(rK.GetAAD().P_Buf == 0); // Если не так, то значит AAD уже установлен
	if(pData) {
		THROW(size);
		THROW(rK.SetAAD(pData, size));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

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
	THROW(SetKey_IV(rK, pIv, ivLen));
	THROW(SetKey_AAD(rK, pAad, aadLen));
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
	THROW(P_Ctx && P_Cphr);
	{
		const  int result_key_len = EVP_BytesToKey(static_cast<const EVP_CIPHER *>(P_Cphr), EVP_md5(), p_salt, 
			reinterpret_cast<const uchar *>(pPassword), sstrleni(pPassword), 1/*count*/, key, iv);
		THROW(result_key_len);
		assert(result_key_len == static_cast<int>(Cp.KeySize));
		if(Cp.KeySize) {
			rK.SetKey(key, result_key_len);
		}
	}
	if(Cp.IvSize) {
		rK.SetIV(iv, Cp.IvSize);
	}
	CATCHZOK
	return ok;
}

int SlCrypto::Encrypt(const SlCrypto::Key & rKey, const void * pData, size_t dataLen, SBinaryChunk & rResult, SBinaryChunk * pTag)
{
	rResult.Z();
	int    ok = 1;
	int    outl = 0;
	int    final_outl = 0;
	bool   is_unstd_iv_size = false;
	size_t tag_size = 0;
	const  size_t tag_size_std = 16;
	EVP_CIPHER_CTX * p_ctx = static_cast<EVP_CIPHER_CTX *>(P_Ctx);
	const SBaseBuffer & r_key = rKey.GetKey();
	const SBaseBuffer & r_iv = rKey.GetIV();
	const SBaseBuffer & r_aad = rKey.GetAAD();
	THROW_S(p_ctx && P_Cphr, SLERR_CRYPTO_INVPARAM);
	//THROW(EVP_EncryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), NULL, NULL, NULL)); // Set cipher type and mode 
	THROW_S(!Cp.KeySize || !r_key.Size || r_key.Size == Cp.KeySize, SLERR_CRYPTO_INVKEYSIZE);
	if(r_iv.Size) {
		if(IsIvSizeConfigurable()) {
			if(r_iv.Size != Cp.IvSize) {
				is_unstd_iv_size = true;
			}
		}
		else {
			THROW_S(r_iv.Size == Cp.IvSize, SLERR_CRYPTO_INVIVSIZE);
		}
	}
	if(oneof2(AlgModif, algmodGcm, algmodCcm)) {
		THROW(pTag && pTag->Len() >= 8); // @todo @err // @v12.6.3 (>=12)-->(>=8)
		tag_size = pTag->Len();
	}
	if(is_unstd_iv_size) {
		THROW_S(EVP_EncryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), 0, 0, 0), SLERR_CRYPTO_OPENSSL); // Initialise key and IV 
		// @fixme Возможно, в зависимости от алгоритма параметр EVP_CTRL_GCM_SET_IVLEN должен меняться //
		THROW_S(EVP_CIPHER_CTX_ctrl(p_ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(r_iv.Size), NULL), SLERR_CRYPTO_OPENSSL);
		if(tag_size && tag_size != tag_size_std) {
			OSSL_PARAM params[2];
			params[0] = OSSL_PARAM_construct_size_t(OSSL_CIPHER_PARAM_AEAD_TAGLEN, &tag_size);
			params[1] = OSSL_PARAM_construct_end();
			THROW_S(EVP_CIPHER_CTX_set_params(p_ctx, params), SLERR_CRYPTO_OPENSSL);
		}
		THROW_S(EVP_EncryptInit_ex(p_ctx, 0, 0, PTR8C(r_key.P_Buf), PTR8C(r_iv.P_Buf)), SLERR_CRYPTO_OPENSSL); // Initialise key and IV 
	}
	else {
		if(tag_size && tag_size != tag_size_std) {
			OSSL_PARAM params[2];
			params[0] = OSSL_PARAM_construct_size_t(OSSL_CIPHER_PARAM_AEAD_TAGLEN, &tag_size);
			params[1] = OSSL_PARAM_construct_end();
			THROW_S(EVP_CIPHER_CTX_set_params(p_ctx, params), SLERR_CRYPTO_OPENSSL);
		}
		THROW_S(EVP_EncryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), 0, PTR8C(r_key.P_Buf), PTR8C(r_iv.P_Buf)), SLERR_CRYPTO_OPENSSL); // Initialise key and IV 
	}
	EVP_CIPHER_CTX_set_padding(p_ctx, 1);
	State &= ~stInitDecr;
	State |= stInitEncr;
	{
		uint8  zero_buf[16];
		size_t estimated_result_size = ((dataLen + Cp.BlockSize) / Cp.BlockSize) * Cp.BlockSize;
		THROW(rResult.Ensure(estimated_result_size));
		MEMSZERO(zero_buf);
		if(dataLen == 0) {
			pData = zero_buf;
		}
		{
			/*
			if(oneof2(AlgModif, algmodGcm, algmodCcm)) {
				int   tag_type = 0;
				THROW(pTag && pTag->Len() >= 12); // @todo @err
				if(AlgModif == algmodGcm)
					tag_type = EVP_CTRL_GCM_SET_TAG;
				else if(AlgModif == algmodCcm)
					tag_type = EVP_CTRL_CCM_SET_TAG;
				if(tag_type) {
					EVP_CIPHER_CTX_ctrl(p_ctx, tag_type, static_cast<int>(pTag->Len()), 0);
				}
			}
			*/
			if(r_aad.Size) {
				THROW_S(EVP_EncryptUpdate(p_ctx, 0, &outl, PTR8C(r_aad.P_Buf), static_cast<int>(r_aad.Size)), SLERR_CRYPTO_OPENSSL);
			}
			THROW_S(EVP_EncryptUpdate(p_ctx, PTR8(rResult.Ptr()), &outl, PTR8C(pData), static_cast<int>(dataLen)), SLERR_CRYPTO_OPENSSL);
			THROW_S(EVP_EncryptFinal_ex(p_ctx, PTR8(rResult.Ptr())+outl, &final_outl), SLERR_CRYPTO_OPENSSL);
			outl += final_outl;
			assert(static_cast<ssize_t>(outl) <= static_cast<ssize_t>(rResult.Len())); // Если условие не выполняется, то мы ошиблись с первоначальной оценкой максимального размера результата
			THROW(rResult.Ensure(outl));
			if(oneof2(AlgModif, algmodGcm, algmodCcm)) {
				int   tag_type = 0;
				if(AlgModif == algmodGcm)
					tag_type = EVP_CTRL_GCM_GET_TAG;
				else if(AlgModif == algmodCcm)
					tag_type = EVP_CTRL_CCM_GET_TAG;
				if(tag_type) {
					THROW(EVP_CIPHER_CTX_ctrl(p_ctx, tag_type, static_cast<int>(pTag->Len()), pTag->Ptr()));
				}
			}
		}
	}
	CATCHZOK
	//ASSIGN_PTR(pActualResultLen, outl);
	return ok;
}

int SlCrypto::Decrypt(const SlCrypto::Key & rKey, const void * pData, size_t dataLen, SBinaryChunk & rResult, const SBinaryChunk * pTag)
{
	rResult.Z();
	int    ok = 1;
	int    outl = 0;
	EVP_CIPHER_CTX * p_ctx = static_cast<EVP_CIPHER_CTX *>(P_Ctx);
	THROW(p_ctx && P_Cphr);
	{
		int    final_outl = 0;
		const  SBaseBuffer & r_key = rKey.GetKey();
		const  SBaseBuffer & r_iv = rKey.GetIV();
		const  SBaseBuffer & r_aad = rKey.GetAAD();
		const  size_t estimated_result_size = ((dataLen + Cp.BlockSize) / Cp.BlockSize) * Cp.BlockSize;
		//THROW(EVP_DecryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), NULL, NULL, NULL)); // Set cipher type and mode 
		THROW_S(!Cp.KeySize || !r_key.Size || r_key.Size == Cp.KeySize, SLERR_CRYPTO_INVKEYSIZE);
		THROW(r_iv.Size == Cp.IvSize);
		THROW(EVP_DecryptInit_ex(p_ctx, static_cast<const EVP_CIPHER *>(P_Cphr), 0, PTR8C(r_key.P_Buf), PTR8C(r_iv.P_Buf))); // Initialise key and IV 
		EVP_CIPHER_CTX_set_padding(p_ctx, 1);
		State |= stInitDecr;
		State &= ~stInitEncr;
		{
			THROW(State & stInitDecr);
			if(pData && dataLen) {
				if(oneof2(AlgModif, algmodGcm, algmodCcm)) {
					THROW(pTag && pTag->Len() > 0); // @todo @err
				}
				THROW(rResult.Ensure(estimated_result_size));
				if(r_aad.Size) {
					THROW(EVP_DecryptUpdate(p_ctx, 0, &outl, reinterpret_cast<const uchar *>(r_aad.P_Buf), static_cast<int>(r_aad.Size)));
				}
				THROW(EVP_DecryptUpdate(p_ctx, PTR8(rResult.Ptr()), &outl, PTR8C(pData), static_cast<int>(dataLen)));
				if(pTag) {
					// Set expected tag value. Works in OpenSSL 1.0.1d and later 
					int   tag_type = 0;
					if(AlgModif == algmodGcm)
						tag_type = EVP_CTRL_GCM_SET_TAG;
					else if(AlgModif == algmodCcm)
						tag_type = EVP_CTRL_CCM_SET_TAG;
					if(tag_type) {
						THROW(EVP_CIPHER_CTX_ctrl(p_ctx, tag_type, static_cast<int>(pTag->Len()), const_cast<void *>(pTag->PtrC())));
					}
				}
				THROW(EVP_DecryptFinal/*_ex*/(p_ctx, PTR8(rResult.Ptr())+outl, &final_outl));
				outl += final_outl; // ?
				assert(static_cast<ssize_t>(outl) <= static_cast<ssize_t>(rResult.Len())); // Если условие не выполняется, то мы ошиблись с первоначальной оценкой максимального размера результата
				THROW(rResult.Ensure(outl));
			}
			else {
				//THROW(EVP_DecryptFinal_ex(p_ctx, static_cast<uchar *>(pResult), &outl));
			}
		}
	}
	CATCHZOK
	//ASSIGN_PTR(pActualResultLen, outl);
	return ok;
}

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
		EVP_CIPHER_CTX_set_padding(p_ctx, 1); // @v11.1.11
		State &= ~stInitDecr;
		State |= stInitEncr;
		if(pResult) {
			//THROW(State & stInitEncr);
			uint8 zero_buf[16];
			MEMSZERO(zero_buf);
			if(dataLen == 0) {
				pData = zero_buf;
			}
			//if(pData && dataLen) {
				if(r_aad.Size) {
					THROW_S(EVP_EncryptUpdate(p_ctx, 0, &outl, reinterpret_cast<const uchar *>(r_aad.P_Buf), static_cast<int>(r_aad.Size)), SLERR_CRYPTO_OPENSSL);
				}
				THROW_S(EVP_EncryptUpdate(p_ctx, static_cast<uchar *>(pResult), &outl, static_cast<const uchar *>(pData), static_cast<int>(dataLen)), SLERR_CRYPTO_OPENSSL);
				THROW_S(EVP_EncryptFinal_ex(p_ctx, static_cast<uchar *>(pResult)+outl, &final_outl), SLERR_CRYPTO_OPENSSL);
				outl += final_outl;
			//}
			//else {
				//THROW_S(EVP_EncryptFinal_ex(p_ctx, static_cast<uchar *>(pResult), &outl), SLERR_CRYPTO_OPENSSL);
			//}
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
		EVP_CIPHER_CTX_set_padding(p_ctx, 1); // @v11.1.11
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
//
// Descr: Экспериментальный блок для формирования функциональной цепочки вычисления ключа.
//   Цель - уйти от необходимости хранить в памяти даже derived-версию ключа.
// 
// Идея такова (предварительно):
//   Вместо ключа мы будем хранить цепочку, определяющую правило вычисления этого ключа. То есть, фактически, это - "умная" обфускация.
//   Цепочка вычисления содержит сложные ссылки на функции и их аргументы. Последовательно применяя функции к соответствующим аргументам
//   получаем все байты ключа.
//   Все функции обратимы - при формировании цепочки мы вычислим аргумент на основе оригинальных байтов ключа дабы при вычислении произвести реверсивное действие.
//
class SlKeyEvaluationChain { // @v12.6.3
public:
	SlKeyEvaluationChain();
	int    Make(const void * pKey, size_t keySize);
	int    Evaluate(SBinaryChunk & rKey) const;
private:
	static uint64 Func01(uint64 arg, bool reverse);
	static uint64 Func02(uint64 arg, bool reverse);
	static uint64 Func03(uint64 arg, bool reverse);
	static uint64 Func04(uint64 arg, bool reverse);
	static uint64 Func05(uint64 arg, bool reverse);
	struct AdjustmentBlock {
		uint64 Pad[3];
	};
	Uint64Array Chain;
};

class SlDynamicBitSpreader {
private:
	// Мультипликативный генератор круглых ключей (64-bit MurmurHash3 mixer)
	static uint64 GetRoundKey(uint64 factor, int round) 
	{
		uint64 x = factor + round * 0x9e3779b97f4a7c15ULL; // Дробная часть золотого сечения
		x ^= x >> 30;
		x *= 0xbf58476d1ce4e5b9ULL;
		x ^= x >> 27;
		x *= 0x94d049bb133111ebULL;
		x ^= x >> 31;
		return x;
	}
	// Циклический сдвиг влево внутри динамического пространства M бит
	static uint64 RotateLeft(uint64 val, uint32 shift, uint32 M) 
	{
		if(M <= 1) 
			return val;
		shift = shift % M;
		if(shift == 0) 
			return val;
		uint64 mask = (M == 64) ? ~0ULL : ((1ULL << M) - 1);
		val &= mask;
		return ((val << shift) | (val >> (M - shift))) & mask;
	}
	// Циклический сдвиг вправо внутри динамического пространства M бит
	static uint64 RotateRight(uint64 val, uint32 shift, uint32 M) 
	{
		if(M <= 1) 
			return val;
		shift = shift % M;
		if(shift == 0) 
			return val;
		uint64 mask = (M == 64) ? ~0ULL : ((1ULL << M) - 1);
		val &= mask;
		return ((val >> shift) | (val << (M - shift))) & mask;
	}
public:
	// Прямой ход: распределяет N полезных бит в M-битное пространство с шумом и фактором
	// Требование: N < M <= 64
	static uint64 Spread(uint64 key_bits, uint32 N, uint32 M, uint64 factor) 
	{
		assert(M > N);
		assert(M <= 64);
		assert(N >= 1);
		uint64 mask_N = (N == 64) ? ~0ULL : ((1ULL << N) - 1);
		key_bits &= mask_N; // Гарантируем чистоту исходных N бит
		// 1. Заполняем свободные M - N бит детерминированным шумом на основе фактора
		uint32 noise_bits_count = M - N;
		uint64 noise_seed = GetRoundKey(factor, 99);
		uint64 noise_mask = (noise_bits_count == 64) ? ~0ULL : ((1ULL << noise_bits_count) - 1);
		uint64 noise = noise_seed & noise_mask;
		// 2. Собираем M-битный блок: полезные биты сдвигаем в голову, шум — в хвост
		uint64 block = (key_bits << noise_bits_count) | noise;
		// 3. Смешиваем блок по схеме ARX (6 раундов)
		uint64 mask_M = (M == 64) ? ~0ULL : ((1ULL << M) - 1);
		uint64 state = block & mask_M;
		for(int r = 0; r < 6; ++r) {
			uint64 rk = GetRoundKey(factor, r) & mask_M;
			state = (state + rk) & mask_M;       // Сложение по модулю 2^M
			state = state ^ rk;                  // XOR
			state = RotateLeft(state, r + 1, M); // Циклический сдвиг
		}
		return state;
	}
	// Обратный ход: восстанавливает исходные N бит с точностью до бита
	static uint64 Restore(uint64 diffused_value, uint32 N, uint32 M, uint64 factor) 
	{
		assert(M > N);
		assert(M <= 64);
		assert(N >= 1);
		uint64 mask_M = (M == 64) ? ~0ULL : ((1ULL << M) - 1);
		uint64 state = diffused_value & mask_M;
		// 1. Разворачиваем смешивание в обратном порядке
		for(int r = 5; r >= 0; --r) {
			uint64 rk = GetRoundKey(factor, r) & mask_M;
			state = RotateRight(state, r + 1, M); // Обратный сдвиг
			state = state ^ rk;                   // Обратный XOR
			state = (state - rk) & mask_M;        // Вычитание по модулю 2^M (поддерживает беззнаковый подрез)
		}
		// 2. Отбрасываем восстановленный шум, возвращая оригинальные N бит
		uint32 noise_bits_count = M - N;
		uint64 recovered_key = state >> noise_bits_count;
		uint64 mask_N = (N == 64) ? ~0ULL : ((1ULL << N) - 1);
		return recovered_key & mask_N;
	}
};

class SlDynamicBitSpreaderTest {
public:
	//
	// Тест 1: Проверка биективности на случайных данных (Fuzzing-тест)
	// Строгая биективность (Roundtrip): Проверка того, что для любых случайных данных, размеров N,M и факторов обратный ход восстанавливает исходный ключ с точностью до бита.
	//
	static bool TestRoundtrip(uint32 iterations) 
	{
		uint64 seed = 0x1234567890ABCDEFULL;
		for(uint32 i = 0; i < iterations; ++i) {
			// Псевдослучайная генерация параметров на основе LCG
			seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
			uint32 M = 2 + (seed % 63); // M в диапазоне [2, 64]
			uint32 N = 1 + (seed % (M - 1)); // N в диапазоне [1, M-1]
			uint64 key_bits = seed & ((N == 64) ? ~0ULL : ((1ULL << N) - 1));
			uint64 factor = seed ^ (seed >> 32);
			// Прямой и обратный ход
			uint64 diffused = SlDynamicBitSpreader::Spread(key_bits, N, M, factor);
			uint64 recovered = SlDynamicBitSpreader::Restore(diffused, N, M, factor);
			if(recovered != key_bits) {
				std::cerr << "FAIL [Roundtrip]: Iteration " << i << "\n"
				<< "  N=" << N << ", M=" << M << "\n"
				<< "  Key bits:  0x" << std::hex << key_bits << "\n"
				<< "  Recovered: 0x" << recovered << "\n";
				return false;
			}
		}
		std::cout << "OK   [Roundtrip]: " << std::dec << iterations << " iterations passed.\n";
		return true;
	}
	// Тест 2: Проверка экстремальных граничных условий
	static bool TestEdgeCases() 
	{
		struct TestCase {
			uint32 N;
			uint32 M;
			uint64 Key;
			uint64 Factor;
		} cases[] = {
			{ 1, 2,  1, 0 },                      // Минимальный размер
			{ 1, 2,  0, 0xFFFFFFFFFFFFFFFFULL },  // Минимальный размер, экстремальный фактор
			{ 63, 64, 0x5555555555555555ULL & ((1ULL<<63)-1), 0xAAAAAAAAAAAAAAAAULL }, // Максимальный размер
			{ 32, 64, 0xDEADC0DEULL, 0x0123456789ABCDEFULL }                           // Стандартный split
		};
		for(size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
			TestCase tc = cases[i];
			uint64 diffused = SlDynamicBitSpreader::Spread(tc.Key, tc.N, tc.M, tc.Factor);
			uint64 recovered = SlDynamicBitSpreader::Restore(diffused, tc.N, tc.M, tc.Factor);
			if(recovered != tc.Key) {
				std::cerr << "FAIL [EdgeCase] " << i << ": N=" << tc.N << ", M=" << tc.M << "\n";
				return false;
			}
		}
		std::cout << "OK   [EdgeCases]: All boundary tests passed.\n";
		return true;
	}
	// Тест 3: Оценка лавинного эффекта (криптографическая диффузия)
	static bool TestAvalancheEffect() 
	{
		const uint32 N = 32;
		const uint32 M = 64;
		const uint64 factor = 0x9e3779b97f4a7c15ULL;
		const uint64 base_key = 0xDEADC0DEULL;
		uint64 base_diffused = SlDynamicBitSpreader::Spread(base_key, N, M, factor);
		double total_bit_flips = 0.0;
		uint32 test_runs = 0;
		// Изменяем по одному биту в исходном ключе и смотрим, сколько бит изменилось на выходе
		for(uint32 i = 0; i < N; ++i) {
			uint64 modified_key = base_key ^ (1ULL << i);
			uint64 modified_diffused = SlDynamicBitSpreader::Spread(modified_key, N, M, factor);
			// Разница между базовым и измененным выходом (Hamming distance)
			uint32 flipped_bits = SBits::Cpop(base_diffused ^ modified_diffused);
			total_bit_flips += flipped_bits;
			test_runs++;
		}
		double average_flips = total_bit_flips / test_runs;
		double target_flips = static_cast<double>(M) / 2.0; // В идеале должно быть ровно M/2 (32 бита)
		double deviation = std::abs(average_flips - target_flips);
		std::cout << "INFO [Avalanche]: Average flipped bits: " << std::fixed << std::setprecision(2) << average_flips << " (Target: " << target_flips << ")\n";
		// Допускаем отклонение не более 10% для небольшого набора тестов
		if(deviation > (M * 0.10)) {
			std::cerr << "FAIL [Avalanche]: Poor diffusion. Deviation too high: " << deviation << "\n";
			return false;
		}
		std::cout << "OK   [Avalanche]: High diffusion confirmed.\n";
		return true;
	}
};

int main__() 
{
	bool ok = true;
	ok &= SlDynamicBitSpreaderTest::TestEdgeCases();
	ok &= SlDynamicBitSpreaderTest::TestRoundtrip(100000); // 100 тысяч fuzzed-тестов
	ok &= SlDynamicBitSpreaderTest::TestAvalancheEffect();
	if(ok) {
		std::cout << "--- ALL TESTS PASSED SUCCESSFULLY ---\n";
		return 0;
	} 
	else {
		std::cerr << "--- TEST SUITE FAILED ---\n";
		return 1;
	}
}

/*static*/uint64 SlKeyEvaluationChain::Func01(uint64 arg, bool reverse) // @construction
{
	uint64 result = 0;
	if(reverse) {
		//
		const  uint64 rv = Func01(result, false);
		if(rv != arg) {
			result = 0;
		}
	}
	else {
	}
	return result;
}

/*static*/uint64 SlKeyEvaluationChain::Func02(uint64 arg, bool reverse) // @construction
{
	uint64 result = 0;
	return result;
}

/*static*/uint64 SlKeyEvaluationChain::Func03(uint64 arg, bool reverse) // @construction
{
	uint64 result = 0;
	return result;
}

/*static*/uint64 SlKeyEvaluationChain::Func04(uint64 arg, bool reverse) // @construction
{
	uint64 result = 0;
	return result;
}

/*static*/uint64 SlKeyEvaluationChain::Func05(uint64 arg, bool reverse) // @construction
{
	uint64 result = 0;
	return result;
}

SlKeyEvaluationChain::SlKeyEvaluationChain()
{
}

int SlKeyEvaluationChain::Make(const void * pKey, size_t keySize) // @construction
{
	int    ok = 0;
	if(pKey && keySize) {
		for(size_t i = 0; i < keySize; i++) {
			const uint8 src_byte = PTR8C(pKey)[i];
		}
	}
	return ok;
}
	
int SlKeyEvaluationChain::Evaluate(SBinaryChunk & rKey) const // @construction
{
	int    ok = 0;
	return ok;
}
