/*
 * Copyright © 2017,2018  Google, Inc.
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
#ifndef HB_VECTOR_HH
#define HB_VECTOR_HH

#include "hb.hh"
#include "hb-array.hh"
#include "hb-null.hh"

template <typename Type> struct hb_vector_t {
	typedef Type item_t;
	static constexpr unsigned item_size = hb_static_size(Type);

	hb_vector_t() 
	{
		init();
	}
	hb_vector_t(const hb_vector_t &o)
	{
		init();
		alloc(o.length);
		hb_copy(o, *this);
	}
	hb_vector_t(hb_vector_t &&o)
	{
		allocated = o.allocated;
		length = o.length;
		arrayZ = o.arrayZ;
		o.init();
	}
	~hb_vector_t () 
	{
		fini();
	}
private:
	int allocated; /* == -1 means allocation failed. */
public:
	uint length;
public:
	Type * arrayZ;
	void init()
	{
		allocated = length = 0;
		arrayZ = nullptr;
	}
	void fini()
	{
		SAlloc::F(arrayZ);
		init();
	}
	void fini_deep()
	{
		uint count = length;
		for(uint i = 0; i < count; i++)
			arrayZ[i].fini();
		fini();
	}
	void reset() 
	{
		resize(0);
	}
	hb_vector_t& operator = (const hb_vector_t &o)
	{
		reset();
		alloc(o.length);
		hb_copy(o, *this);
		return *this;
	}
	hb_vector_t& operator = (hb_vector_t &&o)
	{
		fini();
		allocated = o.allocated;
		length = o.length;
		arrayZ = o.arrayZ;
		o.init();
		return *this;
	}
	hb_bytes_t as_bytes() const { return hb_bytes_t((const char*)arrayZ, length * item_size); }
	bool operator == (const hb_vector_t &o) const { return as_array() == o.as_array(); }
	bool operator != (const hb_vector_t &o) const { return !(*this == o); }
	uint32_t hash() const { return as_array().hash(); }
	Type& operator [] (int i_)
	{
		uint i = (uint)i_;
		if(UNLIKELY(i >= length))
			return Crap(Type);
		return arrayZ[i];
	}
	const Type& operator [] (int i_)const
	{
		uint i = (uint)i_;
		if(UNLIKELY(i >= length))
			return Null(Type);
		return arrayZ[i];
	}

	Type& tail() { return (*this)[length - 1]; }
	const Type& tail() const { return (*this)[length - 1]; }
	explicit operator bool() const { return length; }
	unsigned get_size() const { return length * item_size; }
	/* Sink interface. */
	template <typename T> hb_vector_t& operator << (T&& v) { push(hb_forward<T> (v)); return *this; }
	hb_array_t <Type> as_array() { return hb_array(arrayZ, length); }
	hb_array_t<const Type> as_array() const { return hb_array(arrayZ, length); }
	/* Iterator. */
	typedef hb_array_t <const Type> iter_t;
	typedef hb_array_t <Type> writer_t;
	iter_t   iter() const { return as_array(); }
	writer_t writer() { return as_array(); }
	operator   iter_t() const { return iter(); }
	operator writer_t() { return writer(); }
	hb_array_t<const Type> sub_array(uint start_offset, uint count) const { return as_array().sub_array(start_offset, count); }
	hb_array_t<const Type> sub_array(uint start_offset, uint * count = nullptr /* IN/OUT */) const { return as_array().sub_array(start_offset, count); }
	hb_array_t<Type> sub_array(uint start_offset, uint count) { return as_array().sub_array(start_offset, count); }
	hb_array_t<Type> sub_array(uint start_offset, uint * count = nullptr /* IN/OUT */) { return as_array().sub_array(start_offset, count); }
	hb_sorted_array_t<Type> as_sorted_array() { return hb_sorted_array(arrayZ, length); }
	hb_sorted_array_t<const Type> as_sorted_array() const { return hb_sorted_array(arrayZ, length); }
	template <typename T> explicit operator T * () { return arrayZ; }
	template <typename T> explicit operator const T * () const { return arrayZ; }
	Type * operator  + (uint i) {return arrayZ + i; }
	const Type * operator  + (uint i)const { return arrayZ + i; }
	Type * push()
	{
		if(UNLIKELY(!resize(length + 1)))
			return &Crap(Type);
		return &arrayZ[length - 1];
	}
	template <typename T> Type * push(T&& v)
	{
		Type * p = push();
		* p = hb_forward<T> (v);
		return p;
	}
	bool in_error() const { return allocated < 0; }
	// Allocate for size but don't adjust length
	bool alloc(uint size)
	{
		if(UNLIKELY(allocated < 0))
			return false;
		if(LIKELY(size <= (uint)allocated))
			return true;
		/* Reallocate */
		uint new_allocated = allocated;
		while(size >= new_allocated)
			new_allocated += (new_allocated >> 1) + 8;
		Type * new_array = nullptr;
		bool overflows = (int)new_allocated < 0 || (new_allocated < (uint)allocated) || hb_unsigned_mul_overflows(new_allocated, sizeof(Type));
		if(LIKELY(!overflows))
			new_array = (Type*)SAlloc::R(arrayZ, new_allocated * sizeof(Type));
		if(UNLIKELY(!new_array)) {
			allocated = -1;
			return false;
		}
		arrayZ = new_array;
		allocated = new_allocated;
		return true;
	}
	bool resize(int size_)
	{
		uint size = size_ < 0 ? 0u : (uint)size_;
		if(!alloc(size))
			return false;
		if(size > length)
			memzero(arrayZ + length, (size - length) * sizeof(*arrayZ));
		length = size;
		return true;
	}
	Type pop()
	{
		if(!length) 
			return Null(Type);
		return hb_move(arrayZ[--length]); /* Does this move actually work? */
	}
	void remove(uint i)
	{
		if(UNLIKELY(i >= length))
			return;
		memmove(static_cast<void *> (&arrayZ[i]), static_cast<void *> (&arrayZ[i+1]), (length - i - 1) * sizeof(Type));
		length--;
	}
	void shrink(int size_)
	{
		uint size = size_ < 0 ? 0u : (uint)size_;
		if(size < length)
			length = size;
	}
	template <typename T> Type * find(T v)
	{
		for(uint i = 0; i < length; i++)
			if(arrayZ[i] == v)
				return &arrayZ[i];
		return nullptr;
	}
	template <typename T> const Type * find(T v) const
	{
		for(uint i = 0; i < length; i++)
			if(arrayZ[i] == v)
				return &arrayZ[i];
		return nullptr;
	}
	void qsort(int (*cmp)(const void*, const void*))
	{
		as_array().qsort(cmp);
	}
	void qsort(uint start = 0, uint end = (uint)-1)
	{
		as_array().qsort(start, end);
	}
	template <typename T> Type * lsearch(const T &x, Type * not_found = nullptr)
	{
		return as_array().lsearch(x, not_found);
	}
	template <typename T> const Type * lsearch(const T &x, const Type * not_found = nullptr) const
	{
		return as_array().lsearch(x, not_found);
	}
	template <typename T> bool lfind(const T &x, unsigned * pos = nullptr) const
	{
		return as_array().lfind(x, pos);
	}
};

template <typename Type> struct hb_sorted_vector_t : hb_vector_t<Type> {
	hb_sorted_array_t<Type> as_array() 
	{
		return hb_sorted_array(this->arrayZ, this->length);
	}
	hb_sorted_array_t<const Type> as_array() const { return hb_sorted_array(this->arrayZ, this->length); }

	/* Iterator. */
	typedef hb_sorted_array_t<const Type> const_iter_t;
	typedef hb_sorted_array_t<Type>       iter_t;
	const_iter_t  iter() const {
		return as_array();
	}

	const_iter_t citer() const {
		return as_array();
	}

	iter_t  iter() {
		return as_array();
	}

	operator       iter_t() {
		return iter();
	}

	operator const_iter_t() const {
		return iter();
	}

	template <typename T>
	Type * bsearch(const T &x, Type * not_found = nullptr)
	{
		return as_array().bsearch(x, not_found);
	}

	template <typename T>
	const Type * bsearch(const T &x, const Type * not_found = nullptr) const
	{
		return as_array().bsearch(x, not_found);
	}

	template <typename T>
	bool bfind(const T &x, uint * i = nullptr,
	    hb_bfind_not_found_t not_found = HB_BFIND_NOT_FOUND_DONT_STORE,
	    uint to_store = (uint)-1) const
	{
		return as_array().bfind(x, i, not_found, to_store);
	}
};

#endif /* HB_VECTOR_HH */
