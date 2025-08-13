// bzlib.c
// Library top-level functions.
//
/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.6 of 6 September 2010
   Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>

   Please read the WARNING, DISCLAIMER and PATENTS sections in the README file.

   This program is released under the terms of the license contained in the file LICENSE.
   ------------------------------------------------------------------ */

/* CHANGES
   0.9.0    -- original version.
   0.9.0a/b -- no changes in this file.
   0.9.0c   -- made zero-length BZ_FLUSH work correctly in bzCompress().
     fixed bzWrite/bzRead to ignore zero-length requests.
     fixed bzread to correctly handle read requests after EOF.
     wrong parameter order in call to bzDecompressInit in
     bzBuffToBuffDecompress.  Fixed.
 */
#include <slib-internal.h>
#pragma hdrstop
#include "bzlib_private.h"
// 
// Compression stuff
// 
#ifndef BZ_NO_STDIO
#if 0 // @v11.7.11 {
void BZ2_bz__AssertH__fail(int errcode)
{
	slfprintf_stderr("\n\nbzip2/libbzip2: internal error number %d.\n"
	    "This is a bug in bzip2/libbzip2, %s.\n"
	    "Please report it to me at: jseward@bzip.org.  If this happened\n"
	    "when you were using some program which uses libbzip2 as a\n"
	    "component, you should also report this bug to the author(s)\n"
	    "of that program.  Please make an effort to report this bug;\n"
	    "timely and accurate bug reports eventually lead to higher\n"
	    "quality software.  Thanks.  Julian Seward, 10 December 2007.\n\n", errcode, BZ2_bzlibVersion());

	if(errcode == 1007) {
		slfprintf_stderr("\n*** A special note about internal error number 1007 ***\n"
		    "\n"
		    "Experience suggests that a common cause of i.e. 1007\n"
		    "is unreliable memory or other hardware.  The 1007 assertion\n"
		    "just happens to cross-check the results of huge numbers of\n"
		    "memory reads/writes, and so acts (unintendedly) as a stress\n"
		    "test of your memory system.\n"
		    "\n"
		    "I suggest the following: try compressing the file again,\n"
		    "possibly monitoring progress in detail with the -vv flag.\n"
		    "\n"
		    "* If the error cannot be reproduced, and/or happens at different\n"
		    "  points in compression, you may have a flaky memory system.\n"
		    "  Try a memory-test program.  I have used Memtest86\n"
		    "  (www.memtest86.com).  At the time of writing it is free (GPLd).\n"
		    "  Memtest86 tests memory much more thorougly than your BIOSs\n"
		    "  power-on test, and may find failures that the BIOS doesn't.\n"
		    "\n"
		    "* If the error can be repeatably reproduced, this is a bug in\n"
		    "  bzip2, and I would very much like to hear about it.  Please\n"
		    "  let me know, and, ideally, save a copy of the file causing the\n"
		    "  problem -- without which I will be unable to investigate it.\n"
		    "\n"
		    );
	}
	exit(3);
}
#endif // } @v11.7.11
#endif

//static void * default_bzalloc(void * opaque, size_t items, size_t size) { return SAlloc::M(items * size); }
//static void default_bzfree(void * opaque, void * addr) { SAlloc::F(addr); }

static void prepare_new_block(EState* s)
{
	s->nblock = 0;
	s->numZ = 0;
	s->state_out_pos = 0;
	BZ_INITIALISE_CRC(s->blockCRC);
	for(size_t i = 0; i < 256; i++) 
		s->inUse[i] = false;
	s->blockNo++;
}

static void init_RL(EState* s)
{
	s->state_in_ch  = 256;
	s->state_in_len = 0;
}

static bool FASTCALL isempty_RL(const EState * s)
{
	return (s->state_in_ch < 256 && s->state_in_len > 0) ? false : true;
}

int BZ2_bzCompressInit(bz_stream* strm, int blockSize100k, int verbosity, int workFactor)
{
	int32 n;
	EState* s;
	if(strm == NULL || blockSize100k < 1 || blockSize100k > 9 || workFactor < 0 || workFactor > 250)
		return BZ_PARAM_ERROR;
	SETIFZQ(workFactor, 30);
	//if(strm->bzalloc == NULL) strm->bzalloc = default_bzalloc;
	//if(strm->bzfree == NULL) strm->bzfree = default_bzfree;
	s = (EState *)SAlloc::M(sizeof(EState));
	if(!s) 
		return BZ_MEM_ERROR;
	s->strm = strm;
	s->arr1 = NULL;
	s->arr2 = NULL;
	s->ftab = NULL;
	n       = 100000 * blockSize100k;
	s->arr1 = (uint32 *)SAlloc::M(n  * sizeof(uint32));
	s->arr2 = (uint32 *)SAlloc::M((n+BZ_N_OVERSHOOT) * sizeof(uint32));
	s->ftab = (uint32 *)SAlloc::M(65537   * sizeof(uint32));
	if(!s->arr1 || !s->arr2 || !s->ftab) {
		SAlloc::F(s->arr1);
		SAlloc::F(s->arr2);
		SAlloc::F(s->ftab);
		SAlloc::F(s);
		return BZ_MEM_ERROR;
	}
	else {
		s->blockNo   = 0;
		s->state     = BZ_S_INPUT;
		s->mode      = BZ_M_RUNNING;
		s->combinedCRC    = 0;
		s->blockSize100k  = blockSize100k;
		s->nblockMAX = 100000 * blockSize100k - 19;
		s->verbosity = verbosity;
		s->workFactor     = workFactor;
		s->block     = (uchar *)s->arr2;
		s->mtfv      = (uint16 *)s->arr1;
		s->zbits     = NULL;
		s->ptr       = (uint32 *)s->arr1;
		strm->state  = s;
		//strm->total_in_lo32  = 0;
		//strm->total_in_hi32  = 0;
		//strm->total_out_lo32 = 0;
		//strm->total_out_hi32 = 0;
		strm->TotalIn = 0;
		strm->TotalOut = 0;
		init_RL(s);
		prepare_new_block(s);
		return BZ_OK;
	}
}

static void FASTCALL add_pair_to_block(EState * s)
{
	const uchar ch = static_cast<uchar>(s->state_in_ch);
	for(int32 i = 0; i < s->state_in_len; i++) {
		BZ_UPDATE_CRC(s->blockCRC, ch);
	}
	s->inUse[s->state_in_ch] = true;
	switch(s->state_in_len) {
		case 1:
		    s->block[s->nblock] = ch; s->nblock++;
		    break;
		case 2:
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ch; s->nblock++;
		    break;
		case 3:
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ch; s->nblock++;
		    break;
		default:
		    s->inUse[s->state_in_len-4] = true;
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ch; s->nblock++;
		    s->block[s->nblock] = ((uchar)(s->state_in_len-4));
		    s->nblock++;
		    break;
	}
}

static void flush_RL(EState* s)
{
	if(s->state_in_ch < 256) 
		add_pair_to_block(s);
	init_RL(s);
}

#define ADD_CHAR_TO_BLOCK(zs, zchh0)		   \
	{						  \
		uint32 zchh = static_cast<uint32>(zchh0);		       \
		/*-- fast track the common case --*/	       \
		if(zchh != zs->state_in_ch && zs->state_in_len == 1) { \
			uchar ch = static_cast<uchar>(zs->state_in_ch);	    \
			BZ_UPDATE_CRC(zs->blockCRC, ch);	  \
			zs->inUse[zs->state_in_ch] = true;	    \
			zs->block[zs->nblock] = static_cast<uchar>(ch); \
			zs->nblock++;				    \
			zs->state_in_ch = zchh;			    \
		}					       \
		else					       \
		/*-- general, uncommon cases --*/	       \
		if(zchh != zs->state_in_ch || zs->state_in_len == 255) { \
			if(zs->state_in_ch < 256)		   \
				add_pair_to_block(zs);		      \
			zs->state_in_ch = zchh;			    \
			zs->state_in_len = 1;			    \
		} \
		else {				       \
			zs->state_in_len++;			    \
		}					       \
	}

static bool copy_input_until_stop(EState* s)
{
	bool progress_in = false;
	if(s->mode == BZ_M_RUNNING) {
		/*-- fast track the common case --*/
		while(true) {
			/*-- block full? --*/
			if(s->nblock >= s->nblockMAX) break;
			/*-- no input? --*/
			if(s->strm->avail_in == 0) break;
			progress_in = true;
			ADD_CHAR_TO_BLOCK(s, (uint32)(*((uchar *)(s->strm->next_in))));
			s->strm->next_in++;
			s->strm->avail_in--;
			s->strm->TotalIn++;
		}
	}
	else {
		/*-- general, uncommon case --*/
		while(true) {
			/*-- block full? --*/
			if(s->nblock >= s->nblockMAX) 
				break;
			/*-- no input? --*/
			if(s->strm->avail_in == 0) 
				break;
			/*-- flush/finish end? --*/
			if(s->avail_in_expect == 0) 
				break;
			progress_in = true;
			ADD_CHAR_TO_BLOCK(s, (uint32)(*((uchar *)(s->strm->next_in))));
			s->strm->next_in++;
			s->strm->avail_in--;
			s->strm->TotalIn++;
			s->avail_in_expect--;
		}
	}
	return progress_in;
}

static bool copy_output_until_stop(EState* s)
{
	bool progress_out = false;
	while(true) {
		/*-- no output space? --*/
		if(s->strm->avail_out == 0) 
			break;
		/*-- block done? --*/
		if(s->state_out_pos >= s->numZ) 
			break;
		progress_out = true;
		*(s->strm->next_out) = s->zbits[s->state_out_pos];
		s->state_out_pos++;
		s->strm->avail_out--;
		s->strm->next_out++;
		s->strm->TotalOut++;
	}
	return progress_out;
}

static bool handle_compress(bz_stream* strm)
{
	bool progress_in  = false;
	bool progress_out = false;
	EState * s = (EState *)strm->state;
	while(true) {
		if(s->state == BZ_S_OUTPUT) {
			progress_out |= copy_output_until_stop(s);
			if(s->state_out_pos < s->numZ) 
				break;
			if(s->mode == BZ_M_FINISHING && s->avail_in_expect == 0 && isempty_RL(s)) 
				break;
			prepare_new_block(s);
			s->state = BZ_S_INPUT;
			if(s->mode == BZ_M_FLUSHING && s->avail_in_expect == 0 && isempty_RL(s)) 
				break;
		}
		if(s->state == BZ_S_INPUT) {
			progress_in |= copy_input_until_stop(s);
			if(s->mode != BZ_M_RUNNING && s->avail_in_expect == 0) {
				flush_RL(s);
				BZ2_compressBlock(s, (bool)(s->mode == BZ_M_FINISHING));
				s->state = BZ_S_OUTPUT;
			}
			else if(s->nblock >= s->nblockMAX) {
				BZ2_compressBlock(s, false);
				s->state = BZ_S_OUTPUT;
			}
			else if(s->strm->avail_in == 0) {
				break;
			}
		}
	}
	return (progress_in || progress_out);
}

int BZ2_bzCompress(bz_stream *strm, int action)
{
	bool progress;
	EState * s;
	if(strm == NULL) 
		return BZ_PARAM_ERROR;
	s = (EState *)strm->state;
	if(!s) 
		return BZ_PARAM_ERROR;
	if(s->strm != strm) 
		return BZ_PARAM_ERROR;
preswitch:
	switch(s->mode) {
		case BZ_M_IDLE:
		    return BZ_SEQUENCE_ERROR;
		case BZ_M_RUNNING:
		    if(action == BZ_RUN) {
			    progress = handle_compress(strm);
			    return progress ? BZ_RUN_OK : BZ_PARAM_ERROR;
		    }
		    else if(action == BZ_FLUSH) {
			    s->avail_in_expect = strm->avail_in;
			    s->mode = BZ_M_FLUSHING;
			    goto preswitch;
		    }
		    else if(action == BZ_FINISH) {
			    s->avail_in_expect = strm->avail_in;
			    s->mode = BZ_M_FINISHING;
			    goto preswitch;
		    }
		    else
			    return BZ_PARAM_ERROR;
		case BZ_M_FLUSHING:
		    if(action != BZ_FLUSH) 
				return BZ_SEQUENCE_ERROR;
		    if(s->avail_in_expect != s->strm->avail_in)
			    return BZ_SEQUENCE_ERROR;
		    progress = handle_compress(strm);
		    if(s->avail_in_expect > 0 || !isempty_RL(s) || s->state_out_pos < s->numZ) 
				return BZ_FLUSH_OK;
		    s->mode = BZ_M_RUNNING;
		    return BZ_RUN_OK;
		case BZ_M_FINISHING:
		    if(action != BZ_FINISH) 
				return BZ_SEQUENCE_ERROR;
		    if(s->avail_in_expect != s->strm->avail_in)
			    return BZ_SEQUENCE_ERROR;
		    progress = handle_compress(strm);
		    if(!progress) 
				return BZ_SEQUENCE_ERROR;
		    if(s->avail_in_expect > 0 || !isempty_RL(s) || s->state_out_pos < s->numZ) 
				return BZ_FINISH_OK;
		    s->mode = BZ_M_IDLE;
		    return BZ_STREAM_END;
	}
	return BZ_OK; /*--not reached--*/
}

int BZ2_bzCompressEnd(bz_stream *strm)
{
	if(!strm) 
		return BZ_PARAM_ERROR;
	else {
		EState * s = (EState *)strm->state;
		if(!s) 
			return BZ_PARAM_ERROR;
		if(s->strm != strm) 
			return BZ_PARAM_ERROR;
		SAlloc::F(s->arr1);
		SAlloc::F(s->arr2);
		SAlloc::F(s->ftab);
		SAlloc::F(strm->state);
		strm->state = NULL;
		return BZ_OK;
	}
}
// 
// Decompression stuff
// 
int BZ2_bzDecompressInit(bz_stream* strm, int verbosity, int small)
{
	DState* s;
	if(strm == NULL) return BZ_PARAM_ERROR;
	if(small != 0 && small != 1) return BZ_PARAM_ERROR;
	if(verbosity < 0 || verbosity > 4) return BZ_PARAM_ERROR;
	//if(strm->bzalloc == NULL) strm->bzalloc = default_bzalloc;
	//if(strm->bzfree == NULL) strm->bzfree = default_bzfree;
	s = (DState *)SAlloc::M(sizeof(DState));
	if(!s) 
		return BZ_MEM_ERROR;
	s->strm          = strm;
	strm->state      = s;
	s->state         = BZ_X_MAGIC_1;
	s->bsLive        = 0;
	s->bsBuff        = 0;
	s->calculatedCombinedCRC = 0;
	//strm->total_in_lo32      = 0;
	//strm->total_in_hi32      = 0;
	//strm->total_out_lo32     = 0;
	//strm->total_out_hi32     = 0;
	strm->TotalIn = 0;
	strm->TotalOut = 0;
	s->smallDecompress = small ? true : false;
	s->ll4           = NULL;
	s->ll16          = NULL;
	s->tt            = NULL;
	s->currBlockNo   = 0;
	s->verbosity     = verbosity;
	return BZ_OK;
}
// 
// Return  true iff data corruption is discovered.
// Returns false if there is no problem.
// 
static bool unRLE_obuf_to_output_FAST(DState* s)
{
	uchar k1;
	if(s->blockRandomised) {
		while(true) {
			/* try to finish existing run */
			while(true) {
				if(s->strm->avail_out == 0) 
					return false;
				if(s->state_out_len == 0) 
					break;
				*reinterpret_cast<uchar *>(s->strm->next_out) = s->state_out_ch;
				BZ_UPDATE_CRC(s->calculatedBlockCRC, s->state_out_ch);
				s->state_out_len--;
				s->strm->next_out++;
				s->strm->avail_out--;
				s->strm->TotalOut++;
			}
			/* can a new run be started? */
			if(s->nblock_used == s->save_nblock+1) 
				return false;
			/* Only caused by corrupt data stream? */
			if(s->nblock_used > s->save_nblock+1)
				return true;
			s->state_out_len = 1;
			s->state_out_ch = (uchar)(s->k0);
			BZ_GET_FAST(k1); BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			s->state_out_len = 2;
			BZ_GET_FAST(k1); BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			s->state_out_len = 3;
			BZ_GET_FAST(k1); BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			BZ_GET_FAST(k1); 
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; 
			s->nblock_used++;
			s->state_out_len = ((int32)k1) + 4;
			BZ_GET_FAST(s->k0); 
			BZ_RAND_UPD_MASK;
			s->k0 ^= BZ_RAND_MASK; 
			s->nblock_used++;
		}
	}
	else {
		/* restore */
		uint32 c_calculatedBlockCRC = s->calculatedBlockCRC;
		uchar c_state_out_ch       = s->state_out_ch;
		int32 c_state_out_len      = s->state_out_len;
		int32 c_nblock_used        = s->nblock_used;
		int32 c_k0         = s->k0;
		uint32 * c_tt      = s->tt;
		uint32 c_tPos      = s->tPos;
		char   * cs_next_out       = s->strm->next_out;
		uint cs_avail_out = s->strm->avail_out;
		int32 ro_blockSize100k     = s->blockSize100k;
		/* end restore */
		uint32 avail_out_INIT = cs_avail_out;
		int32 s_save_nblockPP = s->save_nblock+1;
		//uint total_out_lo32_old;
		while(true) {
			/* try to finish existing run */
			if(c_state_out_len > 0) {
				while(true) {
					if(cs_avail_out == 0) 
						goto return_notr;
					if(c_state_out_len == 1) 
						break;
					*((uchar *)(cs_next_out)) = c_state_out_ch;
					BZ_UPDATE_CRC(c_calculatedBlockCRC, c_state_out_ch);
					c_state_out_len--;
					cs_next_out++;
					cs_avail_out--;
				}
s_state_out_len_eq_one:
				{
					if(cs_avail_out == 0) {
						c_state_out_len = 1; goto return_notr;
					}
					*((uchar *)(cs_next_out)) = c_state_out_ch;
					BZ_UPDATE_CRC(c_calculatedBlockCRC, c_state_out_ch);
					cs_next_out++;
					cs_avail_out--;
				}
			}
			/* Only caused by corrupt data stream? */
			if(c_nblock_used > s_save_nblockPP)
				return true;
			/* can a new run be started? */
			if(c_nblock_used == s_save_nblockPP) {
				c_state_out_len = 0; 
				goto return_notr;
			}
			c_state_out_ch = (uchar)c_k0;
			BZ_GET_FAST_C(k1); 
			c_nblock_used++;
			if(k1 != c_k0) {
				c_k0 = k1; 
				goto s_state_out_len_eq_one;
			}
			if(c_nblock_used == s_save_nblockPP)
				goto s_state_out_len_eq_one;
			c_state_out_len = 2;
			BZ_GET_FAST_C(k1); c_nblock_used++;
			if(c_nblock_used == s_save_nblockPP) 
				continue;
			if(k1 != c_k0) {
				c_k0 = k1; 
				continue;
			}
			c_state_out_len = 3;
			BZ_GET_FAST_C(k1); c_nblock_used++;
			if(c_nblock_used == s_save_nblockPP) 
				continue;
			if(k1 != c_k0) {
				c_k0 = k1; 
				continue;
			}
			BZ_GET_FAST_C(k1); 
			c_nblock_used++;
			c_state_out_len = ((int32)k1) + 4;
			BZ_GET_FAST_C(c_k0); 
			c_nblock_used++;
		}
return_notr:
		//total_out_lo32_old = s->strm->total_out_lo32;
		//s->strm->total_out_lo32 += (avail_out_INIT - cs_avail_out);
		//if(s->strm->total_out_lo32 < total_out_lo32_old)
			//s->strm->total_out_hi32++;
		s->strm->TotalOut += (avail_out_INIT - cs_avail_out);
		// save
		s->calculatedBlockCRC = c_calculatedBlockCRC;
		s->state_out_ch       = c_state_out_ch;
		s->state_out_len      = c_state_out_len;
		s->nblock_used        = c_nblock_used;
		s->k0         = c_k0;
		s->tt         = c_tt;
		s->tPos       = c_tPos;
		s->strm->next_out     = cs_next_out;
		s->strm->avail_out    = cs_avail_out;
		/* end save */
	}
	return false;
}

__inline__ int32 BZ2_indexIntoF(int32 indx, int32 * cftab)
{
	int32 nb = 0;
	int32 na = 256;
	do {
		int32 mid = (nb + na) >> 1;
		if(indx >= cftab[mid]) 
			nb = mid; 
		else 
			na = mid;
	} while((na - nb) != 1);
	return nb;
}
// 
// Return  true iff data corruption is discovered.
// Returns false if there is no problem.
// 
static bool unRLE_obuf_to_output_SMALL(DState* s)
{
	uchar k1;
	if(s->blockRandomised) {
		while(true) {
			/* try to finish existing run */
			while(true) {
				if(s->strm->avail_out == 0) 
					return false;
				if(s->state_out_len == 0) 
					break;
				*((uchar *)(s->strm->next_out)) = s->state_out_ch;
				BZ_UPDATE_CRC(s->calculatedBlockCRC, s->state_out_ch);
				s->state_out_len--;
				s->strm->next_out++;
				s->strm->avail_out--;
				s->strm->TotalOut++;
			}
			/* can a new run be started? */
			if(s->nblock_used == s->save_nblock+1) 
				return false;
			/* Only caused by corrupt data stream? */
			if(s->nblock_used > s->save_nblock+1)
				return true;
			s->state_out_len = 1;
			s->state_out_ch = (uchar)s->k0;
			BZ_GET_SMALL(k1); 
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; 
			s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			s->state_out_len = 2;
			BZ_GET_SMALL(k1); 
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; 
			s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			s->state_out_len = 3;
			BZ_GET_SMALL(k1); 
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; 
			s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			BZ_GET_SMALL(k1); 
			BZ_RAND_UPD_MASK;
			k1 ^= BZ_RAND_MASK; 
			s->nblock_used++;
			s->state_out_len = ((int32)k1) + 4;
			BZ_GET_SMALL(s->k0); 
			BZ_RAND_UPD_MASK;
			s->k0 ^= BZ_RAND_MASK; 
			s->nblock_used++;
		}
	}
	else {
		while(true) {
			/* try to finish existing run */
			while(true) {
				if(s->strm->avail_out == 0) 
					return false;
				if(s->state_out_len == 0) 
					break;
				*((uchar *)(s->strm->next_out)) = s->state_out_ch;
				BZ_UPDATE_CRC(s->calculatedBlockCRC, s->state_out_ch);
				s->state_out_len--;
				s->strm->next_out++;
				s->strm->avail_out--;
				s->strm->TotalOut++;
			}
			// can a new run be started?
			if(s->nblock_used == s->save_nblock+1) 
				return false;
			// Only caused by corrupt data stream?
			if(s->nblock_used > s->save_nblock+1)
				return true;
			s->state_out_len = 1;
			s->state_out_ch = static_cast<uchar>(s->k0);
			BZ_GET_SMALL(k1); 
			s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			s->state_out_len = 2;
			BZ_GET_SMALL(k1); 
			s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			s->state_out_len = 3;
			BZ_GET_SMALL(k1); 
			s->nblock_used++;
			if(s->nblock_used == s->save_nblock+1) 
				continue;
			if(k1 != s->k0) {
				s->k0 = k1; 
				continue;
			}
			BZ_GET_SMALL(k1); 
			s->nblock_used++;
			s->state_out_len = ((int32)k1) + 4;
			BZ_GET_SMALL(s->k0); 
			s->nblock_used++;
		}
	}
}

int BZ2_bzDecompress(bz_stream *strm)
{
	bool corrupt;
	DState* s;
	if(strm == NULL) 
		return BZ_PARAM_ERROR;
	s = (DState *)strm->state;
	if(!s) 
		return BZ_PARAM_ERROR;
	if(s->strm != strm) 
		return BZ_PARAM_ERROR;
	while(true) {
		if(s->state == BZ_X_IDLE) 
			return BZ_SEQUENCE_ERROR;
		if(s->state == BZ_X_OUTPUT) {
			corrupt = s->smallDecompress ? unRLE_obuf_to_output_SMALL(s) : unRLE_obuf_to_output_FAST(s);
			if(corrupt) 
				return BZ_DATA_ERROR;
			if(s->nblock_used == s->save_nblock+1 && s->state_out_len == 0) {
				BZ_FINALISE_CRC(s->calculatedBlockCRC);
				if(s->verbosity >= 3)
					VPrintf2(" {0x%08x, 0x%08x}", s->storedBlockCRC, s->calculatedBlockCRC);
				if(s->verbosity >= 2) 
					VPrintf0("]");
				if(s->calculatedBlockCRC != s->storedBlockCRC)
					return BZ_DATA_ERROR;
				s->calculatedCombinedCRC = (s->calculatedCombinedCRC << 1) | (s->calculatedCombinedCRC >> 31);
				s->calculatedCombinedCRC ^= s->calculatedBlockCRC;
				s->state = BZ_X_BLKHDR_1;
			}
			else {
				return BZ_OK;
			}
		}
		if(s->state >= BZ_X_MAGIC_1) {
			int32 r = BZ2_decompress(s);
			if(r == BZ_STREAM_END) {
				if(s->verbosity >= 3)
					VPrintf2("\n    combined CRCs: stored = 0x%08x, computed = 0x%08x", s->storedCombinedCRC, s->calculatedCombinedCRC);
				if(s->calculatedCombinedCRC != s->storedCombinedCRC)
					return BZ_DATA_ERROR;
				return r;
			}
			if(s->state != BZ_X_OUTPUT) return r;
		}
	}
	assert(0/*, 6001*/);
	return 0; /*NOTREACHED*/
}

int BZ2_bzDecompressEnd(bz_stream *strm)
{
	DState * s;
	if(strm == NULL) 
		return BZ_PARAM_ERROR;
	s = (DState *)strm->state;
	if(!s) 
		return BZ_PARAM_ERROR;
	if(s->strm != strm) 
		return BZ_PARAM_ERROR;
	SAlloc::F(s->tt);
	SAlloc::F(s->ll16);
	SAlloc::F(s->ll4);
	SAlloc::F(strm->state);
	strm->state = NULL;
	return BZ_OK;
}

#ifndef BZ_NO_STDIO
// 
// File I/O stuff
// 
#define BZ_SETERR(eee) { ASSIGN_PTR(bzerror, eee); if(bzf) bzf->lastErr = eee; }

typedef struct {
	FILE * handle;
	char   buf[BZ_MAX_UNUSED];
	int32  bufN;
	bool   writing;
	bz_stream strm;
	int32  lastErr;
	bool   initialisedOk;
} bzFile;

static bool FASTCALL myfeof(FILE* f)
{
	const int32 c = fgetc(f);
	if(c == EOF) 
		return true;
	else {
		ungetc(c, f);
		return false;
	}
}

BZFILE* BZ2_bzWriteOpen(int*  bzerror, FILE* f, int blockSize100k, int verbosity, int workFactor)
{
	int32 ret;
	bzFile * bzf = NULL;
	BZ_SETERR(BZ_OK);
	if(!f || (blockSize100k < 1 || blockSize100k > 9) || (workFactor < 0 || workFactor > 250) || (verbosity < 0 || verbosity > 4)) {
		BZ_SETERR(BZ_PARAM_ERROR); return NULL;
	}
	if(ferror(f)) {
		BZ_SETERR(BZ_IO_ERROR); return NULL;
	}
	bzf = static_cast<bzFile *>(SAlloc::M(sizeof(bzFile)));
	if(bzf == NULL) {
		BZ_SETERR(BZ_MEM_ERROR); return NULL;
	}
	BZ_SETERR(BZ_OK);
	bzf->initialisedOk = false;
	bzf->bufN  = 0;
	bzf->handle        = f;
	bzf->writing       = true;
	//bzf->strm.bzalloc  = NULL;
	//bzf->strm.bzfree   = NULL;
	//bzf->strm.opaque   = NULL;
	if(workFactor == 0) 
		workFactor = 30;
	ret = BZ2_bzCompressInit(&(bzf->strm), blockSize100k, verbosity, workFactor);
	if(ret != BZ_OK) {
		BZ_SETERR(ret); 
		SAlloc::F(bzf); 
		return NULL;
	}
	bzf->strm.avail_in = 0;
	bzf->initialisedOk = true;
	return bzf;
}

void BZ2_bzWrite(int * bzerror, BZFILE * b, void * buf, int len)
{
	int32 n, n2, ret;
	bzFile * bzf = (bzFile*)b;
	BZ_SETERR(BZ_OK);
	if(bzf == NULL || buf == NULL || len < 0) {
		BZ_SETERR(BZ_PARAM_ERROR); 
		return;
	}
	if(!(bzf->writing)) {
		BZ_SETERR(BZ_SEQUENCE_ERROR); 
		return;
	}
	if(ferror(bzf->handle)) {
		BZ_SETERR(BZ_IO_ERROR); 
		return;
	}
	if(!len) {
		BZ_SETERR(BZ_OK); 
		return;
	}
	bzf->strm.avail_in = len;
	bzf->strm.next_in  = (char *)buf;
	while(true) {
		bzf->strm.avail_out = BZ_MAX_UNUSED;
		bzf->strm.next_out = bzf->buf;
		ret = BZ2_bzCompress(&(bzf->strm), BZ_RUN);
		if(ret != BZ_RUN_OK) {
			BZ_SETERR(ret); 
			return;
		}
		if(bzf->strm.avail_out < BZ_MAX_UNUSED) {
			n = BZ_MAX_UNUSED - bzf->strm.avail_out;
			n2 = static_cast<int32>(fwrite((void *)(bzf->buf), sizeof(uchar), n, bzf->handle));
			if(n != n2 || ferror(bzf->handle)) {
				BZ_SETERR(BZ_IO_ERROR); 
				return;
			}
		}
		if(bzf->strm.avail_in == 0) {
			BZ_SETERR(BZ_OK); 
			return;
		}
	}
}

void BZ2_bzWriteClose(int * bzerror, BZFILE * b, int abandon, uint * pNBytesIn, uint * pNBytesOut)
{
	uint64 bytes_in = 0;
	uint64 bytes_out = 0;
	BZ2_bzWriteClose64(bzerror, b, abandon, &bytes_in, &bytes_out);
	ASSIGN_PTR(pNBytesIn, static_cast<uint32>(bytes_in));
	ASSIGN_PTR(pNBytesOut, static_cast<uint32>(bytes_out));
}

void BZ2_bzWriteClose64(int * bzerror, BZFILE * b, int abandon, uint64 * pNBytesIn, uint64 * pNBytesOut)
{
	int32 n, n2, ret;
	bzFile * bzf = (bzFile*)b;
	if(bzf == NULL) {
		BZ_SETERR(BZ_OK); return;
	}
	if(!(bzf->writing)) {
		BZ_SETERR(BZ_SEQUENCE_ERROR); return;
	}
	if(ferror(bzf->handle)) {
		BZ_SETERR(BZ_IO_ERROR); return;
	}
	ASSIGN_PTR(pNBytesIn, 0);
	ASSIGN_PTR(pNBytesOut, 0);
	if((!abandon) && bzf->lastErr == BZ_OK) {
		while(true) {
			bzf->strm.avail_out = BZ_MAX_UNUSED;
			bzf->strm.next_out = bzf->buf;
			ret = BZ2_bzCompress(&(bzf->strm), BZ_FINISH);
			if(ret != BZ_FINISH_OK && ret != BZ_STREAM_END) {
				BZ_SETERR(ret); 
				return;
			}
			if(bzf->strm.avail_out < BZ_MAX_UNUSED) {
				n = BZ_MAX_UNUSED - bzf->strm.avail_out;
				n2 = static_cast<int32>(fwrite((void *)(bzf->buf), sizeof(uchar), n, bzf->handle));
				if(n != n2 || ferror(bzf->handle)) {
					BZ_SETERR(BZ_IO_ERROR); return;
				}
			}
			if(ret == BZ_STREAM_END) 
				break;
		}
	}
	if(!abandon && !ferror(bzf->handle)) {
		fflush(bzf->handle);
		if(ferror(bzf->handle)) {
			BZ_SETERR(BZ_IO_ERROR); return;
		}
	}
	ASSIGN_PTR(pNBytesIn, bzf->strm.TotalIn);
	ASSIGN_PTR(pNBytesOut, bzf->strm.TotalOut);
	BZ_SETERR(BZ_OK);
	BZ2_bzCompressEnd(&(bzf->strm));
	SAlloc::F(bzf);
}

BZFILE* BZ2_bzReadOpen(int*  bzerror, FILE* f, int verbosity, int small, void * unused, int nUnused)
{
	bzFile * bzf = NULL;
	int ret;
	BZ_SETERR(BZ_OK);
	if(!f || (small != 0 && small != 1) || (verbosity < 0 || verbosity > 4) ||
	    (!unused && nUnused != 0) || (unused && (nUnused < 0 || nUnused > BZ_MAX_UNUSED))) {
		BZ_SETERR(BZ_PARAM_ERROR); return NULL;
	}
	if(ferror(f)) {
		BZ_SETERR(BZ_IO_ERROR); return NULL;
	}
	bzf = static_cast<bzFile *>(SAlloc::M(sizeof(bzFile)));
	if(bzf == NULL) {
		BZ_SETERR(BZ_MEM_ERROR); return NULL;
	}
	BZ_SETERR(BZ_OK);
	bzf->initialisedOk = false;
	bzf->handle  = f;
	bzf->bufN    = 0;
	bzf->writing = false;
	//bzf->strm.bzalloc  = NULL;
	//bzf->strm.bzfree   = NULL;
	//bzf->strm.opaque   = NULL;
	while(nUnused > 0) {
		bzf->buf[bzf->bufN] = *((uchar *)(unused)); bzf->bufN++;
		unused = ((void *)( 1 + ((uchar *)(unused))  ));
		nUnused--;
	}
	ret = BZ2_bzDecompressInit(&(bzf->strm), verbosity, small);
	if(ret != BZ_OK) {
		BZ_SETERR(ret); 
		SAlloc::F(bzf); 
		return NULL;
	}
	bzf->strm.avail_in = bzf->bufN;
	bzf->strm.next_in  = bzf->buf;
	bzf->initialisedOk = true;
	return bzf;
}

void BZ2_bzReadClose(int * bzerror, BZFILE *b)
{
	bzFile * bzf = (bzFile *)b;
	BZ_SETERR(BZ_OK);
	if(bzf == NULL) {
		BZ_SETERR(BZ_OK); return;
	}
	if(bzf->writing) {
		BZ_SETERR(BZ_SEQUENCE_ERROR); return;
	}
	if(bzf->initialisedOk)
		BZ2_bzDecompressEnd(&(bzf->strm));
	SAlloc::F(bzf);
}

int BZ2_bzRead(int * bzerror, BZFILE* b, void *   buf, int len)
{
	int32 n, ret;
	bzFile * bzf = (bzFile*)b;
	BZ_SETERR(BZ_OK);
	if(bzf == NULL || buf == NULL || len < 0) {
		BZ_SETERR(BZ_PARAM_ERROR); return 0;
	}
	if(bzf->writing) {
		BZ_SETERR(BZ_SEQUENCE_ERROR); return 0;
	}
	if(!len) {
		BZ_SETERR(BZ_OK); return 0;
	}
	bzf->strm.avail_out = len;
	bzf->strm.next_out = (char *)buf;
	while(true) {
		if(ferror(bzf->handle)) {
			BZ_SETERR(BZ_IO_ERROR); return 0;
		}
		if(bzf->strm.avail_in == 0 && !myfeof(bzf->handle)) {
			n = static_cast<int32>(fread(bzf->buf, sizeof(uchar), BZ_MAX_UNUSED, bzf->handle));
			if(ferror(bzf->handle)) {
				BZ_SETERR(BZ_IO_ERROR); return 0;
			}
			bzf->bufN = n;
			bzf->strm.avail_in = bzf->bufN;
			bzf->strm.next_in = bzf->buf;
		}
		ret = BZ2_bzDecompress(&(bzf->strm));
		if(ret != BZ_OK && ret != BZ_STREAM_END) {
			BZ_SETERR(ret); 
			return 0;
		}
		if(ret == BZ_OK && myfeof(bzf->handle) && bzf->strm.avail_in == 0 && bzf->strm.avail_out > 0) {
			BZ_SETERR(BZ_UNEXPECTED_EOF); 
			return 0;
		}
		if(ret == BZ_STREAM_END) {
			BZ_SETERR(BZ_STREAM_END);
			return len - bzf->strm.avail_out;
		}
		if(bzf->strm.avail_out == 0) {
			BZ_SETERR(BZ_OK); return len;
		}
	}
	return 0; /*not reached*/
}

void BZ2_bzReadGetUnused(int * bzerror, BZFILE * b, const void ** unused, int * nUnused)
{
	bzFile* bzf = (bzFile*)b;
	if(bzf == NULL) {
		BZ_SETERR(BZ_PARAM_ERROR); return;
	}
	if(bzf->lastErr != BZ_STREAM_END) {
		BZ_SETERR(BZ_SEQUENCE_ERROR); return;
	}
	if(unused == NULL || nUnused == NULL) {
		BZ_SETERR(BZ_PARAM_ERROR); return;
	}
	BZ_SETERR(BZ_OK);
	*nUnused = bzf->strm.avail_in;
	*unused = bzf->strm.next_in;
}
#endif
// 
// Misc convenience stuff
// 
int BZ2_bzBuffToBuffCompress(char * dest, uint * destLen, char * source, uint sourceLen, int blockSize100k, int verbosity, int workFactor)
{
	bz_stream strm;
	int ret;
	if(dest == NULL || destLen == NULL || source == NULL || blockSize100k < 1 || blockSize100k > 9 || verbosity < 0 || verbosity > 4 || workFactor < 0 || workFactor > 250)
		return BZ_PARAM_ERROR;
	SETIFZQ(workFactor, 30);
	//strm.bzalloc = NULL;
	//strm.bzfree = NULL;
	//strm.opaque = NULL;
	ret = BZ2_bzCompressInit(&strm, blockSize100k, verbosity, workFactor);
	if(ret != BZ_OK) 
		return ret;
	strm.next_in = source;
	strm.next_out = dest;
	strm.avail_in = sourceLen;
	strm.avail_out = *destLen;
	ret = BZ2_bzCompress(&strm, BZ_FINISH);
	if(ret == BZ_FINISH_OK) 
		goto output_overflow;
	if(ret != BZ_STREAM_END) 
		goto errhandler;
	/* normal termination */
	*destLen -= strm.avail_out;
	BZ2_bzCompressEnd(&strm);
	return BZ_OK;
output_overflow:
	BZ2_bzCompressEnd(&strm);
	return BZ_OUTBUFF_FULL;
errhandler:
	BZ2_bzCompressEnd(&strm);
	return ret;
}

int BZ2_bzBuffToBuffDecompress(char * dest, uint* destLen, char* source, uint sourceLen, int small, int verbosity)
{
	bz_stream strm;
	int ret;
	if(dest == NULL || destLen == NULL || source == NULL || (small != 0 && small != 1) || verbosity < 0 || verbosity > 4)
		return BZ_PARAM_ERROR;
	//strm.bzalloc = NULL;
	//strm.bzfree = NULL;
	//strm.opaque = NULL;
	ret = BZ2_bzDecompressInit(&strm, verbosity, small);
	if(ret != BZ_OK) 
		return ret;
	strm.next_in = source;
	strm.next_out = dest;
	strm.avail_in = sourceLen;
	strm.avail_out = *destLen;
	ret = BZ2_bzDecompress(&strm);
	if(ret == BZ_OK) 
		goto output_overflow_or_eof;
	if(ret != BZ_STREAM_END) 
		goto errhandler;
	/* normal termination */
	*destLen -= strm.avail_out;
	BZ2_bzDecompressEnd(&strm);
	return BZ_OK;
output_overflow_or_eof:
	if(strm.avail_out > 0) {
		BZ2_bzDecompressEnd(&strm);
		return BZ_UNEXPECTED_EOF;
	}
	else {
		BZ2_bzDecompressEnd(&strm);
		return BZ_OUTBUFF_FULL;
	};
errhandler:
	BZ2_bzDecompressEnd(&strm);
	return ret;
}
// 
// Code contributed by Yoshioka Tsuneo (tsuneo@rr.iij4u.or.jp)
// to support better zlib compatibility.
// This code is not _officially_ part of libbzip2 (yet);
// I haven't tested it, documented it, or considered the
// threading-safeness of it.
// If this code breaks, please contact both Yoshioka and me.
// 
// return version like "0.9.5d, 4-Sept-1999".
//
const char * BZ2_bzlibVersion(void) { return BZ_VERSION; }

#ifndef BZ_NO_STDIO
#if defined(_WIN32) || defined(OS2) || defined(MSDOS)
	#define SET_BINARY_MODE(file) _setmode(_fileno(file), O_BINARY)
#else
	#define SET_BINARY_MODE(file)
#endif
static BZFILE * bzopen_or_bzdopen(const char * path/* no use when bzdopen */, int fd/* no use when bzdopen */, const char * mode, int open_mode/* bzopen: 0, bzdopen:1 */)
{
	BZFILE * bzfp = NULL;
	int bzerr;
	char unused[BZ_MAX_UNUSED];
	int blockSize100k = 9;
	int writing       = 0;
	char mode2[10]    = "";
	FILE * fp   = NULL;
	int verbosity     = 0;
	int workFactor    = 30;
	int smallMode     = 0;
	int nUnused       = 0;
	if(mode) { 
		while(*mode) {
			switch(*mode) {
				case 'r': writing = 0; break;
				case 'w': writing = 1; break;
				case 's': smallMode = 1; break;
				default:
					if(isdec((int)(*mode))) {
						blockSize100k = *mode-BZ_HDR_0;
					}
			}
			mode++;
		}
		strcat(mode2, writing ? "w" : "r");
		strcat(mode2, "b"/* binary mode */); 
		if(open_mode==0) {
			if(isempty(path)) {
				fp = (writing ? stdout : stdin);
				SET_BINARY_MODE(fp);
			}
			else {
				fp = fopen(path, mode2);
			}
		}
		else {
	#ifdef BZ_STRICT_ANSI
			fp = NULL;
	#else
			fp = _fdopen(fd, mode2);
	#endif
		}
		if(fp) {
			if(writing) {
				// Guard against total chaos and anarchy -- JRS
				SETMAX(blockSize100k, 1);
				SETMIN(blockSize100k, 9);
				bzfp = BZ2_bzWriteOpen(&bzerr, fp, blockSize100k, verbosity, workFactor);
			}
			else {
				bzfp = BZ2_bzReadOpen(&bzerr, fp, verbosity, smallMode, unused, nUnused);
			}
			if(!bzfp) {
				if(fp != stdin && fp != stdout) 
					fclose(fp);
			}
		}
	}
	return bzfp;
}
// 
// open file for read or write.
// ex) bzopen("file","w9")
// case path="" or NULL => use stdin or stdout.
// 
BZFILE * BZ2_bzopen(const char * path, const char * mode) { return bzopen_or_bzdopen(path, -1, mode, /*bzopen*/ 0); }
BZFILE * BZ2_bzdopen(int fd, const char * mode) { return bzopen_or_bzdopen(NULL, fd, mode, /*bzdopen*/ 1); }

int BZ2_bzread(BZFILE* b, void * buf, int len)
{
	int bzerr, nread;
	if(static_cast<const bzFile *>(b)->lastErr == BZ_STREAM_END) 
		return 0;
	nread = BZ2_bzRead(&bzerr, b, buf, len);
	return (bzerr == BZ_OK || bzerr == BZ_STREAM_END) ? nread : -1;
}

int BZ2_bzwrite(BZFILE* b, void * buf, int len)
{
	int bzerr;
	BZ2_bzWrite(&bzerr, b, buf, len);
	return (bzerr == BZ_OK) ? len : -1;
}

int BZ2_bzflush(BZFILE *b) { return 0; /* do nothing now... */ }

void BZ2_bzclose(BZFILE * b)
{
	if(b) {
		FILE * fp = static_cast<bzFile *>(b)->handle;
		int bzerr;
		if(static_cast<const bzFile *>(b)->writing) {
			BZ2_bzWriteClose(&bzerr, b, 0, NULL, NULL);
			if(bzerr != BZ_OK) {
				BZ2_bzWriteClose(NULL, b, 1, NULL, NULL);
			}
		}
		else {
			BZ2_bzReadClose(&bzerr, b);
		}
		if(fp!=stdin && fp!=stdout) {
			fclose(fp);
		}
	}
}
// 
// return last error code
// 
static const char * bzerrorstrings[] = {
	"OK", 
	"SEQUENCE_ERROR", 
	"PARAM_ERROR", 
	"MEM_ERROR", 
	"DATA_ERROR", 
	"DATA_ERROR_MAGIC", 
	"IO_ERROR", 
	"UNEXPECTED_EOF", 
	"OUTBUFF_FULL", 
	"CONFIG_ERROR", 
	"???" /* for future */, "???" /* for future */, "???" /* for future */, "???" /* for future */, "???" /* for future */, "???" /* for future */
};

const char * BZ2_bzerror(BZFILE *b, int * errnum)
{
	int err = static_cast<bzFile *>(b)->lastErr;
	if(err > 0) 
		err = 0;
	*errnum = err;
	return bzerrorstrings[err* -1];
}
#endif
