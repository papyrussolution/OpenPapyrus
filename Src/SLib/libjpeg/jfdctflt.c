/*
 * jfdctflt.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * Modified 2003-2017 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a floating-point implementation of the
 * forward DCT (Discrete Cosine Transform).
 *
 * This implementation should be more accurate than either of the integer
 * DCT implementations.  However, it may not give the same results on all
 * machines because of differences in roundoff behavior.  Speed will depend
 * on the hardware's floating point capacity.
 *
 * A 2-D DCT can be done by 1-D DCT on each row followed by 1-D DCT
 * on each column.  Direct algorithms are also available, but they are
 * much more complex and seem not to be any faster when reduced to code.
 *
 * This implementation is based on Arai, Agui, and Nakajima's algorithm for
 * scaled DCT.  Their original paper (Trans. IEICE E-71(11):1095) is in
 * Japanese, but the algorithm is described in the Pennebaker & Mitchell
 * JPEG textbook (see REFERENCES section in file README).  The following code
 * is based directly on figure 4-8 in P&M.
 * While an 8-point DCT cannot be done in less than 11 multiplies, it is
 * possible to arrange the computation so that many of the multiplies are
 * simple scalings of the final outputs.  These multiplies can then be
 * folded into the multiplications or divisions by the JPEG quantization
 * table entries.  The AA&N method leaves only 5 multiplies and 29 adds
 * to be done in the DCT itself.
 * The primary disadvantage of this method is that with a fixed-point
 * implementation, accuracy is lost due to imprecise representation of the
 * scaled quantization values.  However, that problem does not arise if
 * we use floating point arithmetic.
 */
// @v9c(done)
#include <slib-internal.h>
#pragma hdrstop
#define JPEG_INTERNALS
#include "cdjpeg.h"
#include "jdct.h" // Private declarations for DCT subsystem 

#ifdef DCT_FLOAT_SUPPORTED
/*
 * This module is specialized to the case DCTSIZE = 8.
 */
#if DCTSIZE != 8
	Sorry, this code only copes with 8x8 DCT blocks. /* deliberate syntax err */
#endif
/*
 * Perform the forward DCT on one block of samples.
 *
 * cK represents cos(K*pi/16).
 */
void  jpeg_fdct_float(FAST_FLOAT * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
	FAST_FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	FAST_FLOAT tmp10, tmp11, tmp12, tmp13;
	FAST_FLOAT z1, z2, z3, z4, z5, z11, z13;
	JSAMPROW elemptr;
	int ctr;
	// Pass 1: process rows
	FAST_FLOAT * dataptr = data;
	for(ctr = 0; ctr < DCTSIZE; ctr++) {
		elemptr = sample_data[ctr] + start_col;
		//
		// Load data into workspace 
		//
		tmp0 = (FAST_FLOAT)(GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[7]));
		tmp7 = (FAST_FLOAT)(GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[7]));
		tmp1 = (FAST_FLOAT)(GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[6]));
		tmp6 = (FAST_FLOAT)(GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[6]));
		tmp2 = (FAST_FLOAT)(GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[5]));
		tmp5 = (FAST_FLOAT)(GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[5]));
		tmp3 = (FAST_FLOAT)(GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[4]));
		tmp4 = (FAST_FLOAT)(GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[4]));

		/* Even part */

		tmp10 = tmp0 + tmp3; /* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		/* Apply unsigned->signed conversion. */
		dataptr[0] = tmp10 + tmp11 - 8 * CENTERJSAMPLE; /* phase 3 */
		dataptr[4] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * ((FAST_FLOAT)0.707106781); /* c4 */
		dataptr[2] = tmp13 + z1; /* phase 5 */
		dataptr[6] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5; /* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * ((FAST_FLOAT)0.382683433); /* c6 */
		z2 = ((FAST_FLOAT)0.541196100) * tmp10 + z5; /* c2-c6 */
		z4 = ((FAST_FLOAT)1.306562965) * tmp12 + z5; /* c2+c6 */
		z3 = tmp11 * ((FAST_FLOAT)0.707106781); /* c4 */

		z11 = tmp7 + z3; /* phase 5 */
		z13 = tmp7 - z3;

		dataptr[5] = z13 + z2; /* phase 6 */
		dataptr[3] = z13 - z2;
		dataptr[1] = z11 + z4;
		dataptr[7] = z11 - z4;

		dataptr += DCTSIZE; /* advance pointer to next row */
	}
	//
	// Pass 2: process columns
	//
	dataptr = data;
	for(ctr = DCTSIZE-1; ctr >= 0; ctr--) {
		tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
		tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
		tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
		tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
		tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
		tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
		tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
		tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

		/* Even part */

		tmp10 = tmp0 + tmp3; /* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[DCTSIZE*0] = tmp10 + tmp11; /* phase 3 */
		dataptr[DCTSIZE*4] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * ((FAST_FLOAT)0.707106781); /* c4 */
		dataptr[DCTSIZE*2] = tmp13 + z1; /* phase 5 */
		dataptr[DCTSIZE*6] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5; /* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * ((FAST_FLOAT)0.382683433); /* c6 */
		z2 = ((FAST_FLOAT)0.541196100) * tmp10 + z5; /* c2-c6 */
		z4 = ((FAST_FLOAT)1.306562965) * tmp12 + z5; /* c2+c6 */
		z3 = tmp11 * ((FAST_FLOAT)0.707106781); /* c4 */

		z11 = tmp7 + z3; /* phase 5 */
		z13 = tmp7 - z3;

		dataptr[DCTSIZE*5] = z13 + z2; /* phase 6 */
		dataptr[DCTSIZE*3] = z13 - z2;
		dataptr[DCTSIZE*1] = z11 + z4;
		dataptr[DCTSIZE*7] = z11 - z4;

		dataptr++; /* advance pointer to next column */
	}
}

#endif /* DCT_FLOAT_SUPPORTED */
