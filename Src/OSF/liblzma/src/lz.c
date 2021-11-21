// LZ.C
// Authors: Igor Pavlov, Lasse Collin
// This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
//#include "memcmplen.h"
// See lz_encoder_hash.h. This is a bit hackish but avoids making endianness a conditional in makefiles.
#if defined(WORDS_BIGENDIAN) && !defined(HAVE_SMALL)
	#include "lz_encoder_hash_table.h"
#endif

struct lzma_encoder_coder {
	lzma_lz_encoder lz; /// LZ-based encoder e.g. LZMA
	lzma_mf mf; /// History buffer and match finder
	lzma_next_coder next; /// Next coder in the chain
};

struct lzma_decoder_coder {
	lzma_dict dict; /// Dictionary (history buffer)
	lzma_lz_decoder lz; /// The actual LZ-based decoder e.g. LZMA
		/// Next filter in the chain, if any. Note that LZMA and LZMA2 are
		/// only allowed as the last filter, but the long-range filter in
		/// future can be in the middle of the chain.
	lzma_next_coder next;
	bool next_finished; /// True if the next filter in the chain has returned LZMA_STREAM_END.
		/// True if the LZ decoder (e.g. LZMA) has detected end of payload
		/// marker. This may become true before next_finished becomes true.
	bool this_finished;
	/// Temporary buffer needed when the LZ-based filter is not the last
	/// filter in the chain. The output of the next filter is first
	/// decoded into buffer[], which is then used as input for the actual
	/// LZ-based decoder.
	struct {
		size_t pos;
		size_t size;
		uint8_t buffer[LZMA_BUFFER_SIZE];
	} temp;
};
//
// lz_encoder
// LZ in window
//
/// \brief      Moves the data in the input window to free space for new data
///
/// mf->buffer is a sliding input window, which keeps mf->keep_size_before
/// bytes of input history available all the time. Now and then we need to
/// "slide" the buffer to make space for the new data to the end of the
/// buffer. At the same time, data older than keep_size_before is dropped.
///
static void move_window(lzma_mf * mf)
{
	// Align the move to a multiple of 16 bytes. Some LZ-based encoders
	// like LZMA use the lowest bits of mf->read_pos to know the
	// alignment of the uncompressed data. We also get better speed
	// for memmove() with aligned buffers.
	assert(mf->read_pos > mf->keep_size_before);
	const uint32 move_offset = (mf->read_pos - mf->keep_size_before) & ~UINT32_C(15);
	assert(mf->write_pos > move_offset);
	const size_t move_size = mf->write_pos - move_offset;
	assert(move_offset + move_size <= mf->size);
	memmove(mf->buffer, mf->buffer + move_offset, move_size);
	mf->offset += move_offset;
	mf->read_pos -= move_offset;
	mf->read_limit -= move_offset;
	mf->write_pos -= move_offset;
}

/// \brief      Tries to fill the input window (mf->buffer)
///
/// If we are the last encoder in the chain, our input data is in in[].
/// Otherwise we call the next filter in the chain to process in[] and
/// write its output to mf->buffer.
///
/// This function must not be called once it has returned LZMA_STREAM_END.
///
static lzma_ret fill_window(lzma_encoder_coder * coder, const lzma_allocator * allocator, const uint8_t * in, size_t * in_pos, size_t in_size, lzma_action action)
{
	assert(coder->mf.read_pos <= coder->mf.write_pos);
	// Move the sliding window if needed.
	if(coder->mf.read_pos >= coder->mf.size - coder->mf.keep_size_after)
		move_window(&coder->mf);
	// Maybe this is ugly, but lzma_mf uses uint32 for most things
	// (which I find cleanest), but we need size_t here when filling
	// the history window.
	size_t write_pos = coder->mf.write_pos;
	lzma_ret ret;
	if(coder->next.code == NULL) {
		// Not using a filter, simply memcpy() as much as possible.
		lzma_bufcpy(in, in_pos, in_size, coder->mf.buffer, &write_pos, coder->mf.size);
		ret = action != LZMA_RUN && *in_pos == in_size ? LZMA_STREAM_END : LZMA_OK;
	}
	else {
		ret = coder->next.code(coder->next.coder, allocator, in, in_pos, in_size, coder->mf.buffer, &write_pos, coder->mf.size, action);
	}
	coder->mf.write_pos = write_pos;
	// Silence Valgrind. lzma_memcmplen() can read extra bytes
	// and Valgrind will give warnings if those bytes are uninitialized
	// because Valgrind cannot see that the values of the uninitialized
	// bytes are eventually ignored.
	memzero(coder->mf.buffer + write_pos, LZMA_MEMCMPLEN_EXTRA);
	// If end of stream has been reached or flushing completed, we allow
	// the encoder to process all the input (that is, read_pos is allowed
	// to reach write_pos). Otherwise we keep keep_size_after bytes
	// available as prebuffer.
	if(ret == LZMA_STREAM_END) {
		assert(*in_pos == in_size);
		ret = LZMA_OK;
		coder->mf.action = action;
		coder->mf.read_limit = coder->mf.write_pos;
	}
	else if(coder->mf.write_pos > coder->mf.keep_size_after) {
		// This needs to be done conditionally, because if we got
		// only little new input, there may be too little input
		// to do any encoding yet.
		coder->mf.read_limit = coder->mf.write_pos - coder->mf.keep_size_after;
	}
	// Restart the match finder after finished LZMA_SYNC_FLUSH.
	if(coder->mf.pending > 0 && coder->mf.read_pos < coder->mf.read_limit) {
		// Match finder may update coder->pending and expects it to
		// start from zero, so use a temporary variable.
		const uint32 pending = coder->mf.pending;
		coder->mf.pending = 0;
		// Rewind read_pos so that the match finder can hash
		// the pending bytes.
		assert(coder->mf.read_pos >= pending);
		coder->mf.read_pos -= pending;
		// Call the skip function directly instead of using
		// mf_skip(), since we don't want to touch mf->read_ahead.
		coder->mf.skip(&coder->mf, pending);
	}
	return ret;
}

static lzma_ret lz_encode(void * coder_ptr, const lzma_allocator * allocator, const uint8_t * in, size_t * in_pos,
    size_t in_size, uint8_t * out, size_t * out_pos, size_t out_size, lzma_action action)
{
	lzma_encoder_coder * coder = (lzma_encoder_coder *)coder_ptr;
	while(*out_pos < out_size && (*in_pos < in_size || action != LZMA_RUN)) {
		// Read more data to coder->mf.buffer if needed.
		if(coder->mf.action == LZMA_RUN && coder->mf.read_pos >= coder->mf.read_limit)
			return_if_error(fill_window(coder, allocator, in, in_pos, in_size, action));
		// Encode
		const lzma_ret ret = coder->lz.code(coder->lz.coder, &coder->mf, out, out_pos, out_size);
		if(ret != LZMA_OK) {
			// Setting this to LZMA_RUN for cases when we are
			// flushing. It doesn't matter when finishing or if
			// an error occurred.
			coder->mf.action = LZMA_RUN;
			return ret;
		}
	}
	return LZMA_OK;
}

static bool lz_encoder_prepare(lzma_mf * mf, const lzma_allocator * allocator, const lzma_lz_encoder_options * lz_options)
{
	// For now, the dictionary size is limited to 1.5 GiB. This may grow
	// in the future if needed, but it needs a little more work than just
	// changing this check.
	if(lz_options->dict_size < LZMA_DICT_SIZE_MIN || lz_options->dict_size > (UINT32_C(1) << 30) + (UINT32_C(1) << 29) || lz_options->nice_len > lz_options->match_len_max)
		return true;
	mf->keep_size_before = lz_options->before_size + lz_options->dict_size;
	mf->keep_size_after = lz_options->after_size + lz_options->match_len_max;
	// To avoid constant memmove()s, allocate some extra space. Since
	// memmove()s become more expensive when the size of the buffer
	// increases, we reserve more space when a large dictionary is
	// used to make the memmove() calls rarer.
	//
	// This works with dictionaries up to about 3 GiB. If bigger
	// dictionary is wanted, some extra work is needed:
	//   - Several variables in lzma_mf have to be changed from uint32
	//     to size_t.
	//   - Memory usage calculation needs something too, e.g. use uint64_t
	//     for mf->size.
	uint32 reserve = lz_options->dict_size / 2;
	if(reserve > (UINT32_C(1) << 30))
		reserve /= 2;
	reserve += (lz_options->before_size + lz_options->match_len_max + lz_options->after_size) / 2 + (UINT32_C(1) << 19);
	const uint32 old_size = mf->size;
	mf->size = mf->keep_size_before + reserve + mf->keep_size_after;
	// Deallocate the old history buffer if it exists but has different
	// size than what is needed now.
	if(mf->buffer != NULL && old_size != mf->size) {
		lzma_free(mf->buffer, allocator);
		mf->buffer = NULL;
	}
	// Match finder options
	mf->match_len_max = lz_options->match_len_max;
	mf->nice_len = lz_options->nice_len;
	// cyclic_size has to stay smaller than 2 Gi. Note that this doesn't
	// mean limiting dictionary size to less than 2 GiB. With a match
	// finder that uses multibyte resolution (hashes start at e.g. every
	// fourth byte), cyclic_size would stay below 2 Gi even when
	// dictionary size is greater than 2 GiB.
	//
	// It would be possible to allow cyclic_size >= 2 Gi, but then we
	// would need to be careful to use 64-bit types in various places
	// (size_t could do since we would need bigger than 32-bit address
	// space anyway). It would also require either zeroing a multigigabyte
	// buffer at initialization (waste of time and RAM) or allow
	// normalization in lz_encoder_mf.c to access uninitialized
	// memory to keep the code simpler. The current way is simple and
	// still allows pretty big dictionaries, so I don't expect these
	// limits to change.
	mf->cyclic_size = lz_options->dict_size + 1;

	// Validate the match finder ID and setup the function pointers.
	switch(lz_options->match_finder) {
#ifdef HAVE_MF_HC3
		case LZMA_MF_HC3:
		    mf->find = &lzma_mf_hc3_find;
		    mf->skip = &lzma_mf_hc3_skip;
		    break;
#endif
#ifdef HAVE_MF_HC4
		case LZMA_MF_HC4:
		    mf->find = &lzma_mf_hc4_find;
		    mf->skip = &lzma_mf_hc4_skip;
		    break;
#endif
#ifdef HAVE_MF_BT2
		case LZMA_MF_BT2:
		    mf->find = &lzma_mf_bt2_find;
		    mf->skip = &lzma_mf_bt2_skip;
		    break;
#endif
#ifdef HAVE_MF_BT3
		case LZMA_MF_BT3:
		    mf->find = &lzma_mf_bt3_find;
		    mf->skip = &lzma_mf_bt3_skip;
		    break;
#endif
#ifdef HAVE_MF_BT4
		case LZMA_MF_BT4:
		    mf->find = &lzma_mf_bt4_find;
		    mf->skip = &lzma_mf_bt4_skip;
		    break;
#endif
		default:
		    return true;
	}
	// Calculate the sizes of mf->hash and mf->son and check that
	// nice_len is big enough for the selected match finder.
	const uint32 hash_bytes = lz_options->match_finder & 0x0F;
	if(hash_bytes > mf->nice_len)
		return true;
	const bool is_bt = (lz_options->match_finder & 0x10) != 0;
	uint32 hs;
	if(hash_bytes == 2) {
		hs = 0xFFFF;
	}
	else {
		// Round dictionary size up to the next 2^n - 1 so it can
		// be used as a hash mask.
		hs = lz_options->dict_size - 1;
		hs |= hs >> 1;
		hs |= hs >> 2;
		hs |= hs >> 4;
		hs |= hs >> 8;
		hs >>= 1;
		hs |= 0xFFFF;

		if(hs > (UINT32_C(1) << 24)) {
			if(hash_bytes == 3)
				hs = (UINT32_C(1) << 24) - 1;
			else
				hs >>= 1;
		}
	}
	mf->hash_mask = hs;
	++hs;
	if(hash_bytes > 2)
		hs += HASH_2_SIZE;
	if(hash_bytes > 3)
		hs += HASH_3_SIZE;
/*
        No match finder uses this at the moment.
        if (mf->hash_bytes > 4)
                hs += HASH_4_SIZE;
 */
 	const uint32 old_hash_count = mf->hash_count;
	const uint32 old_sons_count = mf->sons_count;
	mf->hash_count = hs;
	mf->sons_count = mf->cyclic_size;
	if(is_bt)
		mf->sons_count *= 2;
	// Deallocate the old hash array if it exists and has different size
	// than what is needed now.
	if(old_hash_count != mf->hash_count || old_sons_count != mf->sons_count) {
		lzma_free(mf->hash, allocator);
		mf->hash = NULL;
		lzma_free(mf->son, allocator);
		mf->son = NULL;
	}
	// Maximum number of match finder cycles
	mf->depth = lz_options->depth;
	if(mf->depth == 0) {
		if(is_bt)
			mf->depth = 16 + mf->nice_len / 2;
		else
			mf->depth = 4 + mf->nice_len / 4;
	}
	return false;
}

static bool lz_encoder_init(lzma_mf * mf, const lzma_allocator * allocator, const lzma_lz_encoder_options * lz_options)
{
	// Allocate the history buffer.
	if(mf->buffer == NULL) {
		// lzma_memcmplen() is used for the dictionary buffer
		// so we need to allocate a few extra bytes to prevent
		// it from reading past the end of the buffer.
		mf->buffer = (uint8_t *)lzma_alloc(mf->size + LZMA_MEMCMPLEN_EXTRA, allocator);
		if(mf->buffer == NULL)
			return true;
		// Keep Valgrind happy with lzma_memcmplen() and initialize
		// the extra bytes whose value may get read but which will
		// effectively get ignored.
		memzero(mf->buffer + mf->size, LZMA_MEMCMPLEN_EXTRA);
	}
	// Use cyclic_size as initial mf->offset. This allows
	// avoiding a few branches in the match finders. The downside is
	// that match finder needs to be normalized more often, which may
	// hurt performance with huge dictionaries.
	mf->offset = mf->cyclic_size;
	mf->read_pos = 0;
	mf->read_ahead = 0;
	mf->read_limit = 0;
	mf->write_pos = 0;
	mf->pending = 0;

#if UINT32_MAX >= SIZE_MAX / 4
	// Check for integer overflow. (Huge dictionaries are not
	// possible on 32-bit CPU.)
	if(mf->hash_count > SIZE_MAX / sizeof(uint32) || mf->sons_count > SIZE_MAX / sizeof(uint32))
		return true;
#endif

	// Allocate and initialize the hash table. Since EMPTY_HASH_VALUE
	// is zero, we can use lzma_alloc_zero() or memzero() for mf->hash.
	//
	// We don't need to initialize mf->son, but not doing that may
	// make Valgrind complain in normalization (see normalize() in
	// lz_encoder_mf.c). Skipping the initialization is *very* good
	// when big dictionary is used but only small amount of data gets
	// actually compressed: most of the mf->son won't get actually
	// allocated by the kernel, so we avoid wasting RAM and improve
	// initialization speed a lot.
	if(mf->hash == NULL) {
		mf->hash = (uint32 *)lzma_alloc_zero(mf->hash_count * sizeof(uint32), allocator);
		mf->son = (uint32 *)lzma_alloc(mf->sons_count * sizeof(uint32), allocator);
		if(mf->hash == NULL || mf->son == NULL) {
			lzma_free(mf->hash, allocator);
			mf->hash = NULL;
			lzma_free(mf->son, allocator);
			mf->son = NULL;

			return true;
		}
	}
	else {
/*
                for (uint32 i = 0; i < mf->hash_count; ++i)
                        mf->hash[i] = EMPTY_HASH_VALUE;
 */
		memzero(mf->hash, mf->hash_count * sizeof(uint32));
	}
	mf->cyclic_pos = 0;
	// Handle preset dictionary.
	if(lz_options->preset_dict != NULL && lz_options->preset_dict_size > 0) {
		// If the preset dictionary is bigger than the actual
		// dictionary, use only the tail.
		mf->write_pos = MIN(lz_options->preset_dict_size, mf->size);
		memcpy(mf->buffer, lz_options->preset_dict + lz_options->preset_dict_size - mf->write_pos, mf->write_pos);
		mf->action = LZMA_SYNC_FLUSH;
		mf->skip(mf, mf->write_pos);
	}
	mf->action = LZMA_RUN;
	return false;
}

extern uint64 lzma_lz_encoder_memusage(const lzma_lz_encoder_options * lz_options)
{
	// Old buffers must not exist when calling lz_encoder_prepare().
	lzma_mf mf;// = { .buffer = NULL, .hash = NULL, .son = NULL, .hash_count = 0, .sons_count = 0, };
	memzero(&mf, sizeof(mf));
	// Setup the size information into mf.
	if(lz_encoder_prepare(&mf, NULL, lz_options))
		return UINT64_MAX;
	// Calculate the memory usage.
	return ((uint64_t)(mf.hash_count) + mf.sons_count) * sizeof(uint32) + mf.size + sizeof(lzma_encoder_coder);
}

static void lz_encoder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_encoder_coder * coder = (lzma_encoder_coder *)coder_ptr;
	lzma_next_end(&coder->next, allocator);
	lzma_free(coder->mf.son, allocator);
	lzma_free(coder->mf.hash, allocator);
	lzma_free(coder->mf.buffer, allocator);
	if(coder->lz.end != NULL)
		coder->lz.end(coder->lz.coder, allocator);
	else
		lzma_free(coder->lz.coder, allocator);
	lzma_free(coder, allocator);
}

static lzma_ret lz_encoder_update(void * coder_ptr, const lzma_allocator * allocator,
    const lzma_filter * filters_null lzma_attribute((__unused__)), const lzma_filter * reversed_filters)
{
	lzma_encoder_coder * coder = (lzma_encoder_coder *)coder_ptr;
	if(coder->lz.options_update == NULL)
		return LZMA_PROG_ERROR;
	return_if_error(coder->lz.options_update(coder->lz.coder, reversed_filters));
	return lzma_next_filter_update(&coder->next, allocator, reversed_filters + 1);
}

extern lzma_ret lzma_lz_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator,
    const lzma_filter_info * filters, lzma_ret (*lz_init)(lzma_lz_encoder * lz,
    const lzma_allocator * allocator, const void * options, lzma_lz_encoder_options * lz_options))
{
#ifdef HAVE_SMALL
	// We need that the CRC32 table has been initialized.
	lzma_crc32_init();
#endif
	// Allocate and initialize the base data structure.
	lzma_encoder_coder * coder = (lzma_encoder_coder *)next->coder;
	if(coder == NULL) {
		coder = (lzma_encoder_coder *)lzma_alloc(sizeof(lzma_encoder_coder), allocator);
		if(coder == NULL)
			return LZMA_MEM_ERROR;
		next->coder = coder;
		next->code = &lz_encode;
		next->end = &lz_encoder_end;
		next->update = &lz_encoder_update;
		coder->lz.coder = NULL;
		coder->lz.code = NULL;
		coder->lz.end = NULL;

		// mf.size is initialized to silence Valgrind
		// when used on optimized binaries (GCC may reorder
		// code in a way that Valgrind gets unhappy).
		coder->mf.buffer = NULL;
		coder->mf.size = 0;
		coder->mf.hash = NULL;
		coder->mf.son = NULL;
		coder->mf.hash_count = 0;
		coder->mf.sons_count = 0;
		coder->next.SetDefault();// = LZMA_NEXT_CODER_INIT;
	}
	// Initialize the LZ-based encoder.
	lzma_lz_encoder_options lz_options;
	return_if_error(lz_init(&coder->lz, allocator, filters[0].options, &lz_options));
	// Setup the size information into coder->mf and deallocate
	// old buffers if they have wrong size.
	if(lz_encoder_prepare(&coder->mf, allocator, &lz_options))
		return LZMA_OPTIONS_ERROR;
	// Allocate new buffers if needed, and do the rest of the initialization.
	if(lz_encoder_init(&coder->mf, allocator, &lz_options))
		return LZMA_MEM_ERROR;
	// Initialize the next filter in the chain, if any.
	return lzma_next_filter_init(&coder->next, allocator, filters + 1);
}

bool lzma_mf_is_supported(lzma_match_finder mf)
{
	bool ret = false;
#ifdef HAVE_MF_HC3
	if(mf == LZMA_MF_HC3)
		ret = true;
#endif
#ifdef HAVE_MF_HC4
	if(mf == LZMA_MF_HC4)
		ret = true;
#endif
#ifdef HAVE_MF_BT2
	if(mf == LZMA_MF_BT2)
		ret = true;
#endif
#ifdef HAVE_MF_BT3
	if(mf == LZMA_MF_BT3)
		ret = true;
#endif
#ifdef HAVE_MF_BT4
	if(mf == LZMA_MF_BT4)
		ret = true;
#endif
	return ret;
}
//
// lz_encoder_mf
//
/// \brief      Find matches starting from the current byte
///
/// \return     The length of the longest match found
extern uint32 lzma_mf_find(lzma_mf * mf, uint32 * count_ptr, lzma_match * matches)
{
	// Call the match finder. It returns the number of length-distance
	// pairs found.
	// FIXME: Minimum count is zero, what _exactly_ is the maximum?
	const uint32 count = mf->find(mf, matches);
	// Length of the longest match; assume that no matches were found
	// and thus the maximum length is zero.
	uint32 len_best = 0;
	if(count > 0) {
#ifndef NDEBUG
		// Validate the matches.
		for(uint32 i = 0; i < count; ++i) {
			assert(matches[i].len <= mf->nice_len);
			assert(matches[i].dist < mf->read_pos);
			assert(memcmp(mf_ptr(mf) - 1, mf_ptr(mf) - matches[i].dist - 2, matches[i].len) == 0);
		}
#endif

		// The last used element in the array contains
		// the longest match.
		len_best = matches[count - 1].len;

		// If a match of maximum search length was found, try to
		// extend the match to maximum possible length.
		if(len_best == mf->nice_len) {
			// The limit for the match length is either the
			// maximum match length supported by the LZ-based
			// encoder or the number of bytes left in the
			// dictionary, whichever is smaller.
			uint32 limit = mf_avail(mf) + 1;
			if(limit > mf->match_len_max)
				limit = mf->match_len_max;
			// Pointer to the byte we just ran through
			// the match finder.
			const uint8_t * p1 = mf_ptr(mf) - 1;
			// Pointer to the beginning of the match. We need -1
			// here because the match distances are zero based.
			const uint8_t * p2 = p1 - matches[count - 1].dist - 1;
			len_best = lzma_memcmplen(p1, p2, len_best, limit);
		}
	}
	*count_ptr = count;
	// Finally update the read position to indicate that match finder was
	// run for this dictionary offset.
	++mf->read_ahead;
	return len_best;
}

/// Hash value to indicate unused element in the hash. Since we start the
/// positions from dict_size + 1, zero is always too far to qualify
/// as usable match position.
#define EMPTY_HASH_VALUE 0

/// Normalization must be done when lzma_mf.offset + lzma_mf.read_pos
/// reaches MUST_NORMALIZE_POS.
#define MUST_NORMALIZE_POS UINT32_MAX

/// \brief      Normalizes hash values
///
/// The hash arrays store positions of match candidates. The positions are
/// relative to an arbitrary offset that is not the same as the absolute
/// offset in the input stream. The relative position of the current byte
/// is lzma_mf.offset + lzma_mf.read_pos. The distances of the matches are
/// the differences of the current read position and the position found from
/// the hash.
///
/// To prevent integer overflows of the offsets stored in the hash arrays,
/// we need to "normalize" the stored values now and then. During the
/// normalization, we drop values that indicate distance greater than the
/// dictionary size, thus making space for new values.
static void normalize(lzma_mf * mf)
{
	assert(mf->read_pos + mf->offset == MUST_NORMALIZE_POS);
	// In future we may not want to touch the lowest bits, because there
	// may be match finders that use larger resolution than one byte.
	const uint32 subvalue = (MUST_NORMALIZE_POS - mf->cyclic_size);
	// & ~((UINT32_C(1) << 10) - 1);
	for(uint32 i = 0; i < mf->hash_count; ++i) {
		// If the distance is greater than the dictionary size,
		// we can simply mark the hash element as empty.
		if(mf->hash[i] <= subvalue)
			mf->hash[i] = EMPTY_HASH_VALUE;
		else
			mf->hash[i] -= subvalue;
	}
	for(uint32 i = 0; i < mf->sons_count; ++i) {
		// Do the same for mf->son.
		//
		// NOTE: There may be uninitialized elements in mf->son.
		// Valgrind may complain that the "if" below depends on
		// an uninitialized value. In this case it is safe to ignore
		// the warning. See also the comments in lz_encoder_init()
		// in lz_encoder.c.
		if(mf->son[i] <= subvalue)
			mf->son[i] = EMPTY_HASH_VALUE;
		else
			mf->son[i] -= subvalue;
	}
	// Update offset to match the new locations.
	mf->offset -= subvalue;
}

/// Mark the current byte as processed from point of view of the match finder.
static void move_pos(lzma_mf * mf)
{
	if(++mf->cyclic_pos == mf->cyclic_size)
		mf->cyclic_pos = 0;
	++mf->read_pos;
	assert(mf->read_pos <= mf->write_pos);
	if(UNLIKELY(mf->read_pos + mf->offset == UINT32_MAX))
		normalize(mf);
}

/// When flushing, we cannot run the match finder unless there is nice_len
/// bytes available in the dictionary. Instead, we skip running the match
/// finder (indicating that no match was found), and count how many bytes we
/// have ignored this way.
///
/// When new data is given after the flushing was completed, the match finder
/// is restarted by rewinding mf->read_pos backwards by mf->pending. Then
/// the missed bytes are added to the hash using the match finder's skip
/// function (with small amount of input, it may start using mf->pending
/// again if flushing).
///
/// Due to this rewinding, we don't touch cyclic_pos or test for
/// normalization. It will be done when the match finder's skip function
/// catches up after a flush.
static void move_pending(lzma_mf * mf)
{
	++mf->read_pos;
	assert(mf->read_pos <= mf->write_pos);
	++mf->pending;
}

/// Calculate len_limit and determine if there is enough input to run
/// the actual match finder code. Sets up "cur" and "pos". This macro
/// is used by all find functions and binary tree skip functions. Hash
/// chain skip function doesn't need len_limit so a simpler code is used
/// in them.
#define header(is_bt, len_min, ret_op) \
	uint32 len_limit = mf_avail(mf); \
	if(mf->nice_len <= len_limit) { \
		len_limit = mf->nice_len; \
	} else if(len_limit < (len_min) || (is_bt && mf->action == LZMA_SYNC_FLUSH)) { \
		assert(mf->action != LZMA_RUN); \
		move_pending(mf); \
		ret_op; \
	} \
	const uint8 * cur = mf_ptr(mf); \
	const uint32 pos = mf->read_pos + mf->offset

/// Header for find functions. "return 0" indicates that zero matches
/// were found.
#define header_find(is_bt, len_min) header(is_bt, len_min, return 0); uint32 matches_count = 0

/// Header for a loop in a skip function. "continue" tells to skip the rest
/// of the code in the loop.
#define header_skip(is_bt, len_min) header(is_bt, len_min, continue)

/// Calls hc_find_func() or bt_find_func() and calculates the total number
/// of matches found. Updates the dictionary position and returns the number
/// of matches found.
#define call_find(func, len_best) \
	do { \
		matches_count = func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos, mf->cyclic_size, \
			matches + matches_count, len_best) - matches; \
		move_pos(mf); \
		return matches_count; \
	} while(0)
//
// Hash Chain
//
#if defined(HAVE_MF_HC3) || defined(HAVE_MF_HC4)
///
///
/// \param      len_limit       Don't look for matches longer than len_limit.
/// \param      pos             lzma_mf.read_pos + lzma_mf.offset
/// \param      cur             Pointer to current byte (mf_ptr(mf))
/// \param      cur_match       Start position of the current match candidate
/// \param      depth           Maximum length of the hash chain
/// \param      son             lzma_mf.son (contains the hash chain)
/// \param      cyclic_pos
/// \param      cyclic_size
/// \param      matches         Array to hold the matches.
/// \param      len_best        The length of the longest match found so far.
static lzma_match * hc_find_func(const uint32 len_limit, const uint32 pos, const uint8_t * const cur, uint32 cur_match,
    uint32 depth, uint32 * const son, const uint32 cyclic_pos, const uint32 cyclic_size, lzma_match * matches, uint32 len_best)
{
	son[cyclic_pos] = cur_match;
	while(true) {
		const uint32 delta = pos - cur_match;
		if(depth-- == 0 || delta >= cyclic_size)
			return matches;
		const uint8_t * const pb = cur - delta;
		cur_match = son[cyclic_pos - delta + (delta > cyclic_pos ? cyclic_size : 0)];
		if(pb[len_best] == cur[len_best] && pb[0] == cur[0]) {
			uint32 len = lzma_memcmplen(pb, cur, 1, len_limit);
			if(len_best < len) {
				len_best = len;
				matches->len = len;
				matches->dist = delta - 1;
				++matches;
				if(len == len_limit)
					return matches;
			}
		}
	}
}

#define hc_find(len_best) call_find(hc_find_func, len_best)

#define hc_skip() \
	do { \
		mf->son[mf->cyclic_pos] = cur_match; \
		move_pos(mf); \
	} while(0)

#endif

#ifdef HAVE_MF_HC3
extern uint32 lzma_mf_hc3_find(lzma_mf * mf, lzma_match * matches)
{
	header_find(false, 3);
	hash_3_calc();
	const uint32 delta2 = pos - mf->hash[hash_2_value];
	const uint32 cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
	mf->hash[hash_2_value] = pos;
	mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
	uint32 len_best = 2;
	if(delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
		len_best = lzma_memcmplen(cur - delta2, cur, len_best, len_limit);
		matches[0].len = len_best;
		matches[0].dist = delta2 - 1;
		matches_count = 1;
		if(len_best == len_limit) {
			hc_skip();
			return 1; // matches_count
		}
	}
	hc_find(len_best);
}

extern void lzma_mf_hc3_skip(lzma_mf * mf, uint32 amount)
{
	do {
		if(mf_avail(mf) < 3) {
			move_pending(mf);
			continue;
		}
		const uint8 * cur = mf_ptr(mf);
		const uint32 pos = mf->read_pos + mf->offset;
		hash_3_calc();
		const uint32 cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
		mf->hash[hash_2_value] = pos;
		mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
		hc_skip();
	} while(--amount != 0);
}

#endif

#ifdef HAVE_MF_HC4
extern uint32 lzma_mf_hc4_find(lzma_mf * mf, lzma_match * matches)
{
	header_find(false, 4);
	hash_4_calc();
	uint32 delta2 = pos - mf->hash[hash_2_value];
	const uint32 delta3 = pos - mf->hash[FIX_3_HASH_SIZE + hash_3_value];
	const uint32 cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
	mf->hash[hash_2_value ] = pos;
	mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
	mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
	uint32 len_best = 1;
	if(delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
		len_best = 2;
		matches[0].len = 2;
		matches[0].dist = delta2 - 1;
		matches_count = 1;
	}
	if(delta2 != delta3 && delta3 < mf->cyclic_size && *(cur - delta3) == *cur) {
		len_best = 3;
		matches[matches_count++].dist = delta3 - 1;
		delta2 = delta3;
	}
	if(matches_count != 0) {
		len_best = lzma_memcmplen(cur - delta2, cur, len_best, len_limit);
		matches[matches_count - 1].len = len_best;
		if(len_best == len_limit) {
			hc_skip();
			return matches_count;
		}
	}
	if(len_best < 3)
		len_best = 3;
	hc_find(len_best);
}

extern void lzma_mf_hc4_skip(lzma_mf * mf, uint32 amount)
{
	do {
		if(mf_avail(mf) < 4) {
			move_pending(mf);
			continue;
		}
		const uint8 * cur = mf_ptr(mf);
		const uint32 pos = mf->read_pos + mf->offset;
		hash_4_calc();
		const uint32 cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
		mf->hash[hash_2_value] = pos;
		mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
		mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
		hc_skip();
	} while(--amount != 0);
}

#endif
//
// Binary Tree //
//
#if defined(HAVE_MF_BT2) || defined(HAVE_MF_BT3) || defined(HAVE_MF_BT4)
static lzma_match * bt_find_func(const uint32 len_limit, const uint32 pos, const uint8_t * const cur,
    uint32 cur_match, uint32 depth, uint32 * const son, const uint32 cyclic_pos, const uint32 cyclic_size,
    lzma_match * matches, uint32 len_best)
{
	uint32 * ptr0 = son + (cyclic_pos << 1) + 1;
	uint32 * ptr1 = son + (cyclic_pos << 1);
	uint32 len0 = 0;
	uint32 len1 = 0;
	while(true) {
		const uint32 delta = pos - cur_match;
		if(depth-- == 0 || delta >= cyclic_size) {
			*ptr0 = EMPTY_HASH_VALUE;
			*ptr1 = EMPTY_HASH_VALUE;
			return matches;
		}
		uint32 * const pair = son + ((cyclic_pos - delta + (delta > cyclic_pos ? cyclic_size : 0)) << 1);
		const uint8_t * const pb = cur - delta;
		uint32 len = MIN(len0, len1);
		if(pb[len] == cur[len]) {
			len = lzma_memcmplen(pb, cur, len + 1, len_limit);
			if(len_best < len) {
				len_best = len;
				matches->len = len;
				matches->dist = delta - 1;
				++matches;

				if(len == len_limit) {
					*ptr1 = pair[0];
					*ptr0 = pair[1];
					return matches;
				}
			}
		}

		if(pb[len] < cur[len]) {
			*ptr1 = cur_match;
			ptr1 = pair + 1;
			cur_match = *ptr1;
			len1 = len;
		}
		else {
			*ptr0 = cur_match;
			ptr0 = pair;
			cur_match = *ptr0;
			len0 = len;
		}
	}
}

static void bt_skip_func(const uint32 len_limit, const uint32 pos, const uint8 * const cur, uint32 cur_match,
    uint32 depth, uint32 * const son, const uint32 cyclic_pos, const uint32 cyclic_size)
{
	uint32 * ptr0 = son + (cyclic_pos << 1) + 1;
	uint32 * ptr1 = son + (cyclic_pos << 1);
	uint32 len0 = 0;
	uint32 len1 = 0;
	while(true) {
		const uint32 delta = pos - cur_match;
		if(depth-- == 0 || delta >= cyclic_size) {
			*ptr0 = EMPTY_HASH_VALUE;
			*ptr1 = EMPTY_HASH_VALUE;
			return;
		}
		uint32 * pair = son + ((cyclic_pos - delta + (delta > cyclic_pos ? cyclic_size : 0)) << 1);
		const uint8_t * pb = cur - delta;
		uint32 len = MIN(len0, len1);
		if(pb[len] == cur[len]) {
			len = lzma_memcmplen(pb, cur, len + 1, len_limit);
			if(len == len_limit) {
				*ptr1 = pair[0];
				*ptr0 = pair[1];
				return;
			}
		}
		if(pb[len] < cur[len]) {
			*ptr1 = cur_match;
			ptr1 = pair + 1;
			cur_match = *ptr1;
			len1 = len;
		}
		else {
			*ptr0 = cur_match;
			ptr0 = pair;
			cur_match = *ptr0;
			len0 = len;
		}
	}
}

#define bt_find(len_best) call_find(bt_find_func, len_best)

#define bt_skip() \
	do { \
		bt_skip_func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos, mf->cyclic_size); \
		move_pos(mf); \
	} while(0)

#endif

#ifdef HAVE_MF_BT2
extern uint32 lzma_mf_bt2_find(lzma_mf * mf, lzma_match * matches)
{
	header_find(true, 2);
	hash_2_calc();
	const uint32 cur_match = mf->hash[hash_value];
	mf->hash[hash_value] = pos;
	bt_find(1);
}

extern void lzma_mf_bt2_skip(lzma_mf * mf, uint32 amount)
{
	do {
		header_skip(true, 2);
		hash_2_calc();
		const uint32 cur_match = mf->hash[hash_value];
		mf->hash[hash_value] = pos;
		bt_skip();
	} while(--amount != 0);
}
#endif

#ifdef HAVE_MF_BT3
extern uint32 lzma_mf_bt3_find(lzma_mf * mf, lzma_match * matches)
{
	header_find(true, 3);
	hash_3_calc();
	const uint32 delta2 = pos - mf->hash[hash_2_value];
	const uint32 cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
	mf->hash[hash_2_value] = pos;
	mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
	uint32 len_best = 2;
	if(delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
		len_best = lzma_memcmplen(cur, cur - delta2, len_best, len_limit);
		matches[0].len = len_best;
		matches[0].dist = delta2 - 1;
		matches_count = 1;
		if(len_best == len_limit) {
			bt_skip();
			return 1; // matches_count
		}
	}
	bt_find(len_best);
}

extern void lzma_mf_bt3_skip(lzma_mf * mf, uint32 amount)
{
	do {
		header_skip(true, 3);
		hash_3_calc();
		const uint32 cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
		mf->hash[hash_2_value] = pos;
		mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
		bt_skip();
	} while(--amount != 0);
}

#endif

#ifdef HAVE_MF_BT4
extern uint32 lzma_mf_bt4_find(lzma_mf * mf, lzma_match * matches)
{
	header_find(true, 4);
	hash_4_calc();
	uint32 delta2 = pos - mf->hash[hash_2_value];
	const uint32 delta3 = pos - mf->hash[FIX_3_HASH_SIZE + hash_3_value];
	const uint32 cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
	mf->hash[hash_2_value] = pos;
	mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
	mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
	uint32 len_best = 1;
	if(delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
		len_best = 2;
		matches[0].len = 2;
		matches[0].dist = delta2 - 1;
		matches_count = 1;
	}
	if(delta2 != delta3 && delta3 < mf->cyclic_size && *(cur - delta3) == *cur) {
		len_best = 3;
		matches[matches_count++].dist = delta3 - 1;
		delta2 = delta3;
	}
	if(matches_count != 0) {
		len_best = lzma_memcmplen(cur, cur - delta2, len_best, len_limit);
		matches[matches_count - 1].len = len_best;
		if(len_best == len_limit) {
			bt_skip();
			return matches_count;
		}
	}
	if(len_best < 3)
		len_best = 3;
	bt_find(len_best);
}

extern void lzma_mf_bt4_skip(lzma_mf * mf, uint32 amount)
{
	do {
		header_skip(true, 4);
		hash_4_calc();
		const uint32 cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
		mf->hash[hash_2_value] = pos;
		mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
		mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
		bt_skip();
	} while(--amount != 0);
}
#endif
//
// lz_decoder
// LZ out window
//
static void lz_decoder_reset(lzma_decoder_coder * coder)
{
	coder->dict.pos = 0;
	coder->dict.full = 0;
	coder->dict.buf[coder->dict.size - 1] = '\0';
	coder->dict.need_reset = false;
}

static lzma_ret decode_buffer(lzma_decoder_coder * coder, const uint8_t * in, size_t * in_pos,
    size_t in_size, uint8_t * out, size_t * out_pos, size_t out_size)
{
	while(true) {
		// Wrap the dictionary if needed.
		if(coder->dict.pos == coder->dict.size)
			coder->dict.pos = 0;
		// Store the current dictionary position. It is needed to know
		// where to start copying to the out[] buffer.
		const size_t dict_start = coder->dict.pos;
		// Calculate how much we allow coder->lz.code() to decode.
		// It must not decode past the end of the dictionary
		// buffer, and we don't want it to decode more than is
		// actually needed to fill the out[] buffer.
		coder->dict.limit = coder->dict.pos + MIN(out_size - *out_pos, coder->dict.size - coder->dict.pos);
		// Call the coder->lz.code() to do the actual decoding.
		const lzma_ret ret = coder->lz.code(coder->lz.coder, &coder->dict, in, in_pos, in_size);
		// Copy the decoded data from the dictionary to the out[]
		// buffer. Do it conditionally because out can be NULL
		// (in which case copy_size is always 0). Calling memcpy()
		// with a null-pointer is undefined even if the third
		// argument is 0.
		const size_t copy_size = coder->dict.pos - dict_start;
		assert(copy_size <= out_size - *out_pos);
		if(copy_size > 0)
			memcpy(out + *out_pos, coder->dict.buf + dict_start, copy_size);
		*out_pos += copy_size;
		// Reset the dictionary if so requested by coder->lz.code().
		if(coder->dict.need_reset) {
			lz_decoder_reset(coder);
			// Since we reset dictionary, we don't check if
			// dictionary became full.
			if(ret != LZMA_OK || *out_pos == out_size)
				return ret;
		}
		else {
			// Return if everything got decoded or an error
			// occurred, or if there's no more data to decode.
			//
			// Note that detecting if there's something to decode
			// is done by looking if dictionary become full
			// instead of looking if *in_pos == in_size. This
			// is because it is possible that all the input was
			// consumed already but some data is pending to be
			// written to the dictionary.
			if(ret != LZMA_OK || *out_pos == out_size || coder->dict.pos < coder->dict.size)
				return ret;
		}
	}
}

static lzma_ret lz_decode(void * coder_ptr, const lzma_allocator * allocator, const uint8_t * in, size_t * in_pos,
    size_t in_size, uint8_t * out, size_t * out_pos, size_t out_size, lzma_action action)
{
	lzma_decoder_coder * coder = (lzma_decoder_coder *)coder_ptr;
	if(coder->next.code == NULL)
		return decode_buffer(coder, in, in_pos, in_size, out, out_pos, out_size);
	// We aren't the last coder in the chain, we need to decode
	// our input to a temporary buffer.
	while(*out_pos < out_size) {
		// Fill the temporary buffer if it is empty.
		if(!coder->next_finished && coder->temp.pos == coder->temp.size) {
			coder->temp.pos = 0;
			coder->temp.size = 0;
			const lzma_ret ret = coder->next.code(coder->next.coder, allocator, in, in_pos, in_size, coder->temp.buffer, &coder->temp.size, LZMA_BUFFER_SIZE, action);
			if(ret == LZMA_STREAM_END)
				coder->next_finished = true;
			else if(ret != LZMA_OK || coder->temp.size == 0)
				return ret;
		}
		if(coder->this_finished) {
			if(coder->temp.size != 0)
				return LZMA_DATA_ERROR;
			if(coder->next_finished)
				return LZMA_STREAM_END;
			return LZMA_OK;
		}
		const lzma_ret ret = decode_buffer(coder, coder->temp.buffer, &coder->temp.pos, coder->temp.size, out, out_pos, out_size);
		if(ret == LZMA_STREAM_END)
			coder->this_finished = true;
		else if(ret != LZMA_OK)
			return ret;
		else if(coder->next_finished && *out_pos < out_size)
			return LZMA_DATA_ERROR;
	}
	return LZMA_OK;
}

static void lz_decoder_end(void * coder_ptr, const lzma_allocator * allocator)
{
	lzma_decoder_coder * coder = (lzma_decoder_coder *)coder_ptr;
	lzma_next_end(&coder->next, allocator);
	lzma_free(coder->dict.buf, allocator);
	if(coder->lz.end != NULL)
		coder->lz.end(coder->lz.coder, allocator);
	else
		lzma_free(coder->lz.coder, allocator);
	lzma_free(coder, allocator);
}

extern lzma_ret lzma_lz_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator,
    const lzma_filter_info * filters, lzma_ret (*lz_init)(lzma_lz_decoder * lz,
    const lzma_allocator * allocator, const void * options, lzma_lz_decoder_options * lz_options))
{
	// Allocate the base structure if it isn't already allocated.
	lzma_decoder_coder * coder = (lzma_decoder_coder *)next->coder;
	if(coder == NULL) {
		coder = (lzma_decoder_coder *)lzma_alloc(sizeof(lzma_decoder_coder), allocator);
		if(coder == NULL)
			return LZMA_MEM_ERROR;
		next->coder = coder;
		next->code = &lz_decode;
		next->end = &lz_decoder_end;
		coder->dict.buf = NULL;
		coder->dict.size = 0;
		coder->lz.SetDefault(); // = LZMA_LZ_DECODER_INIT;
		coder->next.SetDefault();// = LZMA_NEXT_CODER_INIT;
	}
	// Allocate and initialize the LZ-based decoder. It will also give
	// us the dictionary size.
	lzma_lz_decoder_options lz_options;
	return_if_error(lz_init(&coder->lz, allocator, filters[0].options, &lz_options));
	// If the dictionary size is very small, increase it to 4096 bytes.
	// This is to prevent constant wrapping of the dictionary, which
	// would slow things down. The downside is that since we don't check
	// separately for the real dictionary size, we may happily accept
	// corrupt files.
	if(lz_options.dict_size < 4096)
		lz_options.dict_size = 4096;

	// Make dictionary size a multiple of 16. Some LZ-based decoders like
	// LZMA use the lowest bits lzma_dict.pos to know the alignment of the
	// data. Aligned buffer is also good when memcpying from the
	// dictionary to the output buffer, since applications are
	// recommended to give aligned buffers to liblzma.
	//
	// Avoid integer overflow.
	if(lz_options.dict_size > SIZE_MAX - 15)
		return LZMA_MEM_ERROR;
	lz_options.dict_size = (lz_options.dict_size + 15) & ~((size_t)(15));
	// Allocate and initialize the dictionary.
	if(coder->dict.size != lz_options.dict_size) {
		lzma_free(coder->dict.buf, allocator);
		coder->dict.buf = (uint8_t *)lzma_alloc(lz_options.dict_size, allocator);
		if(coder->dict.buf == NULL)
			return LZMA_MEM_ERROR;
		coder->dict.size = lz_options.dict_size;
	}
	lz_decoder_reset((lzma_decoder_coder *)next->coder);
	// Use the preset dictionary if it was given to us.
	if(lz_options.preset_dict != NULL && lz_options.preset_dict_size > 0) {
		// If the preset dictionary is bigger than the actual
		// dictionary, copy only the tail.
		const size_t copy_size = MIN(lz_options.preset_dict_size, lz_options.dict_size);
		const size_t offset = lz_options.preset_dict_size - copy_size;
		memcpy(coder->dict.buf, lz_options.preset_dict + offset, copy_size);
		coder->dict.pos = copy_size;
		coder->dict.full = copy_size;
	}
	// Miscellaneous initializations
	coder->next_finished = false;
	coder->this_finished = false;
	coder->temp.pos = 0;
	coder->temp.size = 0;
	// Initialize the next filter in the chain, if any.
	return lzma_next_filter_init(&coder->next, allocator, filters + 1);
}

extern uint64_t lzma_lz_decoder_memusage(size_t dictionary_size)
{
	return sizeof(lzma_decoder_coder) + (uint64_t)(dictionary_size);
}

extern void lzma_lz_decoder_uncompressed(void * coder_ptr, lzma_vli uncompressed_size)
{
	lzma_decoder_coder * coder = (lzma_decoder_coder *)coder_ptr;
	coder->lz.set_uncompressed(coder->lz.coder, uncompressed_size);
}

