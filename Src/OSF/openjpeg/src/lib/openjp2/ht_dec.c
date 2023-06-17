// ht_dec.c
// This software is released under the 2-Clause BSD license, included below.
//
// Copyright (c) 2021, Aous Naman
// Copyright (c) 2021, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2021, The University of New South Wales, Australia
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// This file is part of the OpenJpeg software implementation.
// Author: Aous Naman
// Date: 01 September 2021
//
// @brief implements HTJ2K block decoder
//
#include "opj_includes.h"
#pragma hdrstop
#include "t1_ht_luts.h"
//
// compiler detection
//
#ifdef _MSC_VER
#define OPJ_COMPILER_MSVC
#elif (defined __GNUC__)
#define OPJ_COMPILER_GNUC
#endif

//************************************************************************/
/** @brief Displays the error message for disabling the decoding of SPP and
 * MRP passes
 */
static boolint only_cleanup_pass_is_decoded = FALSE;

//************************************************************************/
/** @brief Generates population count (i.e., the number of set bits)
 *
 *   @param [in]  val is the value for which population count is sought
 */
static INLINE uint32_t population_count(uint32_t val)
{
#ifdef OPJ_COMPILER_MSVC
	return (uint32_t)__popcnt(val);
#elif (defined OPJ_COMPILER_GNUC)
	return (uint32_t)__builtin_popcount(val);
#else
	val -= ((val >> 1) & 0x55555555);
	val = (((val >> 2) & 0x33333333) + (val & 0x33333333));
	val = (((val >> 4) + val) & 0x0f0f0f0f);
	val += (val >> 8);
	val += (val >> 16);
	return (uint32_t)(val & 0x0000003f);
#endif
}

//************************************************************************/
/** @brief Counts the number of leading zeros
 *
 *   @param [in]  val is the value for which leading zero count is sought
 */
#ifdef OPJ_COMPILER_MSVC
#pragma intrinsic(_BitScanReverse)
#endif
static INLINE uint32_t count_leading_zeros(uint32_t val)
{
#ifdef OPJ_COMPILER_MSVC
	unsigned long result = 0;
	_BitScanReverse(&result, val);
	return 31U ^ (uint32_t)result;
#elif (defined OPJ_COMPILER_GNUC)
	return (uint32_t)__builtin_clz(val);
#else
	val |= (val >> 1);
	val |= (val >> 2);
	val |= (val >> 4);
	val |= (val >> 8);
	val |= (val >> 16);
	return 32U - population_count(val);
#endif
}

//************************************************************************/
/** @brief Read a little-endian serialized UINT32.
 *
 *   @param [in]  dataIn pointer to byte stream to read from
 */
static INLINE uint32_t read_le_uint32(const void* dataIn)
{
#if defined(OPJ_BIG_ENDIAN)
	const uint8* data = (const uint8 *)dataIn;
	return ((uint32_t)data[0]) | (uint32_t)(data[1] << 8) | (uint32_t)(
		data[2] << 16) | (((
		       uint32_t)data[3]) <<
		24U);
#else
	return *(uint32_t*)dataIn;
#endif
}

//************************************************************************/
/** @brief MEL state structure for reading and decoding the MEL bitstream
 *
 *  A number of events is decoded from the MEL bitstream ahead of time
 *  and stored in run/num_runs.
 *  Each run represents the number of zero events before a one event.
 */
typedef struct dec_mel {
	// data decoding machinery
	uint8* data; //!<the address of data (or bitstream)
	uint64 tmp; //!<temporary buffer for read data
	int bits;     //!<number of bits stored in tmp
	int size;     //!<number of bytes in MEL code
	boolint unstuff; //!<true if the next bit needs to be unstuffed
	int k;        //!<state of MEL decoder

	// queue of decoded runs
	int num_runs; //!<number of decoded runs left in runs (maximum 8)
	uint64 runs; //!<runs of decoded MEL codewords (7 bits/run)
} dec_mel_t;

//************************************************************************/
/** @brief Reads and unstuffs the MEL bitstream
 *
 *  This design needs more bytes in the codeblock buffer than the length
 *  of the cleanup pass by up to 2 bytes.
 *
 *  Unstuffing removes the MSB of the byte following a byte whose
 *  value is 0xFF; this prevents sequences larger than 0xFF7F in value
 *  from appearing the bitstream.
 *
 *  @param [in]  melp is a pointer to dec_mel_t structure
 */
static INLINE void mel_read(dec_mel_t * melp)
{
	uint32_t val;
	int bits;
	uint32_t t;
	boolint unstuff;
	if(melp->bits > 32) { //there are enough bits in the tmp variable
		return; // return without reading new data
	}

	val = 0xFFFFFFFF;  // feed in 0xFF if buffer is exhausted
	if(melp->size > 4) { // if there is more than 4 bytes the MEL segment
		val = read_le_uint32(melp->data); // read 32 bits from MEL data
		melp->data += 4;   // advance pointer
		melp->size -= 4;   // reduce counter
	}
	else if(melp->size > 0) { // 4 or less
		uint32_t m, v;
		int i = 0;
		while(melp->size > 1) {
			uint32_t v = *melp->data++; // read one byte at a time
			uint32_t m = ~(0xFFu << i); // mask of location
			val = (val & m) | (v << i); // put byte in its correct location
			--melp->size;
			i += 8;
		}
		// size equal to 1
		v = *melp->data++; // the one before the last is different
		v |= 0xF;                 // MEL and VLC segments can overlap
		m = ~(0xFFu << i);
		val = (val & m) | (v << i);
		--melp->size;
	}

	// next we unstuff them before adding them to the buffer
	bits = 32 - melp->unstuff;  // number of bits in val, subtract 1 if
	// the previously read byte requires
	// unstuffing

	// data is unstuffed and accumulated in t
	// bits has the number of bits in t
	t = val & 0xFF;
	unstuff = ((val & 0xFF) == 0xFF); // true if the byte needs unstuffing
	bits -= unstuff; // there is one less bit in t if unstuffing is needed
	t = t << (8 - unstuff); // move up to make room for the next byte

	//this is a repeat of the above
	t |= (val >> 8) & 0xFF;
	unstuff = (((val >> 8) & 0xFF) == 0xFF);
	bits -= unstuff;
	t = t << (8 - unstuff);

	t |= (val >> 16) & 0xFF;
	unstuff = (((val >> 16) & 0xFF) == 0xFF);
	bits -= unstuff;
	t = t << (8 - unstuff);

	t |= (val >> 24) & 0xFF;
	melp->unstuff = (((val >> 24) & 0xFF) == 0xFF);

	// move t to tmp, and push the result all the way up, so we read from
	// the MSB
	melp->tmp |= ((uint64)t) << (64 - bits - melp->bits);
	melp->bits += bits; //increment the number of bits in tmp
}

//************************************************************************/
/** @brief Decodes unstuffed MEL segment bits stored in tmp to runs
 *
 *  Runs are stored in "runs" and the number of runs in "num_runs".
 *  Each run represents a number of zero events that may or may not
 *  terminate in a 1 event.
 *  Each run is stored in 7 bits.  The LSB is 1 if the run terminates in
 *  a 1 event, 0 otherwise.  The next 6 bits, for the case terminating
 *  with 1, contain the number of consecutive 0 zero events * 2; for the
 *  case terminating with 0, they store (number of consecutive 0 zero
 *  events - 1) * 2.
 *  A total of 6 bits (made up of 1 + 5) should have been enough.
 *
 *  @param [in]  melp is a pointer to dec_mel_t structure
 */
static INLINE void mel_decode(dec_mel_t * melp)
{
	static const int mel_exp[13] = { //MEL exponents
		0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5
	};

	if(melp->bits < 6) { // if there are less than 6 bits in tmp
		mel_read(melp); // then read from the MEL bitstream
	}
	// 6 bits is the largest decodable MEL cwd

	//repeat so long that there is enough decodable bits in tmp,
	// and the runs store is not full (num_runs < 8)
	while(melp->bits >= 6 && melp->num_runs < 8) {
		int eval = mel_exp[melp->k]; // number of bits associated with state
		int run = 0;
		if(melp->tmp & (1ull << 63)) { //The next bit to decode (stored in MSB)
			//one is found
			run = 1 << eval;
			run--; // consecutive runs of 0 events - 1
			melp->k = melp->k + 1 < 12 ? melp->k + 1 : 12;//increment, max is 12
			melp->tmp <<= 1; // consume one bit from tmp
			melp->bits -= 1;
			run = run << 1; // a stretch of zeros not terminating in one
		}
		else {
			//0 is found
			run = (int)(melp->tmp >> (63 - eval)) & ((1 << eval) - 1);
			melp->k = melp->k - 1 > 0 ? melp->k - 1 : 0; //decrement, min is 0
			melp->tmp <<= eval + 1; //consume eval + 1 bits (max is 6)
			melp->bits -= eval + 1;
			run = (run << 1) + 1; // a stretch of zeros terminating with one
		}
		eval = melp->num_runs * 7;         // 7 bits per run
		melp->runs &= ~((uint64)0x3F << eval); // 6 bits are sufficient
		melp->runs |= ((uint64)run) << eval; // store the value in runs
		melp->num_runs++;                  // increment count
	}
}

//************************************************************************/
/** @brief Initiates a dec_mel_t structure for MEL decoding and reads
 *         some bytes in order to get the read address to a multiple
 *         of 4
 *
 *  @param [in]  melp is a pointer to dec_mel_t structure
 *  @param [in]  bbuf is a pointer to byte buffer
 *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
 *  @param [in]  scup is the length of MEL+VLC segments
 */
static INLINE void mel_init(dec_mel_t * melp, uint8* bbuf, int lcup, int scup)
{
	int num;
	int i;
	melp->data = bbuf + lcup - scup; // move the pointer to the start of MEL
	melp->bits = 0;              // 0 bits in tmp
	melp->tmp = 0;               //
	melp->unstuff = FALSE;   // no unstuffing
	melp->size = scup - 1;       // size is the length of MEL+VLC-1
	melp->k = 0;                 // 0 for state
	melp->num_runs = 0;          // num_runs is 0
	melp->runs = 0;              //

	//This code is borrowed; original is for a different architecture
	//These few lines take care of the case where data is not at a multiple
	// of 4 boundary.  It reads 1,2,3 up to 4 bytes from the MEL segment
	num = 4 - (int)((intptr_t)(melp->data) & 0x3);
	for(i = 0; i < num; ++i) { // this code is similar to mel_read
		uint64 d;
		int d_bits;

		assert(melp->unstuff == FALSE || melp->data[0] <= 0x8F);
		d = (melp->size > 0) ? *melp->data : 0xFF; // if buffer is consumed
		// set data to 0xFF
		if(melp->size == 1) {
			d |= 0xF; //if this is MEL+VLC-1, set LSBs to 0xF
		}
		// see the standard
		melp->data += melp->size-- > 0; //increment if the end is not reached
		d_bits = 8 - melp->unstuff; //if unstuffing is needed, reduce by 1
		melp->tmp = (melp->tmp << d_bits) | d; //store bits in tmp
		melp->bits += d_bits; //increment tmp by number of bits
		melp->unstuff = ((d & 0xFF) == 0xFF); //true of next byte needs
		//unstuffing
	}
	melp->tmp <<= (64 - melp->bits); //push all the way up so the first bit
	// is the MSB
}

//************************************************************************/
/** @brief Retrieves one run from dec_mel_t; if there are no runs stored
 *         MEL segment is decoded
 *
 * @param [in]  melp is a pointer to dec_mel_t structure
 */
static INLINE int mel_get_run(dec_mel_t * melp)
{
	int t;
	if(melp->num_runs == 0) { //if no runs, decode more bit from MEL segment
		mel_decode(melp);
	}
	t = melp->runs & 0x7F; //retrieve one run
	melp->runs >>= 7; // remove the retrieved run
	melp->num_runs--;
	return t; // return run
}

//************************************************************************/
/** @brief A structure for reading and unstuffing a segment that grows
 *         backward, such as VLC and MRP
 */
typedef struct rev_struct {
	//storage
	uint8* data; //!<pointer to where to read data
	uint64 tmp; //!<temporary buffer of read data
	uint32_t bits; //!<number of bits stored in tmp
	int size;     //!<number of bytes left
	boolint unstuff; //!<true if the last byte is more than 0x8F
	//!<then the current byte is unstuffed if it is 0x7F
} rev_struct_t;

//************************************************************************/
/** @brief Read and unstuff data from a backwardly-growing segment
 *
 *  This reader can read up to 8 bytes from before the VLC segment.
 *  Care must be taken not read from unreadable memory, causing a
 *  segmentation fault.
 *
 *  Note that there is another subroutine rev_read_mrp that is slightly
 *  different.  The other one fills zeros when the buffer is exhausted.
 *  This one basically does not care if the bytes are consumed, because
 *  any extra data should not be used in the actual decoding.
 *
 *  Unstuffing is needed to prevent sequences more than 0xFF8F from
 *  appearing in the bits stream; since we are reading backward, we keep
 *  watch when a value larger than 0x8F appears in the bitstream.
 *  If the byte following this is 0x7F, we unstuff this byte (ignore the
 *  MSB of that byte, which should be 0).
 *
 *  @param [in]  vlcp is a pointer to rev_struct_t structure
 */
static INLINE void rev_read(rev_struct_t * vlcp)
{
	uint32_t val;
	uint32_t tmp;
	uint32_t bits;
	boolint unstuff;
	//process 4 bytes at a time
	if(vlcp->bits > 32) { // if there are more than 32 bits in tmp, then
		return; // reading 32 bits can overflow vlcp->tmp
	}
	val = 0;
	//the next line (the if statement) needs to be tested first
	if(vlcp->size > 3) { // if there are more than 3 bytes left in VLC
		// (vlcp->data - 3) move pointer back to read 32 bits at once
		val = read_le_uint32(vlcp->data - 3); // then read 32 bits
		vlcp->data -= 4;        // move data pointer back by 4
		vlcp->size -= 4;        // reduce available byte by 4
	}
	else if(vlcp->size > 0) { // 4 or less
		int i = 24;
		while(vlcp->size > 0) {
			uint32_t v = *vlcp->data--; // read one byte at a time
			val |= (v << i);  // put byte in its correct location
			--vlcp->size;
			i -= 8;
		}
	}

	//accumulate in tmp, number of bits in tmp are stored in bits
	tmp = val >> 24; //start with the MSB byte

	// test unstuff (previous byte is >0x8F), and this byte is 0x7F
	bits = 8u - ((vlcp->unstuff && (((val >> 24) & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = (val >> 24) > 0x8F; //this is for the next byte

	tmp |= ((val >> 16) & 0xFF) << bits; //process the next byte
	bits += 8u - ((unstuff && (((val >> 16) & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = ((val >> 16) & 0xFF) > 0x8F;

	tmp |= ((val >> 8) & 0xFF) << bits;
	bits += 8u - ((unstuff && (((val >> 8) & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = ((val >> 8) & 0xFF) > 0x8F;

	tmp |= (val & 0xFF) << bits;
	bits += 8u - ((unstuff && ((val & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = (val & 0xFF) > 0x8F;

	// now move the read and unstuffed bits into vlcp->tmp
	vlcp->tmp |= (uint64)tmp << vlcp->bits;
	vlcp->bits += bits;
	vlcp->unstuff = unstuff; // this for the next read
}

//************************************************************************/
/** @brief Initiates the rev_struct_t structure and reads a few bytes to
 *         move the read address to multiple of 4
 *
 *  There is another similar rev_init_mrp subroutine.  The difference is
 *  that this one, rev_init, discards the first 12 bits (they have the
 *  sum of the lengths of VLC and MEL segments), and first unstuff depends
 *  on first 4 bits.
 *
 *  @param [in]  vlcp is a pointer to rev_struct_t structure
 *  @param [in]  data is a pointer to byte at the start of the cleanup pass
 *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
 *  @param [in]  scup is the length of MEL+VLC segments
 */
static INLINE void rev_init(rev_struct_t * vlcp, uint8* data, int lcup, int scup)
{
	uint32_t d;
	int num, tnum, i;
	//first byte has only the upper 4 bits
	vlcp->data = data + lcup - 2;
	//size can not be larger than this, in fact it should be smaller
	vlcp->size = scup - 2;
	d = *vlcp->data--;        // read one byte (this is a half byte)
	vlcp->tmp = d >> 4;       // both initialize and set
	vlcp->bits = 4 - ((vlcp->tmp & 7) == 7); //check standard
	vlcp->unstuff = (d | 0xF) > 0x8F; //this is useful for the next byte

	//This code is designed for an architecture that read address should
	// align to the read size (address multiple of 4 if read size is 4)
	//These few lines take care of the case where data is not at a multiple
	// of 4 boundary. It reads 1,2,3 up to 4 bytes from the VLC bitstream.
	// To read 32 bits, read from (vlcp->data - 3)
	num = 1 + (int)((intptr_t)(vlcp->data) & 0x3);
	tnum = num < vlcp->size ? num : vlcp->size;
	for(i = 0; i < tnum; ++i) {
		uint64 d;
		uint32_t d_bits;
		d = *vlcp->data--; // read one byte and move read pointer
		//check if the last byte was >0x8F (unstuff == true) and this is 0x7F
		d_bits = 8u - ((vlcp->unstuff && ((d & 0x7F) == 0x7F)) ? 1u : 0u);
		vlcp->tmp |= d << vlcp->bits; // move data to vlcp->tmp
		vlcp->bits += d_bits;
		vlcp->unstuff = d > 0x8F; // for next byte
	}
	vlcp->size -= tnum;
	rev_read(vlcp); // read another 32 buts
}

//************************************************************************/
/** @brief Retrieves 32 bits from the head of a rev_struct structure
 *
 *  By the end of this call, vlcp->tmp must have no less than 33 bits
 *
 *  @param [in]  vlcp is a pointer to rev_struct structure
 */
static INLINE uint32_t rev_fetch(rev_struct_t * vlcp)
{
	if(vlcp->bits < 32) { // if there are less then 32 bits, read more
		rev_read(vlcp); // read 32 bits, but unstuffing might reduce this
		if(vlcp->bits < 32) { // if there is still space in vlcp->tmp for 32 bits
			rev_read(vlcp); // read another 32
		}
	}
	return (uint32_t)vlcp->tmp; // return the head (bottom-most) of vlcp->tmp
}

//************************************************************************/
/** @brief Consumes num_bits from a rev_struct structure
 *
 *  @param [in]  vlcp is a pointer to rev_struct structure
 *  @param [in]  num_bits is the number of bits to be removed
 */
static INLINE uint32_t rev_advance(rev_struct_t * vlcp, uint32_t num_bits)
{
	assert(num_bits <= vlcp->bits); // vlcp->tmp must have more than num_bits
	vlcp->tmp >>= num_bits;     // remove bits
	vlcp->bits -= num_bits;     // decrement the number of bits
	return (uint32_t)vlcp->tmp;
}

//************************************************************************/
/** @brief Reads and unstuffs from rev_struct
 *
 *  This is different than rev_read in that this fills in zeros when the
 *  the available data is consumed.  The other does not care about the
 *  values when all data is consumed.
 *
 *  See rev_read for more information about unstuffing
 *
 *  @param [in]  mrp is a pointer to rev_struct structure
 */
static INLINE void rev_read_mrp(rev_struct_t * mrp)
{
	uint32_t val;
	uint32_t tmp;
	uint32_t bits;
	boolint unstuff;
	//process 4 bytes at a time
	if(mrp->bits > 32) {
		return;
	}
	val = 0;
	if(mrp->size > 3) { // If there are 3 byte or more
		// (mrp->data - 3) move pointer back to read 32 bits at once
		val = read_le_uint32(mrp->data - 3); // read 32 bits
		mrp->data -= 4;              // move back pointer
		mrp->size -= 4;              // reduce count
	}
	else if(mrp->size > 0) {
		int i = 24;
		while(mrp->size > 0) {
			uint32_t v = *mrp->data--; // read one byte at a time
			val |= (v << i); // put byte in its correct location
			--mrp->size;
			i -= 8;
		}
	}

	//accumulate in tmp, and keep count in bits
	tmp = val >> 24;

	//test if the last byte > 0x8F (unstuff must be true) and this is 0x7F
	bits = 8u - ((mrp->unstuff && (((val >> 24) & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = (val >> 24) > 0x8F;

	//process the next byte
	tmp |= ((val >> 16) & 0xFF) << bits;
	bits += 8u - ((unstuff && (((val >> 16) & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = ((val >> 16) & 0xFF) > 0x8F;

	tmp |= ((val >> 8) & 0xFF) << bits;
	bits += 8u - ((unstuff && (((val >> 8) & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = ((val >> 8) & 0xFF) > 0x8F;

	tmp |= (val & 0xFF) << bits;
	bits += 8u - ((unstuff && ((val & 0x7F) == 0x7F)) ? 1u : 0u);
	unstuff = (val & 0xFF) > 0x8F;

	mrp->tmp |= (uint64)tmp << mrp->bits; // move data to mrp pointer
	mrp->bits += bits;
	mrp->unstuff = unstuff;               // next byte
}

//************************************************************************/
/** @brief Initialized rev_struct structure for MRP segment, and reads
 *         a number of bytes such that the next 32 bits read are from
 *         an address that is a multiple of 4. Note this is designed for
 *         an architecture that read size must be compatible with the
 *         alignment of the read address
 *
 *  There is another similar subroutine rev_init.  This subroutine does
 *  NOT skip the first 12 bits, and starts with unstuff set to true.
 *
 *  @param [in]  mrp is a pointer to rev_struct structure
 *  @param [in]  data is a pointer to byte at the start of the cleanup pass
 *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
 *  @param [in]  len2 is the length of SPP+MRP segments
 */
static INLINE void rev_init_mrp(rev_struct_t * mrp, uint8* data, int lcup, int len2)
{
	int num, i;
	mrp->data = data + lcup + len2 - 1;
	mrp->size = len2;
	mrp->unstuff = TRUE;
	mrp->bits = 0;
	mrp->tmp = 0;
	//This code is designed for an architecture that read address should
	// align to the read size (address multiple of 4 if read size is 4)
	//These few lines take care of the case where data is not at a multiple
	// of 4 boundary.  It reads 1,2,3 up to 4 bytes from the MRP stream
	num = 1 + (int)((intptr_t)(mrp->data) & 0x3);
	for(i = 0; i < num; ++i) {
		uint64 d;
		uint32_t d_bits;

		//read a byte, 0 if no more data
		d = (mrp->size-- > 0) ? *mrp->data-- : 0;
		//check if unstuffing is needed
		d_bits = 8u - ((mrp->unstuff && ((d & 0x7F) == 0x7F)) ? 1u : 0u);
		mrp->tmp |= d << mrp->bits; // move data to vlcp->tmp
		mrp->bits += d_bits;
		mrp->unstuff = d > 0x8F; // for next byte
	}
	rev_read_mrp(mrp);
}

//************************************************************************/
/** @brief Retrieves 32 bits from the head of a rev_struct structure
 *
 *  By the end of this call, mrp->tmp must have no less than 33 bits
 *
 *  @param [in]  mrp is a pointer to rev_struct structure
 */
static INLINE uint32_t rev_fetch_mrp(rev_struct_t * mrp)
{
	if(mrp->bits < 32) { // if there are less than 32 bits in mrp->tmp
		rev_read_mrp(mrp); // read 30-32 bits from mrp
		if(mrp->bits < 32) { // if there is a space of 32 bits
			rev_read_mrp(mrp); // read more
		}
	}
	return (uint32_t)mrp->tmp; // return the head of mrp->tmp
}

//************************************************************************/
/** @brief Consumes num_bits from a rev_struct structure
 *
 *  @param [in]  mrp is a pointer to rev_struct structure
 *  @param [in]  num_bits is the number of bits to be removed
 */
static INLINE uint32_t rev_advance_mrp(rev_struct_t * mrp, uint32_t num_bits)
{
	assert(num_bits <= mrp->bits); // we must not consume more than mrp->bits
	mrp->tmp >>= num_bits;     // discard the lowest num_bits bits
	mrp->bits -= num_bits;
	return (uint32_t)mrp->tmp; // return data after consumption
}

//************************************************************************/
/** @brief Decode initial UVLC to get the u value (or u_q)
 *
 *  @param [in]  vlc is the head of the VLC bitstream
 *  @param [in]  mode is 0, 1, 2, 3, or 4. Values in 0 to 3 are composed of
 *               u_off of 1st quad and 2nd quad of a quad pair.  The value
 *               4 occurs when both bits are 1, and the event decoded
 *               from MEL bitstream is also 1.
 *  @param [out] u is the u value (or u_q) + 1.  Note: we produce u + 1;
 *               this value is a partial calculation of u + kappa.
 */
static INLINE uint32_t decode_init_uvlc(uint32_t vlc, uint32_t mode, uint32_t * u)
{
	//table stores possible decoding three bits from vlc
	// there are 8 entries for xx1, x10, 100, 000, where x means do not care
	// table value is made up of
	// 2 bits in the LSB for prefix length
	// 3 bits for suffix length
	// 3 bits in the MSB for prefix value (u_pfx in Table 3 of ITU T.814)
	static const uint8 dec[8] = { // the index is the prefix codeword
		3 | (5 << 2) | (5 << 5), //000 == 000, prefix codeword "000"
		1 | (0 << 2) | (1 << 5), //001 == xx1, prefix codeword "1"
		2 | (0 << 2) | (2 << 5), //010 == x10, prefix codeword "01"
		1 | (0 << 2) | (1 << 5), //011 == xx1, prefix codeword "1"
		3 | (1 << 2) | (3 << 5), //100 == 100, prefix codeword "001"
		1 | (0 << 2) | (1 << 5), //101 == xx1, prefix codeword "1"
		2 | (0 << 2) | (2 << 5), //110 == x10, prefix codeword "01"
		1 | (0 << 2) | (1 << 5) //111 == xx1, prefix codeword "1"
	};

	uint32_t consumed_bits = 0;
	if(mode == 0) { // both u_off are 0
		u[0] = u[1] = 1; //Kappa is 1 for initial line
	}
	else if(mode <= 2) { // u_off are either 01 or 10
		uint32_t d;
		uint32_t suffix_len;

		d = dec[vlc & 0x7]; //look at the least significant 3 bits
		vlc >>= d & 0x3;         //prefix length
		consumed_bits += d & 0x3;

		suffix_len = ((d >> 2) & 0x7);
		consumed_bits += suffix_len;

		d = (d >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
		u[0] = (mode == 1) ? d + 1 : 1; // kappa is 1 for initial line
		u[1] = (mode == 1) ? 1 : d + 1; // kappa is 1 for initial line
	}
	else if(mode == 3) { // both u_off are 1, and MEL event is 0
		uint32_t d1 = dec[vlc & 0x7]; // LSBs of VLC are prefix codeword
		vlc >>= d1 & 0x3;        // Consume bits
		consumed_bits += d1 & 0x3;

		if((d1 & 0x3) > 2) {
			uint32_t suffix_len;

			//u_{q_2} prefix
			u[1] = (vlc & 1) + 1 + 1; //Kappa is 1 for initial line
			++consumed_bits;
			vlc >>= 1;

			suffix_len = ((d1 >> 2) & 0x7);
			consumed_bits += suffix_len;
			d1 = (d1 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
			u[0] = d1 + 1; //Kappa is 1 for initial line
		}
		else {
			uint32_t d2;
			uint32_t suffix_len;

			d2 = dec[vlc & 0x7]; // LSBs of VLC are prefix codeword
			vlc >>= d2 & 0x3;    // Consume bits
			consumed_bits += d2 & 0x3;

			suffix_len = ((d1 >> 2) & 0x7);
			consumed_bits += suffix_len;

			d1 = (d1 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
			u[0] = d1 + 1; //Kappa is 1 for initial line
			vlc >>= suffix_len;

			suffix_len = ((d2 >> 2) & 0x7);
			consumed_bits += suffix_len;

			d2 = (d2 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
			u[1] = d2 + 1; //Kappa is 1 for initial line
		}
	}
	else if(mode == 4) { // both u_off are 1, and MEL event is 1
		uint32_t d1;
		uint32_t d2;
		uint32_t suffix_len;

		d1 = dec[vlc & 0x7]; // LSBs of VLC are prefix codeword
		vlc >>= d1 & 0x3;        // Consume bits
		consumed_bits += d1 & 0x3;

		d2 = dec[vlc & 0x7]; // LSBs of VLC are prefix codeword
		vlc >>= d2 & 0x3;        // Consume bits
		consumed_bits += d2 & 0x3;

		suffix_len = ((d1 >> 2) & 0x7);
		consumed_bits += suffix_len;

		d1 = (d1 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
		u[0] = d1 + 3; // add 2+kappa
		vlc >>= suffix_len;

		suffix_len = ((d2 >> 2) & 0x7);
		consumed_bits += suffix_len;

		d2 = (d2 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
		u[1] = d2 + 3; // add 2+kappa
	}
	return consumed_bits;
}

//************************************************************************/
/** @brief Decode non-initial UVLC to get the u value (or u_q)
 *
 *  @param [in]  vlc is the head of the VLC bitstream
 *  @param [in]  mode is 0, 1, 2, or 3. The 1st bit is u_off of 1st quad
 *               and 2nd for 2nd quad of a quad pair
 *  @param [out] u is the u value (or u_q) + 1.  Note: we produce u + 1;
 *               this value is a partial calculation of u + kappa.
 */
static INLINE uint32_t decode_noninit_uvlc(uint32_t vlc, uint32_t mode, uint32_t * u)
{
	//table stores possible decoding three bits from vlc
	// there are 8 entries for xx1, x10, 100, 000, where x means do not care
	// table value is made up of
	// 2 bits in the LSB for prefix length
	// 3 bits for suffix length
	// 3 bits in the MSB for prefix value (u_pfx in Table 3 of ITU T.814)
	static const uint8 dec[8] = {
		3 | (5 << 2) | (5 << 5), //000 == 000, prefix codeword "000"
		1 | (0 << 2) | (1 << 5), //001 == xx1, prefix codeword "1"
		2 | (0 << 2) | (2 << 5), //010 == x10, prefix codeword "01"
		1 | (0 << 2) | (1 << 5), //011 == xx1, prefix codeword "1"
		3 | (1 << 2) | (3 << 5), //100 == 100, prefix codeword "001"
		1 | (0 << 2) | (1 << 5), //101 == xx1, prefix codeword "1"
		2 | (0 << 2) | (2 << 5), //110 == x10, prefix codeword "01"
		1 | (0 << 2) | (1 << 5) //111 == xx1, prefix codeword "1"
	};

	uint32_t consumed_bits = 0;
	if(mode == 0) {
		u[0] = u[1] = 1; //for kappa
	}
	else if(mode <= 2) { //u_off are either 01 or 10
		uint32_t d;
		uint32_t suffix_len;

		d = dec[vlc & 0x7]; //look at the least significant 3 bits
		vlc >>= d & 0x3;        //prefix length
		consumed_bits += d & 0x3;

		suffix_len = ((d >> 2) & 0x7);
		consumed_bits += suffix_len;

		d = (d >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
		u[0] = (mode == 1) ? d + 1 : 1; //for kappa
		u[1] = (mode == 1) ? 1 : d + 1; //for kappa
	}
	else if(mode == 3) { // both u_off are 1
		uint32_t d1;
		uint32_t d2;
		uint32_t suffix_len;

		d1 = dec[vlc & 0x7]; // LSBs of VLC are prefix codeword
		vlc >>= d1 & 0x3;        // Consume bits
		consumed_bits += d1 & 0x3;

		d2 = dec[vlc & 0x7]; // LSBs of VLC are prefix codeword
		vlc >>= d2 & 0x3;        // Consume bits
		consumed_bits += d2 & 0x3;

		suffix_len = ((d1 >> 2) & 0x7);
		consumed_bits += suffix_len;

		d1 = (d1 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
		u[0] = d1 + 1; //1 for kappa
		vlc >>= suffix_len;

		suffix_len = ((d2 >> 2) & 0x7);
		consumed_bits += suffix_len;

		d2 = (d2 >> 5) + (vlc & ((1U << suffix_len) - 1)); // u value
		u[1] = d2 + 1; //1 for kappa
	}
	return consumed_bits;
}

//************************************************************************/
/** @brief State structure for reading and unstuffing of forward-growing
 *         bitstreams; these are: MagSgn and SPP bitstreams
 */
typedef struct frwd_struct {
	const uint8* data; //!<pointer to bitstream
	uint64 tmp;    //!<temporary buffer of read data
	uint32_t bits;   //!<number of bits stored in tmp
	boolint unstuff;  //!<true if a bit needs to be unstuffed from next byte
	int size;          //!<size of data
	uint32_t X;      //!<0 or 0xFF, X's are inserted at end of bitstream
} frwd_struct_t;

//************************************************************************/
/** @brief Read and unstuffs 32 bits from forward-growing bitstream
 *
 *  A subroutine to read from both the MagSgn or SPP bitstreams;
 *  in particular, when MagSgn bitstream is consumed, 0xFF's are fed,
 *  while when SPP is exhausted 0's are fed in.
 *  X controls this value.
 *
 *  Unstuffing prevent sequences that are more than 0xFF7F from appearing
 *  in the conpressed sequence.  So whenever a value of 0xFF is coded, the
 *  MSB of the next byte is set 0 and must be ignored during decoding.
 *
 *  Reading can go beyond the end of buffer by up to 3 bytes.
 *
 *  @param  [in]  msp is a pointer to frwd_struct_t structure
 *
 */
static INLINE void frwd_read(frwd_struct_t * msp)
{
	uint32_t val;
	uint32_t bits;
	uint32_t t;
	boolint unstuff;
	assert(msp->bits <= 32); // assert that there is a space for 32 bits
	val = 0u;
	if(msp->size > 3) {
		val = read_le_uint32(msp->data); // read 32 bits
		msp->data += 4;   // increment pointer
		msp->size -= 4;   // reduce size
	}
	else if(msp->size > 0) {
		int i = 0;
		val = msp->X != 0 ? 0xFFFFFFFFu : 0;
		while(msp->size > 0) {
			uint32_t v = *msp->data++; // read one byte at a time
			uint32_t m = ~(0xFFu << i); // mask of location
			val = (val & m) | (v << i); // put one byte in its correct location
			--msp->size;
			i += 8;
		}
	}
	else {
		val = msp->X != 0 ? 0xFFFFFFFFu : 0;
	}

	// we accumulate in t and keep a count of the number of bits in bits
	bits = 8u - (msp->unstuff ? 1u : 0u);
	t = val & 0xFF;
	unstuff = ((val & 0xFF) == 0xFF); // Do we need unstuffing next?

	t |= ((val >> 8) & 0xFF) << bits;
	bits += 8u - (unstuff ? 1u : 0u);
	unstuff = (((val >> 8) & 0xFF) == 0xFF);

	t |= ((val >> 16) & 0xFF) << bits;
	bits += 8u - (unstuff ? 1u : 0u);
	unstuff = (((val >> 16) & 0xFF) == 0xFF);

	t |= ((val >> 24) & 0xFF) << bits;
	bits += 8u - (unstuff ? 1u : 0u);
	msp->unstuff = (((val >> 24) & 0xFF) == 0xFF); // for next byte

	msp->tmp |= ((uint64)t) << msp->bits; // move data to msp->tmp
	msp->bits += bits;
}

//************************************************************************/
/** @brief Initialize frwd_struct_t struct and reads some bytes
 *
 *  @param [in]  msp is a pointer to frwd_struct_t
 *  @param [in]  data is a pointer to the start of data
 *  @param [in]  size is the number of byte in the bitstream
 *  @param [in]  X is the value fed in when the bitstream is exhausted.
 *               See frwd_read.
 */
static INLINE void frwd_init(frwd_struct_t * msp, const uint8* data, int size, uint32_t X)
{
	int num, i;
	msp->data = data;
	msp->tmp = 0;
	msp->bits = 0;
	msp->unstuff = FALSE;
	msp->size = size;
	msp->X = X;
	assert(msp->X == 0 || msp->X == 0xFF);
	//This code is designed for an architecture that read address should
	// align to the read size (address multiple of 4 if read size is 4)
	//These few lines take care of the case where data is not at a multiple
	// of 4 boundary.  It reads 1,2,3 up to 4 bytes from the bitstream
	num = 4 - (int)((intptr_t)(msp->data) & 0x3);
	for(i = 0; i < num; ++i) {
		uint64 d;
		//read a byte if the buffer is not exhausted, otherwise set it to X
		d = msp->size-- > 0 ? *msp->data++ : msp->X;
		msp->tmp |= (d << msp->bits); // store data in msp->tmp
		msp->bits += 8u - (msp->unstuff ? 1u : 0u); // number of bits added to msp->tmp
		msp->unstuff = ((d & 0xFF) == 0xFF); // unstuffing for next byte
	}
	frwd_read(msp); // read 32 bits more
}

//************************************************************************/
/** @brief Consume num_bits bits from the bitstream of frwd_struct_t
 *
 *  @param [in]  msp is a pointer to frwd_struct_t
 *  @param [in]  num_bits is the number of bit to consume
 */
static INLINE void frwd_advance(frwd_struct_t * msp, uint32_t num_bits)
{
	assert(num_bits <= msp->bits);
	msp->tmp >>= num_bits; // consume num_bits
	msp->bits -= num_bits;
}

//************************************************************************/
/** @brief Fetches 32 bits from the frwd_struct_t bitstream
 *
 *  @param [in]  msp is a pointer to frwd_struct_t
 */
static INLINE uint32_t frwd_fetch(frwd_struct_t * msp)
{
	if(msp->bits < 32) {
		frwd_read(msp);
		if(msp->bits < 32) { //need to test
			frwd_read(msp);
		}
	}
	return (uint32_t)msp->tmp;
}

//************************************************************************/
/** @brief Allocates T1 buffers
 *
 *  @param [in, out]  t1 is codeblock cofficients storage
 *  @param [in]       w is codeblock width
 *  @param [in]       h is codeblock height
 */
static boolint opj_t1_allocate_buffers(opj_t1_t * t1, uint32_t w, uint32_t h)
{
	uint32_t flagssize;
	/* No risk of overflow. Prior checks ensure those assert are met */
	/* They are per the specification */
	assert(w <= 1024);
	assert(h <= 1024);
	assert(w * h <= 4096);
	/* encoder uses tile buffer, so no need to allocate */
	{
		uint32_t datasize = w * h;
		if(datasize > t1->datasize) {
			opj_aligned_free(t1->data);
			t1->data = (int32_t*)opj_aligned_malloc(datasize * sizeof(int32_t));
			if(!t1->data) {
				/* FIXME event manager error callback */
				return FALSE;
			}
			t1->datasize = datasize;
		}
		/* memset first arg is declared to never be null by gcc */
		memzero(t1->data, datasize * sizeof(int32_t));
	}
	// We expand these buffers to multiples of 16 bytes.
	// We need 4 buffers of 129 integers each, expanded to 132 integers each
	// We also need 514 bytes of buffer, expanded to 528 bytes
	flagssize = 132U * sizeof(uint32_t) * 4U; // expanded to multiple of 16
	flagssize += 528U; // 514 expanded to multiples of 16

	{
		if(flagssize > t1->flagssize) {
			opj_aligned_free(t1->flags);
			t1->flags = (opj_flag_t*)opj_aligned_malloc(flagssize);
			if(!t1->flags) {
				/* FIXME event manager error callback */
				return FALSE;
			}
		}
		t1->flagssize = flagssize;
		memzero(t1->flags, flagssize);
	}
	t1->w = w;
	t1->h = h;
	return TRUE;
}

//************************************************************************/
/** @brief Decodes one codeblock, processing the cleanup, siginificance
 *         propagation, and magnitude refinement pass
 *
 *  @param [in, out]  t1 is codeblock cofficients storage
 *  @param [in]       cblk is codeblock properties
 *  @param [in]       orient is the subband to which the codeblock belongs (not needed)
 *  @param [in]       roishift is region of interest shift
 *  @param [in]       cblksty is codeblock style
 *  @param [in]       p_manager is events print manager
 *  @param [in]       p_manager_mutex a mutex to control access to p_manager
 *  @param [in]       check_pterm: check termination (not used)
 */
boolint opj_t1_ht_decode_cblk(opj_t1_t * t1, opj_tcd_cblk_dec_t* cblk, uint32_t orient, uint32_t roishift,
    uint32_t cblksty, opj_event_mgr_t * p_manager, opj_mutex_t* p_manager_mutex, boolint check_pterm)
{
	uint8* cblkdata = NULL;
	uint8* coded_data;
	uint32_t* decoded_data;
	uint32_t zero_bplanes;
	uint32_t num_passes;
	uint32_t lengths1;
	uint32_t lengths2;
	int32_t width;
	int32_t height;
	int32_t stride;
	uint32_t * pflags, * sigma1, * sigma2, * mbr1, * mbr2, * sip, sip_shift;
	uint32_t p;
	uint32_t zero_bplanes_p1;
	int lcup, scup;
	dec_mel_t mel;
	rev_struct_t vlc;
	frwd_struct_t magsgn;
	frwd_struct_t sigprop;
	rev_struct_t magref;
	uint8 * lsp, * line_state;
	int run;
	uint32_t vlc_val;          // fetched data from VLC bitstream
	uint32_t qinf[2];
	uint32_t c_q;
	uint32_t* sp;
	int32_t x, y; // loop indices
	boolint stripe_causal = (cblksty & J2K_CCP_CBLKSTY_VSC) != 0;
	uint32_t cblk_len = 0;

	(void)(orient);  // stops unused parameter message
	(void)(check_pterm); // stops unused parameter message

	// We ignor orient, because the same decoder is used for all subbands
	// We also ignore check_pterm, because I am not sure how it applies
	if(roishift != 0) {
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_ERROR, "We do not support ROI in decoding HT codeblocks\n");
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	if(!opj_t1_allocate_buffers(t1, (uint32_t)(cblk->x1 - cblk->x0), (uint32_t)(cblk->y1 - cblk->y0))) {
		return FALSE;
	}
	if(cblk->Mb == 0) {
		return TRUE;
	}
	/* numbps = Mb + 1 - zero_bplanes, Mb = Kmax, zero_bplanes = missing_msbs */
	zero_bplanes = (cblk->Mb + 1) - cblk->numbps;
	/* Compute whole codeblock length from chunk lengths */
	cblk_len = 0;
	{
		uint32_t i;
		for(i = 0; i < cblk->numchunks; i++) {
			cblk_len += cblk->chunks[i].len;
		}
	}
	if(cblk->numchunks > 1 || t1->mustuse_cblkdatabuffer) {
		uint32_t i;
		/* Allocate temporary memory if needed */
		if(cblk_len > t1->cblkdatabuffersize) {
			cblkdata = (uint8*)opj_realloc(t1->cblkdatabuffer, cblk_len);
			if(cblkdata == NULL) {
				return FALSE;
			}
			t1->cblkdatabuffer = cblkdata;
			t1->cblkdatabuffersize = cblk_len;
		}
		/* Concatenate all chunks */
		cblkdata = t1->cblkdatabuffer;
		cblk_len = 0;
		for(i = 0; i < cblk->numchunks; i++) {
			memcpy(cblkdata + cblk_len, cblk->chunks[i].data, cblk->chunks[i].len);
			cblk_len += cblk->chunks[i].len;
		}
	}
	else if(cblk->numchunks == 1) {
		cblkdata = cblk->chunks[0].data;
	}
	else {
		/* Not sure if that can happen in practice, but avoid Coverity to */
		/* think we will dereference a null cblkdta pointer */
		return TRUE;
	}

	// uint8* coded_data is a pointer to bitstream
	coded_data = cblkdata;
	// uint32_t* decoded_data is a pointer to decoded codeblock data buf.
	decoded_data = (uint32_t*)t1->data;
	// uint32_t num_passes is the number of passes: 1 if CUP only, 2 for
	// CUP+SPP, and 3 for CUP+SPP+MRP
	num_passes = cblk->numsegs > 0 ? cblk->segs[0].real_num_passes : 0;
	num_passes += cblk->numsegs > 1 ? cblk->segs[1].real_num_passes : 0;
	// uint32_t lengths1 is the length of cleanup pass
	lengths1 = num_passes > 0 ? cblk->segs[0].len : 0;
	// uint32_t lengths2 is the length of refinement passes (either SPP only or SPP+MRP)
	lengths2 = num_passes > 1 ? cblk->segs[1].len : 0;
	// int32_t width is the decoded codeblock width
	width = cblk->x1 - cblk->x0;
	// int32_t height is the decoded codeblock height
	height = cblk->y1 - cblk->y0;
	// int32_t stride is the decoded codeblock buffer stride
	stride = width;
	/*  sigma1 and sigma2 contains significant (i.e., non-zero) pixel
	 *  locations.  The buffers are used interchangeably, because we need
	 *  more than 4 rows of significance information at a given time.
	 *  Each 32 bits contain significance information for 4 rows of 8
	 *  columns each.  If we denote 32 bits by 0xaaaaaaaa, the each "a" is
	 *  called a nibble and has significance information for 4 rows.
	 *  The least significant nibble has information for the first column,
	 *  and so on. The nibble's LSB is for the first row, and so on.
	 *  Since, at most, we can have 1024 columns in a quad, we need 128
	 *  entries; we added 1 for convenience when propagation of signifcance
	 *  goes outside the structure
	 *  To work in OpenJPEG these buffers has been expanded to 132.
	 */
	// uint32_t *pflags, *sigma1, *sigma2, *mbr1, *mbr2, *sip, sip_shift;
	pflags = (uint32_t*)t1->flags;
	sigma1 = pflags;
	sigma2 = sigma1 + 132;
	// mbr arrangement is similar to sigma; mbr contains locations
	// that become significant during significance propagation pass
	mbr1 = sigma2 + 132;
	mbr2 = mbr1 + 132;
	//a pointer to sigma
	sip = sigma1; //pointers to arrays to be used interchangeably
	sip_shift = 0; //the amount of shift needed for sigma

	if(num_passes > 1 && lengths2 == 0) {
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_WARNING, "A malformed codeblock that has more than one coding pass, but zero length for 2nd and potentially the 3rd pass in an HT codeblock.\n");
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		num_passes = 1;
	}
	if(num_passes > 3) {
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_ERROR, "We do not support more than 3 coding passes in an HT codeblock; This codeblocks has %d passes.\n", num_passes);
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	if(cblk->Mb > 30) {
		/* This check is better moved to opj_t2_read_packet_header() in t2.c
		   We do not have enough precision to decode any passes
		   The design of openjpeg assumes that the bits of a 32-bit integer are
		   assigned as follows:
		   bit 31 is for sign
		   bits 30-1 are for magnitude
		   bit 0 is for the center of the quantization bin
		   Therefore we can only do values of cblk->Mb <= 30
		 */
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_ERROR, "32 bits are not enough to decode this codeblock, since the number of bitplane, %d, is larger than 30.\n", cblk->Mb);
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	if(zero_bplanes > cblk->Mb) {
		/* This check is better moved to opj_t2_read_packet_header() in t2.c,
		   in the line "l_cblk->numbps = (uint32_t)l_band->numbps + 1 - i;"
		   where i is the zero bitplanes, and should be no larger than cblk->Mb
		   We cannot have more zero bitplanes than there are planes. */
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. Decoding this codeblock is stopped. There are %d zero bitplanes in %d bitplanes.\n", zero_bplanes, cblk->Mb);
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	else if(zero_bplanes == cblk->Mb && num_passes > 1) {
		/* When the number of zero bitplanes is equal to the number of bitplanes,
		   only the cleanup pass makes sense*/
		if(only_cleanup_pass_is_decoded == FALSE) {
			if(p_manager_mutex) {
				opj_mutex_lock(p_manager_mutex);
			}
			/* We have a second check to prevent the possibility of an overrun condition,
			   in the very unlikely event of a second thread discovering that
			   only_cleanup_pass_is_decoded is false before the first thread changing
			   the condition. */
			if(only_cleanup_pass_is_decoded == FALSE) {
				only_cleanup_pass_is_decoded = TRUE;
				opj_event_msg(p_manager, EVT_WARNING, "Malformed HT codeblock. "
				    "When the number of zero planes bitplanes is "
				    "equal to the number of bitplanes, only the cleanup "
				    "pass makes sense, but we have %d passes in this "
				    "codeblock. Therefore, only the cleanup pass will be "
				    "decoded. This message will not be displayed again.\n",
				    num_passes);
			}
			if(p_manager_mutex) {
				opj_mutex_unlock(p_manager_mutex);
			}
		}
		num_passes = 1;
	}
	/* uint32_t */
	p = cblk->numbps;
	// uint32_t zero planes plus 1
	zero_bplanes_p1 = zero_bplanes + 1;
	if(lengths1 < 2 || (uint32_t)lengths1 > cblk_len ||
	    (uint32_t)(lengths1 + lengths2) > cblk_len) {
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. Invalid codeblock length values.\n");
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	// read scup and fix the bytes there
	lcup = (int)lengths1; // length of CUP
	//scup is the length of MEL + VLC
	scup = (((int)coded_data[lcup - 1]) << 4) + (coded_data[lcup - 2] & 0xF);
	if(scup < 2 || scup > lcup || scup > 4079) { //something is wrong
		/* The standard stipulates 2 <= Scup <= min(Lcup, 4079) */
		if(p_manager_mutex) {
			opj_mutex_lock(p_manager_mutex);
		}
		opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. One of the following condition is not met: 2 <= Scup <= min(Lcup, 4079)\n");
		if(p_manager_mutex) {
			opj_mutex_unlock(p_manager_mutex);
		}
		return FALSE;
	}
	// init structures
	mel_init(&mel, coded_data, lcup, scup);
	rev_init(&vlc, coded_data, lcup, scup);
	frwd_init(&magsgn, coded_data, lcup - scup, 0xFF);
	if(num_passes > 1) { // needs to be tested
		frwd_init(&sigprop, coded_data + lengths1, (int)lengths2, 0);
	}
	if(num_passes > 2) {
		rev_init_mrp(&magref, coded_data, (int)lengths1, (int)lengths2);
	}

	/** State storage
	 *  One byte per quad; for 1024 columns, or 512 quads, we need
	 *  512 bytes. We are using 2 extra bytes one on the left and one on
	 *  the right for convenience.
	 *
	 *  The MSB bit in each byte is (\sigma^nw | \sigma^n), and the 7 LSBs
	 *  contain max(E^nw | E^n)
	 */

	// 514 is enough for a block width of 1024, +2 extra
	// here expanded to 528
	line_state = (uint8*)(mbr2 + 132);

	//initial 2 lines
	/////////////////
	lsp = line_state;          // point to line state
	lsp[0] = 0;                // for initial row of quad, we set to 0
	run = mel_get_run(&mel); // decode runs of events from MEL bitstrm
	// data represented as runs of 0 events
	// See mel_decode description
	qinf[0] = qinf[1] = 0;  // quad info decoded from VLC bitstream
	c_q = 0;                // context for quad q
	sp = decoded_data;      // decoded codeblock samples
	// vlc_val;                 // fetched data from VLC bitstream

	for(x = 0; x < width; x += 4) { // one iteration per quad pair
		uint32_t U_q[2]; // u values for the quad pair
		uint32_t uvlc_mode;
		uint32_t consumed_bits;
		uint32_t m_n, v_n;
		uint32_t ms_val;
		uint32_t locs;

		// decode VLC
		/////////////

		//first quad
		// Get the head of the VLC bitstream. One fetch is enough for two
		// quads, since the largest VLC code is 7 bits, and maximum number of
		// bits used for u is 8.  Therefore for two quads we need 30 bits
		// (if we include unstuffing, then 32 bits are enough, since we have
		// a maximum of one stuffing per two bytes)
		vlc_val = rev_fetch(&vlc);

		//decode VLC using the context c_q and the head of the VLC bitstream
		qinf[0] = vlc_tbl0[(c_q << 7) | (vlc_val & 0x7F) ];

		if(c_q == 0) { // if zero context, we need to use one MEL event
			run -= 2; //the number of 0 events is multiplied by 2, so subtract 2

			// Is the run terminated in 1? if so, use decoded VLC code,
			// otherwise, discard decoded data, since we will decoded again
			// using a different context
			qinf[0] = (run == -1) ? qinf[0] : 0;

			// is run -1 or -2? this means a run has been consumed
			if(run < 0) {
				run = mel_get_run(&mel); // get another run
			}
		}

		// prepare context for the next quad; eqn. 1 in ITU T.814
		c_q = ((qinf[0] & 0x10) >> 4) | ((qinf[0] & 0xE0) >> 5);

		//remove data from vlc stream (0 bits are removed if qinf is not used)
		vlc_val = rev_advance(&vlc, qinf[0] & 0x7);

		//update sigma
		// The update depends on the value of x; consider one uint32_t
		// if x is 0, 8, 16 and so on, then this line update c locations
		//      nibble (4 bits) number   0 1 2 3 4 5 6 7
		//                         LSB   c c 0 0 0 0 0 0
		//                               c c 0 0 0 0 0 0
		//                               0 0 0 0 0 0 0 0
		//                               0 0 0 0 0 0 0 0
		// if x is 4, 12, 20, then this line update locations c
		//      nibble (4 bits) number   0 1 2 3 4 5 6 7
		//                         LSB   0 0 0 0 c c 0 0
		//                               0 0 0 0 c c 0 0
		//                               0 0 0 0 0 0 0 0
		//                               0 0 0 0 0 0 0 0
		*sip |= (((qinf[0] & 0x30) >> 4) | ((qinf[0] & 0xC0) >> 2)) << sip_shift;

		//second quad
		qinf[1] = 0;
		if(x + 2 < width) { // do not run if codeblock is narrower
			//decode VLC using the context c_q and the head of the VLC bitstream
			qinf[1] = vlc_tbl0[(c_q << 7) | (vlc_val & 0x7F)];

			// if context is zero, use one MEL event
			if(c_q == 0) { //zero context
				run -= 2; //subtract 2, since events number if multiplied by 2

				// if event is 0, discard decoded qinf
				qinf[1] = (run == -1) ? qinf[1] : 0;

				if(run < 0) { // have we consumed all events in a run
					run = mel_get_run(&mel); // if yes, then get another run
				}
			}

			//prepare context for the next quad, eqn. 1 in ITU T.814
			c_q = ((qinf[1] & 0x10) >> 4) | ((qinf[1] & 0xE0) >> 5);

			//remove data from vlc stream, if qinf is not used, cwdlen is 0
			vlc_val = rev_advance(&vlc, qinf[1] & 0x7);
		}

		//update sigma
		// The update depends on the value of x; consider one uint32_t
		// if x is 0, 8, 16 and so on, then this line update c locations
		//      nibble (4 bits) number   0 1 2 3 4 5 6 7
		//                         LSB   0 0 c c 0 0 0 0
		//                               0 0 c c 0 0 0 0
		//                               0 0 0 0 0 0 0 0
		//                               0 0 0 0 0 0 0 0
		// if x is 4, 12, 20, then this line update locations c
		//      nibble (4 bits) number   0 1 2 3 4 5 6 7
		//                         LSB   0 0 0 0 0 0 c c
		//                               0 0 0 0 0 0 c c
		//                               0 0 0 0 0 0 0 0
		//                               0 0 0 0 0 0 0 0
		*sip |= (((qinf[1] & 0x30) | ((qinf[1] & 0xC0) << 2))) << (4 + sip_shift);

		sip += x & 0x7 ? 1 : 0; // move sigma pointer to next entry
		sip_shift ^= 0x10; // increment/decrement sip_shift by 16

		// retrieve u
		/////////////

		// uvlc_mode is made up of u_offset bits from the quad pair
		uvlc_mode = ((qinf[0] & 0x8) >> 3) | ((qinf[1] & 0x8) >> 2);
		if(uvlc_mode == 3) { // if both u_offset are set, get an event from
			// the MEL run of events
			run -= 2; //subtract 2, since events number if multiplied by 2
			uvlc_mode += (run == -1) ? 1 : 0; //increment uvlc_mode if event is 1
			if(run < 0) { // if run is consumed (run is -1 or -2), get another run
				run = mel_get_run(&mel);
			}
		}
		//decode uvlc_mode to get u for both quads
		consumed_bits = decode_init_uvlc(vlc_val, uvlc_mode, U_q);
		if(U_q[0] > zero_bplanes_p1 || U_q[1] > zero_bplanes_p1) {
			if(p_manager_mutex) {
				opj_mutex_lock(p_manager_mutex);
			}
			opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. Decoding this codeblock is stopped. U_q is larger than zero bitplanes + 1 \n");
			if(p_manager_mutex) {
				opj_mutex_unlock(p_manager_mutex);
			}
			return FALSE;
		}
		//consume u bits in the VLC code
		vlc_val = rev_advance(&vlc, consumed_bits);
		//decode magsgn and update line_state
		//
		// We obtain a mask for the samples locations that needs evaluation
		//
		locs = 0xFF;
		if(x + 4 > width) {
			locs >>= (x + 4 - width) << 1; // limits width
		}
		locs = height > 1 ? locs : (locs & 0x55); // limits height
		if((((qinf[0] & 0xF0) >> 4) | (qinf[1] & 0xF0)) & ~locs) {
			if(p_manager_mutex) {
				opj_mutex_lock(p_manager_mutex);
			}
			opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. VLC code produces significant samples outside the codeblock area.\n");
			if(p_manager_mutex) {
				opj_mutex_unlock(p_manager_mutex);
			}
			return FALSE;
		}
		//first quad, starting at first sample in quad and moving on
		if(qinf[0] & 0x10) { //is it significant? (sigma_n)
			uint32_t val;
			ms_val = frwd_fetch(&magsgn); //get 32 bits of magsgn data
			m_n = U_q[0] - ((qinf[0] >> 12) & 1); //evaluate m_n (number of bits
			// to read from bitstream), using EMB e_k
			frwd_advance(&magsgn, m_n); //consume m_n
			val = ms_val << 31;     //get sign bit
			v_n = ms_val & ((1U << m_n) - 1); //keep only m_n bits
			v_n |= ((qinf[0] & 0x100) >> 8) << m_n; //add EMB e_1 as MSB
			v_n |= 1;                    //add center of bin
			//v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
			//add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
			sp[0] = val | ((v_n + 2) << (p - 1));
		}
		else if(locs & 0x1) { // if this is inside the codeblock, set the
			sp[0] = 0; // sample to zero
		}

		if(qinf[0] & 0x20) { //sigma_n
			uint32_t val, t;

			ms_val = frwd_fetch(&magsgn); //get 32 bits
			m_n = U_q[0] - ((qinf[0] >> 13) & 1); //m_n, uses EMB e_k
			frwd_advance(&magsgn, m_n); //consume m_n
			val = ms_val << 31;       //get sign bit
			v_n = ms_val & ((1U << m_n) - 1); //keep only m_n bits
			v_n |= ((qinf[0] & 0x200) >> 9) << m_n; //add EMB e_1
			v_n |= 1;                   //bin center
			//v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
			//add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
			sp[stride] = val | ((v_n + 2) << (p - 1));

			//update line_state: bit 7 (\sigma^N), and E^N
			t = lsp[0] & 0x7F; // keep E^NW
			v_n = 32 - count_leading_zeros(v_n);
			lsp[0] = (uint8)(0x80 | (t > v_n ? t : v_n)); //max(E^NW, E^N) | s
		}
		else if(locs & 0x2) { // if this is inside the codeblock, set the
			sp[stride] = 0; // sample to zero
		}

		++lsp; // move to next quad information
		++sp; // move to next column of samples

		//this is similar to the above two samples
		if(qinf[0] & 0x40) {
			uint32_t val;

			ms_val = frwd_fetch(&magsgn);
			m_n = U_q[0] - ((qinf[0] >> 14) & 1);
			frwd_advance(&magsgn, m_n);
			val = ms_val << 31;
			v_n = ms_val & ((1U << m_n) - 1);
			v_n |= (((qinf[0] & 0x400) >> 10) << m_n);
			v_n |= 1;
			sp[0] = val | ((v_n + 2) << (p - 1));
		}
		else if(locs & 0x4) {
			sp[0] = 0;
		}

		lsp[0] = 0;
		if(qinf[0] & 0x80) {
			uint32_t val;
			ms_val = frwd_fetch(&magsgn);
			m_n = U_q[0] - ((qinf[0] >> 15) & 1); //m_n
			frwd_advance(&magsgn, m_n);
			val = ms_val << 31;
			v_n = ms_val & ((1U << m_n) - 1);
			v_n |= ((qinf[0] & 0x800) >> 11) << m_n;
			v_n |= 1; //center of bin
			sp[stride] = val | ((v_n + 2) << (p - 1));

			//line_state: bit 7 (\sigma^NW), and E^NW for next quad
			lsp[0] = (uint8)(0x80 | (32 - count_leading_zeros(v_n)));
		}
		else if(locs & 0x8) { //if outside set to 0
			sp[stride] = 0;
		}

		++sp; //move to next column

		//second quad
		if(qinf[1] & 0x10) {
			uint32_t val;

			ms_val = frwd_fetch(&magsgn);
			m_n = U_q[1] - ((qinf[1] >> 12) & 1); //m_n
			frwd_advance(&magsgn, m_n);
			val = ms_val << 31;
			v_n = ms_val & ((1U << m_n) - 1);
			v_n |= (((qinf[1] & 0x100) >> 8) << m_n);
			v_n |= 1;
			sp[0] = val | ((v_n + 2) << (p - 1));
		}
		else if(locs & 0x10) {
			sp[0] = 0;
		}

		if(qinf[1] & 0x20) {
			uint32_t val, t;

			ms_val = frwd_fetch(&magsgn);
			m_n = U_q[1] - ((qinf[1] >> 13) & 1); //m_n
			frwd_advance(&magsgn, m_n);
			val = ms_val << 31;
			v_n = ms_val & ((1U << m_n) - 1);
			v_n |= (((qinf[1] & 0x200) >> 9) << m_n);
			v_n |= 1;
			sp[stride] = val | ((v_n + 2) << (p - 1));

			//update line_state: bit 7 (\sigma^N), and E^N
			t = lsp[0] & 0x7F; //E^NW
			v_n = 32 - count_leading_zeros(v_n); //E^N
			lsp[0] = (uint8)(0x80 | (t > v_n ? t : v_n)); //max(E^NW, E^N) | s
		}
		else if(locs & 0x20) {
			sp[stride] = 0; //no need to update line_state
		}

		++lsp; //move line state to next quad
		++sp; //move to next sample

		if(qinf[1] & 0x40) {
			uint32_t val;

			ms_val = frwd_fetch(&magsgn);
			m_n = U_q[1] - ((qinf[1] >> 14) & 1); //m_n
			frwd_advance(&magsgn, m_n);
			val = ms_val << 31;
			v_n = ms_val & ((1U << m_n) - 1);
			v_n |= (((qinf[1] & 0x400) >> 10) << m_n);
			v_n |= 1;
			sp[0] = val | ((v_n + 2) << (p - 1));
		}
		else if(locs & 0x40) {
			sp[0] = 0;
		}

		lsp[0] = 0;
		if(qinf[1] & 0x80) {
			uint32_t val;

			ms_val = frwd_fetch(&magsgn);
			m_n = U_q[1] - ((qinf[1] >> 15) & 1); //m_n
			frwd_advance(&magsgn, m_n);
			val = ms_val << 31;
			v_n = ms_val & ((1U << m_n) - 1);
			v_n |= (((qinf[1] & 0x800) >> 11) << m_n);
			v_n |= 1; //center of bin
			sp[stride] = val | ((v_n + 2) << (p - 1));

			//line_state: bit 7 (\sigma^NW), and E^NW for next quad
			lsp[0] = (uint8)(0x80 | (32 - count_leading_zeros(v_n)));
		}
		else if(locs & 0x80) {
			sp[stride] = 0;
		}

		++sp;
	}
	//
	// non-initial lines
	//
	for(y = 2; y < height; /*done at the end of loop*/) {
		uint32_t * sip;
		uint8 ls0;
		int32_t x;

		sip_shift ^= 0x2; // shift sigma to the upper half od the nibble
		sip_shift &= 0xFFFFFFEFU; //move back to 0 (it might have been at 0x10)
		sip = y & 0x4 ? sigma2 : sigma1; //choose sigma array

		lsp = line_state;
		ls0 = lsp[0];           // read the line state value
		lsp[0] = 0;             // and set it to zero
		sp = decoded_data + y * stride; // generated samples
		c_q = 0;                // context
		for(x = 0; x < width; x += 4) {
			uint32_t U_q[2];
			uint32_t uvlc_mode, consumed_bits;
			uint32_t m_n, v_n;
			uint32_t ms_val;
			uint32_t locs;

			// decode vlc
			/////////////

			//first quad
			// get context, eqn. 2 ITU T.814
			// c_q has \sigma^W | \sigma^SW
			c_q |= (ls0 >> 7); //\sigma^NW | \sigma^N
			c_q |= (lsp[1] >> 5) & 0x4; //\sigma^NE | \sigma^NF

			//the following is very similar to previous code, so please refer to
			// that
			vlc_val = rev_fetch(&vlc);
			qinf[0] = vlc_tbl1[(c_q << 7) | (vlc_val & 0x7F)];
			if(c_q == 0) { //zero context
				run -= 2;
				qinf[0] = (run == -1) ? qinf[0] : 0;
				if(run < 0) {
					run = mel_get_run(&mel);
				}
			}
			//prepare context for the next quad, \sigma^W | \sigma^SW
			c_q = ((qinf[0] & 0x40) >> 5) | ((qinf[0] & 0x80) >> 6);

			//remove data from vlc stream
			vlc_val = rev_advance(&vlc, qinf[0] & 0x7);

			//update sigma
			// The update depends on the value of x and y; consider one uint32_t
			// if x is 0, 8, 16 and so on, and y is 2, 6, etc., then this
			// line update c locations
			//      nibble (4 bits) number   0 1 2 3 4 5 6 7
			//                         LSB   0 0 0 0 0 0 0 0
			//                               0 0 0 0 0 0 0 0
			//                               c c 0 0 0 0 0 0
			//                               c c 0 0 0 0 0 0
			*sip |= (((qinf[0] & 0x30) >> 4) | ((qinf[0] & 0xC0) >> 2)) << sip_shift;

			//second quad
			qinf[1] = 0;
			if(x + 2 < width) {
				c_q |= (lsp[1] >> 7);
				c_q |= (lsp[2] >> 5) & 0x4;
				qinf[1] = vlc_tbl1[(c_q << 7) | (vlc_val & 0x7F)];
				if(c_q == 0) { //zero context
					run -= 2;
					qinf[1] = (run == -1) ? qinf[1] : 0;
					if(run < 0) {
						run = mel_get_run(&mel);
					}
				}
				//prepare context for the next quad
				c_q = ((qinf[1] & 0x40) >> 5) | ((qinf[1] & 0x80) >> 6);
				//remove data from vlc stream
				vlc_val = rev_advance(&vlc, qinf[1] & 0x7);
			}

			//update sigma
			*sip |= (((qinf[1] & 0x30) | ((qinf[1] & 0xC0) << 2))) << (4 + sip_shift);

			sip += x & 0x7 ? 1 : 0;
			sip_shift ^= 0x10;

			//retrieve u
			////////////
			uvlc_mode = ((qinf[0] & 0x8) >> 3) | ((qinf[1] & 0x8) >> 2);
			consumed_bits = decode_noninit_uvlc(vlc_val, uvlc_mode, U_q);
			vlc_val = rev_advance(&vlc, consumed_bits);

			//calculate E^max and add it to U_q, eqns 5 and 6 in ITU T.814
			if((qinf[0] & 0xF0) & ((qinf[0] & 0xF0) - 1)) { // is \gamma_q 1?
				uint32_t E = (ls0 & 0x7Fu);
				E = E > (lsp[1] & 0x7Fu) ? E : (lsp[1] & 0x7Fu); //max(E, E^NE, E^NF)
				//since U_q already has u_q + 1, we subtract 2 instead of 1
				U_q[0] += E > 2 ? E - 2 : 0;
			}

			if((qinf[1] & 0xF0) & ((qinf[1] & 0xF0) - 1)) { //is \gamma_q 1?
				uint32_t E = (lsp[1] & 0x7Fu);
				E = E > (lsp[2] & 0x7Fu) ? E : (lsp[2] & 0x7Fu); //max(E, E^NE, E^NF)
				//since U_q already has u_q + 1, we subtract 2 instead of 1
				U_q[1] += E > 2 ? E - 2 : 0;
			}
			if(U_q[0] > zero_bplanes_p1 || U_q[1] > zero_bplanes_p1) {
				if(p_manager_mutex) {
					opj_mutex_lock(p_manager_mutex);
				}
				opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. Decoding this codeblock is stopped. U_q is larger than bitplanes + 1 \n");
				if(p_manager_mutex) {
					opj_mutex_unlock(p_manager_mutex);
				}
				return FALSE;
			}
			ls0 = lsp[2]; //for next double quad
			lsp[1] = lsp[2] = 0;
			//
			// decode magsgn and update line_state
			//
			//locations where samples need update
			locs = 0xFF;
			if(x + 4 > width) {
				locs >>= (x + 4 - width) << 1;
			}
			locs = y + 2 <= height ? locs : (locs & 0x55);

			if((((qinf[0] & 0xF0) >> 4) | (qinf[1] & 0xF0)) & ~locs) {
				if(p_manager_mutex) {
					opj_mutex_lock(p_manager_mutex);
				}
				opj_event_msg(p_manager, EVT_ERROR, "Malformed HT codeblock. VLC code produces significant samples outside the codeblock area.\n");
				if(p_manager_mutex) {
					opj_mutex_unlock(p_manager_mutex);
				}
				return FALSE;
			}
			if(qinf[0] & 0x10) { //sigma_n
				uint32_t val;
				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[0] - ((qinf[0] >> 12) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= ((qinf[0] & 0x100) >> 8) << m_n;
				v_n |= 1; //center of bin
				sp[0] = val | ((v_n + 2) << (p - 1));
			}
			else if(locs & 0x1) {
				sp[0] = 0;
			}

			if(qinf[0] & 0x20) { //sigma_n
				uint32_t val, t;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[0] - ((qinf[0] >> 13) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= ((qinf[0] & 0x200) >> 9) << m_n;
				v_n |= 1; //center of bin
				sp[stride] = val | ((v_n + 2) << (p - 1));

				//update line_state: bit 7 (\sigma^N), and E^N
				t = lsp[0] & 0x7F; //E^NW
				v_n = 32 - count_leading_zeros(v_n);
				lsp[0] = (uint8)(0x80 | (t > v_n ? t : v_n));
			}
			else if(locs & 0x2) {
				sp[stride] = 0; //no need to update line_state
			}

			++lsp;
			++sp;

			if(qinf[0] & 0x40) { //sigma_n
				uint32_t val;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[0] - ((qinf[0] >> 14) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= (((qinf[0] & 0x400) >> 10) << m_n);
				v_n |= 1;            //center of bin
				sp[0] = val | ((v_n + 2) << (p - 1));
			}
			else if(locs & 0x4) {
				sp[0] = 0;
			}

			if(qinf[0] & 0x80) { //sigma_n
				uint32_t val;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[0] - ((qinf[0] >> 15) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= ((qinf[0] & 0x800) >> 11) << m_n;
				v_n |= 1; //center of bin
				sp[stride] = val | ((v_n + 2) << (p - 1));

				//update line_state: bit 7 (\sigma^NW), and E^NW for next quad
				lsp[0] = (uint8)(0x80 | (32 - count_leading_zeros(v_n)));
			}
			else if(locs & 0x8) {
				sp[stride] = 0;
			}

			++sp;

			if(qinf[1] & 0x10) { //sigma_n
				uint32_t val;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[1] - ((qinf[1] >> 12) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= (((qinf[1] & 0x100) >> 8) << m_n);
				v_n |= 1;            //center of bin
				sp[0] = val | ((v_n + 2) << (p - 1));
			}
			else if(locs & 0x10) {
				sp[0] = 0;
			}

			if(qinf[1] & 0x20) { //sigma_n
				uint32_t val, t;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[1] - ((qinf[1] >> 13) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= (((qinf[1] & 0x200) >> 9) << m_n);
				v_n |= 1; //center of bin
				sp[stride] = val | ((v_n + 2) << (p - 1));

				//update line_state: bit 7 (\sigma^N), and E^N
				t = lsp[0] & 0x7F; //E^NW
				v_n = 32 - count_leading_zeros(v_n);
				lsp[0] = (uint8)(0x80 | (t > v_n ? t : v_n));
			}
			else if(locs & 0x20) {
				sp[stride] = 0; //no need to update line_state
			}

			++lsp;
			++sp;

			if(qinf[1] & 0x40) { //sigma_n
				uint32_t val;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[1] - ((qinf[1] >> 14) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= (((qinf[1] & 0x400) >> 10) << m_n);
				v_n |= 1;            //center of bin
				sp[0] = val | ((v_n + 2) << (p - 1));
			}
			else if(locs & 0x40) {
				sp[0] = 0;
			}

			if(qinf[1] & 0x80) { //sigma_n
				uint32_t val;

				ms_val = frwd_fetch(&magsgn);
				m_n = U_q[1] - ((qinf[1] >> 15) & 1); //m_n
				frwd_advance(&magsgn, m_n);
				val = ms_val << 31;
				v_n = ms_val & ((1U << m_n) - 1);
				v_n |= (((qinf[1] & 0x800) >> 11) << m_n);
				v_n |= 1; //center of bin
				sp[stride] = val | ((v_n + 2) << (p - 1));

				//update line_state: bit 7 (\sigma^NW), and E^NW for next quad
				lsp[0] = (uint8)(0x80 | (32 - count_leading_zeros(v_n)));
			}
			else if(locs & 0x80) {
				sp[stride] = 0;
			}

			++sp;
		}

		y += 2;
		if(num_passes > 1 && (y & 3) == 0) { //executed at multiples of 4
			// This is for SPP and potentially MRP

			if(num_passes > 2) { //do MRP
				// select the current stripe
				uint32_t * cur_sig = y & 0x4 ? sigma1 : sigma2;
				// the address of the data that needs updating
				uint32_t * dpp = decoded_data + (y - 4) * stride;
				uint32_t half = 1u << (p - 2); // half the center of the bin
				int32_t i;
				for(i = 0; i < width; i += 8) {
					//Process one entry from sigma array at a time
					// Each nibble (4 bits) in the sigma array represents 4 rows,
					// and the 32 bits contain 8 columns
					uint32_t cwd = rev_fetch_mrp(&magref); // get 32 bit data
					uint32_t sig = *cur_sig++; // 32 bit that will be processed now
					uint32_t col_mask = 0xFu; // a mask for a column in sig
					uint32_t * dp = dpp + i; // next column in decode samples
					if(sig) { // if any of the 32 bits are set
						int j;
						for(j = 0; j < 8; ++j, dp++) { //one column at a time
							if(sig & col_mask) { // lowest nibble
								uint32_t sample_mask = 0x11111111u & col_mask; //LSB
								if(sig & sample_mask) { //if LSB is set
									uint32_t sym;
									assert(dp[0] != 0); // decoded value cannot be zero
									sym = cwd & 1; // get it value
									// remove center of bin if sym is 0
									dp[0] ^= (1 - sym) << (p - 1);
									dp[0] |= half; // put half the center of bin
									cwd >>= 1; //consume word
								}
								sample_mask += sample_mask; //next row

								if(sig & sample_mask) {
									uint32_t sym;

									assert(dp[stride] != 0);
									sym = cwd & 1;
									dp[stride] ^= (1 - sym) << (p - 1);
									dp[stride] |= half;
									cwd >>= 1;
								}
								sample_mask += sample_mask;

								if(sig & sample_mask) {
									uint32_t sym;

									assert(dp[2 * stride] != 0);
									sym = cwd & 1;
									dp[2 * stride] ^= (1 - sym) << (p - 1);
									dp[2 * stride] |= half;
									cwd >>= 1;
								}
								sample_mask += sample_mask;

								if(sig & sample_mask) {
									uint32_t sym;

									assert(dp[3 * stride] != 0);
									sym = cwd & 1;
									dp[3 * stride] ^= (1 - sym) << (p - 1);
									dp[3 * stride] |= half;
									cwd >>= 1;
								}
								sample_mask += sample_mask;
							}
							col_mask <<= 4; //next column
						}
					}
					// consume data according to the number of bits set
					rev_advance_mrp(&magref, population_count(sig));
				}
			}

			if(y >= 4) { // update mbr array at the end of each stripe
				//generate mbr corresponding to a stripe
				uint32_t * sig = y & 0x4 ? sigma1 : sigma2;
				uint32_t * mbr = y & 0x4 ? mbr1 : mbr2;

				//data is processed in patches of 8 columns, each
				// each 32 bits in sigma1 or mbr1 represent 4 rows

				//integrate horizontally
				uint32_t prev = 0; // previous columns
				int32_t i;
				for(i = 0; i < width; i += 8, mbr++, sig++) {
					uint32_t t, z;

					mbr[0] = sig[0]; //start with significant samples
					mbr[0] |= prev >> 28; //for first column, left neighbors
					mbr[0] |= sig[0] << 4; //left neighbors
					mbr[0] |= sig[0] >> 4; //right neighbors
					mbr[0] |= sig[1] << 28; //for last column, right neighbors
					prev = sig[0]; // for next group of columns

					//integrate vertically
					t = mbr[0], z = mbr[0];
					z |= (t & 0x77777777) << 1; //above neighbors
					z |= (t & 0xEEEEEEEE) >> 1; //below neighbors
					mbr[0] = z & ~sig[0]; //remove already significance samples
				}
			}

			if(y >= 8) { //wait until 8 rows has been processed
				uint32_t * cur_sig, * cur_mbr, * nxt_sig, * nxt_mbr;
				uint32_t prev;
				uint32_t val;
				int32_t i;

				// add membership from the next stripe, obtained above
				cur_sig = y & 0x4 ? sigma2 : sigma1;
				cur_mbr = y & 0x4 ? mbr2 : mbr1;
				nxt_sig = y & 0x4 ? sigma1 : sigma2; //future samples
				prev = 0; // the columns before these group of 8 columns
				for(i = 0; i < width; i += 8, cur_mbr++, cur_sig++, nxt_sig++) {
					uint32_t t = nxt_sig[0];
					t |= prev >> 28; //for first column, left neighbors
					t |= nxt_sig[0] << 4; //left neighbors
					t |= nxt_sig[0] >> 4; //right neighbors
					t |= nxt_sig[1] << 28; //for last column, right neighbors
					prev = nxt_sig[0]; // for next group of columns

					if(!stripe_causal) {
						cur_mbr[0] |= (t & 0x11111111u) << 3; //propagate up to cur_mbr
					}
					cur_mbr[0] &= ~cur_sig[0]; //remove already significance samples
				}

				//find new locations and get signs
				cur_sig = y & 0x4 ? sigma2 : sigma1;
				cur_mbr = y & 0x4 ? mbr2 : mbr1;
				nxt_sig = y & 0x4 ? sigma1 : sigma2; //future samples
				nxt_mbr = y & 0x4 ? mbr1 : mbr2; //future samples
				val = 3u << (p - 2); // sample values for newly discovered
				// significant samples including the bin center
				for(i = 0; i < width;
				    i += 8, cur_sig++, cur_mbr++, nxt_sig++, nxt_mbr++) {
					uint32_t ux, tx;
					uint32_t mbr = *cur_mbr;
					uint32_t new_sig = 0;
					if(mbr) { //are there any samples that might be significant
						int32_t n;
						for(n = 0; n < 8; n += 4) {
							uint32_t col_mask;
							uint32_t inv_sig;
							int32_t end;
							int32_t j;
							uint32_t cwd = frwd_fetch(&sigprop); //get 32 bits
							uint32_t cnt = 0;
							uint32_t * dp = decoded_data + (y - 8) * stride;
							dp += i + n; //address for decoded samples
							col_mask = 0xFu << (4 * n); //a mask to select a column
							inv_sig = ~cur_sig[0]; // insignificant samples
							//find the last sample we operate on
							end = n + 4 + i < width ? n + 4 : width - i;
							for(j = n; j < end; ++j, ++dp, col_mask <<= 4) {
								uint32_t sample_mask;

								if((col_mask & mbr) == 0) { //no samples need checking
									continue;
								}
								//scan mbr to find a new significant sample
								sample_mask = 0x11111111u & col_mask; // LSB
								if(mbr & sample_mask) {
									assert(dp[0] == 0); // the sample must have been 0
									if(cwd & 1) { //if this sample has become significant
										// must propagate it to nearby samples
										uint32_t t;
										new_sig |= sample_mask; // new significant samples
										t = 0x32u << (j * 4);// propagation to neighbors
										mbr |= t & inv_sig; //remove already significant samples
									}
									cwd >>= 1;
									++cnt; //consume bit and increment number of
									//consumed bits
								}
								sample_mask += sample_mask; // next row
								if(mbr & sample_mask) {
									assert(dp[stride] == 0);
									if(cwd & 1) {
										uint32_t t;
										new_sig |= sample_mask;
										t = 0x74u << (j * 4);
										mbr |= t & inv_sig;
									}
									cwd >>= 1;
									++cnt;
								}

								sample_mask += sample_mask;
								if(mbr & sample_mask) {
									assert(dp[2 * stride] == 0);
									if(cwd & 1) {
										uint32_t t;
										new_sig |= sample_mask;
										t = 0xE8u << (j * 4);
										mbr |= t & inv_sig;
									}
									cwd >>= 1;
									++cnt;
								}

								sample_mask += sample_mask;
								if(mbr & sample_mask) {
									assert(dp[3 * stride] == 0);
									if(cwd & 1) {
										uint32_t t;
										new_sig |= sample_mask;
										t = 0xC0u << (j * 4);
										mbr |= t & inv_sig;
									}
									cwd >>= 1;
									++cnt;
								}
							}

							//obtain signs here
							if(new_sig & (0xFFFFu << (4 * n))) { //if any
								uint32_t col_mask;
								int32_t j;
								uint32_t * dp = decoded_data + (y - 8) * stride;
								dp += i + n; // decoded samples address
								col_mask = 0xFu << (4 * n); //mask to select a column

								for(j = n; j < end; ++j, ++dp, col_mask <<= 4) {
									uint32_t sample_mask;

									if((col_mask & new_sig) == 0) { //if non is
										                        // significant
										continue;
									}

									//scan 4 signs
									sample_mask = 0x11111111u & col_mask;
									if(new_sig & sample_mask) {
										assert(dp[0] == 0);
										dp[0] |= ((cwd & 1) << 31) | val; //put
										                                  // value
										                                  // and
										                                  // sign
										cwd >>= 1;
										++cnt; //consume bit and increment
										       // number
										//of consumed bits
									}

									sample_mask += sample_mask;
									if(new_sig & sample_mask) {
										assert(dp[stride] == 0);
										dp[stride] |= ((cwd & 1) << 31) | val;
										cwd >>= 1;
										++cnt;
									}

									sample_mask += sample_mask;
									if(new_sig & sample_mask) {
										assert(dp[2 * stride] == 0);
										dp[2 * stride] |= ((cwd & 1) << 31) | val;
										cwd >>= 1;
										++cnt;
									}

									sample_mask += sample_mask;
									if(new_sig & sample_mask) {
										assert(dp[3 * stride] == 0);
										dp[3 * stride] |= ((cwd & 1) << 31) | val;
										cwd >>= 1;
										++cnt;
									}
								}
							}
							frwd_advance(&sigprop, cnt); //consume the bits from bitstrm
							cnt = 0;

							//update the next 8 columns
							if(n == 4) {
								//horizontally
								uint32_t t = new_sig >> 28;
								t |= ((t & 0xE) >> 1) | ((t & 7) << 1);
								cur_mbr[1] |= t & ~cur_sig[1];
							}
						}
					}
					//update the next stripe (vertically propagation)
					new_sig |= cur_sig[0];
					ux = (new_sig & 0x88888888) >> 3;
					tx = ux | (ux << 4) | (ux >> 4); //left and right neighbors
					if(i > 0) {
						nxt_mbr[-1] |= (ux << 28) & ~nxt_sig[-1];
					}
					nxt_mbr[0] |= tx & ~nxt_sig[0];
					nxt_mbr[1] |= (ux >> 28) & ~nxt_sig[1];
				}

				//clear current sigma
				//mbr need not be cleared because it is overwritten
				cur_sig = y & 0x4 ? sigma2 : sigma1;
				memzero(cur_sig, ((((uint32_t)width + 7u) >> 3) + 1u) << 2);
			}
		}
	}
	//terminating
	if(num_passes > 1) {
		int32_t st, y;
		if(num_passes > 2 && ((height & 3) == 1 || (height & 3) == 2)) {
			//do magref
			uint32_t * cur_sig = height & 0x4 ? sigma2 : sigma1; //reversed
			uint32_t * dpp = decoded_data + (height & 0xFFFFFC) * stride;
			uint32_t half = 1u << (p - 2);
			int32_t i;
			for(i = 0; i < width; i += 8) {
				uint32_t cwd = rev_fetch_mrp(&magref);
				uint32_t sig = *cur_sig++;
				uint32_t col_mask = 0xF;
				uint32_t * dp = dpp + i;
				if(sig) {
					int j;
					for(j = 0; j < 8; ++j, dp++) {
						if(sig & col_mask) {
							uint32_t sample_mask = 0x11111111 & col_mask;

							if(sig & sample_mask) {
								uint32_t sym;
								assert(dp[0] != 0);
								sym = cwd & 1;
								dp[0] ^= (1 - sym) << (p - 1);
								dp[0] |= half;
								cwd >>= 1;
							}
							sample_mask += sample_mask;

							if(sig & sample_mask) {
								uint32_t sym;
								assert(dp[stride] != 0);
								sym = cwd & 1;
								dp[stride] ^= (1 - sym) << (p - 1);
								dp[stride] |= half;
								cwd >>= 1;
							}
							sample_mask += sample_mask;

							if(sig & sample_mask) {
								uint32_t sym;
								assert(dp[2 * stride] != 0);
								sym = cwd & 1;
								dp[2 * stride] ^= (1 - sym) << (p - 1);
								dp[2 * stride] |= half;
								cwd >>= 1;
							}
							sample_mask += sample_mask;

							if(sig & sample_mask) {
								uint32_t sym;
								assert(dp[3 * stride] != 0);
								sym = cwd & 1;
								dp[3 * stride] ^= (1 - sym) << (p - 1);
								dp[3 * stride] |= half;
								cwd >>= 1;
							}
							sample_mask += sample_mask;
						}
						col_mask <<= 4;
					}
				}
				rev_advance_mrp(&magref, population_count(sig));
			}
		}

		//do the last incomplete stripe
		// for cases of (height & 3) == 0 and 3
		// the should have been processed previously
		if((height & 3) == 1 || (height & 3) == 2) {
			//generate mbr of first stripe
			uint32_t * sig = height & 0x4 ? sigma2 : sigma1;
			uint32_t * mbr = height & 0x4 ? mbr2 : mbr1;
			//integrate horizontally
			uint32_t prev = 0;
			int32_t i;
			for(i = 0; i < width; i += 8, mbr++, sig++) {
				uint32_t t, z;

				mbr[0] = sig[0];
				mbr[0] |= prev >> 28; //for first column, left neighbors
				mbr[0] |= sig[0] << 4; //left neighbors
				mbr[0] |= sig[0] >> 4; //left neighbors
				mbr[0] |= sig[1] << 28; //for last column, right neighbors
				prev = sig[0];

				//integrate vertically
				t = mbr[0], z = mbr[0];
				z |= (t & 0x77777777) << 1; //above neighbors
				z |= (t & 0xEEEEEEEE) >> 1; //below neighbors
				mbr[0] = z & ~sig[0]; //remove already significance samples
			}
		}

		st = height;
		st -= height > 6 ? (((height + 1) & 3) + 3) : height;
		for(y = st; y < height; y += 4) {
			uint32_t * cur_sig, * cur_mbr, * nxt_sig, * nxt_mbr;
			uint32_t val;
			int32_t i;

			uint32_t pattern = 0xFFFFFFFFu; // a pattern needed samples
			if(height - y == 3) {
				pattern = 0x77777777u;
			}
			else if(height - y == 2) {
				pattern = 0x33333333u;
			}
			else if(height - y == 1) {
				pattern = 0x11111111u;
			}

			//add membership from the next stripe, obtained above
			if(height - y > 4) {
				uint32_t prev = 0;
				int32_t i;
				cur_sig = y & 0x4 ? sigma2 : sigma1;
				cur_mbr = y & 0x4 ? mbr2 : mbr1;
				nxt_sig = y & 0x4 ? sigma1 : sigma2;
				for(i = 0; i < width; i += 8, cur_mbr++, cur_sig++, nxt_sig++) {
					uint32_t t = nxt_sig[0];
					t |= prev >> 28; //for first column, left neighbors
					t |= nxt_sig[0] << 4; //left neighbors
					t |= nxt_sig[0] >> 4; //left neighbors
					t |= nxt_sig[1] << 28; //for last column, right neighbors
					prev = nxt_sig[0];

					if(!stripe_causal) {
						cur_mbr[0] |= (t & 0x11111111u) << 3;
					}
					//remove already significance samples
					cur_mbr[0] &= ~cur_sig[0];
				}
			}

			//find new locations and get signs
			cur_sig = y & 0x4 ? sigma2 : sigma1;
			cur_mbr = y & 0x4 ? mbr2 : mbr1;
			nxt_sig = y & 0x4 ? sigma1 : sigma2;
			nxt_mbr = y & 0x4 ? mbr1 : mbr2;
			val = 3u << (p - 2);
			for(i = 0; i < width; i += 8,
			    cur_sig++, cur_mbr++, nxt_sig++, nxt_mbr++) {
				uint32_t mbr = *cur_mbr & pattern; //skip unneeded samples
				uint32_t new_sig = 0;
				uint32_t ux, tx;
				if(mbr) {
					int32_t n;
					for(n = 0; n < 8; n += 4) {
						uint32_t col_mask;
						uint32_t inv_sig;
						int32_t end;
						int32_t j;

						uint32_t cwd = frwd_fetch(&sigprop);
						uint32_t cnt = 0;

						uint32_t * dp = decoded_data + y * stride;
						dp += i + n;

						col_mask = 0xFu << (4 * n);

						inv_sig = ~cur_sig[0] & pattern;

						end = n + 4 + i < width ? n + 4 : width - i;
						for(j = n; j < end; ++j, ++dp, col_mask <<= 4) {
							uint32_t sample_mask;

							if((col_mask & mbr) == 0) {
								continue;
							}

							//scan 4 mbr
							sample_mask = 0x11111111u & col_mask;
							if(mbr & sample_mask) {
								assert(dp[0] == 0);
								if(cwd & 1) {
									uint32_t t;
									new_sig |= sample_mask;
									t = 0x32u << (j * 4);
									mbr |= t & inv_sig;
								}
								cwd >>= 1;
								++cnt;
							}

							sample_mask += sample_mask;
							if(mbr & sample_mask) {
								assert(dp[stride] == 0);
								if(cwd & 1) {
									uint32_t t;
									new_sig |= sample_mask;
									t = 0x74u << (j * 4);
									mbr |= t & inv_sig;
								}
								cwd >>= 1;
								++cnt;
							}

							sample_mask += sample_mask;
							if(mbr & sample_mask) {
								assert(dp[2 * stride] == 0);
								if(cwd & 1) {
									uint32_t t;
									new_sig |= sample_mask;
									t = 0xE8u << (j * 4);
									mbr |= t & inv_sig;
								}
								cwd >>= 1;
								++cnt;
							}

							sample_mask += sample_mask;
							if(mbr & sample_mask) {
								assert(dp[3 * stride] == 0);
								if(cwd & 1) {
									uint32_t t;
									new_sig |= sample_mask;
									t = 0xC0u << (j * 4);
									mbr |= t & inv_sig;
								}
								cwd >>= 1;
								++cnt;
							}
						}

						//signs here
						if(new_sig & (0xFFFFu << (4 * n))) {
							uint32_t col_mask;
							int32_t j;
							uint32_t * dp = decoded_data + y * stride;
							dp += i + n;
							col_mask = 0xFu << (4 * n);

							for(j = n; j < end; ++j, ++dp, col_mask <<= 4) {
								uint32_t sample_mask;
								if((col_mask & new_sig) == 0) {
									continue;
								}

								//scan 4 signs
								sample_mask = 0x11111111u & col_mask;
								if(new_sig & sample_mask) {
									assert(dp[0] == 0);
									dp[0] |= ((cwd & 1) << 31) | val;
									cwd >>= 1;
									++cnt;
								}
								sample_mask += sample_mask;
								if(new_sig & sample_mask) {
									assert(dp[stride] == 0);
									dp[stride] |= ((cwd & 1) << 31) | val;
									cwd >>= 1;
									++cnt;
								}
								sample_mask += sample_mask;
								if(new_sig & sample_mask) {
									assert(dp[2 * stride] == 0);
									dp[2 * stride] |= ((cwd & 1) << 31) | val;
									cwd >>= 1;
									++cnt;
								}
								sample_mask += sample_mask;
								if(new_sig & sample_mask) {
									assert(dp[3 * stride] == 0);
									dp[3 * stride] |= ((cwd & 1) << 31) | val;
									cwd >>= 1;
									++cnt;
								}
							}
						}
						frwd_advance(&sigprop, cnt);
						cnt = 0;

						//update next columns
						if(n == 4) {
							//horizontally
							uint32_t t = new_sig >> 28;
							t |= ((t & 0xE) >> 1) | ((t & 7) << 1);
							cur_mbr[1] |= t & ~cur_sig[1];
						}
					}
				}
				//propagate down (vertically propagation)
				new_sig |= cur_sig[0];
				ux = (new_sig & 0x88888888) >> 3;
				tx = ux | (ux << 4) | (ux >> 4);
				if(i > 0) {
					nxt_mbr[-1] |= (ux << 28) & ~nxt_sig[-1];
				}
				nxt_mbr[0] |= tx & ~nxt_sig[0];
				nxt_mbr[1] |= (ux >> 28) & ~nxt_sig[1];
			}
		}
	}
	{
		int32_t x, y;
		for(y = 0; y < height; ++y) {
			int32_t* sp = (int32_t*)decoded_data + y * stride;
			for(x = 0; x < width; ++x, ++sp) {
				int32_t val = (*sp & 0x7FFFFFFF);
				*sp = ((uint32_t)*sp & 0x80000000) ? -val : val;
			}
		}
	}
	return TRUE;
}
