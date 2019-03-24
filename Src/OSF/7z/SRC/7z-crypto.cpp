// 7Z-CRYPTO.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//
// MyAes.cpp HMACSHA.CPP Pbkdf2HmacSha1.cpp WzAes.cpp 7zAes.cpp RarAes.cpp Rar20Crypto.cpp Rar5Aes.cpp ZipCrypto.cpp ZipStrong.cpp MyAesReg.cpp 7zAesRegister.cpp

// @sobolev EXTERN_C_BEGIN
void FASTCALL AesCbc_Encode(uint32 * ivAes, Byte * data, size_t numBlocks);
void FASTCALL AesCbc_Decode(uint32 * ivAes, Byte * data, size_t numBlocks);
void FASTCALL AesCtr_Code(uint32 * ivAes, Byte * data, size_t numBlocks);

extern "C" void FASTCALL AesCbc_Encode_Intel(uint32 * ivAes, Byte * data, size_t numBlocks);
extern "C" void FASTCALL AesCbc_Decode_Intel(uint32 * ivAes, Byte * data, size_t numBlocks);
extern "C" void FASTCALL AesCtr_Code_Intel(uint32 * ivAes, Byte * data, size_t numBlocks);
// @sobolev EXTERN_C_END

namespace NCrypto {
	static struct CAesTabInit { CAesTabInit() { AesGenTables(); } } g_AesTabInit;

	CAesCbcCoder::CAesCbcCoder(bool encodeMode, unsigned keySize) : _keySize(keySize), _keyIsSet(false), _encodeMode(encodeMode)
	{
		_offset = ((0 - (uint)(ptrdiff_t)_aes) & 0xF) / sizeof(uint32);
		memzero(_iv, AES_BLOCK_SIZE);
		SetFunctions(0);
	}

	STDMETHODIMP CAesCbcCoder::Init()
	{
		AesCbc_Init(_aes + _offset, _iv);
		return _keyIsSet ? S_OK : E_FAIL;
	}

	STDMETHODIMP_(uint32) CAesCbcCoder::Filter(Byte *data, uint32 size)
	{
		if(!_keyIsSet)
			return 0;
		if(size == 0)
			return 0;
		if(size < AES_BLOCK_SIZE)
			return AES_BLOCK_SIZE;
		size >>= 4;
		_codeFunc(_aes + _offset, data, size);
		return size << 4;
	}

	STDMETHODIMP CAesCbcCoder::SetKey(const Byte * data, uint32 size)
	{
		if((size & 0x7) != 0 || size < 16 || size > 32)
			return E_INVALIDARG;
		if(_keySize != 0 && size != _keySize)
			return E_INVALIDARG;
		AES_SET_KEY_FUNC setKeyFunc = _encodeMode ? Aes_SetKey_Enc : Aes_SetKey_Dec;
		setKeyFunc(_aes + _offset + 4, data, size);
		_keyIsSet = true;
		return S_OK;
	}

	STDMETHODIMP CAesCbcCoder::SetInitVector(const Byte * data, uint32 size)
	{
		if(size != AES_BLOCK_SIZE)
			return E_INVALIDARG;
		memcpy(_iv, data, size);
		CAesCbcCoder::Init(); // don't call virtual function here !!!
		return S_OK;
	}

	bool CAesCbcCoder::SetFunctions(uint32 algo)
	{
		_codeFunc = _encodeMode ? g_AesCbc_Encode : g_AesCbc_Decode;
		if(algo == 1) {
			_codeFunc = _encodeMode ? AesCbc_Encode : AesCbc_Decode;
		}
		if(algo == 2) {
		#ifdef MY_CPU_X86_OR_AMD64
			if(g_AesCbc_Encode != AesCbc_Encode_Intel)
		#endif
			return false;
		}
		return true;
	}

	STDMETHODIMP CAesCbcCoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
	{
		for(uint32 i = 0; i < numProps; i++) {
			const PROPVARIANT &prop = coderProps[i];
			if(propIDs[i] == NCoderPropID::kDefaultProp) {
				if(prop.vt != VT_UI4)
					return E_INVALIDARG;
				if(!SetFunctions(prop.ulVal))
					return E_NOTIMPL;
			}
		}
		return S_OK;
	}

	struct CAesCbcEncoder : public CAesCbcCoder {
		CAesCbcEncoder(unsigned keySize = 0) : CAesCbcCoder(true, keySize) 
		{
		}
	};

	namespace NSha1 {
		void Pbkdf2Hmac(const Byte * pwd, size_t pwdSize, const Byte * salt, size_t saltSize, uint32 numIterations, Byte * key, size_t keySize)
		{
			CHmac baseCtx;
			baseCtx.SetKey(pwd, pwdSize);
			for(uint32 i = 1; keySize != 0; i++) {
				CHmac ctx = baseCtx;
				ctx.Update(salt, saltSize);
				Byte u[kDigestSize];
				SetBe32(u, i);
				ctx.Update(u, 4);
				ctx.Final(u, kDigestSize);
				const  uint curSize = (keySize < kDigestSize) ? (uint)keySize : kDigestSize;
				uint   s;
				for(s = 0; s < curSize; s++)
					key[s] = u[s];
				for(uint32 j = numIterations; j > 1; j--) {
					ctx = baseCtx;
					ctx.Update(u, kDigestSize);
					ctx.Final(u, kDigestSize);
					for(s = 0; s < curSize; s++)
						key[s] ^= u[s];
				}

				key += curSize;
				keySize -= curSize;
			}
		}
		void Pbkdf2Hmac32(const Byte * pwd, size_t pwdSize, const uint32 * salt, size_t saltSize,
			uint32 numIterations, uint32 * key, size_t keySize)
		{
			CHmac32 baseCtx;
			baseCtx.SetKey(pwd, pwdSize);
			for(uint32 i = 1; keySize != 0; i++) {
				CHmac32 ctx = baseCtx;
				ctx.Update(salt, saltSize);
				uint32 u[kNumDigestWords];
				u[0] = i;
				ctx.Update(u, 1);
				ctx.Final(u, kNumDigestWords);
				// Speed-optimized code start
				ctx = baseCtx;
				ctx.GetLoopXorDigest(u, numIterations - 1);
				// Speed-optimized code end
				const uint curSize = (keySize < kNumDigestWords) ? (uint)keySize : kNumDigestWords;
				unsigned s;
				for(s = 0; s < curSize; s++)
					key[s] = u[s];
				/*
				   // Default code start
				   for(uint32 j = numIterations; j > 1; j--)
				   {
				   ctx = baseCtx;
				   ctx.Update(u, kNumDigestWords);
				   ctx.Final(u, kNumDigestWords);
				   for(s = 0; s < curSize; s++)
					key[s] ^= u[s];
				   }
				   // Default code end
				 */

				key += curSize;
				keySize -= curSize;
			}
		}
		//
		void CHmac::SetKey(const Byte * key, size_t keySize)
		{
			Byte keyTemp[kBlockSize];
			size_t i;
			for(i = 0; i < kBlockSize; i++)
				keyTemp[i] = 0;
			if(keySize > kBlockSize) {
				_sha.Init();
				_sha.Update(key, keySize);
				_sha.Final(keyTemp);
			}
			else
				for(i = 0; i < keySize; i++)
					keyTemp[i] = key[i];
			for(i = 0; i < kBlockSize; i++)
				keyTemp[i] ^= 0x36;
			_sha.Init();
			_sha.Update(keyTemp, kBlockSize);
			for(i = 0; i < kBlockSize; i++)
				keyTemp[i] ^= 0x36 ^ 0x5C;

			_sha2.Init();
			_sha2.Update(keyTemp, kBlockSize);
		}

		void CHmac::Final(Byte * mac, size_t macSize)
		{
			Byte digest[kDigestSize];
			_sha.Final(digest);
			_sha2.Update(digest, kDigestSize);
			_sha2.Final(digest);
			for(size_t i = 0; i < macSize; i++)
				mac[i] = digest[i];
		}

		void CHmac32::SetKey(const Byte * key, size_t keySize)
		{
			uint32 keyTemp[kNumBlockWords];
			size_t i;
			for(i = 0; i < kNumBlockWords; i++)
				keyTemp[i] = 0;
			if(keySize > kBlockSize) {
				CContext sha;
				sha.Init();
				sha.Update(key, keySize);
				Byte digest[kDigestSize];
				sha.Final(digest);
				for(i = 0; i < kNumDigestWords; i++)
					keyTemp[i] = GetBe32(digest + i * 4 + 0);
			}
			else
				for(i = 0; i < keySize; i++)
					keyTemp[i / 4] |= (key[i] << (24 - 8 * (i & 3)));
			for(i = 0; i < kNumBlockWords; i++)
				keyTemp[i] ^= 0x36363636;
			_sha.Init();
			_sha.Update(keyTemp, kNumBlockWords);
			for(i = 0; i < kNumBlockWords; i++)
				keyTemp[i] ^= 0x36363636 ^ 0x5C5C5C5C;
			_sha2.Init();
			_sha2.Update(keyTemp, kNumBlockWords);
		}

		void CHmac32::Final(uint32 * mac, size_t macSize)
		{
			uint32 digest[kNumDigestWords];
			_sha.Final(digest);
			_sha2.Update(digest, kNumDigestWords);
			_sha2.Final(digest);
			for(size_t i = 0; i < macSize; i++)
				mac[i] = digest[i];
		}

		void CHmac32::GetLoopXorDigest(uint32 * mac, uint32 numIteration)
		{
			uint32 block[kNumBlockWords];
			uint32 block2[kNumBlockWords];
			_sha.PrepareBlock(block, kNumDigestWords);
			_sha2.PrepareBlock(block2, kNumDigestWords);
			for(uint s = 0; s < kNumDigestWords; s++)
				block[s] = mac[s];
			for(uint32 i = 0; i < numIteration; i++) {
				_sha.GetBlockDigest(block, block2);
				_sha2.GetBlockDigest(block2, block);
				for(uint s = 0; s < kNumDigestWords; s++)
					mac[s] ^= block[s];
			}
		}
	}
	namespace NSha256 {
		static const uint kBlockSize = 64;

		void CHmac::SetKey(const Byte * key, size_t keySize)
		{
			Byte temp[kBlockSize];
			size_t i;
			for(i = 0; i < kBlockSize; i++)
				temp[i] = 0;
			if(keySize > kBlockSize) {
				Sha256_Init(&_sha);
				Sha256_Update(&_sha, key, keySize);
				Sha256_Final(&_sha, temp);
			}
			else
				for(i = 0; i < keySize; i++)
					temp[i] = key[i];
			for(i = 0; i < kBlockSize; i++)
				temp[i] ^= 0x36;
			Sha256_Init(&_sha);
			Sha256_Update(&_sha, temp, kBlockSize);
			for(i = 0; i < kBlockSize; i++)
				temp[i] ^= 0x36 ^ 0x5C;
			Sha256_Init(&_sha2);
			Sha256_Update(&_sha2, temp, kBlockSize);
		}

		void CHmac::Final(Byte * mac)
		{
			Sha256_Final(&_sha, mac);
			Sha256_Update(&_sha2, mac, SHA256_DIGEST_SIZE);
			Sha256_Final(&_sha2, mac);
		}
		/*void CHmac::Final(Byte *mac, size_t macSize)
			{
				Byte digest[SHA256_DIGEST_SIZE];
				Final(digest);
				for(size_t i = 0; i < macSize; i++)
					mac[i] = digest[i];
			}*/
	}
	// 
	// This code implements Brian Gladman's scheme
	// specified in "A Password Based File Encryption Utility".
	// 
	// Note: you must include MyAes.cpp to project to initialize AES tables
	// 
	// define it if you don't want to use speed-optimized version of NSha1::Pbkdf2Hmac
	// #define _NO_WZAES_OPTIMIZATIONS
	//
	namespace NWzAes {
		const uint kAesKeySizeMax = 32;

		static const uint32 kNumKeyGenIterations = 1000;

		CBaseCoder::~CBaseCoder() 
		{
		}

		unsigned CBaseCoder::GetHeaderSize() const { return _key.GetSaltSize() + kPwdVerifSize; }
		unsigned CBaseCoder::GetAddPackSize() const { return GetHeaderSize() + kMacSize; }

		bool CBaseCoder::SetKeyMode(unsigned mode)
		{
			if(mode < kKeySizeMode_AES128 || mode > kKeySizeMode_AES256)
				return false;
			_key.KeySizeMode = (EKeySizeMode)mode;
			return true;
		}

		STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte * data, uint32 size)
		{
			if(size > kPasswordSizeMax)
				return E_INVALIDARG;
			_key.Password.CopyFrom(data, (size_t)size);
			return S_OK;
		}

		void CBaseCoder::Init2()
		{
			const uint dkSizeMax32 = (2 * kAesKeySizeMax + kPwdVerifSize + 3) / 4;
			Byte dk[dkSizeMax32 * 4];
			const uint keySize = _key.GetKeySize();
			const uint dkSize = 2 * keySize + kPwdVerifSize;
			// for(unsigned ii = 0; ii < 1000; ii++)
			{
			#ifdef _NO_WZAES_OPTIMIZATIONS
				NSha1::Pbkdf2Hmac(_key.Password, _key.Password.Size(), _key.Salt, _key.GetSaltSize(), kNumKeyGenIterations, dk, dkSize);
			#else
				uint32 dk32[dkSizeMax32];
				const  uint dkSize32 = (dkSize + 3) / 4;
				uint32 salt[kSaltSizeMax / 4];
				uint   numSaltWords = _key.GetNumSaltWords();
				for(uint i = 0; i < numSaltWords; i++) {
					const Byte * src = _key.Salt + i * 4;
					salt[i] = GetBe32(src);
				}
				NSha1::Pbkdf2Hmac32(_key.Password, _key.Password.Size(), salt, numSaltWords, kNumKeyGenIterations, dk32, dkSize32);
				/*
				   for(unsigned j = 0; j < dkSize; j++)
				   dk[j] = (Byte)(dk32[j / 4] >> (24 - 8 * (j & 3)));
				 */
				for(uint j = 0; j < dkSize32; j++)
					SetBe32(dk + j * 4, dk32[j]);
			#endif
			}
			_hmac.SetKey(dk + keySize, keySize);
			memcpy(_key.PwdVerifComputed, dk + 2 * keySize, kPwdVerifSize);
			Aes_SetKey_Enc(_aes.aes + _aes.offset + 8, dk, keySize);
			AesCtr2_Init(&_aes);
		}

		STDMETHODIMP CBaseCoder::Init()
		{
			return S_OK;
		}

		HRESULT CEncoder::WriteHeader(ISequentialOutStream * outStream)
		{
			unsigned saltSize = _key.GetSaltSize();
			g_RandomGenerator.Generate(_key.Salt, saltSize);
			Init2();
			RINOK(WriteStream(outStream, _key.Salt, saltSize));
			return WriteStream(outStream, _key.PwdVerifComputed, kPwdVerifSize);
		}

		HRESULT CEncoder::WriteFooter(ISequentialOutStream * outStream)
		{
			Byte mac[kMacSize];
			_hmac.Final(mac, kMacSize);
			return WriteStream(outStream, mac, kMacSize);
		}

		/*
		   STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, uint32 size)
		   {
		   if(size != 1)
			return E_INVALIDARG;
		   _key.Init();
		   return SetKeyMode(data[0]) ? S_OK : E_INVALIDARG;
		   }
		 */

		HRESULT CDecoder::ReadHeader(ISequentialInStream * inStream)
		{
			uint   saltSize = _key.GetSaltSize();
			uint   extraSize = saltSize + kPwdVerifSize;
			Byte   temp[kSaltSizeMax + kPwdVerifSize];
			RINOK(ReadStream_FAIL(inStream, temp, extraSize));
			uint   i;
			for(i = 0; i < saltSize; i++)
				_key.Salt[i] = temp[i];
			for(i = 0; i < kPwdVerifSize; i++)
				_pwdVerifFromArchive[i] = temp[saltSize + i];
			return S_OK;
		}

		static inline bool CompareArrays(const Byte * p1, const Byte * p2, uint size)
		{
			for(uint i = 0; i < size; i++)
				if(p1[i] != p2[i])
					return false;
			return true;
		}
		bool CDecoder::Init_and_CheckPassword()
		{
			Init2();
			return CompareArrays(_key.PwdVerifComputed, _pwdVerifFromArchive, kPwdVerifSize);
		}
		HRESULT CDecoder::CheckMac(ISequentialInStream * inStream, bool &isOK)
		{
			isOK = false;
			Byte mac1[kMacSize];
			RINOK(ReadStream_FAIL(inStream, mac1, kMacSize));
			Byte mac2[kMacSize];
			_hmac.Final(mac2, kMacSize);
			isOK = CompareArrays(mac1, mac2, kMacSize);
			return S_OK;
		}
		CAesCtr2::CAesCtr2()
		{
			offset = ((0 - (uint)(ptrdiff_t)aes) & 0xF) / sizeof(uint32);
		}

		void AesCtr2_Init(CAesCtr2 * p)
		{
			uint32 * ctr = p->aes + p->offset + 4;
			for(uint i = 0; i < 4; i++)
				ctr[i] = 0;
			p->pos = AES_BLOCK_SIZE;
		}

		/* (size != 16 * N) is allowed only for last call */

		void AesCtr2_Code(CAesCtr2 * p, Byte * data, SizeT size)
		{
			uint   pos = p->pos;
			uint32 * buf32 = p->aes + p->offset;
			if(size == 0)
				return;
			if(pos != AES_BLOCK_SIZE) {
				const Byte * buf = (const Byte*)buf32;
				do {
					*data++ ^= buf[pos++];
				} while(--size != 0 && pos != AES_BLOCK_SIZE);
			}
			if(size >= 16) {
				SizeT size2 = size >> 4;
				g_AesCtr_Code(buf32 + 4, data, size2);
				size2 <<= 4;
				data += size2;
				size -= size2;
				pos = AES_BLOCK_SIZE;
			}
			if(size != 0) {
				const Byte * buf;
				for(uint j = 0; j < 4; j++)
					buf32[j] = 0;
				g_AesCtr_Code(buf32 + 4, (Byte *)buf32, 1);
				buf = (const Byte*)buf32;
				pos = 0;
				do {
					*data++ ^= buf[pos++];
				} while(--size != 0);
			}
			p->pos = pos;
		}

		/* (size != 16 * N) is allowed only for last Filter() call */

		STDMETHODIMP_(uint32) CEncoder::Filter(Byte *data, uint32 size)
		{
			AesCtr2_Code(&_aes, data, size);
			_hmac.Update(data, size);
			return size;
		}

		STDMETHODIMP_(uint32) CDecoder::Filter(Byte *data, uint32 size)
		{
			_hmac.Update(data, size);
			AesCtr2_Code(&_aes, data, size);
			return size;
		}
	}
	//
	namespace N7z {
		static const uint k_NumCyclesPower_Supported_MAX = 24;

		CBase::CKeyInfo::CKeyInfo() 
		{ 
			ClearProps(); 
		}
		void CBase::CKeyInfo::ClearProps()
		{
			NumCyclesPower = 0;
			SaltSize = 0;
			for(uint i = 0; i < sizeof(Salt); i++)
				Salt[i] = 0;
		}
		bool CBase::CKeyInfo::IsEqualTo(const CKeyInfo &a) const
		{
			if(SaltSize != a.SaltSize || NumCyclesPower != a.NumCyclesPower)
				return false;
			for(uint i = 0; i < SaltSize; i++)
				if(Salt[i] != a.Salt[i])
					return false;
			return (Password == a.Password);
		}
		void CBase::CKeyInfo::CalcKey()
		{
			if(NumCyclesPower == 0x3F) {
				uint   pos;
				for(pos = 0; pos < SaltSize; pos++)
					Key[pos] = Salt[pos];
				for(uint i = 0; i < Password.Size() && pos < kKeySize; i++)
					Key[pos++] = Password[i];
				for(; pos < kKeySize; pos++)
					Key[pos] = 0;
			}
			else {
				size_t bufSize = 8 + SaltSize + Password.Size();
				CObjArray <Byte> buf(bufSize);
				memcpy(buf, Salt, SaltSize);
				memcpy(buf + SaltSize, Password, Password.Size());
				CSha256 sha;
				Sha256_Init(&sha);
				Byte * ctr = buf + SaltSize + Password.Size();
				for(uint i = 0; i < 8; i++)
					ctr[i] = 0;
				uint64 numRounds = (uint64)1 << NumCyclesPower;
				do {
					Sha256_Update(&sha, buf, bufSize);
					for(uint i = 0; i < 8; i++)
						if(++(ctr[i]) != 0)
							break;
				} while(--numRounds != 0);
				Sha256_Final(&sha, Key);
			}
		}
		bool CBase::CKeyInfoCache::GetKey(CKeyInfo &key)
		{
			FOR_VECTOR(i, Keys) {
				const CKeyInfo &cached = Keys[i];
				if(key.IsEqualTo(cached)) {
					for(uint j = 0; j < kKeySize; j++)
						key.Key[j] = cached.Key[j];
					if(i != 0)
						Keys.MoveToFront(i);
					return true;
				}
			}
			return false;
		}
		void CBase::CKeyInfoCache::FindAndAdd(const CKeyInfo &key)
		{
			FOR_VECTOR(i, Keys) {
				const CKeyInfo &cached = Keys[i];
				if(key.IsEqualTo(cached)) {
					if(i != 0)
						Keys.MoveToFront(i);
					return;
				}
			}
			Add(key);
		}
		void CBase::CKeyInfoCache::Add(const CKeyInfo &key)
		{
			if(Keys.Size() >= Size)
				Keys.DeleteBack();
			Keys.Insert(0, key);
		}

		static CBase::CKeyInfoCache g_GlobalKeyCache(32);

		#ifndef _7ZIP_ST
		static NWindows::NSynchronization::CCriticalSection g_GlobalKeyCacheCriticalSection;
		  #define MT_LOCK NWindows::NSynchronization::CCriticalSectionLock lock(g_GlobalKeyCacheCriticalSection);
		#else
		  #define MT_LOCK
		#endif

		CBase::CBase() : _cachedKeys(16), _ivSize(0)
		{
			/*for(uint i = 0; i < sizeof(_iv); i++)
				_iv[i] = 0;*/
			memzero(_iv, sizeof(_iv));
		}

		void CBase::PrepareKey()
		{
			// BCJ2 threads use same password. So we use long lock.
			MT_LOCK
			bool finded = false;
			if(!_cachedKeys.GetKey(_key)) {
				finded = g_GlobalKeyCache.GetKey(_key);
				if(!finded)
					_key.CalcKey();
				_cachedKeys.Add(_key);
			}
			if(!finded)
				g_GlobalKeyCache.FindAndAdd(_key);
		}

		#ifndef EXTRACT_ONLY

		/*
		   STDMETHODIMP CEncoder::ResetSalt()
		   {
		   _key.SaltSize = 4;
		   g_RandomGenerator.Generate(_key.Salt, _key.SaltSize);
		   return S_OK;
		   }
		 */

		STDMETHODIMP CEncoder::ResetInitVector()
		{
			/*for(uint i = 0; i < sizeof(_iv); i++)
				_iv[i] = 0;*/
			memzero(_iv, sizeof(_iv));
			_ivSize = 8;
			g_RandomGenerator.Generate(_iv, _ivSize);
			return S_OK;
		}

		STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
		{
			Byte   props[2 + sizeof(_key.Salt) + sizeof(_iv)];
			uint   propsSize = 1;
			props[0] = (Byte)(_key.NumCyclesPower | (_key.SaltSize == 0 ? 0 : (1 << 7)) | (_ivSize == 0 ? 0 : (1 << 6)));
			if(_key.SaltSize != 0 || _ivSize != 0) {
				props[1] = (Byte)(((_key.SaltSize == 0 ? 0 : _key.SaltSize - 1) << 4) | (_ivSize == 0 ? 0 : _ivSize - 1));
				memcpy(props + 2, _key.Salt, _key.SaltSize);
				propsSize = 2 + _key.SaltSize;
				memcpy(props + propsSize, _iv, _ivSize);
				propsSize += _ivSize;
			}
			return WriteStream(outStream, props, propsSize);
		}

		CEncoder::CEncoder()
		{
			// _key.SaltSize = 4; g_RandomGenerator.Generate(_key.Salt, _key.SaltSize);
			// _key.NumCyclesPower = 0x3F;
			_key.NumCyclesPower = 19;
			_aesFilter = new CAesCbcEncoder(kKeySize);
		}

		#endif

		CDecoder::CDecoder()
		{
			_aesFilter = new CAesCbcDecoder(kKeySize);
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * data, uint32 size)
		{
			_key.ClearProps();
			_ivSize = 0;
			uint   i;
			/*for(i = 0; i < sizeof(_iv); i++)
				_iv[i] = 0;*/
			memzero(_iv, sizeof(_iv));
			if(size == 0)
				return S_OK;
			Byte b0 = data[0];
			_key.NumCyclesPower = b0 & 0x3F;
			if((b0 & 0xC0) == 0)
				return size == 1 ? S_OK : E_INVALIDARG;
			if(size <= 1)
				return E_INVALIDARG;
			Byte b1 = data[1];
			unsigned saltSize = ((b0 >> 7) & 1) + (b1 >> 4);
			unsigned ivSize   = ((b0 >> 6) & 1) + (b1 & 0x0F);
			if(size != 2 + saltSize + ivSize)
				return E_INVALIDARG;
			_key.SaltSize = saltSize;
			data += 2;
			for(i = 0; i < saltSize; i++)
				_key.Salt[i] = *data++;
			for(i = 0; i < ivSize; i++)
				_iv[i] = *data++;
			return (_key.NumCyclesPower <= k_NumCyclesPower_Supported_MAX || _key.NumCyclesPower == 0x3F) ? S_OK : E_NOTIMPL;
		}

		STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte * data, uint32 size)
		{
			COM_TRY_BEGIN
			_key.Password.CopyFrom(data, (size_t)size);
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CBaseCoder::Init()
		{
			COM_TRY_BEGIN
			PrepareKey();
			CMyComPtr<ICryptoProperties> cp;
			RINOK(_aesFilter.QueryInterface(IID_ICryptoProperties, &cp));
			if(!cp)
				return E_FAIL;
			RINOK(cp->SetKey(_key.Key, kKeySize));
			RINOK(cp->SetInitVector(_iv, sizeof(_iv)));
			return _aesFilter->Init();
			COM_TRY_END
		}

		STDMETHODIMP_(uint32) CBaseCoder::Filter(Byte *data, uint32 size)
		{
			return _aesFilter->Filter(data, size);
		}
		// 7zAesRegister
		REGISTER_FILTER_E(7zAES, CDecoder(), CEncoder(), 0x6F10701, "7zAES")
	}
	namespace NRar3 {
		CDecoder::CDecoder() : CAesCbcDecoder(kAesKeySize), _thereIsSalt(false), _needCalc(true) /*_rar350Mode(false)*/
		{
			/*for(uint i = 0; i < sizeof(_salt); i++)
				_salt[i] = 0;*/
			memzero(_salt, sizeof(_salt));
		}
		HRESULT CDecoder::SetDecoderProperties2(const Byte * data, uint32 size)
		{
			bool prev = _thereIsSalt;
			_thereIsSalt = false;
			if(size == 0) {
				if(!_needCalc && prev)
					_needCalc = true;
				return S_OK;
			}
			else if(size < 8)
				return E_INVALIDARG;
			else {
				_thereIsSalt = true;
				bool same = false;
				if(_thereIsSalt == prev) {
					same = true;
					if(_thereIsSalt) {
						for(uint i = 0; i < sizeof(_salt); i++)
							if(_salt[i] != data[i]) {
								same = false;
								break;
							}
					}
				}
				/*for(uint i = 0; i < sizeof(_salt); i++)
					_salt[i] = data[i];*/
				memcpy(_salt, data, sizeof(_salt));
				if(!_needCalc && !same)
					_needCalc = true;
				return S_OK;
			}
		}

		static const uint kPasswordLen_Bytes_MAX = 127 * 2;

		void CDecoder::SetPassword(const Byte * data, uint size)
		{
			if(size > kPasswordLen_Bytes_MAX)
				size = kPasswordLen_Bytes_MAX;
			bool same = false;
			if(size == _password.Size()) {
				same = true;
				for(uint32 i = 0; i < size; i++)
					if(data[i] != _password[i]) {
						same = false;
						break;
					}
			}
			if(!_needCalc && !same)
				_needCalc = true;
			_password.CopyFrom(data, (size_t)size);
		}

		STDMETHODIMP CDecoder::Init()
		{
			CalcKey();
			RINOK(SetKey(_key, kAesKeySize));
			RINOK(SetInitVector(_iv, AES_BLOCK_SIZE));
			return CAesCbcCoder::Init();
		}

		void CDecoder::CalcKey()
		{
			if(!_needCalc)
				return;
			const uint kSaltSize = 8;
			Byte buf[kPasswordLen_Bytes_MAX + kSaltSize];
			if(_password.Size() != 0)
				memcpy(buf, _password, _password.Size());
			size_t rawSize = _password.Size();
			if(_thereIsSalt) {
				memcpy(buf + rawSize, _salt, kSaltSize);
				rawSize += kSaltSize;
			}
			NSha1::CContext sha;
			sha.Init();
			Byte digest[NSha1::kDigestSize];
			// rar reverts hash for sha.
			const uint32 kNumRounds = ((uint32)1 << 18);
			uint32 i;
			for(i = 0; i < kNumRounds; i++) {
				sha.UpdateRar(buf, rawSize /* , _rar350Mode */);
				Byte pswNum[3] = { (Byte)i, (Byte)(i >> 8), (Byte)(i >> 16) };
				sha.UpdateRar(pswNum, 3 /* , _rar350Mode */);
				if(i % (kNumRounds / 16) == 0) {
					NSha1::CContext shaTemp = sha;
					shaTemp.Final(digest);
					_iv[i / (kNumRounds / 16)] = (Byte)digest[4 * 4 + 3];
				}
			}
			sha.Final(digest);
			for(i = 0; i < 4; i++)
				for(uint j = 0; j < 4; j++)
					_key[i * 4 + j] = (digest[i * 4 + 3 - j]);
			_needCalc = false;
		}
	}
	namespace NRar2 {
		static const uint kNumRounds = 32;

		static const Byte g_InitSubstTable[256] = {
			215, 19, 149, 35, 73, 197, 192, 205, 249, 28, 16, 119, 48, 221,  2, 42,
			232,  1, 177, 233, 14, 88, 219, 25, 223, 195, 244, 90, 87, 239, 153, 137,
			255, 199, 147, 70, 92, 66, 246, 13, 216, 40, 62, 29, 217, 230, 86,  6,
			71, 24, 171, 196, 101, 113, 218, 123, 93, 91, 163, 178, 202, 67, 44, 235,
			107, 250, 75, 234, 49, 167, 125, 211, 83, 114, 157, 144, 32, 193, 143, 36,
			158, 124, 247, 187, 89, 214, 141, 47, 121, 228, 61, 130, 213, 194, 174, 251,
			97, 110, 54, 229, 115, 57, 152, 94, 105, 243, 212, 55, 209, 245, 63, 11,
			164, 200, 31, 156, 81, 176, 227, 21, 76, 99, 139, 188, 127, 17, 248, 51,
			207, 120, 189, 210,  8, 226, 41, 72, 183, 203, 135, 165, 166, 60, 98,  7,
			122, 38, 155, 170, 69, 172, 252, 238, 39, 134, 59, 128, 236, 27, 240, 80,
			131,  3, 85, 206, 145, 79, 154, 142, 159, 220, 201, 133, 74, 64, 20, 129,
			224, 185, 138, 103, 173, 182, 43, 34, 254, 82, 198, 151, 231, 180, 58, 10,
			118, 26, 102, 12, 50, 132, 22, 191, 136, 111, 162, 179, 45,  4, 148, 108,
			161, 56, 78, 126, 242, 222, 15, 175, 146, 23, 33, 241, 181, 190, 77, 225,
			0, 46, 169, 186, 68, 95, 237, 65, 53, 208, 253, 168,  9, 18, 100, 52,
			116, 184, 160, 96, 109, 37, 30, 106, 140, 104, 150,  5, 204, 117, 112, 84
		};

		uint32 FASTCALL CData::SubstLong(uint32 t) const
		{
			return (uint32)SubstTable[(uint)t & 255] | ((uint32)SubstTable[(uint)(t >> 8) & 255] << 8) | 
				((uint32)SubstTable[(uint)(t >> 16) & 255] << 16) | ((uint32)SubstTable[(uint)(t >> 24)] << 24);
		}

		void CData::UpdateKeys(const Byte * data)
		{
			for(uint i = 0; i < 16; i += 4)
				for(uint j = 0; j < 4; j++)
					Keys[j] ^= g_CrcTable[data[i + j]];
		}

		static inline void Swap(Byte &b1, Byte &b2)
		{
			Byte b = b1;
			b1 = b2;
			b2 = b;
		}

		void CData::SetPassword(const Byte * data, uint size)
		{
			Keys[0] = 0xD3A3B879L;
			Keys[1] = 0x3F6D12F7L;
			Keys[2] = 0x7515A235L;
			Keys[3] = 0xA4E7F123L;

			Byte psw[128];
			memzero(psw, sizeof(psw));
			if(size != 0) {
				if(size >= sizeof(psw))
					size = sizeof(psw) - 1;
				memcpy(psw, data, size);
			}
			memcpy(SubstTable, g_InitSubstTable, sizeof(SubstTable));
			for(uint j = 0; j < 256; j++)
				for(uint i = 0; i < size; i += 2) {
					unsigned n1 = (Byte)g_CrcTable[(psw[i] - j) & 0xFF];
					unsigned n2 = (Byte)g_CrcTable[(psw[(size_t)i + 1] + j) & 0xFF];
					for(uint k = 1; (n1 & 0xFF) != n2; n1++, k++)
						Swap(SubstTable[n1 & 0xFF], SubstTable[(n1 + i + k) & 0xFF]);
				}
			for(uint i = 0; i < size; i += 16)
				EncryptBlock(psw + i);
		}

		void CData::CryptBlock(Byte * buf, bool encrypt)
		{
			Byte inBuf[16];
			uint32 A, B, C, D;

			A = GetUi32(buf +  0) ^ Keys[0];
			B = GetUi32(buf +  4) ^ Keys[1];
			C = GetUi32(buf +  8) ^ Keys[2];
			D = GetUi32(buf + 12) ^ Keys[3];

			if(!encrypt)
				memcpy(inBuf, buf, sizeof(inBuf));

			for(uint i = 0; i < kNumRounds; i++) {
				uint32 key = Keys[(encrypt ? i : (kNumRounds - 1 - i)) & 3];
				uint32 TA = A ^ SubstLong((C + rotlFixed(D, 11)) ^ key);
				uint32 TB = B ^ SubstLong((D ^ rotlFixed(C, 17)) + key);
				A = C; C = TA;
				B = D; D = TB;
			}

			SetUi32(buf +  0, C ^ Keys[0]);
			SetUi32(buf +  4, D ^ Keys[1]);
			SetUi32(buf +  8, A ^ Keys[2]);
			SetUi32(buf + 12, B ^ Keys[3]);

			UpdateKeys(encrypt ? buf : inBuf);
		}

		STDMETHODIMP CDecoder::Init()
		{
			return S_OK;
		}

		static const uint32 kBlockSize = 16;

		STDMETHODIMP_(uint32) CDecoder::Filter(Byte *data, uint32 size)
		{
			if(size == 0)
				return 0;
			if(size < kBlockSize)
				return kBlockSize;
			size -= kBlockSize;
			uint32 i;
			for(i = 0; i <= size; i += kBlockSize)
				DecryptBlock(data + i);
			return i;
		}
	}
	namespace NRar5 {
		static const uint kNumIterationsLog_Max = 24;
		static const uint kPswCheckCsumSize = 4;
		static const uint kCheckSize = kPswCheckSize + kPswCheckCsumSize;

		CKey::CKey() : _needCalc(true), _numIterationsLog(0)
		{
			/*for(uint i = 0; i < sizeof(_salt); i++)
				_salt[i] = 0;*/
			memzero(_salt, sizeof(_salt));
		}

		void FASTCALL CKey::CopyCalcedKeysFrom(const CKey &k)
		{
			memcpy(_key, k._key, sizeof(_key));
			memcpy(_check_Calced, k._check_Calced, sizeof(_check_Calced));
			memcpy(_hashKey, k._hashKey, sizeof(_hashKey));
		}

		bool FASTCALL CKey::IsKeyEqualTo(const CKey &key)
		{
			return (_numIterationsLog == key._numIterationsLog && memcmp(_salt, key._salt, sizeof(_salt)) == 0 && _password == key._password);
		}

		CDecoder::CDecoder() : CAesCbcDecoder(kAesKeySize) 
		{
		}

		static uint FASTCALL ReadVarInt(const Byte * p, uint maxSize, uint64 * val)
		{
			*val = 0;
			for(uint i = 0; i < maxSize; ) {
				Byte b = p[i];
				if(i < 10)
					*val |= (uint64)(b & 0x7F) << (7 * i++);
				if((b & 0x80) == 0)
					return i;
			}
			return 0;
		}

		HRESULT CDecoder::SetDecoderProps(const Byte * p, uint size, bool includeIV, bool isService)
		{
			uint64 Version;
			unsigned num = ReadVarInt(p, size, &Version);
			if(num == 0)
				return E_NOTIMPL;
			p += num;
			size -= num;
			if(Version != 0)
				return E_NOTIMPL;
			num = ReadVarInt(p, size, &Flags);
			if(num == 0)
				return E_NOTIMPL;
			p += num;
			size -= num;
			bool isCheck = IsThereCheck();
			if(size != 1 + kSaltSize + (includeIV ? AES_BLOCK_SIZE : 0) + (uint)(isCheck ? kCheckSize : 0))
				return E_NOTIMPL;
			if(_numIterationsLog != p[0]) {
				_numIterationsLog = p[0];
				_needCalc = true;
			}
			p++;
			if(memcmp(_salt, p, kSaltSize) != 0) {
				memcpy(_salt, p, kSaltSize);
				_needCalc = true;
			}
			p += kSaltSize;
			if(includeIV) {
				memcpy(_iv, p, AES_BLOCK_SIZE);
				p += AES_BLOCK_SIZE;
			}
			_canCheck = true;
			if(isCheck) {
				memcpy(_check, p, kPswCheckSize);
				CSha256 sha;
				Byte digest[SHA256_DIGEST_SIZE];
				Sha256_Init(&sha);
				Sha256_Update(&sha, _check, kPswCheckSize);
				Sha256_Final(&sha, digest);
				_canCheck = (memcmp(digest, p + kPswCheckSize, kPswCheckCsumSize) == 0);
				if(_canCheck && isService) {
					// There was bug in RAR 5.21- : PswCheck field in service records ("QO") contained zeros.
					// so we disable password checking for such bad records.
					_canCheck = false;
					for(uint i = 0; i < kPswCheckSize; i++)
						if(p[i] != 0) {
							_canCheck = true;
							break;
						}
				}
			}

			return (_numIterationsLog <= kNumIterationsLog_Max ? S_OK : E_NOTIMPL);
		}

		void CDecoder::SetPassword(const Byte * data, size_t size)
		{
			if(size != _password.Size() || memcmp(data, _password, size) != 0) {
				_needCalc = true;
				_password.CopyFrom(data, size);
			}
		}

		STDMETHODIMP CDecoder::Init()
		{
			CalcKey_and_CheckPassword();
			RINOK(SetKey(_key, kAesKeySize));
			RINOK(SetInitVector(_iv, AES_BLOCK_SIZE));
			return CAesCbcCoder::Init();
		}

		uint32 CDecoder::Hmac_Convert_Crc32(uint32 crc) const
		{
			NSha256::CHmac ctx;
			ctx.SetKey(_hashKey, NSha256::kDigestSize);
			Byte v[4];
			SetUi32(v, crc);
			ctx.Update(v, 4);
			Byte h[NSha256::kDigestSize];
			ctx.Final(h);
			crc = 0;
			for(uint i = 0; i < NSha256::kDigestSize; i++)
				crc ^= (uint32)h[i] << ((i & 3) * 8);
			return crc;
		};

		void CDecoder::Hmac_Convert_32Bytes(Byte * data) const
		{
			NSha256::CHmac ctx;
			ctx.SetKey(_hashKey, NSha256::kDigestSize);
			ctx.Update(data, NSha256::kDigestSize);
			ctx.Final(data);
		};

		#ifndef _7ZIP_ST
		static CKey g_Key;
		static NWindows::NSynchronization::CCriticalSection g_GlobalKeyCacheCriticalSection;
		  #define MT_LOCK NWindows::NSynchronization::CCriticalSectionLock lock(g_GlobalKeyCacheCriticalSection);
		#else
		  #define MT_LOCK
		#endif

		bool CDecoder::CalcKey_and_CheckPassword()
		{
			if(_needCalc) {
				{
					MT_LOCK
					if(!g_Key._needCalc && IsKeyEqualTo(g_Key)) {
						CopyCalcedKeysFrom(g_Key);
						_needCalc = false;
					}
				}
				if(_needCalc) {
					Byte pswCheck[SHA256_DIGEST_SIZE];
					{
						// Pbkdf HMAC-SHA-256
						NSha256::CHmac baseCtx;
						baseCtx.SetKey(_password, _password.Size());
						NSha256::CHmac ctx = baseCtx;
						ctx.Update(_salt, sizeof(_salt));
						Byte u[NSha256::kDigestSize];
						Byte key[NSha256::kDigestSize];
						u[0] = 0;
						u[1] = 0;
						u[2] = 0;
						u[3] = 1;

						ctx.Update(u, 4);
						ctx.Final(u);
						memcpy(key, u, NSha256::kDigestSize);
						uint32 numIterations = ((uint32)1 << _numIterationsLog) - 1;
						for(uint i = 0; i < 3; i++) {
							uint32 j = numIterations;
							for(; j != 0; j--) {
								ctx = baseCtx;
								ctx.Update(u, NSha256::kDigestSize);
								ctx.Final(u);
								for(uint s = 0; s < NSha256::kDigestSize; s++)
									key[s] ^= u[s];
							}

							// RAR uses additional iterations for additional keys
							memcpy((i == 0 ? _key : (i == 1 ? _hashKey : pswCheck)), key, NSha256::kDigestSize);
							numIterations = 16;
						}
					}
					{
						uint i;
						for(i = 0; i < kPswCheckSize; i++)
							_check_Calced[i] = pswCheck[i];
						for(i = kPswCheckSize; i < SHA256_DIGEST_SIZE; i++)
							_check_Calced[i & (kPswCheckSize - 1)] ^= pswCheck[i];
					}
					_needCalc = false;
					{
						MT_LOCK
							g_Key = *this;
					}
				}
			}
			return (IsThereCheck() && _canCheck) ? (memcmp(_check_Calced, _check, kPswCheckSize) == 0) : true;
		}
	}
	namespace NZip {
		#define UPDATE_KEYS(b) { \
				key0 = CRC_UPDATE_BYTE(key0, b); \
				key1 = (key1 + (key0 & 0xFF)) * 0x8088405 + 1; \
				key2 = CRC_UPDATE_BYTE(key2, (Byte)(key1 >> 24)); \
		} \

		#define DECRYPT_BYTE_1 uint32 temp = key2 | 2;
		#define DECRYPT_BYTE_2 ((Byte)((temp * (temp ^ 1)) >> 8))

		CCipher::~CCipher() 
		{
		}
		void CCipher::RestoreKeys()
		{
			Key0 = KeyMem0;
			Key1 = KeyMem1;
			Key2 = KeyMem2;
		}
		STDMETHODIMP CCipher::CryptoSetPassword(const Byte * data, uint32 size)
		{
			uint32 key0 = 0x12345678;
			uint32 key1 = 0x23456789;
			uint32 key2 = 0x34567890;
			for(uint32 i = 0; i < size; i++)
				UPDATE_KEYS(data[i]);
			KeyMem0 = key0;
			KeyMem1 = key1;
			KeyMem2 = key2;
			return S_OK;
		}
		STDMETHODIMP CCipher::Init()
		{
			return S_OK;
		}
		HRESULT CEncoder::WriteHeader_Check16(ISequentialOutStream * outStream, uint16 crc)
		{
			Byte h[kHeaderSize];
			/* PKZIP before 2.0 used 2 byte CRC check.
			   PKZIP 2.0+ used 1 byte CRC check. It's more secure.
			   We also use 1 byte CRC. */
			g_RandomGenerator.Generate(h, kHeaderSize - 1);
			// h[kHeaderSize - 2] = (Byte)(crc);
			h[kHeaderSize - 1] = (Byte)(crc >> 8);
			RestoreKeys();
			Filter(h, kHeaderSize);
			return WriteStream(outStream, h, kHeaderSize);
		}
		STDMETHODIMP_(uint32) CEncoder::Filter(Byte *data, uint32 size)
		{
			uint32 key0 = this->Key0;
			uint32 key1 = this->Key1;
			uint32 key2 = this->Key2;
			for(uint32 i = 0; i < size; i++) {
				Byte b = data[i];
				DECRYPT_BYTE_1
					data[i] = (Byte)(b ^ DECRYPT_BYTE_2);
				UPDATE_KEYS(b);
			}
			this->Key0 = key0;
			this->Key1 = key1;
			this->Key2 = key2;
			return size;
		}

		HRESULT CDecoder::ReadHeader(ISequentialInStream * inStream)
		{
			return ReadStream_FAIL(inStream, _header, kHeaderSize);
		}

		void CDecoder::Init_BeforeDecode()
		{
			RestoreKeys();
			Filter(_header, kHeaderSize);
		}

		STDMETHODIMP_(uint32) CDecoder::Filter(Byte *data, uint32 size)
		{
			uint32 key0 = this->Key0;
			uint32 key1 = this->Key1;
			uint32 key2 = this->Key2;
			for(uint32 i = 0; i < size; i++) {
				DECRYPT_BYTE_1
				Byte b = (Byte)(data[i] ^ DECRYPT_BYTE_2);
				UPDATE_KEYS(b);
				data[i] = b;
			}
			this->Key0 = key0;
			this->Key1 = key1;
			this->Key2 = key2;
			return size;
		}
	}
	namespace NZipStrong {
		static const uint16 kAES128 = 0x660E;
		/*
		   DeriveKey() function is similar to CryptDeriveKey() from Windows.
		   New version of MSDN contains the following condition in CryptDeriveKey() description:
			"If the hash is not a member of the SHA-2 family and the required key is for either 3DES or AES".
		   Now we support ZipStrong for AES only. And it uses SHA1.
		   Our DeriveKey() code is equal to CryptDeriveKey() in Windows for such conditions: (SHA1 + AES).
		   if(method != AES && method != 3DES), probably we need another code.
		 */
		static void DeriveKey2(const Byte * digest, Byte c, Byte * dest)
		{
			Byte buf[64];
			memset(buf, c, 64);
			for(uint i = 0; i < NSha1::kDigestSize; i++)
				buf[i] ^= digest[i];
			NSha1::CContext sha;
			sha.Init();
			sha.Update(buf, 64);
			sha.Final(dest);
		}

		static void DeriveKey(NSha1::CContext &sha, Byte * key)
		{
			Byte digest[NSha1::kDigestSize];
			sha.Final(digest);
			Byte temp[NSha1::kDigestSize * 2];
			DeriveKey2(digest, 0x36, temp);
			DeriveKey2(digest, 0x5C, temp + NSha1::kDigestSize);
			memcpy(key, temp, 32);
		}

		void CBaseCoder::CKeyInfo::SetPassword(const Byte * data, uint32 size)
		{
			NSha1::CContext sha;
			sha.Init();
			sha.Update(data, size);
			DeriveKey(sha, MasterKey);
		}

		STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte * data, uint32 size)
		{
			_key.SetPassword(data, size);
			return S_OK;
		}

		STDMETHODIMP CBaseCoder::Init()
		{
			return S_OK;
		}

		HRESULT CDecoder::ReadHeader(ISequentialInStream * inStream, uint32 crc, uint64 unpackSize)
		{
			Byte temp[4];
			RINOK(ReadStream_FALSE(inStream, temp, 2));
			_ivSize = GetUi16(temp);
			if(_ivSize == 0) {
				memzero(_iv, 16);
				SetUi32(_iv + 0, crc);
				SetUi64(_iv + 4, unpackSize);
				_ivSize = 12;
			}
			else if(_ivSize == 16) {
				RINOK(ReadStream_FALSE(inStream, _iv, _ivSize));
			}
			else
				return E_NOTIMPL;
			RINOK(ReadStream_FALSE(inStream, temp, 4));
			_remSize = GetUi32(temp);
			const uint32 kAlign = 16;
			if(_remSize < 16 || _remSize > (1 << 18))
				return E_NOTIMPL;
			if(_remSize + kAlign > _buf.Size()) {
				_buf.Alloc(_remSize + kAlign);
				_bufAligned = (Byte *)((ptrdiff_t)((Byte *)_buf + kAlign - 1) & ~(ptrdiff_t)(kAlign - 1));
			}
			return ReadStream_FALSE(inStream, _bufAligned, _remSize);
		}

		uint32 FASTCALL CDecoder::GetPadSize(uint32 packSize32) const
		{
			// Padding is to align to blockSize of cipher.
			// Change it, if is not AES
			return kAesPadAllign - (packSize32 & (kAesPadAllign - 1));
		}

		HRESULT CDecoder::Init_and_CheckPassword(bool &passwOK)
		{
			passwOK = false;
			if(_remSize < 16)
				return E_NOTIMPL;
			Byte * p = _bufAligned;
			uint16 format = GetUi16(p);
			if(format != 3)
				return E_NOTIMPL;
			uint16 algId = GetUi16(p + 2);
			if(algId < kAES128)
				return E_NOTIMPL;
			algId -= kAES128;
			if(algId > 2)
				return E_NOTIMPL;
			uint16 bitLen = GetUi16(p + 4);
			uint16 flags = GetUi16(p + 6);
			if(algId * 64 + 128 != bitLen)
				return E_NOTIMPL;
			_key.KeySize = 16 + algId * 8;
			bool cert = ((flags & 2) != 0);

			if((flags & 0x4000) != 0) {
				// Use 3DES for rd data
				return E_NOTIMPL;
			}

			if(cert) {
				return E_NOTIMPL;
			}
			else {
				if((flags & 1) == 0)
					return E_NOTIMPL;
			}

			uint32 rdSize = GetUi16(p + 8);

			if(rdSize + 16 > _remSize)
				return E_NOTIMPL;

			const uint kPadSize = kAesPadAllign; // is equal to blockSize of cipher for rd

			/*
			   if(cert)
			   {
			   if((rdSize & 0x7) != 0)
				return E_NOTIMPL;
			   }
			   else
			 */
			{
				// PKCS7 padding
				if(rdSize < kPadSize)
					return E_NOTIMPL;
				if((rdSize & (kPadSize - 1)) != 0)
					return E_NOTIMPL;
			}

			memmove(p, p + 10, rdSize);
			const Byte * p2 = p + rdSize + 10;
			uint32 reserved = GetUi32(p2);
			p2 += 4;

			/*
			   if(cert)
			   {
			   uint32 numRecipients = reserved;

			   if(numRecipients == 0)
				return E_NOTIMPL;

			   {
				uint32 hashAlg = GetUi16(p2);
				hashAlg = hashAlg;
				uint32 hashSize = GetUi16(p2 + 2);
				hashSize = hashSize;
				p2 += 4;

				reserved = reserved;
				// return E_NOTIMPL;

				for(unsigned r = 0; r < numRecipients; r++)
				{
				  uint32 specSize = GetUi16(p2);
				  p2 += 2;
				  p2 += specSize;
				}
			   }
			   }
			   else
			 */
			{
				if(reserved != 0)
					return E_NOTIMPL;
			}

			uint32 validSize = GetUi16(p2);
			p2 += 2;
			const size_t validOffset = p2 - p;
			if((validSize & 0xF) != 0 || validOffset + validSize != _remSize)
				return E_NOTIMPL;

			{
				RINOK(SetKey(_key.MasterKey, _key.KeySize));
				RINOK(SetInitVector(_iv, 16));
				RINOK(Init());
				Filter(p, rdSize);

				rdSize -= kPadSize;
				for(uint i = 0; i < kPadSize; i++)
					if(p[(size_t)rdSize + i] != kPadSize)
						return S_OK;  // passwOK = false;
			}

			Byte fileKey[32];
			NSha1::CContext sha;
			sha.Init();
			sha.Update(_iv, _ivSize);
			sha.Update(p, rdSize);
			DeriveKey(sha, fileKey);

			RINOK(SetKey(fileKey, _key.KeySize));
			RINOK(SetInitVector(_iv, 16));
			Init();

			memmove(p, p + validOffset, validSize);
			Filter(p, validSize);

			if(validSize < 4)
				return E_NOTIMPL;
			validSize -= 4;
			if(GetUi32(p + validSize) != CrcCalc(p, validSize))
				return S_OK;
			passwOK = true;
			return S_OK;
		}
	}
	//
	// MyAesReg 
	//
	REGISTER_FILTER_E(AES256CBC, CAesCbcDecoder(32), CAesCbcEncoder(32), 0x6F00181, "AES256CBC")
}
//
