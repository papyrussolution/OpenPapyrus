// SMEMCHR.CPP
// 
#include <slib-internal.h>
#pragma hdrstop
#include <emmintrin.h>
//
// fast_memchr
//
// Return `true` if `x` contains any zero byte.
//
// From *Matters Computational*, J. Arndt
//
// "The idea is to subtract one from each of the bytes and then look for
// bytes where the borrow propagated all the way to the most significant bit."
//
static FORCEINLINE bool contains_zero_byte(uintptr_t x) { return (((x - ((uintptr_t)0x0101010101010101L)) & ~x & ((uintptr_t)0x8080808080808080L)) != 0); }
//
// Repeat the given byte into a word size number. That is, every 8 bits
// is equivalent to the given byte. For example, if `b` is `\x4E` or
// `01001110` in binary, then the returned value on a 32-bit system would be:
// `01001110_01001110_01001110_01001110`.
//
static FORCEINLINE uintptr_t repeat_byte(uint8 b)  { return(((uintptr_t)b) * (UINTPTR_MAX / 255)); }

static FORCEINLINE uintptr_t read_unaligned_word(const uint8 * ptr) 
{
	uintptr_t ret;
	memcpy(&ret, ptr, sizeof(uintptr_t));
	return ret;
}

static FORCEINLINE void * forward_search(const uint8 * ptr, const uint8 * end_ptr, uint8 n) 
{
	while(ptr < end_ptr) {
		if(*ptr == n) {
			return (void *)ptr;
		}
		ptr++;
	}
	return NULL;
}

const void * fast_memchr(const void * haystack, int n, size_t len) 
{
	constexpr uint _LOOP_SIZE = (2 * sizeof(uintptr_t));
	const   uint8 n1 = (uint8)n;
	uintptr_t vn1 = repeat_byte(n1);
	const   uint32 loop_size = MIN(_LOOP_SIZE, len);
	const uint8 * start_ptr = static_cast<const uint8 *>(haystack);
	const uint8 * end_ptr = start_ptr + len;
	const uint8 * ptr = start_ptr;
	if(len < sizeof(uintptr_t)) {
		return forward_search(ptr, end_ptr, n1);
	}
	else {
		uintptr_t chunk = read_unaligned_word(ptr);
		if(contains_zero_byte(chunk ^ vn1)) {
			return forward_search(ptr, end_ptr, n1);
		}
		else {
			constexpr uintptr_t align = sizeof(uintptr_t) - 1;
			ptr += sizeof(uintptr_t) - (((uintptr_t)start_ptr) & align);
			while(loop_size == _LOOP_SIZE && ptr < (end_ptr - loop_size)) {
				uintptr_t a = read_unaligned_word(ptr);
				uintptr_t b = read_unaligned_word(ptr + sizeof(uintptr_t));
				bool eqa = contains_zero_byte(a ^ vn1);
				bool eqb = contains_zero_byte(b ^ vn1);
				if(eqa || eqb) {
					break;
				}
				ptr += _LOOP_SIZE;
			}
			return forward_search(ptr, end_ptr, n1);
		}
	}
}
//
// fast_memchr_sse2
//
static FORCEINLINE uintptr_t forward_pos(uint32 mask) 
{
#if(_MSC_VER >= 1600) && !defined(LZ4_FORCE_SW_BITCOUNT)
	ulong r;
	_BitScanForward(&r, (uint32)mask);
	return (uintptr_t)(r);
#elif (defined(__clang__) || (defined(__GNUC__) && (__GNUC__>=3))) && !defined(LZ4_FORCE_SW_BITCOUNT)
	return __builtin_ctz(val);
#else
	assert(0);
	return 0;
#endif
}

static FORCEINLINE void * forward_search_sse2(const uint8 * ptr, __m128i vn1) 
{
	__m128i chunk = _mm_loadu_si128((__m128i *)ptr);
	uint32 mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, vn1));
	return (mask != 0) ? (void *)(ptr + forward_pos(mask)) : NULL;
}

const void * /*fast_memchr_sse2*/smemchr(const void * haystack, int n, size_t len) 
{
	constexpr size_t _VECTOR_SIZE  = (sizeof(__m128i));
	constexpr size_t _VECTOR_ALIGN = (_VECTOR_SIZE - 1);
	constexpr size_t _LOOP_SIZE = (4 * _VECTOR_SIZE);
	__m128i vn1 = _mm_set1_epi8((uint8_t)n);
	uint32 loop_size = MIN(_LOOP_SIZE, len);
	const uint8 * start_ptr = static_cast<const uint8 *>(haystack);
	const uint8 * end_ptr = start_ptr + len;
	const uint8 * ptr = start_ptr;
	if(len < _VECTOR_SIZE) {
		while(ptr < end_ptr) {
			if(*ptr == n) {
				return (void *)ptr;
			}
			ptr++;
		}
		return NULL;
	}
	else {
		void * pos = forward_search_sse2(ptr, vn1);
		if(pos) {
			return pos;
		}
		else {
			ptr += _VECTOR_SIZE - (((uintptr_t)start_ptr) & _VECTOR_ALIGN);
			assert(ptr > start_ptr && (end_ptr - _VECTOR_SIZE) >= start_ptr);
			while(loop_size == _LOOP_SIZE && ptr <= (end_ptr - loop_size)) {
				assert((uintptr_t)ptr % _VECTOR_SIZE == 0);
				__m128i a = _mm_load_si128((__m128i *)ptr);
				__m128i b = _mm_load_si128((__m128i *)(ptr + _VECTOR_SIZE));
				__m128i c = _mm_load_si128((__m128i *)(ptr + (2 * _VECTOR_SIZE)));
				__m128i d = _mm_load_si128((__m128i *)(ptr + (3 * _VECTOR_SIZE)));
				__m128i eqa = _mm_cmpeq_epi8(vn1, a);
				__m128i eqb = _mm_cmpeq_epi8(vn1, b);
				__m128i eqc = _mm_cmpeq_epi8(vn1, c);
				__m128i eqd = _mm_cmpeq_epi8(vn1, d);
				__m128i or1 = _mm_or_si128(eqa, eqb);
				__m128i or2 = _mm_or_si128(eqc, eqd);
				__m128i or3 = _mm_or_si128(or1, or2);
				if(_mm_movemask_epi8(or3) != 0) {
					const uint8 * at = ptr;
					uint32 mask = _mm_movemask_epi8(eqa);
					if(mask != 0) {
						return (void *)(at + forward_pos(mask));
					}
					else {
						at += _VECTOR_SIZE;
						mask = _mm_movemask_epi8(eqb);
						if(mask != 0) {
							return (void *)(at + forward_pos(mask));
						}
						else {
							at += _VECTOR_SIZE;
							mask = _mm_movemask_epi8(eqc);
							if(mask != 0) {
								return (void *)(at + forward_pos(mask));
							}
							else {
								at += _VECTOR_SIZE;
								mask = _mm_movemask_epi8(eqd);
								assert(mask != 0);
								return (void *)(at + forward_pos(mask));
							}
						}
					}
				}
				ptr += loop_size;
			}
			while(ptr <= (end_ptr - _VECTOR_SIZE)) {
				assert(end_ptr - ptr >= _VECTOR_SIZE);
				pos = forward_search_sse2(ptr, vn1);
				if(pos) {
					return pos;
				}
				ptr += _VECTOR_SIZE;
			}
			if(ptr < end_ptr) {
				assert(end_ptr - ptr < _VECTOR_SIZE);
				ptr -= _VECTOR_SIZE - (end_ptr - ptr);
				assert(end_ptr - ptr == _VECTOR_SIZE);
				return forward_search_sse2(ptr, vn1);
			}
			else
				return NULL;
		}
	}
}
