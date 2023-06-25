// BASE64.CPP
// @threadsafe
//
#include <slib-internal.h>
#pragma hdrstop

/*static void FASTCALL makebasis64(char * pBuf)
{
// 
// ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
// ????????????????????????????????????????????????????????????????
// ????????????????????????????????????????????????????????????????
// ???????????
// 
	const size_t count = 203;
	size_t p = 0;
	char   i;
	for(i = 0; i < 'Z'-'A'+1; i++)
		pBuf[p++] = 'A' + i;
	for(i = 0; i < 'z'-'a'+1; i++)
		pBuf[p++] = 'a' + i;
	for(i = 0; i < 10; i++)
		pBuf[p++] = '0'+i;
	pBuf[p++] = '+';
	pBuf[p++] = '/';
	while(p < count)
		pBuf[p++] = '?';
	pBuf[p] = 0;
}*/

int STDCALL encode64(const char * pIn, size_t inLen, char * pOut, size_t outMax, size_t * pOutLen)
{
	int    ok = 1;
	uchar * out = reinterpret_cast<uchar *>(pOut);
	uchar  oval;
	const  uchar * in = reinterpret_cast<const uchar *>(pIn);
	size_t olen = (inLen + 2) / 3 * 4;
	size_t real_len = 0;
	ASSIGN_PTR(pOutLen, olen);
	const char * p_basis = STextConst::Get(STextConst::cBasis64, 0);
	THROW(outMax >= olen);
	while(inLen >= 3) {
		THROW(real_len < (outMax-4)); // user provided max buffer size; make sure we don't go over it
		*out++ = p_basis[in[0] >> 2];
		*out++ = p_basis[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = p_basis[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = p_basis[in[2] & 0x3f];
		real_len += 4;
		in += 3;
		inLen -= 3;
	}
	if(inLen > 0) {
		THROW(real_len < (outMax-4)); // user provided max buffer size; make sure we don't go over it
		*out++ = p_basis[in[0] >> 2];
		oval = (in[0] << 4) & 0x30;
		if(inLen > 1)
			oval |= in[1] >> 4;
		*out++ = p_basis[oval];
		*out++ = (inLen < 2) ? '=' : p_basis[(in[1] << 2) & 0x3c];
		*out++ = '=';
		real_len += 4;
	}
	THROW(real_len < outMax);
	*out = '\0';
	real_len++;
	ASSIGN_PTR(pOutLen, real_len);
	CATCH
		ok = (SLibError = SLERR_BUFTOOSMALL, 0);
	ENDCATCH
	return ok;
}

static const char index_64[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,

	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

#define CHAR64(c) (index_64[(uint8)(c)])

int STDCALL decode64(const char * pIn, size_t inLen, char * pOut, size_t * pOutLen)
{
	assert(sizeof(index_64) == 256);
	int    ok = 1;//, c1 = 0, c2 = 0, c3 = 0, c4 = 0;
	size_t len = 0, lup = 0;
	char * p_out = pOut;
	const  char * p_in = pIn;
	if(p_in[0] == '+' && p_in[1] == ' ')
		p_in += 2;
	THROW_S(*p_in != 0, SLERR_BASE64DECODEFAULT);
	if(p_out) {
		for(lup = 0; lup < inLen / 4; lup++) {
			const int b1 = CHAR64(p_in[0]);
			const int b2 = CHAR64(p_in[1]);
			const int c3 = p_in[2];
			const int b3 = CHAR64(c3);
			const int c4 = p_in[3];
			const int b4 = CHAR64(c4);
			THROW_S(b1 != -1 && b2 != -1 && (c3 == '=' || b3 != -1) && (c4 == '=' || b4 != -1), SLERR_BASE64DECODEFAULT);
			p_in += 4;
			*p_out++ = (char)(b1 << 2) | (b2 >> 4);
			len++;
			if(c3 != '=') {
				*p_out++ = (char)((b2 << 4) & 0xf0) | (b3 >> 2);
				len++;
				if(c4 != '=') {
					*p_out++ = (char)((b3 << 6) & 0xc0) | b4;
					len++;
				}
			}
		}
	}
	else {
		for(lup = 0; lup < inLen / 4; lup++) {
			const int b1 = CHAR64(p_in[0]);
			const int b2 = CHAR64(p_in[1]);
			const int c3 = p_in[2];
			const int b3 = CHAR64(c3);
			const int c4 = p_in[3];
			const int b4 = CHAR64(c4);
			THROW_S(b1 != -1 && b2 != -1 && (c3 == '=' || b3 != -1) && (c4 == '=' || b4 != -1), SLERR_BASE64DECODEFAULT);
			p_in += 4;
			len++;
			if(c3 != '=') {
				len++;
				if(c4 != '=') {
					len++;
				}
			}
		}
	}
	ASSIGN_PTR(pOutLen, len);
	CATCHZOK
	return ok;
}
//
//
//
MIME64::MIME64()
{
}

int MIME64::Encode(const void * pIn, size_t inLen, char * pOut, size_t outBufLen, size_t * pOutDataLen) const
{
	int    ok = 1;
	uchar * out = reinterpret_cast<uchar *>(pOut);
	const  uchar * in = reinterpret_cast<const uchar *>(pIn);
	size_t olen = (inLen + 2) / 3 * 4;
	ASSIGN_PTR(pOutDataLen, olen);
	if(outBufLen >= olen) {
		const char * p_basis = STextConst::Get(STextConst::cBasis64, 0);
		while(inLen >= 3) {
			// user provided max buffer size; make sure we don't go over it
			*out++ = p_basis[in[0] >> 2];
			*out++ = p_basis[((in[0] << 4) & 0x30) | (in[1] >> 4)];
			*out++ = p_basis[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
			*out++ = p_basis[in[2] & 0x3f];
			in += 3;
			inLen -= 3;
		}
		if(inLen > 0) {
			// user provided max buffer size; make sure we don't go over it
			*out++ = p_basis[in[0] >> 2];
			uchar  oval = (in[0] << 4) & 0x30;
			if(inLen > 1)
				oval |= in[1] >> 4;
			*out++ = p_basis[oval];
			*out++ = (inLen < 2) ? '=' : p_basis[(in[1] << 2) & 0x3c];
			*out++ = '=';
		}
		if(olen < outBufLen)
			*out = '\0';
	}
	else
		ok = (SLibError = SLERR_BUFTOOSMALL, 0);
	return ok;
}

int MIME64::Decode(const char * pIn, size_t inLen, char * pOut, size_t * pOutDataLen) const
{
	int    ok = 1;
	size_t len = 0, lup = 0;
	char * p_out = pOut;
	const  char * p_in = pIn;
	if(p_in[0] == '+' && p_in[1] == ' ')
		p_in += 2;
	THROW(*p_in != 0);
	for(lup = 0; lup < inLen / 4; lup++) {
		char   c64[4];
		char   c[4];
		*reinterpret_cast<uint32 *>(c) = *reinterpret_cast<const uint32 *>(p_in);
		p_in += 4;
		c64[0] = CHAR64(c[0]);
		c64[1] = CHAR64(c[1]);
		c64[2] = CHAR64(c[2]);
		c64[3] = CHAR64(c[3]);
		THROW(c64[0] != -1 && c64[1] != -1);
		THROW(c[2] == '=' || c64[2] != -1);
		THROW(c[3] == '=' || c64[3] != -1);
		*p_out++ = (char)(c64[0] << 2) | (c64[1] >> 4);
		len++;
		if(c[2] != '=') {
			*p_out++ = (char)((c64[1] << 4) & 0xf0) | (c64[2] >> 2);
			len++;
			if(c[3] != '=') {
				*p_out++ = (char) ((c64[2] << 6) & 0xc0) | c64[3];
				len++;
			}
		}
	}
	*p_out = 0;
	ASSIGN_PTR(pOutDataLen, len);
	CATCHZOK
	return ok;
}
//
//
//
/**
 * Let this be a sequence of plain data before encoding:
 *
 *  01234567 01234567 01234567 01234567 01234567
 * +--------+--------+--------+--------+--------+
 * |< 0 >< 1| >< 2 ><|.3 >< 4.|>< 5 ><.|6 >< 7 >|
 * +--------+--------+--------+--------+--------+
 *
 * There are 5 octets of 8 bits each in each sequence.
 * There are 8 blocks of 5 bits each in each sequence.
 *
 * You probably want to refer to that graph when reading the algorithms in this
 * file. We use "octet" instead of "byte" intentionnaly as we really work with
 * 8 bits quantities. This implementation will probably not work properly on
 * systems that don't have exactly 8 bits per (unsigned) char.
 **/
/**
 * Given a block id between 0 and 7 inclusive, this will return the index of
 * the octet in which this block starts. For example, given 3 it will return 1
 * because block 3 starts in octet 1:
 *
 * +--------+--------+
 * | ......<|.3 >....|
 * +--------+--------+
 *  octet 1 | octet 2
 */
static int Base32_get_octet(int block)
{
	assert(block >= 0 && block < 8);
	return (block*5) / 8;
}
// 
// Given a block id between 0 and 7 inclusive, this will return how many bits
// we can drop at the end of the octet in which this block starts.
// For example, given block 0 it will return 3 because there are 3 bits
// we don't care about at the end:
// 
//  +--------+-
//  |< 0 >...|
//  +--------+-
// 
// Given block 1, it will return -2 because there
// are actually two bits missing to have a complete block:
// 
//  +--------+-
//  |.....< 1|..
//  +--------+-
// 
static FORCEINLINE int Base32_get_offset(int block)
{
	assert(block >= 0 && block < 8);
	return (8 - 5 - (5*block) % 8);
}
// 
// Like "b >> offset" but it will do the right thing with negative offset.
// We need this as bitwise shifting by a negative offset is undefined behavior.
// 
static FORCEINLINE uchar Base32_shift_right(uchar byte, int8 offset) { return (offset > 0) ? (byte >> offset) : (byte << -offset); }
static FORCEINLINE uchar Base32_shift_left(uchar byte, int8 offset) { return Base32_shift_right(byte, -offset); }
// 
// Encode a sequence. A sequence is no longer than 5 octets by definition.
// Thus passing a length greater than 5 to this function is an error. Encoding
// sequences shorter than 5 octets is supported and padding will be added to the
// output as per the specification.
// 
static void Base32_encode_sequence(const uchar * plain, size_t len, SString & rBuf)
{
	static const uchar base32[] = "abcdefghijklmnopqrstuvwxyz234567";
	static const uchar Base32_PADDING_CHAR = '=';
	assert(CHAR_BIT == 8);  // not sure this would work otherwise
	assert(len >= 0 && len <= 5);
	for(int block = 0; block < 8; block++) {
		int octet = Base32_get_octet(block);  // figure out which octet this block starts in
		int junk  = Base32_get_offset(block);  // how many bits do we drop from this octet?
		if(octet >= static_cast<int>(len)) {  // we hit the end of the buffer
			rBuf.CatCharN(Base32_PADDING_CHAR, len);
			return;
		}
		else {
			uchar c = Base32_shift_right(plain[octet], junk);  // first part
			if(junk < 0 /*is there a second part?*/ && octet < static_cast<int>(len-1)) { // is there still something to read?
				c |= Base32_shift_right(plain[octet+1], 8 + junk);
			}
			rBuf.CatChar(base32[c & 0x1F]);  // 0001 1111
		}
	}
}

int Base32_Encode(const uint8 * pData, size_t dataLen, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	if(pData && dataLen) {
		// All the hard work is done in encode_sequence(),
		// here we just need to feed it the data sequence by sequence.
		for(size_t i = 0, j = 0; i < dataLen; i += 5, j += 8) {
			Base32_encode_sequence(&pData[i], smin(dataLen-i, static_cast<size_t>(5)), /*&coded[j]*/rBuf);
		}
	}
	return ok;
}

static int Base32_decode_sequence(const uchar * pCoded, uchar * plain)
{
	assert(CHAR_BIT == 8);
	assert(pCoded && plain);
	plain[0] = 0;
	for(int block = 0; block < 8; block++) {
		int offset = Base32_get_offset(block);
		int octet  = Base32_get_octet(block);
		int c = -1;
		{
			const uchar chr = pCoded[block];
			if(chr >= 'a' && chr <= 'z')
				c = (chr - 'a');
			else if(chr >= '2' && chr <= '7')
				c = (chr - '2' + 26);
			assert(c == -1 || ((c & 0x1F) == c));
		}
		if(c < 0) // invalid char, stop here
			return octet;
		plain[octet] |= Base32_shift_left(c, offset);
		if(offset < 0) {   // does this block overflows to next octet?
			assert(octet < 4);
			plain[octet+1] = Base32_shift_left(c, 8 + offset);
		}
	}
	return 5;
}

size_t Base32_Decode(const uchar * coded, SBinaryChunk & rResult)
{
	rResult.Z();
	size_t written = 0;
	for(size_t i = 0, j = 0;; i += 8, j += 5) {
		uint8 step_buf[32];
		int n = Base32_decode_sequence(&coded[i], /*&plain[j]*/step_buf);
		written += n;
		if(n > 0) {
			rResult.Cat(step_buf, n);
		}
		if(n < 5)
			return written;
	}
}

size_t base32_encoded_size(size_t count)
{
	size_t size = (count << 3); // multiply by 8 
	if(size % 5 == 0)
		return (size/5);
	else
		return (1+size/5);
}

size_t base32_decoded_size(size_t count)
{
    size_t size = count * 5;
    return (size >> 3) + (size % 8) ? 1 : 0;
}
//
//
//
static const char kHexTable[] =
	"000102030405060708090a0b0c0d0e0f"
	"101112131415161718191a1b1c1d1e1f"
	"202122232425262728292a2b2c2d2e2f"
	"303132333435363738393a3b3c3d3e3f"
	"404142434445464748494a4b4c4d4e4f"
	"505152535455565758595a5b5c5d5e5f"
	"606162636465666768696a6b6c6d6e6f"
	"707172737475767778797a7b7c7d7e7f"
	"808182838485868788898a8b8c8d8e8f"
	"909192939495969798999a9b9c9d9e9f"
	"a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
	"b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
	"c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
	"d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
	"e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
	"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

static const char kHexValueLenient[] = {
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0,// '0'..'9'
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 'A'..'F'
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 'a'..'f'
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

size_t Hex_Encode(uint flags, const uint8 * pData, size_t dataLen, SString & rBuf)
{
	rBuf.Z();
	for(size_t i = 0; i < dataLen; i++) {
		const char * p_hex = &kHexTable[pData[i] * 2];
		rBuf.CatN(p_hex, 2);
	}
	return rBuf.Len();
}

size_t Hex_Decode(const uchar * pCoded, size_t srcLen, SBinaryChunk & rResult)
{
	rResult.Z();
	for(size_t i = 0; i < srcLen/2; i++) {
		rResult.Cat((kHexValueLenient[pCoded[i * 2] & 0xFF] << 4) + (kHexValueLenient[pCoded[i * 2 + 1] & 0xFF]));
	}
	return rResult.Len();
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(Base32)
{
	int    ok = 1;
	SString temp_buf;
	SBinaryChunk bc1;
	SBinaryChunk bc2;
	uint   iteridx = 0;
	Base32_Encode(0, 13, temp_buf);
	THROW(SLCHECK_NZ(temp_buf.IsEmpty()));
	/*for(size_t i = 0; i < SIZEOFARRAY(p_test_pair); i++) {
		const TestPair & r_pair = p_test_pair[i];
		THROW(SLCHECK_NZ(ZBase32_Encode(reinterpret_cast<const uint8 *>(r_pair.P_Src), sstrlen(r_pair.P_Src), temp_buf)));
		THROW(SLCHECK_EQ(temp_buf, r_pair.P_Dest));
		//
		THROW(SLCHECK_NZ(ZBase32_Decode(temp_buf, bc1)));
		THROW(SLCHECK_NZ(bc1.IsEq(reinterpret_cast<const uint8 *>(r_pair.P_Src), sstrlen(r_pair.P_Src))));
	}*/
	{
		bc1.Z();
		Base32_Encode(static_cast<const uint8 *>(bc1.PtrC()), bc1.Len(), temp_buf);
		Base32_Decode(temp_buf.ucptr(), bc2);
	}
	{
		{
			iteridx = 32;
			bc1.Z().Cat(&iteridx, 4);
			Base32_Encode(static_cast<const uint8 *>(bc1.PtrC()), bc1.Len(), temp_buf);
			Base32_Decode(temp_buf.ucptr(), bc2);
			THROW(SLCHECK_NZ(bc1.IsEq(bc2)));
		}
		for(iteridx = 0; iteridx < 1000000; iteridx++) {
			bc1.Z().Cat(&iteridx, 4);
			Base32_Encode(static_cast<const uint8 *>(bc1.PtrC()), bc1.Len(), temp_buf);
			Base32_Decode(temp_buf.ucptr(), bc2);
			THROW(SLCHECK_NZ(bc1.IsEq(bc2)));
		}
	}
	{
		for(iteridx = 0; iteridx < 1000; iteridx++) {
			bc1.Randomize(SLS.GetTLA().Rg.GetUniformInt(SKILOBYTE(64)));
			Base32_Encode(static_cast<const uint8 *>(bc1.PtrC()), bc1.Len(), temp_buf);
			Base32_Decode(temp_buf.ucptr(), bc2);
			THROW(SLCHECK_NZ(bc1.IsEq(bc2)));
		}
	}
	CATCHZOK
	return ok ? CurrentStatus : 0;
}

#endif // } SLTEST_RUNNING

/*
class ZBase32EncoderTest {        
    [TestCase("BA", "ejyo")]
    [TestCase("a", "cr")]
    [TestCase("", "")]
    public void EncodingTest(string sourceData, string encodedData)
    {
        var bytes = Encoding.ASCII.GetBytes(sourceData);
        var result = ZBase32Encoder.Encode(bytes);
        Assert.AreEqual(encodedData, result);
    }

    [TestCase("Hello, World!")]
    [TestCase("&^%%&*BJKjbjkb&^%%$^&b")]
    [TestCase("  My NaMe Is DeNiS ")]
    [TestCase("--=__--=)(\\//$4")]
    [TestCase("  ")]
    [TestCase("")]
    public void EncodeDecodeTest(string sourceData)
    {
        var bytes = Encoding.ASCII.GetBytes(sourceData);
        var encodedData = ZBase32Encoder.Encode(bytes);
        var decodedData = ZBase32Encoder.Decode(encodedData);

        Assert.That(Encoding.ASCII.GetBytes(sourceData), Is.EquivalentTo(decodedData));
    }
}
*/