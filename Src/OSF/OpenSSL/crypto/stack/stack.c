/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

struct stack_st {
	int num;
	const char ** data;
	int sorted;
	size_t num_alloc;
	OPENSSL_sk_compfunc comp;
};

#undef MIN_NODES
#define MIN_NODES       4

OPENSSL_sk_compfunc OPENSSL_sk_set_cmp_func(OPENSSL_STACK * sk, OPENSSL_sk_compfunc c)
{
	OPENSSL_sk_compfunc old = sk->comp;
	if(sk->comp != c)
		sk->sorted = 0;
	sk->comp = c;
	return old;
}

OPENSSL_STACK * FASTCALL OPENSSL_sk_dup(const OPENSSL_STACK * sk)
{
	OPENSSL_STACK * ret;
	if(sk->num < 0)
		return NULL;
	if((ret = (OPENSSL_STACK*)OPENSSL_malloc(sizeof(*ret))) == NULL)
		return NULL;
	/* direct structure assignment */
	*ret = *sk;
	if((ret->data = (const char**)OPENSSL_malloc(sizeof(*ret->data) * sk->num_alloc)) == NULL)
		goto err;
	memcpy((void*)ret->data, sk->data, sizeof(char *) * sk->num); // @badcast
	return ret;
err:
	OPENSSL_sk_free(ret);
	return NULL;
}

OPENSSL_STACK * OPENSSL_sk_deep_copy(const OPENSSL_STACK * sk, OPENSSL_sk_copyfunc copy_func, OPENSSL_sk_freefunc free_func)
{
	OPENSSL_STACK * ret;
	int i;
	if(sk->num < 0)
		return NULL;
	if((ret = (OPENSSL_STACK*)OPENSSL_malloc(sizeof(*ret))) == NULL)
		return NULL;
	/* direct structure assignment */
	*ret = *sk;
	ret->num_alloc = sk->num > MIN_NODES ? (size_t)sk->num : MIN_NODES;
	ret->data = (const char**)OPENSSL_zalloc(sizeof(*ret->data) * ret->num_alloc);
	if(ret->data == NULL) {
		OPENSSL_free(ret);
		return NULL;
	}
	for(i = 0; i < ret->num; ++i) {
		if(sk->data[i]) {
			if((ret->data[i] = (const char*)copy_func(sk->data[i])) == NULL) {
				while(--i >= 0)
					if(ret->data[i] != NULL)
						free_func((void*)ret->data[i]);
				OPENSSL_sk_free(ret);
				return NULL;
			}
		}
	}
	return ret;
}

OPENSSL_STACK * OPENSSL_sk_new_null(void)
{
	return OPENSSL_sk_new((OPENSSL_sk_compfunc)NULL);
}

OPENSSL_STACK * FASTCALL OPENSSL_sk_new(OPENSSL_sk_compfunc c)
{
	OPENSSL_STACK * ret;
	if((ret = (OPENSSL_STACK*)OPENSSL_zalloc(sizeof(*ret))) == NULL)
		goto err;
	if((ret->data = (const char**)OPENSSL_zalloc(sizeof(*ret->data) * MIN_NODES)) == NULL)
		goto err;
	ret->comp = c;
	ret->num_alloc = MIN_NODES;
	return ret;
err:
	OPENSSL_free(ret);
	return NULL;
}

int FASTCALL OPENSSL_sk_insert(OPENSSL_STACK * st, const void * data, int loc)
{
	if(st == NULL || st->num < 0 || st->num == INT_MAX) {
		return 0;
	}
	if(st->num_alloc <= (size_t)(st->num + 1)) {
		size_t doub_num_alloc = st->num_alloc * 2;
		const char ** tmpdata;
		/* Overflow checks */
		if(doub_num_alloc < st->num_alloc)
			return 0;
		/* Avoid overflow due to multiplication by sizeof(char *) */
		if(doub_num_alloc > SIZE_MAX / sizeof(char *))
			return 0;
		tmpdata = (const char**)OPENSSL_realloc((char*)st->data, sizeof(char *) * doub_num_alloc);
		if(tmpdata == NULL)
			return 0;
		st->data = tmpdata;
		st->num_alloc = doub_num_alloc;
	}
	if((loc >= st->num) || (loc < 0)) {
		st->data[st->num] = (const char*)data;
	}
	else {
		memmove((void*)&st->data[loc + 1], &st->data[loc], sizeof(st->data[0]) * (st->num - loc)); // @badcast
		st->data[loc] = (const char*)data;
	}
	st->num++;
	st->sorted = 0;
	return st->num;
}

void * FASTCALL OPENSSL_sk_delete_ptr(OPENSSL_STACK * st, const void * p)
{
	for(int i = 0; i < st->num; i++)
		if(st->data[i] == p)
			return OPENSSL_sk_delete(st, i);
	return NULL;
}

void * FASTCALL OPENSSL_sk_delete(OPENSSL_STACK * st, int loc)
{
	const char * ret;
	if(st == NULL || loc < 0 || loc >= st->num)
		return NULL;
	ret = st->data[loc];
	if(loc != st->num - 1)
		memmove((void*)&st->data[loc], &st->data[loc + 1], sizeof(st->data[0]) * (st->num - loc - 1));  // @badcast
	st->num--;
	return (void*)ret;
}

static int internal_find(OPENSSL_STACK * st, const void * data, int ret_val_options)
{
	const void * r;
	int i;
	if(st == NULL)
		return -1;
	if(st->comp == NULL) {
		for(i = 0; i < st->num; i++)
			if(st->data[i] == data)
				return (i);
		return (-1);
	}
	OPENSSL_sk_sort(st);
	if(data == NULL)
		return (-1);
	r = OBJ_bsearch_ex_(&data, st->data, st->num, sizeof(void *), st->comp, ret_val_options);
	if(r == NULL)
		return (-1);
	return (int)((const char**)r - st->data);
}

int FASTCALL OPENSSL_sk_find(OPENSSL_STACK * st, const void * data)
{
	return internal_find(st, data, OBJ_BSEARCH_FIRST_VALUE_ON_MATCH);
}

int FASTCALL OPENSSL_sk_find_ex(OPENSSL_STACK * st, const void * data)
{
	return internal_find(st, data, OBJ_BSEARCH_VALUE_ON_NOMATCH);
}

int FASTCALL OPENSSL_sk_push(OPENSSL_STACK * st, const void * data)
{
	return (OPENSSL_sk_insert(st, data, st->num));
}

int FASTCALL OPENSSL_sk_unshift(OPENSSL_STACK * st, const void * data)
{
	return (OPENSSL_sk_insert(st, data, 0));
}

void * FASTCALL OPENSSL_sk_shift(OPENSSL_STACK * st)
{
	if(st == NULL)
		return NULL;
	else if(st->num <= 0)
		return NULL;
	else
		return OPENSSL_sk_delete(st, 0);
}

void * FASTCALL OPENSSL_sk_pop(OPENSSL_STACK * st)
{
	if(st == NULL)
		return NULL;
	else if(st->num <= 0)
		return NULL;
	else
		return (OPENSSL_sk_delete(st, st->num - 1));
}

void FASTCALL OPENSSL_sk_zero(OPENSSL_STACK * st)
{
	if(st && st->num > 0) {
		memzero((void*)st->data, sizeof(*st->data) * st->num); // @badcast
		st->num = 0;
	}
}

void OPENSSL_sk_pop_free(OPENSSL_STACK * st, OPENSSL_sk_freefunc func)
{
	if(st) {
		for(int i = 0; i < st->num; i++)
			if(st->data[i] != NULL)
				func((char*)st->data[i]);
		OPENSSL_sk_free(st);
	}
}

void OPENSSL_sk_free(OPENSSL_STACK * st)
{
	if(st) {
		OPENSSL_free((void*)st->data); // @badcast
		OPENSSL_free(st);
	}
}

int FASTCALL OPENSSL_sk_num(const OPENSSL_STACK * st)
{
	return st ? st->num : -1;
}

void * FASTCALL OPENSSL_sk_value(const OPENSSL_STACK * st, int i)
{
	return (st == NULL || i < 0 || i >= st->num) ? 0 : (void*)st->data[i];
}

void * FASTCALL OPENSSL_sk_set(OPENSSL_STACK * st, int i, const void * data)
{
	if(st == NULL || i < 0 || i >= st->num)
		return NULL;
	st->data[i] = (const char*)data;
	return (void*)st->data[i];
}

void OPENSSL_sk_sort(OPENSSL_STACK * st)
{
	if(st && !st->sorted && st->comp != NULL) {
		qsort((void*)st->data, st->num, sizeof(char *), st->comp); // @badcast
		st->sorted = 1;
	}
}

int FASTCALL OPENSSL_sk_is_sorted(const OPENSSL_STACK * st)
{
	return st ? st->sorted : 1;
}
