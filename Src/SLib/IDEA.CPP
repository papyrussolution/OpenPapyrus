/*
	idea.cpp - C++ source code for IDEA block cipher.
	IDEA (International Data Encryption Algorithm), formerly known as
	IPES (Improved Proposed Encryption Standard).
	Algorithm developed by Xuejia Lai and James L. Massey, of ETH Zurich.

	This implementation modified and derived from original C code
	developed by Xuejia Lai.
	Zero-based indexing added, names changed from IPES to IDEA.
	CFB functions added.  Random number routines added.

	Optimized for speed 21 Oct 92 by Colin Plumb.
	Very minor speedup on 23 Feb 93 by Colin Plumb.
	idearand() given a separate expanded key on 25 Feb 93, Colin Plumb.

	There are two adjustments that can be made to this code to
	speed it up.  Defaults may be used for PCs.  Only the -DIDEA32
	pays off significantly if selectively set or not set.
	Experiment to see what works better for you.

	Multiplication: default is inline, -DAVOID_JUMPS uses a
		different version that does not do any conditional
		jumps (a few percent worse on a SPARC), while
		-DSMALL_CACHE takes it out of line to stay
		within a small on-chip code cache.
	Variables: normally, 16-bit variables are used, but some
		machines (notably RISCs) do not have 16-bit registers,
		so they do a great deal of masking.  -DIDEA32 uses "int"
		register variables and masks explicitly only where
		necessary.  On a SPARC, for example, this boosts
		performace by 30%.

	The IDEA(tm) block cipher is covered by a patent held by ETH and a
	Swiss company called Ascom-Tech AG.  The Swiss patent number is
	PCT/CH91/00117.  International patents are pending. IDEA(tm) is a
	trademark of Ascom-Tech AG.  There is no license fee required for
	noncommercial use.  Commercial users may obtain licensing details
	from Dieter Profos, Ascom Tech AG, Solothurn Lab, Postfach 151, 4502
	Solothurn, Switzerland, Tel +41 65 242885, Fax +41 65 235761.

	The IDEA block cipher uses a 64-bit block size, and a 128-bit key
	size.  It breaks the 64-bit cipher block into four 16-bit words
	because all of the primitive inner operations are done with 16-bit
	arithmetic.  It likewise breaks the 128-bit cipher key into eight
	16-bit words.

	For further information on the IDEA cipher, see these papers:
	1) Xuejia Lai, "Detailed Description and a Software Implementation of
	the IPES Cipher", Institute for Signal and Information
	Processing, ETH-Zentrum, Zurich, Switzerland, 1991
	2) Xuejia Lai, James L. Massey, Sean Murphy, "Markov Ciphers and
	Differential Cryptanalysis", Advances in Cryptology- EUROCRYPT'91

	This code assumes that each pair of 8-bit bytes comprising a 16-bit
	word in the key and in the cipher block are externally represented
	with the Most Significant Byte (MSB) first, regardless of the
	internal native byte order of the target CPU.

	Modified for C++ by Anton Sobolev. (Feb 1998-2018)
*/
#include <slib-internal.h>
#pragma hdrstop
// @v10.9.3 #if defined(TEST) || defined(TEST2)
	// @v10.9.3 #include <process.h>
// @v10.9.3 #endif
#define low16(x) (x)

#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8

#define IDEA_ROUNDS	8            // Don't change this value, should be 8
#define IDEA_KEYLEN	(6*IDEA_ROUNDS+4) // length of key schedule */

typedef uint16 IDEAkey[IDEA_KEYLEN];

class IDEACFB {
public:
	IDEACFB();
	IDEACFB(uint16 iv0[4], uint8 key[16], int decryp);
	~IDEACFB();
	void   init(uint16 iv0[4], uint8 key[16], int decryp);
	void   close();
	void   run(uint8 * buf, int count);
private:
	// Key schedules for IDEA encryption and decryption
	IDEAkey  Z;
	uint16 * iv_idea;     // pointer to IV for CFB or CBC
	int      cfb_dc_idea; // TRUE iff CFB decrypting
};

class IDEARAND {
public:
	IDEARAND();
	IDEARAND(uint8 key[16], uint8 seed[8], uint32 tstamp);
	~IDEARAND();
	void   init(uint8 key[16], uint8 seed[8], uint32 tstamp);
	uint8  rand();
	void   close();
private:
	uint16 dtbuf[4];        // buffer for enciphered timestamp
	uint16 randseed[4];     // seed for IDEA random # generator
	uint16 randbuf[4];      // buffer for IDEA random # generator
	uint8  randbuf_counter; // # of random bytes left in randbuf
	IDEAkey randkey;         // Expanded key for IDEA random # generator
};

static void en_key_idea(uint16 *userkey, uint16 *Z);
static void de_key_idea(IDEAkey Z, IDEAkey DK);
static void cipher_idea(uint16 in[4], uint16 out[4], IDEAkey Z);
//
// Multiplication, modulo (2**16)+1
// Note that this code is structured like this on the assumption that
// untaken branches are cheaper than taken branches, and the compiler
// doesn't schedule branches.
//
#ifdef SMALL_CACHE

const static uint16 mul(uint16 a, uint16 b)
{
	uint32 p;
	if(a)
		if(b) {
			p = (uint32)a * b;
			b = low16(p);
			a = p >> 16;
			return b - a + (b < a);
		}
		else
			return 1-a;
	else
		return 1-b;
}

#endif // SMALL_CACHE
//
// Compute multiplicative inverse of x, modulo (2**16)+1,
// using Euclid's GCD algorithm.  It is unrolled twice to
// avoid swapping the meaning of the registers each iteration,
// and some subtracts of t have been changed to adds.
//
static uint16 FASTCALL inv(uint16 x)
{
	uint16 t0, t1;
	uint16 q, y;
	if(x <= 1)
		return x;	/* 0 and 1 are self-inverse */
	t1 = static_cast<uint16>(0x10001L / x);	/* Since x >= 2, this fits into 16 bits */
	y  = static_cast<uint16>(0x10001L % x);
	if(y == 1)
		return low16(1-t1);
	t0 = 1;
	do {
		q = x / y;
		x = x % y;
		t0 += q * t1;
		if(x == 1)
			return t0;
		q = y / x;
		y = y % x;
		t1 += q * t0;
	} while(y != 1);
	return low16(1-t1);
}
//
// Compute IDEA encryption subkeys Z
//
static void en_key_idea(uint16 *userkey, uint16 *Z)
{
	int i,j;
	// shifts
	for(j = 0; j < 8; j++)
		Z[j] = *userkey++;
	for(i = 0; j < IDEA_KEYLEN; j++) {
		i++;
		Z[i+7] = (Z[i & 7] << 9) | (Z[i+1 & 7] >> 7);
		Z += (i & 8);
		i &= 7;
	}
}
//
// Compute IDEA decryption subkeys DK from encryption subkeys Z
// Note: these buffers *may* overlap!
//
static void de_key_idea(IDEAkey Z, IDEAkey DK)
{
	int    j;
	uint16 t1, t2, t3;
	IDEAkey T;
	uint16 *p = T + IDEA_KEYLEN;

	t1 = inv(*Z++);
	t2 = -*Z++;
	t3 = -*Z++;
	*--p = inv(*Z++);
	*--p = t3;
	*--p = t2;
	*--p = t1;
	for(j = 1; j < IDEA_ROUNDS; j++) {
		t1 = *Z++;
		*--p = *Z++;
		*--p = t1;

		t1 = inv(*Z++);
		t2 = -*Z++;
		t3 = -*Z++;
		*--p = inv(*Z++);
		*--p = t2;
		*--p = t3;
		*--p = t1;
	}
	t1 = *Z++;
	*--p = *Z++;
	*--p = t1;

	t1 = inv(*Z++);
	t2 = -*Z++;
	t3 = -*Z++;
	*--p = inv(*Z++);
	*--p = t3;
	*--p = t2;
	*--p = t1;
	/* Copy and destroy temp copy */
	for(j = 0, p = T; j < IDEA_KEYLEN; j++) {
		*DK++ = *p;
		*p++ = 0;
	}
}
//
// MUL(x,y) computes x = x*y, modulo 0x10001.  Requires two temps,
// t16 and t32.  x must me a side-effect-free lvalue.  y may be
// anything, but unlike x, must be strictly 16 bits even if low16()
// is #defined.
// All of these are equivalent - see which is faster on your machine
//
#ifdef SMALL_CACHE
#define MUL(x,y) (x = mul(low16(x),y))
#else
#ifdef AVOID_JUMPS
#define MUL(x,y) (x = low16(x-1), t16 = low16((y)-1), \
		t32 = (uint32)x*t16+x+t16+1, x = low16(t32), \
		t16 = t32>>16, x = x-t16+(x<t16) )
#else
#define MUL(x,y) ((t16 = (y)) ? (x=low16(x)) ? \
	 t32 = (uint32)x*t16, x = low16(t32), t16 = t32>>16, \
	 x = x-t16+(x<t16) : \
	 (x = 1-t16) : (x = 1-x))
#endif
#endif
//
// IDEA encryption/decryption algorithm
// Note that in and out can be the same buffer
//
#pragma warn -pia

static void cipher_idea(uint16 in[4], uint16 out[4], IDEAkey Z)
{
// Microsoft specific
// warning C4244: '=' : conversion from 'unsigned long' to
// 'unsigned short', possible loss of data
#pragma warning (disable : 4244)
	uint16 x1, x2, x3, x4, s2, s3;
#ifndef SMALL_CACHE
	uint16 t16;
	uint32 t32;
#endif
	int r = IDEA_ROUNDS;
	x1 = *in++;  x2 = *in++;
	x3 = *in++;  x4 = *in;
	do {
		MUL(x1, *Z++);
		x2 += *Z++;
		x3 += *Z++;
		MUL(x4, *Z++);

		s3 = x3;
		x3 ^= x1;
		MUL(x3, *Z++);
		s2 = x2;
		x2 ^= x4;
		x2 += x3;
		MUL(x2, *Z++);
		x3 += x2;

		x1 ^= x2;
		x4 ^= x3;

		x2 ^= s3;
		x3 ^= s2;
	} while(--r);
	MUL(x1, *Z++);
	*out++ = x1;
	*out++ = x3 + *Z++;
	*out++ = x2 + *Z++;
	MUL(x4, *Z);
	*out = x4;
// Microsoft specific
#pragma warning (default : 4244)
}

#pragma warn .pia

/*-------------------------------------------------------------*/

#ifdef TEST
/*
 * This is the number of Kbytes of test data to encrypt.
 * It defaults to 1 MByte.
 */
#ifndef KBYTES
#define KBYTES 1024
#endif
//
// Test driver for IDEA cipher
//
void main(void)
{
	int i, j, k;
	IDEAkey Z, DK;
	uint16 XX[4], TT[4], YY[4];
	uint16 userkey[8];
	clock_t start, end;
	long l;

	// Make a sample user key for testing...
	for(i = 0; i < 8; i++)
		userkey[i] = i+1;
	// Compute encryption subkeys from user key...
	en_key_idea(userkey, Z);
	printf("\nEncryption key subblocks: ");
	for(j = 0; j < ROUNDS+1; j++) {
		printf("\nround %d:   ", j+1);
		if(j == ROUNDS)
			for(i = 0; i < 4; i++)
				printf(" %6u", Z[j*6+i]);
		else
			for(i = 0; i < 6; i++)
				printf(" %6u", Z[j*6+i]);
	}

	/* Compute decryption subkeys from encryption subkeys... */
	de_key_idea(Z, DK);
	printf("\nDecryption key subblocks: ");
	for(j = 0; j < ROUNDS+1; j++) {
		printf("\nround %d:   ", j+1);
		if(j == ROUNDS)
			for(i = 0; i < 4; i++)
				printf(" %6u", DK[j*6+i]);
		else
			for(i = 0; i < 6; i++)
				printf(" %6u", DK[j*6+i]);
	}
	/* Make a sample plaintext pattern for testing... */
	for(k = 0; k < 4; k++)
		XX[k] = 0; // k;
	printf("\n Encrypting %d KBytes (%ld blocks)...", KBYTES, KBYTES*64l);
	fflush(stdout);
	start = clock();
	cipher_idea(XX,YY,Z); /* encrypt plaintext XX, making YY */
	for(l = 1; l < 64L*KBYTES; l++)
		cipher_idea(YY,YY,Z);	/* repeated encryption */
	cipher_idea(YY,TT,DK); /* decrypt ciphertext YY, making TT */
	for(l = 1; l < 64L*KBYTES; l++)
		cipher_idea(TT,TT,DK);	/* repeated decryption */
	end = clock() - start;
	l = end * 1000. / CLOCKS_PER_SEC + 1;
	i = l/1000;
	j = l%1000;
	if(end != 0)
		l = KBYTES * 1024. * CLOCKS_PER_SEC / end;
	printf("%d.%03d seconds = %ld bytes per second\n", i, j, l);
	printf("\nX %6u   %6u  %6u  %6u \n", XX[0], XX[1],  XX[2], XX[3]);
	printf("Y %6u   %6u  %6u  %6u \n",   YY[0], YY[1],  YY[2], YY[3]);
	printf("T %6u   %6u  %6u  %6u \n",   TT[0], TT[1],  TT[2], TT[3]);

	/* Now decrypted TT should be same as original XX */
	for(k = 0; k < 4; k++)
		if(TT[k] != XX[k]) {
			printf("\n\07Error!  Noninvertable encryption.\n");
			exit(-1);	/* error exit */
		}
	printf("\nNormal exit.\n");
	exit(0);
}

#endif // TEST
//
// IDEACFB
//
// xorbuf - change buffer via xor with random mask block
// Used for Cipher Feedback (CFB) or Cipher Block Chaining
// (CBC) modes of encryption.
// Can be applied for any block encryption algorithm,
// with any block size, such as the DES or the IDEA cipher.
//
static void xorbuf(uint8 * buf, uint8 * mask, int count)
// count must be > 0
{
	if(count) do {
		*buf++ ^= *mask++;
	} while(--count);
}
//
// cfbshift - shift bytes into IV for CFB input
// Used only for Cipher Feedback (CFB) mode of encryption.
// Can be applied for any block encryption algorithm with any
// block size, such as the DES or the IDEA cipher.
//
static void cfbshift(uint8 * iv, uint8 * buf, int count, int blocksize)
//
// iv is the initialization vector.
// buf is the buffer pointer.
// count is the number of bytes to shift in...must be > 0.
// blocksize is 8 bytes for DES or IDEA ciphers.
//
{
	int retained;
	if(count) {
		retained = blocksize-count;	// number bytes in iv to retain 
		// left-shift retained bytes of IV over by count bytes to make room 
		while(retained--) {
			*iv = *(iv+count);
			iv++;
		}
		// now copy count bytes from buf to shifted tail of IV 
		do {
			*iv++ = *buf++;
		} while(--count);
	}
}
//
// initkey_idea initializes IDEA for ECB mode operations
//
static void initkey_idea(IDEAkey Z, uint8 key[16], int decryp)
{
	uint16 userkey[8]; // IDEA key is 16 bytes long
	int i;
	// Assume each pair of bytes comprising a word is ordered MSB-first.
	for(i = 0; i < 8; i++) {
		userkey[i] = (key[0] << 8) + key[1];
		key++; key++;
	}
	en_key_idea(userkey, Z);
	if(decryp)
		de_key_idea(Z, Z); // compute inverse key schedule DK
	for(i = 0; i < 8; i++) // Erase dangerous traces
		userkey[i] = 0;
}
//
// Run a 64-bit block thru IDEA in ECB (Electronic Code Book) mode,
// using the currently selected key schedule.
//
static void idea_ecb(IDEAkey Z, uint16 * inbuf, uint16 * outbuf)
{
	/* Assume each pair of bytes comprising a word is ordered MSB-first. */
#ifndef HIGHFIRST	/* If this is a least-significant-byte-first CPU */
	uint16 x;
	size_t i;
	/* Invert the byte order for each 16-bit word for internal use. */
   for(i = 0; i < 4; i++) {
		x = inbuf[i];
	  outbuf[i] = (x >> 8) | (x << 8);
   }
	cipher_idea(outbuf, outbuf, Z);
   for(i = 0; i < 4; i++) {
		x = outbuf[i];
		outbuf[i] = (x >> 8) | (x << 8);
   }
#else	/* HIGHFIRST */
	/* Byte order for internal and external representations is the same. */
	cipher_idea(inbuf, outbuf, Z);
#endif	/* HIGHFIRST */
}

IDEACFB::IDEACFB() : iv_idea(0), cfb_dc_idea(0)
{
}
//
// IDEACFB::IDEACFB [initcfb] - Initializes the IDEA key schedule tables via key,
// and initializes the Cipher Feedback mode IV.
// References context variables cfb_dc_idea and iv_idea.
//
IDEACFB::IDEACFB(uint16 iv0[4], uint8 key[16], int decryp)
//
// iv0 is copied to global iv_idea, buffer will be destroyed by ideacfb.
// key is pointer to key buffer.
// decryp is TRUE if decrypting, FALSE if encrypting.
//
{
	init(iv0, key, decryp);
}

void IDEACFB::init(uint16 iv0[4], uint8 key[16], int decryp)
{
	iv_idea = iv0;
	cfb_dc_idea = decryp;
	initkey_idea(Z, key, FALSE);
}
//
// IDEACFB::run
// [ideacfb] - encipher a buffer with IDEA enciphering algorithm,
// using Cipher Feedback (CFB) mode.
//
// Assumes initcfb_idea has already been called.
// References context variables cfb_dc_idea and iv_idea.
//
void IDEACFB::run(uint8 * buf, int count)
//
// buf is input, output buffer, may be more than 1 block.
// count is byte count of buffer.  May be > IDEABLOCKSIZE.
//
{
	int chunksize;	// smaller of count, IDEABLOCKSIZE
	uint16 temp[IDEABLOCKSIZE/2];
	while((chunksize = MIN(count, IDEABLOCKSIZE)) > 0) {
		idea_ecb(Z, iv_idea, temp); // encrypt iv_idea, making temp
		if(cfb_dc_idea) // buf is ciphertext
			// shift in ciphertext to IV...
			cfbshift((uint8 *)iv_idea, buf, chunksize, IDEABLOCKSIZE);
		// convert buf via xor
		xorbuf(buf, (uint8 *)temp, chunksize); // buf now has enciphered output
		if(!cfb_dc_idea) // buf was plaintext, is now ciphertext
			// shift in ciphertext to IV...
			cfbshift((uint8 *)iv_idea, buf, chunksize, IDEABLOCKSIZE);
		count -= chunksize;
		buf += chunksize;
	}
}

IDEACFB::~IDEACFB() // erase current key schedule tables
{
	close();
}
/*
	IDEACFB::close()
	[close_idea] function erases all the key schedule information when
	we are all done with a set of operations for a particular IDEA key
	context.  This is to prevent any sensitive data from being left
	around in memory.
*/
void IDEACFB::close()
{
	/* @v9.6.3
	for(int i = 0; i < IDEA_KEYLEN; i++)
		Z[i] = 0;
	*/
	memzero(Z, sizeof(Z)); // @v9.6.3
}
//
// IDEARAND
//
IDEARAND::IDEARAND()
{
}

IDEARAND::IDEARAND(uint8 key[16], uint8 seed[8], uint32 tstamp)
{
	init(key, seed, tstamp);
}

IDEARAND::~IDEARAND()
{
	close();
}
//
// IDEARAND::init
// [init_idearand] - initialize idearand, IDEA random number generator.
//   Used for generating cryptographically strong random numbers.
//   Much of the design comes from Appendix C of ANSI X9.17.
//   key is pointer to IDEA key buffer.
//   seed is pointer to random number seed buffer.
//   tstamp is a 32-bit timestamp
//
void IDEARAND::init(uint8 key[16], uint8 seed[8], uint32 tstamp)
{
	int i;
	en_key_idea((uint16 *)key, randkey);
	for(i = 0; i < 4; i++) {       // capture timestamp material
		dtbuf[i] = (uint16)tstamp; // get bottom word
		tstamp = tstamp >> 16;     // drop bottom word
		// tstamp has only 4 bytes-- last 4 bytes will always be 0
	}
	// Start with enciphered timestamp:
	cipher_idea(dtbuf, dtbuf, randkey);
	// initialize seed material
	for(i = 0; i < 8; i++)
		((uint8 *)randseed)[i] = seed[i];
	randbuf_counter = 0; // # of random bytes left in randbuf
}
//
// IDEARAND::rand
// [idearand] - IDEA pseudo-random number generator
//   Used for generating cryptographically strong random numbers.
//   Much of the design comes from Appendix C of ANSI X9.17.
//
uint8 IDEARAND::rand()
{
	int i;
	if(randbuf_counter == 0) { // if random buffer is spent...
		// Combine enciphered timestamp with seed material:
		for(i = 0; i < 4; i++)
			randseed[i] ^= dtbuf[i];
		cipher_idea(randseed, randbuf, randkey); // fill new block
		// Compute new seed vector:
		for(i = 0; i < 4; i++)
			randseed[i] = randbuf[i] ^ dtbuf[i];
		cipher_idea(randseed,randseed,randkey); // fill new seed
		randbuf_counter = 8; // reset counter for full buffer
	}
	// Take a byte from randbuf:
	return (((uint8 *)randbuf)[--randbuf_counter]);
}

void IDEARAND::close()
{
	// Erase random IDEA buffers and wipe out IDEA key info
	memzero(randbuf, sizeof(randbuf));
	memzero(randseed, sizeof(randseed));
	memzero(dtbuf, sizeof(dtbuf));
	memzero(randkey, sizeof(randkey));
}
//
// @todo Класс S_MD5 реализует ошибочный вариант md5. Я его оставил только из-за того, что
// некоторые варианты унаследованного шифрования используют этот вариант хэша.
// Необходимо переработать старые варианты шифрования и избавиться от этого класса!
//
class S_MD5 {
public:
	S_MD5()
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
	~S_MD5()
	{
	}
	// 
	// Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious initialization constants.
	// 
	void Init()
	{
		m_buf[0] = 0x67452301;
		m_buf[1] = 0xefcdab89;
		m_buf[2] = 0x98badcfe;
		m_buf[3] = 0x10325476;
		m_bits[0] = 0;
		m_bits[1] = 0;
	}
	void   Update(const void *data, uint32 len);
	void   Final(uint8 digest[16]);
	/* internal function */
	static void Transform(uint32 buf[4], uint32 in_data[16]);
	static void Calc(const void * data, uint32 length, uint8 digest[16])
	{
		S_MD5 md5;
		md5.Init();
		md5.Update(data, length);
		md5.Final(digest);
	}
private:
	// 
	// Reverse the bytes in 32-bit chunks.
	// 'buf' might not be word-aligned.
	// 
	// @param buf     The byte array to reverse
	// @param n_u32   The number of uint32's in the data
	// 
	void   reverse_u32(uint8 * buf, int n_u32)
	{
		if(SLS.SSys.IsBigEndian) {
			// change { 4, 3, 2, 1 } => { 1, 2, 3, 4 } 
			while(n_u32-- > 0) {
				uint8 tmp    = buf[0];
				buf[0] = buf[3];
				buf[3] = tmp;
				tmp    = buf[1];
				buf[1] = buf[2];
				buf[2] = tmp;
				buf += 4;
			}
		}
		else {
			// change { 4, 3, 2, 1 } => { 3, 4, 1, 2 } 
			while(n_u32-- > 0) {
				uint8 tmp    = buf[0];
				buf[0] = buf[1];
				buf[1] = tmp;
				tmp    = buf[2];
				buf[2] = buf[3];
				buf[3] = tmp;
				buf += 4;
			}
		}
	}
	uint32 m_buf[4];
	uint32 m_bits[2];
	uint8  m_in[64];
	bool   m_need_byteswap;
	//bool   m_big_endian;
	uint8  Reserve[3]; // @alignment
};
// 
// Update context to reflect the concatenation of another buffer full of bytes.
// 
void S_MD5::Update(const void * data, uint32 len)
{
	const uint8 * buf = (const uint8 *)data;
	/* Update bitcount */
	uint32 t = m_bits[0];
	if((m_bits[0] = t+((uint32)len<<3)) < t) {
		m_bits[1]++; /* Carry from low to high */
	}
	m_bits[1] += len>>29;
	t = (t>>3)&0x3f; /* Bytes already in shsInfo->data */
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
// 
// Final wrapup - pad to 64-byte boundary with the bit pattern
// 1 0* (64-bit count of bits processed, MSB-first)
// 
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

// This is the central step in the MD5 algorithm. 
#define MD5STEP(f, w, x, y, z, data, s) (w += f(x, y, z)+data, w = w<<s|w>>(32-s), w += x)
// 
// The core of the MD5 algorithm, this alters an existing MD5 hash to
// reflect the addition of 16 longwords of new data.  MD5::Update blocks
// the data and converts bytes into longwords for this routine.
// 
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
//
// hashpass - Hash pass phrase down to 128 bits (16 bytes).
// keylen must be less than 1024.
// Use the MD5 algorithm.
//
static void hashpass(char * keystring, int keylen, uint8 * hash)
{
	S_MD5::Calc((uchar *)keystring, keylen, hash);
}

static void IdeaCrypt(const char * pPw, void * pBuf, size_t bufLen, int decrypt)
{
	uint16 iv[4];
	uint8  key[16];
	char   temp_pw[128];
	if(pPw == 0) {
		SlExtraProcBlock epb;
		SLS.GetExtraProcBlock(&epb);
		if(epb.F_GetDefaultEncrKey) {
			SString dk;
			epb.F_GetDefaultEncrKey(dk);
			STRNSCPY(temp_pw, dk);
		}
	}
	else
		STRNSCPY(temp_pw, pPw);
	hashpass(temp_pw, strlen(temp_pw), key);
	memzero(iv, sizeof(iv));
	IDEACFB idea(iv, key, decrypt);
	idea.run((uint8 *)pBuf, bufLen);
}

void IdeaEncrypt(const char * pw, void * buf, size_t len)
{
	IdeaCrypt(pw, buf, len, 0);
}

void IdeaDecrypt(const char * pw, void * buf, size_t len)
{
	IdeaCrypt(pw, buf, len, 1);
}

/* @v11.1.1 (replaced with SObfuscateBuffer()) void FASTCALL IdeaRandMem(void * buf, size_t len)
{
	size_t i;
	uint8  key[16];
	uint8  seed[8];
	time_t tm;
	srand((uint)time(&tm));
	for(i = 0; i < sizeof(key); i++)
		key[i] = rand() % 253;
	for(i = 0; i < sizeof(seed); i++)
		seed[i] = rand() % 253;
	time(&tm);
	IDEARAND rand(key, seed, (uint32)tm);
	for(i = 0; i < len; i++)
		((uint8 *)buf)[i] = rand.rand();
}*/

#ifdef TEST2
void main()
{
	FILE * out = fopen("test.", "wb");
	int i;
	uint16 iv[4];
	char  buffer[20];
	char  pw[80];
	uint8 key[16];
	IDEACFB  idea;
	IDEARAND rand;
	strcpy(pw, "PASSWORD");
	hashpass(pw, strlen(pw), key);

	memzero(buffer, sizeof(buffer));
	for(i = 0; i < sizeof(buffer); i++)
		buffer[i] = '0' + i;
	buffer[10/*sizeof(buffer) - 1*/] = '\0';

	fprintf(out, "Original buffer\n");
	fwrite(buffer, sizeof(buffer), 1, out);
	fprintf(out, "\n");

	idea.init((uint16 *)memset(iv, 'z', sizeof(iv)), key, 0);
	idea.run(buffer, sizeof(buffer));
	idea.close();

	fprintf(out, "Encrypted buffer\n");
	fwrite(buffer, sizeof(buffer), 1, out);
	fprintf(out, "\n");

	idea.init((uint16 *)memset(iv, 'z', sizeof(iv)), key, 1);
	idea.run(buffer, sizeof(buffer));
	idea.close();

	fprintf(out, "Decrypted buffer\n");
	fwrite(buffer, sizeof(buffer), 1, out);
	fprintf(out, "\n");

	fclose(out);
	out = fopen("rand.", "wb");
	memzero(key, sizeof(key));
	memzero(buffer, sizeof(buffer));
	//key[0] = 'z';
	//buffer[0] = 'y';
	time_t tm;
	time(&tm);
	rand.init(key, buffer, (uint32)tm);

	long   tab[256];
	memzero(tab, sizeof(tab));
	for(long l = 0; l < 1024L * 1024L; l++) {
		uint8 b = rand.rand();
		tab[b]++;
		fputc(b, out);
	}
	fclose(out);

	out = fopen("tab.", "wt");
	for(i = 0; i < 256; i++)
		fprintf(out, "%2d %6ld\n", i, tab[i]);
	fclose(out);
}
#endif // TEST2
