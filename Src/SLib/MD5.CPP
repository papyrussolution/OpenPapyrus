/**
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.	This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * @license Public Domain / GPL v2+
 */
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <zlib.h>
// @v9.6.3 #include <idea.h>
/**
 * Reverse the bytes in 32-bit chunks.
 * 'buf' might not be word-aligned.
 *
 * @param buf     The byte array to reverse
 * @param n_u32   The number of uint32's in the data
 */
void S_MD5::reverse_u32(uint8 * buf, int n_u32)
{
	uint8 tmp;
	if(SLS.GetSSys().Flags & SSystem::fBigEndian) {
		/* change { 4, 3, 2, 1 } => { 1, 2, 3, 4 } */
		while(n_u32-- > 0) {
			tmp    = buf[0];
			buf[0] = buf[3];
			buf[3] = tmp;

			tmp    = buf[1];
			buf[1] = buf[2];
			buf[2] = tmp;

			buf += 4;
		}
	}
	else {
		/* change { 4, 3, 2, 1 } => { 3, 4, 1, 2 } */
		while(n_u32-- > 0) {
			tmp    = buf[0];
			buf[0] = buf[1];
			buf[1] = tmp;

			tmp    = buf[2];
			buf[2] = buf[3];
			buf[3] = tmp;

			buf += 4;
		}
	}
}

S_MD5::S_MD5()
{
	m_buf[0] = 0x01020304;
	/*
	 * Little endian = { 4, 3, 2, 1 }
	 * Big endian    = { 1, 2, 3, 4 }
	 * PDP endian    = { 3, 4, 1, 2 }
	 *
	 * The MD5 stuff is written for little endian.
	 */
	m_need_byteswap = *(uint8 *)m_buf != 4;
	//m_big_endian    = *(uint8 *)m_buf == 1;
}

S_MD5::~S_MD5()
{
}
/**
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void S_MD5::Init()
{
	m_buf[0] = 0x67452301;
	m_buf[1] = 0xefcdab89;
	m_buf[2] = 0x98badcfe;
	m_buf[3] = 0x10325476;

	m_bits[0] = 0;
	m_bits[1] = 0;
}

/**
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void S_MD5::Update(const void * data, uint32 len)
{
	const uint8 * buf = (const uint8 *)data;
	/* Update bitcount */
	uint32 t = m_bits[0];
	if((m_bits[0] = t+((uint32)len<<3)) < t) {
		m_bits[1]++; /* Carry from low to high */
	}
	m_bits[1] += len>>29;

	t = (t>>3)&0x3f;  /* Bytes already in shsInfo->data */
	/* Handle any leading odd-sized chunks */
	if(t) {
		uint8 * p = (uint8 *)m_in+t;
		t = 64-t;
		if(len < t) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		if(m_need_byteswap) {
			reverse_u32(m_in, 16);
		}
		Transform(m_buf, (uint32 *)m_in);
		buf += t;
		len -= t;
	}
	/* Process data in 64-byte chunks */

	while(len >= 64) {
		memcpy(m_in, buf, 64);
		if(m_need_byteswap) {
			reverse_u32(m_in, 16);
		}
		Transform(m_buf, (uint32 *)m_in);
		buf += 64;
		len -= 64;
	}
	/* Save off any remaining bytes of data */
	memcpy(m_in, buf, len);
}

/**
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void S_MD5::Final(uint8 digest[16])
{
	uint32 count = (m_bits[0]>>3)&0x3F; /* Compute number of bytes mod 64 */
	/* Set the first char of padding to 0x80.  This is safe since there is
	 * always at least one byte free */
	uint8 * p    = m_in+count;
	*p++ = 0x80;
	/* Bytes of padding needed to make 64 bytes */
	count = 64-1-count;
	/* Pad out to 56 mod 64 */
	if(count < 8) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memzero(p, count);
		if(m_need_byteswap) {
			reverse_u32(m_in, 16);
		}
		Transform(m_buf, (uint32 *)m_in);
		/* Now fill the next block with 56 bytes */
		memzero(m_in, 56);
	}
	else {
		/* Pad block to 56 bytes */
		memzero(p, count-8);
	}
	if(m_need_byteswap) {
		reverse_u32(m_in, 14);
	}
	/* Append length in bits and transform */
	((uint32 *)m_in)[14] = m_bits[0];
	((uint32 *)m_in)[15] = m_bits[1];

	Transform(m_buf, (uint32 *)m_in);
	if(m_need_byteswap) {
		reverse_u32((uint8 *)m_buf, 4);
	}
	memcpy(digest, m_buf, 16);
}

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z)    (z^(x&(y^z)))
#define F2(x, y, z)    F1(z, x, y)
#define F3(x, y, z)    ((x)^(y)^(z))
#define F4(x, y, z)    ((y)^((x)|~(z)))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) (w += f(x, y, z)+data, w = w<<s|w>>(32-s), w += x)

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5::Update blocks
 * the data and converts bytes into longwords for this routine.
 */
void S_MD5::Transform(uint32 buf[4], uint32 in_data[16])
{
	uint32 a = buf[0];
	uint32 b = buf[1];
	uint32 c = buf[2];
	uint32 d = buf[3];

	MD5STEP(F1, a, b, c, d, in_data[0]+0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in_data[1]+0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in_data[2]+0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in_data[3]+0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in_data[4]+0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in_data[5]+0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in_data[6]+0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in_data[7]+0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in_data[8]+0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in_data[9]+0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in_data[10]+0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in_data[11]+0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in_data[12]+0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in_data[13]+0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in_data[14]+0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in_data[15]+0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in_data[1]+0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in_data[6]+0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in_data[11]+0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in_data[0]+0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in_data[5]+0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in_data[10]+0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in_data[15]+0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in_data[4]+0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in_data[9]+0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in_data[14]+0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in_data[3]+0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in_data[8]+0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in_data[13]+0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in_data[2]+0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in_data[7]+0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in_data[12]+0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in_data[5]+0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in_data[8]+0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in_data[11]+0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in_data[14]+0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in_data[1]+0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in_data[4]+0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in_data[7]+0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in_data[10]+0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in_data[13]+0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in_data[0]+0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in_data[3]+0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in_data[6]+0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in_data[9]+0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in_data[12]+0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in_data[15]+0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in_data[2]+0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in_data[0]+0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in_data[7]+0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in_data[14]+0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in_data[5]+0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in_data[12]+0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in_data[3]+0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in_data[10]+0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in_data[1]+0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in_data[8]+0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in_data[15]+0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in_data[6]+0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in_data[13]+0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in_data[4]+0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in_data[11]+0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in_data[2]+0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in_data[9]+0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

void S_MD5::Calc(const void * data, uint32 length, uint8 digest[16])
{
	S_MD5 md5;
	md5.Init();
	md5.Update(data, length);
	md5.Final(digest);
}

#if SLTEST_RUNNING // {

SLTEST_R(TestMd5)
{
	char * p_tab_name = 0;
	uchar md_buf[16];
	uint num_of_rows = 0;
	uint num_of_col = 0;
	int length = 0;
	int    ok = 1;
	int j = 0;
	ulong crc = 0L;
	S_MD5 md_obj;
	STab tab_obj;
	STab::Row row_obj;
	STabFile file_obj;
	SString in_file_name(MakeInputFilePath("hash.tab"));
	SString in_buf, out_buf;
	STempBuffer input(2048);
	STempBuffer output(2048);

	THROW(SLTEST_CHECK_NZ(file_obj.Open(in_file_name, 0)));
	THROW(SLTEST_CHECK_NZ(file_obj.LoadTab("MD5", tab_obj)));
	num_of_rows = tab_obj.GetCount() - 1;
	for (int k = 0; k < (int)num_of_rows; k++) {
		input[0] = 0;
		output[0] = 0;
		memset(md_buf, 0, 16);
		in_buf = 0;
		out_buf = 0;
		THROW(SLTEST_CHECK_NZ(tab_obj.GetRow(k, row_obj)));
		num_of_col = row_obj.GetCount();
		//Может быть и 3 колонки
		if(num_of_col == 2) {
			THROW(SLTEST_CHECK_NZ(row_obj.Get(0, in_buf)));
			THROW(SLTEST_CHECK_NZ(row_obj.Get(1, out_buf)));
			THROW(SLTEST_CHECK_EQ(out_buf.Len() % 2, 0));
			for(j = 0; j < (int)(out_buf.Len()/2); j++) {
				if(j < (int)output.GetSize()) {
					output[j] = hextobyte(((const char *)out_buf)+j*2);
				}
				else {
					// error!!!
				}
			}
			output[j] = 0;
			THROW(SLTEST_CHECK_EQ(in_buf.Len() % 2, 0));
			for(j = 0; j < (int)(in_buf.Len()/2); j++) {
				if(j < (int)input.GetSize()) {
					input[j] = hextobyte(((const char *)in_buf)+j*2);
				}
				else {
					// error!!!
				}
			}
			length = j;
			md_obj.Calc((uchar *)(const char *)input, length, md_buf);
			THROW(SLTEST_CHECK_Z(strcmpi((const char *)output, (const char *)md_buf)));
		}
	}
	tab_obj.Z();
	CATCHZOK
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
