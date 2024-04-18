// FNV_HASH.C
//
/*
 * fnv_hash - 32 and 64 bit Fowler/Noll/Vo hash code
 *
 * @(#) $Revision: 5.1 $
 * @(#) $Id: hash_32.c,v 5.1 2009/06/30 09:13:32 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/hash_32.c,v $
 *
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 * In a subsequent ballot round:
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See: http://www.isthe.com/chongo/tech/comp/fnv/index.html
 * for more details as well as other forms of the FNV hash.
 *
 * NOTE: The FNV-0 historic hash is not recommended.  One should use the FNV-1 hash instead.
 *
 * To use the 32 bit FNV-0 historic hash, pass SlConst::FnvHash0Init32 as the
 * uint32 hashval argument to fnv_32_buf() or fnv_32_str().
 *
 * To use the recommended 32 bit FNV-1 hash, pass SlConst::FnvHash1Init32 as the
 * uint32 hashval argument to fnv_32_buf() or fnv_32_str().
 *
 ***
 *
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By: chongo <Landon Curt Noll> /\oo/\ http://www.isthe.com/chongo/
 *
 * Share and Enjoy!	:-)
 */
#include <slib-internal.h>
#pragma hdrstop
//#include "fnv.h"
/*
 * fnv_32_buf - perform a 32 bit Fowler/Noll/Vo hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the 32 bit FNV-0 historic hash, use SlConst::FnvHash0Init32 as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 *
 * NOTE: To use the recommended 32 bit FNV-1 hash, use SlConst::FnvHash1Init32 as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 */
uint32 fnv_32_buf(const void * buf, size_t len, uint32 hval)
{
	const uchar * bp = (const uchar *)buf; // start of buffer
	const uchar * be = bp + len; // beyond end of buffer
	/*
	 * FNV-1 hash each octet in the buffer
	 */
	while(bp < be) {
		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= SlConst::FnvHashPrime32;
#else
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
		/* xor the bottom with the current octet */
		hval ^= (uint32)*bp++;
	}
	return hval; // return our new hash value
}
//
//
//
/*
 * fnv_32a_buf - perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the recommended 32 bit FNV-1a hash, use SlConst::FnvHash1Init32 as the
 * 	 hval arg on the first call to either fnv_32a_buf() or fnv_32a_str().
 */
uint32 fnv_32a_buf(const void * buf, size_t len, uint32 hval)
{
	const uchar * bp = (const uchar *)buf; // start of buffer
	const uchar * be = bp + len; // beyond end of buffer
	/*
	 * FNV-1a hash each octet in the buffer
	 */
	while(bp < be) {
		/* xor the bottom with the current octet */
		hval ^= (uint32)*bp++;
		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= SlConst::FnvHashPrime32;
#else
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
	}
	return hval; // return our new hash value
}
//
//
//
/*
 * fnv_64_buf - perform a 64 bit Fowler/Noll/Vo hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	64 bit hash as a static hash type
 *
 * NOTE: To use the 64 bit FNV-0 historic hash, use SlConst::FnvHash0Init64 as the hval
 *	 argument on the first call to either fnv_64_buf() or fnv_64_str().
 *
 * NOTE: To use the recommended 64 bit FNV-1 hash, use SlConst::FnvHash1Init64 as the hval
 *	 argument on the first call to either fnv_64_buf() or fnv_64_str().
 */
uint64 fnv_64_buf(const void * buf, size_t len, uint64 hval)
{
	const uchar * bp = (const uchar *)buf; // start of buffer
	const uchar * be = bp + len; // beyond end of buffer
	/*
	 * FNV-1 hash each octet of the buffer
	 */
	while(bp < be) {
		/* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= SlConst::FnvHashPrime64;
#else /* NO_FNV_GCC_OPTIMIZATION */
		hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */
		/* xor the bottom with the current octet */
		hval ^= (uint64)*bp++;
	}
	return hval; // return our new hash value
}
//
//
//
/*
 * fnv_64a_buf - perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	64 bit hash as a static hash type
 *
 * NOTE: To use the recommended 64 bit FNV-1a hash, use SlConst::FnvHash1Init64 as the
 * 	 hval arg on the first call to either fnv_64a_buf() or fnv_64a_str().
 */
uint64 fnv_64a_buf(const void * buf, size_t len, uint64 hval)
{
	const uchar * bp = (const uchar *)buf; // start of buffer
	const uchar * be = bp + len; // beyond end of buffer
	//
	// FNV-1a hash each octet of the buffer
	//
	while(bp < be) {
		hval ^= (uint64)*bp++; // xor the bottom with the current octet
		// multiply by the 64 bit FNV magic prime mod 2^64
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= SlConst::FnvHashPrime64;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
#endif
	}
	return hval; // return our new hash value
}
