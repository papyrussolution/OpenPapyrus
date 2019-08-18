/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2013
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */
#include "amqp_private.h"
#pragma hdrstop

char const * amqp_version() { return AMQP_VERSION_STRING; }
uint32 amqp_version_number() { return AMQP_VERSION; }

void FASTCALL init_amqp_pool(amqp_pool_t * pPool, size_t pagesize) 
{
	if(pPool) {
		pPool->pagesize = pagesize ? pagesize : 4096;
		pPool->pages.num_blocks = 0;
		pPool->pages.blocklist = NULL;
		pPool->large_blocks.num_blocks = 0;
		pPool->large_blocks.blocklist = NULL;
		pPool->next_page = 0;
		pPool->alloc_block = NULL;
		pPool->alloc_used = 0;
	}
}

static void FASTCALL empty_blocklist(amqp_pool_blocklist_t * x) 
{
	if(x->blocklist) {
		for(int i = 0; i < x->num_blocks; i++) {
			SAlloc::F(x->blocklist[i]);
		}
		SAlloc::F(x->blocklist);
	}
	x->num_blocks = 0;
	x->blocklist = NULL;
}

void recycle_amqp_pool(amqp_pool_t * pPool) 
{
	if(pPool) {
		empty_blocklist(&pPool->large_blocks);
		pPool->next_page = 0;
		pPool->alloc_block = NULL;
		pPool->alloc_used = 0;
	}
}

void FASTCALL empty_amqp_pool(amqp_pool_t * pPool) 
{
	if(pPool) {
		recycle_amqp_pool(pPool);
		empty_blocklist(&pPool->pages);
	}
}

/* Returns 1 on success, 0 on failure */
static int record_pool_block(amqp_pool_blocklist_t * x, void * block) 
{
	size_t blocklistlength = sizeof(void *) * (x->num_blocks + 1);
	if(!x->blocklist) {
		x->blocklist = static_cast<void **>(SAlloc::M(blocklistlength));
		if(!x->blocklist)
			return 0;
	}
	else {
		void * newbl = SAlloc::R(x->blocklist, blocklistlength);
		if(!newbl)
			return 0;
		else
			x->blocklist = static_cast<void **>(newbl);
	}
	x->blocklist[x->num_blocks] = block;
	x->num_blocks++;
	return 1;
}

void * FASTCALL amqp_pool_alloc(amqp_pool_t * pool, size_t amount) 
{
	if(amount == 0) {
		return NULL;
	}
	amount = (amount + 7) & (~7); // round up to nearest 8-byte boundary 
	if(amount > pool->pagesize) {
		void * result = SAlloc::C(1, amount);
		if(!result) {
			return NULL;
		}
		else if(!record_pool_block(&pool->large_blocks, result)) {
			SAlloc::F(result);
			return NULL;
		}
		else
			return result;
	}
	if(pool->alloc_block) {
		assert(pool->alloc_used <= pool->pagesize);
		if((pool->alloc_used + amount) <= pool->pagesize) {
			void * result = pool->alloc_block + pool->alloc_used;
			pool->alloc_used += amount;
			return result;
		}
	}
	if(pool->next_page >= pool->pages.num_blocks) {
		pool->alloc_block = static_cast<char *>(SAlloc::C(1, pool->pagesize));
		if(!pool->alloc_block)
			return NULL;
		else if(!record_pool_block(&pool->pages, pool->alloc_block))
			return NULL;
		else
			pool->next_page = pool->pages.num_blocks;
	}
	else {
		pool->alloc_block = static_cast<char *>(pool->pages.blocklist[pool->next_page]);
		pool->next_page++;
	}
	pool->alloc_used = amount;
	return pool->alloc_block;
}

void FASTCALL amqp_pool_alloc_bytes(amqp_pool_t * pool, size_t amount, amqp_bytes_t * output) 
{
	output->len = amount;
	output->bytes = amqp_pool_alloc(pool, amount);
}

amqp_bytes_t amqp_cstring_bytes(char const * cstr) 
{
	amqp_bytes_t result;
	result.len = sstrlen(cstr);
	result.bytes = const_cast<void *>(static_cast<const void *>(cstr)); // @badcast
	return result;
}

amqp_bytes_t FASTCALL amqp_bytes_malloc_dup(const amqp_bytes_t & rS) 
{
	amqp_bytes_t result;
	result.len = rS.len;
	result.bytes = SAlloc::M(rS.len);
	if(result.bytes)
		memcpy(result.bytes, rS.bytes, rS.len);
	return result;
}

amqp_bytes_t amqp_bytes_malloc(size_t amount) 
{
	amqp_bytes_t result;
	result.len = amount;
	result.bytes = SAlloc::M(amount); // will return NULL if it fails 
	return result;
}

void FASTCALL amqp_bytes_free(amqp_bytes_t & rBytes) 
{
	SAlloc::F(rBytes.bytes);
	rBytes.bytes = 0; // @sobolev
	rBytes.len = 0; // @sobolev
}

amqp_pool_t * amqp_get_or_create_channel_pool(amqp_connection_state_t state, amqp_channel_t channel) 
{
	size_t index = channel % POOL_TABLE_SIZE;
	amqp_pool_table_entry_t * entry = state->pool_table[index];
	for(; entry; entry = entry->next) {
		if(channel == entry->channel)
			return &entry->pool;
	}
	entry = static_cast<amqp_pool_table_entry_t *>(SAlloc::M(sizeof(amqp_pool_table_entry_t)));
	if(entry) {
		entry->channel = channel;
		entry->next = state->pool_table[index];
		state->pool_table[index] = entry;
		init_amqp_pool(&entry->pool, state->frame_max);
		return &entry->pool;
	}
	else
		return 0;
}

amqp_pool_t * amqp_get_channel_pool(amqp_connection_state_t state, amqp_channel_t channel) 
{
	size_t index = channel % POOL_TABLE_SIZE;
	for(amqp_pool_table_entry_t * entry = state->pool_table[index]; entry; entry = entry->next) {
		if(channel == entry->channel)
			return &entry->pool;
	}
	return NULL;
}

int amqp_bytes_equal(const amqp_bytes_t r, const amqp_bytes_t l) 
{
	return (r.len == l.len && (r.bytes == l.bytes || 0 == memcmp(r.bytes, l.bytes, r.len))) ? 1 : 0;
}
