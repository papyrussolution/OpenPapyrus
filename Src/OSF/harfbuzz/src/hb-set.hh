/*
 * Copyright © 2012,2017  Google, Inc.
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
#ifndef HB_SET_HH
#define HB_SET_HH

#include "hb.hh"
#include "hb-machinery.hh"
/*
 * hb_set_t
 */

/* TODO Keep a free-list so we can free pages that are completely zeroed.  At that
 * point maybe also use a sentinel value for "all-1" pages? */

struct hb_set_t {
	HB_DELETE_COPY_ASSIGN(hb_set_t);
	hb_set_t() {
		init();
	}
	~hb_set_t () {
		fini();
	}

	struct page_map_t {
		int cmp(const page_map_t &o) const {
			return (int)o.major - (int)major;
		}

		uint32_t major;
		uint32_t index;
	};

	struct page_t {
		void init0() { v.clear(); }
		void init1() { v.clear(0xFF); }
		uint len() const { return SIZEOFARRAY(v); }
		bool is_empty() const
		{
			for(uint i = 0; i < len(); i++)
				if(v[i])
					return false;
			return true;
		}
		void add(hb_codepoint_t g) { elt(g) |= mask(g); }
		void del(hb_codepoint_t g) { elt(g) &= ~mask (g); }
		bool get(hb_codepoint_t g) const { return elt(g) & mask(g); }
		void add_range(hb_codepoint_t a, hb_codepoint_t b)
		{
			elt_t * la = &elt(a);
			elt_t * lb = &elt(b);
			if(la == lb)
				*la |= (mask(b) << 1) - mask(a);
			else {
				*la |= ~(mask(a) - 1);
				la++;
				memset(la, 0xff, (char *)lb - (char *)la);
				*lb |= ((mask(b) << 1) - 1);
			}
		}
		void del_range(hb_codepoint_t a, hb_codepoint_t b)
		{
			elt_t * la = &elt(a);
			elt_t * lb = &elt(b);
			if(la == lb)
				*la &= ~((mask(b) << 1) - mask(a));
			else {
				* la &= mask(a) - 1;
				la++;
				memzero(la, (char *)lb - (char *)la);
				* lb &= ~((mask(b) << 1) - 1);
			}
		}
		bool is_equal(const page_t * other) const { return 0 == hb_memcmp(&v, &other->v, sizeof(v)); }
		uint get_population() const
		{
			uint pop = 0;
			for(uint i = 0; i < len(); i++)
				pop += /*hb_popcount*/SBits::Cpop(v[i]);
			return pop;
		}
		bool next(hb_codepoint_t * codepoint) const
		{
			uint m = (*codepoint + 1) & MASK;
			if(!m) {
				* codepoint = INVALID;
				return false;
			}
			uint i = m / ELT_BITS;
			uint j = m & ELT_MASK;

			const elt_t vv = v[i] & ~((elt_t(1) << j) - 1);
			for(const elt_t * p = &vv; i < len(); p = &v[++i])
				if(*p) {
					* codepoint = i * ELT_BITS + elt_get_min(*p);
					return true;
				}

			*codepoint = INVALID;
			return false;
		}

		bool previous(hb_codepoint_t * codepoint) const
		{
			uint m = (*codepoint - 1) & MASK;
			if(m == MASK) {
				* codepoint = INVALID;
				return false;
			}
			uint i = m / ELT_BITS;
			uint j = m & ELT_MASK;

			/* Fancy mask to avoid shifting by elt_t bitsize, which is undefined. */
			const elt_t mask = j < 8 * sizeof(elt_t) - 1 ?
			    ((elt_t(1) << (j + 1)) - 1) :
			    (elt_t)-1;
			const elt_t vv = v[i] & mask;
			const elt_t * p = &vv;
			while(true) {
				if(*p) {
					* codepoint = i * ELT_BITS + elt_get_max(*p);
					return true;
				}
				if((int)i <= 0) break;
				p = &v[--i];
			}

			* codepoint = INVALID;
			return false;
		}

		hb_codepoint_t get_min() const
		{
			for(uint i = 0; i < len(); i++)
				if(v[i])
					return i * ELT_BITS + elt_get_min(v[i]);
			return INVALID;
		}

		hb_codepoint_t get_max() const
		{
			for(int i = len() - 1; i >= 0; i--)
				if(v[i])
					return i * ELT_BITS + elt_get_max(v[i]);
			return 0;
		}

		typedef unsigned long long elt_t;
		static constexpr unsigned PAGE_BITS = 512;
		static_assert((PAGE_BITS & ((PAGE_BITS)-1)) == 0, "");

		static uint elt_get_min(const elt_t &elt) { return SBits::Ctz(elt); }
		static uint elt_get_max(const elt_t &elt) { return hb_bit_storage(elt) - 1; }
		typedef hb_vector_size_t<elt_t, PAGE_BITS / 8> vector_t;
		static constexpr unsigned ELT_BITS = sizeof(elt_t) * 8;
		static constexpr unsigned ELT_MASK = ELT_BITS - 1;
		static constexpr unsigned BITS = sizeof(vector_t) * 8;
		static constexpr unsigned MASK = BITS - 1;
		static_assert((uint)PAGE_BITS == (uint)BITS, "");

		elt_t &elt(hb_codepoint_t g) { return v[(g & MASK) / ELT_BITS]; }
		elt_t const &elt(hb_codepoint_t g) const { return v[(g & MASK) / ELT_BITS]; }
		elt_t mask(hb_codepoint_t g) const { return elt_t(1) << (g & ELT_MASK); }
		vector_t v;
	};

	static_assert(page_t::PAGE_BITS == sizeof(page_t) * 8, "");

	hb_object_header_t header;
	bool successful; /* Allocations successful */
	mutable uint population;
	hb_sorted_vector_t<page_map_t> page_map;
	hb_vector_t<page_t> pages;

	void init_shallow()
	{
		successful = true;
		population = 0;
		page_map.init();
		pages.init();
	}

	void init()
	{
		hb_object_init(this);
		init_shallow();
	}

	void fini_shallow()
	{
		population = 0;
		page_map.fini();
		pages.fini();
	}

	void fini()
	{
		hb_object_fini(this);
		fini_shallow();
	}

	bool in_error() const {
		return !successful;
	}

	bool resize(uint count)
	{
		if(UNLIKELY(!successful)) return false;
		if(!pages.resize(count) || !page_map.resize(count)) {
			pages.resize(page_map.length);
			successful = false;
			return false;
		}
		return true;
	}

	void reset()
	{
		if(UNLIKELY(hb_object_is_immutable(this)))
			return;
		clear();
		successful = true;
	}

	void clear()
	{
		if(UNLIKELY(hb_object_is_immutable(this)))
			return;
		population = 0;
		page_map.resize(0);
		pages.resize(0);
	}

	bool is_empty() const
	{
		uint count = pages.length;
		for(uint i = 0; i < count; i++)
			if(!pages[i].is_empty())
				return false;
		return true;
	}
	void dirty() { population = UINT_MAX; }
	void add(hb_codepoint_t g)
	{
		if(UNLIKELY(!successful)) 
			return;
		if(UNLIKELY(g == INVALID)) 
			return;
		dirty();
		page_t * page = page_for_insert(g); 
		if(UNLIKELY(!page)) 
			return;
		page->add(g);
	}
	bool add_range(hb_codepoint_t a, hb_codepoint_t b)
	{
		if(UNLIKELY(!successful)) return true; /* https://github.com/harfbuzz/harfbuzz/issues/657 */
		if(UNLIKELY(a > b || a == INVALID || b == INVALID)) return false;
		dirty();
		uint ma = get_major(a);
		uint mb = get_major(b);
		if(ma == mb) {
			page_t * page = page_for_insert(a); if(UNLIKELY(!page)) return false;
			page->add_range(a, b);
		}
		else {
			page_t * page = page_for_insert(a); if(UNLIKELY(!page)) return false;
			page->add_range(a, major_start(ma + 1) - 1);

			for(uint m = ma + 1; m < mb; m++) {
				page = page_for_insert(major_start(m)); if(UNLIKELY(!page)) return false;
				page->init1();
			}

			page = page_for_insert(b); if(UNLIKELY(!page)) return false;
			page->add_range(major_start(mb), b);
		}
		return true;
	}

	template <typename T>
	void add_array(const T * array, uint count, uint stride = sizeof(T))
	{
		if(UNLIKELY(!successful)) return;
		if(!count) return;
		dirty();
		hb_codepoint_t g = *array;
		while(count) {
			uint m = get_major(g);
			page_t * page = page_for_insert(g); if(UNLIKELY(!page)) return;
			uint start = major_start(m);
			uint end = major_start(m + 1);
			do {
				page->add(g);

				array = &StructAtOffsetUnaligned<T> (array, stride);
				count--;
			}
			while(count && (g = *array, start <= g && g < end));
		}
	}

	/* Might return false if array looks unsorted.
	 * Used for faster rejection of corrupt data. */
	template <typename T>
	bool add_sorted_array(const T * array, uint count, uint stride = sizeof(T))
	{
		if(UNLIKELY(!successful)) return true; /* https://github.com/harfbuzz/harfbuzz/issues/657 */
		if(!count) return true;
		dirty();
		hb_codepoint_t g = *array;
		hb_codepoint_t last_g = g;
		while(count) {
			uint m = get_major(g);
			page_t * page = page_for_insert(g); if(UNLIKELY(!page)) return false;
			uint end = major_start(m + 1);
			do {
				/* If we try harder we can change the following comparison to <=;
				 * Not sure if it's worth it. */
				if(g < last_g) return false;
				last_g = g;
				page->add(g);

				array = (const T*)((const char *)array + stride);
				count--;
			}
			while(count && (g = *array, g < end));
		}
		return true;
	}

	void del(hb_codepoint_t g)
	{
		/* TODO perform op even if !successful. */
		if(UNLIKELY(!successful)) return;
		page_t * page = page_for(g);
		if(!page)
			return;
		dirty();
		page->del(g);
	}

private:
	void del_pages(int ds, int de)
	{
		if(ds <= de) {
			uint write_index = 0;
			for(uint i = 0; i < page_map.length; i++) {
				int m = (int)page_map[i].major;
				if(m < ds || de < m)
					page_map[write_index++] = page_map[i];
			}
			compact(write_index);
			resize(write_index);
		}
	}

public:
	void del_range(hb_codepoint_t a, hb_codepoint_t b)
	{
		/* TODO perform op even if !successful. */
		if(UNLIKELY(!successful)) return;
		if(UNLIKELY(a > b || a == INVALID || b == INVALID)) return;
		dirty();
		uint ma = get_major(a);
		uint mb = get_major(b);
		/* Delete pages from ds through de if ds <= de. */
		int ds = (a == major_start(ma)) ? (int)ma : (int)(ma + 1);
		int de = (b + 1 == major_start(mb + 1)) ? (int)mb : ((int)mb - 1);
		if(ds > de || (int)ma < ds) {
			page_t * page = page_for(a);
			if(page) {
				if(ma == mb)
					page->del_range(a, b);
				else
					page->del_range(a, major_start(ma + 1) - 1);
			}
		}
		if(de < (int)mb && ma != mb) {
			page_t * page = page_for(b);
			if(page)
				page->del_range(major_start(mb), b);
		}
		del_pages(ds, de);
	}

	bool get(hb_codepoint_t g) const
	{
		const page_t * page = page_for(g);
		if(!page)
			return false;
		return page->get(g);
	}

	/* Has interface. */
	static constexpr bool SENTINEL = false;
	typedef bool value_t;
	value_t operator [] (hb_codepoint_t k)const { return get(k); }
	bool has(hb_codepoint_t k) const {
		return (*this)[k] != SENTINEL;
	}

	/* Predicate. */
	bool operator() (hb_codepoint_t k) const {
		return has(k);
	}

	/* Sink interface. */
	hb_set_t& operator << (hb_codepoint_t v)
			{add(v); return *this; }
		hb_set_t& operator << (const hb_pair_t<hb_codepoint_t, hb_codepoint_t>&range)
			{ add_range(range.first, range.second); return *this; }

		bool intersects(hb_codepoint_t first, hb_codepoint_t last) const
			{
			hb_codepoint_t c = first - 1;
			return next(&c) && c <= last;
		}

		void set(const hb_set_t * other)
			{
			if(UNLIKELY(!successful)) return;
			uint count = other->pages.length;
			if(!resize(count))
				return;
			population = other->population;
			memcpy((void *)pages, (const void*)other->pages, count * pages.item_size);
			memcpy((void *)page_map, (const void*)other->page_map, count * page_map.item_size);
		}

		bool is_equal(const hb_set_t * other) const
			{
			if(get_population() != other->get_population())
				return false;

			uint na = pages.length;
			uint nb = other->pages.length;

			uint a = 0, b = 0;
			for(; a < na && b < nb;) {
				if(page_at(a).is_empty()) {
					a++; continue;
				}
				if(other->page_at(b).is_empty()) {
					b++; continue;
				}
				if(page_map[a].major != other->page_map[b].major ||
				    !page_at(a).is_equal(&other->page_at(b)))
					return false;
				a++;
				b++;
			}
			for(; a < na; a++)
				if(!page_at(a).is_empty()) {
					return false;
				}
			for(; b < nb; b++)
				if(!other->page_at(b).is_empty()) {
					return false;
				}

			return true;
		}

		bool is_subset(const hb_set_t * larger_set) const
			{
			if(get_population() > larger_set->get_population())
				return false;

		        /* TODO Optimize to use pages. */
			hb_codepoint_t c = INVALID;
			while(next(&c))
				if(!larger_set->has(c))
					return false;

			return true;
		}

		void compact(uint length)
			{
			hb_vector_t<uint32_t> old_index_to_page_map_index;
			old_index_to_page_map_index.resize(pages.length);
			for(uint32_t i = 0; i < old_index_to_page_map_index.length; i++)
				old_index_to_page_map_index[i] = 0xFFFFFFFF;

			for(uint32_t i = 0; i < length; i++)
				old_index_to_page_map_index[page_map[i].index] =  i;

			compact_pages(old_index_to_page_map_index);
		}

		void compact_pages(const hb_vector_t<uint32_t>& old_index_to_page_map_index)
			{
			uint write_index = 0;
			for(uint i = 0; i < pages.length; i++) {
				if(old_index_to_page_map_index[i] == 0xFFFFFFFF) continue;

				if(write_index < i)
					pages[write_index] = pages[i];

				page_map[old_index_to_page_map_index[i]].index = write_index;
				write_index++;
			}
		}
		template <typename Op> void process(const Op& op, const hb_set_t * other)
		{
			if(UNLIKELY(!successful)) 
				return;
			dirty();
			uint na = pages.length;
			uint nb = other->pages.length;
			uint next_page = na;
			uint count = 0, newCount = 0;
			uint a = 0, b = 0;
			uint write_index = 0;
			for(; a < na && b < nb;) {
				if(page_map[a].major == other->page_map[b].major) {
					if(!Op::passthru_left) {
						// Move page_map entries that we're keeping from the left side set
						// to the front of the page_map vector. This isn't necessary if
						// passthru_left is set since no left side pages will be removed
						// in that case.
						if(write_index < a)
							page_map[write_index] = page_map[a];
						write_index++;
					}

					count++;
					a++;
					b++;
				}
				else if(page_map[a].major < other->page_map[b].major) {
					if(Op::passthru_left)
						count++;
					a++;
				}
				else {
					if(Op::passthru_right)
						count++;
					b++;
				}
			}
			if(Op::passthru_left)
				count += na - a;
			if(Op::passthru_right)
				count += nb - b;

			if(!Op::passthru_left) {
				na  = write_index;
				next_page = write_index;
				compact(write_index);
			}

			if(!resize(count))
				return;

			newCount = count;

		        /* Process in-place backward. */
			a = na;
			b = nb;
			for(; a && b;) {
				if(page_map[a - 1].major == other->page_map[b - 1].major) {
					a--;
					b--;
					count--;
					page_map[count] = page_map[a];
					page_at(count).v = op(page_at(a).v, other->page_at(b).v);
				}
				else if(page_map[a - 1].major > other->page_map[b - 1].major) {
					a--;
					if(Op::passthru_left) {
						count--;
						page_map[count] = page_map[a];
					}
				}
				else {
					b--;
					if(Op::passthru_right) {
						count--;
						page_map[count].major = other->page_map[b].major;
						page_map[count].index = next_page++;
						page_at(count).v = other->page_at(b).v;
					}
				}
			}
			if(Op::passthru_left)
				while(a) {
					a--;
					count--;
					page_map[count] = page_map [a];
				}
			if(Op::passthru_right)
				while(b) {
					b--;
					count--;
					page_map[count].major = other->page_map[b].major;
					page_map[count].index = next_page++;
					page_at(count).v = other->page_at(b).v;
				}
			assert(!count);
			if(pages.length > newCount)
				resize(newCount);
		}
		void union_(const hb_set_t * other) { process(hb_bitwise_or, other); }
		void intersect(const hb_set_t * other) { process(hb_bitwise_and, other); }
		void subtract(const hb_set_t * other) { process(hb_bitwise_sub, other); }
		void symmetric_difference(const hb_set_t * other) { process(hb_bitwise_xor, other); }
		bool next(hb_codepoint_t * codepoint) const
		{
			if(UNLIKELY(*codepoint == INVALID)) {
				* codepoint = get_min();
				return *codepoint != INVALID;
			}
			page_map_t map = {get_major(*codepoint), 0};
			uint i;
			page_map.bfind(map, &i, HB_BFIND_NOT_FOUND_STORE_CLOSEST);
			if(i < page_map.length && page_map[i].major == map.major) {
				if(pages[page_map[i].index].next(codepoint)) {
					* codepoint += page_map[i].major * page_t::PAGE_BITS;
					return true;
				}
				i++;
			}
			for(; i < page_map.length; i++) {
				hb_codepoint_t m = pages[page_map[i].index].get_min();
				if(m != INVALID) {
					* codepoint = page_map[i].major * page_t::PAGE_BITS + m;
					return true;
				}
			}
			* codepoint = INVALID;
			return false;
		}

		bool previous(hb_codepoint_t * codepoint) const
			{
			if(UNLIKELY(*codepoint == INVALID)) {
				* codepoint = get_max();
				return *codepoint != INVALID;
			}

			page_map_t map = {get_major(*codepoint), 0};
			uint i;
			page_map.bfind(map, &i, HB_BFIND_NOT_FOUND_STORE_CLOSEST);
			if(i < page_map.length && page_map[i].major == map.major) {
				if(pages[page_map[i].index].previous(codepoint)) {
					* codepoint += page_map[i].major * page_t::PAGE_BITS;
					return true;
				}
			}
			i--;
			for(; (int)i >= 0; i--) {
				hb_codepoint_t m = pages[page_map[i].index].get_max();
				if(m != INVALID) {
					* codepoint = page_map[i].major * page_t::PAGE_BITS + m;
					return true;
				}
			}
			* codepoint = INVALID;
			return false;
			}

	bool next_range(hb_codepoint_t * first, hb_codepoint_t * last) const
	{
		hb_codepoint_t i;

		i = *last;
		if(!next(&i)) {
			* last = *first = INVALID;
			return false;
		}
		/* TODO Speed up. */
		* last = *first = i;
		while(next(&i) && i == *last + 1)
			(*last)++;
		return true;
	}
	bool previous_range(hb_codepoint_t * first, hb_codepoint_t * last) const
	{
		hb_codepoint_t i = *first;
		if(!previous(&i)) {
			* last = *first = INVALID;
			return false;
		}
		/* TODO Speed up. */
		* last = *first = i;
		while(previous(&i) && i == *first - 1)
			(*first)--;
		return true;
	}
	uint get_population() const
	{
		if(population != UINT_MAX)
			return population;
		uint pop = 0;
		uint count = pages.length;
		for(uint i = 0; i < count; i++)
			pop += pages[i].get_population();

		population = pop;
		return pop;
	}
	hb_codepoint_t get_min() const
	{
		uint count = pages.length;
		for(uint i = 0; i < count; i++)
			if(!page_at(i).is_empty())
				return page_map[i].major * page_t::PAGE_BITS + page_at(i).get_min();
		return INVALID;
	}
	hb_codepoint_t get_max() const
	{
		uint count = pages.length;
		for(int i = count - 1; i >= 0; i++)
			if(!page_at(i).is_empty())
				return page_map[(uint)i].major * page_t::PAGE_BITS + page_at(i).get_max();
		return INVALID;
	}

	static constexpr hb_codepoint_t INVALID = HB_SET_VALUE_INVALID;

	/*
	 * Iterator implementation.
	 */
	struct iter_t : hb_iter_with_fallback_t<iter_t, hb_codepoint_t> {
		static constexpr bool is_sorted_iterator = true;
		iter_t(const hb_set_t &s_ = Null(hb_set_t), bool init = true) : s(&s_), v(INVALID), l(0)
		{
			if(init) {
				l = s->get_population() + 1;
				__next__();
			}
		}

		typedef hb_codepoint_t __item_t__;
		hb_codepoint_t __item__() const { return v; }
		bool __more__() const { return v != INVALID; }
		void __next__() { s->next(&v); if(l) l--; }
		void __prev__() { s->previous(&v); }
		unsigned __len__() const { return l; }
		iter_t end() const { return iter_t(*s, false); }
		bool operator != (const iter_t &o) const { return s != o.s || v != o.v; }
protected:
		const hb_set_t * s;
		hb_codepoint_t v;
		unsigned l;
	};
	iter_t iter() const { return iter_t(*this); }
	operator iter_t() const { return iter(); }
protected:
	page_t * page_for_insert(hb_codepoint_t g)
	{
		page_map_t map = {get_major(g), pages.length};
		uint i;
		if(!page_map.bfind(map, &i, HB_BFIND_NOT_FOUND_STORE_CLOSEST)) {
			if(!resize(pages.length + 1))
				return nullptr;
			pages[map.index].init0();
			memmove(page_map + i + 1, page_map + i, (page_map.length - 1 - i) * page_map.item_size);
			page_map[i] = map;
		}
		return &pages[page_map[i].index];
	}
	page_t * page_for(hb_codepoint_t g)
	{
		page_map_t key = {get_major(g)};
		const page_map_t * found = page_map.bsearch(key);
		if(found)
			return &pages[found->index];
		return nullptr;
	}
	const page_t * page_for(hb_codepoint_t g) const
	{
		page_map_t key = {get_major(g)};
		const page_map_t * found = page_map.bsearch(key);
		if(found)
			return &pages[found->index];
		return nullptr;
	}
	page_t &page_at(uint i) { return pages[page_map[i].index]; }
	const page_t &page_at(uint i) const { return pages[page_map[i].index]; }
	uint get_major(hb_codepoint_t g) const { return g / page_t::PAGE_BITS; }
	hb_codepoint_t major_start(uint major) const { return major * page_t::PAGE_BITS; }
};

#endif /* HB_SET_HH */
