//
// IDEA.H
//
#ifndef __IDEA_H
#define __IDEA_H

#ifndef __SLIB_H
	#include <slib.h>
#endif

// Определен в md5.h
//class MD5 {
//public:
//	SLAPI  MD5() { init(); }
//	void   SLAPI init();
//	void   SLAPI update(uchar * bug, uint len);
//	void   SLAPI final(uchar digest[16]);
//private:
//	uint32 buf[4]; // scratch buffer
//	uint32 i[2];   // number of _bits_ handled mod 2^64
//	uchar  in[64]; // input buffer
//};


//#ifndef MD5_H_INCLUDED
//#define MD5_H_INCLUDED

//#include "base_types.h"

class MD5
{
public:
   MD5();
   ~MD5() { }

   void Init();
   void Update(const void *data, UINT32 len);

   void Final(UINT8 digest[16]);

   /* internal function */
   static void Transform(UINT32 buf[4], UINT32 in_data[16]);

   static void Calc(const void *data, UINT32 length, UINT8 digest[16]);

private:
   UINT32 m_buf[4];
   UINT32 m_bits[2];
   UINT8  m_in[64];
   bool   m_need_byteswap;
   bool   m_big_endian;

   void reverse_u32(UINT8 *buf, int n_u32);
};

//#endif /* MD5_H_INCLUDED */

void SLAPI Transform(uint32 *buf, uint32 *in);

#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8

// hashpass - Hash pass phrase down to 128 bits (16 bytes).
void SLAPI hashpass(char *keystring, int keylen, uint8 *hash);

#define ROUNDS	8            // Don't change this value, should be 8
#define KEYLEN	(6*ROUNDS+4) // length of key schedule */

typedef uint16 IDEAkey[KEYLEN];

class IDEACFB {
public:
	SLAPI  IDEACFB();
	SLAPI  IDEACFB(uint16 iv0[4], uint8 key[16], int decryp);
	SLAPI ~IDEACFB();
	void   SLAPI init(uint16 iv0[4], uint8 key[16], int decryp);
	void   SLAPI close();
	void   SLAPI run(uint8 * buf, int count);
private:
	// Key schedules for IDEA encryption and decryption
	IDEAkey  Z;
	uint16 * iv_idea;     // pointer to IV for CFB or CBC
	int      cfb_dc_idea; // TRUE iff CFB decrypting
};

class IDEARAND {
public:
	SLAPI  IDEARAND();
	SLAPI  IDEARAND(uint8 key[16], uint8 seed[8], uint32 tstamp);
	SLAPI ~IDEARAND();
	void   SLAPI init(uint8 key[16], uint8 seed[8], uint32 tstamp);
	uint8  SLAPI rand();
	void   SLAPI close();
private:
	uint16 dtbuf[4];        // buffer for enciphered timestamp
	uint16 randseed[4];     // seed for IDEA random # generator
	uint16 randbuf[4];      // buffer for IDEA random # generator
	uint8  randbuf_counter; // # of random bytes left in randbuf
	IDEAkey randkey;         // Expanded key for IDEA random # generator
};

void SLAPI IdeaEncrypt(const char * password, void * buf, size_t len);
void SLAPI IdeaDecrypt(const char * password, void * buf, size_t len);
void SLAPI IdeaRandMem(void * buf, size_t len);

#endif // __IDEA_H
