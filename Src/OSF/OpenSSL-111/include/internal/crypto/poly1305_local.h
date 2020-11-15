/*
 * Copyright 2015-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at  https://www.openssl.org/source/license.html
 */

typedef void (*poly1305_blocks_f) (void *ctx, const uchar *inp,
                                   size_t len, uint padbit);
typedef void (*poly1305_emit_f) (void *ctx, uchar mac[16],
                                 const uint nonce[4]);

struct poly1305_context {
    double opaque[24];  /* large enough to hold internal state, declared
                         * 'double' to ensure at least 64-bit invariant
                         * alignment across all platforms and
                         * configurations */
    uint nonce[4];
    uchar data[POLY1305_BLOCK_SIZE];
    size_t num;
    struct {
        poly1305_blocks_f blocks;
        poly1305_emit_f emit;
    } func;
};
