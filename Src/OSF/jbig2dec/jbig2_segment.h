/* Copyright (C) 2001-2020 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  1305 Grant Avenue - Suite 200, Novato,
   CA 94945, U.S.A., +1(415)492-9861, for further information.
 */

/*
    jbig2dec
 */

#ifndef _JBIG2_SEGMENT_H
#define _JBIG2_SEGMENT_H

/* segment header routines */

struct _Jbig2Segment {
	uint32 number;
	uint8 flags;
	uint32 page_association;
	size_t data_length;
	int referred_to_segment_count;
	uint32 * referred_to_segments;
	uint32 rows;
	void * result;
};

Jbig2Segment * jbig2_parse_segment_header(Jbig2Ctx * ctx, uint8 * buf, size_t buf_size, size_t * p_header_size);
int jbig2_parse_segment(Jbig2Ctx * ctx, Jbig2Segment * segment, const uint8 * segment_data);
void jbig2_free_segment(Jbig2Ctx * ctx, Jbig2Segment * segment);
Jbig2Segment * FASTCALL jbig2_find_segment(Jbig2Ctx * ctx, uint32 number);

/* region segment info */

typedef struct {
	uint32 width;
	uint32 height;
	uint32 x;
	uint32 y;
	Jbig2ComposeOp op;
	uint8 flags;
} Jbig2RegionSegmentInfo;

void jbig2_get_region_segment_info(Jbig2RegionSegmentInfo * info, const uint8 * segment_data);

#endif /* _JBIG2_SEGMENT_H */
