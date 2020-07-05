/*
 * dict.c: dictionary of reusable strings, just used to avoid allocation
 *    and freeing operations.
 *
 * Copyright (C) 2003-2012 Daniel Veillard.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 * Author: daniel@veillard.com
 */
#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
/*
 * Following http://www.ocert.org/advisories/ocert-2011-003.html
 * it seems that having hash randomization might be a good idea
 * when using XML with untrusted data
 * Note1: that it works correctly only if compiled with WITH_BIG_KEY
 *  which is the default.
 * Note2: the fast function used for a small dict won't protect very
 *  well but since the attack is based on growing a very big hash
 *  list we will use the BigKey algo as soon as the hash size grows
 *  over MIN_DICT_SIZE so this actually works
 */
#if defined(HAVE_RAND) && defined(HAVE_SRAND) && defined(HAVE_TIME)
	#define DICT_RANDOMIZATION
#endif
#ifdef HAVE_STDINT_H
	#include <stdint.h>
#else
#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#elif defined(WIN32)
//typedef unsigned __int32 uint32_t;
#endif
#endif

/* #define DEBUG_GROW */
/* #define DICT_DEBUG_PATTERNS */

#define MAX_HASH_LEN 3
#define MIN_DICT_SIZE 128
#define MAX_DICT_HASH 8 * 2048
#define WITH_BIG_KEY

#ifdef WITH_BIG_KEY
#define xmlDictComputeKey(dict, name, len) (((dict)->size == MIN_DICT_SIZE) ? xmlDictComputeFastKey(name, len, (dict)->seed) : xmlDictComputeBigKey(name, len, (dict)->seed))
#define xmlDictComputeQKey(dict, prefix, plen, name, len) ((!(prefix)) ? (xmlDictComputeKey(dict, name, len)) : \
	(((dict)->size == MIN_DICT_SIZE) ? xmlDictComputeFastQKey(prefix, plen, name, len, (dict)->seed) : xmlDictComputeBigQKey(prefix, plen, name, len, (dict)->seed)))

#else /* !WITH_BIG_KEY */
#define xmlDictComputeKey(dict, name, len)                xmlDictComputeFastKey(name, len, (dict)->seed)
#define xmlDictComputeQKey(dict, prefix, plen, name, len) xmlDictComputeFastQKey(prefix, plen, name, len, (dict)->seed)
#endif /* WITH_BIG_KEY */
/*
 * An entry in the dictionnary
 */
//typedef struct _xmlDictEntry xmlDictEntry;

struct xmlDictEntry {
	xmlDictEntry * next;
	const xmlChar * name;
	uint   len;
	int    valid;
	ulong  okey;
};

//typedef xmlDictEntry * xmlDictEntryPtr;

//typedef struct _xmlDictStrings xmlDictStrings;
//typedef xmlDictStrings * xmlDictStringsPtr;

struct xmlDictStrings {
	xmlDictStrings * next;
	xmlChar * free;
	xmlChar * end;
	size_t size;
	size_t nbStrings;
	xmlChar array[1];
};
// 
// The entire dictionnary
// 
struct xmlDict {
	int    ref_counter;
	xmlDictEntry * dict;
	size_t size;
	uint   nbElems;
	xmlDictStrings * strings;
	xmlDict * subdict;
	int    seed; // used for randomization 
	size_t limit; // used to impose a limit on size 
};

static xmlRMutex * xmlDictMutex = NULL; // A mutex for modifying the reference counter for shared dictionaries.
static int xmlDictInitialized = 0; // Whether the dictionary mutex was initialized.

#ifdef DICT_RANDOMIZATION
	#ifdef HAVE_RAND_R
		static uint rand_seed = 0; // Internal data for random function, protected by xmlDictMutex
	#endif
#endif
/**
 * xmlInitializeDict:
 *
 * Do the dictionary mutex initialization.
 * this function is deprecated
 *
 * Returns 0 if initialization was already done, and 1 if that
 * call led to the initialization
 */
int xmlInitializeDict()
{
	return 0;
}
/**
 * __xmlInitializeDict:
 *
 * This function is not public
 * Do the dictionary mutex initialization.
 * this function is not thread safe, initialization should
 * normally be done once at setup when called from xmlOnceInit()
 * we may also land in this code if thread support is not compiled in
 *
 * Returns 0 if initialization was already done, and 1 if that
 * call led to the initialization
 */
int __xmlInitializeDict()
{
	if(xmlDictInitialized)
		return 1;
	if((xmlDictMutex = xmlNewRMutex()) == NULL)
		return 0;
	xmlRMutexLock(xmlDictMutex);
#ifdef DICT_RANDOMIZATION
#ifdef HAVE_RAND_R
	rand_seed = time(NULL);
	rand_r(&rand_seed);
#else
	srand(time(NULL));
#endif
#endif
	xmlDictInitialized = 1;
	xmlRMutexUnlock(xmlDictMutex);
	return 1;
}

#ifdef DICT_RANDOMIZATION
int __xmlRandom()
{
	int ret;
	if(xmlDictInitialized == 0)
		__xmlInitializeDict();
	xmlRMutexLock(xmlDictMutex);
#ifdef HAVE_RAND_R
	ret = rand_r(&rand_seed);
#else
	ret = rand();
#endif
	xmlRMutexUnlock(xmlDictMutex);
	return ret;
}

#endif
/**
 * xmlDictCleanup:
 *
 * Free the dictionary mutex. Do not call unless sure the library
 * is not in use anymore !
 */
void xmlDictCleanup()
{
	if(xmlDictInitialized) {
		xmlFreeRMutex(xmlDictMutex);
		xmlDictInitialized = 0;
	}
}
/*
 * xmlDictAddString:
 * @dict: the dictionnary
 * @name: the name of the userdata
 * @len: the length of the name
 *
 * Add the string to the array[s]
 *
 * Returns the pointer of the local string, or NULL in case of error.
 */
static const xmlChar * xmlDictAddString(xmlDict * dict, const xmlChar * name, uint namelen)
{
	xmlDictStrings * pool;
	const xmlChar * ret;
	size_t size = 0; /* + sizeof(_xmlDictStrings) == 1024 */
	size_t limit = 0;
#ifdef DICT_DEBUG_PATTERNS
	fprintf(stderr, "-");
#endif
	pool = dict->strings;
	while(pool) {
		if((pool->end - pool->free) > (int)namelen)
			goto found_pool;
		if(pool->size > size) size = pool->size;
		limit += pool->size;
		pool = pool->next;
	}
	/*
	 * Not found, need to allocate
	 */
	if(!pool) {
		if((dict->limit > 0) && (limit > dict->limit)) {
			return 0;
		}
		if(size == 0)
			size = 1000;
		else
			size *= 4;  /* exponential growth */
		if(size < 4 * namelen)
			size = 4 * namelen;  /* just in case ! */
		pool = (xmlDictStrings *)SAlloc::M(sizeof(xmlDictStrings) + size);
		if(!pool)
			return 0;
		pool->size = size;
		pool->nbStrings = 0;
		pool->free = &pool->array[0];
		pool->end = &pool->array[size];
		pool->next = dict->strings;
		dict->strings = pool;
#ifdef DICT_DEBUG_PATTERNS
		fprintf(stderr, "+");
#endif
	}
found_pool:
	ret = pool->free;
	memcpy(pool->free, name, namelen);
	pool->free += namelen;
	*(pool->free++) = 0;
	pool->nbStrings++;
	return ret;
}
/*
 * xmlDictAddQString:
 * @dict: the dictionnary
 * @prefix: the prefix of the userdata
 * @plen: the prefix length
 * @name: the name of the userdata
 * @len: the length of the name
 *
 * Add the QName to the array[s]
 *
 * Returns the pointer of the local string, or NULL in case of error.
 */
static const xmlChar * xmlDictAddQString(xmlDict * dict, const xmlChar * prefix, uint plen, const xmlChar * name, uint namelen)
{
	xmlDictStrings * pool;
	const xmlChar * ret;
	size_t size = 0; /* + sizeof(_xmlDictStrings) == 1024 */
	size_t limit = 0;
	if(prefix == NULL)
		return xmlDictAddString(dict, name, namelen);
#ifdef DICT_DEBUG_PATTERNS
	fprintf(stderr, "=");
#endif
	pool = dict->strings;
	while(pool != NULL) {
		if((pool->end - pool->free) > (int)(namelen + plen + 1))
			goto found_pool;
		if(pool->size > size) 
			size = pool->size;
		limit += pool->size;
		pool = pool->next;
	}
	/*
	 * Not found, need to allocate
	 */
	if(!pool) {
		if((dict->limit > 0) && (limit > dict->limit)) {
			return 0;
		}
		if(size == 0)
			size = 1000;
		else
			size *= 4;  /* exponential growth */
		if(size < 4 * (namelen + plen + 1))
			size = 4 * (namelen + plen + 1);  /* just in case ! */
		pool = (xmlDictStrings *)SAlloc::M(sizeof(xmlDictStrings) + size);
		if(!pool)
			return 0;
		pool->size = size;
		pool->nbStrings = 0;
		pool->free = &pool->array[0];
		pool->end = &pool->array[size];
		pool->next = dict->strings;
		dict->strings = pool;
#ifdef DICT_DEBUG_PATTERNS
		fprintf(stderr, "+");
#endif
	}
found_pool:
	ret = pool->free;
	memcpy(pool->free, prefix, plen);
	pool->free += plen;
	*(pool->free++) = ':';
	memcpy(pool->free, name, namelen);
	pool->free += namelen;
	*(pool->free++) = 0;
	pool->nbStrings++;
	return ret;
}

#ifdef WITH_BIG_KEY
/*
 * xmlDictComputeBigKey:
 *
 * Calculate a hash key using a good hash function that works well for
 * larger hash table sizes.
 *
 * Hash function by "One-at-a-Time Hash" see
 * http://burtleburtle.net/bob/hash/doobs.html
 */
static uint32 FASTCALL xmlDictComputeBigKey(const xmlChar * data, int namelen, int seed)
{
	uint32 hash = 0;
	if(namelen > 0 && data) {
		hash = seed;
		for(int i = 0; i < namelen; i++) {
			hash += data[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
	}
	return hash;
}
/*
 * xmlDictComputeBigQKey:
 *
 * Calculate a hash key for two strings using a good hash function
 * that works well for larger hash table sizes.
 *
 * Hash function by "One-at-a-Time Hash" see
 * http://burtleburtle.net/bob/hash/doobs.html
 *
 * Neither of the two strings must be NULL.
 */
static ulong FASTCALL xmlDictComputeBigQKey(const xmlChar * prefix, int plen, const xmlChar * name, int len, int seed)
{
	int i;
	uint32 hash = seed;
	for(i = 0; i < plen; i++) {
		hash += prefix[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += ':';
	hash += (hash << 10);
	hash ^= (hash >> 6);
	for(i = 0; i < len; i++) {
		hash += name[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

#endif /* WITH_BIG_KEY */
/*
 * xmlDictComputeFastKey:
 *
 * Calculate a hash key using a fast hash function that works well for low hash table fill.
 */
static ulong FASTCALL xmlDictComputeFastKey(const xmlChar * name, int namelen, int seed)
{
	ulong value = seed;
	if(!name)
		return 0;
	value = *name;
	value <<= 5;
	if(namelen > 10) {
		value += name[namelen - 1];
		namelen = 10;
	}
	switch(namelen) {
		case 10: value += name[9];
		case 9: value += name[8];
		case 8: value += name[7];
		case 7: value += name[6];
		case 6: value += name[5];
		case 5: value += name[4];
		case 4: value += name[3];
		case 3: value += name[2];
		case 2: value += name[1];
		default: break;
	}
	return value;
}
/*
 * xmlDictComputeFastQKey:
 *
 * Calculate a hash key for two strings using a fast hash function
 * that works well for low hash table fill.
 *
 * Neither of the two strings must be NULL.
 */
static ulong FASTCALL xmlDictComputeFastQKey(const xmlChar * prefix, int plen, const xmlChar * name, int len, int seed)
{
	ulong value = static_cast<ulong>(seed);
	if(plen == 0)
		value += 30 * static_cast<ulong>(':');
	else
		value += 30 * (*prefix);
	if(len > 10) {
		value += name[len - (plen + 1 + 1)];
		len = 10;
		if(plen > 10)
			plen = 10;
	}
	switch(plen) {
		case 10: value += prefix[9];
		case 9: value += prefix[8];
		case 8: value += prefix[7];
		case 7: value += prefix[6];
		case 6: value += prefix[5];
		case 5: value += prefix[4];
		case 4: value += prefix[3];
		case 3: value += prefix[2];
		case 2: value += prefix[1];
		case 1: value += prefix[0];
		default: break;
	}
	len -= plen;
	if(len > 0) {
		value += static_cast<ulong>(':');
		len--;
	}
	switch(len) {
		case 10: value += name[9];
		case 9: value += name[8];
		case 8: value += name[7];
		case 7: value += name[6];
		case 6: value += name[5];
		case 5: value += name[4];
		case 4: value += name[3];
		case 3: value += name[2];
		case 2: value += name[1];
		case 1: value += name[0];
		default: break;
	}
	return value;
}
/**
 * xmlDictCreate:
 *
 * Create a new dictionary
 *
 * Returns the newly created dictionnary, or NULL if an error occured.
 */
xmlDict * xmlDictCreate()
{
	xmlDict * dict;
	if(!xmlDictInitialized)
		if(!__xmlInitializeDict())
			return 0;
#ifdef DICT_DEBUG_PATTERNS
	fprintf(stderr, "C");
#endif
	dict = static_cast<xmlDict *>(SAlloc::M(sizeof(xmlDict)));
	if(dict) {
		dict->ref_counter = 1;
		dict->limit = 0;
		dict->size = MIN_DICT_SIZE;
		dict->nbElems = 0;
		dict->dict = static_cast<xmlDictEntry *>(SAlloc::M(MIN_DICT_SIZE * sizeof(xmlDictEntry)));
		dict->strings = NULL;
		dict->subdict = NULL;
		if(dict->dict) {
			memzero(dict->dict, MIN_DICT_SIZE * sizeof(xmlDictEntry));
#ifdef DICT_RANDOMIZATION
			dict->seed = __xmlRandom();
#else
			dict->seed = 0;
#endif
			return dict;
		}
		SAlloc::F(dict);
	}
	return 0;
}
/**
 * xmlDictCreateSub:
 * @sub: an existing dictionnary
 *
 * Create a new dictionary, inheriting strings from the read-only
 * dictionnary @sub. On lookup, strings are first searched in the
 * new dictionnary, then in @sub, and if not found are created in the
 * new dictionnary.
 *
 * Returns the newly created dictionnary, or NULL if an error occured.
 */
xmlDict * xmlDictCreateSub(xmlDict * sub)
{
	xmlDict * dict = xmlDictCreate();
	if(dict && sub) {
#ifdef DICT_DEBUG_PATTERNS
		fprintf(stderr, "R");
#endif
		dict->seed = sub->seed;
		dict->subdict = sub;
		xmlDictReference(dict->subdict);
	}
	return (dict);
}
/**
 * xmlDictReference:
 * @dict: the dictionnary
 *
 * Increment the reference counter of a dictionary
 *
 * Returns 0 in case of success and -1 in case of error
 */
int FASTCALL xmlDictReference(xmlDict * dict)
{
	if(!xmlDictInitialized)
		if(!__xmlInitializeDict())
			return -1;
	if(!dict)
		return -1;
	xmlRMutexLock(xmlDictMutex);
	dict->ref_counter++;
	xmlRMutexUnlock(xmlDictMutex);
	return 0;
}
/**
 * xmlDictGrow:
 * @dict: the dictionnary
 * @size: the new size of the dictionnary
 *
 * resize the dictionnary
 *
 * Returns 0 in case of success, -1 in case of failure
 */
static int xmlDictGrow(xmlDict * dict, size_t size)
{
	ulong key, okey;
	size_t oldsize, i;
	xmlDictEntry * iter;
	xmlDictEntry * next;
	xmlDictEntry * olddict;
#ifdef DEBUG_GROW
	ulong nbElem = 0;
#endif
	int ret = 0;
	int keep_keys = 1;
	if(!dict)
		return -1;
	if(size < 8)
		return -1;
	if(size > 8 * 2048)
		return -1;
#ifdef DICT_DEBUG_PATTERNS
	fprintf(stderr, "*");
#endif
	oldsize = dict->size;
	olddict = dict->dict;
	if(olddict == NULL)
		return -1;
	if(oldsize == MIN_DICT_SIZE)
		keep_keys = 0;
	dict->dict = static_cast<xmlDictEntry *>(SAlloc::M(size * sizeof(xmlDictEntry)));
	if(dict->dict == NULL) {
		dict->dict = olddict;
		return -1;
	}
	memzero(dict->dict, size * sizeof(xmlDictEntry));
	dict->size = size;
	/*	If the two loops are merged, there would be situations where
	    a new entry needs to allocated and data copied into it from
	    the main dict. It is nicer to run through the array twice, first
	    copying all the elements in the main array (less probability of
	    allocate) and then the rest, so we only free in the second loop.
	 */
	for(i = 0; i < oldsize; i++) {
		if(olddict[i].valid) {
			okey = keep_keys ? olddict[i].okey : xmlDictComputeKey(dict, olddict[i].name, olddict[i].len);
			key = okey % dict->size;
			if(dict->dict[key].valid == 0) {
				memcpy(&(dict->dict[key]), &(olddict[i]), sizeof(xmlDictEntry));
				dict->dict[key].next = NULL;
				dict->dict[key].okey = okey;
			}
			else {
				xmlDictEntry * entry = static_cast<xmlDictEntry *>(SAlloc::M(sizeof(xmlDictEntry)));
				if(entry) {
					entry->name = olddict[i].name;
					entry->len = olddict[i].len;
					entry->okey = okey;
					entry->next = dict->dict[key].next;
					entry->valid = 1;
					dict->dict[key].next = entry;
				}
				else {
					// we don't have much ways to alert from herei result is loosing an entry and unicity garantee
					ret = -1;
				}
			}
#ifdef DEBUG_GROW
			nbElem++;
#endif
		}
	}
	for(i = 0; i < oldsize; i++) {
		iter = olddict[i].next;
		while(iter) {
			next = iter->next;
			// 
			// put back the entry in the new dict
			// 
			okey = keep_keys ? iter->okey : xmlDictComputeKey(dict, iter->name, iter->len);
			key = okey % dict->size;
			if(dict->dict[key].valid == 0) {
				memcpy(&(dict->dict[key]), iter, sizeof(xmlDictEntry));
				dict->dict[key].next = NULL;
				dict->dict[key].valid = 1;
				dict->dict[key].okey = okey;
				SAlloc::F(iter);
			}
			else {
				iter->next = dict->dict[key].next;
				iter->okey = okey;
				dict->dict[key].next = iter;
			}

#ifdef DEBUG_GROW
			nbElem++;
#endif
			iter = next;
		}
	}
	SAlloc::F(olddict);
#ifdef DEBUG_GROW
	xmlGenericError(0, "xmlDictGrow : from %lu to %lu, %u elems\n", oldsize, size, nbElem);
#endif
	return ret;
}
/**
 * xmlDictFree:
 * @dict: the dictionnary
 *
 * Free the hash @dict and its contents. The userdata is
 * deallocated with @f if provided.
 */
void FASTCALL xmlDictFree(xmlDict * dict)
{
	size_t i;
	xmlDictEntry * iter;
	xmlDictEntry * next;
	int inside_dict = 0;
	if(dict) {
		if(!xmlDictInitialized)
			if(!__xmlInitializeDict())
				return;
		// decrement the counter, it may be shared by a parser and docs 
		xmlRMutexLock(xmlDictMutex);
		dict->ref_counter--;
		if(dict->ref_counter > 0) {
			xmlRMutexUnlock(xmlDictMutex);
		}
		else {
			xmlRMutexUnlock(xmlDictMutex);
			xmlDictFree(dict->subdict); // @recursion
			if(dict->dict) {
				for(i = 0; ((i < dict->size) && (dict->nbElems > 0)); i++) {
					iter = &(dict->dict[i]);
					if(iter->valid) {
						inside_dict = 1;
						while(iter) {
							next = iter->next;
							if(!inside_dict)
								SAlloc::F(iter);
							dict->nbElems--;
							inside_dict = 0;
							iter = next;
						}
					}
				}
				SAlloc::F(dict->dict);
			}
			for(xmlDictStrings * pool = dict->strings; pool;) {
				xmlDictStrings * nextp = pool->next;
				SAlloc::F(pool);
				pool = nextp;
			}
			SAlloc::F(dict);
		}
	}
}

const xmlChar * FASTCALL xmlDictLookupSL(xmlDict * dict, const xmlChar * name)
{
	return xmlDictLookup(dict, name, -1);
}
/**
 * xmlDictLookup:
 * @dict: the dictionnary
 * @name: the name of the userdata
 * @len: the length of the name, if -1 it is recomputed
 *
 * Add the @name to the dictionnary @dict if not present.
 *
 * Returns the internal copy of the name or NULL in case of internal error
 */
const xmlChar * FASTCALL xmlDictLookup(xmlDict * dict, const xmlChar * name, int len)
{
	ulong key, okey, nbi = 0;
	xmlDictEntry * entry;
	xmlDictEntry * insert;
	const xmlChar * ret;
	uint l;
	if(!dict || !name)
		return 0;
	l = (len < 0) ? sstrlen(name) : len;
	if(((dict->limit > 0) && (l >= dict->limit)) || (l > INT_MAX / 2))
		return 0;
	// 
	// Check for duplicate and insertion location.
	// 
	okey = xmlDictComputeKey(dict, name, l);
	key = okey % dict->size;
	if(dict->dict[key].valid == 0) {
		insert = NULL;
	}
	else {
		for(insert = &(dict->dict[key]); insert->next; insert = insert->next) {
#ifdef __GNUC__
			if((insert->okey == okey) && (insert->len == l)) {
				if(!memcmp(insert->name, name, l))
					return insert->name;
			}
#else
			if((insert->okey == okey) && (insert->len == l) && (!xmlStrncmp(insert->name, name, l)))
				return insert->name;
#endif
			nbi++;
		}
#ifdef __GNUC__
		if((insert->okey == okey) && (insert->len == l)) {
			if(!memcmp(insert->name, name, l))
				return insert->name;
		}
#else
		if((insert->okey == okey) && (insert->len == l) && (!xmlStrncmp(insert->name, name, l)))
			return insert->name;
#endif
	}
	if(dict->subdict) {
		ulong skey;
		// we cannot always reuse the same okey for the subdict 
		if(((dict->size == MIN_DICT_SIZE) && (dict->subdict->size != MIN_DICT_SIZE)) || ((dict->size != MIN_DICT_SIZE) && (dict->subdict->size == MIN_DICT_SIZE)))
			skey = xmlDictComputeKey(dict->subdict, name, l);
		else
			skey = okey;
		key = skey % dict->subdict->size;
		if(dict->subdict->dict[key].valid != 0) {
			xmlDictEntry * tmp;
			for(tmp = &(dict->subdict->dict[key]); tmp->next; tmp = tmp->next) {
#ifdef __GNUC__
				if((tmp->okey == skey) && (tmp->len == l)) {
					if(!memcmp(tmp->name, name, l))
						return tmp->name;
				}
#else
				if((tmp->okey == skey) && (tmp->len == l) && (!xmlStrncmp(tmp->name, name, l)))
					return tmp->name;
#endif
				nbi++;
			}
#ifdef __GNUC__
			if((tmp->okey == skey) && (tmp->len == l)) {
				if(!memcmp(tmp->name, name, l))
					return tmp->name;
			}
#else
			if((tmp->okey == skey) && (tmp->len == l) && (!xmlStrncmp(tmp->name, name, l)))
				return tmp->name;
#endif
		}
		key = okey % dict->size;
	}
	ret = xmlDictAddString(dict, name, l);
	if(ret) {
		if(!insert) {
			entry = &(dict->dict[key]);
		}
		else {
			entry = static_cast<xmlDictEntry *>(SAlloc::M(sizeof(xmlDictEntry)));
			if(!entry)
				return 0;
		}
		entry->name = ret;
		entry->len = l;
		entry->next = NULL;
		entry->valid = 1;
		entry->okey = okey;
		if(insert)
			insert->next = entry;
		dict->nbElems++;
		if((nbi > MAX_HASH_LEN) && (dict->size <= ((MAX_DICT_HASH / 2) / MAX_HASH_LEN))) {
			if(xmlDictGrow(dict, MAX_HASH_LEN * 2 * dict->size) != 0)
				return 0;
		}
	}
	return ret; // Note that entry may have been freed at this point by xmlDictGrow 
}
/**
 * xmlDictExists:
 * @dict: the dictionnary
 * @name: the name of the userdata
 * @len: the length of the name, if -1 it is recomputed
 *
 * Check if the @name exists in the dictionnary @dict.
 *
 * Returns the internal copy of the name or NULL if not found.
 */
const xmlChar * xmlDictExists(xmlDict * dict, const xmlChar * name, int len)
{
	ulong key, okey, nbi = 0;
	xmlDictEntry * insert;
	uint l;
	if((dict == NULL) || !name)
		return 0;
	l = (len < 0) ? sstrlen(name) : len;
	if(((dict->limit > 0) && (l >= dict->limit)) || (l > INT_MAX / 2))
		return 0;
	/*
	 * Check for duplicate and insertion location.
	 */
	okey = xmlDictComputeKey(dict, name, l);
	key = okey % dict->size;
	if(dict->dict[key].valid == 0) {
		insert = NULL;
	}
	else {
		for(insert = &(dict->dict[key]); insert->next; insert = insert->next) {
#ifdef __GNUC__
			if((insert->okey == okey) && (insert->len == l)) {
				if(!memcmp(insert->name, name, l))
					return insert->name;
			}
#else
			if((insert->okey == okey) && (insert->len == l) && (!xmlStrncmp(insert->name, name, l)))
				return insert->name;
#endif
			nbi++;
		}
#ifdef __GNUC__
		if((insert->okey == okey) && (insert->len == l)) {
			if(!memcmp(insert->name, name, l))
				return insert->name;
		}
#else
		if((insert->okey == okey) && (insert->len == l) && (!xmlStrncmp(insert->name, name, l)))
			return insert->name;
#endif
	}

	if(dict->subdict) {
		ulong skey;
		/* we cannot always reuse the same okey for the subdict */
		if(((dict->size == MIN_DICT_SIZE) && (dict->subdict->size != MIN_DICT_SIZE)) || ((dict->size != MIN_DICT_SIZE) && (dict->subdict->size == MIN_DICT_SIZE)))
			skey = xmlDictComputeKey(dict->subdict, name, l);
		else
			skey = okey;
		key = skey % dict->subdict->size;
		if(dict->subdict->dict[key].valid != 0) {
			xmlDictEntry * tmp;
			for(tmp = &(dict->subdict->dict[key]); tmp->next; tmp = tmp->next) {
#ifdef __GNUC__
				if((tmp->okey == skey) && (tmp->len == l)) {
					if(!memcmp(tmp->name, name, l))
						return tmp->name;
				}
#else
				if((tmp->okey == skey) && (tmp->len == l) && (!xmlStrncmp(tmp->name, name, l)))
					return tmp->name;
#endif
				nbi++;
			}
#ifdef __GNUC__
			if((tmp->okey == skey) && (tmp->len == l)) {
				if(!memcmp(tmp->name, name, l))
					return tmp->name;
			}
#else
			if((tmp->okey == skey) && (tmp->len == l) && (!xmlStrncmp(tmp->name, name, l)))
				return tmp->name;
#endif
		}
	}
	return 0; // not found 
}
/**
 * xmlDictQLookup:
 * @dict: the dictionnary
 * @prefix: the prefix
 * @name: the name
 *
 * Add the QName @prefix:@name to the hash @dict if not present.
 *
 * Returns the internal copy of the QName or NULL in case of internal error
 */
const xmlChar * xmlDictQLookup(xmlDict * dict, const xmlChar * prefix, const xmlChar * name)
{
	const xmlChar * ret = 0;
	if(dict && name) {
		if(!prefix)
			ret = xmlDictLookupSL(dict, name);
		else {
			const uint l = sstrlen(name);
			const uint plen = sstrlen(prefix);
			const uint len = l + 1 + plen;
			//len += 1 + plen;
			// 
			// Check for duplicate and insertion location.
			// 
			const ulong okey = xmlDictComputeQKey(dict, prefix, plen, name, l);
			ulong key = okey % dict->size;
			ulong nbi = 0;
			xmlDictEntry * insert = 0;
			if(dict->dict[key].valid) {
				for(insert = &(dict->dict[key]); insert->next; insert = insert->next) {
					if(insert->okey == okey && insert->len == len && xmlStrQEqual(prefix, name, insert->name))
						return insert->name;
					nbi++;
				}
				if(insert->okey == okey && insert->len == len && xmlStrQEqual(prefix, name, insert->name))
					return insert->name;
			}
			if(dict->subdict) {
				ulong skey;
				// we cannot always reuse the same okey for the subdict 
				if((dict->size == MIN_DICT_SIZE && dict->subdict->size != MIN_DICT_SIZE) || (dict->size != MIN_DICT_SIZE && dict->subdict->size == MIN_DICT_SIZE))
					skey = xmlDictComputeQKey(dict->subdict, prefix, plen, name, l);
				else
					skey = okey;
				key = skey % dict->subdict->size;
				if(dict->subdict->dict[key].valid != 0) {
					xmlDictEntry * tmp;
					for(tmp = &(dict->subdict->dict[key]); tmp->next; tmp = tmp->next) {
						if(tmp->okey == skey && tmp->len == len && xmlStrQEqual(prefix, name, tmp->name))
							return tmp->name;
						nbi++;
					}
					if(tmp->okey == skey && tmp->len == len && xmlStrQEqual(prefix, name, tmp->name))
						return tmp->name;
				}
				key = okey % dict->size;
			}
			ret = xmlDictAddQString(dict, prefix, plen, name, l);
			if(ret) {
				xmlDictEntry * p_entry = (!insert) ? &dict->dict[key] : (xmlDictEntry *)SAlloc::M(sizeof(xmlDictEntry));
				if(p_entry) {
					p_entry->name = ret;
					p_entry->len = len;
					p_entry->next = NULL;
					p_entry->valid = 1;
					p_entry->okey = okey;
					if(insert)
						insert->next = p_entry;
					dict->nbElems++;
					if(nbi > MAX_HASH_LEN && (dict->size <= ((MAX_DICT_HASH / 2) / MAX_HASH_LEN)))
						xmlDictGrow(dict, MAX_HASH_LEN * 2 * dict->size);
					// Note that entry may have been freed at this point by xmlDictGrow 
				}
				else
					ret = 0;
			}
		}
	}
	return ret;
}
/**
 * xmlDictOwns:
 * @dict: the dictionnary
 * @str: the string
 *
 * check if a string is owned by the disctionary
 *
 * Returns 1 if true, 0 if false and -1 in case of error
 * -1 in case of error
 */
int FASTCALL xmlDictOwns(xmlDict * dict, const xmlChar * str) 
{
	if(!dict || !str)
		return -1;
	else {
		for(xmlDictStrings * pool = dict->strings; pool; pool = pool->next) {
			if((str >= &pool->array[0]) && (str <= pool->free))
				return 1;
		}
		return dict->subdict ? xmlDictOwns(dict->subdict, str) : 0;
	}
}

void FASTCALL XmlDestroyStringWithDict(xmlDict * pDict, xmlChar * pStr)
{
	//#define DICT_FREE(p_dict__, str) if((str) && ((!p_dict__) || (xmlDictOwns(p_dict__, (const xmlChar *)(str)) == 0))) SAlloc::F((char *)(str));
	if(pStr && (!pDict || !xmlDictOwns(pDict, pStr)))
		SAlloc::F(pStr);
}
/**
 * xmlDictSize:
 * @dict: the dictionnary
 *
 * Query the number of elements installed in the hash @dict.
 *
 * Returns the number of elements in the dictionnary or
 * -1 in case of error
 */
int xmlDictSize(xmlDict * dict) 
{
	return dict ? (dict->subdict ? (dict->nbElems + dict->subdict->nbElems) : dict->nbElems) : -1;
}
/**
 * xmlDictSetLimit:
 * @dict: the dictionnary
 * @limit: the limit in bytes
 *
 * Set a size limit for the dictionary
 * Added in 2.9.0
 *
 * Returns the previous limit of the dictionary or 0
 */
size_t xmlDictSetLimit(xmlDict * dict, size_t limit) 
{
	size_t ret = 0;
	if(dict) {
		ret = dict->limit;
		dict->limit = limit;
	}
	return ret;
}
/**
 * xmlDictGetUsage:
 * @dict: the dictionnary
 *
 * Get how much memory is used by a dictionary for strings
 * Added in 2.9.0
 *
 * Returns the amount of strings allocated
 */
size_t xmlDictGetUsage(xmlDict * dict) 
{
	size_t limit = 0;
	if(dict)
		for(xmlDictStrings * pool = dict->strings; pool; pool = pool->next)
			limit += pool->size;
	return limit;
}

#define bottom_dict
//#include "elfgcchack.h"
