// HASHFUNC.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2016, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <xxhash.h>
//#define TEST_ZLIB_IMPLEMENTATION
//#define _LOCAL_USE_SSL
#ifdef _LOCAL_USE_SSL
	#include <slib-ossl.h>
#endif

/*static*/uint SlHash::CalcHashTabIndex(const void * pData, uint dataLen, uint tabCount, uint tryN)
{
	// double hashing
	assert(pData && dataLen && tabCount > 2);
	uint64 h = SlHash::XX64(pData, dataLen, 843353061036582067ULL);
	if(tryN) {
		uint64 h2 = SlHash::XX64(pData, dataLen, 458480108401970387ULL);
		if(h2 == 0ULL)
			h2 = 1ULL;
		h = (h + h2 * tryN);
	}
	return (h % tabCount);
}

static const SIntToSymbTabEntry P_HashFuncDeclList[] = {
	{ SHASHF_CRC8, "crc8" },
	{ SHASHF_CRC16, "crc16" },
	{ SHASHF_CRC24, "crc24" },
	{ SHASHF_CRC32, "crc32" },
	{ SHASHF_CRC64, "crc64" },
	{ SHASHF_CRCCCITT, "crcccitt" },
	{ SHASHF_CRCDNP, "crddnp" },
	{ SHASHF_CRCKERMIT, "crckermit" },
	{ SHASHF_CRCSICK, "crcsick" },
	{ SHASHF_ADLER32, "adler32" },
	{ SHASHF_XX32, "xxhash32" },
	{ SHASHF_XX64, "xxhash64" },
	{ SHASHF_MURMUR2_32, "murmur2-32" },
	{ SHASHF_MURMUR2_64, "murmur2-64" },
	{ SHASHF_MURMUR3_32, "murmur3-32" },
	{ SHASHF_MURMUR3_128X32, "murmur3-128x32" },
	{ SHASHF_MURMUR3_128X64, "murmur3-128x64" },
	{ SHASHF_MD2, "md2" },
	{ SHASHF_MD4, "md4" },
	{ SHASHF_MD5, "md5" },
	{ SHASHF_SHA1, "sha1" },
	{ SHASHF_SHA256, "sha256" },
	{ SHASHF_SHA512, "sha512" },
	{ SHASHF_BLAKE_256, "blake256" },
	{ SHASHF_BLAKE_512, "blake512" },
	{ SHASHF_BLAKE2B, "blake2b" },
	{ SHASHF_BLAKE2BP, "blake2bp" },
	{ SHASHF_BLAKE2S, "blake2s" },
	{ SHASHF_BLAKE2SP, "blake2sp" },
	{ SHASHF_BLAKE3, "blake3" },
};

/*static*/int SlHash::GetAlgorithmSymb(int ident, SString & rBuf)
{
	return SIntToSymbTab_GetSymb(P_HashFuncDeclList, SIZEOFARRAY(P_HashFuncDeclList), ident, rBuf);
}

/*static*/int SlHash::IdentifyAlgorithmSymb(const char * pSymb)
{
	int id = SIntToSymbTab_GetId(P_HashFuncDeclList, SIZEOFARRAY(P_HashFuncDeclList), pSymb);
	if(id == 0) {
		//STATIC_ASSERT(__max_symb() == 16);
	}
	return id;
}

/*static*/int SlHash::CalcBufferHash(int hashFuncIdent, const void * pBuf, size_t bufLen, SBinaryChunk & rResult)
{
	rResult.Z();
	int    ok = 1;
	THROW(pBuf && bufLen); // @err
	switch(hashFuncIdent) {
		case SHASHF_SHA1:
			{
				const binary160 h = SlHash::Sha1(0, pBuf, bufLen);
				rResult.Put(&h, sizeof(h));
			}
			break;
		case SHASHF_SHA256:
			{
				const binary256 h = SlHash::Sha256(0, pBuf, bufLen);
				rResult.Put(&h, sizeof(h));
			}
			break;
		case SHASHF_SHA512:
			{
				const binary512 h = SlHash::Sha512(0, pBuf, bufLen);
				rResult.Put(&h, sizeof(h));
			}
			break;
		case SHASHF_MD5:
			{
				const binary128 h = SlHash::Md5(0, pBuf, bufLen);
				rResult.Put(&h, sizeof(h));
			}
			break;
		case SHASHF_CRC32:
			{
				const uint32 h = SlHash::CRC32(0, pBuf, bufLen);
				rResult.Put(&h, sizeof(h));
			}
			break;
		default:
			CALLEXCEPT_S(SLERR_INVORUNSUPPHASHFUNC);
			break;
	}
	CATCHZOK
	return ok;
}

/*static*/int SlHash::VerifyBuffer(int hashFuncIdent, const void * pHash, size_t hashLen, const void * pBuf, size_t bufLen)
{
	int    ok = 1;
	THROW(pHash && hashLen); // @err
	THROW(pBuf && bufLen); // @err
	switch(hashFuncIdent) {
		case SHASHF_SHA1:
			THROW(hashLen == 20); // @err
			{
				const binary160 h = SlHash::Sha1(0, pBuf, bufLen);
				THROW(memcmp(&h, pHash, sizeof(h)) == 0);
			}
			break;
		case SHASHF_SHA256:
			THROW(hashLen == 32); // @err
			{
				const binary256 h = SlHash::Sha256(0, pBuf, bufLen);
				THROW(memcmp(&h, pHash, sizeof(h)) == 0);
			}
			break;
		case SHASHF_SHA512:
			THROW(hashLen == 64); // @err
			{
				const binary512 h = SlHash::Sha512(0, pBuf, bufLen);
				THROW(memcmp(&h, pHash, sizeof(h)) == 0);
			}
			break;
		case SHASHF_MD5:
			THROW(hashLen == 16); // @err
			{
				const binary128 h = SlHash::Md5(0, pBuf, bufLen);
				THROW(memcmp(&h, pHash, sizeof(h)) == 0);
			}
			break;
		case SHASHF_CRC32:
			THROW(hashLen == 4); // @err
			{
				const uint32 h = SlHash::CRC32(0, pBuf, bufLen);
				THROW(memcmp(&h, pHash, sizeof(h)) == 0);
			}
			break;
		default:
			CALLEXCEPT_S(SLERR_INVORUNSUPPHASHFUNC);
			break;
	}
	CATCHZOK
	return ok;
}

uint8  * SlHash::State::P_Tab_Crc8 = 0;
uint16 * SlHash::State::P_Tab_Crc16 = 0;
uint32 * SlHash::State::P_Tab_Crc32 = 0;
uint64 * SlHash::State::P_Tab_Crc64[8];

BdtTestItem::Buffer::Buffer() : DataLen(0)
{
	SBaseBuffer::Init();
}
		
BdtTestItem::Buffer::~Buffer()
{
	SBaseBuffer::Destroy();
}

int BdtTestItem::Buffer::Put(const void * pData, size_t dataLen)
{
	int    ok = 1;
	if(pData) {
		if(dataLen <= Size || Alloc(dataLen)) {
			memcpy(P_Buf, pData, dataLen);
			DataLen = dataLen;
		}
		else
			ok = 0;
	}
	else {
		DataLen = 0;
	}
	return ok;
}

/*static*/int BdtTestItem::_DecodeTestData(const SString & rSrc, BdtTestItem::Buffer & rDest, STempBuffer & rTempBuffer)
{
	int    ok = 1;
	size_t req_size = rSrc.Len(); // На самом деле Len()/2 но подстрахуемся
	size_t real_bin_size = 0;
	if(req_size > rTempBuffer.GetSize())
		THROW(rTempBuffer.Alloc(req_size));
	THROW(rSrc.DecodeHex(0, rTempBuffer, rTempBuffer.GetSize(), &real_bin_size));
	THROW(rDest.Put(rTempBuffer, real_bin_size));
	CATCHZOK
	return ok;
}

BdtTestItem::BdtTestItem() : Flags(0), OutLen(0), Iterations(0), Seek(0)
{
}
	
BdtTestItem::~BdtTestItem()
{
}

/*static*/int BdtTestItem::ReadBdtTestData(const char * pFileName, const char * pSetSymb, TSCollection <BdtTestItem> & rData)
{
    int    ok = 1;
	STempBuffer temp_data_buffer(4096);
    THROW(fileExists(pFileName));
    THROW(temp_data_buffer.IsValid());
    {
    	SString line_buf;
    	SString hdr_buf, data_buf;
    	SString set_name;
    	BdtTestItem * p_current_item = 0;
    	SFile f_in(pFileName, SFile::mRead);
		while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			if(line_buf.NotEmpty()) {
                if(line_buf.C(0) == '#') { // comment
				}
				else if(line_buf.C(0) == '[') {
					size_t cpos = 0;
                    if(line_buf.SearchChar(']', &cpos)) {
						line_buf.Sub(1, cpos-1, set_name);
                    }
				}
				else if(isempty(pSetSymb) || set_name.IsEqiAscii(pSetSymb)) {
					THROW(SETIFZ(p_current_item, rData.CreateNewItem()));
                    if(line_buf.Divide('=', hdr_buf, data_buf) > 0) {
                        hdr_buf.Strip();
                        data_buf.Strip();
                        if(hdr_buf.IsEqiAscii("In")) {
							if(!(p_current_item->Flags & BdtTestItem::fIn)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->In, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fIn;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Out") || hdr_buf.IsEqiAscii("Output")) {
							if(!(p_current_item->Flags & BdtTestItem::fOut)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Out, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fOut;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Key")) {
							if(!(p_current_item->Flags & (BdtTestItem::fKey|BdtTestItem::fSecret|BdtTestItem::fPassphrase))) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Key, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fKey;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Secret")) {
							if(!(p_current_item->Flags & (BdtTestItem::fKey|BdtTestItem::fSecret|BdtTestItem::fPassphrase))) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Key, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fSecret;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Passphrase")) {
							if(!(p_current_item->Flags & (BdtTestItem::fKey|BdtTestItem::fSecret|BdtTestItem::fPassphrase))) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Key, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fPassphrase;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Salt")) {
							if(!(p_current_item->Flags & BdtTestItem::fSalt)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Salt, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fSalt;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Label")) {
							if(!(p_current_item->Flags & BdtTestItem::fLabel)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Label, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fLabel;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("IKM")) {
							if(!(p_current_item->Flags & BdtTestItem::fIKM)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->IKM, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fIKM;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("XTS")) {
							if(!(p_current_item->Flags & BdtTestItem::fXTS)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->XTS, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fXTS;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Nonce")) {
							if(!(p_current_item->Flags & BdtTestItem::fNonce)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Nonce, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fNonce;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("AD")) { // @v10.5.11
							if(!(p_current_item->Flags & BdtTestItem::fAD)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Aad, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fAD;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("OutputLen")) {
							if(!(p_current_item->Flags & BdtTestItem::fOutLen)) {
								p_current_item->OutLen = static_cast<size_t>(data_buf.ToLong());
								p_current_item->Flags |= BdtTestItem::fOutLen;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Iterations")) {
							if(!(p_current_item->Flags & BdtTestItem::fIterations)) {
								p_current_item->Iterations = static_cast<uint>(data_buf.ToLong());
								p_current_item->Flags |= BdtTestItem::fIterations;
							}
                        }
                        else if(hdr_buf.IsEqiAscii("Seek")) {
							if(!(p_current_item->Flags & BdtTestItem::fSeek)) {
								p_current_item->Seek = static_cast<uint>(data_buf.ToLong());
								p_current_item->Flags |= BdtTestItem::fSeek;
							}
                        }
                    }
				}
			}
			else {
				p_current_item = 0;
			}
		}
    }
    CATCHZOK
    return ok;
}

uint32 FASTCALL SlHash::RS(const void * pData, size_t len)
{
	uint b = 378551;
	uint a = 63689;
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash = hash * a + PTR8C(pData)[i];
		a = a * b;
	}
	return hash;
}

uint32 FASTCALL SlHash::JS(const void * pData, size_t len)
{
	uint32 hash = 1315423911;
	for(uint i = 0; i < len; i++) {
		hash ^= ((hash << 5) + PTR8C(pData)[i] + (hash >> 2));
	}
	return hash;
}

uint32 FASTCALL SlHash::PJW(const void * pData, size_t len)
{
	const uint BitsInUnsignedInt = (uint)(sizeof(uint) * 8);
	const uint ThreeQuarters     = (uint)((BitsInUnsignedInt  * 3) / 4);
	const uint OneEighth = (uint)(BitsInUnsignedInt / 8);
	const uint HighBits  = _FFFF32 << (BitsInUnsignedInt - OneEighth);
	uint32 hash = 0;
	uint32 test = 0;
	for(uint i = 0; i < len; i++) {
		hash = (hash << OneEighth) + PTR8C(pData)[i];
		if((test = hash & HighBits)  != 0) {
			hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
		}
	}
	return hash;
}

uint32 FASTCALL SlHash::ELF(const void * pData, size_t len)
{
	uint32 hash = 0;
	uint   x = 0;
	for(uint i = 0; i < len; i++) {
		hash = (hash << 4) + PTR8C(pData)[i];
		if((x = hash & 0xF0000000L) != 0) {
			hash ^= (x >> 24);
		}
		hash &= ~x;
	}
	return hash;
}

uint32 FASTCALL SlHash::BKDR(const void * pData, size_t len)
{
	const uint32 seed = 131; /* 31 131 1313 13131 131313 etc.. */
	uint32 hash = 0;
	for(uint i = 0; i < len; i++)
		hash = (hash * seed) + PTR8C(pData)[i];
	return hash;
}

uint32 FASTCALL SlHash::SDBM(const void * pData, size_t len)
{
	uint32 hash = 0;
	for(uint i = 0; i < len; i++)
		hash = PTR8C(pData)[i] + (hash << 6) + (hash << 16) - hash;
	return hash;
}

uint32 FASTCALL SlHash::DJB(const void * pData, size_t len)
{
	uint32 hash = SlConst::DjbHashInit32;
	for(uint i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + PTR8C(pData)[i];
	return hash;
}

uint32 FASTCALL SlHash::DEK(const void * pData, size_t len)
{
	uint32 hash = len;
	for(uint i = 0; i < len; i++)
		hash = ((hash << 5) ^ (hash >> 27)) ^ PTR8C(pData)[i];
	return hash;
}

uint32 FASTCALL SlHash::BP(const void * pData, size_t len)
{
	uint32 hash = 0;
	for(uint i = 0; i < len; i++)
		hash = hash << 7 ^ PTR8C(pData)[i];
	return hash;
}
//
// FNV Hash Function
// Fowler / Noll / Vo (FNV) Hash (http://www.isthe.com/chongo/tech/comp/fnv/)
//
uint32 FASTCALL SlHash::FNV(const void * pData, size_t len)
{
	//
	// @20240405 Похоже, я нашел неверную реализацию функции. Судя по вариантам из других источников,
	// значение 0x811C9DC5 применяется в качестве инициализации, а в итерации используется 0x01000193.
	// Черт! Это - плохо, я из того источника много функций взял и с тестами - придется все пересматривать.
	//
	const uint fnv_prime = SlConst::FnvHash1Init32;
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash *= fnv_prime;
		hash ^= PTR8C(pData)[i];
	}
	return hash;
}

uint32 FASTCALL SlHash::AP(const void * pData, size_t len)
{
	uint32 hash = 0xAAAAAAAAU;
	for(uint i = 0; i < len; i++) {
		hash ^= ((i & 1) == 0) ? ((hash << 7) ^ PTR8C(pData)[i] * (hash >> 3)) : (~((hash << 11) + (PTR8C(pData)[i] ^ (hash >> 5))));
	}
	return hash;
}
//
// Bob Jenkins hash function
//
/*
   These are functions for producing 32-bit hashes for hash table lookup.
   BobJencHash_Word(), BobJencHash_Little(), BobJencHash_Little2(), BobJencHash_Big(), mix(), and final()
   are externally useful functions.  Routines to test the hash are included
   if SELF_TEST is defined.  You can use this free for any purpose.  It's in
   the public domain.  It has no warranty.

   You probably want to use BobJencHash_Little().  BobJencHash_Little() and hashbig()
   hash byte arrays.  BobJencHash_Little() is is faster than hashbig() on
   little-endian machines.  Intel and AMD are little-endian machines.
   On second thought, you probably want BobJencHash_Little2(), which is identical to
   BobJencHash_Little() except it returns two 32-bit hashes for the price of one.
   You could implement hashbig2() if you wanted but I haven't bothered here.

   If you want to find a hash of, say, exactly 7 integers, do
   a = i1;  b = i2;  c = i3;
   mix(a,b,c);
   a += i4; b += i5; c += i6;
   mix(a,b,c);
   a += i7;
   final(a,b,c);
   then use c as the hash value.  If you have a variable length array of
   4-byte integers to hash, use BobJencHash_Word().  If you have a byte array (like
   a character string), use BobJencHash_Little().  If you have several byte arrays, or
   a mix of things, see the comments above BobJencHash_Little().

   Why is this so big?  I read 12 bytes at a time into 3 4-byte integers,
   then mix those integers.  This is fast (you can do a lot more thorough
   mixing with 12*3 instructions on 3 integers than you can with 3 instructions
   on 1 byte), but shoehorning those bytes into integers efficiently is messy.
*/
// @v10.8.0 (replaced with _rotl) #define rot(x, k)   (((x) << (k)) | ((x) >> (32-(k))))
/*
   BobJencHash_Mix -- mix 3 32-bit values reversibly.

   This is reversible, so any information in (a,b,c) before mix() is
   still in (a,b,c) after mix().

   If four pairs of (a,b,c) inputs are run through mix(), or through
   mix() in reverse, there are at least 32 bits of the output that
   are sometimes the same for one pair and different for another pair.
   This was tested for:
 * pairs that differed by one bit, by two bits, in any combination
   of top bits of (a,b,c), or in any combination of bottom bits of
   (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
   the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
   is commonly produced by subtraction) look like a single 1-bit
   difference.
 * the base values were pseudorandom, all zero but one bit set, or
   all zero plus a counter that starts at zero.

   Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
   satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
   Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
   for "differ" defined as + with a one-bit base and a two-bit delta.  I
   used http://burtleburtle.net/bob/hash/avalanche.html to choose
   the operations, constants, and arrangements of the variables.

   This does not achieve avalanche.  There are input bits of (a,b,c)
   that fail to affect some output bits of (a,b,c), especially of a.  The
   most thoroughly mixed value is c, but it doesn't really even achieve
   avalanche in c.

   This allows some parallelism.  Read-after-writes are good at doubling
   the number of bits affected, so the goal of mixing pulls in the opposite
   direction as the goal of parallelism.  I did what I could.  Rotates
   seem to cost as much as shifts on every machine I could lay my hands
   on, and rotates are much kinder to the top and bottom bits, so I used
   rotates.
*/
#define BobJencHash_Mix(a, b, c) { \
	a -= c;  a ^= /*_rotl*/SBits::Rotl(c, 4);  c += b; \
	b -= a;  b ^= /*_rotl*/SBits::Rotl(a, 6);  a += c; \
	c -= b;  c ^= /*_rotl*/SBits::Rotl(b, 8);  b += a; \
	a -= c;  a ^= /*_rotl*/SBits::Rotl(c, 16);  c += b; \
	b -= a;  b ^= /*_rotl*/SBits::Rotl(a, 19);  a += c; \
	c -= b;  c ^= /*_rotl*/SBits::Rotl(b, 4);  b += a; \
}
/*
   BobJencHash_Final -- final mixing of 3 32-bit values (a,b,c) into c

   Pairs of (a,b,c) values differing in only a few bits will usually
   produce values of c that look totally different.  This was tested for
 * pairs that differed by one bit, by two bits, in any combination
   of top bits of (a,b,c), or in any combination of bottom bits of (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
   the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
   is commonly produced by subtraction) look like a single 1-bit difference.
 * the base values were pseudorandom, all zero but one bit set, or
   all zero plus a counter that starts at zero.

   These constants passed:
   14 11 25 16 4 14 24
   12 14 25 16 4 14 24
   and these came close:
   4  8 15 26 3 22 24
   10  8 15 26 3 22 24
   11  8 15 26 3 22 24
*/
#define BobJencHash_Final(a, b, c) { \
	c ^= b; c -= /*_rotl*/SBits::Rotl(b, 14); \
	a ^= c; a -= /*_rotl*/SBits::Rotl(c, 11); \
	b ^= a; b -= /*_rotl*/SBits::Rotl(a, 25); \
	c ^= b; c -= /*_rotl*/SBits::Rotl(b, 16); \
	a ^= c; a -= /*_rotl*/SBits::Rotl(c, 4);	 \
	b ^= a; b -= /*_rotl*/SBits::Rotl(a, 14); \
	c ^= b; c -= /*_rotl*/SBits::Rotl(b, 24); \
}
//
// This works on all machines.  To be useful, it requires
// -- that the key be an array of uint32's, and
// -- that the length be the number of uint32's in the key
//
// The function BobJencHash_Word() is identical to BobJencHash_Little() on little-endian
// machines, and identical to hashbig() on big-endian machines,
// except that the length has to be measured in uint32_ts rather than in
// bytes.  BobJencHash_Little() is more complicated than BobJencHash_Word() only because
// BobJencHash_Little() has to dance around fitting the key bytes into registers.
//
// ARG(k       IN) the key, an array of uint32 values
// ARG(length  IN) the length of the key, in uint32_ts
// ARG(initval IN) the previous hash, or an arbitrary value
//
/*static*/uint32 SlHash::BobJencHash_Word(const uint32 * k, size_t length, uint32 initval)
{
	uint32 a = 0xdeadbeef + (((uint32)length)<<2) + initval;
	uint32 b = a;
	uint32 c = a;
	while(length > 3) {
		a += k[0];
		b += k[1];
		c += k[2];
		BobJencHash_Mix(a, b, c);
		length -= 3;
		k += 3;
	}
	//
	// handle the last 3 uint32's
	//
	switch(length) { // all the case statements fall through
		case 3: c += k[2];
		case 2: b += k[1];
		case 1:
			a += k[0];
		    BobJencHash_Final(a, b, c);
		case 0: // case 0: nothing left to add 
		    break;
	}
	return c;
}
//
// hashword2() -- same as BobJencHash_Word(), but take two seeds and return two
// 32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
// both be initialized with seeds.  If you pass in (*pb)==0, the output
// (*pc) will be the same as the return value from BobJencHash_Word().
//
// ARG(k      IN) the key, an array of uint32 values
// ARG(length IN) the length of the key, in uint32_ts
// ARG(pc INOUT) IN: seed OUT: primary hash value
// ARG(pb INOUT) IN: more seed OUT: secondary hash value
//
/*static*/void SlHash::BobJencHash_Word2(const uint32 * k, size_t length, uint32 * pc, uint32 * pb)
{
	uint32 a = 0xdeadbeef + ((uint32)(length<<2)) + *pc;
	uint32 b = a;
	uint32 c = a + *pb;
	while(length > 3) {
		a += k[0];
		b += k[1];
		c += k[2];
		BobJencHash_Mix(a, b, c);
		length -= 3;
		k += 3;
	}
	//
	// handle the last 3 uint32's
	//
	switch(length) { // all the case statements fall through
		case 3: c += k[2];
		case 2: b += k[1];
		case 1: a += k[0];
		    BobJencHash_Final(a, b, c);
		case 0: /* case 0: nothing left to add */
		    break;
	}
	// report the result
	*pc = c;
	*pb = b;
}
//
// BobJencHash_Little() -- hash a variable-length key into a 32-bit value
// k       : the key (the unaligned variable-length array of bytes)
// length  : the length of the key, counting by bytes
// initval : can be any 4-byte value
// Returns a 32-bit value.  Every bit of the key affects every bit of
// the return value.  Two keys differing by one or two bits will have
// totally different hash values.
//
// The best hash table sizes are powers of 2.  There is no need to do
// mod a prime (mod is sooo slow!).  If you need less than 32 bits,
// use a bitmask.  For example, if you need only 10 bits, do
// h = (h & hashmask(10));
// In which case, the hash table should have hashsize(10) elements.
//
// If you are hashing n strings (uint8 **)k, do it like this:
// for(i=0, h=0; i < n; ++i) h = BobJencHash_Little( k[i], len[i], h);
//
// By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
// code any way you wish, private, educational, or commercial.  It's free.
//
// Use for hash table lookup, or anything where one collision in 2^^32 is
// acceptable.  Do NOT use for cryptographic purposes.
//
/*static*/uint32 SlHash::BobJencHash_Little(const void * key, size_t length, uint32 initval)
{
	union {
		const void * ptr;
		size_t i;
	} u; // needed for Mac Powerbook G4
	uint32 a = 0xdeadbeef + ((uint32)length) + initval;
	uint32 b = a;
	uint32 c = a;
	u.ptr = key;
	if(!SLS.SSys.IsBigEndian && ((u.i & 0x3) == 0)) {
		const uint32 * k = static_cast<const uint32 *>(key); // read 32-bit chunks
		//
		// all but last block: aligned reads and affect 32 bits of (a,b,c)
		//
		while(length > 12) {
			a += k[0];
			b += k[1];
			c += k[2];
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 3;
		}
		//
		// handle the last (probably partial) block */
		//
		// "k[2]&0xffffff" actually reads beyond the end of the string, but
		// then masks off the part it's not allowed to read.  Because the
		// string is aligned, the masked-off tail is in the same word as the
		// rest of the string.  Every machine with memory protection I've seen
		// does it on word boundaries, so is OK with this.  But VALGRIND will
		// still catch it and complain.  The masking trick does make the hash
		// noticably faster for short strings (like English words).
		//
		switch(length) {
			case 12: c += k[2]; b += k[1]; a += k[0]; break;
			case 11: c += k[2]&0xffffff; b += k[1]; a += k[0]; break;
			case 10: c += k[2]&0xffff; b += k[1]; a += k[0]; break;
			case 9: c += k[2]&0xff; b += k[1]; a += k[0]; break;
			case 8: b += k[1]; a += k[0]; break;
			case 7: b += k[1]&0xffffff; a += k[0]; break;
			case 6: b += k[1]&0xffff; a += k[0]; break;
			case 5: b += k[1]&0xff; a += k[0]; break;
			case 4: a += k[0]; break;
			case 3: a += k[0]&0xffffff; break;
			case 2: a += k[0]&0xffff; break;
			case 1: a += k[0]&0xff; break;
			case 0: return c; /* zero length strings require no mixing */
		}
	}
	else if(!SLS.SSys.IsBigEndian && ((u.i & 0x1) == 0)) {
		const uint16 * k = static_cast<const uint16 *>(key); /* read 16-bit chunks */
		const uint8  * k8;
		//
		// all but last block: aligned reads and different mixing
		//
		while(length > 12) {
			a += k[0] + (((uint32)k[1])<<16);
			b += k[2] + (((uint32)k[3])<<16);
			c += k[4] + (((uint32)k[5])<<16);
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 6;
		}
		//
		// handle the last (probably partial) block
		//
		k8 = reinterpret_cast<const uint8 *>(k);
		switch(length) {
			case 12:
				c += k[4]+(((uint32)k[5])<<16);
			    b += k[2]+(((uint32)k[3])<<16);
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 11:
				c += ((uint32)k8[10])<<16; // @fallthrough
			case 10:
				c += k[4];
			    b += k[2]+(((uint32)k[3])<<16);
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 9:
				c += k8[8]; // @fallthrough
			case 8:
				b += k[2]+(((uint32)k[3])<<16);
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 7:
				b += ((uint32)k8[6])<<16; // @fallthrough
			case 6:
				b += k[2];
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 5:
				b += k8[4]; // @fallthrough
			case 4:
				a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 3:
				a += ((uint32)k8[2])<<16; // @fallthrough
			case 2:
				a += k[0];
			    break;
			case 1:
				a += k8[0];
			    break;
			case 0:
				return c; /* zero length requires no mixing */
		}
	}
	else { // need to read the key one byte at a time
		const uint8 * k = static_cast<const uint8 *>(key);
		//
		// all but the last block: affect some 32 bits of (a,b,c)
		//
		while(length > 12) {
			a += k[0];
			a += ((uint32)k[1])<<8;
			a += ((uint32)k[2])<<16;
			a += ((uint32)k[3])<<24;
			b += k[4];
			b += ((uint32)k[5])<<8;
			b += ((uint32)k[6])<<16;
			b += ((uint32)k[7])<<24;
			c += k[8];
			c += ((uint32)k[9])<<8;
			c += ((uint32)k[10])<<16;
			c += ((uint32)k[11])<<24;
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 12;
		}
		//
		// last block: affect all 32 bits of (c)
		//
		switch(length) { // all the case statements fall through
			case 12: c += ((uint32)k[11])<<24;
			case 11: c += ((uint32)k[10])<<16;
			case 10: c += ((uint32)k[9])<<8;
			case 9: c += k[8];
			case 8: b += ((uint32)k[7])<<24;
			case 7: b += ((uint32)k[6])<<16;
			case 6: b += ((uint32)k[5])<<8;
			case 5: b += k[4];
			case 4: a += ((uint32)k[3])<<24;
			case 3: a += ((uint32)k[2])<<16;
			case 2: a += ((uint32)k[1])<<8;
			case 1: a += k[0]; break;
			case 0: return c;
		}
	}
	BobJencHash_Final(a, b, c);
	return c;
}
//
// BobJencHash_Little2: return 2 32-bit hash values
//
// This is identical to BobJencHash_Little(), except it returns two 32-bit hash
// values instead of just one.  This is good enough for hash table
// lookup with 2^^64 buckets, or if you want a second hash if you're not
// happy with the first, or if you want a probably-unique 64-bit ID for
// the key.  *pc is better mixed than *pb, so use *pc first.  If you want
// a 64-bit value do something like "*pc + (((uint64)*pb)<<32)".
//
// ARG(key    IN) the key to hash
// ARG(length IN) length of the key
// ARG(pc INOUT)  IN: primary initval, OUT: primary hash
// ARG(pb INOUT)  IN: secondary initval, OUT: secondary hash
//
/*static*/void SlHash::BobJencHash_Little2(const void * key, size_t length, uint32 * pc, uint32 * pb)
{
	union { // needed for Mac Powerbook G4
		const void * ptr;
		size_t i;
	} u;
	uint32 a = 0xdeadbeef + ((uint32)length) + *pc;
	uint32 b = a;
	uint32 c = a + *pb;
	u.ptr = key;
	if(((u.i & 0x3) == 0) && !SLS.SSys.IsBigEndian) {
		const uint32 * k = static_cast<const uint32 *>(key); /* read 32-bit chunks */
		//
		// all but last block: aligned reads and affect 32 bits of (a,b,c)
		//
		while(length > 12) {
			a += k[0];
			b += k[1];
			c += k[2];
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 3;
		}
		/*----------------------------- handle the last (probably partial) block */
		/*
		 * "k[2]&0xffffff" actually reads beyond the end of the string, but
		 * then masks off the part it's not allowed to read.  Because the
		 * string is aligned, the masked-off tail is in the same word as the
		 * rest of the string.  Every machine with memory protection I've seen
		 * does it on word boundaries, so is OK with this.  But VALGRIND will
		 * still catch it and complain.  The masking trick does make the hash
		 * noticably faster for short strings (like English words).
		 */
		switch(length) {
			case 12: c += k[2]; b += k[1]; a += k[0]; break;
			case 11: c += k[2]&0xffffff; b += k[1]; a += k[0]; break;
			case 10: c += k[2]&0xffff; b += k[1]; a += k[0]; break;
			case 9: c += k[2]&0xff; b += k[1]; a += k[0]; break;
			case 8: b += k[1]; a += k[0]; break;
			case 7: b += k[1]&0xffffff; a += k[0]; break;
			case 6: b += k[1]&0xffff; a += k[0]; break;
			case 5: b += k[1]&0xff; a += k[0]; break;
			case 4: a += k[0]; break;
			case 3: a += k[0]&0xffffff; break;
			case 2: a += k[0]&0xffff; break;
			case 1: a += k[0]&0xff; break;
			case 0: *pc = c; *pb = b; return; /* zero length strings require no mixing */
		}
	}
	else if(((u.i & 0x1) == 0) && !SLS.SSys.IsBigEndian) {
		const uint16 * k = static_cast<const uint16 *>(key); /* read 16-bit chunks */
		const uint8  * k8;
		// all but last block: aligned reads and different mixing 
		while(length > 12) {
			a += k[0] + (((uint32)k[1])<<16);
			b += k[2] + (((uint32)k[3])<<16);
			c += k[4] + (((uint32)k[5])<<16);
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 6;
		}
		/*----------------------------- handle the last (probably partial) block */
		k8 = reinterpret_cast<const uint8 *>(k);
		switch(length) {
			case 12: c += k[4]+(((uint32)k[5])<<16);
			    b += k[2]+(((uint32)k[3])<<16);
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 11: c += ((uint32)k8[10])<<16; // @fallthrough
			case 10: c += k[4];
			    b += k[2]+(((uint32)k[3])<<16);
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 9: c += k8[8]; // @fallthrough
			case 8: b += k[2]+(((uint32)k[3])<<16);
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 7: b += ((uint32)k8[6])<<16; // @fallthrough
			case 6: b += k[2];
			    a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 5: b += k8[4]; // @fallthrough
			case 4: a += k[0]+(((uint32)k[1])<<16);
			    break;
			case 3: a += ((uint32)k8[2])<<16; // @fallthrough
			case 2: a += k[0];
			    break;
			case 1: a += k8[0];
			    break;
			case 0: *pc = c; *pb = b; return; /* zero length strings require no mixing */
		}
	}
	else {                    /* need to read the key one byte at a time */
		const uint8 * k = static_cast<const uint8 *>(key);
		/*--------------- all but the last block: affect some 32 bits of (a,b,c) */
		while(length > 12) {
			a += k[0];
			a += ((uint32)k[1])<<8;
			a += ((uint32)k[2])<<16;
			a += ((uint32)k[3])<<24;
			b += k[4];
			b += ((uint32)k[5])<<8;
			b += ((uint32)k[6])<<16;
			b += ((uint32)k[7])<<24;
			c += k[8];
			c += ((uint32)k[9])<<8;
			c += ((uint32)k[10])<<16;
			c += ((uint32)k[11])<<24;
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 12;
		}
		/*-------------------------------- last block: affect all 32 bits of (c) */
		switch(length) { // all the case statements fall through
			case 12: c += ((uint32)k[11])<<24;
			case 11: c += ((uint32)k[10])<<16;
			case 10: c += ((uint32)k[9])<<8;
			case 9: c += k[8];
			case 8: b += ((uint32)k[7])<<24;
			case 7: b += ((uint32)k[6])<<16;
			case 6: b += ((uint32)k[5])<<8;
			case 5: b += k[4];
			case 4: a += ((uint32)k[3])<<24;
			case 3: a += ((uint32)k[2])<<16;
			case 2: a += ((uint32)k[1])<<8;
			case 1: a += k[0];
			    break;
			case 0: *pc = c; *pb = b; return; /* zero length strings require no mixing */
		}
	}
	BobJencHash_Final(a, b, c);
	*pc = c; *pb = b;
}
//
// hashbig():
// This is the same as BobJencHash_Word() on big-endian machines.  It is different
// from BobJencHash_Little() on all machines.  hashbig() takes advantage of big-endian byte ordering.
//
static uint32 BobJencHash_Big(const void * key, size_t length, uint32 initval)
{
	union {
		const void * ptr;
		size_t i;
	} u; // to cast key to (size_t) happily
	uint32 a = 0xdeadbeef + ((uint32)length) + initval;
	uint32 b = a;
	uint32 c = a;
	u.ptr = key;
	if(((u.i & 0x3) == 0) && SLS.SSys.IsBigEndian) {
		const uint32 * k = static_cast<const uint32 *>(key); /* read 32-bit chunks */
		// all but last block: aligned reads and affect 32 bits of (a,b,c)
		while(length > 12) {
			a += k[0];
			b += k[1];
			c += k[2];
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 3;
		}
		// handle the last (probably partial) block 
		// 
		// "k[2]<<8" actually reads beyond the end of the string, but
		// then shifts out the part it's not allowed to read.  Because the
		// string is aligned, the illegal read is in the same word as the
		// rest of the string.  Every machine with memory protection I've seen
		// does it on word boundaries, so is OK with this.  But VALGRIND will
		// still catch it and complain.  The masking trick does make the hash
		// noticably faster for short strings (like English words).
		// 
		switch(length) {
			case 12: c += k[2]; b += k[1]; a += k[0]; break;
			case 11: c += k[2]&0xffffff00; b += k[1]; a += k[0]; break;
			case 10: c += k[2]&0xffff0000; b += k[1]; a += k[0]; break;
			case 9: c += k[2]&0xff000000; b += k[1]; a += k[0]; break;
			case 8: b += k[1]; a += k[0]; break;
			case 7: b += k[1]&0xffffff00; a += k[0]; break;
			case 6: b += k[1]&0xffff0000; a += k[0]; break;
			case 5: b += k[1]&0xff000000; a += k[0]; break;
			case 4: a += k[0]; break;
			case 3: a += k[0]&0xffffff00; break;
			case 2: a += k[0]&0xffff0000; break;
			case 1: a += k[0]&0xff000000; break;
			case 0: return c; /* zero length strings require no mixing */
		}
	}
	else {                    /* need to read the key one byte at a time */
		const uint8 * k = static_cast<const uint8 *>(key);
		/*--------------- all but the last block: affect some 32 bits of (a,b,c) */
		while(length > 12) {
			a += ((uint32)k[0])<<24;
			a += ((uint32)k[1])<<16;
			a += ((uint32)k[2])<<8;
			a += ((uint32)k[3]);
			b += ((uint32)k[4])<<24;
			b += ((uint32)k[5])<<16;
			b += ((uint32)k[6])<<8;
			b += ((uint32)k[7]);
			c += ((uint32)k[8])<<24;
			c += ((uint32)k[9])<<16;
			c += ((uint32)k[10])<<8;
			c += ((uint32)k[11]);
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 12;
		}
		/*-------------------------------- last block: affect all 32 bits of (c) */
		switch(length) { // all the case statements fall through
			case 12: c += k[11];
			case 11: c += ((uint32)k[10])<<8;
			case 10: c += ((uint32)k[9])<<16;
			case 9: c += ((uint32)k[8])<<24;
			case 8: b += k[7];
			case 7: b += ((uint32)k[6])<<8;
			case 6: b += ((uint32)k[5])<<16;
			case 5: b += ((uint32)k[4])<<24;
			case 4: a += k[3];
			case 3: a += ((uint32)k[2])<<8;
			case 2: a += ((uint32)k[1])<<16;
			case 1: a += ((uint32)k[0])<<24;
			    break;
			case 0: return c;
		}
	}
	BobJencHash_Final(a, b, c);
	return c;
}

uint32 FASTCALL SlHash::BobJenc(const void * pData, size_t len)
{
	uint32 hash = 0;
	if(!(len & 0x3))
		hash = BobJencHash_Word(static_cast<const uint32 *>(pData), (len >> 2), 0xfeedbeef);
	else
		hash = BobJencHash_Little(static_cast<const uint32 *>(pData), len, 0xfeedbeef);
	return hash;
}

uint32 FASTCALL SlHash::XX32(const void * pData, size_t len, uint seed) { return XXH32(pData, len, seed); }
uint64 FASTCALL SlHash::XX64(const void * pData, size_t len, uint64 seed) { return XXH64(pData, len, seed); }
//
// MurmurHash2, by Austin Appleby
// Note - This code makes a few assumptions about how your machine behaves -
// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4
//
// And it has a few limitations -
//
// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian machines.
//
static uint32 MurmurHash2(const void * key, int len, uint32 seed)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well. 
	const uint32 m = 0x5bd1e995;
	const int r = 24;
	// Initialize the hash to a 'random' value 
	uint32 h = seed ^ len;
	// Mix 4 bytes at a time into the hash 
	const uchar * p_data = (const uchar *)key;
	while(len >= 4) {
		uint32 k = *(uint32 *)p_data;
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		p_data += 4;
		len -= 4;
	}
	// Handle the last few bytes of the input array
	switch(len) {
		case 3: h ^= p_data[2] << 16;
		case 2: h ^= p_data[1] << 8;
		case 1: h ^= p_data[0];
		h *= m;
	};
	// Do a few final mixes of the hash to ensure the last few bytes are well-incorporated. 
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
} 
//
// MurmurHash2A, by Austin Appleby
//
// This is a variant of MurmurHash2 modified to use the Merkle-Damgard 
// construction. Bulk speed should be identical to Murmur2, small-key speed 
// will be 10%-20% slower due to the added overhead at the end of the hash.
//
// This variant fixes a minor issue where null keys were more likely to
// collide with each other than expected, and also makes the function
// more amenable to incremental implementations.
//
#define mmix(h, k)   { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
#define MIX(h, k ,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

static uint32 MurmurHash2A(const void * key, int len, uint32 seed)
{
	const uint32 m = 0x5bd1e995;
	const int r = 24;
	uint32 l = len;
	const uchar * data = (const uchar *)key;
	uint32 h = seed;
	while(len >= 4) {
		uint32 k = *(uint32 *)data;
		mmix(h, k);
		data += 4;
		len -= 4;
	}
	uint32 t = 0;
	switch(len) {
		case 3: t ^= data[2] << 16;
		case 2: t ^= data[1] << 8;
		case 1: t ^= data[0];
	};
	mmix(h, t);
	mmix(h, l);
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}
//
// MurmurHashNeutral2, by Austin Appleby
//
// Same as MurmurHash2, but endian- and alignment-neutral.
// Half the speed though, alas.
//
static uint32 MurmurHashNeutral2(const void * key, int len, uint32 seed)
{
	const uint32 m = 0x5bd1e995;
	const int r = 24;
	uint32 h = seed ^ len;
	const uchar * data = (const uchar *)key;
	while(len >= 4) {
		uint32 k;
		k  = data[0];
		k |= data[1] << 8;
		k |= data[2] << 16;
		k |= data[3] << 24;
		k *= m; 
		k ^= k >> r; 
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		len -= 4;
	}
	switch(len) {
		case 3: h ^= data[2] << 16;
		case 2: h ^= data[1] << 8;
		case 1: h ^= data[0];
		h *= m;
	};
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
} 
//
// MurmurHashAligned2, by Austin Appleby
//
// Same algorithm as MurmurHash2, but only does aligned reads - should be safer on certain platforms. 
//
// Performance will be lower than MurmurHash2
//
static uint32 MurmurHashAligned2(const void * key, int len, uint32 seed)
{
	const uint32 m = 0x5bd1e995;
	const int r = 24;
	const uchar * data = (const uchar *)key;
	uint32 h = seed ^ len;
	int align = static_cast<int>(reinterpret_cast<uint64>(data) & 3);
	if(align && (len >= 4)) {
		// Pre-load the temp registers 
		uint32 t = 0, d = 0;
		switch(align) {
			case 1: t |= data[2] << 16;
			case 2: t |= data[1] << 8;
			case 3: t |= data[0];
		}
		t <<= (8 * align);
		data += 4-align;
		len -= 4-align;
		int sl = 8 * (4-align);
		int sr = 8 * align;
		// Mix 
		while(len >= 4) {
			d = *(uint32 *)data;
			t = (t >> sr) | (d << sl);
			uint32 k = t;
			MIX(h,k,m);
			t = d;
			data += 4;
			len -= 4;
		}
		/* Handle leftover data in temp registers  */
		d = 0;
		if(len >= align) {
			switch(align) {
				case 3: d |= data[2] << 16;
				case 2: d |= data[1] << 8;
				case 1: d |= data[0];
			}
			uint32 k = (t >> sr) | (d << sl);
			MIX(h, k, m);
			data += align;
			len -= align;
			// Handle tail bytes
			switch(len) {
				case 3: h ^= data[2] << 16;
				case 2: h ^= data[1] << 8;
				case 1: h ^= data[0];
				h *= m;
			};
		}
		else {
			switch(len) {
				case 3: d |= data[2] << 16;
				case 2: d |= data[1] << 8;
				case 1: d |= data[0];
				case 0: h ^= (t >> sr) | (d << sl);
				h *= m;
			}
		}
		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;
		return h;
	}
	else {
		while(len >= 4) {
			uint32 k = *(uint32 *)data;
			MIX(h,k,m);
			data += 4;
			len -= 4;
		}
		// Handle tail bytes 
		switch(len) {
			case 3: h ^= data[2] << 16;
			case 2: h ^= data[1] << 8;
			case 1: h ^= data[0];
			h *= m;
		};
		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;
		return h;
	}
}

/*static*/uint32 FASTCALL SlHash::Murmur2_32(const void * pData, size_t len, uint32 seed)
{
	//
	// MurmurHash2, by Austin Appleby
	// Note - This code makes a few assumptions about how your machine behaves -
	// 1. We can read a 4-byte value from any address without crashing
	// 2. sizeof(int) == 4
	//
	// And it has a few limitations -
	//
	// 1. It will not work incrementally.
	// 2. It will not produce the same results on little-endian and big-endian machines.
	//
	//static uint32 MurmurHash2(const void * key, int len, uint32 seed)
	//
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well. 
	const uint32 m = 0x5bd1e995;
	const int r = 24;
	// Initialize the hash to a 'random' value 
	uint32 hash = seed ^ len;
	// Mix 4 bytes at a time into the hash 
	const uchar * p_data = static_cast<const uchar *>(pData);
	while(len >= 4) {
		uint32 k = *reinterpret_cast<const uint32 *>(p_data);
		k *= m;
		k ^= k >> r;
		k *= m;
		hash *= m;
		hash ^= k;
		p_data += 4;
		len -= 4;
	}
	// Handle the last few bytes of the input array
	switch(len) {
		case 3: hash ^= p_data[2] << 16;
		case 2: hash ^= p_data[1] << 8;
		case 1: hash ^= p_data[0];
		hash *= m;
	};
	// Do a few final mixes of the hash to ensure the last few bytes are well-incorporated. 
	hash ^= hash >> 13;
	hash *= m;
	hash ^= hash >> 15;
	return hash;
}

/*static*/uint64 FASTCALL SlHash::Murmur2_64(const void * pData, size_t len, uint64 seed)
{
	uint64 hash = 0;
	if(sizeof(void *) == 8) {
		//
		// MurmurHash2, 64-bit versions, by Austin Appleby
		//
		// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
		// and endian-ness issues if used across multiple platforms.
		//
		// 64-bit hash for 64-bit platforms
		//
		const uint64 m = 0xc6a4a7935bd1e995ULL;
		const int r = 47;
		hash = seed ^ (len * m);
		const uint64 * data = static_cast<const uint64 *>(pData);
		const uint64 * end = data + (len/8);
		while(data != end) {
			uint64 k = *data++;
			k *= m; 
			k ^= k >> r; 
			k *= m; 
			hash ^= k;
			hash *= m; 
		}
		const uchar * data2 = (const uchar *)data;
		switch(len & 7) {
			case 7: hash ^= ((uint64)data2[6]) << 48;
			case 6: hash ^= ((uint64)data2[5]) << 40;
			case 5: hash ^= ((uint64)data2[4]) << 32;
			case 4: hash ^= ((uint64)data2[3]) << 24;
			case 3: hash ^= ((uint64)data2[2]) << 16;
			case 2: hash ^= ((uint64)data2[1]) << 8;
			case 1: hash ^= ((uint64)data2[0]);
			hash *= m;
		};
		hash ^= hash >> r;
		hash *= m;
		hash ^= hash >> r;
	}
	else {
		//
		// 64-bit hash for 32-bit platforms 
		//
		const uint32 m = 0x5bd1e995;
		const int r = 24;
		uint32 h1 = ((uint32)seed) ^ len;
		uint32 h2 = ((uint32)(seed >> 32));
		const uint32 * data = static_cast<const uint32 *>(pData);
		while(len >= 8) {
			uint32 k1 = *data++;
			k1 *= m; k1 ^= k1 >> r; k1 *= m;
			h1 *= m; h1 ^= k1;
			len -= 4;

			uint32 k2 = *data++;
			k2 *= m; k2 ^= k2 >> r; k2 *= m;
			h2 *= m; h2 ^= k2;
			len -= 4;
		}
		if(len >= 4) {
			uint32 k1 = *data++;
			k1 *= m; k1 ^= k1 >> r; k1 *= m;
			h1 *= m; h1 ^= k1;
			len -= 4;
		}
		switch(len) {
			case 3: h2 ^= ((uchar *)data)[2] << 16;
			case 2: h2 ^= ((uchar *)data)[1] << 8;
			case 1: h2 ^= ((uchar *)data)[0];
			h2 *= m;
		};
		h1 ^= h2 >> 18; h1 *= m;
		h2 ^= h1 >> 22; h2 *= m;
		h1 ^= h2 >> 17; h1 *= m;
		h2 ^= h1 >> 19; h2 *= m;
		hash = h1;
		hash = (hash << 32) | h2;
	}
	return hash;
}
//
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here
//
#define getblock(p, i) (p[i])
//
// Finalization mix - force all bits of a hash block to avalanche
//
static FORCEINLINE uint32 fmix32(uint32 h) { h ^= h >> 16; h *= 0x85ebca6b;            h ^= h >> 13; h *= 0xc2b2ae35;            h ^= h >> 16; return h; }
static FORCEINLINE uint64 fmix64(uint64 h) { h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL; h ^= h >> 33; return h; }

/*static*/uint32 FASTCALL SlHash::Murmur3_32(const void * pData, size_t len, uint32 seed)
{
	const uint8 * data = static_cast<const uint8 *>(pData);
	const int nblocks = len / 4;
	int i;
	uint32 h1 = seed;
	const uint32 c1 = SlConst::MagicMurmurC1;
	const uint32 c2 = SlConst::MagicMurmurC2;
	// body
	const uint32 * p_blocks = (const uint32 *)(data + nblocks*4);
	for(i = -nblocks; i; i++) {
		uint32 k1 = getblock(p_blocks, i);
		k1 *= c1;
		k1 = /*_rotl*/SBits::Rotl(k1, 15);
		k1 *= c2;
    
		h1 ^= k1;
		h1 = /*_rotl*/SBits::Rotl(h1, 13); 
		h1 = h1*5+0xe6546b64;
	}
	// tail
	const uint8 * p_tail = (const uint8 *)(data + nblocks*4);
	uint32 k1 = 0;
	switch(len & 3) {
		case 3: k1 ^= p_tail[2] << 16;
		case 2: k1 ^= p_tail[1] << 8;
		case 1: 
			k1 ^= p_tail[0];
			k1 *= c1; 
			k1 = /*_rotl*/SBits::Rotl(k1, 15); 
			k1 *= c2; 
			h1 ^= k1;
	};
	// finalization
	h1 ^= len;
	h1 = fmix32(h1);
	return h1;
}

/*static*/binary128 FASTCALL SlHash::Murmur3_128x32(const void * pData, size_t len, uint32 seed)
{
	const uint8 * p_data = (const uint8 *)pData;
	const int nblocks = len / 16;
	int i;
	uint32 h1 = seed;
	uint32 h2 = seed;
	uint32 h3 = seed;
	uint32 h4 = seed;
	uint32 c1 = 0x239b961b; 
	uint32 c2 = 0xab0e9789;
	uint32 c3 = 0x38b34ae5; 
	uint32 c4 = 0xa1e38b93;
	//----------
	// body
	const uint32 * blocks = (const uint32 *)(p_data + nblocks*16);
	for(i = -nblocks; i; i++) {
		uint32 k1 = getblock(blocks,i*4+0);
		uint32 k2 = getblock(blocks,i*4+1);
		uint32 k3 = getblock(blocks,i*4+2);
		uint32 k4 = getblock(blocks,i*4+3);
		k1 *= c1; 
		k1  = SBits::Rotl(k1,15); 
		k1 *= c2; 
		h1 ^= k1;
		
		h1 = SBits::Rotl(h1,19); 
		h1 += h2; 
		h1 = h1*5+0x561ccd1b;
		
		k2 *= c2; 
		k2  = SBits::Rotl(k2,16); 
		k2 *= c3; 
		h2 ^= k2;
		
		h2 = SBits::Rotl(h2,17); 
		h2 += h3; 
		h2 = h2*5+0x0bcaa747;
		
		k3 *= c3; 
		k3  = SBits::Rotl(k3,17); 
		k3 *= c4; 
		h3 ^= k3;
		h3 = SBits::Rotl(h3,15); 
		h3 += h4; 
		h3 = h3*5+0x96cd1c35;
		k4 *= c4; 
		k4  = SBits::Rotl(k4,18); 
		k4 *= c1; 
		h4 ^= k4;
		h4 = SBits::Rotl(h4,13); 
		h4 += h1; 
		h4 = h4*5+0x32ac3b17;
	}
	//----------
	// tail
	const uint8 * p_tail = (const uint8 *)(p_data + nblocks*16);
	uint32 k1 = 0;
	uint32 k2 = 0;
	uint32 k3 = 0;
	uint32 k4 = 0;
	switch(len & 15) {
		case 15: k4 ^= p_tail[14] << 16;
		case 14: k4 ^= p_tail[13] << 8;
		case 13: k4 ^= p_tail[12] << 0; k4 *= c4; k4  = SBits::Rotl(k4,18); k4 *= c1; h4 ^= k4;
		case 12: k3 ^= p_tail[11] << 24;
		case 11: k3 ^= p_tail[10] << 16;
		case 10: k3 ^= p_tail[ 9] << 8;
		case  9: k3 ^= p_tail[ 8] << 0; k3 *= c3; k3  = SBits::Rotl(k3,17); k3 *= c4; h3 ^= k3;
		case  8: k2 ^= p_tail[ 7] << 24;
		case  7: k2 ^= p_tail[ 6] << 16;
		case  6: k2 ^= p_tail[ 5] << 8;
		case  5: k2 ^= p_tail[ 4] << 0; k2 *= c2; k2  = SBits::Rotl(k2,16); k2 *= c3; h2 ^= k2;
		case  4: k1 ^= p_tail[ 3] << 24;
		case  3: k1 ^= p_tail[ 2] << 16;
		case  2: k1 ^= p_tail[ 1] << 8;
		case  1: k1 ^= p_tail[ 0] << 0; k1 *= c1; k1  = SBits::Rotl(k1,15); k1 *= c2; h1 ^= k1;
	};
	//----------
	// finalization

	h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;

	h1 = fmix32(h1);
	h2 = fmix32(h2);
	h3 = fmix32(h3);
	h4 = fmix32(h4);

	h1 += h2; 
	h1 += h3; 
	h1 += h4;
	h2 += h1; 
	h3 += h1; 
	h4 += h1;
	binary128 result;
	reinterpret_cast<uint32 *>(result.D)[0] = h1;
	reinterpret_cast<uint32 *>(result.D)[1] = h2;
	reinterpret_cast<uint32 *>(result.D)[2] = h3;
	reinterpret_cast<uint32 *>(result.D)[3] = h4;
	return result;
}

/*static*/binary128 FASTCALL SlHash::Murmur3_128x64(const void * pData, size_t len, uint32 seed)
{
	const uint8 * p_data = (const uint8 *)pData;
	const int nblocks = len / 16;
	int i;
	uint64 h1 = seed;
	uint64 h2 = seed;
	uint64 c1 = 0x87c37b91114253d5ULL;
	uint64 c2 = 0x4cf5ad432745937fULL;
	//----------
	// body
	const uint64 * blocks = reinterpret_cast<const uint64 *>(p_data);
	for(i = 0; i < nblocks; i++) {
		uint64 k1 = getblock(blocks,i*2+0);
		uint64 k2 = getblock(blocks,i*2+1);
		k1 *= c1; 
		k1  = SBits::Rotl(k1,31); 
		k1 *= c2; 
		h1 ^= k1;
		h1 = SBits::Rotl(h1,27); 
		h1 += h2; 
		h1 = h1*5+0x52dce729;
		k2 *= c2; 
		k2  = SBits::Rotl(k2,33); 
		k2 *= c1; 
		h2 ^= k2;
		h2 = SBits::Rotl(h2,31); 
		h2 += h1; 
		h2 = h2*5+0x38495ab5;
	}
	//----------
	// tail
	const uint8 * p_tail = (const uint8 *)(p_data + nblocks*16);
	uint64 k1 = 0;
	uint64 k2 = 0;
	switch(len & 15) {
		case 15: k2 ^= (uint64)(p_tail[14]) << 48;
		case 14: k2 ^= (uint64)(p_tail[13]) << 40;
		case 13: k2 ^= (uint64)(p_tail[12]) << 32;
		case 12: k2 ^= (uint64)(p_tail[11]) << 24;
		case 11: k2 ^= (uint64)(p_tail[10]) << 16;
		case 10: k2 ^= (uint64)(p_tail[ 9]) << 8;
		case  9: k2 ^= (uint64)(p_tail[ 8]) << 0; k2 *= c2; k2  = SBits::Rotl(k2,33); k2 *= c1; h2 ^= k2;

		case  8: k1 ^= (uint64)(p_tail[ 7]) << 56;
		case  7: k1 ^= (uint64)(p_tail[ 6]) << 48;
		case  6: k1 ^= (uint64)(p_tail[ 5]) << 40;
		case  5: k1 ^= (uint64)(p_tail[ 4]) << 32;
		case  4: k1 ^= (uint64)(p_tail[ 3]) << 24;
		case  3: k1 ^= (uint64)(p_tail[ 2]) << 16;
		case  2: k1 ^= (uint64)(p_tail[ 1]) << 8;
		case  1: k1 ^= (uint64)(p_tail[ 0]) << 0; k1 *= c1; k1  = SBits::Rotl(k1, 31); k1 *= c2; h1 ^= k1;
	};
	// finalization
	h1 ^= len; h2 ^= len;
	h1 += h2;
	h2 += h1;
	h1 = fmix64(h1);
	h2 = fmix64(h2);
	h1 += h2;
	h2 += h1;
	binary128 result;
	reinterpret_cast<uint64 *>(result.D)[0] = h1;
	reinterpret_cast<uint64 *>(result.D)[1] = h2;
	return result;
}

SlHash::State::State() : Flags(fEmpty)
{
}

SlHash::State::~State()
{
}

SlHash::State & SlHash::State::Z()
{
	Flags |= fEmpty;
	Result.R64 = 0;
	return *this;
}
//
// CRC model
//
struct CrcModel { // cm_t
	enum {
		fRefIn  = 0x0001,
		fRefOut = 0x0002
	};
	CrcModel(uint width, uint poly, uint flags, uint xorot, uint init) : cm_init(init), cm_reg(cm_init), Width(width), Poly(poly),
		cm_xorot(xorot), cm_refin(LOGIC(flags & fRefIn)), cm_refot(LOGIC(flags & fRefOut))
	{
		// Reflect initial value
		if(cm_refin)
			cm_reg = reflect(cm_reg, Width);
	}
	//
	// Returns the value v with the bottom b [0,32] bits reflected.
	// Example: reflect(0x3e23L,3) == 0x3e26
	//
	static uint32 FASTCALL reflect(uint32 v, const uint b)
	{
		uint32 t = v;
		const uint b_1 = (b-1);
		for(uint i = 0; i <= b_1; i++) {
			if(t & 1)
				v |= (1 << (b_1-i));
			else
				v &= ~(1 << (b_1-i));
			t >>= 1;
		}
		return v;
	}
	//
	// Returns a longword whose value is (2^p_cm->cm_width)-1.
	// The trick is to do this portably (e.g. without doing <<32).
	//
	uint32 widmask() const
	{
		return (((1 << (Width-1)) - 1) << 1) | 1;
	}
	uint32 FASTCALL Tab(uint index)
	{
		const  uint32 topbit = (1 << (Width-1));
		uint32 inbyte = static_cast<uint32>(index);
		if(cm_refin)
			inbyte = reflect(inbyte, 8);
		uint32 c = inbyte << (Width - 8);
		for(uint i = 0; i < 8; i++)
			c = (c & topbit) ? (Poly ^ (c << 1)) : (c << 1);
		if(cm_refin)
			c = reflect(c, Width);
		return (c & widmask());
	}
	//
	// Степень алгоритма, выраженная в битах.  Она всегда на единицу
	// меньше длины полинома, но равна его степени.
	//
	uint8 Width; // Parameter: Width in bits [8,32]
	//
	// Собственно полином. Это битовая величина, которая для удобства
	// может быть представлена шестнадцатеричным числом. Старший бит
	// при этом опускается. Например, если используется полином 10110, то
	// он обозначается числом "06h". Важной особенностью данного параметра является то,
	// что он всегда представляет собой необращенный
	// полином, младшая часть этого параметра во время вычислений всегда
	// является наименее значащими битами делителя вне зависимости от
	// того, какой – "зеркальный" или прямой алгоритм моделируется.
	//
	uint32 Poly; // Parameter: The algorithm's polynomial
	//
	// Этот параметр определяет исходное содержимое регистра на момент
	// запуска вычислений. Именно это значение должно быть занесено в
	// регистр в прямой табличном алгоритме. В принципе, в табличных алгоритмах
	// мы всегда может считать, что регистр инициализируется
	// нулевым значением, а начальное значение комбинируется по XOR с
	// содержимым регистра после N цикла.
	//
	uint32 cm_init;		// Parameter: Initial register value
	//
	// Логический параметр. Если он имеет значение False, байты
	// сообщения обрабатываются, начиная с 7 бита, который считается
	// наиболее значащим, а наименее значащим считается бит 0.
	// Если параметр имеет значение True, то каждый байт перед обработкой обращается.
	//
	bool  cm_refin;		// Parameter: Reflect input bytes?
	//
	// Логический параметр. Если он имеет значение False, то
	// конечное содержимое регистра сразу передается на стадию XorOut, в
	// противном случае, когда параметр имеет значение True,
	// содержимое регистра обращается перед передачей на следующую
	// стадию вычислений.
	//
	bool  cm_refot;		// Parameter: Reflect output CRC?
	//
	// W битное значение, обозначаемое шестнадцатеричным числом. Оно
	// комбинируется с конечным содержимым регистра (после стадии
	// RefOut), прежде чем будет получено окончательное значение контрольной суммы.
	//
	uint32 cm_xorot;		// Parameter: XOR this to output CRC
	//
	// Это поле, собственно, не является частью определения алгоритма, и, в
	// случае противоречия с ним предшествующих параметров, именно эти
	// предыдущие параметры имеют наибольший приоритет.  Данное поле
	// служит контрольным значением, которое может быть использовано для
	// слабой проверки правильности реализации алгоритма.  Поле содержит
	// контрольную сумму, рассчитанную для ASCII строки "123456789"
	// (шестнадцатеричные значение "313233...").
	//
	uint32 cm_reg;          // Context: Context during execution
};
//
//
//
SCRC32::SCRC32() : P_Tab(0)
{
}

SCRC32::~SCRC32()
{
	delete P_Tab;
}
// 
// #define CRC_POLY_xxxx
// 
// The constants of the form CRC_POLY_xxxx define the polynomials for some well known CRC calculations.
// 
// @v11.7.11 (replaced with SlConst::CrcPoly_ANSI16) #define CRC_POLY_16     0xA001                 // ANSI-16
// @v11.7.11 (replaced with SlConst::CrcPoly_CCITT32) #define CRC_POLY_32     0xEDB88320ul           // CCITT-32
#define CRC_POLY_64     0x42F0E1EBA9EA3693ull
// @v11.7.11 (replaced with SlConst::CrcPoly_CCITT16) #define CRC_POLY_CCITT  0x1021                 // CCITT-16
#define CRC_POLY_DNP    0xA6BC
#define CRC_POLY_KERMIT 0x8408
#define CRC_POLY_SICK   0x8005
// 
// The constants of the form CRC_START_xxxx define the values that are used for
// initialization of a CRC value for common used calculation methods.
// 
#define CRC_START_8          0x00
#define CRC_START_16         0x0000
#define CRC_START_MODBUS     0xFFFF
#define CRC_START_XMODEM     0x0000
#define CRC_START_CCITT_1D0F 0x1D0F
#define CRC_START_CCITT_FFFF 0xFFFF
#define CRC_START_KERMIT     0x0000
#define CRC_START_SICK       0x0000
#define CRC_START_DNP        0x0000
#define CRC_START_32         0xFFFFFFFFul
#define CRC_START_64_ECMA    0x0000000000000000ull
#define CRC_START_64_WE      0xFFFFFFFFFFFFFFFFull

/*static*/uint8 STDCALL SlHash::CRC8(State * pS, const void * pData, size_t dataLen)
{
	const uint8 init_val = CRC_START_8;
	uint8 result = 0;
	if(!pS) {
		State st;
		result = CRC8(&st, pData, dataLen);
	}
	else {
		// 
		// The SHT75 humidity sensor is capable of calculating an 8 bit CRC checksum to
		// ensure data integrity. The lookup table crc_table[] is used to recalculate the CRC. 
		// 
		static const uint8 sht75_crc_table[] = {
			0,   49,  98,  83,  196, 245, 166, 151, 185, 136, 219, 234, 125, 76,  31,  46,
			67,  114, 33,  16,  135, 182, 229, 212, 250, 203, 152, 169, 62,  15,  92,  109,
			134, 183, 228, 213, 66,  115, 32,  17,  63,  14,  93,  108, 251, 202, 153, 168,
			197, 244, 167, 150, 1,   48,  99,  82,  124, 77,  30,  47,  184, 137, 218, 235,
			61,  12,  95,  110, 249, 200, 155, 170, 132, 181, 230, 215, 64,  113, 34,  19,
			126, 79,  28,  45,  186, 139, 216, 233, 199, 246, 165, 148, 3,   50,  97,  80,
			187, 138, 217, 232, 127, 78,  29,  44,  2,   51,  96,  81,  198, 247, 164, 149,
			248, 201, 154, 171, 60,  13,  94,  111, 65,  112, 35,  18,  133, 180, 231, 214,
			122, 75,  24,  41,  190, 143, 220, 237, 195, 242, 161, 144, 7,   54,  101, 84,
			57,  8,   91,  106, 253, 204, 159, 174, 128, 177, 226, 211, 68,  117, 38,  23,
			252, 205, 158, 175, 56,  9,   90,  107, 69,  116, 39,  22,  129, 176, 227, 210,
			191, 142, 221, 236, 123, 74,  25,  40,  6,   55,  100, 85,  194, 243, 160, 145,
			71,  118, 37,  20,  131, 178, 225, 208, 254, 207, 156, 173, 58,  11,  88,  105,
			4,   53,  102, 87,  192, 241, 162, 147, 189, 140, 223, 238, 121, 72,  27,  42,
			193, 240, 163, 146, 5,   52,  103, 86,  120, 73,  26,  43,  188, 141, 222, 239,
			130, 179, 224, 209, 70,  119, 36,  21,  59,  10,  89,  104, 255, 206, 157, 172
		};
		const uint8 * p_tab = sht75_crc_table;
		if(pS->Flags & pS->fEmpty) {
			pS->Result.R8 = init_val;
			pS->Flags &= ~pS->fEmpty;
		}
		else
			result = pS->Result.R8;
		#define DO1(buf)  result = p_tab[result ^ (*p_buf++)];
		#define DO2(buf)  DO1(buf); DO1(buf);
		#define DO4(buf)  DO2(buf); DO2(buf);
		#define DO8(buf)  DO4(buf); DO4(buf);
		if(pData && dataLen) {
			const uint8 * p_buf = PTR8C(pData);
			while(dataLen >= 8) {
				DO8(pData);
				dataLen -= 8;
			}
			if(dataLen) do {
				DO1(pData);
			} while(--dataLen);
		}
		#undef DO1
		#undef DO2
		#undef DO4
		#undef DO8
		pS->Result.R8 = result;
	}
	return result;
}

/*static*/uint32 STDCALL SlHash::CRC32(State * pS, const void * pData, size_t dataLen)
{
	uint32 result = 0;
	if(!pS) {
		State st;
		CRC32(&st, pData, dataLen);
		result = CRC32(&st, 0, 0);
	}
	else {
		if(!State::P_Tab_Crc32) {
			State::P_Tab_Crc32 = static_cast<uint32 *>(SAlloc::M(sizeof(State::P_Tab_Crc32) * 256));
			CrcModel cm(32, SlConst::CrcPoly_BZIP2, CrcModel::fRefIn, 0, 0);
            for(uint i = 0; i < 256; i++) {
				State::P_Tab_Crc32[i] = cm.Tab(i);
            }
		}
		const uint32 * p_tab = State::P_Tab_Crc32;
		if(pS->Flags & pS->fEmpty) {
			pS->Result.R32 = (pData && dataLen) ? 0xffffffffUL : 0UL;
			pS->Flags &= ~pS->fEmpty;
		}
		result = pS->Result.R32;
		#define DO1(buf)  result = p_tab[(result ^ (*p_buf++)) & 0xff] ^ (result >> 8);
		#define DO2(buf)  DO1(buf); DO1(buf);
		#define DO4(buf)  DO2(buf); DO2(buf);
		#define DO8(buf)  DO4(buf); DO4(buf);
		if(pData && dataLen) {
			const uint8 * p_buf = PTR8C(pData);
			while(dataLen >= 8) {
				DO8(pData);
				dataLen -= 8;
			}
			if(dataLen) do {
				DO1(pData);
			} while(--dataLen);
		}
		else 
			result = (result ^ 0xffffffffUL);
		#undef DO1
		#undef DO2
		#undef DO4
		#undef DO8
		pS->Result.R32 = result;
	}
	return result;
}
//
// @construction CRC64 {
//
//
// Reverse the bytes in a 64-bit word. 
//
/*static inline uint64 rev8(uint64 a) 
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(a);
#else
    uint64_t m;
    m = UINT64_C(0xff00ff00ff00ff);
    a = ((a >> 8) & m) | (a & m) << 8;
    m = UINT64_C(0xffff0000ffff);
    a = ((a >> 16) & m) | (a & m) << 16;
    return a >> 32 | a << 32;
#endif
}*/
// 
// Calculate a non-inverted CRC multiple bytes at a time on a little-endian
// architecture. If you need inverted CRC, invert *before* calling and invert *after* calling.
// 64 bit crc = process 8 bytes at once;
// 
/*static*/uint64 SlHash::State::CrcSpeed64le(/*const uint64 pTableLE[8][256],*/uint64 crc, const void * buf, size_t len) 
{
	const uchar * next = static_cast<const uchar *>(buf);
	// process individual bytes until we reach an 8-byte aligned pointer 
	while(len && ((uintptr_t)next & 7) != 0) {
		crc = P_Tab_Crc64[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
		len--;
	}
	// fast middle processing, 8 bytes (aligned!) per loop 
	while(len >= 8) {
		crc ^= *(uint64_t *)next;
		crc = P_Tab_Crc64[7][crc & 0xff] ^ P_Tab_Crc64[6][(crc >> 8) & 0xff] ^ P_Tab_Crc64[5][(crc >> 16) & 0xff] ^
			P_Tab_Crc64[4][(crc >> 24) & 0xff] ^ P_Tab_Crc64[3][(crc >> 32) & 0xff] ^ P_Tab_Crc64[2][(crc >> 40) & 0xff] ^
			P_Tab_Crc64[1][(crc >> 48) & 0xff] ^ P_Tab_Crc64[0][crc >> 56];
		next += 8;
		len -= 8;
	}
	// process remaining bytes (can't be larger than 8) 
	while(len) {
		crc = P_Tab_Crc64[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
		len--;
	}
	return crc;
}
// 
// Calculate a non-inverted CRC eight bytes at a time on a big-endian architecture.
// 
/*static*/uint64 SlHash::State::CrcSpeed64be(/*const uint64 pTableBE[8][256],*/uint64 crc, const void * buf, size_t len) 
{
	const uchar * next = static_cast<const uchar *>(buf);
	crc = sbswap64(crc);
	while(len && ((uintptr_t)next & 7) != 0) {
		crc = P_Tab_Crc64[0][(crc >> 56) ^ *next++] ^ (crc << 8);
		len--;
	}
	while(len >= 8) {
		crc ^= *(uint64_t *)next;
		crc = P_Tab_Crc64[0][crc & 0xff] ^ P_Tab_Crc64[1][(crc >> 8) & 0xff] ^ P_Tab_Crc64[2][(crc >> 16) & 0xff] ^
			P_Tab_Crc64[3][(crc >> 24) & 0xff] ^ P_Tab_Crc64[4][(crc >> 32) & 0xff] ^ P_Tab_Crc64[5][(crc >> 40) & 0xff] ^
			P_Tab_Crc64[6][(crc >> 48) & 0xff] ^ P_Tab_Crc64[7][crc >> 56];
		next += 8;
		len -= 8;
	}
	while(len) {
		crc = P_Tab_Crc64[0][(crc >> 56) ^ *next++] ^ (crc << 8);
		len--;
	}
	return sbswap64(crc);
}

// @v12.3.1 (replaced with SlConst::CrcPoly_64_) #define POLY_CRC64 0xad93d23594c935a9ULL
/******************** BEGIN GENERATED PYCRC FUNCTIONS ********************/
/**
 * Generated on Sun Dec 21 14:14:07 2014,
 * by pycrc v0.8.2, https://www.tty1.net/pycrc/
 *
 * LICENSE ON GENERATED CODE:
 * ==========================
 * As of version 0.6, pycrc is released under the terms of the MIT licence.
 * The code generated by pycrc is not considered a substantial portion of the
 * software, therefore the author of pycrc will not claim any copyright on
 * the generated code.
 * ==========================
 *
 * CRC configuration:
 *    Width        = 64
 *    Poly         = 0xad93d23594c935a9
 *    XorIn        = 0xffffffffffffffff
 *    ReflectIn    = True
 *    XorOut       = 0x0000000000000000
 *    ReflectOut   = True
 *    Algorithm    = bit-by-bit-fast
 *
 * Modifications after generation (by matt):
 *   - included finalize step in-line with update for single-call generation
 *   - re-worked some inner variable architectures
 *   - adjusted function parameters to match expected prototypes.
 *****************************************************************************/
// 
// Reflect all bits of a \a data word of \a data_len bytes.
// 
// \param data         The data word to be reflected.
// \param data_len     The width of \a data expressed in number of bits.
// \return             The reflected data.
// 
/*static inline uint64 crc_reflect(uint64 data, size_t data_len) 
{
	uint64 ret = data & 0x01;
	for(size_t i = 1; i < data_len; i++) {
		data >>= 1;
		ret = (ret << 1) | (data & 0x01);
	}
	return ret;
}*/
// 
// Update the crc value with new data.
// 
// \param crc      The current crc value.
// \param data     Pointer to a buffer of \a data_len bytes.
// \param data_len Number of bytes in the \a data buffer.
// \return         The updated crc value.
// 
uint64 _crc64_proc(uint64 crc, const void * in_data, const uint64_t len) 
{
	const uint8 * data = static_cast<const uint8 *>(in_data);
	uint64 bit;
	for(uint64 offset = 0; offset < len; offset++) {
		uint8 c = data[offset];
		for(uint8 i = 0x01; i & 0xff; i <<= 1) {
			bit = crc & 0x8000000000000000;
			if(c & i) {
				bit = !bit;
			}
			crc <<= 1;
			if(bit) {
				crc ^= SlConst::CrcPoly_64_;
			}
		}
		crc &= 0xffffffffffffffff;
	}
	crc = crc & 0xffffffffffffffff;
	// 
	// Reflect all bits of a \a data word of \a data_len bytes.
	// 
	// \param data         The data word to be reflected.
	// \param data_len     The width of \a data expressed in number of bits.
	// \return             The reflected data.
	// 
	//static inline uint64 crc_reflect(uint64 data, size_t data_len) 
	{
		uint64 ret = crc & 0x01;
		for(size_t i = 1; i < 64; i++) {
			crc >>= 1;
			ret = (ret << 1) | (crc & 0x01);
		}
		return ret ^ 0x0000000000000000;
	}
	//return crc_reflect(crc, 64) ^ 0x0000000000000000;
}

/*static*/void SlHash::State::CrcSpeed64leInit(uint64 (*crcfn)(uint64, const void *, const uint64)/*, uint64 table[8][256]*/)
{
	uint64 crc;
	// generate CRCs for all single byte sequences 
	for(int n = 0; n < 256; n++) {
		P_Tab_Crc64[0][n] = crcfn(0, &n, 1);
	}
	// generate nested CRC table for future slice-by-8 lookup 
	for(int n = 0; n < 256; n++) {
		crc = P_Tab_Crc64[0][n];
		for(int k = 1; k < 8; k++) {
			crc = P_Tab_Crc64[0][crc & 0xff] ^ (crc >> 8);
			P_Tab_Crc64[k][n] = crc;
		}
	}
}

/*static*/void SlHash::State::CrcSpeed64beInit(uint64 (*crcfn)(uint64, const void *, const uint64)/*, uint64 big_table[8][256]*/)
{
	// Create the little endian table then reverse all the entires
	CrcSpeed64leInit(crcfn/*, big_table*/);
	for(int k = 0; k < 8; k++) {
		for(int n = 0; n < 256; n++) {
			P_Tab_Crc64[k][n] = sbswap64(P_Tab_Crc64[k][n]);
		}
	}
}

/*static*/uint64 STDCALL SlHash::CRC64(State * pS, const void * pData, size_t dataLen) // Реализация заимствована из redis
{
	uint64 result = 0;
	if(!pS) {
		State st;
		CRC64(&st, pData, dataLen); // @recursion
		result = CRC64(&st, 0, 0); // @recursion
	}
	else {
		if(!State::P_Tab_Crc64[0]) {
			for(uint i = 0; i < SIZEOFARRAY(State::P_Tab_Crc64); i++) {
				State::P_Tab_Crc64[i] = static_cast<uint64 *>(SAlloc::M(sizeof(uint64) * 256));
			}
			if(SLS.SSys.IsBigEndian) {
				State::CrcSpeed64beInit(_crc64_proc/*, reinterpret_cast<uint64 (*)[256]>(State::P_Tab_Crc64)*/);
			}
			else {
				State::CrcSpeed64leInit(_crc64_proc/*, reinterpret_cast<uint64 (*)[256]>(State::P_Tab_Crc64)*/);
			}
		}
		//const uint64 * p_tab = State::P_Tab_Crc64;
		if(pS->Flags & pS->fEmpty) {
			pS->Result.R64 = 0ULL;
			pS->Flags &= ~pS->fEmpty;
		}
		result = pS->Result.R64;
		if(pData && dataLen) {
			const uint8 * p_buf = PTR8C(pData);
			if(SLS.SSys.IsBigEndian) {
				result = State::CrcSpeed64be(/*reinterpret_cast<uint64 (*)[256]>(State::P_Tab_Crc64),*/result, pData, dataLen);
			}
			else {
				result = State::CrcSpeed64le(/*reinterpret_cast<uint64 (*)[256]>(State::P_Tab_Crc64),*/result, pData, dataLen);
			}
		}
		else {
			//result = (result & 0x00ffffffU);
		}
		pS->Result.R64 = result;
	}
	return result;
}

/*static*/uint32 STDCALL SlHash::CRC24(State * pS, const void * pData, size_t dataLen)
{
	uint32 result = 0;
	if(!pS) {
		State st;
		CRC24(&st, pData, dataLen); // @recursion
		result = CRC24(&st, 0, 0); // @recursion
	}
	else {
		static const uint32 CRC24_T0[256] = {
		   0x00000000, 0x00FB4C86, 0x000DD58A, 0x00F6990C, 0x00E1E693, 0x001AAA15, 0x00EC3319,
		   0x00177F9F, 0x003981A1, 0x00C2CD27, 0x0034542B, 0x00CF18AD, 0x00D86732, 0x00232BB4,
		   0x00D5B2B8, 0x002EFE3E, 0x00894EC5, 0x00720243, 0x00849B4F, 0x007FD7C9, 0x0068A856,
		   0x0093E4D0, 0x00657DDC, 0x009E315A, 0x00B0CF64, 0x004B83E2, 0x00BD1AEE, 0x00465668,
		   0x005129F7, 0x00AA6571, 0x005CFC7D, 0x00A7B0FB, 0x00E9D10C, 0x00129D8A, 0x00E40486,
		   0x001F4800, 0x0008379F, 0x00F37B19, 0x0005E215, 0x00FEAE93, 0x00D050AD, 0x002B1C2B,
		   0x00DD8527, 0x0026C9A1, 0x0031B63E, 0x00CAFAB8, 0x003C63B4, 0x00C72F32, 0x00609FC9,
		   0x009BD34F, 0x006D4A43, 0x009606C5, 0x0081795A, 0x007A35DC, 0x008CACD0, 0x0077E056,
		   0x00591E68, 0x00A252EE, 0x0054CBE2, 0x00AF8764, 0x00B8F8FB, 0x0043B47D, 0x00B52D71,
		   0x004E61F7, 0x00D2A319, 0x0029EF9F, 0x00DF7693, 0x00243A15, 0x0033458A, 0x00C8090C,
		   0x003E9000, 0x00C5DC86, 0x00EB22B8, 0x00106E3E, 0x00E6F732, 0x001DBBB4, 0x000AC42B,
		   0x00F188AD, 0x000711A1, 0x00FC5D27, 0x005BEDDC, 0x00A0A15A, 0x00563856, 0x00AD74D0,
		   0x00BA0B4F, 0x004147C9, 0x00B7DEC5, 0x004C9243, 0x00626C7D, 0x009920FB, 0x006FB9F7,
		   0x0094F571, 0x00838AEE, 0x0078C668, 0x008E5F64, 0x007513E2, 0x003B7215, 0x00C03E93,
		   0x0036A79F, 0x00CDEB19, 0x00DA9486, 0x0021D800, 0x00D7410C, 0x002C0D8A, 0x0002F3B4,
		   0x00F9BF32, 0x000F263E, 0x00F46AB8, 0x00E31527, 0x001859A1, 0x00EEC0AD, 0x00158C2B,
		   0x00B23CD0, 0x00497056, 0x00BFE95A, 0x0044A5DC, 0x0053DA43, 0x00A896C5, 0x005E0FC9,
		   0x00A5434F, 0x008BBD71, 0x0070F1F7, 0x008668FB, 0x007D247D, 0x006A5BE2, 0x00911764,
		   0x00678E68, 0x009CC2EE, 0x00A44733, 0x005F0BB5, 0x00A992B9, 0x0052DE3F, 0x0045A1A0,
		   0x00BEED26, 0x0048742A, 0x00B338AC, 0x009DC692, 0x00668A14, 0x00901318, 0x006B5F9E,
		   0x007C2001, 0x00876C87, 0x0071F58B, 0x008AB90D, 0x002D09F6, 0x00D64570, 0x0020DC7C,
		   0x00DB90FA, 0x00CCEF65, 0x0037A3E3, 0x00C13AEF, 0x003A7669, 0x00148857, 0x00EFC4D1,
		   0x00195DDD, 0x00E2115B, 0x00F56EC4, 0x000E2242, 0x00F8BB4E, 0x0003F7C8, 0x004D963F,
		   0x00B6DAB9, 0x004043B5, 0x00BB0F33, 0x00AC70AC, 0x00573C2A, 0x00A1A526, 0x005AE9A0,
		   0x0074179E, 0x008F5B18, 0x0079C214, 0x00828E92, 0x0095F10D, 0x006EBD8B, 0x00982487,
		   0x00636801, 0x00C4D8FA, 0x003F947C, 0x00C90D70, 0x003241F6, 0x00253E69, 0x00DE72EF,
		   0x0028EBE3, 0x00D3A765, 0x00FD595B, 0x000615DD, 0x00F08CD1, 0x000BC057, 0x001CBFC8,
		   0x00E7F34E, 0x00116A42, 0x00EA26C4, 0x0076E42A, 0x008DA8AC, 0x007B31A0, 0x00807D26,
		   0x009702B9, 0x006C4E3F, 0x009AD733, 0x00619BB5, 0x004F658B, 0x00B4290D, 0x0042B001,
		   0x00B9FC87, 0x00AE8318, 0x0055CF9E, 0x00A35692, 0x00581A14, 0x00FFAAEF, 0x0004E669,
		   0x00F27F65, 0x000933E3, 0x001E4C7C, 0x00E500FA, 0x001399F6, 0x00E8D570, 0x00C62B4E,
		   0x003D67C8, 0x00CBFEC4, 0x0030B242, 0x0027CDDD, 0x00DC815B, 0x002A1857, 0x00D154D1,
		   0x009F3526, 0x006479A0, 0x0092E0AC, 0x0069AC2A, 0x007ED3B5, 0x00859F33, 0x0073063F,
		   0x00884AB9, 0x00A6B487, 0x005DF801, 0x00AB610D, 0x00502D8B, 0x00475214, 0x00BC1E92,
		   0x004A879E, 0x00B1CB18, 0x00167BE3, 0x00ED3765, 0x001BAE69, 0x00E0E2EF, 0x00F79D70,
		   0x000CD1F6, 0x00FA48FA, 0x0001047C, 0x002FFA42, 0x00D4B6C4, 0x00222FC8, 0x00D9634E,
		   0x00CE1CD1, 0x00355057, 0x00C3C95B, 0x003885DD 
		};

		static const uint32 CRC24_T1[256] = {
		   0x00000000, 0x00488F66, 0x00901ECD, 0x00D891AB, 0x00DB711C, 0x0093FE7A, 0x004B6FD1,
		   0x0003E0B7, 0x00B6E338, 0x00FE6C5E, 0x0026FDF5, 0x006E7293, 0x006D9224, 0x00251D42,
		   0x00FD8CE9, 0x00B5038F, 0x006CC771, 0x00244817, 0x00FCD9BC, 0x00B456DA, 0x00B7B66D,
		   0x00FF390B, 0x0027A8A0, 0x006F27C6, 0x00DA2449, 0x0092AB2F, 0x004A3A84, 0x0002B5E2,
		   0x00015555, 0x0049DA33, 0x00914B98, 0x00D9C4FE, 0x00D88EE3, 0x00900185, 0x0048902E,
		   0x00001F48, 0x0003FFFF, 0x004B7099, 0x0093E132, 0x00DB6E54, 0x006E6DDB, 0x0026E2BD,
		   0x00FE7316, 0x00B6FC70, 0x00B51CC7, 0x00FD93A1, 0x0025020A, 0x006D8D6C, 0x00B44992,
		   0x00FCC6F4, 0x0024575F, 0x006CD839, 0x006F388E, 0x0027B7E8, 0x00FF2643, 0x00B7A925,
		   0x0002AAAA, 0x004A25CC, 0x0092B467, 0x00DA3B01, 0x00D9DBB6, 0x009154D0, 0x0049C57B,
		   0x00014A1D, 0x004B5141, 0x0003DE27, 0x00DB4F8C, 0x0093C0EA, 0x0090205D, 0x00D8AF3B,
		   0x00003E90, 0x0048B1F6, 0x00FDB279, 0x00B53D1F, 0x006DACB4, 0x002523D2, 0x0026C365,
		   0x006E4C03, 0x00B6DDA8, 0x00FE52CE, 0x00279630, 0x006F1956, 0x00B788FD, 0x00FF079B,
		   0x00FCE72C, 0x00B4684A, 0x006CF9E1, 0x00247687, 0x00917508, 0x00D9FA6E, 0x00016BC5,
		   0x0049E4A3, 0x004A0414, 0x00028B72, 0x00DA1AD9, 0x009295BF, 0x0093DFA2, 0x00DB50C4,
		   0x0003C16F, 0x004B4E09, 0x0048AEBE, 0x000021D8, 0x00D8B073, 0x00903F15, 0x00253C9A,
		   0x006DB3FC, 0x00B52257, 0x00FDAD31, 0x00FE4D86, 0x00B6C2E0, 0x006E534B, 0x0026DC2D,
		   0x00FF18D3, 0x00B797B5, 0x006F061E, 0x00278978, 0x002469CF, 0x006CE6A9, 0x00B47702,
		   0x00FCF864, 0x0049FBEB, 0x0001748D, 0x00D9E526, 0x00916A40, 0x00928AF7, 0x00DA0591,
		   0x0002943A, 0x004A1B5C, 0x0096A282, 0x00DE2DE4, 0x0006BC4F, 0x004E3329, 0x004DD39E,
		   0x00055CF8, 0x00DDCD53, 0x00954235, 0x002041BA, 0x0068CEDC, 0x00B05F77, 0x00F8D011,
		   0x00FB30A6, 0x00B3BFC0, 0x006B2E6B, 0x0023A10D, 0x00FA65F3, 0x00B2EA95, 0x006A7B3E,
		   0x0022F458, 0x002114EF, 0x00699B89, 0x00B10A22, 0x00F98544, 0x004C86CB, 0x000409AD,
		   0x00DC9806, 0x00941760, 0x0097F7D7, 0x00DF78B1, 0x0007E91A, 0x004F667C, 0x004E2C61,
		   0x0006A307, 0x00DE32AC, 0x0096BDCA, 0x00955D7D, 0x00DDD21B, 0x000543B0, 0x004DCCD6,
		   0x00F8CF59, 0x00B0403F, 0x0068D194, 0x00205EF2, 0x0023BE45, 0x006B3123, 0x00B3A088,
		   0x00FB2FEE, 0x0022EB10, 0x006A6476, 0x00B2F5DD, 0x00FA7ABB, 0x00F99A0C, 0x00B1156A,
		   0x006984C1, 0x00210BA7, 0x00940828, 0x00DC874E, 0x000416E5, 0x004C9983, 0x004F7934,
		   0x0007F652, 0x00DF67F9, 0x0097E89F, 0x00DDF3C3, 0x00957CA5, 0x004DED0E, 0x00056268,
		   0x000682DF, 0x004E0DB9, 0x00969C12, 0x00DE1374, 0x006B10FB, 0x00239F9D, 0x00FB0E36,
		   0x00B38150, 0x00B061E7, 0x00F8EE81, 0x00207F2A, 0x0068F04C, 0x00B134B2, 0x00F9BBD4,
		   0x00212A7F, 0x0069A519, 0x006A45AE, 0x0022CAC8, 0x00FA5B63, 0x00B2D405, 0x0007D78A,
		   0x004F58EC, 0x0097C947, 0x00DF4621, 0x00DCA696, 0x009429F0, 0x004CB85B, 0x0004373D,
		   0x00057D20, 0x004DF246, 0x009563ED, 0x00DDEC8B, 0x00DE0C3C, 0x0096835A, 0x004E12F1,
		   0x00069D97, 0x00B39E18, 0x00FB117E, 0x002380D5, 0x006B0FB3, 0x0068EF04, 0x00206062,
		   0x00F8F1C9, 0x00B07EAF, 0x0069BA51, 0x00213537, 0x00F9A49C, 0x00B12BFA, 0x00B2CB4D,
		   0x00FA442B, 0x0022D580, 0x006A5AE6, 0x00DF5969, 0x0097D60F, 0x004F47A4, 0x0007C8C2,
		   0x00042875, 0x004CA713, 0x009436B8, 0x00DCB9DE 
		};

		static const uint32 CRC24_T2[256] = {
		   0x00000000, 0x00D70983, 0x00555F80, 0x00825603, 0x0051F286, 0x0086FB05, 0x0004AD06,
		   0x00D3A485, 0x0059A88B, 0x008EA108, 0x000CF70B, 0x00DBFE88, 0x00085A0D, 0x00DF538E,
		   0x005D058D, 0x008A0C0E, 0x00491C91, 0x009E1512, 0x001C4311, 0x00CB4A92, 0x0018EE17,
		   0x00CFE794, 0x004DB197, 0x009AB814, 0x0010B41A, 0x00C7BD99, 0x0045EB9A, 0x0092E219,
		   0x0041469C, 0x00964F1F, 0x0014191C, 0x00C3109F, 0x006974A4, 0x00BE7D27, 0x003C2B24,
		   0x00EB22A7, 0x00388622, 0x00EF8FA1, 0x006DD9A2, 0x00BAD021, 0x0030DC2F, 0x00E7D5AC,
		   0x006583AF, 0x00B28A2C, 0x00612EA9, 0x00B6272A, 0x00347129, 0x00E378AA, 0x00206835,
		   0x00F761B6, 0x007537B5, 0x00A23E36, 0x00719AB3, 0x00A69330, 0x0024C533, 0x00F3CCB0,
		   0x0079C0BE, 0x00AEC93D, 0x002C9F3E, 0x00FB96BD, 0x00283238, 0x00FF3BBB, 0x007D6DB8,
		   0x00AA643B, 0x0029A4CE, 0x00FEAD4D, 0x007CFB4E, 0x00ABF2CD, 0x00785648, 0x00AF5FCB,
		   0x002D09C8, 0x00FA004B, 0x00700C45, 0x00A705C6, 0x002553C5, 0x00F25A46, 0x0021FEC3,
		   0x00F6F740, 0x0074A143, 0x00A3A8C0, 0x0060B85F, 0x00B7B1DC, 0x0035E7DF, 0x00E2EE5C,
		   0x00314AD9, 0x00E6435A, 0x00641559, 0x00B31CDA, 0x003910D4, 0x00EE1957, 0x006C4F54,
		   0x00BB46D7, 0x0068E252, 0x00BFEBD1, 0x003DBDD2, 0x00EAB451, 0x0040D06A, 0x0097D9E9,
		   0x00158FEA, 0x00C28669, 0x001122EC, 0x00C62B6F, 0x00447D6C, 0x009374EF, 0x001978E1,
		   0x00CE7162, 0x004C2761, 0x009B2EE2, 0x00488A67, 0x009F83E4, 0x001DD5E7, 0x00CADC64,
		   0x0009CCFB, 0x00DEC578, 0x005C937B, 0x008B9AF8, 0x00583E7D, 0x008F37FE, 0x000D61FD,
		   0x00DA687E, 0x00506470, 0x00876DF3, 0x00053BF0, 0x00D23273, 0x000196F6, 0x00D69F75,
		   0x0054C976, 0x0083C0F5, 0x00A9041B, 0x007E0D98, 0x00FC5B9B, 0x002B5218, 0x00F8F69D,
		   0x002FFF1E, 0x00ADA91D, 0x007AA09E, 0x00F0AC90, 0x0027A513, 0x00A5F310, 0x0072FA93,
		   0x00A15E16, 0x00765795, 0x00F40196, 0x00230815, 0x00E0188A, 0x00371109, 0x00B5470A,
		   0x00624E89, 0x00B1EA0C, 0x0066E38F, 0x00E4B58C, 0x0033BC0F, 0x00B9B001, 0x006EB982,
		   0x00ECEF81, 0x003BE602, 0x00E84287, 0x003F4B04, 0x00BD1D07, 0x006A1484, 0x00C070BF,
		   0x0017793C, 0x00952F3F, 0x004226BC, 0x00918239, 0x00468BBA, 0x00C4DDB9, 0x0013D43A,
		   0x0099D834, 0x004ED1B7, 0x00CC87B4, 0x001B8E37, 0x00C82AB2, 0x001F2331, 0x009D7532,
		   0x004A7CB1, 0x00896C2E, 0x005E65AD, 0x00DC33AE, 0x000B3A2D, 0x00D89EA8, 0x000F972B,
		   0x008DC128, 0x005AC8AB, 0x00D0C4A5, 0x0007CD26, 0x00859B25, 0x005292A6, 0x00813623,
		   0x00563FA0, 0x00D469A3, 0x00036020, 0x0080A0D5, 0x0057A956, 0x00D5FF55, 0x0002F6D6,
		   0x00D15253, 0x00065BD0, 0x00840DD3, 0x00530450, 0x00D9085E, 0x000E01DD, 0x008C57DE,
		   0x005B5E5D, 0x0088FAD8, 0x005FF35B, 0x00DDA558, 0x000AACDB, 0x00C9BC44, 0x001EB5C7,
		   0x009CE3C4, 0x004BEA47, 0x00984EC2, 0x004F4741, 0x00CD1142, 0x001A18C1, 0x009014CF,
		   0x00471D4C, 0x00C54B4F, 0x001242CC, 0x00C1E649, 0x0016EFCA, 0x0094B9C9, 0x0043B04A,
		   0x00E9D471, 0x003EDDF2, 0x00BC8BF1, 0x006B8272, 0x00B826F7, 0x006F2F74, 0x00ED7977,
		   0x003A70F4, 0x00B07CFA, 0x00677579, 0x00E5237A, 0x00322AF9, 0x00E18E7C, 0x003687FF,
		   0x00B4D1FC, 0x0063D87F, 0x00A0C8E0, 0x0077C163, 0x00F59760, 0x00229EE3, 0x00F13A66,
		   0x002633E5, 0x00A465E6, 0x00736C65, 0x00F9606B, 0x002E69E8, 0x00AC3FEB, 0x007B3668,
		   0x00A892ED, 0x007F9B6E, 0x00FDCD6D, 0x002AC4EE 
		};
		static const uint32 CRC24_T3[256] = {
		   0x00000000, 0x00520936, 0x00A4126C, 0x00F61B5A, 0x004825D8, 0x001A2CEE, 0x00EC37B4,
		   0x00BE3E82, 0x006B0636, 0x00390F00, 0x00CF145A, 0x009D1D6C, 0x002323EE, 0x00712AD8,
		   0x00873182, 0x00D538B4, 0x00D60C6C, 0x0084055A, 0x00721E00, 0x00201736, 0x009E29B4,
		   0x00CC2082, 0x003A3BD8, 0x006832EE, 0x00BD0A5A, 0x00EF036C, 0x00191836, 0x004B1100,
		   0x00F52F82, 0x00A726B4, 0x00513DEE, 0x000334D8, 0x00AC19D8, 0x00FE10EE, 0x00080BB4,
		   0x005A0282, 0x00E43C00, 0x00B63536, 0x00402E6C, 0x0012275A, 0x00C71FEE, 0x009516D8,
		   0x00630D82, 0x003104B4, 0x008F3A36, 0x00DD3300, 0x002B285A, 0x0079216C, 0x007A15B4,
		   0x00281C82, 0x00DE07D8, 0x008C0EEE, 0x0032306C, 0x0060395A, 0x00962200, 0x00C42B36,
		   0x00111382, 0x00431AB4, 0x00B501EE, 0x00E708D8, 0x0059365A, 0x000B3F6C, 0x00FD2436,
		   0x00AF2D00, 0x00A37F36, 0x00F17600, 0x00076D5A, 0x0055646C, 0x00EB5AEE, 0x00B953D8,
		   0x004F4882, 0x001D41B4, 0x00C87900, 0x009A7036, 0x006C6B6C, 0x003E625A, 0x00805CD8,
		   0x00D255EE, 0x00244EB4, 0x00764782, 0x0075735A, 0x00277A6C, 0x00D16136, 0x00836800,
		   0x003D5682, 0x006F5FB4, 0x009944EE, 0x00CB4DD8, 0x001E756C, 0x004C7C5A, 0x00BA6700,
		   0x00E86E36, 0x005650B4, 0x00045982, 0x00F242D8, 0x00A04BEE, 0x000F66EE, 0x005D6FD8,
		   0x00AB7482, 0x00F97DB4, 0x00474336, 0x00154A00, 0x00E3515A, 0x00B1586C, 0x006460D8,
		   0x003669EE, 0x00C072B4, 0x00927B82, 0x002C4500, 0x007E4C36, 0x0088576C, 0x00DA5E5A,
		   0x00D96A82, 0x008B63B4, 0x007D78EE, 0x002F71D8, 0x00914F5A, 0x00C3466C, 0x00355D36,
		   0x00675400, 0x00B26CB4, 0x00E06582, 0x00167ED8, 0x004477EE, 0x00FA496C, 0x00A8405A,
		   0x005E5B00, 0x000C5236, 0x0046FF6C, 0x0014F65A, 0x00E2ED00, 0x00B0E436, 0x000EDAB4,
		   0x005CD382, 0x00AAC8D8, 0x00F8C1EE, 0x002DF95A, 0x007FF06C, 0x0089EB36, 0x00DBE200,
		   0x0065DC82, 0x0037D5B4, 0x00C1CEEE, 0x0093C7D8, 0x0090F300, 0x00C2FA36, 0x0034E16C,
		   0x0066E85A, 0x00D8D6D8, 0x008ADFEE, 0x007CC4B4, 0x002ECD82, 0x00FBF536, 0x00A9FC00,
		   0x005FE75A, 0x000DEE6C, 0x00B3D0EE, 0x00E1D9D8, 0x0017C282, 0x0045CBB4, 0x00EAE6B4,
		   0x00B8EF82, 0x004EF4D8, 0x001CFDEE, 0x00A2C36C, 0x00F0CA5A, 0x0006D100, 0x0054D836,
		   0x0081E082, 0x00D3E9B4, 0x0025F2EE, 0x0077FBD8, 0x00C9C55A, 0x009BCC6C, 0x006DD736,
		   0x003FDE00, 0x003CEAD8, 0x006EE3EE, 0x0098F8B4, 0x00CAF182, 0x0074CF00, 0x0026C636,
		   0x00D0DD6C, 0x0082D45A, 0x0057ECEE, 0x0005E5D8, 0x00F3FE82, 0x00A1F7B4, 0x001FC936,
		   0x004DC000, 0x00BBDB5A, 0x00E9D26C, 0x00E5805A, 0x00B7896C, 0x00419236, 0x00139B00,
		   0x00ADA582, 0x00FFACB4, 0x0009B7EE, 0x005BBED8, 0x008E866C, 0x00DC8F5A, 0x002A9400,
		   0x00789D36, 0x00C6A3B4, 0x0094AA82, 0x0062B1D8, 0x0030B8EE, 0x00338C36, 0x00618500,
		   0x00979E5A, 0x00C5976C, 0x007BA9EE, 0x0029A0D8, 0x00DFBB82, 0x008DB2B4, 0x00588A00,
		   0x000A8336, 0x00FC986C, 0x00AE915A, 0x0010AFD8, 0x0042A6EE, 0x00B4BDB4, 0x00E6B482,
		   0x00499982, 0x001B90B4, 0x00ED8BEE, 0x00BF82D8, 0x0001BC5A, 0x0053B56C, 0x00A5AE36,
		   0x00F7A700, 0x00229FB4, 0x00709682, 0x00868DD8, 0x00D484EE, 0x006ABA6C, 0x0038B35A,
		   0x00CEA800, 0x009CA136, 0x009F95EE, 0x00CD9CD8, 0x003B8782, 0x00698EB4, 0x00D7B036,
		   0x0085B900, 0x0073A25A, 0x0021AB6C, 0x00F493D8, 0x00A69AEE, 0x005081B4, 0x00028882,
		   0x00BCB600, 0x00EEBF36, 0x0018A46C, 0x004AAD5A 
		};
		const uint32 * p_tab = State::P_Tab_Crc32;
		if(pS->Flags & pS->fEmpty) {
			pS->Result.R32 = 0xCE04B7UL;
			pS->Flags &= ~pS->fEmpty;
		}
		result = pS->Result.R32;
		if(pData && dataLen) {
			const uint8 * p_buf = PTR8C(pData);
			// 
			// Update a CRC24 Checksum
			// 
			// Implementation uses Slicing-by-N algorithm described in
			// "Novel Table Lookup-Based Algorithms for High-Performance CRC Generation", by M.Kounavis.
			// 
			// This algorithm uses 4 precomputed look-up tables. First
			// table T0 is computed same way as in a method proposed
			// by D. Sarwate (1988). Then T_1, T2 and T3 are computed in following way:
			// 
			// T1[j] = (T0[j] >> 8) ^ T0[ T0[j] & 0xFF ]
			// T2[j] = (T1[j] >> 8) ^ T0[ T1[j] & 0xFF ]
			// T3[j] = (T2[j] >> 8) ^ T0[ T2[j] & 0xFF ]
			// 
			//uint32 tmp = result;
			// Input is word aligned if WA & input == 0
			static const uint8 WA = sizeof(uintptr_t) - 1;
			// Ensure input is word aligned before processing in parallel
			size_t _len = dataLen;
			for(;_len && (reinterpret_cast<uintptr_t>(p_buf) & WA); _len--) {
				result = (result >> 8) ^ CRC24_T0[(result & 0xff) ^ *p_buf++];
			}
			while(_len >= 16) {
				result ^= PTR32C(p_buf)[0];
				result = CRC24_T3[(result >> 0) & 0xff] ^ CRC24_T2[(result >> 8) & 0xff] ^ CRC24_T1[(result >> 16) & 0xff] ^ CRC24_T0[(result >> 24) & 0xff];
				result ^= PTR32C(p_buf)[1];
				result = CRC24_T3[(result >> 0) & 0xff] ^ CRC24_T2[(result >> 8) & 0xff] ^ CRC24_T1[(result >> 16) & 0xff] ^ CRC24_T0[(result >> 24) & 0xff];
				result ^= PTR32C(p_buf)[2];
				result = CRC24_T3[(result >> 0) & 0xff] ^ CRC24_T2[(result >> 8) & 0xff] ^ CRC24_T1[(result >> 16) & 0xff] ^ CRC24_T0[(result >> 24) & 0xff];
				result ^= PTR32C(p_buf)[3];
				result = CRC24_T3[(result >> 0) & 0xff] ^ CRC24_T2[(result >> 8) & 0xff] ^ CRC24_T1[(result >> 16) & 0xff] ^ CRC24_T0[(result >> 24) & 0xff];
				p_buf += 16;
				_len -= 16;
			}
			while(_len) {
				result = (result >> 8) ^ CRC24_T0[(result & 0xff) ^ *p_buf++];
				_len--;
			}
		}
		else {
			result = (result & 0x00ffffffU);
		}
		pS->Result.R32 = result;
	}
	return result;
}

/*static*/uint32 STDCALL SlHash::Adler32(State * pS, const void * pData, size_t dataLen)
{
	uint32 result = 0;
	if(!pS) {
		State st;
		Adler32(&st, pData, dataLen);
		result = Adler32(&st, 0, 0);
	}
	else {
		if(pS->Flags & pS->fEmpty) {
			pS->Result.R32 = 1UL;
			pS->Flags &= ~pS->fEmpty;
		}
		result = pS->Result.R32;
		if(pData && dataLen) {
			const uint8 * p_buf = PTR8C(pData);
			{
				#define DO1(buf, i) { value += (buf)[i]; sum2 += value; }
				#define DO2(buf, i)  DO1(buf, i); DO1(buf, i+1);
				#define DO4(buf, i)  DO2(buf, i); DO2(buf, i+2);
				#define DO8(buf, i)  DO4(buf, i); DO4(buf, i+4);
				#define DO16(buf)    DO8(buf, 0); DO8(buf, 8);
				ulong value = result;
				size_t _len = dataLen;
				const uint _base_ = 65521U; // largest prime smaller than 65536
				const uint _nmax_ = 5552;   // _nmax_ is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1
				uint   n;
				ulong  sum2 = (value >> 16) & 0xffff; // split Adler-32 into component sums
				value &= 0xffff;
				if(_len == 1) { // in case user likes doing a byte at a time, keep it fast
					value += p_buf[0];
					if(value >= _base_)
						value -= _base_;
					sum2 += value;
					if(sum2 >= _base_)
						sum2 -= _base_;
					result = (value | (sum2 << 16));
				}
				else if(p_buf) { // initial Adler-32 value (deferred check for len == 1 speed)
					if(_len < 16) { // in case short lengths are provided, keep it somewhat fast
						while(_len--) {
							value += *p_buf++;
							sum2 += value;
						}
						if(value >= _base_)
							value -= _base_;
						sum2 %= _base_; // only added so many _base_'s
						result = (value | (sum2 << 16));
					}
					else {
						// do length NMAX blocks -- requires just one modulo operation
						while(_len >= _nmax_) {
							_len -= _nmax_;
							n = _nmax_ / 16; // NMAX is divisible by 16
							do {
								DO16(p_buf); // 16 sums unrolled
								p_buf += 16;
							} while(--n);
							value %= _base_;
							sum2 %= _base_;
						}
						// do remaining bytes (less than NMAX, still just one modulo)
						if(_len) { // avoid modulos if none remaining
							while(_len >= 16) {
								_len -= 16;
								DO16(p_buf);
								p_buf += 16;
							}
							while(_len--) {
								value += *p_buf++;
								sum2 += value;
							}
							value %= _base_;
							sum2 %= _base_;
						}
						result = (value | (sum2 << 16)); // return recombined sums
					}
				}
				#undef DO1
				#undef DO2
				#undef DO4
				#undef DO8
				#undef DO16
			}
		}
		pS->Result.R32 = result;
	}
	return result;
}
//
//
//
#define MD5_DIGEST_SIZE		16
#define MD5_HMAC_BLOCK_SIZE	64
#define MD5_BLOCK_WORDS		16
#define MD5_HASH_WORDS		4

static inline void sl_cpu_to_le32_array(uint32 * buf, uint words)
{
	if(SLS.SSys.IsBigEndian) {
		while(words--) {
			*buf = _byteswap_ulong(*buf);
			buf++;
		}
	}
}

static inline void sl_le32_to_cpu_array(uint32 * buf, uint words)
{
	if(SLS.SSys.IsBigEndian) {
		while(words--) {
			*buf = _byteswap_ulong(*buf);
			buf++;
		}
	}
}

//static const uint8 md5_zero_message_hash[MD5_DIGEST_SIZE] = { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e, };

static void md5_transform(uint32 * pHash, const uint32 * in)
{
	uint32 a = pHash[0];
	uint32 b = pHash[1];
	uint32 c = pHash[2];
	uint32 d = pHash[3];
#define F1(x, y, z)	(z ^ (x & (y ^ z)))
#define F2(x, y, z)	F1(z, x, y)
#define F3(x, y, z)	(x ^ y ^ z)
#define F4(x, y, z)	(y ^ (x | ~z))
#define MD5STEP(f, w, x, y, z, in, s) (w += f(x, y, z) + in, w = (w<<s | w>>(32-s)) + x)
	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);
#undef F1
#undef F2
#undef F3
#undef F4
#undef MD5STEP
	pHash[0] += a;
	pHash[1] += b;
	pHash[2] += c;
	pHash[3] += d;
}

/*static*/void FASTCALL SlHash::Md5TransformHelper(State::Md5Ctx * pCtx)
{
	sl_le32_to_cpu_array(pCtx->Data, sizeof(pCtx->Data) / sizeof(uint32));
	md5_transform(pCtx->H, pCtx->Data);
}

/*static*/binary128 STDCALL SlHash::Md5(State * pS, const void * pData, size_t dataLen)
{
	binary128 result;
	if(!pS) {
		State st;
		result = Md5(&st, pData, dataLen); // @recursion
		if(pData && dataLen)
			result = Md5(&st, 0, 0); // @recursion
	}
	else {
		State::Md5Ctx & r_ctx = pS->Result.Md5;
		if(pS->Flags & pS->fEmpty) {
			r_ctx.H[0] = 0x67452301UL;
			r_ctx.H[1] = 0xefcdab89UL;
			r_ctx.H[2] = 0x98badcfeUL;
			r_ctx.H[3] = 0x10325476UL;
			r_ctx.Count = 0;
			pS->Flags &= ~pS->fEmpty;
		}
		if(pData && dataLen) {
			const uint32 avail = sizeof(r_ctx.Data) - static_cast<uint32>(r_ctx.Count & 0x3f);
			r_ctx.Count += dataLen;
			if(avail > dataLen) {
				memcpy((char *)r_ctx.Data + (sizeof(r_ctx.Data) - avail), pData, dataLen);
			}
			else {
				memcpy((char *)r_ctx.Data + (sizeof(r_ctx.Data) - avail), pData, avail);
				Md5TransformHelper(&r_ctx);
				{
					const uint8 * p_local_data_ptr = static_cast<const uint8 *>(pData);
					p_local_data_ptr += avail;
					dataLen -= avail;
					while(dataLen >= sizeof(r_ctx.Data)) {
						memcpy(r_ctx.Data, p_local_data_ptr, sizeof(r_ctx.Data));
						Md5TransformHelper(&r_ctx);
						p_local_data_ptr += sizeof(r_ctx.Data);
						dataLen -= sizeof(r_ctx.Data);
					}
					memcpy(r_ctx.Data, p_local_data_ptr, dataLen);
				}
			}
		}
		else { // final
			const  uint offset = static_cast<uint>(r_ctx.Count & 0x3f);
			uint8 * p = reinterpret_cast<uint8 *>(r_ctx.Data) + offset;
			int padding = 56 - (offset + 1);
			*p++ = 0x80;
			if(padding < 0) {
				memzero(p, padding + sizeof(uint64));
				Md5TransformHelper(&r_ctx);
				p = reinterpret_cast<uint8 *>(r_ctx.Data);
				padding = 56;
			}
			memzero(p, padding);
			r_ctx.Data[14] = static_cast<uint32>(r_ctx.Count << 3);
			r_ctx.Data[15] = static_cast<uint32>(r_ctx.Count >> 29);
			sl_le32_to_cpu_array(r_ctx.Data, (sizeof(r_ctx.Data) - sizeof(uint64)) / sizeof(uint32));
			md5_transform(r_ctx.H, r_ctx.Data);
			sl_cpu_to_le32_array(r_ctx.H, sizeof(r_ctx.H) / sizeof(uint32));
			memcpy(&result, r_ctx.H, sizeof(r_ctx.H));
			r_ctx.Z();
		}
	}
	return result;
}
// 
// Helper functions.
// 
//#define SHA1_ROTATE(bits, word)  (((word) << (bits)) | ((word) >> (32 - (bits))))
#define SHA1_F1(b, c, d)  (((b) & (c)) | ((~(b)) & (d)))
#define SHA1_F2(b, c, d)  ((b) ^ (c) ^ (d))
#define SHA1_F3(b, c, d)  (((b) & (c)) | ((b) & (d)) | ((c) & (d)))
//#define SHA1_STEP(f, a, b, c, d, e, w, t) temp = SHA1_ROTATE(5, (a)) + f((b), (c), (d)) + (e) + (w) + (t); (e) = (d); (d) = (c); (c) = SHA1_ROTATE(30, (b)); (b) = (a); (a) = temp;
#define SHA1_STEP(f, a, b, c, d, e, w, t) temp = SBits::Rotl((a), 5) + f((b), (c), (d)) + (e) + (w) + (t); (e) = (d); (d) = (c); (c) = SBits::Rotl((b), 30); (b) = (a); (a) = temp;
//
#define sHA256_BLOCK_SIZE 64
#define SHA512_BLOCK_SIZE 128

#define SHA256_STORE32H(x, y) { (y)[0] = (uint8)(((x)>>24)&255); (y)[1] = (uint8)(((x)>>16)&255); (y)[2] = (uint8)(((x)>>8)&255); (y)[3] = (uint8)((x)&255); }
#define SHA256_LOAD32H(x, y) { x = ((uint32)((y)[0] & 255)<<24) | ((uint32)((y)[1] & 255)<<16) | ((uint32)((y)[2] & 255)<<8) | ((uint32)((y)[3] & 255)); }
#define SHA256_STORE64H(x, y) { (y)[0] = (uint8)(((x)>>56)&255); (y)[1] = (uint8)(((x)>>48)&255); (y)[2] = (uint8)(((x)>>40)&255); (y)[3] = (uint8)(((x)>>32)&255); \
	(y)[4] = (uint8)(((x)>>24)&255); (y)[5] = (uint8)(((x)>>16)&255); (y)[6] = (uint8)(((x)>>8)&255); (y)[7] = (uint8)((x)&255); }
//#define SHA256_ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
//#define SHA256_ror(value, bits) SBits::Rotr(value, bits)

#define SHA256_Ch(x, y, z)     (z ^ (x & (y ^ z)))
#define SHA256_Maj(x, y, z)    (((x | y) & z) | (x & y))
#define SHA256_S(x, n)         SBits::Rotr((x), (n))
#define SHA256_R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define SHA256_Sigma0(x)       (SHA256_S(x, 2) ^ SHA256_S(x, 13) ^ SHA256_S(x, 22))
#define SHA256_Sigma1(x)       (SHA256_S(x, 6) ^ SHA256_S(x, 11) ^ SHA256_S(x, 25))
#define SHA256_Gamma0(x)       (SHA256_S(x, 7) ^ SHA256_S(x, 18) ^ SHA256_R(x, 3))
#define SHA256_Gamma1(x)       (SHA256_S(x, 17) ^ SHA256_S(x, 19) ^ SHA256_R(x, 10))

#define Sha256Round(a, b, c, d, e, f, g, h, i)       \
	t0 = h + SHA256_Sigma1(e) + SHA256_Ch(e, f, g) + K[i] + W[i];   \
	t1 = SHA256_Sigma0(a) + SHA256_Maj(a, b, c);                    \
	d += t0;                                          \
	h  = t0 + t1;


//#define ROR64(value, bits) (((value) >> (bits)) | ((value) << (64 - (bits))))
//#define ROR64(value, bits) SBits::Rotr(value, bits)
#define SHA512_LOAD64H(x, y) { x = (((uint64)((y)[0] & 255))<<56)|(((uint64)((y)[1] & 255))<<48) | \
	(((uint64)((y)[2] & 255))<<40)|(((uint64)((y)[3] & 255))<<32) | (((uint64)((y)[4] & 255))<<24)|(((uint64)((y)[5] & 255))<<16) | \
	(((uint64)((y)[6] & 255))<<8)|(((uint64)((y)[7] & 255))); }

#define SHA512_STORE64H(x, y) { (y)[0] = (uint8)(((x)>>56)&255); (y)[1] = (uint8)(((x)>>48)&255);     \
	(y)[2] = (uint8)(((x)>>40)&255); (y)[3] = (uint8)(((x)>>32)&255); (y)[4] = (uint8)(((x)>>24)&255); (y)[5] = (uint8)(((x)>>16)&255); \
	(y)[6] = (uint8)(((x)>>8)&255); (y)[7] = (uint8)((x)&255); }

#define SHA512_Ch(x, y, z)     (z ^ (x & (y ^ z)))
#define SHA512_Maj(x, y, z)     (((x | y) & z) | (x & y))
#define SHA512_S(x, n)         SBits::Rotr(x, n)
#define SHA512_R(x, n)         (((x)&0xFFFFFFFFFFFFFFFFULL)>>((uint64)n))
#define SHA512_Sigma0(x)       (SHA512_S(x, 28) ^ SHA512_S(x, 34) ^ SHA512_S(x, 39))
#define SHA512_Sigma1(x)       (SHA512_S(x, 14) ^ SHA512_S(x, 18) ^ SHA512_S(x, 41))
#define SHA512_Gamma0(x)       (SHA512_S(x, 1)  ^ SHA512_S(x, 8)  ^ SHA512_R(x, 7))
#define SHA512_Gamma1(x)       (SHA512_S(x, 19) ^ SHA512_S(x, 61) ^ SHA512_R(x, 6))

#define Sha512Round(a, b, c, d, e, f, g, h, i) \
	t0 = h + SHA512_Sigma1(e) + SHA512_Ch(e, f, g) + K[i] + W[i];   \
	t1 = SHA512_Sigma0(a) + SHA512_Maj(a, b, c); \
	d += t0; \
	h  = t0 + t1;
// 
// This processes one or more 64-byte data blocks, but does not update
// the bit counters. There are no alignment requirements.
// 
/*static*/const uchar * SlHash::Sha1_Body(State::ShaCtx * pCtx, const uchar * data, size_t size)
{
	uint32 words[80];
	uint   i;
	const  uchar * p = data;
	uint32 a = pCtx->H[0];
	uint32 b = pCtx->H[1];
	uint32 c = pCtx->H[2];
	uint32 d = pCtx->H[3];
	uint32 e = pCtx->H[4];
	do {
		const uint32 saved_a = a;
		const uint32 saved_b = b;
		const uint32 saved_c = c;
		const uint32 saved_d = d;
		const uint32 saved_e = e;
		uint32 temp;
		// Load data block into the words array 
		// 
		// GET() reads 4 input bytes in big-endian byte order and returns them as uint32.
		// 
		#define SHA1_GET(n) ((uint32)p[n * 4 + 3] | ((uint32)p[n * 4 + 2] << 8) | ((uint32)p[n * 4 + 1] << 16) | ((uint32)p[n * 4] << 24))
		for(i = 0; i < 16; i++) {
			words[i] = SHA1_GET(i);
		}
		#undef SHA1_GET
		for(i = 16; i < 80; i++) {
			//words[i] = SHA1_ROTATE(1, words[i-3] ^ words[i-8] ^ words[i-14] ^ words[i-16]);
			words[i] = SBits::Rotl(words[i-3] ^ words[i-8] ^ words[i-14] ^ words[i-16], 1);
		}
		// Transformations 
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[0],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[1],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[2],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[3],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[4],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[5],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[6],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[7],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[8],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[9],  0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[10], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[11], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[12], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[13], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[14], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[15], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[16], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[17], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[18], 0x5a827999);
		SHA1_STEP(SHA1_F1, a, b, c, d, e, words[19], 0x5a827999);

		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[20], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[21], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[22], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[23], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[24], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[25], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[26], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[27], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[28], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[29], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[30], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[31], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[32], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[33], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[34], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[35], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[36], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[37], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[38], 0x6ed9eba1);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[39], 0x6ed9eba1);

		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[40], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[41], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[42], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[43], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[44], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[45], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[46], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[47], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[48], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[49], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[50], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[51], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[52], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[53], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[54], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[55], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[56], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[57], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[58], 0x8f1bbcdc);
		SHA1_STEP(SHA1_F3, a, b, c, d, e, words[59], 0x8f1bbcdc);

		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[60], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[61], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[62], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[63], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[64], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[65], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[66], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[67], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[68], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[69], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[70], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[71], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[72], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[73], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[74], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[75], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[76], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[77], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[78], 0xca62c1d6);
		SHA1_STEP(SHA1_F2, a, b, c, d, e, words[79], 0xca62c1d6);

		a += saved_a;
		b += saved_b;
		c += saved_c;
		d += saved_d;
		e += saved_e;

		p += 64;
	} while(size -= 64);
	pCtx->H[0] = a;
	pCtx->H[1] = b;
	pCtx->H[2] = c;
	pCtx->H[3] = d;
	pCtx->H[4] = e;
	return p;
}

/*static*/binary160 STDCALL SlHash::Sha1(State * pS, const void * pData, size_t dataLen)
{
	binary160 result;
	if(!pS) {
		State st;
		result = Sha1(&st, pData, dataLen);
		if(pData && dataLen)
			result = Sha1(&st, 0, 0);
	}
	else {
		State::ShaCtx & r_ctx = pS->Result.Sha;
		if(pS->Flags & pS->fEmpty) {
			r_ctx.H[0] = 0x67452301;
			r_ctx.H[1] = 0xefcdab89;
			r_ctx.H[2] = 0x98badcfe;
			r_ctx.H[3] = 0x10325476;
			r_ctx.H[4] = 0xc3d2e1f0;
			r_ctx.Count = 0;
			pS->Flags &= ~pS->fEmpty;
		}
		if(pData && dataLen) {
			int    done = 0;
			const  size_t used = static_cast<size_t>(r_ctx.Count & 0x3f);
			r_ctx.Count += dataLen;
			if(used) {
				const size_t free__ = 64 - used;
				if(dataLen < free__) {
					memcpy(PTR8(r_ctx.Data)+used, pData, dataLen);
					done = 1;
				}
				else {
					memcpy(PTR8(r_ctx.Data)+used, pData, free__);
					pData = PTR8C(pData) + free__;
					dataLen -= free__;
					Sha1_Body(&r_ctx, reinterpret_cast<const uchar *>(r_ctx.Data), 64);
				}
			}
			if(!done) {
				if(dataLen >= 64) {
					pData = Sha1_Body(&r_ctx, static_cast<const uchar *>(pData), dataLen & ~0x3fU);
					dataLen &= 0x3f;
				}
				assert(dataLen <= sizeof(r_ctx.Data));
				memcpy(r_ctx.Data, pData, dataLen);
			}
		}
		else { // final
			size_t used = static_cast<size_t>(r_ctx.Count & 0x3f);
			PTR8(r_ctx.Data)[used++] = 0x80;
			size_t free__ = 64 - used;
			if(free__ < 8) {
				memzero(PTR8(r_ctx.Data) + used, free__);
				Sha1_Body(&r_ctx, PTR8(r_ctx.Data), 64);
				used = 0;
				free__ = 64;
			}
			memzero(PTR8(r_ctx.Data) + used, free__ - 8);
			r_ctx.Count <<= 3;
			PTR8(r_ctx.Data)[56] = static_cast<uchar>(r_ctx.Count >> 56);
			PTR8(r_ctx.Data)[57] = static_cast<uchar>(r_ctx.Count >> 48);
			PTR8(r_ctx.Data)[58] = static_cast<uchar>(r_ctx.Count >> 40);
			PTR8(r_ctx.Data)[59] = static_cast<uchar>(r_ctx.Count >> 32);
			PTR8(r_ctx.Data)[60] = static_cast<uchar>(r_ctx.Count >> 24);
			PTR8(r_ctx.Data)[61] = static_cast<uchar>(r_ctx.Count >> 16);
			PTR8(r_ctx.Data)[62] = static_cast<uchar>(r_ctx.Count >> 8);
			PTR8(r_ctx.Data)[63] = (uchar)r_ctx.Count;
			{
				Sha1_Body(&r_ctx, PTR8(r_ctx.Data), 64);
			}
			result.D[0]  = static_cast<uchar>(r_ctx.H[0] >> 24);
			result.D[1]  = static_cast<uchar>(r_ctx.H[0] >> 16);
			result.D[2]  = static_cast<uchar>(r_ctx.H[0] >> 8);
			result.D[3]  = static_cast<uchar>(r_ctx.H[0]);
			result.D[4]  = static_cast<uchar>(r_ctx.H[1] >> 24);
			result.D[5]  = static_cast<uchar>(r_ctx.H[1] >> 16);
			result.D[6]  = static_cast<uchar>(r_ctx.H[1] >> 8);
			result.D[7]  = static_cast<uchar>(r_ctx.H[1]);
			result.D[8]  = static_cast<uchar>(r_ctx.H[2] >> 24);
			result.D[9]  = static_cast<uchar>(r_ctx.H[2] >> 16);
			result.D[10] = static_cast<uchar>(r_ctx.H[2] >> 8);
			result.D[11] = static_cast<uchar>(r_ctx.H[2]);
			result.D[12] = static_cast<uchar>(r_ctx.H[3] >> 24);
			result.D[13] = static_cast<uchar>(r_ctx.H[3] >> 16);
			result.D[14] = static_cast<uchar>(r_ctx.H[3] >> 8);
			result.D[15] = static_cast<uchar>(r_ctx.H[3]);
			result.D[16] = static_cast<uchar>(r_ctx.H[4] >> 24);
			result.D[17] = static_cast<uchar>(r_ctx.H[4] >> 16);
			result.D[18] = static_cast<uchar>(r_ctx.H[4] >> 8);
			result.D[19] = static_cast<uchar>(r_ctx.H[4]);
			r_ctx.Z();
		}
	}
	return result;
}

void FASTCALL SlHash::__Sha256TransformHelper(State::ShaCtx * pCtx, const void * pBuffer)
{
	// The K array
	static const uint32 K[64] = {
		0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
		0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
		0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
		0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
		0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
		0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
		0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
		0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
		0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
		0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
		0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
		0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
		0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
	};
	uint32 S[8];
	uint32 W[64];
	uint32 t0;
	uint32 t1;
	uint32 t;
	uint   i;
	// Copy H into S
	// @v11.3.10 for(i = 0; i < 8; i++) { S[i] = pCtx->H[i]; }
	memcpy(S, pCtx->H, sizeof(S)); // @v11.3.10
	// Copy the H into 512-bits into W[0..15]
	for(i = 0; i < 16; i++) {
		SHA256_LOAD32H(W[i], PTR8C(pBuffer) + (4*i));
	}
	// Fill W[16..63]
	for(i = 16; i < 64; i++) {
		W[i] = SHA256_Gamma1(W[i-2]) + W[i-7] + SHA256_Gamma0(W[i-15]) + W[i-16];
	}
	// Compress
	for(i = 0; i < 64; i++) {
		Sha256Round(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i);
		t = S[7];
		S[7] = S[6];
		S[6] = S[5];
		S[5] = S[4];
		S[4] = S[3];
		S[3] = S[2];
		S[2] = S[1];
		S[1] = S[0];
		S[0] = t;
	}
	// Feedback
	for(i = 0; i < 8; i++) {
		pCtx->H[i] = pCtx->H[i] + S[i];
	}
}

/*static*/binary256 STDCALL SlHash::Sha256(State * pS, const void * pData, size_t dataLen)
{
	binary256 result;
	if(!pS) {
		State st;
		result = Sha256(&st, pData, dataLen);
		if(pData && dataLen) {
			result = Sha256(&st, 0, 0);
		}
	}
	else {
		State::ShaCtx & r_ctx = pS->Result.Sha;
		if(pS->Flags & pS->fEmpty) {
			r_ctx.Num = 0;
			r_ctx.Count = 0;
			r_ctx.H[0] = 0x6A09E667UL;
			r_ctx.H[1] = 0xBB67AE85UL;
			r_ctx.H[2] = 0x3C6EF372UL;
			r_ctx.H[3] = 0xA54FF53AUL;
			r_ctx.H[4] = 0x510E527FUL;
			r_ctx.H[5] = 0x9B05688CUL;
			r_ctx.H[6] = 0x1F83D9ABUL;
			r_ctx.H[7] = 0x5BE0CD19UL;
			pS->Flags &= ~pS->fEmpty;
		}
		if(pData && dataLen) {
			if(r_ctx.Num <= sizeof(r_ctx.Data)) {
				while(dataLen > 0) {
					if(r_ctx.Num == 0 && dataLen >= sHA256_BLOCK_SIZE) {
						__Sha256TransformHelper(&r_ctx, (uint8 *)pData);
						r_ctx.Count += sHA256_BLOCK_SIZE * 8;
						pData = (uint8 *)pData + sHA256_BLOCK_SIZE;
						dataLen -= sHA256_BLOCK_SIZE;
					}
					else {
						uint32 n = MIN(dataLen, (sHA256_BLOCK_SIZE - r_ctx.Num));
						memcpy(PTR8(r_ctx.Data) + r_ctx.Num, pData, (size_t)n);
						r_ctx.Num += n;
						pData = (uint8 *)pData + n;
						dataLen -= n;
						if(r_ctx.Num == sHA256_BLOCK_SIZE) {
							__Sha256TransformHelper(&r_ctx, r_ctx.Data);
							r_ctx.Count += 8 * sHA256_BLOCK_SIZE;
							r_ctx.Num = 0;
						}
					}
				}
			}
		}
		else { // final
			if(r_ctx.Num < sizeof(r_ctx.Data)) {
				r_ctx.Count += r_ctx.Num * 8; // Increase the Count of the message
				PTR8(r_ctx.Data)[r_ctx.Num++] = (uint8)0x80; // Append the '1' bit
				// if the Count is currently above 56 bytes we append zeros
				// then compress.  Then we can fall back to padding zeros and Count
				// encoding like normal.
				if(r_ctx.Num > 56) {
					while(r_ctx.Num < 64) {
						PTR8(r_ctx.Data)[r_ctx.Num++] = 0;
					}
					__Sha256TransformHelper(&r_ctx, r_ctx.Data);
					r_ctx.Num = 0;
				}
				// Pad up to 56 bytes of zeroes
				while(r_ctx.Num < 56) {
					PTR8(r_ctx.Data)[r_ctx.Num++] = 0;
				}
				// Store Count
				SHA256_STORE64H(r_ctx.Count, PTR8(r_ctx.Data) + 56);
				__Sha256TransformHelper(&r_ctx, r_ctx.Data);
			}
			// Copy output
			for(int i = 0; i < 8; i++) {
				SHA256_STORE32H(r_ctx.H[i], PTR8(&result) + (4*i));
			}
			r_ctx.Z();
		}
	}
	return result;
}

//
//  TransformFunction
//  Compress 1024-bits
//
/*static*/void SlHash::__Sha512TransformHelper(State::Sha512Ctx * pCtx, const void * pBuffer)
{
	static const uint64 K[80] = { // The K array
		0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
		0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
		0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
		0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
		0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
		0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
		0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
		0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
		0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
		0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
		0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
		0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
		0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
		0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
		0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
		0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
		0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
		0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
		0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
		0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
	};
	uint64 S[8];
	uint64 W[80];
	uint64 t0;
	uint64 t1;
	int i;
	// Copy state into S
	for(i = 0; i < 8; i++) {
		S[i] = pCtx->H[i];
	}
	// Copy the state into 1024-bits into W[0..15]
	for(i = 0; i < 16; i++) {
		SHA512_LOAD64H(W[i], PTR8C(pBuffer) + (8*i));
	}
	// Fill W[16..79]
	for(i = 16; i < 80; i++) {
		W[i] = SHA512_Gamma1(W[i - 2]) + W[i - 7] + SHA512_Gamma0(W[i - 15]) + W[i - 16];
	}
	// Compress
	for(i = 0; i < 80; i += 8) {
		Sha512Round(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i+0);
		Sha512Round(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], i+1);
		Sha512Round(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], i+2);
		Sha512Round(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], i+3);
		Sha512Round(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], i+4);
		Sha512Round(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], i+5);
		Sha512Round(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], i+6);
		Sha512Round(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], i+7);
	}
	// Feedback
	for(i = 0; i < 8; i++) {
		pCtx->H[i] = pCtx->H[i] + S[i];
	}
}

/*static*/binary512 STDCALL SlHash::Sha512(State * pS, const void * pData, size_t dataLen)
{
	binary512 result;
	if(!pS) {
		State st;
		result = Sha512(&st, pData, dataLen); // @recursion
		if(pData && dataLen)
			result = Sha512(&st, 0, 0); // @recursion
	}
	else {
		State::Sha512Ctx & r_ctx = pS->Result.Sha512;
		if(pS->Flags & pS->fEmpty) {
			r_ctx.Num = 0;
			r_ctx.Count = 0;
			r_ctx.H[0] = 0x6a09e667f3bcc908ULL;
			r_ctx.H[1] = 0xbb67ae8584caa73bULL;
			r_ctx.H[2] = 0x3c6ef372fe94f82bULL;
			r_ctx.H[3] = 0xa54ff53a5f1d36f1ULL;
			r_ctx.H[4] = 0x510e527fade682d1ULL;
			r_ctx.H[5] = 0x9b05688c2b3e6c1fULL;
			r_ctx.H[6] = 0x1f83d9abfb41bd6bULL;
			r_ctx.H[7] = 0x5be0cd19137e2179ULL;
			pS->Flags &= ~pS->fEmpty;
		}
		if(pData && dataLen) {
			if(r_ctx.Num <= sizeof(r_ctx.Data)) {
				while(dataLen > 0) {
					if(r_ctx.Num == 0 && dataLen >= SHA512_BLOCK_SIZE) {
						__Sha512TransformHelper(&r_ctx, PTR8C(pData));
						r_ctx.Count += (SHA512_BLOCK_SIZE * 8);
						pData = PTR8C(pData) + SHA512_BLOCK_SIZE;
						dataLen -= SHA512_BLOCK_SIZE;
					}
					else {
						uint32 n = MIN(dataLen, (SHA512_BLOCK_SIZE - r_ctx.Num));
						memcpy(r_ctx.Data + r_ctx.Num, pData, (size_t)n);
						r_ctx.Num += n;
						pData = PTR8C(pData) + n;
						dataLen -= n;
						if(r_ctx.Num == SHA512_BLOCK_SIZE) {
							__Sha512TransformHelper(&r_ctx, r_ctx.Data);
							r_ctx.Count += (8 * SHA512_BLOCK_SIZE);
							r_ctx.Num = 0;
						}
					}
				}
			}
		}
		else { // final
			if(r_ctx.Num < sizeof(r_ctx.Data)) {
				// Increase the length of the message
				r_ctx.Count += (r_ctx.Num * 8ULL);
				// Append the '1' bit
				r_ctx.Data[r_ctx.Num++] = (uint8)0x80;
				// If the length is currently above 112 bytes we append zeros
				// then compress.  Then we can fall back to padding zeros and length
				// encoding like normal.
				if(r_ctx.Num > 112) {
					while(r_ctx.Num < 128) {
						r_ctx.Data[r_ctx.Num++] = 0;
					}
					__Sha512TransformHelper(&r_ctx, r_ctx.Data);
					r_ctx.Num = 0;
				}
				// Pad up to 120 bytes of zeroes
				// note: that from 112 to 120 is the 64 MSB of the length.  We assume that you won't hash
				// > 2^64 bits of data... :-)
				while(r_ctx.Num < 120) {
					r_ctx.Data[r_ctx.Num++] = 0;
				}
				// Store length
				SHA512_STORE64H(r_ctx.Count, r_ctx.Data+120);
				__Sha512TransformHelper(&r_ctx, r_ctx.Data);
			}
			// Copy output
			for(int i = 0; i < 8; i++) {
				SHA512_STORE64H(r_ctx.H[i], PTR8(&result) + (8*i));
			}
			r_ctx.Z();
		}
	}
	return result;
}

SlHash::State::ShaCtx & SlHash::State::ShaCtx::Z()
{
	THISZERO();
	return *this;
}

SlHash::State::Sha512Ctx & SlHash::State::Sha512Ctx::Z()
{
	THISZERO();
	return *this;
}

SlHash::State::Md5Ctx & SlHash::State::Md5Ctx::Z()
{
	THISZERO();
	return *this;
}

SlHash::State::R::R()
{
	THISZERO();
}

static int TestCrypto(const SString & rInFileName, const char * pSetName, int alg, int kbl, int algmod)
{
	int    ok = 1;
	{
		const char * p_password = "test_crypto_password";
		const size_t pattern_buf_size = SKILOBYTE(4096);
		const size_t pattern_work_size = pattern_buf_size /*- 13*/;
		const size_t gcm_tag_size = 12;
		uint8 gcm_tag[16]; // really 12 bytes
		SlCrypto::Key key;
		//size_t total_encr_size = 0;
		//size_t work_offs = 0;
		//size_t total_decr_size = 0;
		//size_t actual_size = 0;
		//STempBuffer dest_buf(pattern_buf_size + SKILOBYTE(512)); // with ensuring
		STempBuffer pattern_buf(pattern_buf_size);
		//STempBuffer result_buf(pattern_buf_size + SKILOBYTE(512)); // with ensuring
		SBinaryChunk bc_result;
		SBinaryChunk bc_dest; // 
		SBinaryChunk bc_tag;
		bc_tag.Set(0, 16);
		bc_result.Randomize(pattern_buf_size + SKILOBYTE(512));
		//SObfuscateBuffer(result_buf.vptr(), result_buf.GetSize());
		SObfuscateBuffer(pattern_buf.vptr(), pattern_buf.GetSize());
		{
			SlCrypto cs(alg, kbl, algmod);
			const int skr = cs.SetupKey(key, p_password);
			assert(skr);
			//work_offs = 0;
			//actual_size = 0;
			const int er = cs.Encrypt(key, pattern_buf.vptr(/*total_encr_size*/), pattern_work_size, bc_result, &bc_tag);
			assert(er);
			//work_offs += actual_size;
			//total_encr_size += pattern_work_size;
			//total_encr_size += actual_size;
		}
		{
			SlCrypto cs(alg, kbl, algmod);
			const int ckr = cs.SetupKey(key, p_password);
			assert(ckr);
			//work_offs = 0;
			//actual_size = 0;
			const int dr = cs.Decrypt(key, bc_result.PtrC(), bc_result.Len(), bc_dest, &bc_tag);
			assert(dr);
			//work_offs += actual_size;
			//total_decr_size += total_encr_size;
			//total_decr_size += actual_size;
		}
		assert(bc_dest.Len() == pattern_work_size);
		int r = memcmp(bc_dest.PtrC(), pattern_buf.vcptr(), pattern_work_size);
		assert(r == 0);
	}
	if(!isempty(pSetName)) {
		//STempBuffer result_buf(SKILOBYTE(512));
		SBinaryChunk bc_result;
		SBinaryChunk bc_tag;
		
		TSCollection <BdtTestItem> data_set;
		BdtTestItem::ReadBdtTestData(rInFileName, pSetName, data_set);
		SlCrypto cs(alg, kbl, algmod);
		for(uint i = 0; i < data_set.getCount(); i++) {
			const BdtTestItem * p_item = data_set.at(i);
			const size_t src_size = p_item->In.GetLen();
			const void * p_src_buf = p_item->In.GetBufC();
			const size_t pattern_size = p_item->Out.GetLen();
			const void * p_pattern_buf = p_item->Out.GetBufC();
			//size_t total_size = 0;
			SlCrypto::Key key;
			bc_tag.Set(0, 16);
			{
				int skr = cs.SetupKey(key, p_item->Key.GetBufC(), p_item->Key.GetLen(), p_item->Nonce.GetBufC(), p_item->Nonce.GetLen(), p_item->Aad.GetBufC(), p_item->Aad.GetLen());
				assert(skr);
			}
			size_t actual_size = 0;
			const int er = cs.Encrypt(key, p_src_buf, src_size, bc_result, &bc_tag);
			assert(er);
			//total_size += actual_size;
			//assert(total_size >= pattern_size);
			const int mcr = memcmp(bc_result.PtrC(), p_pattern_buf, pattern_size);
			assert(mcr == 0);
		}
	}
	return ok;
}

void TestCRC()
{
	SString data_trasform_path;
	SLS.QueryPath("testroot", data_trasform_path);
	data_trasform_path.SetLastDSlash().Cat("data/DataTransform").SetLastDSlash();
	SString in_file_name;
	TSCollection <BdtTestItem> data_set;
	{
		(in_file_name = data_trasform_path).Cat("gcm.vec");
		TestCrypto(in_file_name, "AES-128/GCM", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodGcm); // nonce 
		TestCrypto(in_file_name, "AES-128/GCM(12)", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-192/GCM", SlCrypto::algAes, SlCrypto::kbl192, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-192/GCM(12)", SlCrypto::algAes, SlCrypto::kbl192, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-256/GCM", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-256/GCM(12)", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-256/GCM(13)", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-256/GCM(14)", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodGcm); // nonce ad
		TestCrypto(in_file_name, "AES-256/GCM(15)", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodGcm); // nonce ad
		// Wycheproof GCM tests
		TestCrypto(in_file_name, "AES-128/GCM(8)", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodGcm); // nonce ad
		// (will be processed above) TestCrypto(in_file_name, "AES-128/GCM(12)", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodGcm); // nonce ad
		// (will be processed above) TestCrypto(in_file_name, "AES-128/GCM", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodGcm); // nonce ad
	}
	{
		(in_file_name = data_trasform_path).Cat("aes.vec");
		TestCrypto(in_file_name, "AES-128", SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
		TestCrypto(in_file_name, "AES-192", SlCrypto::algAes, SlCrypto::kbl192, SlCrypto::algmodEcb);
		TestCrypto(in_file_name, "AES-256", SlCrypto::algAes, SlCrypto::kbl256, SlCrypto::algmodEcb);
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
			{
				uint32 r = SlHash::CRC24(0, p_src_buf, src_size);
				assert(r == pattern_value);
			}
			if(src_size > 10) {
				SlHash::State st;
				size_t total_sz = 0;
				uint32 r = 0;
				for(size_t ps = 1; total_sz < src_size; ps++) {
					SlHash::CRC24(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
					total_sz += ps;
				}
				SlHash::CRC24(&st, 0, 0); // finalize
				r = st.GetResult32();
				assert(r == pattern_value);
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
			{
				uint32 r = SlHash::CRC32(0, p_src_buf, src_size);
				assert(r == pattern_value);
			}
			if(src_size > 10) {
				SlHash::State st;
				size_t total_sz = 0;
				uint32 r = 0;
				for(size_t ps = 1; total_sz < src_size; ps++) {
					SlHash::CRC32(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
					total_sz += ps;
				}
				SlHash::CRC32(&st, 0, 0); // finalize
				r = st.GetResult32();
				assert(r == pattern_value);
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
			{
				uint32 r = SlHash::Adler32(0, p_src_buf, src_size);
				assert(r == pattern_value);
			}
			if(src_size > 10) {
				SlHash::State st;
				size_t total_sz = 0;
				uint32 r = 0;
				for(size_t ps = 1; total_sz < src_size; ps++) {
					SlHash::Adler32(&st, PTR8C(p_src_buf)+total_sz, MIN(ps, (src_size - total_sz)));
					total_sz += ps;
				}
				SlHash::Adler32(&st, 0, 0); // finalize
				r = st.GetResult32();
				assert(r == pattern_value);
			}
		}
	}
	{
		(in_file_name = data_trasform_path).Cat("sha1.vec");
		data_set.freeAll();
		BdtTestItem::ReadBdtTestData(in_file_name, "SHA-160", data_set);
		for(uint i = 0; i < data_set.getCount(); i++) {
			const BdtTestItem * p_item = data_set.at(i);
			const size_t src_size = p_item->In.GetLen();
			const void * p_src_buf = p_item->In.GetBufC();
			//uint8 result_buf[128];
			const uint8 * p_pattern_buf = static_cast<const uint8 *>(p_item->Out.GetBufC());
			{
				binary160 s1 = SlHash::Sha1(0, p_src_buf, src_size);
				assert(memcmp(&s1, p_pattern_buf, 20) == 0);
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
				assert(memcmp(&s1, p_pattern_buf, 20) == 0);
			}
		}
	}
}

/*
  CRC-Poly-Tab              0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
  {"CRC-4",    "1 1 1 1 1"},
  {"CRC-7",    "1 1 0 1 0 0 0 1"},
  {"CRC-8",    "1 1 1 0 1 0 1 0 1"},
  {"CRC-12",   "1 1 0 0 0 0 0 0 0 1 1 1 1"},
  {"CRC-24",   "1 1 0 0 0 0 0 0 0 0 1 0 1 0 0 0 1 0 0 0 0 0 0 0 1"},
  {"CRC-32",   "1 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 1 0 0 1 1 0 0 0 1 1 1 0 0 0 1 0"},
  {"CCITT-4",  "1 0 0 1 1"},
  {"CCITT-5",  "1 1 0 1 0 1"},
  {"CCITT-6",  "1 0 0 0 0 1 1"},
  {"CCITT-16", "1 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 1"},
  {"CCITT-32", "1 0 0 0 0 0 1 0 0 1 1 0 0 0 0 0 1 0 0 0 1 1 1 0 1 1 0 1 1 0 1 1 1"}, // 0xEDB88320ul
  {"WCDMA-8",  "1 1 0 0 1 1 0 1 1"},
  {"WCDMA-12", "1 1 0 0 0 0 0 0 0 1 1 1 1"},
  {"WCDMA-16", "1 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 1"},
  {"WCDMA-24", "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 1 1"},
  {"ATM-8",    "1 0 0 0 0 0 1 1 1"},
  {"ANSI-16",  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1"}, // 0xA001
  {"SDLC-16",  "1 1 0 1 0 0 0 0 0 1 0 0 1 0 1 1 1"},
};
*/
/*
  Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The table is simply the CRC of all possible eight bit values.  This is all
  the information needed to generate CRC's on data a byte at a time for all
  combinations of CRC register values and incoming bytes.
*/
int SCRC32::MakeTab()
{
	ulong  c;
	uint   n, k;
	//
	// terms of polynomial defining this crc (except x^32):
	//
	static const uint8 p[] = { 0,1,2,4,5,7,8,10,11,12,16,22,23,26 }; // CCITT-32 номера битов соответствуют таблице CRC-Poly-Tab с отсчетом от последней позиции
	if(P_Tab)
		return 1;
	P_Tab = new ulong[256];
	if(P_Tab) {
		//
		// make exclusive-or pattern from polynomial (0xedb88320L)
		//
		ulong poly = 0L; // polynomial exclusive-or pattern
		for(n = 0; n < SIZEOFARRAY(p); n++)
			poly |= 1L << (31 - p[n]);
		for(n = 0; n < 256; n++) {
			c = static_cast<ulong>(n);
			for(k = 0; k < 8; k++)
				c = (c & 1) ? (poly ^ (c >> 1)) : (c >> 1);
			P_Tab[n] = c;
		}
		//
#ifndef NDEBUG
		{
			// norm       reverse in  reverse out
			// 0x04C11DB7 0xEDB88320  0x82608EDB
			CrcModel cm(32, SlConst::CrcPoly_BZIP2, CrcModel::fRefIn, 0, 0);
            ulong   cm_test[256];
            for(uint i = 0; i < SIZEOFARRAY(cm_test); i++) {
				cm_test[i] = cm.Tab(i);
				assert(cm_test[i] == P_Tab[i]);
            }
		}
#endif // NDEBUG
		//
		return 1;
	}
	else
		return (SLibError = SLERR_NOMEM, 0);
}

ulong SCRC32::Calc(ulong crc, const void * pData, size_t dataLen)
{
	#define DO1(buf)  crc = P_Tab[((int)crc ^ (*p_buf++)) & 0xff] ^ (crc >> 8);
	#define DO2(buf)  DO1(buf); DO1(buf);
	#define DO4(buf)  DO2(buf); DO2(buf);
	#define DO8(buf)  DO4(buf); DO4(buf);
	ulong result = 0;
	if(pData && dataLen) {
		if(P_Tab == NULL)
			MakeTab();
		const uint8 * p_buf = PTR8C(pData);
		crc = crc ^ 0xffffffffL;
		while(dataLen >= 8) {
			DO8(pData);
			dataLen -= 8;
		}
		if(dataLen) do {
			DO1(pData);
		} while(--dataLen);
		result = (crc ^ 0xffffffffL);
	}
	return result;
	#undef DO1
	#undef DO2
	#undef DO4
	#undef DO8
}
//
//
//
char * FASTCALL SUpceToUpca(const char * pUpce, char * pUpca)
{
	char   code[32];
	char   dest[32];
	STRNSCPY(code, pUpce);
	int    last = code[6] - '0';
	memset(dest, '0', 12);
	dest[11] = 0;
	dest[0] = code[0];
	if(last == 0 || last == 1 || last == 2) {
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[6];

		dest[8] = code[3];
		dest[9] = code[4];
		dest[10] = code[5];
	}
	else if(last == 3) {
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[3];

		dest[9] = code[4];
		dest[10] = code[5];
	}
	else if(last == 4) {
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[3];
		dest[4] = code[4];

		dest[10] = code[5];
	}
	else { // last = 5..9
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[3];
		dest[4] = code[4];
		dest[5] = code[5];

		dest[10] = code[6];
	}
	return strcpy(pUpca, dest);
}
//
//
//
int SCalcCheckDigit(int alg, const char * pInput, size_t inputLen)
{
	int    cd = 0;
	if(pInput && inputLen) {
		size_t  len = 0;
		size_t  i;
		char    code[128];
		for(i = 0; i < inputLen; i++) {
			const char c = pInput[i];
			if(isdec(c)) {
				if(len >= sizeof(code))
					break;
				code[len++] = c;
			}
			else if(!oneof2(c, '-', ' '))
				break;
		}
		if(len) {
			const int _alg = (alg & ~SCHKDIGALG_TEST);
			const int _do_check = BIN(alg & SCHKDIGALG_TEST);
			if(_alg == SCHKDIGALG_BARCODE) {
				int    c = 0;
				int    c1 = 0;
				int    c2 = 0;
				const  size_t _len = _do_check ? (len-1) : len;
				for(i = 0; i < _len; i++) {
					if((i % 2) == 0)
						c1 += (code[_len-i-1] - '0');
					else
						c2 += (code[_len-i-1] - '0');
				}
				c = c1 * 3 + c2;
				cd = '0' + ((c % 10) ? (10 - c % 10) : 0);
				if(_do_check)
					cd = BIN(cd == code[len-1]);
			}
			else if(_alg == SCHKDIGALG_LUHN) {
				/*
				// Num[1..N] — номер карты, Num[N] — контрольная цифра.
				sum = 0
				for i = 1 to N-1 do
					p = Num[N-i]
					if(i mod 2 <> 0) then
						p = 2*p
						if(p > 9) then
							p = p - 9
						end if
					end if
					sum = sum + p
				next i
				//дополнение до 10
				sum = 10 - (sum mod 10)
				if(sum == 10) then
					sum = 0
				end if
				Num[N] = sum
				*/
				int    s = 0;
				const size_t _len = _do_check ? (len-1) : len;
				for(i = 0; i < _len; i++) {
					int    p = (code[_len - i - 1] - '0');
					if((i & 1) == 0) {
						p <<= 1; // *2
						if(p > 9)
							p -= 9;
					}
					s += p;
				}
				s = 10 - (s % 10);
				if(s == 10)
					s = 0;
				cd = '0' + s;
				if(_do_check)
					cd = BIN(cd == code[len-1]);
			}
			else if(_alg == SCHKDIGALG_RUINN) {
				//int CheckINN(const char * pCode)
				{
					int    r = 1;
					if((_do_check && len == 10) || (!_do_check && len == 9)) {
						const int8 w[] = {2,4,10,3,5,9,4,6,8,0};
						ulong  sum = 0;
						for(i = 0; i < 9; i++) {
							uint   p = (code[i] - '0');
							sum += (w[i] * p);
						}
						cd = '0' + (sum % 11) % 10;
						if(_do_check) {
							cd = BIN(code[9] == cd);
						}
					}
					else if((_do_check && len == 12) || (!_do_check && len == 11)) {
						if(_do_check) {
							static const int8 w1[] = {7,2,4,10, 3,5,9,4,6,8,0};
							static const int8 w2[] = {3,7,2, 4,10,3,5,9,4,6,8,0};
							ulong  sum1 = 0, sum2 = 0;
							for(i = 0; i < 11; i++) {
								uint   p = (code[i] - '0');
								sum1 += (w1[i] * p);
							}
							for(i = 0; i < 12; i++) {
								uint   p = (code[i] - '0');
								sum2 += (w2[i] * p);
							}
							int    cd1 = (sum1 % 11) % 10;
							int    cd2 = (sum2 % 11) % 10;
							cd = BIN((code[10]-'0') == cd1 && (code[11]-'0') == cd2);
						}
						else {
							cd = -1;
						}
					}
					else
						cd = 0;
				}
			}
			else if(_alg == SCHKDIGALG_RUOKATO) {
			}
			else if(_alg == SCHKDIGALG_RUSNILS) {
			}
			else if(_alg == SCHKDIGALG_SSCC) { // @v12.4.5
				// @construction
				/*
					Алгоритм расчета контрольной цифры для (00)04611234567890123):
															   12345678901234567

						Присвойте веса: цифры на нечетных позициях (считая слева) умножаются на 3, на четных — на 1.
						Позиции: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
						Цифры: 0, 4, 6, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3
						Считаем:
							Нечетные позиции (x3): (0x3) + (6x3) + (1x3) + (3x3) + (5x3) + (7x3) + (9x3) + (1x3) + (3x3) = 0 + 18 + 3 + 9 + 15 + 21 + 27 + 3 + 9 = 105
							Четные позиции (x1): (4x1) + (1x1) + (2x1) + (4x1) + (6x1) + (8x1) + (0x1) + (2x1) = 4 + 1 + 2 + 4 + 6 + 8 + 0 + 2 = 27
						Суммируем: 105 + 27 = 132
						Контрольная цифра — это наименьшее число, которое, будучи добавленным к сумме, дает число, кратное 10: 132 + 8 = 140.
						Контрольная цифра = 8
				*/
				if(len == 18 || (len == 20 && code[0] == '0' && code[1] == '0')) {
					const uint _shift = (len == 20) ? 2 : 0;
					uint sum = 0;
					for(i = 1; i <= (len-1-_shift); i++) { // i odd/even?
						const uint dig = code[i-1+_shift] - '0';
						sum += ((i & 0x01) ? (dig * 3) : (dig));
					}
					cd = (iroundup(sum, 10u) - sum) + '0';
					if(_do_check) {
						cd = (cd == code[len-1]);
					}
				}
			}
		}
	}
	return cd;
}

int FASTCALL SCalcBarcodeCheckDigitL(const char * pBarcode, size_t len)
{
	int    cd = 0;
	if(pBarcode && len) {
		if(len == 7 && pBarcode[0] == '0') {
			char   code[64];
			memcpy(code, pBarcode, len);
			code[len] = 0;
			len = sstrlen(SUpceToUpca(code, code));
			cd = SCalcCheckDigit(SCHKDIGALG_BARCODE, code, len);
			cd = isdec(cd) ? (cd - '0') : 0;
		}
		else {
			cd = SCalcCheckDigit(SCHKDIGALG_BARCODE, pBarcode, len);
			cd = isdec(cd) ? (cd - '0') : 0;
		}
	}
	return cd;
}
//
//
//
#define CDTCLS_HASH      1
#define CDTCLS_CRYPT     2

#define CDTF_ADDITIVE    0x0001
#define CDTF_REVERSIBLE  0x0002
#define CDTF_OUTSIZEFIX  0x0004
#define CDTF_OUTSIZEMULT 0x0008
#define CDTF_NEEDINIT    0x0010
#define CDTF_NEEDFINISH  0x0020

#define CDT_PHASE_UPDATE   0
#define CDT_PHASE_INIT     1
#define CDT_PHASE_FINISH   2
#define CDT_PHASE_TEST   100

struct SDataTransformAlgorithm {
	int    Alg;
	int    Cls;
	long   Flags;
	uint32 InSizeParam;
	uint32 OutSizeParam;
	const char * P_Symb;
};

int SDataTransform(int alg, int phase, int inpFormat, int outpFormat, const SBaseBuffer & rIn, SBaseBuffer & rOut);
int SDataTransform(int alg, int phase, int inpFormat, int outpFormat, const void * pIn, size_t inLen, void * pOut, size_t outLen, size_t * pResultOutOffs);
uint32 StHash32(int alg, const SBaseBuffer & rIn);
uint32 StCheckSum(int alg, int phase, const SBaseBuffer & rIn);
//
//
//
SBdtFunct::State_::State_() : P_Tab(0), P_Ctx(0), P_Ext(0)
{
	MEMSZERO(O);
	MEMSZERO(S);
}

SBdtFunct::State_ & SBdtFunct::State_::Z()
{
	P_Tab = 0;
	P_Ext = 0;
	P_Ctx = 0;
	MEMSZERO(O);
	MEMSZERO(S);
	return *this;
}

void SBdtFunct::Info::Set(int alg, int cls, uint flags, int inverseAlg, uint inBufQuant, uint outSize, uint keyLen)
{
	Alg = alg;
	Cls = cls;
	InverseAlg = inverseAlg;
	Flags = flags;
	InBufQuant = inBufQuant;
	OutSize = outSize;
	KeyLen = keyLen;
}

SBdtFunct::TransformBlock::TransformBlock() : Phase(phaseInit), InBufLen(0), P_InBuf(0), P_OutBuf(0), OutBufLen(0), OutBufPos(0)
{
	assert(Phase == phaseInit);
}

SBdtFunct::TransformBlock::TransformBlock(const void * pInBuf, size_t inBufLen, void * pOutBuf, size_t outBufLen) : Phase(phaseUpdate), 
	InBufLen(inBufLen), P_InBuf(pInBuf), P_OutBuf(pOutBuf), OutBufLen(outBufLen), OutBufPos(0)
{
	assert(Phase == phaseUpdate);
}

SBdtFunct::TransformBlock::TransformBlock(void * pOutBuf, size_t outBufLen) : Phase(phaseFinish), InBufLen(0), P_InBuf(0), P_OutBuf(pOutBuf), OutBufLen(outBufLen), OutBufPos(0)
{
}

SBdtFunct::TransformBlock::TransformBlock(SBdtFunct::Info * pInfo) : Phase(phaseGetInfo), InBufLen(0), P_InBuf(0), P_OutBuf(pInfo), OutBufLen(pInfo ? sizeof(*pInfo) : 0), OutBufPos(0)
{
}

SBdtFunct::TransformBlock::TransformBlock(SBdtFunct::Stat * pStat) : Phase(phaseGetStat), InBufLen(0), P_InBuf(0), P_OutBuf(pStat), OutBufLen(pStat ? sizeof(*pStat) : 0), OutBufPos(0)
{
}

SBdtFunct::SBdtFunct(int alg) : A(alg)
{
	Key.Init();
	// @v10.6.4 @ctr Ste.Z();
}

SBdtFunct::~SBdtFunct()
{
	Key.Destroy();
}

int SBdtFunct::GetInfo(SBdtFunct::Info & rInfo)
{
	TransformBlock blk(&rInfo);
	return Implement_Transform(blk);
}

int SBdtFunct::GetStat(Stat & rResult)
{
	TransformBlock blk(&rResult);
	return Implement_Transform(blk);
}

int SBdtFunct::Init()
{
	TransformBlock blk;
	return Implement_Transform(blk);
}

int SBdtFunct::SetParam(int param, const void * pData, size_t dataLen)
{
	int    ok = 0;
	if(param == paramKey) {
		Key.Destroy();
		if(dataLen) {
			if(Key.Alloc(dataLen)) {
				memcpy(Key.P_Buf, pData, dataLen);
				ok = 1;
			}
			else
				ok = 0;
		}
		else
			ok = 1;
	}
	return ok;
}

int SBdtFunct::Update(const void * pInBuf, size_t inBufLen, void * pOutBuf, size_t outBufLen, size_t * pOutOffs)
{
	TransformBlock blk(pInBuf, inBufLen, pOutBuf, outBufLen);
	int ok = Implement_Transform(blk);
	ASSIGN_PTR(pOutOffs, blk.OutBufPos);
	return ok;
}

int SBdtFunct::Finish(void * pOutBuf, size_t outBufLen, size_t * pOutOffs)
{
	TransformBlock blk(pOutBuf, outBufLen);
	int ok = Implement_Transform(blk);
	ASSIGN_PTR(pOutOffs, blk.OutBufPos);
	return ok;
}

int FASTCALL SBdtFunct::Implement_Transform(TransformBlock & rBlk)
{
	static uint _Tab_Crc32_Idx = 0; // SlSession SClassWrapper

	int    ok = 1;
	if(rBlk.Phase == phaseInit)
		Ste.Z();
    switch(A) {
		case SBdtFunct::Crc32:
			switch(rBlk.Phase) {
				case phaseGetInfo:
					{
						Info * p_inf = static_cast<Info *>(rBlk.P_OutBuf);
						if(p_inf)
							p_inf->Set(A, clsHash, fFixedSize|fWriteAtFinish, 0, 0, 4, 0);
					}
					break;
				case SBdtFunct::phaseInit:
					{
						int    do_init_tab = 0;
						if(!_Tab_Crc32_Idx) {
							TSClassWrapper <SBdtFunct::Tab4_256> cls;
							_Tab_Crc32_Idx = SLS.CreateGlobalObject(cls);
							do_init_tab = 1;
						}
						uint32 * p_tab = static_cast<uint32 *>(SLS.GetGlobalObject(_Tab_Crc32_Idx));
						THROW(p_tab);
						if(do_init_tab) {
							// norm       reverse in  reverse out
							// 0x04C11DB7 0xEDB88320  0x82608EDB
							CrcModel cm(32, SlConst::CrcPoly_BZIP2, CrcModel::fRefIn, 0, 0);
							for(uint i = 0; i < 256; i++)
								p_tab[i] = cm.Tab(i);
						}
						Ste.P_Tab = p_tab;
						Ste.O.B4 = 0;
					}
					break;
				case SBdtFunct::phaseUpdate:
					{
						const ulong * p_tab = static_cast<const ulong *>(Ste.P_Tab);
						THROW(p_tab);

						#define DO1(buf)  crc = p_tab[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
						#define DO2(buf)  DO1(buf); DO1(buf);
						#define DO4(buf)  DO2(buf); DO2(buf);
						#define DO8(buf)  DO4(buf); DO4(buf);

						uint32 crc = Ste.O.B4;
						crc = crc ^ 0xffffffffL;
						size_t len = rBlk.InBufLen;
						const uint8 * p_buf = PTR8C(rBlk.P_InBuf);
						while(len >= 8) {
							DO8(p_buf);
							len -= 8;
						}
						if(len)
							do {
								DO1(p_buf);
							} while(--len);
						Ste.O.B4 = crc ^ 0xffffffffL;
						Ste.S.InSize += rBlk.InBufLen;
						Ste.S.OutSize = sizeof(crc);

						#undef DO1
						#undef DO2
						#undef DO4
						#undef DO8
					}
					break;
				case SBdtFunct::phaseFinish:
					{
						if(rBlk.P_OutBuf) {
							THROW(rBlk.OutBufLen >= sizeof(Ste.O.B4));
							PTR32(rBlk.P_OutBuf)[0] = Ste.O.B4;
							rBlk.OutBufPos = sizeof(Ste.O.B4);
						}
					}
					break;
				case SBdtFunct::phaseGetStat:
					{
                        Stat * p_stat = static_cast<Stat *>(rBlk.P_OutBuf);
						assert(!p_stat || rBlk.OutBufLen >= sizeof(Stat));
                        ASSIGN_PTR(p_stat, Ste.S);
					}
					break;
			}
			break;
		case SBdtFunct::Crc24:
			switch(rBlk.Phase) {
				case SBdtFunct::phaseInit:
					break;
				case SBdtFunct::phaseUpdate:
					break;
				case SBdtFunct::phaseFinish:
					break;
			}
			break;
		case SBdtFunct::Crc16:
			switch(rBlk.Phase) {
				case SBdtFunct::phaseInit: break;
				case SBdtFunct::phaseUpdate: break;
				case SBdtFunct::phaseFinish: break;
			}
			break;
		case SBdtFunct::Adler32:
			switch(rBlk.Phase) {
				case phaseGetInfo:
					{
						Info * p_inf = static_cast<Info *>(rBlk.P_OutBuf);
						CALLPTRMEMB(p_inf, Set(A, clsHash, fFixedSize|fWriteAtFinish, 0, 0, 4, 0));
					}
					break;
				case phaseInit:
					{
						Ste.P_Tab = 0;
						Ste.O.B4 = 1/*__adler32(0, 0, 0)*/;
					}
					break;
				case phaseUpdate:
					//Ste.O.B4 = __adler32(Ste.O.B4, PTR8(rBlk.P_InBuf), rBlk.InBufLen);
					//static ulong __adler32(ulong value, const uint8 * pBuf, size_t len)
					{
						#define DO1(buf, i) { value += (buf)[i]; sum2 += value; }
						#define DO2(buf, i)  DO1(buf, i); DO1(buf, i+1);
						#define DO4(buf, i)  DO2(buf, i); DO2(buf, i+2);
						#define DO8(buf, i)  DO4(buf, i); DO4(buf, i+4);
						#define DO16(buf)    DO8(buf, 0); DO8(buf, 8);
						ulong value = Ste.O.B4;
						const uint8 * p_buf = PTR8C(rBlk.P_InBuf);
						size_t len = rBlk.InBufLen;
						const uint _base_ = 65521U; // largest prime smaller than 65536
						const uint _nmax_ = 5552;   // _nmax_ is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1
						ulong  result = 1;
						uint   n;
						ulong  sum2 = (value >> 16) & 0xffff; // split Adler-32 into component sums
						value &= 0xffff;
						if(len == 1) { // in case user likes doing a byte at a time, keep it fast
							value += p_buf[0];
							if(value >= _base_)
								value -= _base_;
							sum2 += value;
							if(sum2 >= _base_)
								sum2 -= _base_;
							result = (value | (sum2 << 16));
						}
						else if(p_buf) { // initial Adler-32 value (deferred check for len == 1 speed)
							if(len < 16) { // in case short lengths are provided, keep it somewhat fast
								while(len--) {
									value += *p_buf++;
									sum2 += value;
								}
								if(value >= _base_)
									value -= _base_;
								sum2 %= _base_; // only added so many _base_'s
								result = (value | (sum2 << 16));
							}
							else {
								// do length NMAX blocks -- requires just one modulo operation
								while(len >= _nmax_) {
									len -= _nmax_;
									n = _nmax_ / 16; // NMAX is divisible by 16
									do {
										DO16(p_buf); // 16 sums unrolled
										p_buf += 16;
									} while(--n);
									value %= _base_;
									sum2 %= _base_;
								}
								// do remaining bytes (less than NMAX, still just one modulo)
								if(len) { // avoid modulos if none remaining
									while(len >= 16) {
										len -= 16;
										DO16(p_buf);
										p_buf += 16;
									}
									while(len--) {
										value += *p_buf++;
										sum2 += value;
									}
									value %= _base_;
									sum2 %= _base_;
								}
								result = (value | (sum2 << 16)); // return recombined sums
							}
						}
						Ste.O.B4 = result;
						#undef DO1
						#undef DO2
						#undef DO4
						#undef DO8
						#undef DO16
					}
					Ste.S.InSize += rBlk.InBufLen;
					Ste.S.OutSize = sizeof(Ste.O.B4);
					break;
				case phaseFinish:
					if(rBlk.P_OutBuf) {
						THROW(rBlk.OutBufLen >= sizeof(Ste.O.B4));
						PTR32(rBlk.P_OutBuf)[0] = Ste.O.B4;
						rBlk.OutBufPos = sizeof(Ste.O.B4);
					}
					break;
				case SBdtFunct::phaseGetStat:
					{
                        Stat * p_stat = static_cast<Stat *>(rBlk.P_OutBuf);
						assert(!p_stat || rBlk.OutBufLen >= sizeof(Stat));
                        ASSIGN_PTR(p_stat, Ste.S);
					}
					break;
			}
			break;
		case SBdtFunct::MD2:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::MD4:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::MD5:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::SHA160:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::SHA224:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
#ifdef _LOCAL_USE_SSL // {
		case SBdtFunct::IdeaCrypt:
		case SBdtFunct::IdeaDecrypt:
			switch(rBlk.Phase) {
				case phaseGetInfo:
					{
						Info * p_inf = (Info *)rBlk.P_OutBuf;
						if(p_inf) {
							p_inf->Set(A, clsCrypt, fKey|fReversible, 
								((A == SBdtFunct::IdeaDecrypt) ? SBdtFunct::IdeaCrypt : SBdtFunct::IdeaDecrypt), 0, 0, 16);
						}
					}
					break;
				case phaseInit:
					{
						uint16 iv[4];
						uint8  key[16];
						memzero(iv, sizeof(iv));
						THROW(Key.Size == 16 && Key.P_Buf);
						memcpy(key, Key.P_Buf, Key.Size);
						Ste.P_Ctx = EVP_CIPHER_CTX_new();
						Ste.P_Tab = EVP_idea_cbc(); // EVP_idea_cfb();
						if(A == SBdtFunct::IdeaCrypt) {
							THROW(EVP_EncryptInit((EVP_CIPHER_CTX *)Ste.P_Ctx, (const EVP_CIPHER *)Ste.P_Tab, key, PTR8(iv)));
						}
						else {
							THROW(EVP_DecryptInit((EVP_CIPHER_CTX *)Ste.P_Ctx, (const EVP_CIPHER *)Ste.P_Tab, key, PTR8(iv)));
						}
						//Ste.P_Ext = new IDEACFB(iv, key, BIN(A == SBdtFunct::IdeaDecrypt)); 
					}
					break;
				case phaseUpdate:
					THROW(Ste.P_Ctx);
					THROW(Ste.P_Tab);
					if(rBlk.InBufLen > 0) {
						THROW(rBlk.OutBufLen >= rBlk.InBufLen);
						THROW(rBlk.P_InBuf && rBlk.P_OutBuf);
						//memcpy(rBlk.P_OutBuf, rBlk.P_InBuf, rBlk.InBufLen);
						//((IDEACFB *)Ste.P_Ext)->run(PTR8(rBlk.P_OutBuf), rBlk.InBufLen);
						//rBlk.OutBufPos = rBlk.InBufLen;

						int    out_len = (int)rBlk.OutBufLen;
						if(A == SBdtFunct::IdeaCrypt) {
							THROW(EVP_EncryptUpdate((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len, PTR8(rBlk.P_InBuf), rBlk.InBufLen));
						}
						else {
							THROW(EVP_DecryptUpdate((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len, PTR8(rBlk.P_InBuf), rBlk.InBufLen));
						}
						rBlk.OutBufPos = (size_t)out_len;
					}
					break;
				case phaseFinish:
					//delete (IDEACFB *)Ste.P_Ext;
					//Ste.P_Ext = 0;

					THROW(Ste.P_Ctx);
					int    out_len = (int)rBlk.OutBufLen;
					if(A == SBdtFunct::IdeaCrypt) {
						THROW(EVP_EncryptFinal((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len));
					}
					else {
						THROW(EVP_DecryptFinal((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len));
					}
					rBlk.OutBufPos = (size_t)out_len;
					EVP_CIPHER_CTX_free((EVP_CIPHER_CTX *)Ste.P_Ctx);
					break;
			}
			break;
#endif // } _LOCAL_USE_SSL 
		default:
			break;
    }
    CATCHZOK
	return ok;
}
