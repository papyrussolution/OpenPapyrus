/// \file       common.h
/// \brief      Definitions common to the whole liblzma library
//  Author:     Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#ifndef LZMA_COMMON_H
#define LZMA_COMMON_H

#include <slib.h>

// @sobolev {
#define HAVE_ENCODER_LZMA1
#define HAVE_ENCODER_LZMA2
#define HAVE_ENCODER_X86
#define HAVE_ENCODER_IA64
#define HAVE_ENCODER_DELTA
#define HAVE_DECODER_LZMA1
#define HAVE_DECODER_LZMA2
#define HAVE_DECODER_X86
#define HAVE_DECODER_IA64
#define HAVE_DECODER_DELTA 
#define HAVE_CHECK_CRC32
#define HAVE_CHECK_CRC64
#define HAVE_CHECK_SHA256
#define HAVE_STDBOOL_H
#if _MSC_VER < 1600
	#define MYTHREAD_WIN95
#else
	#define MYTHREAD_VISTA
#endif
// } @sobolev

#include <stdint.h>
#include "sysdefs.h"
#include "mythread.h"
#include "tuklib_integer.h"

#if defined(_WIN32) || defined(__CYGWIN__)
	#ifdef DLL_EXPORT
		#define LZMA_API_EXPORT __declspec(dllexport)
	#else
		#define LZMA_API_EXPORT
	#endif
// Don't use ifdef or defined() below.
#elif HAVE_VISIBILITY
	#define LZMA_API_EXPORT __attribute__((__visibility__("default")))
#else
	#define LZMA_API_EXPORT
#endif

#define LZMA_API(type) LZMA_API_EXPORT type LZMA_API_CALL

#include "lzma.h"

// These allow helping the compiler in some often-executed branches, whose result is almost always the same.
/* @sobolev #ifdef __GNUC__
	#define likely_Removed(expr)   __builtin_expect(expr, true)
	#define unlikely_Removed(expr) __builtin_expect(expr, false)
#else
	#define likely_Removed(expr)   (expr)
	#define unlikely_Removed(expr) (expr)
#endif*/
#define LZMA_BUFFER_SIZE 4096 /// Size of temporary buffers needed in some filters
/// Maximum number of worker threads within one multithreaded component.
/// The limit exists solely to make it simpler to prevent integer overflows
/// when allocating structures etc. This should be big enough for now...
/// the code won't scale anywhere close to this number anyway.
#define LZMA_THREADS_MAX 16384
/// Starting value for memory usage estimates. Instead of calculating size
/// of _every_ structure and taking into account malloc() overhead etc., we
/// add a base size to all memory usage estimates. It's not very accurate
/// but should be easily good enough.
#define LZMA_MEMUSAGE_BASE (UINT64_C(1) << 15)
/// Start of internal Filter ID space. These IDs must never be used in Streams.
#define LZMA_FILTER_RESERVED_START (LZMA_VLI_C(1) << 62)


/// Supported flags that can be passed to lzma_stream_decoder() or lzma_auto_decoder().
#define LZMA_SUPPORTED_FLAGS (LZMA_TELL_NO_CHECK | LZMA_TELL_UNSUPPORTED_CHECK | LZMA_TELL_ANY_CHECK | LZMA_IGNORE_CHECK | LZMA_CONCATENATED)
/// Largest valid lzma_action value as unsigned integer.
#define LZMA_ACTION_MAX ((unsigned int)(LZMA_FULL_BARRIER))
/// Special return value (lzma_ret) to indicate that a timeout was reached
/// and lzma_code() must not return LZMA_BUF_ERROR. This is converted to
/// LZMA_OK in lzma_code().
#define LZMA_TIMED_OUT LZMA_RET_INTERNAL1

/// Type of a function to do some kind of coding work (filters, Stream,
/// Block encoders/decoders etc.). Some special coders use don't use both
/// input and output buffers, but for simplicity they still use this same
/// function prototype.
typedef lzma_ret (*lzma_code_function)(void *coder, const lzma_allocator *allocator,
	const uint8_t * in, size_t * in_pos, size_t in_size, uint8_t * out, size_t * out_pos, size_t out_size, lzma_action action);
/// Type of a function to free the memory allocated for the coder
typedef void (*lzma_end_function)(void *coder, const lzma_allocator *allocator);

//typedef struct lzma_next_coder_s lzma_next_coder;
//typedef struct lzma_filter_info_s lzma_filter_info;

/// Hold data and function pointers of the next filter in the chain.
struct lzma_next_coder {
	lzma_next_coder() : coder(0), init(0), id(LZMA_VLI_UNKNOWN), code(0), end(0), get_progress(0), get_check(0), memconfig(0), update(0)
	{
	}
	void   SetDefault()
	{
		/*#define LZMA_NEXT_CODER_INIT \
			(lzma_next_coder){ \
				.coder = NULL, \
				.init = (uintptr_t)(NULL), \
				.id = LZMA_VLI_UNKNOWN, \
				.code = NULL, \
				.end = NULL, \
				.get_progress = NULL, \
				.get_check = NULL, \
				.memconfig = NULL, \
				.update = NULL, \
			}*/
		coder = 0;
		init = 0;
		id = LZMA_VLI_UNKNOWN;
		code = 0;
		end = 0;
		get_progress = 0;
		get_check = 0;
		memconfig = 0;
		update = 0;
	}
	void * coder; /// Pointer to coder-specific data
	lzma_vli id; /// Filter ID. This is LZMA_VLI_UNKNOWN when this structure doesn't point to a filter coder.
	/// "Pointer" to init function. This is never called here.
	/// We need only to detect if we are initializing a coder
	/// that was allocated earlier. See lzma_next_coder_init and
	/// lzma_next_strm_init macros in this file.
	uintptr_t init;
	lzma_code_function code; /// Pointer to function to do the actual coding
	/// Pointer to function to free lzma_next_coder.coder. This can
	/// be NULL; in that case, lzma_free is called to free
	/// lzma_next_coder.coder.
	lzma_end_function end;
	/// Pointer to a function to get progress information. If this is NULL,
	/// lzma_stream.total_in and .total_out are used instead.
	void (*get_progress)(void *coder, uint64_t *progress_in, uint64_t *progress_out);
	/// Pointer to function to return the type of the integrity check.
	/// Most coders won't support this.
	lzma_check (*get_check)(const void *coder);
	/// Pointer to function to get and/or change the memory usage limit.
	/// If new_memlimit == 0, the limit is not changed.
	lzma_ret (*memconfig)(void *coder, uint64_t *memusage, uint64_t *old_memlimit, uint64_t new_memlimit);
	/// Update the filter-specific options or the whole filter chain in the encoder.
	lzma_ret (*update)(void *coder, const lzma_allocator *allocator, const lzma_filter *filters, const lzma_filter *reversed_filters);
};

struct lzma_filter_info;
/// Type of a function used to initialize a filter encoder or decoder
typedef lzma_ret (*lzma_init_function)(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);

/// Raw coder validates and converts an array of lzma_filter structures to
/// an array of lzma_filter_info structures. This array is used with
/// lzma_next_filter_init to initialize the filter chain.
struct lzma_filter_info {
	lzma_vli id; /// Filter ID. This is used only by the encoder with lzma_filters_update().
	lzma_init_function init; /// Pointer to function used to initialize the filter. This is NULL to indicate end of array.
	void *options; /// Pointer to filter's options structure
};

/// Macro to initialize lzma_next_coder structure
#define LZMA_NEXT_CODER_INIT \
	(lzma_next_coder){ .coder = NULL, .init = (uintptr_t)(NULL), .id = LZMA_VLI_UNKNOWN, .code = NULL, \
		.end = NULL, .get_progress = NULL, .get_check = NULL, .memconfig = NULL, .update = NULL, }


/// Internal data for lzma_strm_init, lzma_code, and lzma_end. A pointer to
/// this is stored in lzma_stream.
struct lzma_internal_s {
	lzma_next_coder next; /// The actual coder that should do something useful
	/// Track the state of the coder. This is used to validate arguments
	/// so that the actual coders can rely on e.g. that LZMA_SYNC_FLUSH
	/// is used on every call to lzma_code until next.code has returned LZMA_STREAM_END.
	enum {
		ISEQ_RUN,
		ISEQ_SYNC_FLUSH,
		ISEQ_FULL_FLUSH,
		ISEQ_FINISH,
		ISEQ_FULL_BARRIER,
		ISEQ_END,
		ISEQ_ERROR,
	} sequence;
	/// A copy of lzma_stream avail_in. This is used to verify that the
	/// amount of input doesn't change once e.g. LZMA_FINISH has been used.
	size_t avail_in;
	bool supported_actions[LZMA_ACTION_MAX + 1]; /// Indicates which lzma_action values are allowed by next.code.
	/// If true, lzma_code will return LZMA_BUF_ERROR if no progress was
	/// made (no input consumed and no output produced by next.code).
	bool allow_buf_error;
};

/// Allocates memory
extern void *lzma_alloc(size_t size, const lzma_allocator *allocator) lzma_attribute((__malloc__)) lzma_attr_alloc_size(1);

/// Allocates memory and zeroes it (like calloc()). This can be faster
/// than lzma_alloc() + memzero() while being backward compatible with
/// custom allocators.
extern void * lzma_attribute((__malloc__)) lzma_attr_alloc_size(1) lzma_alloc_zero(size_t size, const lzma_allocator *allocator);
/// Frees memory
extern void lzma_free(void *ptr, const lzma_allocator *allocator);
/// Allocates strm->internal if it is NULL, and initializes *strm and
/// strm->internal. This function is only called via lzma_next_strm_init macro.
extern lzma_ret lzma_strm_init(lzma_stream *strm);

/// Initializes the next filter in the chain, if any. This takes care of
/// freeing the memory of previously initialized filter if it is different
/// than the filter being initialized now. This way the actual filter
/// initialization functions don't need to use lzma_next_coder_init macro.
extern lzma_ret lzma_next_filter_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);

/// Update the next filter in the chain, if any. This checks that
/// the application is not trying to change the Filter IDs.
extern lzma_ret lzma_next_filter_update(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter *reversed_filters);

/// Frees the memory allocated for next->coder either using next->end or,
/// if next->end is NULL, using lzma_free.
extern void lzma_next_end(lzma_next_coder *next, const lzma_allocator *allocator);

/// Copy as much data as possible from in[] to out[] and update *in_pos
/// and *out_pos accordingly. Returns the number of bytes copied.
extern size_t lzma_bufcpy(const uint8_t * in, size_t * in_pos, size_t in_size, uint8_t * out, size_t * out_pos, size_t out_size);

/// \brief      Return if expression doesn't evaluate to LZMA_OK
///
/// There are several situations where we want to return immediately
/// with the value of expr if it isn't LZMA_OK. This macro shortens
/// the code a little.
#define return_if_error(expr) \
do { \
	const lzma_ret ret_ = (expr); \
	if(ret_ != LZMA_OK) \
		return ret_; \
} while (0)


/// If next isn't already initialized, free the previous coder. Then mark
/// that next is _possibly_ initialized for the coder using this macro.
/// "Possibly" means that if e.g. allocation of next->coder fails, the
/// structure isn't actually initialized for this coder, but leaving
/// next->init to func is still OK.
#define lzma_next_coder_init(func, next, allocator) \
do { \
	if((uintptr_t)(func) != (next)->init) \
		lzma_next_end(next, allocator); \
	(next)->init = (uintptr_t)(func); \
} while (0)


/// Initializes lzma_strm and calls func() to initialize strm->internal->next.
/// (The function being called will use lzma_next_coder_init()). If
/// initialization fails, memory that wasn't freed by func() is freed
/// along strm->internal.
#define lzma_next_strm_init(func, strm, arg1) \
do { \
	return_if_error(lzma_strm_init(strm)); \
	const lzma_ret ret_ = func(&(strm)->internal->next, (strm)->allocator, arg1); \
	if(ret_ != LZMA_OK) { \
		lzma_end(strm); \
		return ret_; \
	} \
} while (0)

#define lzma_next_strm_init2(func, strm, arg1, arg2) \
do { \
	return_if_error(lzma_strm_init(strm)); \
	const lzma_ret ret_ = func(&(strm)->internal->next, (strm)->allocator, arg1, arg2); \
	if(ret_ != LZMA_OK) { \
		lzma_end(strm); \
		return ret_; \
	} \
} while (0)

#define lzma_next_strm_init3(func, strm, arg1, arg2, arg3) \
do { \
	return_if_error(lzma_strm_init(strm)); \
	const lzma_ret ret_ = func(&(strm)->internal->next, (strm)->allocator, arg1, arg2, arg3); \
	if(ret_ != LZMA_OK) { \
		lzma_end(strm); \
		return ret_; \
	} \
} while (0)

#define lzma_next_strm_init4(func, strm, arg1, arg2, arg3, arg4) \
do { \
	return_if_error(lzma_strm_init(strm)); \
	const lzma_ret ret_ = func(&(strm)->internal->next, (strm)->allocator, arg1, arg2, arg3, arg4); \
	if(ret_ != LZMA_OK) { \
		lzma_end(strm); \
		return ret_; \
	} \
} while (0)
//
#include "check-internal.h"
//#include "crc_macros.h"
// Some endian-dependent macros for CRC32 and CRC64
//
#ifdef WORDS_BIGENDIAN
	#define CRC_A(x) ((x) >> 24)
	#define CRC_B(x) (((x) >> 16) & 0xFF)
	#define CRC_C(x) (((x) >> 8) & 0xFF)
	#define CRC_D(x) ((x) & 0xFF)
	#define CRC_S8(x) ((x) << 8)
	#define CRC_S32(x) ((x) << 32)
#else
	#define CRC_A(x) ((x) & 0xFF)
	#define CRC_B(x) (((x) >> 8) & 0xFF)
	#define CRC_C(x) (((x) >> 16) & 0xFF)
	#define CRC_D(x) ((x) >> 24)
	#define CRC_S8(x) ((x) >> 8)
	#define CRC_S32(x) ((x) >> 32)
#endif
//
#include "index-internal.h"
//#include "range_common.h"
// Common things for range encoder and decoder
//
// Constants //
//
#define RC_SHIFT_BITS 8
#define RC_TOP_BITS 24
#define RC_TOP_VALUE (UINT32_C(1) << RC_TOP_BITS)
#define RC_BIT_MODEL_TOTAL_BITS 11
#define RC_BIT_MODEL_TOTAL (UINT32_C(1) << RC_BIT_MODEL_TOTAL_BITS)
#define RC_MOVE_BITS 5
//
// Macros //
//
// Resets the probability so that both 0 and 1 have probability of 50 %
#define bit_reset(prob) prob = RC_BIT_MODEL_TOTAL >> 1

// This does the same for a complete bit tree.
// (A tree represented as an array.)
#define bittree_reset(probs, bit_levels) \
	for(uint32_t bt_i = 0; bt_i < (1 << (bit_levels)); ++bt_i) \
		bit_reset((probs)[bt_i])
//
// Type definitions //
//
/// \brief      Type of probabilities used with range coder
///
/// This needs to be at least 12-bit integer, so uint16_t is a logical choice.
/// However, on some architecture and compiler combinations, a bigger type
/// may give better speed, because the probability variables are accessed
/// a lot. On the other hand, bigger probability type increases cache
/// footprint, since there are 2 to 14 thousand probability variables in
/// LZMA (assuming the limit of lc + lp <= 4; with lc + lp <= 12 there
/// would be about 1.5 million variables).
///
/// With malicious files, the initialization speed of the LZMA decoder can
/// become important. In that case, smaller probability variables mean that
/// there is less bytes to write to RAM, which makes initialization faster.
/// With big probability type, the initialization can become so slow that it
/// can be a problem e.g. for email servers doing virus scanning.
///
/// I will be sticking to uint16_t unless some specific architectures
/// are *much* faster (20-50 %) with uint32_t.
typedef uint16_t probability;
//
#include "lz-internal.h"
//#include "easy_preset.h"
// Preset handling for easy encoder and decoder
struct lzma_options_easy {
	lzma_filter filters[LZMA_FILTERS_MAX + 1]; /// We need to keep the filters array available in case LZMA_FULL_FLUSH is used.
	lzma_options_lzma opt_lzma; /// Options for LZMA2
	// Options for more filters can be added later, so this struct is not ready to be put into the public API.
};

/// Set *easy to the settings given by the preset. Returns true on error, false on success.
extern bool lzma_easy_preset(lzma_options_easy *easy, uint32_t preset);
//
#include "lzma2-internal.h"
#include "lzma_common.h"
//#include "stream_decoder.h"
// Decodes .xz Streams
extern lzma_ret lzma_stream_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, uint64_t memlimit, uint32_t flags);
//
//#include "alone_decoder.h"
// Decoder for LZMA_Alone files
extern lzma_ret lzma_alone_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, uint64_t memlimit, bool picky);
//
//#include "block_buffer_encoder.h"
// Single-call .xz Block encoder
//
/// uint64_t version of lzma_block_buffer_bound(). It is used by
/// stream_encoder_mt.c. Probably the original lzma_block_buffer_bound()
/// should have been 64-bit, but fixing it would break the ABI.
extern uint64_t lzma_block_buffer_bound64(uint64_t uncompressed_size);
//
//#include "block_encoder.h"
/// \brief      Biggest Compressed Size value that the Block encoder supports
///
/// The maximum size of a single Block is limited by the maximum size of
/// a Stream, which in theory is 2^63 - 3 bytes (i.e. LZMA_VLI_MAX - 3).
/// While the size is really big and no one should hit it in practice, we
/// take it into account in some places anyway to catch some errors e.g. if
/// application passes insanely big value to some function.
///
/// We could take into account the headers etc. to determine the exact
/// maximum size of the Compressed Data field, but the complexity would give
/// us nothing useful. Instead, limit the size of Compressed Data so that
/// even with biggest possible Block Header and Check fields the total
/// encoded size of the Block stays as a valid VLI. This doesn't guarantee
/// that the size of the Stream doesn't grow too big, but that problem is
/// taken care outside the Block handling code.
///
/// ~LZMA_VLI_C(3) is to guarantee that if we need padding at the end of
/// the Compressed Data field, it will still stay in the proper limit.
///
/// This constant is in this file because it is needed in both
/// block_encoder.c and block_buffer_encoder.c.
#define COMPRESSED_SIZE_MAX ((LZMA_VLI_MAX - LZMA_BLOCK_HEADER_SIZE_MAX - LZMA_CHECK_SIZE_MAX) & ~LZMA_VLI_C(3))

extern lzma_ret lzma_block_encoder_init(lzma_next_coder *next, const lzma_allocator *allocator, lzma_block *block);
//
//#include "block_decoder.h"
// Decodes .xz Blocks
extern lzma_ret lzma_block_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, lzma_block *block);
//
//#include "filter_common.h"
// Filter-specific stuff common for both encoder and decoder
/// Both lzma_filter_encoder and lzma_filter_decoder begin with these members.
struct lzma_filter_coder {
	lzma_vli id; /// Filter ID
	lzma_init_function init; /// Initializes the filter encoder and calls lzma_next_filter_init() for filters + 1.
	uint64_t (*memusage)(const void *options); /// Calculates memory usage of the encoder. If the options are invalid, UINT64_MAX is returned.
};

typedef const lzma_filter_coder *(*lzma_filter_find)(lzma_vli id);
extern lzma_ret lzma_raw_coder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter *filters, lzma_filter_find coder_find, bool is_encoder);
extern uint64_t lzma_raw_coder_memusage(lzma_filter_find coder_find, const lzma_filter *filters);
// FIXME: Might become a part of the public API.
extern uint64_t lzma_mt_block_size(const lzma_filter *filters);
extern lzma_ret lzma_raw_encoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter *filters);
extern lzma_ret lzma_raw_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter *options);
//
//#include "simple_coder.h"
// Wrapper for simple filters
extern lzma_ret lzma_simple_x86_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_x86_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_powerpc_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_powerpc_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_ia64_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_ia64_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_arm_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_arm_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_armthumb_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_armthumb_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_sparc_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_sparc_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters);
extern lzma_ret lzma_simple_props_size(uint32_t *size, const void *options);
extern lzma_ret lzma_simple_props_encode(const void *options, uint8_t *out);
extern lzma_ret lzma_simple_props_decode(void **options, const lzma_allocator *allocator, const uint8_t *props, size_t props_size);
//
//#include "delta_common.h"
// Common stuff for Delta encoder and decoder
//
struct lzma_delta_coder {
	lzma_next_coder next; /// Next coder in the chain
	size_t distance; /// Delta distance
	uint8_t pos; /// Position in history[]
	uint8_t history[LZMA_DELTA_DIST_MAX]; /// Buffer to hold history of the original data
};

extern lzma_ret lzma_delta_coder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern uint64_t lzma_delta_coder_memusage(const void *options);
extern lzma_ret lzma_delta_encoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern lzma_ret lzma_delta_props_encode(const void *options, uint8_t *out);
extern lzma_ret lzma_delta_decoder_init(lzma_next_coder *next, const lzma_allocator *allocator, const lzma_filter_info *filters);
extern lzma_ret lzma_delta_props_decode(void **options, const lzma_allocator *allocator, const uint8_t *props, size_t props_size);
//
#include "outqueue.h"
//#include "stream_flags_common.h"
// Common stuff for Stream flags coders
#define LZMA_STREAM_FLAGS_SIZE 2 // Size of the Stream Flags field

extern const uint8_t lzma_header_magic[6];
extern const uint8_t lzma_footer_magic[2];

static inline bool is_backward_size_valid(const lzma_stream_flags * options)
{
	return options->backward_size >= LZMA_BACKWARD_SIZE_MIN && options->backward_size <= LZMA_BACKWARD_SIZE_MAX && (options->backward_size & 3) == 0;
}
//
#include "fastpos.h"
//#include "range_encoder.h"
//#include "range_decoder.h"
#include "memcmplen.h"

#endif
