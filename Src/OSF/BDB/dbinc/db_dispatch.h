/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 */
/*
 * Copyright (c) 1995, 1996 The President and Fellows of Harvard University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *  may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 * $Id$
 */
#ifndef _DB_DISPATCH_H_
#define	_DB_DISPATCH_H_

#if defined(__cplusplus)
extern "C" {
#endif
/*
 * Declarations and typedefs for the list of transaction IDs used during
 * recovery.  This is a generic list used to pass along whatever information
 * we need during recovery.
 */
typedef enum {
	TXNLIST_DELETE,
	TXNLIST_LSN,
	TXNLIST_TXNID
} db_txnlist_type;

#define	DB_TXNLIST_MASK(hp, n)  (n % hp->nslots)

LIST_HEAD(__db_headlink, __db_txnlist); // @sobolev

struct __db_txnhead {
	void *td;		/* If abort, the detail for the txn. */
	DB_THREAD_INFO *thread_info;	/* Thread information. */
	uint32 maxid;	/* Maximum transaction id. */
	DB_LSN maxlsn;		/* Maximum commit lsn. */
	DB_LSN ckplsn;		/* LSN of last retained checkpoint. */
	DB_LSN trunc_lsn;	/* Lsn to which we are going to truncate; make sure we abort anyone after this. */
	uint32 generation;	/* Current generation number. */
	uint32 gen_alloc;	/* Number of generations allocated. */
	struct {
		uint32 generation;
		uint32 txn_min;
		uint32 txn_max;
	} *gen_array;		/* Array of txnids associated with a gen. */
	uint nslots;
	// @sobolev LIST_HEAD(__db_headlink, __db_txnlist) head[1];
	struct __db_headlink head[1]; // @sobolev
};

#define	DB_LSN_STACK_SIZE 4
struct __db_txnlist {
	db_txnlist_type type;
	LIST_ENTRY(__db_txnlist) links;
	union {
		struct {
			uint32 txnid;
			uint32 generation;
			uint32 status;
		} t;
		struct {
			uint32 stack_size;
			uint32 stack_indx;
			DB_LSN *lsn_stack;
		} l;
	} u;
};

#if defined(__cplusplus)
}
#endif

#endif /* !_DB_DISPATCH_H_ */
