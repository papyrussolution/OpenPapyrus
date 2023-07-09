/// \file       lz-internal.h
/// \brief      LZ
//  Authors:    Igor Pavlov
//              Lasse Collin
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#ifndef LZMA_LZ_INTERNAL_H
#define LZMA_LZ_INTERNAL_H
//
// lz_encoder {
// 
//
// A table of these is used by the LZ-based encoder to hold
// the length-distance pairs found by the match finder.
//
struct lzma_match {
	uint32 len;
	uint32 dist;
};

struct lzma_mf {
	//
	// In Window //
	//
	uint8 * buffer; /// Pointer to buffer with data to be compressed
	uint32 size; /// Total size of the allocated buffer (that is, including all the extra space)
	/// Number of bytes that must be kept available in our input history.
	/// That is, once keep_size_before bytes have been processed,
	/// buffer[read_pos - keep_size_before] is the oldest byte that
	/// must be available for reading.
	uint32 keep_size_before;
	/// Number of bytes that must be kept in buffer after read_pos.
	/// That is, read_pos <= write_pos - keep_size_after as long as
	/// action is LZMA_RUN; when action != LZMA_RUN, read_pos is allowed
	/// to reach write_pos so that the last bytes get encoded too.
	uint32 keep_size_after;
	/// Match finders store locations of matches using 32-bit integers.
	/// To avoid adjusting several megabytes of integers every time the
	/// input window is moved with move_window, we only adjust the
	/// offset of the buffer. Thus, buffer[value_in_hash_table - offset]
	/// is the byte pointed by value_in_hash_table.
	uint32 offset;
	/// buffer[read_pos] is the next byte to run through the match
	/// finder. This is incremented in the match finder once the byte
	/// has been processed.
	uint32 read_pos;
	/// Number of bytes that have been ran through the match finder, but
	/// which haven't been encoded by the LZ-based encoder yet.
	uint32 read_ahead;
	/// As long as read_pos is less than read_limit, there is enough
	/// input available in buffer for at least one encoding loop.
	///
	/// Because of the stateful API, read_limit may and will get greater
	/// than read_pos quite often. This is taken into account when
	/// calculating the value for keep_size_after.
	uint32 read_limit;
	/// buffer[write_pos] is the first byte that doesn't contain valid
	/// uncompressed data; that is, the next input byte will be copied
	/// to buffer[write_pos].
	uint32 write_pos;
	/// Number of bytes not hashed before read_pos. This is needed to
	/// restart the match finder after LZMA_SYNC_FLUSH.
	uint32 pending;
	//
	// Match Finder //
	//
	/// Find matches. Returns the number of distance-length pairs written
	/// to the matches array. This is called only via lzma_mf_find().
	uint32 (* find)(lzma_mf * mf, lzma_match * matches);
	/// Skips num bytes. This is like find() but doesn't make the
	/// distance-length pairs available, thus being a little faster.
	/// This is called only via mf_skip().
	void (* skip)(lzma_mf * mf, uint32 num);
	uint32 * hash;
	uint32 * son;
	uint32 cyclic_pos;
	uint32 cyclic_size; // Must be dictionary size + 1.
	uint32 hash_mask;
	uint32 depth; /// Maximum number of loops in the match finder
	uint32 nice_len; /// Maximum length of a match that the match finder will try to find.
	/// Maximum length of a match supported by the LZ-based encoder.
	/// If the longest match found by the match finder is nice_len,
	/// mf_find() tries to expand it up to match_len_max bytes.
	uint32 match_len_max;
	/// When running out of input, binary tree match finders need to know
	/// if it is due to flushing or finishing. The action is used also
	/// by the LZ-based encoders themselves.
	lzma_action action;
	uint32 hash_count; /// Number of elements in hash[]
	uint32 sons_count; /// Number of elements in son[]
};

struct lzma_lz_encoder_options { // @sobolev lzma_lz_options-->lzma_lz_encoder_options
	size_t before_size; /// Extra amount of data to keep available before the "actual" dictionary.
	size_t dict_size; /// Size of the history buffer
	size_t after_size; /// Extra amount of data to keep available after the "actual" dictionary.
	/// Maximum length of a match that the LZ-based encoder can accept.
	/// This is used to extend matches of length nice_len to the
	/// maximum possible length.
	size_t match_len_max;
	/// Match finder will search matches up to this length.
	/// This must be less than or equal to match_len_max.
	size_t nice_len;
	lzma_match_finder match_finder; /// Type of the match finder to use
	uint32 depth; /// Maximum search depth
	const uint8 * preset_dict; /// TODO: Comment
	uint32 preset_dict_size;
};

// The total usable buffer space at any moment outside the match finder:
// before_size + dict_size + after_size + match_len_max
//
// In reality, there's some extra space allocated to prevent the number of
// memmove() calls reasonable. The bigger the dict_size is, the bigger
// this extra buffer will be since with bigger dictionaries memmove() would
// also take longer.
//
// A single encoder loop in the LZ-based encoder may call the match finder
// (mf_find() or mf_skip()) at most after_size times. In other words,
// a single encoder loop may increment lzma_mf.read_pos at most after_size
// times. Since matches are looked up to
// lzma_mf.buffer[lzma_mf.read_pos + match_len_max - 1], the total
// amount of extra buffer needed after dict_size becomes
// after_size + match_len_max.
//
// before_size has two uses. The first one is to keep literals available
// in cases when the LZ-based encoder has made some read ahead.
// TODO: Maybe this could be changed by making the LZ-based encoders to
// store the actual literals as they do with length-distance pairs.
//
// Algorithms such as LZMA2 first try to compress a chunk, and then check
// if the encoded result is smaller than the uncompressed one. If the chunk
// was uncompressible, it is better to store it in uncompressed form in
// the output stream. To do this, the whole uncompressed chunk has to be
// still available in the history buffer. before_size achieves that.
//
struct lzma_lz_encoder {
	void * coder; /// Data specific to the LZ-based encoder
	/// Function to encode from *dict to out[]
	lzma_ret (* code)(void * coder, lzma_mf * mf, uint8 * out, size_t * out_pos, size_t out_size);
	void (*end)(void * coder, const lzma_allocator *allocator); /// Free allocated resources
	lzma_ret (* options_update)(void * coder, const lzma_filter * filter); /// Update the options in the middle of the encoding.
};
//
// Basic steps:
//  1. Input gets copied into the dictionary.
//  2. Data in dictionary gets run through the match finder byte by byte.
//  3. The literals and matches are encoded using e.g. LZMA.
//
// The bytes that have been ran through the match finder, but not encoded yet,
// are called `read ahead'.

/// Get pointer to the first byte not ran through the match finder
static inline const uint8 * mf_ptr(const lzma_mf * mf) { return mf->buffer + mf->read_pos; }

/// Get the number of bytes that haven't been ran through the match finder yet.
static inline uint32 mf_avail(const lzma_mf * mf) { return mf->write_pos - mf->read_pos; }

/// Get the number of bytes that haven't been encoded yet (some of these
/// bytes may have been ran through the match finder though).
static inline uint32 mf_unencoded(const lzma_mf * mf) { return mf->write_pos - mf->read_pos + mf->read_ahead; }

/// Calculate the absolute offset from the beginning of the most recent
/// dictionary reset. Only the lowest four bits are important, so there's no
/// problem that we don't know the 64-bit size of the data encoded so far.
///
/// NOTE: When moving the input window, we need to do it so that the lowest
/// bits of dict->read_pos are not modified to keep this macro working
/// as intended.
static inline uint32 mf_position(const lzma_mf * mf) { return mf->read_pos - mf->read_ahead; }

/// Since everything else begins with mf_, use it also for lzma_mf_find().
#define mf_find lzma_mf_find

/// Skip the given number of bytes. This is used when a good match was found.
/// For example, if mf_find() finds a match of 200 bytes long, the first byte
/// of that match was already consumed by mf_find(), and the rest 199 bytes
/// have to be skipped with mf_skip(mf, 199).
static inline void mf_skip(lzma_mf * mf, uint32 amount)
{
	if(amount != 0) {
		mf->skip(mf, amount);
		mf->read_ahead += amount;
	}
}

/// Copies at most *left number of bytes from the history buffer
/// to out[]. This is needed by LZMA2 to encode uncompressed chunks.
static inline void mf_read(lzma_mf * mf, uint8 * out, size_t * out_pos, size_t out_size, size_t * left)
{
	const size_t out_avail = out_size - *out_pos;
	const size_t copy_size = MIN(out_avail, *left);
	assert(mf->read_ahead == 0);
	assert(mf->read_pos >= *left);
	memcpy(out + *out_pos, mf->buffer + mf->read_pos - *left, copy_size);
	*out_pos += copy_size;
	*left -= copy_size;
}

extern lzma_ret lzma_lz_encoder_init(lzma_next_coder * next, const lzma_allocator * allocator, const lzma_filter_info * filters,
    lzma_ret (* lz_init)(lzma_lz_encoder * lz, const lzma_allocator * allocator, const void * options, lzma_lz_encoder_options * lz_options));
extern uint64 lzma_lz_encoder_memusage(const lzma_lz_encoder_options * lz_options);
// These are only for LZ encoder's internal use.
extern uint32 lzma_mf_find(lzma_mf * mf, uint32 * count, lzma_match * matches);
extern uint32 lzma_mf_hc3_find(lzma_mf * dict, lzma_match * matches);
extern void   lzma_mf_hc3_skip(lzma_mf * dict, uint32 amount);
extern uint32 lzma_mf_hc4_find(lzma_mf * dict, lzma_match * matches);
extern void   lzma_mf_hc4_skip(lzma_mf * dict, uint32 amount);
extern uint32 lzma_mf_bt2_find(lzma_mf * dict, lzma_match * matches);
extern void   lzma_mf_bt2_skip(lzma_mf * dict, uint32 amount);
extern uint32 lzma_mf_bt3_find(lzma_mf * dict, lzma_match * matches);
extern void   lzma_mf_bt3_skip(lzma_mf * dict, uint32 amount);
extern uint32 lzma_mf_bt4_find(lzma_mf * dict, lzma_match * matches);
extern void   lzma_mf_bt4_skip(lzma_mf * dict, uint32 amount);
//
// } lz_encoder
// lz_encoder_hash
//
#if defined(SL_BIGENDIAN) && !defined(HAVE_SMALL)
	// This is to make liblzma produce the same output on big endian
	// systems that it does on little endian systems. lz_encoder.c
	// takes care of including the actual table.
	extern const uint32 lzma_lz_hash_table[256];
	#define hash_table lzma_lz_hash_table
#else
	#define hash_table lzma_crc32_table[0]
#endif

#define HASH_2_SIZE (UINT32_C(1) << 10)
#define HASH_3_SIZE (UINT32_C(1) << 16)
#define HASH_4_SIZE (UINT32_C(1) << 20)

#define HASH_2_MASK (HASH_2_SIZE - 1)
#define HASH_3_MASK (HASH_3_SIZE - 1)
#define HASH_4_MASK (HASH_4_SIZE - 1)

#define FIX_3_HASH_SIZE (HASH_2_SIZE)
#define FIX_4_HASH_SIZE (HASH_2_SIZE + HASH_3_SIZE)
#define FIX_5_HASH_SIZE (HASH_2_SIZE + HASH_3_SIZE + HASH_4_SIZE)

// Endianness doesn't matter in hash_2_calc() (no effect on the output).
#ifdef TUKLIB_FAST_UNALIGNED_ACCESS
	#define hash_2_calc() const uint32 hash_value = read16ne(cur)
#else
	#define hash_2_calc() const uint32 hash_value = (uint32)(cur[0]) | ((uint32)(cur[1]) << 8)
#endif

#define hash_3_calc() \
	const uint32 temp = hash_table[cur[0]] ^ cur[1]; \
	const uint32 hash_2_value = temp & HASH_2_MASK; \
	const uint32 hash_value = (temp ^ ((uint32)(cur[2]) << 8)) & mf->hash_mask

#define hash_4_calc() \
	const uint32 temp = hash_table[cur[0]] ^ cur[1]; \
	const uint32 hash_2_value = temp & HASH_2_MASK; \
	const uint32 hash_3_value = (temp ^ ((uint32)(cur[2]) << 8)) & HASH_3_MASK; \
	const uint32 hash_value = (temp ^ ((uint32)(cur[2]) << 8) ^ (hash_table[cur[3]] << 5)) & mf->hash_mask

// The following are not currently used.

#define hash_5_calc() \
	const uint32 temp = hash_table[cur[0]] ^ cur[1]; \
	const uint32 hash_2_value = temp & HASH_2_MASK; \
	const uint32 hash_3_value = (temp ^ ((uint32)(cur[2]) << 8)) & HASH_3_MASK; \
	uint32 hash_4_value = (temp ^ ((uint32)(cur[2]) << 8) ^ ^ hash_table[cur[3]] << 5); \
	const uint32 hash_value = (hash_4_value ^ (hash_table[cur[4]] << 3)) & mf->hash_mask; \
	hash_4_value &= HASH_4_MASK

/*
#define hash_zip_calc() \
	const uint32 hash_value \
			= (((uint32)(cur[0]) | ((uint32)(cur[1]) << 8)) \
				^ hash_table[cur[2]]) & 0xFFFF
*/

#define hash_zip_calc() const uint32 hash_value = (((uint32)(cur[2]) | ((uint32)(cur[0]) << 8)) ^ hash_table[cur[1]]) & 0xFFFF
#define mt_hash_2_calc() const uint32 hash_2_value = (hash_table[cur[0]] ^ cur[1]) & HASH_2_MASK

#define mt_hash_3_calc() \
	const uint32 temp = hash_table[cur[0]] ^ cur[1]; \
	const uint32 hash_2_value = temp & HASH_2_MASK; \
	const uint32 hash_3_value = (temp ^ ((uint32)(cur[2]) << 8)) & HASH_3_MASK

#define mt_hash_4_calc() \
	const uint32 temp = hash_table[cur[0]] ^ cur[1]; \
	const uint32 hash_2_value = temp & HASH_2_MASK; \
	const uint32 hash_3_value = (temp ^ ((uint32)(cur[2]) << 8)) & HASH_3_MASK; \
	const uint32 hash_4_value = (temp ^ ((uint32)(cur[2]) << 8) ^ (hash_table[cur[3]] << 5)) & HASH_4_MASK
//
// } lz_encoder_hash
// lz_decoder {
//
struct lzma_dict {
	/// Pointer to the dictionary buffer. It can be an allocated buffer
	/// internal to liblzma, or it can a be a buffer given by the
	/// application when in single-call mode (not implemented yet).
	uint8 * buf;
	size_t pos; /// Write position in dictionary. The next byte will be written to buf[pos].
	/// Indicates how full the dictionary is. This is used by
	/// dict_is_distance_valid() to detect corrupt files that would
	/// read beyond the beginning of the dictionary.
	size_t full;
	size_t limit; /// Write limit
	size_t size; /// Size of the dictionary
	bool need_reset; /// True when dictionary should be reset before decoding more data.
};

struct lzma_lz_decoder_options { // @sobolev lzma_lz_options-->lzma_lz_decoder_options
	size_t dict_size;
	const uint8 * preset_dict;
	size_t preset_dict_size;
};

struct lzma_lz_decoder {
	void   SetDefault()
	{
		coder = 0;
		code = 0;
		reset = 0;
		set_uncompressed = 0;
		end = 0;
	}
	void * coder; /// Data specific to the LZ-based decoder
	/// Function to decode from in[] to *dict
	lzma_ret (* code)(void * coder, lzma_dict * dict, const uint8 * in, size_t * in_pos, size_t in_size);
	void (* reset)(void * coder, const void * options);
	/// Set the uncompressed size
	void (* set_uncompressed)(void * coder, lzma_vli uncompressed_size);
	/// Free allocated resources
	void (* end)(void * coder, const lzma_allocator * allocator);
};

#define LZMA_LZ_DECODER_INIT (lzma_lz_decoder){ .coder = NULL, .code = NULL, .reset = NULL, .set_uncompressed = NULL, .end = NULL, }

extern lzma_ret lzma_lz_decoder_init(lzma_next_coder * next, const lzma_allocator * allocator,
    const lzma_filter_info * filters, lzma_ret (*lz_init)(lzma_lz_decoder * lz, const lzma_allocator * allocator, const void * options,
    lzma_lz_decoder_options * lz_options));
extern uint64_t lzma_lz_decoder_memusage(size_t dictionary_size);
extern void lzma_lz_decoder_uncompressed(void * coder, lzma_vli uncompressed_size);
//
// Inline functions
//
/// Get a byte from the history buffer.
static inline uint8 dict_get(const lzma_dict * const dict, const uint32 distance) { return dict->buf[dict->pos - distance - 1 + (distance < dict->pos ? 0 : dict->size)]; }
/// Test if dictionary is empty.
static inline bool dict_is_empty(const lzma_dict * const dict) { return dict->full == 0; }
/// Validate the match distance
static inline bool dict_is_distance_valid(const lzma_dict * const dict, const size_t distance) { return dict->full > distance; }

/// Repeat *len bytes at distance.
static inline bool dict_repeat(lzma_dict * dict, uint32 distance, uint32 * len)
{
	// Don't write past the end of the dictionary.
	const size_t dict_avail = dict->limit - dict->pos;
	uint32 left = MIN(dict_avail, static_cast<size_t>(*len));
	*len -= left;
	// Repeat a block of data from the history. Because memcpy() is faster
	// than copying byte by byte in a loop, the copying process gets split
	// into three cases.
	if(distance < left) {
		// Source and target areas overlap, thus we can't use
		// memcpy() nor even memmove() safely.
		do {
			dict->buf[dict->pos] = dict_get(dict, distance);
			++dict->pos;
		} while(--left > 0);
	}
	else if(distance < dict->pos) {
		// The easiest and fastest case
		memcpy(dict->buf + dict->pos, dict->buf + dict->pos - distance - 1, left);
		dict->pos += left;
	}
	else {
		// The bigger the dictionary, the more rare this
		// case occurs. We need to "wrap" the dict, thus
		// we might need two memcpy() to copy all the data.
		assert(dict->full == dict->size);
		const uint32 copy_pos = dict->pos - distance - 1 + dict->size;
		uint32 copy_size = dict->size - copy_pos;
		if(copy_size < left) {
			memmove(dict->buf + dict->pos, dict->buf + copy_pos, copy_size);
			dict->pos += copy_size;
			copy_size = left - copy_size;
			memcpy(dict->buf + dict->pos, dict->buf, copy_size);
			dict->pos += copy_size;
		}
		else {
			memmove(dict->buf + dict->pos, dict->buf + copy_pos, left);
			dict->pos += left;
		}
	}
	// Update how full the dictionary is.
	if(dict->full < dict->pos)
		dict->full = dict->pos;
	return UNLIKELY(*len != 0);
}

/// Puts one byte into the dictionary. Returns true if the dictionary was
/// already full and the byte couldn't be added.
static inline bool dict_put(lzma_dict * dict, uint8 byte)
{
	if(UNLIKELY(dict->pos == dict->limit))
		return true;
	dict->buf[dict->pos++] = byte;
	if(dict->pos > dict->full)
		dict->full = dict->pos;
	return false;
}

/// Copies arbitrary amount of data into the dictionary.
static inline void dict_write(lzma_dict * dict, const uint8 * in, size_t * in_pos, size_t in_size, size_t * left)
{
	// NOTE: If we are being given more data than the size of the
	// dictionary, it could be possible to optimize the LZ decoder
	// so that not everything needs to go through the dictionary.
	// This shouldn't be very common thing in practice though, and
	// the slowdown of one extra memcpy() isn't bad compared to how
	// much time it would have taken if the data were compressed.
	if(in_size - *in_pos > *left)
		in_size = *in_pos + *left;
	*left -= lzma_bufcpy(in, in_pos, in_size, dict->buf, &dict->pos, dict->limit);
	if(dict->pos > dict->full)
		dict->full = dict->pos;
}

static inline void dict_reset(lzma_dict * dict)
{
	dict->need_reset = true;
}
//
// } lz_decoder
// 
#endif LZMA_LZ_INTERNAL_H