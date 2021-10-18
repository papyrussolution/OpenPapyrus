// HB-TEST.CPP
// 
#include "harfbuzz-internal.h"
#pragma hdrstop

int HarfBuzzTestAlgs(); // ###
int HarfBuzzTestArray(); // ###
int HarfBuzzTestBitmap(); // ###
int HarfBuzzTestIter(); // ###
int HarfBuzzTestNumber(); // ###
int HarfBuzzTestUnicodeRanges(); // ###
int HarfBuzzTestMeta(); // ###
int HarfBuzzTestCommon(const char * pFileName); // ###
int HarfBuzzTestBufferSerialize(const char * pFileName); // ###
int HarfBuzzTestGPosSizeParams(const char * pFileName); // ###
int HarfBuzzTestOtGlyphName(const char * pFileName); // ###
int HarfBuzzTestOtMeta(const char * pFileName); // ###

static char * test_func(int a, char ** b)
{
	return b ? b[a] : nullptr;
}

int HarfBuzzTestAlgs() // ###
{
	struct A {
		void a() 
		{
		}
	};
	int    ok = 1;
	int    i = 1;
	auto   p = hb_pair(1, i);
	p.second = 2;
	assert(i == 2);
	const int c = 3;
	auto pc = hb_pair(1, c);
	assert(pc.second == 3);
	auto q = p;
	assert(&q != &p);
	q.second = 4;
	assert(i == 4);
	hb_invoke(test_func, 0, nullptr);
	A a;
	hb_invoke(&A::a, a);
	assert(1 == hb_min(8, 1));
	assert(8 == hb_max(8, 1));
	int x = 1, y = 2;
	hb_min(x, 3);
	hb_min(3, x);
	hb_min(x, 4 + 3);
	int &z = hb_min(x, y);
	z = 3;
	assert(x == 3);
	hb_pair_t<const int*, int> xp = hb_pair_t<int *, long> (nullptr, 0);
	xp = hb_pair_t<int *, double> (nullptr, 1);
	xp = hb_pair_t<const int*, int> (nullptr, 1);
	assert(3 == hb_partial(hb_min, 3) (4));
	assert(3 == hb_partial<1> (hb_min, 4) (3));
	auto M0 = hb_partial<2> (hb_max, 0);
	assert(M0(-2) == 0);
	assert(M0(+2) == 2);
	assert(hb_add(2) (5) == 7);
	assert(hb_add(5) (2) == 7);
	x = 1;
	assert(++hb_inc(x) == 3);
	assert(x == 3);
	return ok;
}
//
//
//
static void test_reverse()
{
	int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	hb_array_t<int> a(values, 9);
	a.reverse();
	int expected_values[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
	hb_array_t<int> expected(expected_values, 9);
	assert(a == expected);
}

static void test_reverse_range()
{
	int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	hb_array_t<int> a(values, 9);
	a.reverse(2, 6);
	int expected_values[] = {1, 2, 6, 5, 4, 3, 7, 8, 9};
	hb_array_t<int> expected(expected_values, 9);
	assert(a == expected);
}

static void test_reverse_invalid()
{
	int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	hb_array_t<int> a(values, 9);
	a.reverse(4, 3);
	a.reverse(2, 3);
	a.reverse(5, 5);
	a.reverse(12, 15);
	int expected_values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	hb_array_t<int> expected(expected_values, 9);
	assert(a == expected);
}

int HarfBuzzTestArray() // ###
{
	int    ok = 1;
	test_reverse();
	test_reverse_range();
	test_reverse_invalid();
	return ok;
}
//
//
//
int HarfBuzzTestBitmap() // ###
{
	int    ok = 1;
	hb_bimap_t bm;
	assert(bm.is_empty() == true);
	bm.set(1, 4);
	bm.set(2, 5);
	bm.set(3, 6);
	assert(bm.get_population() == 3);
	assert(bm.has(1) == true);
	assert(bm.has(4) == false);
	assert(bm[2] == 5);
	assert(bm.backward(6) == 3);
	bm.del(1);
	assert(bm.has(1) == false);
	assert(bm.has(3) == true);
	bm.clear();
	assert(bm.get_population() == 0);
	hb_inc_bimap_t ibm;
	assert(ibm.add(13) == 0);
	assert(ibm.add(8) == 1);
	assert(ibm.add(10) == 2);
	assert(ibm.add(8) == 1);
	assert(ibm.add(7) == 3);
	assert(ibm.get_population() == 4);
	assert(ibm[7] == 3);
	ibm.sort();
	assert(ibm.get_population() == 4);
	assert(ibm[7] == 0);
	assert(ibm[13] == 3);
	ibm.identity(3);
	assert(ibm.get_population() == 3);
	assert(ibm[0] == 0);
	assert(ibm[1] == 1);
	assert(ibm[2] == 2);
	assert(ibm.backward(0) == 0);
	assert(ibm.backward(1) == 1);
	assert(ibm.backward(2) == 2);
	assert(ibm.has(4) == false);
	return ok;
}
//
//
//
template <typename T> struct array_iter_t : hb_iter_with_fallback_t<array_iter_t<T>, T&>{
	array_iter_t (hb_array_t<T> arr_) : arr(arr_) 
	{
	}
	typedef T& __item_t__;
	static constexpr bool is_random_access_iterator = true;
	T& __item_at__(unsigned i) const { return arr[i]; }
	void __forward__(unsigned n) { arr += n; }
	void __rewind__(unsigned n) { arr -= n; }
	unsigned __len__() const { return arr.length; }
	bool operator !=(const array_iter_t& o) { return arr != o.arr; }
private:
	hb_array_t<T> arr;
};

template <typename T> struct some_array_t {
	some_array_t (hb_array_t<T> arr_) : arr(arr_) 
	{
	}
	typedef array_iter_t<T> iter_t;
	array_iter_t<T> iter() { return array_iter_t<T> (arr); }
	operator array_iter_t<T> () { return iter(); }
	operator hb_iter_t<array_iter_t<T> > () { return iter(); }
private:
	hb_array_t<T> arr;
};

template <typename Iter, hb_requires(hb_is_iterator(Iter))> static void test_iterator_non_default_constructable(Iter it)
{
	/* Iterate over a copy of it. */
	for(auto c = it.iter(); c; c++)
		*c;
	/* Same. */
	for(auto c = +it; c; c++)
		*c;
	/* Range-based for over a copy. */
	for(auto _ : +it)
		(void)_;
	it += it.len();
	it = it + 10;
	it = 10 + it;
	assert(*it == it[0]);
	static_assert(true || it.is_random_access_iterator, "");
	static_assert(true || it.is_sorted_iterator, "");
}

template <typename Iter, hb_requires(hb_is_iterator(Iter))> static void test_iterator(Iter it)
{
	Iter default_constructed;
	assert(!default_constructed);
	test_iterator_non_default_constructable(it);
}

template <typename Iterable, hb_requires(hb_is_iterable(Iterable))> static void test_iterable(const Iterable &lst = Null (Iterable))
{
	for(auto _ : lst)
		(void)_;
	// Test that can take iterator from.
	test_iterator(lst.iter());
}

int HarfBuzzTestIter() // ###
{
	int    ok = 1;
	const int src[10] = {};
	int    dst[20];
	hb_vector_t <int> v;
	array_iter_t <const int> s(src); /* Implicit conversion from static array. */
	array_iter_t <const int> s2(v); /* Implicit conversion from vector. */
	array_iter_t <int> t(dst);
	static_assert(array_iter_t<int>::is_random_access_iterator, "");
	some_array_t <const int> a(src);
	s2 = s;
	hb_iter(src);
	hb_iter(src, 2);

	hb_fill(t, 42);
	hb_copy(s, t);
	hb_copy(a.iter(), t);

	test_iterable(v);
	hb_set_t st;
	st << 1 << 15 << 43;
	test_iterable(st);
	hb_sorted_array_t<int> sa;
	(void)static_cast<hb_iter_t<hb_sorted_array_t<int>, hb_sorted_array_t<int>::item_t>&> (sa);
	(void)static_cast<hb_iter_t<hb_sorted_array_t<int>, hb_sorted_array_t<int>::__item_t__>&> (sa);
	(void)static_cast<hb_iter_t<hb_sorted_array_t<int>, int&>&>(sa);
	(void)static_cast<hb_iter_t<hb_sorted_array_t<int> >&>(sa);
	(void)static_cast<hb_iter_t<hb_array_t<int>, int&>&> (sa);
	test_iterable(sa);

	test_iterable<hb_array_t<int> > ();
	test_iterable<hb_sorted_array_t<const int> > ();
	test_iterable<hb_vector_t<float> > ();
	test_iterable<hb_set_t> ();
	test_iterable<OT::Coverage> ();

	test_iterator(hb_zip(st, v));
	test_iterator_non_default_constructable(hb_enumerate(st));
	test_iterator_non_default_constructable(hb_enumerate(st, -5));
	test_iterator_non_default_constructable(hb_enumerate(hb_iter(st)));
	test_iterator_non_default_constructable(hb_enumerate(hb_iter(st) + 1));
	test_iterator_non_default_constructable(hb_iter(st) | hb_filter());
	test_iterator_non_default_constructable(hb_iter(st) | hb_map(hb_lidentity));

	assert(true == hb_all(st));
	assert(false == hb_all(st, 42u));
	assert(true == hb_any(st));
	assert(false == hb_any(st, 14u));
	assert(true == hb_any(st, 14u, [] (unsigned _) { return _ - 1u; }));
	assert(true == hb_any(st, [] (unsigned _) { return _ == 15u; }));
	assert(true == hb_any(st, 15u));
	assert(false == hb_none(st));
	assert(false == hb_none(st, 15u));
	assert(true == hb_none(st, 17u));

	hb_array_t<hb_vector_t<int> > pa;
	pa->as_array();

	hb_map_t m;

	hb_iter(st);
	hb_iter(&st);

	+hb_iter(src)
	| hb_map(m)
	| hb_map(&m)
	| hb_filter()
	| hb_filter(st)
	| hb_filter(&st)
	| hb_filter(hb_bool)
	| hb_filter(hb_bool, hb_identity)
	| hb_sink(st)
	;

	+hb_iter(src)
	| hb_sink(hb_array(dst))
	;

	+hb_iter(src)
	| hb_apply(&st)
	;

	+hb_iter(src)
	| hb_map([] (int i) {
		return 1;
	})
	| hb_reduce([ = ] (int acc, int value) {
		return acc;
	}, 2)
	;

	using map_pair_t = hb_item_type<hb_map_t>;
	+hb_iter(m)
	| hb_map([] (map_pair_t p) {
		return p.first * p.second;
	})
	;

	m.keys();
	using map_key_t = decltype(*m.keys());
	+hb_iter(m.keys())
	| hb_filter([] (map_key_t k) {
		return k < 42;
	})
	| hb_drain
	;

	m.values();
	using map_value_t = decltype(*m.values());
	+hb_iter(m.values())
	| hb_filter([] (map_value_t k) {
		return k < 42;
	})
	| hb_drain
	;

	unsigned int temp1 = 10;
	unsigned int temp2 = 0;
	hb_map_t * result =
	    +hb_iter(src)
	    | hb_map([&] (int i) -> hb_set_t *
	{
		hb_set_t * set = hb_set_create();
		for(uint i = 0; i < temp1; ++i)
			hb_set_add(set, i);
		temp1++;
		return set;
	})
	    | hb_reduce([&] (hb_map_t * acc, hb_set_t * value) -> hb_map_t *
	{
		hb_map_set(acc, temp2++, hb_set_get_population(value));
		/* This is not a memory managed language, take care! */
		hb_set_destroy(value);
		return acc;
	}, hb_map_create())
	;
	/* The result should be something like 0->10, 1->11, ..., 9->19 */
	assert(hb_map_get(result, 9) == 19);
	unsigned int temp3 = 0;
	+hb_iter(src)
	| hb_map([&] (int i) {
		return ++temp3;
	})
	| hb_reduce([&] (float acc, int value) {
		return acc + value;
	}, 0)
	;
	hb_map_destroy(result);

	+hb_iter(src)
	| hb_drain
	;

	t << 1;
	long vl;
	s >> vl;

	hb_iota();
	hb_iota(3);
	hb_iota(3, 2);
	assert((&vl) + 1 == *++hb_iota(&vl, hb_inc));
	hb_range();
	hb_repeat(7u);
	hb_repeat(nullptr);
	hb_repeat(vl) | hb_chop(3);
	assert(hb_len(hb_range(10) | hb_take(3)) == 3);
	assert(hb_range(9).len() == 9);
	assert(hb_range(2, 9).len() == 7);
	assert(hb_range(2, 9, 3).len() == 3);
	assert(hb_range(2, 8, 3).len() == 2);
	assert(hb_range(2, 7, 3).len() == 2);
	assert(hb_range(-2, -9, -3).len() == 3);
	assert(hb_range(-2, -8, -3).len() == 2);
	assert(hb_range(-2, -7, -3).len() == 2);
	return ok;
}
//
//
//
int HarfBuzzTestNumber() // ###
{
	int    ok = 1;
	{
		const char str[] = "123";
		const char * pp = str;
		const char * end = str + 3;

		int pv;
		assert(hb_parse_int(&pp, end, &pv));
		assert(pv == 123);
		assert(pp - str == 3);
		assert(end - pp == 0);
		assert(!*end);
	}
	{
		const char str[] = "123";
		const char * pp = str;
		const char * end = str + strlen(str);

		unsigned int pv;
		assert(hb_parse_uint(&pp, end, &pv));
		assert(pv == 123);
		assert(pp - str == 3);
		assert(end - pp == 0);
		assert(!*end);
	}

	{
		const char str[] = "12F";
		const char * pp = str;
		const char * end = str + 3;

		unsigned int pv;
		assert(hb_parse_uint(&pp, end, &pv, true, 16));
		assert(pv == 0x12F);
		assert(pp - str == 3);
		assert(end - pp == 0);
		assert(!*end);
	}

	{
		const char str[] = "12Fq";
		const char * pp = str;
		const char * end = str + 4;

		unsigned int pv;
		assert(!hb_parse_uint(&pp, end, &pv, true, 16));
		assert(hb_parse_uint(&pp, end, &pv, false, 16));
		assert(pv == 0x12F);
		assert(pp - str == 3);
		assert(end - pp == 1);
		assert(!*end);
	}

	{
		const char str[] = "-123";
		const char * pp = str;
		const char * end = str + 4;

		int pv;
		assert(hb_parse_int(&pp, end, &pv));
		assert(pv == -123);
		assert(pp - str == 4);
		assert(end - pp == 0);
		assert(!*end);
	}

	{
		const char str[] = "123";
		const char * pp = str;
		assert(ARRAY_LENGTH(str) == 4);
		const char * end = str + ARRAY_LENGTH(str);

		unsigned int pv;
		assert(hb_parse_uint(&pp, end, &pv));
		assert(pv == 123);
		assert(pp - str == 3);
		assert(end - pp == 1);
	}

	{
		const char str[] = "123\0";
		const char * pp = str;
		assert(ARRAY_LENGTH(str) == 5);
		const char * end = str + ARRAY_LENGTH(str);

		unsigned int pv;
		assert(hb_parse_uint(&pp, end, &pv));
		assert(pv == 123);
		assert(pp - str == 3);
		assert(end - pp == 2);
	}

	{
		const char str[] = "123V";
		const char * pp = str;
		assert(ARRAY_LENGTH(str) == 5);
		const char * end = str + ARRAY_LENGTH(str);

		unsigned int pv;
		assert(hb_parse_uint(&pp, end, &pv));
		assert(pv == 123);
		assert(pp - str == 3);
		assert(end - pp == 2);
	}

	{
		const char str[] = ".123";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str);

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == 123);
		assert(pp - str == 4);
		assert(end - pp == 1);
	}

	{
		const char str[] = "0.123";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str) - 1;

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == 123);
		assert(pp - str == 5);
		assert(end - pp == 0);
	}

	{
		const char str[] = "0.123e0";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str) - 1;

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == 123);
		assert(pp - str == 7);
		assert(end - pp == 0);
	}

	{
		const char str[] = "123e-3";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str) - 1;

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == 123);
		assert(pp - str == 6);
		assert(end - pp == 0);
	}

	{
		const char str[] = ".000123e+3";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str) - 1;

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == 123);
		assert(pp - str == 10);
		assert(end - pp == 0);
	}

	{
		const char str[] = "-.000000123e6";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str) - 1;

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == -123);
		assert(pp - str == 13);
		assert(end - pp == 0);
	}

	{
		const char str[] = "-1.23E-1";
		const char * pp = str;
		const char * end = str + ARRAY_LENGTH(str) - 1;

		double pv;
		assert(hb_parse_double(&pp, end, &pv));
		assert((int)roundf(pv * 1000.) == -123);
		assert(pp - str == 8);
		assert(end - pp == 0);
	}
	return ok;
}
//
//
//
/*static void test(hb_codepoint_t cp, unsigned int bit)
{
	if(OT::_hb_ot_os2_get_unicode_range_bit(cp) != bit) {
		slfprintf_stderr("got incorrect bit (%d) for cp 0x%X. Should have been %d.", OT::_hb_ot_os2_get_unicode_range_bit(cp), cp, bit);
		abort();
	}
}*/

static void test_get_unicode_range_bit()
{
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x0000) == 0);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x0042) == 0);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x007F) == 0);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x0080) == 1);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x30A0) == 50);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x30B1) == 50);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x30FF) == 50);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x10FFFD) == 90);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x30000) == -1);
	assert(OT::_hb_ot_os2_get_unicode_range_bit(0x110000) == -1);
	//test(0x0000, 0);
	//test(0x0042, 0);
	//test(0x007F, 0);
	//test(0x0080, 1);
	//test(0x30A0, 50);
	//test(0x30B1, 50);
	//test(0x30FF, 50);
	//test(0x10FFFD, 90);
	//test(0x30000, -1);
	//test(0x110000, -1);
}

int HarfBuzzTestUnicodeRanges() // ###
{
	int    ok = 1;
	test_get_unicode_range_bit();
	return ok;
}
//
//
//
template <typename T> struct U { typedef T type; };

int HarfBuzzTestMeta() // ###
{
	int    ok = 1;
	static_assert(hb_is_convertible(void, void), "");
	static_assert(hb_is_convertible(void, const void), "");
	static_assert(hb_is_convertible(const void, void), "");
	static_assert(hb_is_convertible(int,  int), "");
	static_assert(hb_is_convertible(char, int), "");
	static_assert(hb_is_convertible(long, int), "");
	static_assert(hb_is_convertible(int, int), "");
	static_assert(hb_is_convertible(const int, int), "");
	static_assert(hb_is_convertible(int, const int), "");
	static_assert(hb_is_convertible(const int, const int), "");
	static_assert(hb_is_convertible(int&, int), "");
	static_assert(!hb_is_convertible(int, int&), "");
	static_assert(hb_is_convertible(int, const int&), "");
	static_assert(!hb_is_convertible(const int, int&), "");
	static_assert(hb_is_convertible(const int, const int&), "");
	static_assert(hb_is_convertible(int&, const int), "");
	static_assert(hb_is_convertible(const int&, int), "");
	static_assert(hb_is_convertible(const int&, const int), "");
	static_assert(hb_is_convertible(const int&, const int), "");
	struct X {
	};
	struct Y : X {
	};
	static_assert(hb_is_convertible(const X &, const X), "");
	static_assert(hb_is_convertible(X &, const X), "");
	static_assert(hb_is_convertible(X &, const X &), "");
	static_assert(hb_is_convertible(X, const X &), "");
	static_assert(hb_is_convertible(const X, const X &), "");
	static_assert(!hb_is_convertible(const X, X &), "");
	// @sobolev static_assert(!hb_is_convertible(X, X &), "");
	static_assert(hb_is_convertible(X &, X &), "");
	static_assert(hb_is_convertible(int&, long), "");
	static_assert(!hb_is_convertible(int&, long&), "");
	static_assert(hb_is_convertible(int *, int *), "");
	static_assert(hb_is_convertible(int *, const int *), "");
	static_assert(!hb_is_convertible(const int *, int *), "");
	static_assert(!hb_is_convertible(int *, long *), "");
	static_assert(hb_is_convertible(int *, void *), "");
	static_assert(!hb_is_convertible(void *, int *), "");
	static_assert(hb_is_base_of(void, void), "");
	static_assert(hb_is_base_of(void, int), "");
	static_assert(!hb_is_base_of(int, void), "");
	static_assert(hb_is_base_of(int, int), "");
	static_assert(hb_is_base_of(const int, int), "");
	static_assert(hb_is_base_of(int, const int), "");
	static_assert(hb_is_base_of(X, X), "");
	static_assert(hb_is_base_of(X, Y), "");
	static_assert(hb_is_base_of(const X, Y), "");
	static_assert(hb_is_base_of(X, const Y), "");
	static_assert(!hb_is_base_of(Y, X), "");
	static_assert(hb_is_constructible(int), "");
	static_assert(hb_is_constructible(int, int), "");
	static_assert(hb_is_constructible(int, char), "");
	static_assert(hb_is_constructible(int, long), "");
	static_assert(!hb_is_constructible(int, X), "");
	static_assert(!hb_is_constructible(int, int, int), "");
	static_assert(hb_is_constructible(X), "");
	static_assert(!hb_is_constructible(X, int), "");
	static_assert(hb_is_constructible(X, X), "");
	static_assert(!hb_is_constructible(X, X, X), "");
	static_assert(hb_is_constructible(X, Y), "");
	static_assert(!hb_is_constructible(Y, X), "");
	static_assert(hb_is_trivially_default_constructible(X), "");
	static_assert(hb_is_trivially_default_constructible(Y), "");
	static_assert(hb_is_trivially_copy_constructible(X), "");
	static_assert(hb_is_trivially_copy_constructible(Y), "");
	static_assert(hb_is_trivially_move_constructible(X), "");
	static_assert(hb_is_trivially_move_constructible(Y), "");
	static_assert(hb_is_trivially_destructible(Y), "");
	static_assert(hb_is_trivially_copyable(int), "");
	static_assert(hb_is_trivially_copyable(X), "");
	static_assert(hb_is_trivially_copyable(Y), "");
	static_assert(hb_is_trivial(int), "");
	static_assert(hb_is_trivial(X), "");
	static_assert(hb_is_trivial(Y), "");
	static_assert(hb_is_signed(hb_unwrap_type(U<U<U<int> > >)), "");
	static_assert(hb_is_unsigned(hb_unwrap_type(U<U<U<U<unsigned> > > >)), "");
	/* TODO Add more meaningful tests. */
	return ok;
}
//
//
//
int HarfBuzzTestCommon(const char * pFileName) // ###
{
	int    ok = 1;
	/*if(argc != 2) {
		slfprintf_stderr("usage: %s font-file.ttf\n", argv[0]);
		exit(1);
	}*/
	hb_blob_t * blob = hb_blob_create_from_file(pFileName);
	printf("Opened font file %s: %u bytes long\n", pFileName, hb_blob_get_length(blob));
	// Create the face 
	hb_face_t * face = hb_face_create(blob, 0 /* first face */);
	hb_blob_destroy(blob);
	blob = nullptr;
	unsigned int upem = hb_face_get_upem(face);
	hb_font_t * font = hb_font_create(face);
	hb_font_set_scale(font, upem, upem);
#ifdef HAVE_FREETYPE
	hb_ft_font_set_funcs(font);
#endif
	hb_buffer_t * buffer = hb_buffer_create();
	hb_buffer_add_utf8(buffer, "\xe0\xa4\x95\xe0\xa5\x8d\xe0\xa4\xb0\xe0\xa5\x8d\xe0\xa4\x95", -1, 0, -1);
	hb_buffer_guess_segment_properties(buffer);
	hb_shape(font, buffer, nullptr, 0);
	unsigned int count = hb_buffer_get_length(buffer);
	hb_glyph_info_t * infos = hb_buffer_get_glyph_infos(buffer, nullptr);
	hb_glyph_position_t * positions = hb_buffer_get_glyph_positions(buffer, nullptr);
	for(uint i = 0; i < count; i++) {
		hb_glyph_info_t * info = &infos[i];
		hb_glyph_position_t * pos = &positions[i];
		printf("cluster %d	glyph 0x%x at	(%d,%d)+(%d,%d)\n", info->cluster, info->codepoint, pos->x_offset,
		    pos->y_offset, pos->x_advance, pos->y_advance);
	}
	hb_buffer_destroy(buffer);
	hb_font_destroy(font);
	hb_face_destroy(face);
	return ok;
}
//
//
//
int HarfBuzzTestBufferSerialize(const char * pFileName) // ###
{
	int    ok = 1;
#ifndef HB_NO_BUFFER_SERIALIZE
	/*if(argc != 2) {
		slfprintf_stderr("usage: %s font-file\n", argv[0]);
		exit(1);
	}*/
	hb_blob_t * blob = hb_blob_create_from_file(pFileName);
	hb_face_t * face = hb_face_create(blob, 0 /* first face */);
	hb_blob_destroy(blob);
	blob = nullptr;
	unsigned int upem = hb_face_get_upem(face);
	hb_font_t * font = hb_font_create(face);
	hb_face_destroy(face);
	hb_font_set_scale(font, upem, upem);
	hb_ot_font_set_funcs(font);
#ifdef HAVE_FREETYPE
	//hb_ft_font_set_funcs (font);
#endif
	hb_buffer_t * buf = hb_buffer_create();
	char line[BUFSIZ], out[BUFSIZ];
	while(fgets(line, sizeof(line), stdin)) {
		hb_buffer_clear_contents(buf);
		const char * p = line;
		while(hb_buffer_deserialize_glyphs(buf, p, -1, &p, font, HB_BUFFER_SERIALIZE_FORMAT_JSON))
			;
		if(*p && *p != '\n')
			ok = 0;
		hb_buffer_serialize_glyphs(buf, 0, hb_buffer_get_length(buf), out, sizeof(out), nullptr, font, HB_BUFFER_SERIALIZE_FORMAT_JSON, HB_BUFFER_SERIALIZE_FLAG_DEFAULT);
		puts(out);
	}
	hb_buffer_destroy(buf);
	hb_font_destroy(font);
#endif
	return ok;
}
//
//
//
int HarfBuzzTestGPosSizeParams(const char * pFileName) // ###
{
	int    ok = 1;
	/*if(argc != 2) {
		slfprintf_stderr("usage: %s font-file\n", argv[0]);
		exit(1);
	}*/
	// Create the face 
	hb_blob_t * blob = hb_blob_create_from_file(pFileName);
	hb_face_t * face = hb_face_create(blob, 0 /* first face */);
	hb_blob_destroy(blob);
	blob = nullptr;
#ifndef HB_NO_LAYOUT_FEATURE_PARAMS
	unsigned int p[5];
	ok = hb_ot_layout_get_size_params(face, p, p+1, (p+2), p+3, p+4);
	printf("%g %u %u %g %g\n", p[0]/10., p[1], p[2], p[3]/10., p[4]/10.);
#endif
	hb_face_destroy(face);
	return ok;
}
//
//
//
int HarfBuzzTestOtGlyphName(const char * pFileName) // ###
{
	int    ok = 1;
	/*if(argc != 2) {
		slfprintf_stderr("usage: %s font-file\n", argv[0]);
		exit(1);
	}*/
	hb_blob_t * blob = hb_blob_create_from_file(pFileName);
	hb_face_t * face = hb_face_create(blob, 0 /* first face */);
	hb_font_t * font = hb_font_create(face);
	hb_blob_destroy(blob);
	blob = nullptr;
	const unsigned int num_glyphs = hb_face_get_glyph_count(face);
	for(hb_codepoint_t gid = 0; gid < num_glyphs; gid++) {
		char buf[64];
		unsigned int buf_size = sizeof(buf);
		if(hb_font_get_glyph_name(font, gid, buf, buf_size)) {
			hb_codepoint_t gid_inv;
			if(hb_font_get_glyph_from_name(font, buf, strlen(buf), &gid_inv)) {
				if(gid == gid_inv) {
					printf("%u <-> %s\n", gid, buf);
				}
				else {
					printf("%u -> %s -> %u\n", gid, buf, gid_inv);
					ok = 0;
				}
			}
			else {
				printf("%u -> %s -> ?\n", gid, buf);
				ok = 0;
			}
		}
		else {
			printf("%u -> ?\n", gid);
			ok = 0;
		}
	}
	hb_font_destroy(font);
	hb_face_destroy(face);
	return ok;
}
//
//
//
int HarfBuzzTestOtMeta(const char * pFileName) // ###
{
	int    ok = 1;
	/*if(argc != 2) {
		slfprintf_stderr("usage: %s font-file\n", argv[0]);
		exit(1);
	}*/
	hb_blob_t * blob = hb_blob_create_from_file(pFileName);
	hb_face_t * face = hb_face_create(blob, 0 /* first face */);
	hb_blob_destroy(blob);
	blob = nullptr;
	unsigned int count = 0;
#ifndef HB_NO_META
	count = hb_ot_meta_get_entry_tags(face, 0, nullptr, nullptr);
	hb_ot_meta_tag_t * tags = (hb_ot_meta_tag_t *)SAlloc::M(sizeof(hb_ot_meta_tag_t) * count);
	hb_ot_meta_get_entry_tags(face, 0, &count, tags);
	for(unsigned i = 0; i < count; ++i) {
		hb_blob_t * entry = hb_ot_meta_reference_entry(face, tags[i]);
		printf("%c%c%c%c, size: %d: %.*s\n", HB_UNTAG(tags[i]), hb_blob_get_length(entry), hb_blob_get_length(entry), hb_blob_get_data(entry, nullptr));
		hb_blob_destroy(entry);
	}
	SAlloc::F(tags);
#endif
	hb_face_destroy(face);
	if(!count)
		ok = 0;
	return ok;
}
