/*
 * fnv - Fowler/Noll/Vo- hash code
 *
 * @(#) $Revision: 5.4 $
 * @(#) $Id: fnv.h,v 5.4 2009/07/30 22:49:13 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/fnv.h,v $
 *
 ***
 *
 * Fowler/Noll/Vo- hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 *
 ***
 *
 * NOTE: The FNV-0 historic hash is not recommended.  One should use
 *	 the FNV-1 hash instead.
 *
 * To use the 32 bit FNV-0 historic hash, pass SlConst::FnvHash0Init32 as the
 * uint32 hashval argument to fnv_32_buf() or fnv_32_str().
 *
 * To use the 64 bit FNV-0 historic hash, pass SlConst::FnvHash0Init64 as the
 * uint64 hashval argument to fnv_64_buf() or fnv_64_str().
 *
 * To use the recommended 32 bit FNV-1 hash, pass SlConst::FnvHash1Init32 as the
 * uint32 hashval argument to fnv_32_buf() or fnv_32_str().
 *
 * To use the recommended 64 bit FNV-1 hash, pass SlConst::FnvHash1Init64 as the
 * uint64 hashval argument to fnv_64_buf() or fnv_64_str().
 *
 * To use the recommended 32 bit FNV-1a hash, pass SlConst::FnvHash1Init32 as the
 * uint32 hashval argument to fnv_32a_buf() or fnv_32a_str().
 *
 * To use the recommended 64 bit FNV-1a hash, pass SlConst::FnvHash1Init64 as the
 * uint64 hashval argument to fnv_64a_buf() or fnv_64a_str().
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
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 * Share and Enjoy!	:-)
 */

#if !defined(__FNV_H__)
#define __FNV_H__

#include <sys/types.h>

#define FNV_VERSION "5.0.2"     /* @(#) FNV Version */
/*
 * 32 bit FNV-0 zero initial basis
 * This historic hash is not recommended. One should use the FNV-1 hash and initial basis instead.
 */
// SlConst::FnvHash0Init32
/*
 * 32 bit FNV-1 and FNV-1a non-zero initial basis
 * The FNV-1 initial basis is the FNV-0 hash of the following 32 octets:
 *              chongo <Landon Curt Noll> /\../\
 * NOTE: The \'s above are not back-slashing escape characters.
 * They are literal ASCII  backslash 0x5c characters.
 *
 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
 */
// SlConst::FnvHash1Init32
/*
 * 64 bit FNV-0 hash
 */
/*
 * 64 bit FNV-0 zero initial basis
 * This historic hash is not recommended. One should use the FNV-1 hash and initial basis instead.
 */
// SlConst::FnvHash0Init64
/*
 * 64 bit FNV-1 non-zero initial basis
 * The FNV-1 initial basis is the FNV-0 hash of the following 32 octets:
 *              chongo <Landon Curt Noll> /\../\
 * NOTE: The \'s above are not back-slashing escape characters.
 * They are literal ASCII  backslash 0x5c characters.
 *
 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
 */
extern uint32 fnv_32_buf(const void * buf, size_t len, uint32 hashval);
extern uint32 fnv_32a_buf(const void * buf, size_t len, uint32 hashval);
extern uint64 fnv_64_buf(const void * buf, size_t len, uint64 hashval);
extern uint64 fnv_64a_buf(const void * buf, size_t len, uint64 hashval);

#endif /* __FNV_H__ */
