/* 
	ucl.h -- prototypes for the UCL data compression library
	This file is part of the UCL data compression library.
	Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
*/
#ifndef __UCL_H_INCLUDED
#define __UCL_H_INCLUDED

#ifndef __UCLCONF_H_INCLUDED
#include <ucl/uclconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
// Compression fine-tuning configuration.
//
// Pass a NULL pointer to the compression functions for default values.
// Otherwise set all values to -1 [i.e. initialize the struct by a
// `memset(x,0xff,sizeof(x))'] and then set the required values.
************************************************************************/

struct ucl_compress_config_t
{
    int bb_endian; // @sobolev not used param
    int bb_size;
    ucl_uint max_offset;
    ucl_uint max_match;  // match length
    int s_level;
    int h_level;
    int p_level;
    int c_flags;
    ucl_uint m_size;
};

#define ucl_compress_config_p   ucl_compress_config_t __UCL_MMODEL *

/***********************************************************************
// compressors
//
// Pass NULL for `cb' (no progress callback), `conf' (default compression
// configuration) and `result' (no statistical result).
************************************************************************/

UCL_EXTERN(int) ucl_nrv2b_99_compress(const ucl_bytep src, ucl_uint src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_progress_callback_p cb,
	int level, const struct ucl_compress_config_p conf, ucl_uintp result);

UCL_EXTERN(int) ucl_nrv2d_99_compress(const ucl_bytep src, ucl_uint src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_progress_callback_p cb,
	int level, const struct ucl_compress_config_p conf, ucl_uintp result);

UCL_EXTERN(int) ucl_nrv2e_99_compress(const ucl_bytep src, ucl_uint src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_progress_callback_p cb,
	int level, const struct ucl_compress_config_p conf, ucl_uintp result);

/***********************************************************************
// decompressors
//
// Always pass NULL for `wrkmem'. This parameter is for symetry
// with my other compression libaries and is not used in UCL -
// UCL does not need any additional memory (or even local stack space)
// for decompression.
************************************************************************/

UCL_EXTERN(int) ucl_nrv2b_decompress_8(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_le16(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_le32(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_safe_8( const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_safe_le16(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2b_decompress_safe_le32(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_8(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_le16(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_le32(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_safe_8(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_safe_le16(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2d_decompress_safe_le32(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_8(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_le16(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_le32(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_safe_8(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_safe_le16(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);
UCL_EXTERN(int) ucl_nrv2e_decompress_safe_le32(const ucl_bytep src, ucl_uint  src_len,
	ucl_bytep dst, ucl_uintp dst_len, ucl_voidp wrkmem);

/***********************************************************************
// assembler decompressors [TO BE ADDED]
************************************************************************/

/***********************************************************************
// test an overlapping in-place decompression within a buffer:
//   - try a virtual decompression from &buf[src_off] -> &buf[0]
//   - no data is actually written
//   - only the bytes at buf[src_off..src_off+src_len-1] will get accessed
//
// NOTE: always pass NULL for `wrkmem' - see above.
************************************************************************/

UCL_EXTERN(int)
ucl_nrv2b_test_overlap_8        ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_test_overlap_le16     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_test_overlap_le32     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

UCL_EXTERN(int)
ucl_nrv2d_test_overlap_8        ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_test_overlap_le16     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_test_overlap_le32     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

UCL_EXTERN(int)
ucl_nrv2e_test_overlap_8        ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_test_overlap_le16     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_test_overlap_le32     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

