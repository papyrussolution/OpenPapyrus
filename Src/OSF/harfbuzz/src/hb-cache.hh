/*
 * Copyright © 2012  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Google Author(s): Behdad Esfahbod
 */
#ifndef HB_CACHE_HH
#define HB_CACHE_HH

#include "hb.hh"

/* Implements a lock-free cache for int->int functions. */

template <uint key_bits, uint value_bits, uint cache_bits>
struct hb_cache_t {
	static_assert((key_bits >= cache_bits), "");
	static_assert((key_bits + value_bits - cache_bits <= 8 * sizeof(hb_atomic_int_t)), "");
	static_assert(sizeof(hb_atomic_int_t) == sizeof(uint), "");

	void init() {
		clear();
	}

	void fini() {
	}

	void clear()
	{
		for(uint i = 0; i < ARRAY_LENGTH(values); i++)
			values[i].set_relaxed(-1);
	}

	bool get(uint key, uint * value) const
	{
		uint k = key & ((1u<<cache_bits)-1);
		uint v = values[k].get_relaxed();
		if((key_bits + value_bits - cache_bits == 8 * sizeof(hb_atomic_int_t) && v == (uint)-1) ||
		    (v >> value_bits) != (key >> cache_bits))
			return false;
		*value = v & ((1u<<value_bits)-1);
		return true;
	}

	bool set(uint key, uint value)
	{
		if(UNLIKELY((key >> key_bits) || (value >> value_bits)))
			return false; /* Overflows */
		uint k = key & ((1u<<cache_bits)-1);
		uint v = ((key>>cache_bits)<<value_bits) | value;
		values[k].set_relaxed(v);
		return true;
	}

private:
	hb_atomic_int_t values[1u<<cache_bits];
};

typedef hb_cache_t<21, 16, 8> hb_cmap_cache_t;
typedef hb_cache_t<16, 24, 8> hb_advance_cache_t;

#endif /* HB_CACHE_HH */
