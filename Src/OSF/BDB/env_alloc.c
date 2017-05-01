/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * Implement shared memory region allocation.  The initial list is a single
 * memory "chunk" which is carved up as memory is requested.  Chunks are
 * coalesced when free'd.  We maintain two types of linked-lists: a list of
 * all chunks sorted by address, and a set of lists with free chunks sorted
 * by size.
 *
 * The ALLOC_LAYOUT structure is the governing structure for the allocator.
 *
 * The ALLOC_ELEMENT structure is the structure that describes any single
 * chunk of memory, and is immediately followed by the user's memory.
 *
 * The internal memory chunks are always aligned to a uintmax_t boundary so
 * we don't drop core accessing the fields of the ALLOC_ELEMENT structure.
 *
 * The memory chunks returned to the user are aligned to a uintmax_t boundary.
 * This is enforced by terminating the ALLOC_ELEMENT structure with a uintmax_t
 * field as that immediately precedes the user's memory.  Any caller needing
 * more than uintmax_t alignment is responsible for doing alignment themselves.
 */

typedef SH_TAILQ_HEAD (__sizeq) SIZEQ_HEAD;

typedef struct __alloc_layout {
	SH_TAILQ_HEAD(__addrq) addrq;           /* Sorted by address */

	/*
	 * A perfect Berkeley DB application does little allocation because
	 * most things are allocated on startup and never free'd.  This is
	 * true even for the cache, because we don't free and re-allocate
	 * the memory associated with a cache buffer when swapping a page
	 * in memory for a page on disk -- unless the page is changing size.
	 * The latter problem is why we have multiple size queues.  If the
	 * application's working set fits in cache, it's not a problem.  If
	 * the application's working set doesn't fit in cache, but all of
	 * the databases have the same size pages, it's still not a problem.
	 * If the application's working set doesn't fit in cache, and its
	 * databases have different page sizes, we can end up walking a lot
	 * of 512B chunk allocations looking for an available 64KB chunk.
	 *
	 * So, we keep a set of queues, where we expect to find a chunk of
	 * roughly the right size at the front of the list.  The first queue
	 * is chunks <= 1024, the second is <= 2048, and so on.  With 11
	 * queues, we have separate queues for chunks up to 1MB.
	 */
#define DB_SIZE_Q_COUNT 11
	SIZEQ_HEAD sizeq[DB_SIZE_Q_COUNT];      /* Sorted by size */
#ifdef HAVE_STATISTICS
	uint32 pow2_size[DB_SIZE_Q_COUNT];
#endif

#ifdef HAVE_STATISTICS
	uint32 success;                      /* Successful allocations */
	uint32 failure;                      /* Failed allocations */
	uint32 freed;                        /* Free calls */
	uint32 longest;                      /* Longest chain walked */
#endif
	uintmax_t unused;                       /* Guarantee alignment */
} ALLOC_LAYOUT;

typedef struct __alloc_element {
	SH_TAILQ_ENTRY addrq;                   /* List by address */
	SH_TAILQ_ENTRY sizeq;                   /* List by size */

	/*
	 * The "len" field is the total length of the chunk, not the size
	 * available to the caller.  Use a uintmax_t to guarantee that the
	 * size of this struct will be aligned correctly.
	 */
	uintmax_t len;                          /* Chunk length */

	/*
	 * The "ulen" field is the length returned to the caller.
	 *
	 * Set to 0 if the chunk is not currently in use.
	 */
	uintmax_t ulen;                         /* User's length */
} ALLOC_ELEMENT;

/*
 * If the chunk can be split into two pieces, with the fragment holding at
 * least 64 bytes of memory, we divide the chunk into two parts.
 */
#define SHALLOC_FRAGMENT        (sizeof(ALLOC_ELEMENT)+64)

/* Macro to find the appropriate queue for a specific size chunk. */
#undef  SET_QUEUE_FOR_SIZE
#define SET_QUEUE_FOR_SIZE(head, q, i, len) do {                        \
		for(i = 0; i < DB_SIZE_Q_COUNT; ++i) {                         \
			q = &(head)->sizeq[i];                                  \
			if((len) <= (u_int64_t)1024<<i)                      \
				break;                                          \
		}                                                               \
} while(0)

static void __env_size_insert(ALLOC_LAYOUT*, ALLOC_ELEMENT *);
/*
 * __env_alloc_init --
 *	Initialize the area as one large chunk.
 */
void __env_alloc_init(REGINFO * infop, size_t size)
{
	ALLOC_ELEMENT * elp;
	ALLOC_LAYOUT * head;
	uint i;
	ENV * env = infop->env;
	/* No initialization needed for heap memory regions. */
	if(F_ISSET(env, ENV_PRIVATE))
		return;
	/*
	 * The first chunk of memory is the ALLOC_LAYOUT structure.
	 */
	head = (ALLOC_LAYOUT *)infop->head;
	memzero(head, sizeof(*head));
	SH_TAILQ_INIT(&head->addrq);
	for(i = 0; i < DB_SIZE_Q_COUNT; ++i)
		SH_TAILQ_INIT(&head->sizeq[i]);
	COMPQUIET(head->unused, 0);
	/*
	 * The rest of the memory is the first available chunk.
	 */
	elp = (ALLOC_ELEMENT *)((uint8 *)head+sizeof(ALLOC_LAYOUT));
	elp->len = size-sizeof(ALLOC_LAYOUT);
	elp->ulen = 0;

	SH_TAILQ_INSERT_HEAD(&head->addrq, elp, addrq, __alloc_element);
	SH_TAILQ_INSERT_HEAD(&head->sizeq[DB_SIZE_Q_COUNT-1], elp, sizeq, __alloc_element);
}
/*
 * The length, the ALLOC_ELEMENT structure and an optional guard byte,
 * rounded up to standard alignment.
 */
#ifdef DIAGNOSTIC
	#define DB_ALLOC_SIZE(len) (size_t)DB_ALIGN((len)+sizeof(ALLOC_ELEMENT)+1, sizeof(uintmax_t))
#else
	#define DB_ALLOC_SIZE(len) (size_t)DB_ALIGN((len)+sizeof(ALLOC_ELEMENT), sizeof(uintmax_t))
#endif
/*
 * __env_alloc_overhead --
 *	Return the overhead needed for an allocation.
 */
size_t __env_alloc_overhead()
{
	return sizeof(ALLOC_ELEMENT);
}
/*
 * __env_alloc_size --
 *	Return the space needed for an allocation, including alignment.
 */
size_t FASTCALL __env_alloc_size(size_t len)
{
	return DB_ALLOC_SIZE(len);
}
/*
 * __env_alloc --
 *	Allocate space from the shared region.
 */
int __env_alloc(REGINFO * infop, size_t len, void * retp)
{
	SIZEQ_HEAD * q;
	ALLOC_ELEMENT * elp, * frag, * elp_tmp;
	ALLOC_LAYOUT * head;
	ENV * env;
	REGION_MEM * mem;
	REGINFO * envinfop;
	size_t total_len;
	uint8 * p;
	uint i;
	int ret;
#ifdef HAVE_STATISTICS
	uint32 st_search;
#endif
	env = infop->env;
	*(void **)retp = NULL;
#ifdef HAVE_MUTEX_SUPPORT
	MUTEX_REQUIRED(env, infop->mtx_alloc);
#endif
	PERFMON3(env, mpool, env_alloc, len, infop->id, infop->type);
	/*
	 * In a heap-backed environment, we call malloc for additional space.
	 * (Malloc must return memory correctly aligned for our use.)
	 *
	 * In a heap-backed environment, memory is laid out as follows:
	 *
	 * { uintmax_t total-length } { user-memory } { guard-byte }
	 */
	if(F_ISSET(env, ENV_PRIVATE)) {
		//
		// If we are shared then we must track the allocation in the main environment region.
		//
		envinfop = F_ISSET(infop, REGION_SHARED) ? env->reginfo : infop;
		/*
		 * We need an additional uintmax_t to hold the length (and
		 * keep the buffer aligned on 32-bit systems).
		 */
		len += sizeof(uintmax_t);
		if(F_ISSET(infop, REGION_TRACKED))
			len += sizeof(REGION_MEM);
#ifdef DIAGNOSTIC
		/* Plus one byte for the guard byte. */
		++len;
#endif
		/* Check if we're over the limit. */
		if(envinfop->max_alloc != 0 && envinfop->allocated+len > envinfop->max_alloc)
			return ENOMEM;
		/* Allocate the space. */
		if((ret = __os_malloc(env, len, &p)) != 0)
			return ret;
		infop->allocated += len;
		if(infop != envinfop)
			envinfop->allocated += len;
		*(uintmax_t *)p = len;
#ifdef DIAGNOSTIC
		p[len-1] = GUARD_BYTE;
#endif
		if(F_ISSET(infop, REGION_TRACKED)) {
			mem = (REGION_MEM *)(p+sizeof(uintmax_t));
			mem->next = infop->mem;
			infop->mem = mem;
			p += sizeof(mem);
		}
		*(void **)retp = p+sizeof(uintmax_t);
		return 0;
	}
	head = (ALLOC_LAYOUT *)infop->head;
	total_len = DB_ALLOC_SIZE(len);
	/* Find the first size queue that could satisfy the request. */
	COMPQUIET(q, NULL);
#ifdef HAVE_MMAP_EXTEND
retry:
#endif
	SET_QUEUE_FOR_SIZE(head, q, i, total_len);
#ifdef HAVE_STATISTICS
	if(i >= DB_SIZE_Q_COUNT)
		i = DB_SIZE_Q_COUNT-1;
	++head->pow2_size[i];           /* Note the size of the request. */
#endif
	/*
	 * Search this queue, and, if necessary, queues larger than this queue,
	 * looking for a chunk we can use.
	 */
	STAT(st_search = 0);
	for(elp = NULL;; ++q) {
		SH_TAILQ_FOREACH(elp_tmp, q, sizeq, __alloc_element) {
			STAT(++st_search);
			/*
			 * Chunks are sorted from largest to smallest -- if
			 * this chunk is less than what we need, no chunk
			 * further down the list will be large enough.
			 */
			if(elp_tmp->len < total_len)
				break;
			/*
			 * This chunk will do... maybe there's a better one, but this one will do.
			 */
			elp = elp_tmp;
			/*
			 * We might have many chunks of the same size.  Stop
			 * looking if we won't fragment memory by picking the current one.
			 */
			if(elp_tmp->len-total_len <= SHALLOC_FRAGMENT)
				break;
		}
		if(elp != NULL || ++i >= DB_SIZE_Q_COUNT)
			break;
	}
#ifdef HAVE_STATISTICS
	if(head->longest < st_search) {
		head->longest = st_search;
		STAT_PERFMON3(env, mpool, longest_search, len, infop->id, st_search);
	}
#endif
	//
	// If we don't find an element of the right size, try to extend
	// the region, if not then we are done.
	//
	if(elp == NULL) {
		ret = ENOMEM;
#ifdef HAVE_MMAP_EXTEND
		if(infop->rp->size < infop->rp->max && (ret = __env_region_extend(env, infop)) == 0)
			goto retry;
#endif
		STAT_INC_VERB(env, mpool, fail, head->failure, len, infop->id);
		return ret;
	}
	STAT_INC_VERB(env, mpool, alloc, head->success, len, infop->id);
	// Pull the chunk off of the size queue
	SH_TAILQ_REMOVE(q, elp, sizeq, __alloc_element);
	if(elp->len-total_len > SHALLOC_FRAGMENT) {
		frag = (ALLOC_ELEMENT *)((uint8 *)elp+total_len);
		frag->len = elp->len-total_len;
		frag->ulen = 0;
		elp->len = total_len;
		// The fragment follows the chunk on the address queue
		SH_TAILQ_INSERT_AFTER(&head->addrq, elp, frag, addrq, __alloc_element);
		// Insert the frag into the correct size queue
		__env_size_insert(head, frag);
	}
	p = (uint8 *)elp+sizeof(ALLOC_ELEMENT);
	elp->ulen = len;
#ifdef DIAGNOSTIC
	p[len] = GUARD_BYTE;
#endif
	*(void **)retp = p;
	return 0;
}
//
// __env_alloc_free --
// Free space into the shared region.
//
void FASTCALL __env_alloc_free(REGINFO * infop, void * ptr)
{
	ALLOC_ELEMENT * elp, * elp_tmp;
	ALLOC_LAYOUT * head;
	SIZEQ_HEAD * q;
	size_t len;
	uint8 i, * p;
	ENV * env = infop->env;
	// In a private region, we call free
	if(F_ISSET(env, ENV_PRIVATE)) {
		// Find the start of the memory chunk and its length
		p = (uint8 *)((uintmax_t *)ptr-1);
		len = (size_t)*(uintmax_t *)p;
		infop->allocated -= len;
		if(F_ISSET(infop, REGION_SHARED))
			env->reginfo->allocated -= len;
#ifdef DIAGNOSTIC
		DB_ASSERT(env, p[len-1] == GUARD_BYTE); /* Check the guard byte. */
		memset(p, CLEAR_BYTE, len); /* Trash the memory chunk. */
#endif
		__os_free(env, p);
	}
	else {
#ifdef HAVE_MUTEX_SUPPORT
		MUTEX_REQUIRED(env, infop->mtx_alloc);
#endif
		head = (ALLOC_LAYOUT *)infop->head;
		p = (uint8 *)ptr;
		elp = (ALLOC_ELEMENT *)(p-sizeof(ALLOC_ELEMENT));
		STAT_INC_VERB(env, mpool, free, head->freed, elp->ulen, infop->id);
#ifdef DIAGNOSTIC
		// Check the guard byte
		DB_ASSERT(env, p[elp->ulen] == GUARD_BYTE);
		// Trash the memory chunk
		memset(p, CLEAR_BYTE, (size_t)elp->len-sizeof(ALLOC_ELEMENT));
#endif
		// Mark the memory as no longer in use
		elp->ulen = 0;
		//
		// Try and merge this chunk with chunks on either side of it.  Two
		// chunks can be merged if they're contiguous and not in use.
		//
		if((elp_tmp = SH_TAILQ_PREV(&head->addrq, elp, addrq, __alloc_element)) != NULL && elp_tmp->ulen == 0 && (uint8 *)elp_tmp+elp_tmp->len == (uint8 *)elp) {
			// If we're merging the entry into a previous entry, remove the
			// current entry from the addr queue and the previous entry from its size queue, and merge.
			SH_TAILQ_REMOVE(&head->addrq, elp, addrq, __alloc_element);
			SET_QUEUE_FOR_SIZE(head, q, i, elp_tmp->len);
			SH_TAILQ_REMOVE(q, elp_tmp, sizeq, __alloc_element);

			elp_tmp->len += elp->len;
			elp = elp_tmp;
		}
		if((elp_tmp = SH_TAILQ_NEXT(elp, addrq, __alloc_element)) != NULL && elp_tmp->ulen == 0 && (uint8 *)elp+elp->len == (uint8 *)elp_tmp) {
			// If we're merging the current entry into a subsequent entry,
			// remove the subsequent entry from the addr and size queues and merge.
			SH_TAILQ_REMOVE(&head->addrq, elp_tmp, addrq, __alloc_element);
			SET_QUEUE_FOR_SIZE(head, q, i, elp_tmp->len);
			SH_TAILQ_REMOVE(q, elp_tmp, sizeq, __alloc_element);

			elp->len += elp_tmp->len;
		}
		// Insert in the correct place in the size queues
		__env_size_insert(head, elp);
	}
}
/*
 * __env_alloc_extend --
 *	Extend a previously allocated chunk at the end of a region.
 */
int __env_alloc_extend(REGINFO * infop, void * ptr, size_t * lenp)
{
	ALLOC_ELEMENT * elp, * elp_tmp;
	ALLOC_LAYOUT * head;
	ENV * env;
	SIZEQ_HEAD * q;
	size_t len, tlen;
	uint8 i, * p;
	int ret;
	env = infop->env;
	DB_ASSERT(env, !F_ISSET(env, ENV_PRIVATE));
#ifdef HAVE_MUTEX_SUPPORT
	MUTEX_REQUIRED(env, infop->mtx_alloc);
#endif
	head = (ALLOC_LAYOUT *)infop->head;
	p = (uint8 *)ptr;
	len = *lenp;
	elp = (ALLOC_ELEMENT *)(p-sizeof(ALLOC_ELEMENT));
#ifdef DIAGNOSTIC
	/* Check the guard byte. */
	DB_ASSERT(env, p[elp->ulen] == GUARD_BYTE);
#endif
	/* See if there is anything left in the region. */
again:
	if((elp_tmp = SH_TAILQ_NEXT(elp, addrq, __alloc_element)) != NULL && elp_tmp->ulen == 0 && (uint8 *)elp+elp->len == (uint8 *)elp_tmp) {
		/*
		 * If we're merging the current entry into a subsequent entry,
		 * remove the subsequent entry from the addr and size queues and merge.
		 */
		SH_TAILQ_REMOVE(&head->addrq, elp_tmp, addrq, __alloc_element);
		SET_QUEUE_FOR_SIZE(head, q, i, elp_tmp->len);
		SH_TAILQ_REMOVE(q, elp_tmp, sizeq, __alloc_element);
		if(elp_tmp->len < len+SHALLOC_FRAGMENT) {
			elp->len += elp_tmp->len;
			if(elp_tmp->len < len)
				len -= (size_t)elp_tmp->len;
			else
				len = 0;
		}
		else {
			tlen = (size_t)elp_tmp->len;
			elp_tmp = (ALLOC_ELEMENT *)((uint8 *)elp_tmp+len);
			elp_tmp->len = tlen-len;
			elp_tmp->ulen = 0;
			elp->len += len;
			len = 0;
			/* The fragment follows the on the address queue. */
			SH_TAILQ_INSERT_AFTER(&head->addrq, elp, elp_tmp, addrq, __alloc_element);
			/* Insert the frag into the correct size queue. */
			__env_size_insert(head, elp_tmp);
		}
	}
	else if(elp_tmp != NULL) {
		__db_errx(env, DB_STR("1583", "block not at end of region"));
		return __env_panic(env, EINVAL);
	}
	if(len == 0)
		goto done;
	if((ret = __env_region_extend(env, infop)) != 0) {
		if(ret != ENOMEM)
			return ret;
		goto done;
	}
	goto again;

done:   elp->ulen = elp->len-sizeof(ALLOC_ELEMENT);
#ifdef DIAGNOSTIC
	elp->ulen -= sizeof(uintmax_t);
	/* There was room for the guarrd byte in the chunk that came in. */
	p[elp->ulen] = GUARD_BYTE;
#endif
	*lenp -= len;
	infop->allocated += *lenp;
	if(F_ISSET(infop, REGION_SHARED))
		env->reginfo->allocated += *lenp;
	return 0;
}
/*
 * __env_size_insert --
 *	Insert into the correct place in the size queues.
 */
static void __env_size_insert(ALLOC_LAYOUT * head, ALLOC_ELEMENT * elp)
{
	SIZEQ_HEAD * q;
	ALLOC_ELEMENT * elp_tmp;
	uint i;
	/* Find the appropriate queue for the chunk. */
	SET_QUEUE_FOR_SIZE(head, q, i, elp->len);
	/* Find the correct slot in the size queue. */
	SH_TAILQ_FOREACH(elp_tmp, q, sizeq, __alloc_element)
	if(elp->len >= elp_tmp->len)
		break;
	if(elp_tmp == NULL)
		SH_TAILQ_INSERT_TAIL(q, elp, sizeq);
	else
		SH_TAILQ_INSERT_BEFORE(q, elp_tmp, elp, sizeq, __alloc_element);
}
/*
 * __env_region_extend --
 *	Extend a region.
 */
int __env_region_extend(ENV * env, REGINFO * infop)
{
	ALLOC_ELEMENT * elp;
	REGION * rp = infop->rp;
	int ret = 0;
	DB_ASSERT(env, !F_ISSET(env, ENV_PRIVATE));
	if(rp->size >= rp->max)
		return ENOMEM;
	elp = (ALLOC_ELEMENT *)((uint8 *)infop->addr+rp->size);
	if(rp->size+rp->alloc > rp->max)
		rp->alloc = rp->max-rp->size;
	rp->size += rp->alloc;
	rp->size = (size_t)ALIGNP_INC(rp->size, sizeof(size_t));
	if(infop->fhp && (ret = __db_file_extend(env, infop->fhp, rp->size)) != 0)
		return ret;
	elp->len = rp->alloc;
	elp->ulen = 0;
#ifdef DIAGNOSTIC
	*(uint8 *)(elp+1) = GUARD_BYTE;
#endif
	SH_TAILQ_INSERT_TAIL(&((ALLOC_LAYOUT *)infop->head)->addrq, elp, addrq);
	__env_alloc_free(infop, elp+1);
	if(rp->alloc < MEGABYTE)
		rp->alloc += rp->size;
	SETMIN(rp->alloc, MEGABYTE);
	return ret;
}
/*
 * __env_elem_size --
 *	Return the size of an allocated element.
 */
uintmax_t __env_elem_size(ENV * env, void * p)
{
	ALLOC_ELEMENT * elp;
	uintmax_t size;
	if(F_ISSET(env, ENV_PRIVATE)) {
		size = *((uintmax_t *)p-1);
		size -= sizeof(uintmax_t);
	}
	else {
		elp = (ALLOC_ELEMENT *)((uint8 *)p-sizeof(ALLOC_ELEMENT));
		size = elp->ulen;
	}
	return size;
}
/*
 * __env_get_chunk --
 *	Return the next chunk allocated in a private region.
 */
void * __env_get_chunk(REGINFO * infop, void ** nextp, uintmax_t * sizep)
{
	if(infop->mem == NULL)
		return NULL;
	else {
		if(*nextp == NULL)
			*nextp = infop->mem;
		REGION_MEM * mem = *(REGION_MEM **)nextp;
		*nextp = mem->next;
		*sizep = __env_elem_size(infop->env, mem);
		*sizep -= sizeof(*mem);
		return (void *)(mem+1);
	}
}

#ifdef HAVE_STATISTICS
/*
 * __env_alloc_print --
 *	Display the lists of memory chunks.
 *
 * PUBLIC: void __env_alloc_print __P((REGINFO *, uint32));
 */
void __env_alloc_print(REGINFO * infop, uint32 flags)
{
	ALLOC_ELEMENT * elp;
	uint i;
	ENV * env = infop->env;
	ALLOC_LAYOUT * head = (ALLOC_LAYOUT *)infop->head;
	if(F_ISSET(env, ENV_PRIVATE))
		return;
	__db_msg(env, "Region allocations: %lu allocations, %lu failures, %lu frees, %lu longest",
		(ulong)head->success, (ulong)head->failure, (ulong)head->freed, (ulong)head->longest);
	if(!LF_ISSET(DB_STAT_ALL))
		return;
	__db_msg(env, "%s", "Allocations by power-of-two sizes:");
	for(i = 0; i < DB_SIZE_Q_COUNT; ++i)
		__db_msg(env, "%3dKB\t%lu", (1024<<i)/1024, (ulong)head->pow2_size[i]);
	if(!LF_ISSET(DB_STAT_ALLOC))
		return;
	/*
	 * We don't normally display the list of address/chunk pairs, a few
	 * thousand lines of output is too voluminous for even DB_STAT_ALL.
	 */
	__db_msg(env, "Allocation list by address, offset: {chunk length, user length}");
	SH_TAILQ_FOREACH(elp, &head->addrq, addrq, __alloc_element)
	__db_msg(env, "\t%#lx, %lu {%lu, %lu}", P_TO_ULONG(elp), (ulong)R_OFFSET(infop, elp), (ulong)elp->len, (ulong)elp->ulen);
	__db_msg(env, "Allocation free list by size: KB {chunk length}");
	for(i = 0; i < DB_SIZE_Q_COUNT; ++i) {
		__db_msg(env, "%3dKB", (1024<<i)/1024);
		SH_TAILQ_FOREACH(elp, &head->sizeq[i], sizeq, __alloc_element)
		__db_msg(env, "\t%#lx {%lu}", P_TO_ULONG(elp), (ulong)elp->len);
	}
}
#endif
