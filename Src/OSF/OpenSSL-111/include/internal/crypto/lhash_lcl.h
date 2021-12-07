/*
 * Copyright 1995-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at  https://www.openssl.org/source/license.html
 */
//#include <openssl/crypto.h>

#include "internal/tsan_assist.h"

struct lhash_node_st {
    void *data;
    struct lhash_node_st *next;
    ulong hash;
};

struct lhash_st {
    OPENSSL_LH_NODE **b;
    OPENSSL_LH_COMPFUNC comp;
    OPENSSL_LH_HASHFUNC hash;
    uint num_nodes;
    uint num_alloc_nodes;
    uint p;
    uint pmax;
    ulong up_load; /* load times 256 */
    ulong down_load; /* load times 256 */
    ulong num_items;
    ulong num_expands;
    ulong num_expand_reallocs;
    ulong num_contracts;
    ulong num_contract_reallocs;
    TSAN_QUALIFIER ulong num_hash_calls;
    TSAN_QUALIFIER ulong num_comp_calls;
    ulong num_insert;
    ulong num_replace;
    ulong num_delete;
    ulong num_no_delete;
    TSAN_QUALIFIER ulong num_retrieve;
    TSAN_QUALIFIER ulong num_retrieve_miss;
    TSAN_QUALIFIER ulong num_hash_comps;
    int error;
};
