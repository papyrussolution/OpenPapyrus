/*
 * Copyright 2000-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
#include "bn_lcl.h"
/*-
 * @todo list
 *
 * 1. Check a bunch of "(words+1)" type hacks in various bignum functions and
 * check they can be safely removed.
 *  - Check +1 and other ugliness in BN_from_montgomery()
 *
 * 2. Consider allowing a BN_new_ex() that, at least, lets you specify an
 * appropriate 'block' size that will be honoured by bn_expand_internal() to
 * prevent piddly little reallocations. OTOH, profiling bignum expansions in
 * BN_CTX doesn't show this to be a big issue.
 */
#define BN_CTX_POOL_SIZE        16 /* How many bignums are in each "pool item"; */
#define BN_CTX_START_FRAMES     32 /* The stack frame info is resizing, set a first-time expansion size; */

/***********/
/* BN_POOL */
/***********/

/* A bundle of bignums that can be linked with other bundles */
typedef struct bignum_pool_item {
	/* The bignum values */
	BIGNUM vals[BN_CTX_POOL_SIZE];
	/* Linked-list admin */
	struct bignum_pool_item * prev, * next;
} BN_POOL_ITEM;
/* A linked-list of bignums grouped in bundles */
typedef struct bignum_pool {
	/* Linked-list admin */
	BN_POOL_ITEM * head, * current, * tail;
	/* Stack depth and allocation size */
	unsigned used, size;
} BN_POOL;
static void FASTCALL BN_POOL_init(BN_POOL *);
static void FASTCALL BN_POOL_finish(BN_POOL *);
static BIGNUM * FASTCALL BN_POOL_get(BN_POOL *, int);
static void FASTCALL BN_POOL_release(BN_POOL *, uint);

/************/
/* BN_STACK */
/************/

/* A wrapper to manage the "stack frames" */
typedef struct bignum_ctx_stack {
	uint * indexes; /* Array of indexes into the bignum stack */
	uint depth, size; /* Number of stack frames, and the size of the allocated array */
} BN_STACK;
static void FASTCALL BN_STACK_init(BN_STACK *);
static void FASTCALL BN_STACK_finish(BN_STACK *);
static int FASTCALL BN_STACK_push(BN_STACK *, uint);
static uint FASTCALL BN_STACK_pop(BN_STACK *);

/**********/
/* BN_CTX */
/**********/

/* The opaque BN_CTX type */
struct bignum_ctx {
	/* The bignum bundles */
	BN_POOL pool;
	/* The "stack frames", if you will */
	BN_STACK stack;
	/* The number of bignums currently assigned */
	uint used;
	/* Depth of stack overflow */
	int err_stack;
	/* Block "gets" until an "end" (compatibility behaviour) */
	int too_many;
	/* Flags. */
	int flags;
};

/* Enable this to find BN_CTX bugs */
#ifdef BN_CTX_DEBUG
	static const char * ctxdbg_cur = NULL;
	static void ctxdbg(BN_CTX * ctx)
	{
		uint bnidx = 0, fpidx = 0;
		BN_POOL_ITEM * item = ctx->pool.head;
		BN_STACK * stack = &ctx->stack;
		fprintf(stderr, "(%16p): ", ctx);
		while(bnidx < ctx->used) {
			fprintf(stderr, "%03x ", item->vals[bnidx++ % BN_CTX_POOL_SIZE].dmax);
			if(!(bnidx % BN_CTX_POOL_SIZE))
				item = item->next;
		}
		fprintf(stderr, "\n");
		bnidx = 0;
		fprintf(stderr, "          : ");
		while(fpidx < stack->depth) {
			while(bnidx++ < stack->indexes[fpidx])
				fprintf(stderr, "    ");
			fprintf(stderr, "^^^ ");
			bnidx++;
			fpidx++;
		}
		fprintf(stderr, "\n");
	}
	#define CTXDBG_ENTRY(str, ctx)  do { ctxdbg_cur = (str); fprintf(stderr, "Starting %s\n", ctxdbg_cur); ctxdbg(ctx); } while(0)
	#define CTXDBG_EXIT(ctx)    do { fprintf(stderr, "Ending %s\n", ctxdbg_cur); ctxdbg(ctx); } while(0)
	#define CTXDBG_RET(ctx, ret)
#else
	#define CTXDBG_ENTRY(str, ctx)
	#define CTXDBG_EXIT(ctx)
	#define CTXDBG_RET(ctx, ret)
#endif

BN_CTX * BN_CTX_new(void)
{
	BN_CTX * ret;
	if((ret = (BN_CTX*)OPENSSL_zalloc(sizeof(*ret))) == NULL) {
		BNerr(BN_F_BN_CTX_NEW, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	/* Initialise the structure */
	BN_POOL_init(&ret->pool);
	BN_STACK_init(&ret->stack);
	return ret;
}

BN_CTX * BN_CTX_secure_new(void)
{
	BN_CTX * ret = BN_CTX_new();
	if(ret)
		ret->flags = BN_FLG_SECURE;
	return ret;
}

void FASTCALL BN_CTX_free(BN_CTX * ctx)
{
	if(ctx) {
#ifdef BN_CTX_DEBUG
		{
			BN_POOL_ITEM * pool = ctx->pool.head;
			fprintf(stderr, "BN_CTX_free, stack-size=%d, pool-bignums=%d\n", ctx->stack.size, ctx->pool.size);
			fprintf(stderr, "dmaxs: ");
			while(pool) {
				unsigned loop = 0;
				while(loop < BN_CTX_POOL_SIZE)
					fprintf(stderr, "%02x ", pool->vals[loop++].dmax);
				pool = pool->next;
			}
			fprintf(stderr, "\n");
		}
#endif
		BN_STACK_finish(&ctx->stack);
		BN_POOL_finish(&ctx->pool);
		OPENSSL_free(ctx);
	}
}

void FASTCALL BN_CTX_start(BN_CTX * ctx)
{
	CTXDBG_ENTRY("BN_CTX_start", ctx);
	// If we're already overflowing ... 
	if(ctx->err_stack || ctx->too_many)
		ctx->err_stack++;
	// (Try to) get a new frame pointer 
	else if(!BN_STACK_push(&ctx->stack, ctx->used)) {
		BNerr(BN_F_BN_CTX_START, BN_R_TOO_MANY_TEMPORARY_VARIABLES);
		ctx->err_stack++;
	}
	CTXDBG_EXIT(ctx);
}

void FASTCALL BN_CTX_end(BN_CTX * ctx)
{
	if(ctx) { // @sobolev
		CTXDBG_ENTRY("BN_CTX_end", ctx);
		if(ctx->err_stack)
			ctx->err_stack--;
		else {
			uint fp = BN_STACK_pop(&ctx->stack);
			// Does this stack frame have anything to release? 
			if(fp < ctx->used)
				BN_POOL_release(&ctx->pool, ctx->used - fp);
			ctx->used = fp;
			// Unjam "too_many" in case "get" had failed 
			ctx->too_many = 0;
		}
		CTXDBG_EXIT(ctx);
	}
}

BIGNUM * FASTCALL BN_CTX_get(BN_CTX * ctx)
{
	BIGNUM * ret = 0;
	CTXDBG_ENTRY("BN_CTX_get", ctx);
	if(!ctx->err_stack && !ctx->too_many) {
		ret = BN_POOL_get(&ctx->pool, ctx->flags);
		if(!ret) {
			//
			// Setting too_many prevents repeated "get" attempts from cluttering the error stack.
			//
			ctx->too_many = 1;
			BNerr(BN_F_BN_CTX_GET, BN_R_TOO_MANY_TEMPORARY_VARIABLES);
		}
		else {
			// OK, make sure the returned bignum is "zero" 
			BN_zero(ret);
			ctx->used++;
			CTXDBG_RET(ctx, ret);
		}
	}
	return ret;
}

/************/
/* BN_STACK */
/************/

static void FASTCALL BN_STACK_init(BN_STACK * st)
{
	st->indexes = NULL;
	st->depth = st->size = 0;
}

static void FASTCALL BN_STACK_finish(BN_STACK * st)
{
	OPENSSL_free(st->indexes);
	st->indexes = NULL;
}

static int FASTCALL BN_STACK_push(BN_STACK * st, uint idx)
{
	if(st->depth == st->size) {
		/* Need to expand */
		uint newsize = st->size ? (st->size * 3 / 2) : BN_CTX_START_FRAMES;
		uint * newitems = (uint *)OPENSSL_malloc(sizeof(*newitems) * newsize);
		if(newitems == NULL)
			return 0;
		if(st->depth)
			memcpy(newitems, st->indexes, sizeof(*newitems) * st->depth);
		OPENSSL_free(st->indexes);
		st->indexes = newitems;
		st->size = newsize;
	}
	st->indexes[(st->depth)++] = idx;
	return 1;
}

static uint FASTCALL BN_STACK_pop(BN_STACK * st)
{
	return st->indexes[--(st->depth)];
}

/***********/
/* BN_POOL */
/***********/

static void FASTCALL BN_POOL_init(BN_POOL * p)
{
	p->head = p->current = p->tail = NULL;
	p->used = p->size = 0;
}

static void FASTCALL BN_POOL_finish(BN_POOL * p)
{
	uint loop;
	BIGNUM * bn;
	while(p->head) {
		for(loop = 0, bn = p->head->vals; loop++ < BN_CTX_POOL_SIZE; bn++)
			if(bn->d)
				BN_clear_free(bn);
		p->current = p->head->next;
		OPENSSL_free(p->head);
		p->head = p->current;
	}
}

static BIGNUM * FASTCALL BN_POOL_get(BN_POOL * p, int flag)
{
	BIGNUM * bn;
	uint loop;
	/* Full; allocate a new pool item and link it in. */
	if(p->used == p->size) {
		BN_POOL_ITEM * item = (BN_POOL_ITEM*)OPENSSL_malloc(sizeof(*item));
		if(item == NULL)
			return NULL;
		for(loop = 0, bn = item->vals; loop++ < BN_CTX_POOL_SIZE; bn++) {
			bn_init(bn);
			if((flag & BN_FLG_SECURE) != 0)
				BN_set_flags(bn, BN_FLG_SECURE);
		}
		item->prev = p->tail;
		item->next = NULL;

		if(p->head == NULL)
			p->head = p->current = p->tail = item;
		else {
			p->tail->next = item;
			p->tail = item;
			p->current = item;
		}
		p->size += BN_CTX_POOL_SIZE;
		p->used++;
		/* Return the first bignum from the new pool */
		return item->vals;
	}
	if(!p->used)
		p->current = p->head;
	else if((p->used % BN_CTX_POOL_SIZE) == 0)
		p->current = p->current->next;
	return p->current->vals + ((p->used++) % BN_CTX_POOL_SIZE);
}

static void FASTCALL BN_POOL_release(BN_POOL * p, uint num)
{
	uint offset = (p->used - 1) % BN_CTX_POOL_SIZE;
	p->used -= num;
	while(num--) {
		bn_check_top(p->current->vals + offset);
		if(offset == 0) {
			offset = BN_CTX_POOL_SIZE - 1;
			p->current = p->current->prev;
		}
		else
			offset--;
	}
}

