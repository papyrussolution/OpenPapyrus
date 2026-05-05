/*
 * Argon2 reference source code package - reference C implementations
 * Copyright 2015 Daniel Dinu, Dmitry Khovratovich, Jean-Philippe Aumasson, and Samuel Neves
 *
 * You may use this work under the terms of a Creative Commons CC0 1.0
 * License/Waiver or the Apache Public License 2.0, at your option. The terms of
 * these licenses can be found at:
 *
 * - CC0 1.0 Universal : https://creativecommons.org/publicdomain/zero/1.0
 * - Apache 2.0        : https://www.apache.org/licenses/LICENSE-2.0
 *
 * You should have received a copy of both of these licenses along with this software. If not, they may be obtained at the above URLs.
 */
#include <slib-internal.h>
#pragma hdrstop
/*For memory wiping*/
#ifdef _WIN32
#endif
#if defined __STDC_LIB_EXT1__
	#define __STDC_WANT_LIB_EXT1__ 1
#endif
#define VC_GE_2005(version) (version >= 1400)
#define _DEFAULT_SOURCE /* for explicit_bzero() on glibc */

#include "core.h"
#include "thread.h"
#include "blake2/blake2.h"
#include "blake2/blake2-impl.h"

#ifdef GENKAT
	#include "genkat.h"
#endif
#if defined(__clang__)
	#if __has_attribute(optnone)
		#define NOT_OPTIMIZED __attribute__((optnone))
	#endif
#elif defined(__GNUC__)
	#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
	#if GCC_VERSION >= 40400
		#define NOT_OPTIMIZED __attribute__((optimize("O0")))
	#endif
#endif
#ifndef NOT_OPTIMIZED
#define NOT_OPTIMIZED
#endif

/***************Instance and Position constructors**********/
void init_block_value(block * b, uint8 in) 
{
	memset(b->v, in, sizeof(b->v));
}

void copy_block(block * dst, const block * src) 
{
	memcpy(dst->v, src->v, sizeof(uint64) * ARGON2_QWORDS_IN_BLOCK);
}

void xor_block(block * dst, const block * src) 
{
	for(int i = 0; i < ARGON2_QWORDS_IN_BLOCK; ++i) {
		dst->v[i] ^= src->v[i];
	}
}

static void load_block(block * dst, const void * input) 
{
	for(uint i = 0; i < ARGON2_QWORDS_IN_BLOCK; ++i) {
		dst->v[i] = load64((const uint8*)input + i * sizeof(dst->v[i]));
	}
}

static void store_block(void * output, const block * src) 
{
	for(uint i = 0; i < ARGON2_QWORDS_IN_BLOCK; ++i) {
		store64((uint8*)output + i * sizeof(src->v[i]), src->v[i]);
	}
}

/***************Memory functions*****************/

int allocate_memory(const argon2_context * context, uint8 ** memory, size_t num, size_t size) 
{
	size_t memory_size = num*size;
	if(!memory) {
		return ARGON2_MEMORY_ALLOCATION_ERROR;
	}
	// 1. Check for multiplication overflow 
	if(size != 0 && memory_size / size != num) {
		return ARGON2_MEMORY_ALLOCATION_ERROR;
	}
	// 2. Try to allocate with appropriate allocator 
	if(context->allocate_cbk) {
		(context->allocate_cbk)(memory, memory_size);
	}
	else {
		*memory = static_cast<uint8 *>(SAlloc::M(memory_size));
	}
	if(*memory == NULL) {
		return ARGON2_MEMORY_ALLOCATION_ERROR;
	}
	return ARGON2_OK;
}

void free_memory(const argon2_context * context, uint8 * memory, size_t num, size_t size) 
{
	const  size_t memory_size = num*size;
	clear_internal_memory(memory, memory_size);
	if(context->free_cbk) {
		(context->free_cbk)(memory, memory_size);
	}
	else {
		SAlloc::F(memory);
	}
}

#if defined(__OpenBSD__)
	#define HAVE_EXPLICIT_BZERO 1
#elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
	#if __GLIBC_PREREQ(2, 25)
		#define HAVE_EXPLICIT_BZERO 1
	#endif
#endif

void NOT_OPTIMIZED secure_wipe_memory(void * v, size_t n) 
{
#if defined(_MSC_VER) && VC_GE_2005(_MSC_VER) || defined(__MINGW32__)
	SecureZeroMemory(v, n);
#elif defined memset_s
	memset_s(v, n, 0, n);
#elif defined(HAVE_EXPLICIT_BZERO)
	explicit_bzero(v, n);
#else
	static void *(*const volatile memset_sec)(void *, int, size_t) = &memset;
	memset_sec(v, 0, n);
#endif
}

int FLAG_clear_internal_memory = 1; // Memory clear flag defaults to true

void clear_internal_memory(void * v, size_t n) 
{
	if(FLAG_clear_internal_memory && v) {
		secure_wipe_memory(v, n);
	}
}

void finalize(const argon2_context * context, argon2_instance_t * instance) 
{
	if(context && instance) {
		block blockhash;
		uint32 l;
		copy_block(&blockhash, instance->memory + instance->lane_length - 1);
		/* XOR the last blocks */
		for(l = 1; l < instance->lanes; ++l) {
			uint32 last_block_in_lane = l * instance->lane_length + (instance->lane_length - 1);
			xor_block(&blockhash, instance->memory + last_block_in_lane);
		}
		/* Hash the result */
		{
			uint8 blockhash_bytes[ARGON2_BLOCK_SIZE];
			store_block(blockhash_bytes, &blockhash);
			blake2b_long(context->out, context->outlen, blockhash_bytes, ARGON2_BLOCK_SIZE);
			/* clear blockhash and blockhash_bytes */
			clear_internal_memory(blockhash.v, ARGON2_BLOCK_SIZE);
			clear_internal_memory(blockhash_bytes, ARGON2_BLOCK_SIZE);
		}
#ifdef GENKAT
		print_tag(context->out, context->outlen);
#endif
		free_memory(context, (uint8*)instance->memory, instance->memory_blocks, sizeof(block));
	}
}

uint32 index_alpha(const argon2_instance_t * instance, const argon2_position_t * position, uint32 pseudo_rand, int same_lane) 
{
	/*
	 * Pass 0:
	 *      This lane : all already finished segments plus already constructed
	 * blocks in this segment
	 *      Other lanes : all already finished segments
	 * Pass 1+:
	 *      This lane : (SYNC_POINTS - 1) last segments plus already constructed
	 * blocks in this segment
	 *      Other lanes : (SYNC_POINTS - 1) last segments
	 */
	uint32 reference_area_size;
	uint64 relative_position;
	uint32 start_position, absolute_position;
	if(0 == position->pass) {
		/* First pass */
		if(0 == position->slice) {
			/* First slice */
			reference_area_size = position->index - 1; /* all but the previous */
		}
		else {
			if(same_lane) {
				/* The same lane => add current segment */
				reference_area_size = position->slice * instance->segment_length + position->index - 1;
			}
			else {
				reference_area_size = position->slice * instance->segment_length + ((position->index == 0) ? (-1) : 0);
			}
		}
	}
	else {
		/* Second pass */
		if(same_lane) {
			reference_area_size = instance->lane_length - instance->segment_length + position->index - 1;
		}
		else {
			reference_area_size = instance->lane_length - instance->segment_length + ((position->index == 0) ? (-1) : 0);
		}
	}
	/* 1.2.4. Mapping pseudo_rand to 0..<reference_area_size-1> and produce relative position */
	relative_position = pseudo_rand;
	relative_position = relative_position * relative_position >> 32;
	relative_position = reference_area_size - 1 - (reference_area_size * relative_position >> 32);
	/* 1.2.5 Computing starting position */
	start_position = 0;
	if(0 != position->pass) {
		start_position = (position->slice == ARGON2_SYNC_POINTS - 1) ? 0 : (position->slice + 1) * instance->segment_length;
	}
	/* 1.2.6. Computing absolute position */
	absolute_position = (start_position + relative_position) % instance->lane_length; /* absolute position */
	return absolute_position;
}

/* Single-threaded version for p=1 case */
static int fill_memory_blocks_st(argon2_instance_t * instance) 
{
	for(uint32 r = 0; r < instance->passes; ++r) {
		for(uint32 s = 0; s < ARGON2_SYNC_POINTS; ++s) {
			for(uint32 l = 0; l < instance->lanes; ++l) {
				argon2_position_t position = {r, l, (uint8)s, 0};
				fill_segment_Opt(instance, position);
			}
		}
#ifdef GENKAT
		internal_kat(instance, r); /* Print all memory blocks */
#endif
	}
	return ARGON2_OK;
}

#if !defined(ARGON2_NO_THREADS)

#ifdef _WIN32
static unsigned __stdcall fill_segment_thr(void * thread_data)
#else
static void * fill_segment_thr(void * thread_data)
#endif
{
	argon2_thread_data * my_data = static_cast<argon2_thread_data *>(thread_data);
	fill_segment_Opt(my_data->instance_ptr, my_data->pos);
	argon2_thread_exit();
	return 0;
}

/* Multi-threaded version for p > 1 case */
static int fill_memory_blocks_mt(argon2_instance_t * instance) 
{
	uint32 r;
	uint32 s;
	argon2_thread_data * thr_data = NULL;
	int rc = ARGON2_OK;
	/* 1. Allocating space for threads */
	argon2_thread_handle_t * thread = static_cast<argon2_thread_handle_t *>(SAlloc::C(instance->lanes, sizeof(argon2_thread_handle_t)));
	if(!thread) {
		rc = ARGON2_MEMORY_ALLOCATION_ERROR;
		goto fail;
	}
	thr_data = static_cast<argon2_thread_data *>(SAlloc::C(instance->lanes, sizeof(argon2_thread_data)));
	if(!thr_data) {
		rc = ARGON2_MEMORY_ALLOCATION_ERROR;
		goto fail;
	}
	for(r = 0; r < instance->passes; ++r) {
		for(s = 0; s < ARGON2_SYNC_POINTS; ++s) {
			uint32 l, ll;
			/* 2. Calling threads */
			for(l = 0; l < instance->lanes; ++l) {
				argon2_position_t position;
				/* 2.1 Join a thread if limit is exceeded */
				if(l >= instance->threads) {
					if(argon2_thread_join(thread[l - instance->threads])) {
						rc = ARGON2_THREAD_FAIL;
						goto fail;
					}
				}
				/* 2.2 Create thread */
				position.pass = r;
				position.lane = l;
				position.slice = (uint8)s;
				position.index = 0;
				thr_data[l].instance_ptr = instance; /* preparing the thread input */
				memcpy(&(thr_data[l].pos), &position, sizeof(argon2_position_t));
				if(argon2_thread_create(&thread[l], &fill_segment_thr, (void*)&thr_data[l])) {
					/* Wait for already running threads */
					for(ll = 0; ll < l; ++ll)
						argon2_thread_join(thread[ll]);
					rc = ARGON2_THREAD_FAIL;
					goto fail;
				}
				/* fill_segment(instance, position); */
				/*Non-thread equivalent of the lines above */
			}
			/* 3. Joining remaining threads */
			for(l = instance->lanes - instance->threads; l < instance->lanes;
			    ++l) {
				if(argon2_thread_join(thread[l])) {
					rc = ARGON2_THREAD_FAIL;
					goto fail;
				}
			}
		}
#ifdef GENKAT
		internal_kat(instance, r); /* Print all memory blocks */
#endif
	}
fail:
	SAlloc::F(thread);
	SAlloc::F(thr_data);
	return rc;
}

#endif /* ARGON2_NO_THREADS */

int fill_memory_blocks(argon2_instance_t * instance) 
{
	if(instance == NULL || instance->lanes == 0) {
		return ARGON2_INCORRECT_PARAMETER;
	}
#if defined(ARGON2_NO_THREADS)
	return fill_memory_blocks_st(instance);
#else
	return instance->threads == 1 ? fill_memory_blocks_st(instance) : fill_memory_blocks_mt(instance);
#endif
}

int validate_inputs(const argon2_context * context) 
{
	if(!context) {
		return ARGON2_INCORRECT_PARAMETER;
	}
	if(!context->out) {
		return ARGON2_OUTPUT_PTR_NULL;
	}
	/* Validate output length */
	if(ARGON2_MIN_OUTLEN > context->outlen) {
		return ARGON2_OUTPUT_TOO_SHORT;
	}
	if(ARGON2_MAX_OUTLEN < context->outlen) {
		return ARGON2_OUTPUT_TOO_LONG;
	}
	/* Validate password (required param) */
	if(!context->pwd) {
		if(context->pwdlen) {
			return ARGON2_PWD_PTR_MISMATCH;
		}
	}
	if(ARGON2_MIN_PWD_LENGTH > context->pwdlen) {
		return ARGON2_PWD_TOO_SHORT;
	}
	if(ARGON2_MAX_PWD_LENGTH < context->pwdlen) {
		return ARGON2_PWD_TOO_LONG;
	}
	/* Validate salt (required param) */
	if(!context->salt) {
		if(context->saltlen) {
			return ARGON2_SALT_PTR_MISMATCH;
		}
	}
	if(ARGON2_MIN_SALT_LENGTH > context->saltlen) {
		return ARGON2_SALT_TOO_SHORT;
	}
	if(ARGON2_MAX_SALT_LENGTH < context->saltlen) {
		return ARGON2_SALT_TOO_LONG;
	}
	/* Validate secret (optional param) */
	if(!context->secret) {
		if(context->secretlen) {
			return ARGON2_SECRET_PTR_MISMATCH;
		}
	}
	else {
		if(ARGON2_MIN_SECRET > context->secretlen) {
			return ARGON2_SECRET_TOO_SHORT;
		}
		if(ARGON2_MAX_SECRET < context->secretlen) {
			return ARGON2_SECRET_TOO_LONG;
		}
	}
	/* Validate associated data (optional param) */
	if(!context->ad) {
		if(context->adlen) {
			return ARGON2_AD_PTR_MISMATCH;
		}
	}
	else {
		if(ARGON2_MIN_AD_LENGTH > context->adlen) {
			return ARGON2_AD_TOO_SHORT;
		}
		if(ARGON2_MAX_AD_LENGTH < context->adlen) {
			return ARGON2_AD_TOO_LONG;
		}
	}
	/* Validate memory cost */
	if(ARGON2_MIN_MEMORY > context->m_cost) {
		return ARGON2_MEMORY_TOO_LITTLE;
	}
	if(ARGON2_MAX_MEMORY < context->m_cost) {
		return ARGON2_MEMORY_TOO_MUCH;
	}
	if(context->m_cost < 8 * context->lanes) {
		return ARGON2_MEMORY_TOO_LITTLE;
	}
	/* Validate time cost */
	if(ARGON2_MIN_TIME > context->t_cost) {
		return ARGON2_TIME_TOO_SMALL;
	}
	if(ARGON2_MAX_TIME < context->t_cost) {
		return ARGON2_TIME_TOO_LARGE;
	}
	/* Validate lanes */
	if(ARGON2_MIN_LANES > context->lanes) {
		return ARGON2_LANES_TOO_FEW;
	}
	if(ARGON2_MAX_LANES < context->lanes) {
		return ARGON2_LANES_TOO_MANY;
	}
	/* Validate threads */
	if(ARGON2_MIN_THREADS > context->threads) {
		return ARGON2_THREADS_TOO_FEW;
	}
	if(ARGON2_MAX_THREADS < context->threads) {
		return ARGON2_THREADS_TOO_MANY;
	}
	if(NULL != context->allocate_cbk && NULL == context->free_cbk) {
		return ARGON2_FREE_MEMORY_CBK_NULL;
	}
	if(NULL == context->allocate_cbk && NULL != context->free_cbk) {
		return ARGON2_ALLOCATE_MEMORY_CBK_NULL;
	}
	return ARGON2_OK;
}

void fill_first_blocks(uint8 * blockhash, const argon2_instance_t * instance) 
{
	// Make the first and second block in each lane as G(H0||0||i) or G(H0||1||i) 
	uint8 blockhash_bytes[ARGON2_BLOCK_SIZE];
	for(uint32 l = 0; l < instance->lanes; ++l) {
		store32(blockhash + ARGON2_PREHASH_DIGEST_LENGTH, 0);
		store32(blockhash + ARGON2_PREHASH_DIGEST_LENGTH + 4, l);
		blake2b_long(blockhash_bytes, ARGON2_BLOCK_SIZE, blockhash, ARGON2_PREHASH_SEED_LENGTH);
		load_block(&instance->memory[l * instance->lane_length + 0], blockhash_bytes);
		store32(blockhash + ARGON2_PREHASH_DIGEST_LENGTH, 1);
		blake2b_long(blockhash_bytes, ARGON2_BLOCK_SIZE, blockhash, ARGON2_PREHASH_SEED_LENGTH);
		load_block(&instance->memory[l * instance->lane_length + 1], blockhash_bytes);
	}
	clear_internal_memory(blockhash_bytes, ARGON2_BLOCK_SIZE);
}

void initial_hash(uint8 * blockhash, argon2_context * context, argon2_type type) 
{
	if(context && blockhash) {
		blake2b_state BlakeHash;
		uint8 value[sizeof(uint32)];
		blake2b_init(&BlakeHash, ARGON2_PREHASH_DIGEST_LENGTH);
		store32(&value, context->lanes);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		store32(&value, context->outlen);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		store32(&value, context->m_cost);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		store32(&value, context->t_cost);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		store32(&value, context->version);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		store32(&value, (uint32)type);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		store32(&value, context->pwdlen);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		if(context->pwd) {
			blake2b_update(&BlakeHash, (const uint8*)context->pwd, context->pwdlen);
			if(context->flags & ARGON2_FLAG_CLEAR_PASSWORD) {
				secure_wipe_memory(context->pwd, context->pwdlen);
				context->pwdlen = 0;
			}
		}
		store32(&value, context->saltlen);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		if(context->salt) {
			blake2b_update(&BlakeHash, (const uint8*)context->salt, context->saltlen);
		}
		store32(&value, context->secretlen);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		if(context->secret) {
			blake2b_update(&BlakeHash, (const uint8*)context->secret, context->secretlen);
			if(context->flags & ARGON2_FLAG_CLEAR_SECRET) {
				secure_wipe_memory(context->secret, context->secretlen);
				context->secretlen = 0;
			}
		}
		store32(&value, context->adlen);
		blake2b_update(&BlakeHash, (const uint8*)&value, sizeof(value));
		if(context->ad) {
			blake2b_update(&BlakeHash, (const uint8*)context->ad, context->adlen);
		}
		blake2b_final(&BlakeHash, blockhash, ARGON2_PREHASH_DIGEST_LENGTH);
	}
}

int initialize(argon2_instance_t * instance, argon2_context * context) 
{
	uint8 blockhash[ARGON2_PREHASH_SEED_LENGTH];
	int result = ARGON2_OK;
	if(instance == NULL || context == NULL)
		return ARGON2_INCORRECT_PARAMETER;
	instance->context_ptr = context;
	// 1. Memory allocation
	result = allocate_memory(context, (uint8**)&(instance->memory), instance->memory_blocks, sizeof(block));
	if(result != ARGON2_OK) {
		return result;
	}
	/* 2. Initial hashing */
	/* H_0 + 8 extra bytes to produce the first blocks */
	/* uint8 blockhash[ARGON2_PREHASH_SEED_LENGTH]; */
	/* Hashing all inputs */
	initial_hash(blockhash, context, instance->type);
	/* Zeroing 8 extra bytes */
	clear_internal_memory(blockhash + ARGON2_PREHASH_DIGEST_LENGTH, ARGON2_PREHASH_SEED_LENGTH - ARGON2_PREHASH_DIGEST_LENGTH);
#ifdef GENKAT
	initial_kat(blockhash, context, instance->type);
#endif
	// 3. Creating first blocks, we always have at least two blocks in a slice
	fill_first_blocks(blockhash, instance);
	/* Clearing the hash */
	clear_internal_memory(blockhash, ARGON2_PREHASH_SEED_LENGTH);
	return ARGON2_OK;
}
