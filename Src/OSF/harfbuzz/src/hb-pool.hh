/*
 * Copyright © 2019  Facebook, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Facebook Author(s): Behdad Esfahbod
 */
#ifndef HB_POOL_HH
#define HB_POOL_HH

#include "hb.hh"

/* Memory pool for persistent allocation of small objects. */

template <typename T, unsigned ChunkLen = 16> struct hb_pool_t {
	hb_pool_t() : next(nullptr) 
	{
	}
	~hb_pool_t () 
	{
		fini();
	}
	void fini()
	{
		next = nullptr;
		for(chunk_t * _ : chunks) 
			SAlloc::F(_);
		chunks.fini();
	}
	T* alloc()
	{
		if(UNLIKELY(!next)) {
			if(UNLIKELY(!chunks.alloc(chunks.length + 1))) 
				return nullptr;
			chunk_t * chunk = (chunk_t*)SAlloc::C(1, sizeof(chunk_t));
			if(UNLIKELY(!chunk)) 
				return nullptr;
			chunks.push(chunk);
			next = chunk->thread();
		}
		T* obj = next;
		next = *((T**)next);
		memzero(obj, sizeof(T));
		return obj;
	}
	void free(T* obj)
	{
		*(T**)obj = next;
		next = obj;
	}
private:
	static_assert(ChunkLen > 1, "");
	static_assert(sizeof(T) >= sizeof(void *), "");
	static_assert(alignof(T) % alignof(void *) == 0, "");

	struct chunk_t {
		T* thread()
		{
			for(uint i = 0; i < ARRAY_LENGTH(arrayZ) - 1; i++)
				*(T**)&arrayZ[i] = &arrayZ[i+1];

			*(T**)&arrayZ[ARRAY_LENGTH(arrayZ) - 1] = nullptr;

			return arrayZ;
		}

		T arrayZ[ChunkLen];
	};

	T* next;
	hb_vector_t<chunk_t *> chunks;
};

#endif /* HB_POOL_HH */
