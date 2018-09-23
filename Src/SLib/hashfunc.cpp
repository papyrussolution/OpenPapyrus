// HASHFUNC.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
// RS Hash Function
// A simple hash function from Robert Sedgwicks Algorithms in C book.
//
uint32 FASTCALL RSHash(const void * pData, size_t len)
{
	uint b = 378551;
	uint a = 63689;
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash = hash * a + PTR8(pData)[i];
		a = a * b;
	}
	return hash;
}
//
// JS Hash Function
// A bitwise hash function written by Justin Sobel
//
uint32 FASTCALL JSHash(const void * pData, size_t len)
{
	uint32 hash = 1315423911;
	for(uint i = 0; i < len; i++) {
		hash ^= ((hash << 5) + PTR8(pData)[i] + (hash >> 2));
	}
	return hash;
}
//
// P.J. Weinberger Hash Function
// This hash algorithm is based on work by Peter J. Weinberger of AT&T Bell Labs.
// The book Compilers (Principles, Techniques and Tools) by Aho, Sethi and Ulman, recommends
// the use of hash functions that employ the hashing methodology found in this particular algorithm.
//
uint32 FASTCALL PJWHash(const void * pData, size_t len)
{
	const uint BitsInUnsignedInt = (uint)(sizeof(uint) * 8);
	const uint ThreeQuarters     = (uint)((BitsInUnsignedInt  * 3) / 4);
	const uint OneEighth         = (uint)(BitsInUnsignedInt / 8);
	const uint HighBits          = (uint)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
	uint32 hash = 0;
	uint32 test = 0;
	for(uint i = 0; i < len; i++) {
		hash = (hash << OneEighth) + PTR8(pData)[i];
		if((test = hash & HighBits)  != 0) {
			hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
		}
	}
	return hash;
}
//
// ELF Hash Function
// Similar to the PJW Hash function, but tweaked for 32-bit processors.
// Its the hash function widely used on most UNIX systems.
//
uint32 FASTCALL ELFHash(const void * pData, size_t len)
{
	uint32 hash = 0;
	uint   x = 0;
	for(uint i = 0; i < len; i++) {
		hash = (hash << 4) + PTR8(pData)[i];
		if((x = hash & 0xF0000000L) != 0) {
			hash ^= (x >> 24);
		}
		hash &= ~x;
	}
	return hash;
}
//
// BKDR Hash Function
// This hash function comes from Brian Kernighan and Dennis Ritchie's book "The C Programming Language".
// It is a simple hash function using a strange set of possible seeds which all constitute
// a pattern of 31....31...31 etc, it seems to be very similar to the DJB hash function.
//
uint32 FASTCALL BKDRHash(const void * pData, size_t len)
{
	uint32 seed = 131; /* 31 131 1313 13131 131313 etc.. */
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash = (hash * seed) + PTR8(pData)[i];
	}
	return hash;
}
//
// SDBM Hash Function
// This is the algorithm of choice which is used in the open source SDBM project.
// The hash function seems to have a good over-all distribution for many different data sets.
// It seems to work well in situations where there is a high variance in the MSBs of the elements in a data set.
//
uint32 FASTCALL SDBMHash(const void * pData, size_t len)
{
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash = PTR8(pData)[i] + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}
//
// DJB Hash Function
// An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the
// usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.
//
uint32 FASTCALL DJBHash(const void * pData, size_t len)
{
	uint32 hash = 5381;
	for(uint i = 0; i < len; i++) {
		hash = ((hash << 5) + hash) + PTR8(pData)[i];
	}
	return hash;
}
//
// DEK Hash Function
// An algorithm proposed by Donald E. Knuth in The Art Of Computer Programming Volume 3,
// under the topic of sorting and search chapter 6.4.
//
uint32 FASTCALL DEKHash(const void * pData, size_t len)
{
	uint32 hash = len;
	for(uint i = 0; i < len; i++) {
		hash = ((hash << 5) ^ (hash >> 27)) ^ PTR8(pData)[i];
	}
	return hash;
}
//
// BP Hash Function
//
uint32 FASTCALL BPHash(const void * pData, size_t len)
{
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash = hash << 7 ^ PTR8(pData)[i];
	}
	return hash;
}
//
// FNV Hash Function
//
uint32 FASTCALL FNVHash(const void * pData, size_t len)
{
	const uint fnv_prime = 0x811C9DC5;
	uint32 hash = 0;
	for(uint i = 0; i < len; i++) {
		hash *= fnv_prime;
		hash ^= PTR8(pData)[i];
	}
	return hash;
}
//
// End Of AP Hash Function
// An algorithm produced by me Arash Partow. I took ideas from all of the above hash functions
// making a hybrid rotative and additive hash function algorithm. There isn't any real mathematical
// analysis explaining why one should use this hash function instead of the others described above
// other than the fact that I tired to resemble the design as close as possible to a simple LFSR.
// An empirical result which demonstrated the distributive abilities of the hash algorithm
// was obtained using a hash-table with 100003 buckets, hashing The Project Gutenberg Etext
// of Webster's Unabridged Dictionary, the longest encountered chain length was 7, the average
// chain length was 2, the number of empty buckets was 4579.
//
// Note: For uses where high throughput is a requirement for computing hashes using the algorithms
// described above, one should consider unrolling the internal loops and adjusting the hash value
// memory foot-print to be appropriate for the targeted architecture(s).
//
uint32 FASTCALL APHash(const void * pData, size_t len)
{
	uint32 hash = 0xAAAAAAAA;
	for(uint i = 0; i < len; i++) {
		hash ^= ((i & 1) == 0) ? ((hash <<  7) ^ PTR8(pData)[i] * (hash >> 3)) : (~((hash << 11) + (PTR8(pData)[i] ^ (hash >> 5))));
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
#define rot(x, k) (((x)<<(k)) | ((x)>>(32-(k))))
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
	a -= c;  a ^= rot(c, 4);  c += b; \
	b -= a;  b ^= rot(a, 6);  a += c; \
	c -= b;  c ^= rot(b, 8);  b += a; \
	a -= c;  a ^= rot(c, 16);  c += b; \
	b -= a;  b ^= rot(a, 19);  a += c; \
	c -= b;  c ^= rot(b, 4);  b += a; \
}
/*
   BobJencHash_Final -- final mixing of 3 32-bit values (a,b,c) into c

   Pairs of (a,b,c) values differing in only a few bits will usually
   produce values of c that look totally different.  This was tested for
 * pairs that differed by one bit, by two bits, in any combination
   of top bits of (a,b,c), or in any combination of bottom bits of
   (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
   the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
   is commonly produced by subtraction) look like a single 1-bit
   difference.
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
	c ^= b; c -= rot(b, 14); \
	a ^= c; a -= rot(c, 11); \
	b ^= a; b -= rot(a, 25); \
	c ^= b; c -= rot(b, 16); \
	a ^= c; a -= rot(c, 4);	 \
	b ^= a; b -= rot(a, 14); \
	c ^= b; c -= rot(b, 24); \
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
static uint32 BobJencHash_Word(const uint32 * k, size_t length, uint32 initval)
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
		case 3:
			c += k[2];
		case 2:
			b += k[1];
		case 1:
			a += k[0];
		    BobJencHash_Final(a, b, c);
		case 0: /* case 0: nothing left to add */
		    break;
	}
	return c;
}
/*
   --------------------------------------------------------------------
   hashword2() -- same as BobJencHash_Word(), but take two seeds and return two
   32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
   both be initialized with seeds.  If you pass in (*pb)==0, the output
   (*pc) will be the same as the return value from BobJencHash_Word().
   --------------------------------------------------------------------
 */
//
// ARG(k      IN) the key, an array of uint32 values
// ARG(length IN) the length of the key, in uint32_ts
// ARG(pc INOUT) IN: seed OUT: primary hash value
// ARG(pb INOUT) IN: more seed OUT: secondary hash value
//
static void BobJencHash_Word2(const uint32 * k, size_t length, uint32 * pc, uint32 * pb)
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
// for(i=0, h=0; i<n; ++i) h = BobJencHash_Little( k[i], len[i], h);
//
// By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
// code any way you wish, private, educational, or commercial.  It's free.
//
// Use for hash table lookup, or anything where one collision in 2^^32 is
// acceptable.  Do NOT use for cryptographic purposes.
//
static uint32 BobJencHash_Little(const void * key, size_t length, uint32 initval)
{
	union {
		const void * ptr;
		size_t i;
	} u; // needed for Mac Powerbook G4
	uint32 a = 0xdeadbeef + ((uint32)length) + initval;
	uint32 b = a;
	uint32 c = a;
	u.ptr = key;
	if(!(SLS.GetSSys().Flags & SSystem::fBigEndian) && ((u.i & 0x3) == 0)) {
		const uint32 * k = (const uint32*)key; // read 32-bit chunks
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
	else if(!(SLS.GetSSys().Flags & SSystem::fBigEndian) && ((u.i & 0x1) == 0)) {
		const uint16 * k = (const uint16 *)key; /* read 16-bit chunks */
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
		k8 = (const uint8*)k;
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
				return c;  /* zero length requires no mixing */
		}
	}
	else { // need to read the key one byte at a time
		const uint8 * k = (const uint8*)key;
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
// a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
//
// ARG(key    IN) the key to hash
// ARG(length IN) length of the key
// ARG(pc INOUT)  IN: primary initval, OUT: primary hash
// ARG(pb INOUT)  IN: secondary initval, OUT: secondary hash
//
static void BobJencHash_Little2(const void * key, size_t length, uint32 * pc, uint32 * pb)
{
	union { // needed for Mac Powerbook G4
		const void * ptr;
		size_t i;
	} u;
	uint32 a = 0xdeadbeef + ((uint32)length) + *pc;
	uint32 b = a;
	uint32 c = a + *pb;
	u.ptr = key;
	if(((u.i & 0x3) == 0) && !(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		const uint32 * k = (const uint32*)key; /* read 32-bit chunks */
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
	else if(((u.i & 0x1) == 0) && !(SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		const uint16 * k = (const uint16*)key; /* read 16-bit chunks */
		const uint8  * k8;

		/*--------------- all but last block: aligned reads and different mixing */
		while(length > 12) {
			a += k[0] + (((uint32)k[1])<<16);
			b += k[2] + (((uint32)k[3])<<16);
			c += k[4] + (((uint32)k[5])<<16);
			BobJencHash_Mix(a, b, c);
			length -= 12;
			k += 6;
		}
		/*----------------------------- handle the last (probably partial) block */
		k8 = (const uint8*)k;
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
		const uint8 * k = (const uint8*)key;
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
	if(((u.i & 0x3) == 0) && (SLS.GetSSys().Flags & SSystem::fBigEndian)) {
		const uint32 * k = (const uint32 *)key; /* read 32-bit chunks */
		/*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
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
		 * "k[2]<<8" actually reads beyond the end of the string, but
		 * then shifts out the part it's not allowed to read.  Because the
		 * string is aligned, the illegal read is in the same word as the
		 * rest of the string.  Every machine with memory protection I've seen
		 * does it on word boundaries, so is OK with this.  But VALGRIND will
		 * still catch it and complain.  The masking trick does make the hash
		 * noticably faster for short strings (like English words).
		 */
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
		const uint8 * k = (const uint8*)key;
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

uint32 FASTCALL BobJencHash(const void * pData, size_t len)
{
	uint32 hash = 0;
	if(!(len & 0x3))
		hash = BobJencHash_Word((uint32 *)pData, (len >> 2), 0xfeedbeef);
	else
		hash = BobJencHash_Little((uint32 *)pData, len, 0xfeedbeef);
	return hash;
}

#if SLTEST_RUNNING // {

#define BOBJEN_HASHSTATE 1
#define BOBJEN_HASHLEN   1
#define BOBJEN_MAXPAIR 60
#define BOBJEN_MAXLEN  70

SLTEST_R(HashFunction)
{
	{
		SString in_file_name = MakeInputFilePath("botan-validate.dat");
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
						SLTEST_CHECK_EQ(val_additive, val);
					}
					THROW(bin_buf.Alloc(NZOR(val_buf.Len(), 1) * 2));
					THROW(val_buf.DecodeHex(1, bin_buf, bin_buf.GetSize(), &real_bin_size));
					THROW(SLTEST_CHECK_EQ(real_bin_size, 4));
					SLTEST_CHECK_EQ(*(const uint32 *)(const char *)bin_buf, val);
				}
			}
		}
	}
	const char * p_key = "abcdefghijklmnopqrstuvwxyz1234567890";
	const size_t key_size = sstrlen(p_key);
	uint32 h;
	//SString msg;
	SLTEST_CHECK_EQ(h = RSHash(p_key, key_size), 4097835502);
	//SetInfo(msg.Z().CatEq("RSHash", h), -1);
	SLTEST_CHECK_EQ(h = JSHash(p_key, key_size), 1651003062);
	//SetInfo(msg.Z().CatEq("JSHash", h), -1);
	SLTEST_CHECK_EQ(h = PJWHash(p_key, key_size), 126631744);
	//SetInfo(msg.Z().CatEq("PJWHash", h), -1);
	SLTEST_CHECK_EQ(h = ELFHash(p_key, key_size), 126631744);
	//SetInfo(msg.Z().CatEq("ELFHash", h), -1);
	SLTEST_CHECK_EQ(h = BKDRHash(p_key, key_size), 3153586616);
	//SetInfo(msg.Z().CatEq("BKDRHash", h), -1);
	SLTEST_CHECK_EQ(h = SDBMHash(p_key, key_size), 3449571336);
	//SetInfo(msg.Z().CatEq("SDBMHash", h), -1);
	SLTEST_CHECK_EQ(h = DJBHash(p_key, key_size), 729241521);
	//SetInfo(msg.Z().CatEq("DJBHash", h), -1);
	SLTEST_CHECK_EQ(h = DEKHash(p_key, key_size), 2923964919);
	//SetInfo(msg.Z().CatEq("DEKHash", h), -1);
	SLTEST_CHECK_EQ(h = BPHash(p_key, key_size), 1726880944);
	//SetInfo(msg.Z().CatEq("BPHash", h), -1);
	SLTEST_CHECK_EQ(h = FNVHash(p_key, key_size), 3243095106);
	//SetInfo(msg.Z().CatEq("FNVHash", h), -1);
	SLTEST_CHECK_EQ(h = APHash(p_key, key_size), 882643939);
	//SetInfo(msg.Z().CatEq("APHash", h), -1);
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
		    BobJencHash_Word((const uint32*)q, (sizeof(q)-1)/4, 13),
		    BobJencHash_Word((const uint32*)q, (sizeof(q)-5)/4, 13),
		    BobJencHash_Word((const uint32*)q, (sizeof(q)-9)/4, 13)));
		p = q;
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    BobJencHash_Little(p, sizeof(q)-1, 13), BobJencHash_Little(p, sizeof(q)-2, 13),
		    BobJencHash_Little(p, sizeof(q)-3, 13), BobJencHash_Little(p, sizeof(q)-4, 13),
		    BobJencHash_Little(p, sizeof(q)-5, 13), BobJencHash_Little(p, sizeof(q)-6, 13),
		    BobJencHash_Little(p, sizeof(q)-7, 13), BobJencHash_Little(p, sizeof(q)-8, 13),
		    BobJencHash_Little(p, sizeof(q)-9, 13), BobJencHash_Little(p, sizeof(q)-10, 13),
		    BobJencHash_Little(p, sizeof(q)-11, 13), BobJencHash_Little(p, sizeof(q)-12, 13)));
		p = &qq[1];
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    BobJencHash_Little(p, sizeof(q)-1, 13), BobJencHash_Little(p, sizeof(q)-2, 13),
		    BobJencHash_Little(p, sizeof(q)-3, 13), BobJencHash_Little(p, sizeof(q)-4, 13),
		    BobJencHash_Little(p, sizeof(q)-5, 13), BobJencHash_Little(p, sizeof(q)-6, 13),
		    BobJencHash_Little(p, sizeof(q)-7, 13), BobJencHash_Little(p, sizeof(q)-8, 13),
		    BobJencHash_Little(p, sizeof(q)-9, 13), BobJencHash_Little(p, sizeof(q)-10, 13),
		    BobJencHash_Little(p, sizeof(q)-11, 13), BobJencHash_Little(p, sizeof(q)-12, 13)));
		p = &qqq[2];
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    BobJencHash_Little(p, sizeof(q)-1, 13), BobJencHash_Little(p, sizeof(q)-2, 13),
		    BobJencHash_Little(p, sizeof(q)-3, 13), BobJencHash_Little(p, sizeof(q)-4, 13),
		    BobJencHash_Little(p, sizeof(q)-5, 13), BobJencHash_Little(p, sizeof(q)-6, 13),
		    BobJencHash_Little(p, sizeof(q)-7, 13), BobJencHash_Little(p, sizeof(q)-8, 13),
		    BobJencHash_Little(p, sizeof(q)-9, 13), BobJencHash_Little(p, sizeof(q)-10, 13),
		    BobJencHash_Little(p, sizeof(q)-11, 13), BobJencHash_Little(p, sizeof(q)-12, 13)));
		p = &qqqq[3];
		SetInfo(out_buf.Printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
		    BobJencHash_Little(p, sizeof(q)-1, 13), BobJencHash_Little(p, sizeof(q)-2, 13),
		    BobJencHash_Little(p, sizeof(q)-3, 13), BobJencHash_Little(p, sizeof(q)-4, 13),
		    BobJencHash_Little(p, sizeof(q)-5, 13), BobJencHash_Little(p, sizeof(q)-6, 13),
		    BobJencHash_Little(p, sizeof(q)-7, 13), BobJencHash_Little(p, sizeof(q)-8, 13),
		    BobJencHash_Little(p, sizeof(q)-9, 13), BobJencHash_Little(p, sizeof(q)-10, 13),
		    BobJencHash_Little(p, sizeof(q)-11, 13), BobJencHash_Little(p, sizeof(q)-12, 13)));
		SetInfo(out_buf.Z().CR());
		//
		// check that BobJencHash_Little2 and BobJencHash_Little produce the same results
		//
		i = 47; j = 0;
		SLTEST_CHECK_EQ((BobJencHash_Little2(q, sizeof(q), &i, &j), i), BobJencHash_Little(q, sizeof(q), 47));
		//
		// check that hashword2 and BobJencHash_Word produce the same results
		//
		len = 0xdeadbeef;
		i = 47, j = 0;
		SLTEST_CHECK_EQ((BobJencHash_Word2(&len, 1, &i, &j), i), BobJencHash_Word(&len, 1, 47));
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
				ref = BobJencHash_Little(b, len, (uint32)1);
				*(b+i) = (uint8)~0;
				*(b-1) = (uint8)~0;
				SLTEST_CHECK_EQ(BobJencHash_Little(b, len, (uint32)1), ref);
				SLTEST_CHECK_EQ(BobJencHash_Little(b, len, (uint32)1), ref);
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
				h = BobJencHash_Little(buf, 0, h);
				hash_list[i] = h;
			}
			//
			// These should all be different
			//
			for(i = 0; i < cnt; i++) {
				for(j = i+1; j < cnt; j++) {
					SLTEST_CHECK_NZ(hash_list[i] - hash_list[j]);
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
			SLTEST_CHECK_EQ(BobJencHash_Little(p_data, len, 0xdeadbeef), BobJencHash_Word((uint32 *)p_data, len / 4, 0xdeadbeef));
		}
		{
			uint32 b = 0;
			uint32 c = 0;
			BobJencHash_Little2("", 0, &c, &b);
			SLTEST_CHECK_EQ(c, 0xdeadbeefUL);
			SLTEST_CHECK_EQ(b, 0xdeadbeefUL);
			b = 0xdeadbeef;
			c = 0;
			BobJencHash_Little2("", 0, &c, &b);
			SLTEST_CHECK_EQ(c, 0xbd5b7ddeUL);
			SLTEST_CHECK_EQ(b, 0xdeadbeefUL);
			b = 0xdeadbeef;
			c = 0xdeadbeef;
			BobJencHash_Little2("", 0, &c, &b);
			SLTEST_CHECK_EQ(c, 0x9c093ccdUL);
			SLTEST_CHECK_EQ(b, 0xbd5b7ddeUL);
			b = 0;
			c = 0;
			BobJencHash_Little2("Four score and seven years ago", 30, &c, &b);
			SLTEST_CHECK_EQ(c, 0x17770551UL);
			SLTEST_CHECK_EQ(b, 0xce7226e6UL);
			b = 1;
			c = 0;
			BobJencHash_Little2("Four score and seven years ago", 30, &c, &b);
			SLTEST_CHECK_EQ(c, 0xe3607caeUL);
			SLTEST_CHECK_EQ(b, 0xbd371de4UL);
			b = 0;
			c = 1;
			BobJencHash_Little2("Four score and seven years ago", 30, &c, &b);
			SLTEST_CHECK_EQ(c, 0xcd628161UL);
			SLTEST_CHECK_EQ(b, 0x6cbea4b3UL);
			SLTEST_CHECK_EQ(BobJencHash_Little("Four score and seven years ago", 30, 0), 0x17770551UL);
			SLTEST_CHECK_EQ(BobJencHash_Little("Four score and seven years ago", 30, 1), 0xcd628161UL);
		}
		return CurrentStatus;
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
						a[i] ^= (k<<j);
						a[i] ^= (k>>(8-j));
						c[0] = BobJencHash_Little(a, hlen, m);
						b[i] ^= ((k+1)<<j);
						b[i] ^= ((k+1)>>(8-j));
						d[0] = BobJencHash_Little(b, hlen, m);
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

#endif // } SLTEST_RUNNING

