/*
 * Copyright 2015-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at  https://www.openssl.org/source/license.html
 */

struct comp_method_st {
    int type;                   /* NID for compression library */
    const char *name;           /* A text string to identify the library */
    int (*init) (COMP_CTX *ctx);
    void (*finish) (COMP_CTX *ctx);
    int (*compress) (COMP_CTX *ctx,
                     uchar *out, uint olen,
                     uchar *in, uint ilen);
    int (*expand) (COMP_CTX *ctx,
                   uchar *out, uint olen,
                   uchar *in, uint ilen);
};

struct comp_ctx_st {
    struct comp_method_st *meth;
    ulong compress_in;
    ulong compress_out;
    ulong expand_in;
    ulong expand_out;
    void* data;
};
