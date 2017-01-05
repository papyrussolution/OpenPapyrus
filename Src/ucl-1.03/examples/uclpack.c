/* uclpack.c -- example program: a simple file packer

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


/*************************************************************************
// NOTE: this is an example program, so do not use to backup your data.
//
// This program lacks things like sophisticated file handling but is
// pretty complete regarding compression - it should provide a good
// starting point for adaption for you applications.
**************************************************************************/

#include <ucl/ucl.h>
#if defined(UCL_USE_ASM)
#  include <ucl/ucl_asm.h>
#endif
#if 1 && defined(UCL_USE_ASM)
   /* use assembler versions of the decompressors */
#  define ucl_nrv2b_decompress_8        ucl_nrv2b_decompress_asm_8
#  define ucl_nrv2b_decompress_safe_8   ucl_nrv2b_decompress_asm_safe_8
#  define ucl_nrv2d_decompress_8        ucl_nrv2d_decompress_asm_8
#  define ucl_nrv2d_decompress_safe_8   ucl_nrv2d_decompress_asm_safe_8
#  define ucl_nrv2e_decompress_8        ucl_nrv2e_decompress_asm_8
#  define ucl_nrv2e_decompress_safe_8   ucl_nrv2e_decompress_asm_safe_8
#endif

/* benchmark setup - do not introduce floating point dependencies for DOS16 */
#if !defined(WITH_TIMER) && !defined(UCL_OS_DOS16)
#  define WITH_TIMER 1
#endif

/* portability layer */
#define WANT_UCL_MALLOC 1
#define WANT_UCL_FREAD 1
#if defined(WITH_TIMER)
#define WANT_UCL_UCLOCK 1
#endif
#define WANT_UCL_WILDARGV 1
#include "examples/portab.h"


static const char *progname = NULL;

static unsigned long total_in = 0;
static unsigned long total_out = 0;
static int opt_debug = 0;

/* use option `-F' for faster operation - then we don't compute or verify
 * a checksum and always use the fast decompressor */
static ucl_bool opt_fast = 0;

/* magic file header for compressed files */
static const unsigned char magic[8] =
    { 0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a };

/* benchmark */
#if defined(WITH_TIMER)
static ucl_uclock_handle_t uc;
static double benchmark_bytes;
static double benchmark_secs;
#endif


/*************************************************************************
// file IO
**************************************************************************/

ucl_uint xread(FILE *f, ucl_voidp buf, ucl_uint len, ucl_bool allow_eof)
{
    ucl_uint l;

    l = (ucl_uint) ucl_fread(f,buf,len);
    if (l > len)
    {
        fprintf(stderr,"\nsomething's wrong with your C library !!!\n");
        exit(1);
    }
    if (l != len && !allow_eof)
    {
        fprintf(stderr,"\nread error - premature end of file\n");
        exit(1);
    }
    total_in += l;
    return l;
}

ucl_uint xwrite(FILE *f, const ucl_voidp buf, ucl_uint len)
{
    ucl_uint l;

    if (f != NULL)
    {
        l = (ucl_uint) ucl_fwrite(f,buf,len);
        if (l != len)
        {
            fprintf(stderr,"\nwrite error [%ld %ld]  (disk full ?)\n",
                   (long)len, (long)l);
            exit(1);
        }
    }
    total_out += len;
    return len;
}


int xgetc(FILE *f)
{
    unsigned char c;
    xread(f,(ucl_voidp) &c,1,0);
    return c;
}

void xputc(FILE *f, int c)
{
    unsigned char cc = (unsigned char) c;
    xwrite(f,(const ucl_voidp) &cc,1);
}

/* read and write portable 32-bit integers */

ucl_uint32 xread32(FILE *f)
{
    unsigned char b[4];
    ucl_uint32 v;

    xread(f,b,4,0);
    v  = (ucl_uint32) b[3] <<  0;
    v |= (ucl_uint32) b[2] <<  8;
    v |= (ucl_uint32) b[1] << 16;
    v |= (ucl_uint32) b[0] << 24;
    return v;
}

void xwrite32(FILE *f, ucl_uint32 v)
{
    unsigned char b[4];

    b[3] = (unsigned char) (v >>  0);
    b[2] = (unsigned char) (v >>  8);
    b[1] = (unsigned char) (v >> 16);
    b[0] = (unsigned char) (v >> 24);
    xwrite(f,b,4);
}


/*************************************************************************
// util
**************************************************************************/

static ucl_uint get_overhead(int method, ucl_uint size)
{
    if (method == 0x2b || method == 0x2d || method == 0x2e)
        return size / 8 + 256;
    return 0;
}


static char method_name[64];

static ucl_bool set_method_name(int method, int level)
{
    method_name[0] = 0;
    if (level < 1 || level > 10)
        return 0;
    if (method == 0x2b)
        sprintf(method_name,"NRV2B-99/%d", level);
    else if (method == 0x2d)
        sprintf(method_name,"NRV2D-99/%d", level);
    else if (method == 0x2e)
        sprintf(method_name,"NRV2E-99/%d", level);
    else
        return 0;
    return 1;
}


/*************************************************************************
// compress
**************************************************************************/

int do_compress(FILE *fi, FILE *fo, int method, int level, ucl_uint block_size)
{
    int r = 0;
    ucl_bytep in = NULL;
    ucl_bytep out = NULL;
    ucl_uint in_len;
    ucl_uint out_len;
    ucl_uint32 flags = opt_fast ? 0 : 1;
    ucl_uint32 checksum;
    ucl_uint overhead = 0;
#if defined(WITH_TIMER)
    ucl_uclock_t t_start, t_stop;
#endif

    total_in = total_out = 0;

/*
 * Step 1: write magic header, flags & block size, init checksum
 */
    xwrite(fo,magic,sizeof(magic));
    xwrite32(fo,flags);
    xputc(fo,method);           /* compression method */
    xputc(fo,level);            /* compression level */
    xwrite32(fo,block_size);
    checksum = ucl_adler32(0,NULL,0);

/*
 * Step 2: allocate compression buffers and work-memory
 */
    overhead = get_overhead(method,block_size);
    in = (ucl_bytep) ucl_malloc(block_size);
    out = (ucl_bytep) ucl_malloc(block_size + overhead);
    if (in == NULL || out == NULL)
    {
        printf("%s: out of memory\n", progname);
        r = 1;
        goto err;
    }

/*
 * Step 3: process blocks
 */
    for (;;)
    {
        /* read block */
        in_len = xread(fi,in,block_size,1);
        if (in_len <= 0)
            break;

        /* update checksum */
        if (flags & 1)
            checksum = ucl_adler32(checksum,in,in_len);

#if defined(WITH_TIMER)
        ucl_uclock_read(&uc, &t_start);
#endif

        /* compress block */
        r = UCL_E_ERROR;
        out_len = 0;
        if (method == 0x2b)
            r = ucl_nrv2b_99_compress(in,in_len,out,&out_len,0,level,NULL,NULL);
        else if (method == 0x2d)
            r = ucl_nrv2d_99_compress(in,in_len,out,&out_len,0,level,NULL,NULL);
        else if (method == 0x2e)
            r = ucl_nrv2e_99_compress(in,in_len,out,&out_len,0,level,NULL,NULL);
        if (r == UCL_E_OUT_OF_MEMORY)
        {
            printf("%s: out of memory in compress\n", progname);
            r = 1;
            goto err;
        }
        if (r != UCL_E_OK || out_len > in_len + get_overhead(method,in_len))
        {
            /* this should NEVER happen */
            printf("internal error - compression failed: %d\n", r);
            r = 2;
            goto err;
        }

#if defined(WITH_TIMER)
        ucl_uclock_read(&uc, &t_stop);
        benchmark_bytes += in_len;
        benchmark_secs += ucl_uclock_get_elapsed(&uc, &t_start, &t_stop);
#endif

        /* write uncompressed block size */
        xwrite32(fo,in_len);

        if (out_len < in_len)
        {
            /* write compressed block */
            xwrite32(fo,out_len);
            xwrite(fo,out,out_len);
        }
        else
        {
            /* not compressible - write uncompressed block */
            xwrite32(fo,in_len);
            xwrite(fo,in,in_len);
        }
    }

    /* write EOF marker */
    xwrite32(fo,0);

    /* write checksum */
    if (flags & 1)
        xwrite32(fo,checksum);

    r = 0;
err:
    ucl_free(out);
    ucl_free(in);
    return r;
}


/*************************************************************************
// decompression benchmark
**************************************************************************/

#if defined(WITH_TIMER)

void do_decompress_benchmark(int method, unsigned long benchmark_loops,
                             const ucl_bytep in, ucl_uint in_len,
                             ucl_bytep out, ucl_uint out_len)
{
    ucl_uclock_t t_start, t_stop;

    ucl_uclock_read(&uc, &t_start);
    while (benchmark_loops-- > 0)
    {
        int r = -1;
        ucl_uint new_len = out_len;

#if 1 && defined(UCL_USE_ASM)
        /* use fast assembler versions of the decompressors - see
         * asm/i386/00README.ASM for more info */
        if (method == 0x2b)
            r = ucl_nrv2b_decompress_asm_fast_8(in,in_len,out,&new_len,NULL);
        else if (method == 0x2d)
            r = ucl_nrv2d_decompress_asm_fast_8(in,in_len,out,&new_len,NULL);
        else if (method == 0x2e)
            r = ucl_nrv2e_decompress_asm_fast_8(in,in_len,out,&new_len,NULL);
#else
        if (method == 0x2b)
            r = ucl_nrv2b_decompress_8(in,in_len,out,&new_len,NULL);
        else if (method == 0x2d)
            r = ucl_nrv2d_decompress_8(in,in_len,out,&new_len,NULL);
        else if (method == 0x2e)
            r = ucl_nrv2e_decompress_8(in,in_len,out,&new_len,NULL);
#endif
        if (r != UCL_E_OK || new_len != out_len)
        {
            printf("%s: compressed data violation: error %d (0x%x: %ld/%ld/%ld)\n", progname, r, method, (long) in_len, (long) out_len, (long) new_len);
            printf("%s: unexpected failure in benchmark -- exiting.\n", progname);
            exit(1);
        }
        benchmark_bytes += out_len;
    }
    ucl_uclock_read(&uc, &t_stop);
    benchmark_secs += ucl_uclock_get_elapsed(&uc, &t_start, &t_stop);
}

#endif /* WITH_TIMER */


/*************************************************************************
// decompress / test
//
// We are using in-place decompression here.
**************************************************************************/

int do_decompress(FILE *fi, FILE *fo, unsigned long benchmark_loops)
{
    int r = 0;
    ucl_bytep buf = NULL;
    ucl_uint buf_len;
    unsigned char m [ sizeof(magic) ];
    ucl_uint32 flags;
    int method;
    int level;
    ucl_uint block_size;
    ucl_uint32 checksum;
    ucl_uint overhead = 0;

    total_in = total_out = 0;

/*
 * Step 1: check magic header, read flags & block size, init checksum
 */
    if (xread(fi,m,sizeof(magic),1) != sizeof(magic) ||
        memcmp(m,magic,sizeof(magic)) != 0)
    {
        printf("%s: header error - this file is not compressed by uclpack\n", progname);
        r = 1;
        goto err;
    }
    flags = xread32(fi);
    method = xgetc(fi);
    level = xgetc(fi);
    block_size = xread32(fi);
    overhead = get_overhead(method, block_size);
    if (overhead == 0 || !set_method_name(method, level))
    {
        printf("%s: header error - invalid method %d (level %d)\n",
                progname, method, level);
        r = 2;
        goto err;
    }
    if (block_size < 1024 || block_size > 8*1024*1024L)
    {
        printf("%s: header error - invalid block size %ld\n",
                progname, (long) block_size);
        r = 3;
        goto err;
    }
    printf("%s: block-size is %ld bytes\n", progname, (long)block_size);

    checksum = ucl_adler32(0,NULL,0);

/*
 * Step 2: allocate buffer for in-place decompression
 */
    buf_len = block_size + overhead;
    if (benchmark_loops > 0)
    {
        /* cannot use in-place decompression when doing benchmarks */
        buf_len += block_size;
    }
    buf = (ucl_bytep) ucl_malloc(buf_len);
    if (buf == NULL)
    {
        printf("%s: out of memory\n", progname);
        r = 4;
        goto err;
    }

/*
 * Step 3: process blocks
 */
    for (;;)
    {
        ucl_bytep in;
        ucl_bytep out;
        ucl_uint in_len;
        ucl_uint out_len;

        /* read uncompressed size */
        out_len = xread32(fi);

        /* exit if last block (EOF marker) */
        if (out_len == 0)
            break;

        /* read compressed size */
        in_len = xread32(fi);

        /* sanity check of the size values */
        if (in_len > block_size || out_len > block_size ||
            in_len == 0 || in_len > out_len)
        {
            printf("%s: block size error - data corrupted\n", progname);
            r = 5;
            goto err;
        }

        /* place compressed block at the top of the buffer */
        in = buf + buf_len - in_len;
        out = buf;

        /* read compressed block data */
        xread(fi,in,in_len,0);

        if (in_len < out_len)
        {
            /* decompress - use safe decompressor as data might be corrupted */
            ucl_uint new_len = out_len;

            if (method == 0x2b)
            {
                if (opt_fast)
                    r = ucl_nrv2b_decompress_8(in,in_len,out,&new_len,NULL);
                else
                    r = ucl_nrv2b_decompress_safe_8(in,in_len,out,&new_len,NULL);
            }
            else if (method == 0x2d)
            {
                if (opt_fast)
                    r = ucl_nrv2d_decompress_8(in,in_len,out,&new_len,NULL);
                else
                    r = ucl_nrv2d_decompress_safe_8(in,in_len,out,&new_len,NULL);
            }
            else if (method == 0x2e)
            {
                if (opt_fast)
                    r = ucl_nrv2e_decompress_8(in,in_len,out,&new_len,NULL);
                else
                    r = ucl_nrv2e_decompress_safe_8(in,in_len,out,&new_len,NULL);
            }
            if (r != UCL_E_OK || new_len != out_len)
            {
                printf("%s: compressed data violation: error %d (0x%x: %ld/%ld/%ld)\n", progname, r, method, (long) in_len, (long) out_len, (long) new_len);
                r = 6;
                goto err;
            }
            /* write decompressed block */
            xwrite(fo,out,out_len);
            /* update checksum */
            if ((flags & 1) && !opt_fast)
                checksum = ucl_adler32(checksum,out,out_len);
#if defined(WITH_TIMER)
            if (benchmark_loops > 0)
                do_decompress_benchmark(method,benchmark_loops,in,in_len,out,out_len);
#endif
        }
        else
        {
            /* write original (incompressible) block */
            xwrite(fo,in,in_len);
            /* update checksum */
            if ((flags & 1) && !opt_fast)
                checksum = ucl_adler32(checksum,in,in_len);
        }
    }

    /* read and verify checksum */
    if (flags & 1)
    {
        ucl_uint32 c = xread32(fi);
        if (!opt_fast && c != checksum)
        {
            printf("%s: checksum error - data corrupted\n", progname);
            r = 7;
            goto err;
        }
    }

    r = 0;
err:
    ucl_free(buf);
    return r;
}


/*************************************************************************
// misc support functions
**************************************************************************/

static void usage(void)
{
    printf("usage:\n");
    printf("  %s [options] input-file output-file      (compress)\n", progname);
    printf("  %s -d compressed-file output-file        (decompress)\n", progname);
    printf("  %s -t compressed-file...                 (test)\n", progname);
#if defined(WITH_TIMER)
    printf("  %s -t -D1000 compressed-file...          (test decompression speed)\n", progname);
#endif
    printf("\ncompression options:\n");
    printf("  -1...-9, --10   set compression level [default is `-7']\n");
    printf("  --nrv2b         use NRV2B compression method\n");
    printf("  --nrv2d         use NRV2D compression method [default]\n");
    printf("  --nrv2e         use NRV2E compression method\n");
    printf("\nother options:\n");
    printf("  -F              do not store or verify a checksum (faster)\n");
    printf("  -Bxxxx          set block-size for compression [default 262144]\n");
#if defined(WITH_TIMER)
    printf("  -Dxxxx          number of iterations for decompression benchmark\n");
#endif
    exit(1);
}


/* open input file */
static FILE *xopen_fi(const char *name)
{
    FILE *f;

    f = fopen(name,"rb");
    if (f == NULL)
    {
        printf("%s: cannot open input file %s\n", progname, name);
        exit(1);
    }
#if defined(HAVE_STAT) && defined(S_ISREG)
    {
        struct stat st;
#if defined(HAVE_LSTAT)
        if (lstat(name,&st) != 0 || !S_ISREG(st.st_mode))
#else
        if (stat(name,&st) != 0 || !S_ISREG(st.st_mode))
#endif
        {
            printf("%s: %s is not a regular file\n", progname, name);
            fclose(f);
            exit(1);
        }
    }
#endif
    return f;
}


/* open output file */
static FILE *xopen_fo(const char *name)
{
    FILE *f;

#if 0
    /* this is an example program, so make sure we don't overwrite a file */
    f = fopen(name,"rb");
    if (f != NULL)
    {
        printf("%s: file %s already exists -- not overwritten\n", progname, name);
        fclose(f);
        exit(1);
    }
#endif
    f = fopen(name,"wb");
    if (f == NULL)
    {
        printf("%s: cannot open output file %s\n", progname, name);
        exit(1);
    }
    return f;
}


/*************************************************************************
// main
**************************************************************************/

int __acc_cdecl_main main(int argc, char *argv[])
{
    int i = 1;
    int r = 0;
    FILE *fi = NULL;
    FILE *fo = NULL;
    const char *in_name = NULL;
    const char *out_name = NULL;
    ucl_bool opt_decompress = 0;
    ucl_bool opt_test = 0;
    int opt_method = 0x2d;
    int opt_level = 7;
    ucl_uint opt_block_size;
    unsigned long opt_decompress_loops = 0;
    const char *s;

    ucl_wildargv(&argc, &argv);
#if defined(WITH_TIMER)
    ucl_uclock_open(&uc);
#endif
    progname = argv[0];
    for (s = progname; *s; s++)
        if ((*s == '/' || *s == '\\') && s[1])
            progname = s + 1;

    printf("\nUCL data compression library (v%s, %s).\n",
            ucl_version_string(), ucl_version_date());
    printf("Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer\n");
    printf("http://www.oberhumer.com/opensource/ucl/\n\n");

#if 0
    printf(
"*** WARNING ***\n"
"   This is an example program, do not use to backup your data !\n"
"\n");
#endif


/*
 * Step 1: initialize the UCL library
 */
    if (ucl_init() != UCL_E_OK)
    {
        printf("internal error - ucl_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DUCL_DEBUG' for diagnostics)\n");
        exit(1);
    }


/*
 * Step 2: setup default options
 */
    opt_block_size = 256 * 1024L;

#if defined(ACC_MM_AHSHIFT)
    /* reduce memory requirements for ancient 640kB DOS real-mode */
    if (ACC_MM_AHSHIFT != 3)
        opt_block_size = 16 * 1024L;
#endif


/*
 * Step 3: get options
 */

    while (i < argc && argv[i][0] == '-')
    {
        if (strcmp(argv[i],"-d") == 0)
            opt_decompress = 1;
        else if (strcmp(argv[i],"-t") == 0)
            opt_test = 1;
        else if (strcmp(argv[i],"-F") == 0)
            opt_fast = 1;
        else if (strcmp(argv[i],"--2b") == 0)
            opt_method = 0x2b;
        else if (strcmp(argv[i],"--nrv2b") == 0)
            opt_method = 0x2b;
        else if (strcmp(argv[i],"--2d") == 0)
            opt_method = 0x2d;
        else if (strcmp(argv[i],"--nrv2d") == 0)
            opt_method = 0x2d;
        else if (strcmp(argv[i],"--2e") == 0)
            opt_method = 0x2e;
        else if (strcmp(argv[i],"--nrv2e") == 0)
            opt_method = 0x2e;
        else if ((argv[i][1] >= '1' && argv[i][1] <= '9') && !argv[i][2])
            opt_level = argv[i][1] - '0';
        else if (strcmp(argv[i],"--10") == 0)
            opt_level = 10;
        else if (strcmp(argv[i],"--best") == 0)
            opt_level = 10;
        else if (argv[i][1] == 'b' && argv[i][2])
        {
            long x = atol(&argv[i][2]);
            if (x >= 1024L && x <= 8*1024*1024L)
                opt_block_size = (ucl_uint) x;
            else {
                printf("%s: error: invalid block-size %ld\n", progname, x);
                exit(1);
            }
        }
        else if (argv[i][1] == 'D' && argv[i][2])
        {
#if defined(WITH_TIMER)
            long x = atol(&argv[i][2]);
            if (x > 1)
                opt_decompress_loops = x;
            else {
                printf("%s: error: invalid number of benchmark loops %ld\n", progname, x);
                exit(1);
            }
#else
            usage();
#endif
        }
        else if (strcmp(argv[i],"--debug") == 0)
            opt_debug += 1;
        else
            usage();
        i++;
    }
    if (opt_test && i >= argc)
        usage();
    if (!opt_test && i + 2 != argc)
        usage();


/*
 * Step 4: process file(s)
 */

    if (opt_test)
    {
        while (i < argc && r == 0)
        {
            in_name = argv[i++];
            fi = xopen_fi(in_name);
#if defined(WITH_TIMER)
            benchmark_bytes = benchmark_secs = 0.0;
#endif
            r = do_decompress(fi, NULL, opt_decompress_loops);
            if (r == 0)
                printf("%s: tested ok: %-10s %-11s: %6lu -> %6lu bytes\n",
                        progname, in_name, method_name, total_in, total_out);
            fclose(fi);
            fi = NULL;
#if defined(WITH_TIMER)
            if (r == 0 && benchmark_bytes > 0)
            {
                /* speed measured in uncompressed megabytes per second */
                double mb = benchmark_bytes / (1024.0 * 1024.0);
                double mbs = 0.0;
                if (benchmark_secs >= 0.0001) mbs = mb / benchmark_secs;
                printf("\n  [benchmark] %lu loops, %.3f MB, %.3f secs, %.3f MB/s\n\n", opt_decompress_loops, mb, benchmark_secs, mbs);
            }
#endif
        }
    }
    else if (opt_decompress)
    {
        in_name = argv[i++];
        out_name = argv[i++];
        fi = xopen_fi(in_name);
        fo = xopen_fo(out_name);
        r = do_decompress(fi, fo, 0);
        if (r == 0)
            printf("%s: decompressed %lu into %lu bytes\n",
                    progname, total_in, total_out);
    }
    else /* compress */
    {
        printf("%s: using block-size of %lu bytes\n", progname, (unsigned long)opt_block_size);
        if (!set_method_name(opt_method, opt_level))
        {
            printf("%s: internal error - invalid method %d (level %d)\n",
                   progname, opt_method, opt_level);
            goto quit;
        }
        in_name = argv[i++];
        out_name = argv[i++];
        fi = xopen_fi(in_name);
        fo = xopen_fo(out_name);
        r = do_compress(fi,fo,opt_method,opt_level,opt_block_size);
        if (r == 0)
        {
            printf("%s: algorithm %s, compressed %lu into %lu bytes\n",
                    progname, method_name, total_in, total_out);
#if defined(WITH_TIMER)
            if (opt_debug >= 1 && benchmark_bytes > 0)
            {
                /* speed measured in uncompressed kilobytes per second */
                double kb = benchmark_bytes / (1024.0);
                double kbs = 0.0;
                if (benchmark_secs >= 0.0001) kbs = kb / benchmark_secs;
                printf("\n  [compression speeed] %.3f KB, %.3f secs, %.3f KB/s\n\n", kb, benchmark_secs, kbs);
            }
#endif
#if 1 && defined(WITH_TIMER)
            printf("\n  Info: To test the decompression speed on your system type:\n"
                   "    `%s -t -D1000 %s'\n", progname, out_name);
#endif
        }
    }

quit:
    if (fi) fclose(fi);
    if (fo) fclose(fo);
#if defined(WITH_TIMER)
    ucl_uclock_close(&uc);
#endif
    return r;
}

/*
vi:ts=4:et
*/

