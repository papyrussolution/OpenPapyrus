/* simple.c -- the annotated simple example program for the UCL library

   This file is part of the UCL data compression library.

   Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The UCL library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The UCL library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the UCL library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/ucl/
 */

#include <ucl/ucl.h>
#if defined(UCL_USE_ASM)
#  include <ucl/ucl_asm.h>
#endif

/* portability layer */
#define WANT_UCL_MALLOC 1
#include "examples/portab.h"


/*************************************************************************
// This program shows the basic usage of the UCL library.
// We will compress a block of data and decompress again.
**************************************************************************/

int __acc_cdecl_main main(int argc, char *argv[])
{
    int r;
    ucl_bytep in;
    ucl_bytep out;
    ucl_uint in_len;
    ucl_uint out_len;
    ucl_uint new_len;
    int level = 5;                  /* compression level (1-10) */

    if (argc < 0 && argv == NULL)   /* avoid warning about unused args */
        return 0;

    printf("\nUCL data compression library (v%s, %s).\n",
            ucl_version_string(), ucl_version_date());
    printf("Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer\n");
    printf("http://www.oberhumer.com/opensource/ucl/\n\n");


/*
 * Step 1: initialize the UCL library
 */
    if (ucl_init() != UCL_E_OK)
    {
        printf("internal error - ucl_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DUCL_DEBUG' for diagnostics)\n");
        return 1;
    }


/*
 * Step 2: setup memory
 *
 * We want to compress the data block at `in' with length `in_len' to
 * the block at `out'. Because the input block may be incompressible,
 * we must provide a little more output space in case that compression
 * is not possible.
 */
    in_len = 256 * 1024L;

#if defined(ACC_MM_AHSHIFT)
    /* reduce memory requirements for ancient 640kB DOS real-mode */
    if (ACC_MM_AHSHIFT != 3)
        in_len = 16 * 1024L;
#endif

    out_len = in_len + in_len / 8 + 256;

    in = (ucl_bytep) ucl_malloc(in_len);
    out = (ucl_bytep) ucl_malloc(out_len);
    if (in == NULL || out == NULL)
    {
        printf("out of memory\n");
        return 2;
    }


/*
 * Step 3: prepare the input block that will get compressed.
 *         We just fill it with zeros in this example program,
 *         but you would use your real-world data here.
 */
    ucl_memset(in, 0, in_len);


/*
 * Step 4: compress from `in' to `out' with UCL NRV2B
 */
    r = ucl_nrv2b_99_compress(in,in_len,out,&out_len,NULL,level,NULL,NULL);
    if (r == UCL_E_OUT_OF_MEMORY)
    {
        printf("out of memory in compress\n");
        return 3;
    }
    if (r == UCL_E_OK)
        printf("compressed %lu bytes into %lu bytes\n",
            (unsigned long) in_len, (unsigned long) out_len);
    else
    {
        /* this should NEVER happen */
        printf("internal error - compression failed: %d\n", r);
        return 4;
    }
    /* check for an incompressible block */
    if (out_len >= in_len)
    {
        printf("This block contains incompressible data.\n");
        return 0;
    }


/*
 * Step 5: decompress again, now going back from `out' to `in'
 */
    new_len = in_len;
#if defined(UCL_USE_ASM)
    r = ucl_nrv2b_decompress_asm_8(out,out_len,in,&new_len,NULL);
#else
    r = ucl_nrv2b_decompress_8(out,out_len,in,&new_len,NULL);
#endif
    if (r == UCL_E_OK && new_len == in_len)
        printf("decompressed %lu bytes back into %lu bytes\n",
            (unsigned long) out_len, (unsigned long) in_len);
    else
    {
        /* this should NEVER happen */
        printf("internal error - decompression failed: %d\n", r);
        return 5;
    }

    ucl_free(out);
    ucl_free(in);
    printf("\n");
#if defined(UCL_USE_ASM)
    printf("i386 assembler version is enabled.\n");
#endif
    printf("Simple compression test passed.\n");
    return 0;
}

/*
vi:ts=4:et
*/

