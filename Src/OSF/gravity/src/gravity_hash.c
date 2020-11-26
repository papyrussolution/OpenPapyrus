//  gravity_hash.c
//  gravity
//
//  Created by Marco Bambini on 23/04/15.
//  Copyright (c) 2015 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

#if GRAVITYHASH_ENABLE_STATS
	#define INC_COLLISION(tbl) ++tbl->ncollision
	#define INC_RESIZE(tbl)    ++tbl->nresize
#else
	#define INC_COLLISION(tbl)
	#define INC_RESIZE(tbl)
#endif

struct hash_node_t {
	uint32 hash;
	GravityValue key;
	GravityValue value;
	hash_node_t * next;
};

struct gravity_hash_t {
	gravity_hash_t() : size(0), count(0), isequal_fn(0), compute_fn(0), nodes(0), free_fn(0)
#if GRAVITYHASH_ENABLE_STATS
		,ncollision(0), nresize(0)
#endif
	{
	}
	gravity_hash_t(uint32 sz, uint32 cnt, gravity_hash_isequal_fn fnIsEq, gravity_hash_compute_fn fnCompute) :
		size(sz), count(cnt), isequal_fn(fnIsEq), compute_fn(fnCompute), nodes(0), free_fn(0)
#if GRAVITYHASH_ENABLE_STATS
		,ncollision(0), nresize(0)
#endif
	{
	}
	// internals
	uint32 size;
	uint32 count;
	hash_node_t ** nodes;
	gravity_hash_compute_fn compute_fn;
	gravity_hash_isequal_fn isequal_fn;
	gravity_hash_iterate_fn free_fn;
	void * data;
	// stats
    #if GRAVITYHASH_ENABLE_STATS
		uint32 ncollision;
		uint32 nresize;
    #endif
};

// http://programmers.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed

/*
    Hash algorithm used in Gravity Hash Table is DJB2 which does a pretty good job with string keys and it is fast.
    Original algorithm is: http://www.cse.yorku.ca/~oz/hash.html

    DJBX33A (Daniel J. Bernstein, Times 33 with Addition)

    This is Daniel J. Bernstein's popular `times 33' hash function as
    posted by him years ago on comp.lang.c. It basically uses a function
    like ``hash(i) = hash(i-1)     33 + str[i]''. This is one of the best
    known hash functions for strings. Because it is both computed very
    fast and distributes very well.

    Why 33? (<< 5 in the code)
    The magic of number 33, i.e. why it works better than many other
    constants, prime or not, has never been adequately explained by
    anyone. So I try an explanation: if one experimentally tests all
    multipliers between 1 and 256 (as RSE did now) one detects that even
    numbers are not useable at all. The remaining 128 odd numbers
    (except for the number 1) work more or less all equally well. They
    all distribute in an acceptable way and this way fill a hash table
    with an average percent of approx. 86%.

    If one compares the Chi^2 values of the variants, the number 33 not
    even has the best value. But the number 33 and a few other equally
    good numbers like 17, 31, 63, 127 and 129 have nevertheless a great
    advantage to the remaining numbers in the large set of possible
    multipliers: their multiply operation can be replaced by a faster
    operation based on just one shift plus either a single addition
    or subtraction operation. And because a hash function has to both
    distribute good _and_ has to be very fast to compute, those few
    numbers should be preferred and seems to be the reason why Daniel J.
    Bernstein also preferred it.

    Why 5381?
    1. odd number
    2. prime number
    3. deficient number (https://en.wikipedia.org/wiki/Deficient_number)
    4. 001/010/100/000/101 b

    Practically any good multiplier works. I think you're worrying about
    the fact that 31c + d doesn't cover any reasonable range of hash values
    if c and d are between 0 and 255. That's why, when I discovered the 33 hash
    function and started using it in my compressors, I started with a hash value
    of 5381. I think you'll find that this does just as well as a 261 multiplier.

    Note that the starting value of the hash (5381) makes no difference for strings
    of equal lengths, but will play a role in generating different hash values for
    strings of different lengths.
 */

#define HASH_SEED_VALUE                  5381

#if 0 // (Эта реализация заменена на эквивалентную SlHash::Murmur3_32 @testpassed) {

#define ROT32(x, y)                      ((x << y) | (x >> (32 - y)))
#define COMPUTE_HASH(tbl, key, hash)     uint32 hash = murmur3_32(key, len, HASH_SEED_VALUE); hash = hash % tbl->size
#define COMPUTE_HASH_NOMODULO(key, hash) uint32 hash = murmur3_32(key, len, HASH_SEED_VALUE)
#define RECOMPUTE_HASH(tbl, key, hash)   hash = murmur3_32(key, len, HASH_SEED_VALUE); hash = hash % tbl->size

static /*inline*/ uint32 murmur3_32(const char * key, uint32 len, uint32 seed) 
{
	static const uint32 c1 = 0xcc9e2d51;
	static const uint32 c2 = 0x1b873593;
	static const uint32 r1 = 15;
	static const uint32 r2 = 13;
	static const uint32 m = 5;
	static const uint32 n = 0xe6546b64;
	uint32 hash = seed;
	const int nblocks = len / 4;
	const uint32 * blocks = (const uint32*)key;
	for(int i = 0; i < nblocks; i++) {
		uint32 k = blocks[i];
		k *= c1;
		k = ROT32(k, r1);
		k *= c2;
		hash ^= k;
		hash = ROT32(hash, r2) * m + n;
	}
	const uint8_t * tail = (const uint8_t*)(key + nblocks * 4);
	uint32 k1 = 0;
	switch(len & 3) {
		case 3:
		    k1 ^= tail[2] << 16;
		case 2:
		    k1 ^= tail[1] << 8;
		case 1:
		    k1 ^= tail[0];
		    k1 *= c1;
		    k1 = ROT32(k1, r1);
		    k1 *= c2;
		    hash ^= k1;
	}
	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);
	return hash;
}
#endif // } 0

static void table_dump(gravity_hash_t * hashtable, GravityValue key, GravityValue value, void * data) 
{
	const char * k = reinterpret_cast<gravity_string_t *>(key.Ptr)->cptr();
	printf("%-20s=>\t", k);
	gravity_value_dump(NULL, value, NULL, 0);
}

// MARK: -

gravity_hash_t * gravity_hash_create(uint32 size, gravity_hash_compute_fn compute, gravity_hash_isequal_fn isequal, gravity_hash_iterate_fn free_fn, void * data) 
{
	gravity_hash_t * hashtable = 0;
	if(compute && isequal) {
		if(size < GRAVITYHASH_DEFAULT_SIZE) 
			size = GRAVITYHASH_DEFAULT_SIZE;
		hashtable = (gravity_hash_t *)mem_alloc(NULL, sizeof(gravity_hash_t));
		if(hashtable) {
			if(!(hashtable->nodes = static_cast<hash_node_t **>(mem_calloc(NULL, size, sizeof(hash_node_t*))))) {
				mem_free(hashtable); return NULL;
			}
			hashtable->compute_fn = compute;
			hashtable->isequal_fn = isequal;
			hashtable->free_fn = free_fn;
			hashtable->data = data;
			hashtable->size = size;
		}
	}
	return hashtable;
}

void gravity_hash_free(gravity_hash_t * hashtable) 
{
	if(hashtable) {
		gravity_hash_iterate_fn free_fn = hashtable->free_fn;
		for(uint32 n = 0; n < hashtable->size; ++n) {
			hash_node_t * node = hashtable->nodes[n];
			hashtable->nodes[n] = NULL;
			while(node) {
				if(free_fn) 
					free_fn(hashtable, node->key, node->value, hashtable->data);
				hash_node_t * old_node = node;
				node = node->next;
				mem_free(old_node);
			}
		}
		mem_free(hashtable->nodes);
		mem_free(hashtable);
		hashtable = NULL;
	}
}

uint32 gravity_hash_memsize(const gravity_hash_t * hashtable) 
{
	uint32 size = sizeof(gravity_hash_t) + hashtable->size * sizeof(hash_node_t);
	return size;
}

bool gravity_hash_isempty(gravity_hash_t * hashtable) { return (hashtable->count == 0); }

static /*inline*/int gravity_hash_resize(gravity_hash_t * hashtable) 
{
	uint32 size = (hashtable->size * 2);
	gravity_hash_t newtbl(size, 0, hashtable->isequal_fn, hashtable->compute_fn);
	if(!(newtbl.nodes = static_cast<hash_node_t **>(mem_calloc(NULL, size, sizeof(hash_node_t*))))) 
		return -1;
	else {
		for(uint32 n = 0; n < hashtable->size; ++n) {
			hash_node_t * next;
			for(hash_node_t * node = hashtable->nodes[n]; node; node = next) {
				next = node->next;
				gravity_hash_insert(&newtbl, node->key, node->value);
				// temporary disable free callback registered in hashtable
				// because both key and values are reused in the new table
				gravity_hash_iterate_fn free_fn = hashtable->free_fn;
				hashtable->free_fn = NULL;
				gravity_hash_remove(hashtable, node->key);
				hashtable->free_fn = free_fn;
			}
		}
		mem_free(hashtable->nodes);
		hashtable->size = newtbl.size;
		hashtable->count = newtbl.count;
		hashtable->nodes = newtbl.nodes;
		INC_RESIZE(hashtable);
		return 0;
	}
}

bool gravity_hash_remove(gravity_hash_t * hashtable, GravityValue key) 
{
	uint32 hash = hashtable->compute_fn(key);
	uint32 position = hash % hashtable->size;
	gravity_hash_iterate_fn free_fn = hashtable->free_fn;
	hash_node_t * node = hashtable->nodes[position];
	hash_node_t * prevnode = NULL;
	while(node) {
		if((node->hash == hash) && (hashtable->isequal_fn(key, node->key))) {
			if(free_fn) 
				free_fn(hashtable, node->key, node->value, hashtable->data);
			if(prevnode) 
				prevnode->next = node->next;
			else 
				hashtable->nodes[position] = node->next;
			mem_free(node);
			hashtable->count--;
			return true;
		}
		prevnode = node;
		node = node->next;
	}
	return false;
}

bool FASTCALL gravity_hash_insert(gravity_hash_t * hashtable, GravityValue key, GravityValue value) 
{
	if(hashtable->count >= GRAVITYHASH_MAXENTRIES) 
		return false;
	uint32 hash = hashtable->compute_fn(key);
	uint32 position = hash % hashtable->size;
	hash_node_t * node = hashtable->nodes[position];
	if(node) 
		INC_COLLISION(hashtable);
	// check if the key is already in the table
	while(node) {
		if((node->hash == hash) && (hashtable->isequal_fn(key, node->key))) {
			node->value = value;
			return false;
		}
		node = node->next;
	}
	// resize table if the threshold is exceeded
	// default threshold is: <table size> * <load factor GRAVITYHASH_THRESHOLD>
	if(hashtable->count >= hashtable->size * GRAVITYHASH_THRESHOLD) {
		if(gravity_hash_resize(hashtable) == -1) 
			return false;
		// recompute position here because hashtable->size has changed!
		position = hash % hashtable->size;
	}
	// allocate new entry and set new data
	if(!(node = static_cast<hash_node_t *>(mem_alloc(NULL, sizeof(hash_node_t))))) 
		return false;
	node->key = key;
	node->hash = hash;
	node->value = value;
	node->next = hashtable->nodes[position];
	hashtable->nodes[position] = node;
	++hashtable->count;
	return true;
}

GravityValue * FASTCALL gravity_hash_lookup(gravity_hash_t * hashtable, GravityValue key) 
{
	uint32 hash = hashtable->compute_fn(key);
	uint32 position = hash % hashtable->size;
	hash_node_t * node = hashtable->nodes[position];
	while(node) {
		if((node->hash == hash) && (hashtable->isequal_fn(key, node->key))) 
			return &node->value;
		node = node->next;
	}
	return NULL;
}

GravityValue * gravity_hash_lookup_cstring(gravity_hash_t * hashtable, const char * ckey) 
{
	STATICVALUE_FROM_STRING(key, ckey, strlen(ckey));
	return gravity_hash_lookup(hashtable, key);
}

uint32 FASTCALL gravity_hash_count(const gravity_hash_t * hashtable) { return hashtable->count; }

uint32 FASTCALL gravity_hash_compute_buffer(const char * key, uint32 len) 
{
	return SlHash::Murmur3_32(key, len, HASH_SEED_VALUE);
}

uint32 gravity_hash_compute_int(gravity_int_t n) 
{
	char buffer[24];
	snprintf(buffer, sizeof(buffer), "%" PRId64, n);
	return SlHash::Murmur3_32(buffer, (uint32)strlen(buffer), HASH_SEED_VALUE);
}

uint32 gravity_hash_compute_float(gravity_float_t f) 
{
	char buffer[24];
	// was %g but we don't like scientific notation nor the missing .0 in case of float number with no decimals
	snprintf(buffer, sizeof(buffer), "%f", f);
	return SlHash::Murmur3_32(buffer, (uint32)strlen(buffer), HASH_SEED_VALUE);
}

void gravity_hash_stat(gravity_hash_t * hashtable) 
{
    #if GRAVITYHASH_ENABLE_STATS
	printf("==============\n");
	printf("Collision: %d\n", hashtable->ncollision);
	printf("Resize: %d\n", hashtable->nresize);
	printf("Size: %d\n", hashtable->size);
	printf("Count: %d\n", hashtable->count);
	printf("==============\n");
    #endif
}

void gravity_hash_transform(gravity_hash_t * hashtable, gravity_hash_transform_fn transform, void * data) 
{
	if(hashtable && transform) {
		for(uint32 i = 0; i<hashtable->size; ++i) {
			hash_node_t * node = hashtable->nodes[i];
			while(node) {
				transform(hashtable, node->key, &node->value, data);
				node = node->next;
			}
		}
	}
}

void gravity_hash_iterate(gravity_hash_t * hashtable, gravity_hash_iterate_fn iterate, void * data) 
{
	if((!hashtable) || (!iterate)) return;
	for(uint32 i = 0; i<hashtable->size; ++i) {
		hash_node_t * node = hashtable->nodes[i];
		if(!node) 
			continue;
		while(node) {
			iterate(hashtable, node->key, node->value, data);
			node = node->next;
		}
	}
}

void gravity_hash_iterate2(gravity_hash_t * hashtable, gravity_hash_iterate2_fn iterate, void * data1, void * data2) 
{
	if((!hashtable) || (!iterate)) return;
	for(uint32 i = 0; i<hashtable->size; ++i) {
		hash_node_t * node = hashtable->nodes[i];
		if(!node) continue;
		while(node) {
			iterate(hashtable, node->key, node->value, data1, data2);
			node = node->next;
		}
	}
}

void gravity_hash_iterate3(gravity_hash_t * hashtable, gravity_hash_iterate3_fn iterate, void * data1, void * data2, void * data3) 
{
	if(hashtable && iterate) {
		for(uint32 i = 0; i < hashtable->size; ++i) {
			hash_node_t * node = hashtable->nodes[i];
			while(node) {
				iterate(hashtable, node->key, node->value, data1, data2, data3);
				node = node->next;
			}
		}
	}
}

void gravity_hash_dump(gravity_hash_t * hashtable) { gravity_hash_iterate(hashtable, table_dump, NULL); }

void gravity_hash_append(gravity_hash_t * hashtable1, gravity_hash_t * hashtable2) 
{
	for(uint32 i = 0; i < hashtable2->size; ++i) {
		hash_node_t * node = hashtable2->nodes[i];
		while(node) {
			gravity_hash_insert(hashtable1, node->key, node->value);
			node = node->next;
		}
	}
}

void gravity_hash_resetfree(gravity_hash_t * hashtable) { hashtable->free_fn = NULL; }

bool gravity_hash_compare(gravity_hash_t * hashtable1, gravity_hash_t * hashtable2, gravity_hash_compare_fn compare, void * data) 
{
	if(hashtable1->count != hashtable2->count) 
		return false;
	if(!compare) 
		return false;
	// 1. allocate arrays of keys and values
	gravity_value_r keys1; 
	gravity_value_r values1;
	gravity_value_r keys2; 
	gravity_value_r values2;
	// @ctr marray_init(keys1); 
	// @ctr marray_init(values1);
	// @ctr marray_init(keys2); 
	// @ctr marray_init(values2);
	keys1.resize(hashtable1->count + MARRAY_DEFAULT_SIZE);
	keys2.resize(hashtable1->count + MARRAY_DEFAULT_SIZE);
	values1.resize(hashtable1->count + MARRAY_DEFAULT_SIZE);
	values2.resize(hashtable1->count + MARRAY_DEFAULT_SIZE);
	// 2. build arrays of keys and values for hashtable1
	for(uint32 i = 0; i<hashtable1->size; ++i) {
		hash_node_t * node = hashtable1->nodes[i];
		if(!node) 
			continue;
		while(node) {
			keys1.insert(node->key);
			values1.insert(node->value);
			node = node->next;
		}
	}

	// 3. build arrays of keys and values for hashtable2
	for(uint32 i = 0; i<hashtable2->size; ++i) {
		hash_node_t * node = hashtable2->nodes[i];
		if(!node) 
			continue;
		while(node) {
			keys2.insert(node->key);
			values2.insert(node->value);
			node = node->next;
		}
	}
	// sanity check
	bool result = false;
	uint32 count = keys1.getCount();
	if(count != keys2.getCount()) 
		goto cleanup;
	// 4. compare keys and values
	for(uint32 i = 0; i < count; ++i) {
		if(!compare(keys1.at(i), keys2.at(i), data)) 
			goto cleanup;
		if(!compare(values1.at(i), values2.at(i), data)) 
			goto cleanup;
	}
	result = true;
cleanup:
	keys1.Z();
	keys2.Z();
	values1.Z();
	values2.Z();
	return result;
}
