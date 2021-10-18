// MIMALLOC-TEST.CPP
//
#include <slib-internal.h>
#pragma hdrstop
#include "..\mimalloc.h"
#ifdef __cplusplus
	#include <vector>
#endif
#ifdef _WIN32
	//#include <Windows.h>
#else
	#include <pthread.h>
#endif
// #define USE_STD_MALLOC
#ifdef USE_STD_MALLOC
	#define custom_calloc(n, s)    calloc(n, s)
	#define custom_realloc(p, s)   realloc(p, s)
	#define custom_free(p)        free(p)
#else
	#define custom_calloc(n, s)    mi_calloc(n, s)
	#define custom_realloc(p, s)   mi_realloc(p, s)
	#define custom_free(p)        mi_free(p)
#endif
#define STRESS // undefine for leak test

int MiMalloc_Test01() 
{
	class InnerBlock {
	public:
		static void test_heap(void * p_out) 
		{
			mi_heap_t* heap = mi_heap_new();
			void * p1 = mi_heap_malloc(heap, 32);
			void * p2 = mi_heap_malloc(heap, 48);
			mi_free(p_out);
			mi_heap_destroy(heap);
			//mi_heap_delete(heap); mi_free(p1); mi_free(p2);
		}
		static void test_large() 
		{
			const size_t N = 1000;
			for(size_t i = 0; i < N; ++i) {
				size_t sz = 1ull << 21;
				char* a = mi_mallocn_tp(char, sz);
				for(size_t k = 0; k < sz; k++) {
					a[k] = 'x';
				}
				mi_free(a);
			}
		}
	};
	void * p1 = mi_malloc(16);
	void * p2 = mi_malloc(1000000);
	mi_free(p1);
	mi_free(p2);
	p1 = mi_malloc(16);
	p2 = mi_malloc(16);
	mi_free(p1);
	mi_free(p2);
	InnerBlock::test_heap(mi_malloc(32));
	p1 = mi_malloc_aligned(64, 16);
	p2 = mi_malloc_aligned(160, 24);
	mi_free(p2);
	mi_free(p1);
	//test_large();
	mi_collect(true);
	mi_stats_print(NULL);
	return 0;
}
//
//
//
// > mimalloc-test-stress [THREADS] [SCALE] [ITER]
//

// static int THREADS = 8;    // more repeatable if THREADS <= #processors
// static int SCALE   = 100;  // scaling factor

// transfer pointer between threads
#define TRANSFERS     (1000)
typedef uintptr_t * random_t;

//static void * atomic_exchange_ptr(volatile void** p, void * newval);
//static void run_os_threads(size_t nthreads, void (*entry)(intptr_t tid));

int MiMalloc_Test02_Stress(int argc, char ** argv) 
{
	#if (UINTPTR_MAX != UINT32_MAX)
		static const uintptr_t cookie = 0xbf58476d1ce4e5b9UL;
	#else
		static const uintptr_t cookie = 0x1ce4e5b9UL;
	#endif
	// argument defaults
	static int THREADS = 32; // more repeatable if THREADS <= #processors
	static int SCALE   = 10; // scaling factor
	static int ITER    = 50; // N full iterations destructing and re-creating all threads
	static bool allow_large_objects = true;      // allow very large objects?
	static size_t use_one_size = 0;              // use single object size of `N * sizeof(uintptr_t)`?
	static volatile void * transfer[TRANSFERS];
	static void (* thread_entry_fun)(intptr_t);
	class InnerBlock {
	public:
#ifdef _WIN32
		static DWORD WINAPI thread_entry(LPVOID param) 
		{
			thread_entry_fun((intptr_t)param);
			return 0;
		}
		static void run_os_threads(size_t nthreads, void (*fun)(intptr_t)) 
		{
			thread_entry_fun = fun;
			DWORD* tids = (DWORD*)custom_calloc(nthreads, sizeof(DWORD));
			HANDLE* thandles = (HANDLE*)custom_calloc(nthreads, sizeof(HANDLE));
			for(uintptr_t i = 0; i < nthreads; i++) {
				thandles[i] = CreateThread(0, 8*1024, &thread_entry, (void *)(i), 0, &tids[i]);
			}
			for(size_t i = 0; i < nthreads; i++) {
				WaitForSingleObject(thandles[i], INFINITE);
			}
			for(size_t i = 0; i < nthreads; i++) {
				CloseHandle(thandles[i]);
			}
			custom_free(tids);
			custom_free(thandles);
		}
		static void test_stress() 
		{
			uintptr_t r = rand();
			for(int n = 0; n < ITER; n++) {
				run_os_threads(THREADS, &stress);
				for(int i = 0; i < TRANSFERS; i++) {
					if(chance(50, &r) || n + 1 == ITER) { // free all on last run, otherwise free half of the transfers
						void * p = atomic_exchange_ptr(&transfer[i], NULL);
						free_items(p);
					}
				}
				// mi_collect(false);
		#if !defined(NDEBUG) || defined(MI_TSAN)
				if((n + 1) % 10 == 0) {
					printf("- iterations left: %3d\n", ITER - (n + 1));
				}
		#endif
			}
		}
		static void * atomic_exchange_ptr(volatile void** p, void * newval) 
		{
		#if (INTPTR_MAX == INT32_MAX)
			return (void *)InterlockedExchange((volatile LONG*)p, (LONG)newval);
		#else
			return (void *)InterlockedExchange64((volatile LONG64*)p, (LONG64)newval);
		#endif
		}
	#else
		static void * thread_entry(void * param) 
		{
			thread_entry_fun((uintptr_t)param);
			return NULL;
		}
		static void run_os_threads(size_t nthreads, void (*fun)(intptr_t)) 
		{
			thread_entry_fun = fun;
			pthread_t * threads = (pthread_t*)custom_calloc(nthreads, sizeof(pthread_t));
			memzero(threads, sizeof(pthread_t) * nthreads);
			//pthread_setconcurrency(nthreads);
			for(size_t i = 0; i < nthreads; i++) {
				pthread_create(&threads[i], NULL, &thread_entry, (void *)i);
			}
			for(size_t i = 0; i < nthreads; i++) {
				pthread_join(threads[i], NULL);
			}
			custom_free(threads);
		}
		#ifdef __cplusplus
			#include <atomic>
			static void * atomic_exchange_ptr(volatile void** p, void * newval) 
			{
				return std::atomic_exchange((volatile std::atomic<void*>*)p, newval);
			}
		#else
			#include <stdatomic.h>
			static void * atomic_exchange_ptr(volatile void** p, void * newval) 
			{
				return atomic_exchange((volatile _Atomic(void *)*)p, newval);
			}
		#endif
#endif
#ifndef STRESS
		static void leak(intptr_t tid) 
		{
			uintptr_t r = rand();
			void * p = alloc_items(1 /*pick(&r)%128*/, &r);
			if(chance(50, &r)) {
				intptr_t i = (pick(&r) % TRANSFERS);
				void * q = atomic_exchange_ptr(&transfer[i], p);
				free_items(q);
			}
		}
		static void test_leak(void) 
		{
			for(int n = 0; n < ITER; n++) {
				run_os_threads(THREADS, &leak);
				mi_collect(false);
	#ifndef NDEBUG
				if((n + 1) % 10 == 0) {
					printf("- iterations left: %3d\n", ITER - (n + 1));
				}
	#endif
			}
		}
#endif
		static uintptr_t pick(random_t r) 
		{
			uintptr_t x = *r;
		#if (UINTPTR_MAX > UINT32_MAX)
			// by Sebastiano Vigna, see: <http://xoshiro.di.unimi.it/splitmix64.c>
			x ^= x >> 30;
			x *= 0xbf58476d1ce4e5b9UL;
			x ^= x >> 27;
			x *= 0x94d049bb133111ebUL;
			x ^= x >> 31;
		#else
			// by Chris Wellons, see: <https://nullprogram.com/blog/2018/07/31/>
			x ^= x >> 16;
			x *= 0x7feb352dUL;
			x ^= x >> 15;
			x *= 0x846ca68bUL;
			x ^= x >> 16;
		#endif
			*r = x;
			return x;
		}
		static bool chance(size_t perc, random_t r) { return (pick(r) % 100 <= perc); }
		static void * alloc_items(size_t items, random_t r) 
		{
			if(chance(1, r)) {
				if(chance(1, r) && allow_large_objects) 
					items *= 10000; // 0.01% giant
				else if(chance(10, r) && allow_large_objects) 
					items *= 1000; // 0.1% huge
				else 
					items *= 100; // 1% large objects;
			}
			if(items == 40) 
				items++;         // pthreads uses that size for stack increases
			if(use_one_size > 0) 
				items = (use_one_size / sizeof(uintptr_t));
			if(items == 0) 
				items = 1;
			uintptr_t* p = (uintptr_t*)custom_calloc(items, sizeof(uintptr_t));
			if(p) {
				for(uintptr_t i = 0; i < items; i++) {
					p[i] = (items - i) ^ cookie;
				}
			}
			return p;
		}
		static void free_items(void * p) 
		{
			if(p) {
				uintptr_t* q = (uintptr_t*)p;
				uintptr_t items = (q[0] ^ cookie);
				for(uintptr_t i = 0; i < items; i++) {
					if((q[i] ^ cookie) != items - i) {
						fprintf(stderr, "memory corruption at block %p at %zu\n", p, i);
						abort();
					}
				}
			}
			custom_free(p);
		}
		static void stress(intptr_t tid) 
		{
			//bench_start_thread();
			uintptr_t r = ((tid + 1) * 43); // rand();
			const size_t max_item_shift = 5; // 128
			const size_t max_item_retained_shift = max_item_shift + 2;
			size_t allocs = 100 * ((size_t)SCALE) * (tid % 8 + 1); // some threads do more
			size_t retain = allocs / 2;
			void ** data = NULL;
			size_t data_size = 0;
			size_t data_top = 0;
			void ** retained = (void**)custom_calloc(retain, sizeof(void *));
			size_t retain_top = 0;
			while(allocs > 0 || retain > 0) {
				if(retain == 0 || (chance(50, &r) && allocs > 0)) {
					// 50%+ alloc
					allocs--;
					if(data_top >= data_size) {
						data_size += 100000;
						data = (void**)custom_realloc(data, data_size * sizeof(void *));
					}
					data[data_top++] = alloc_items(1ULL << (pick(&r) % max_item_shift), &r);
				}
				else {
					// 25% retain
					retained[retain_top++] = alloc_items(1ULL << (pick(&r) % max_item_retained_shift), &r);
					retain--;
				}
				if(chance(66, &r) && data_top > 0) {
					// 66% free previous alloc
					size_t idx = pick(&r) % data_top;
					free_items(data[idx]);
					data[idx] = NULL;
				}
				if(chance(25, &r) && data_top > 0) {
					// 25% exchange a local pointer with the (shared) transfer buffer.
					size_t data_idx = pick(&r) % data_top;
					size_t transfer_idx = pick(&r) % TRANSFERS;
					void * p = data[data_idx];
					void * q = atomic_exchange_ptr(&transfer[transfer_idx], p);
					data[data_idx] = q;
				}
			}
			// free everything that is left
			for(size_t i = 0; i < retain_top; i++) {
				free_items(retained[i]);
			}
			for(size_t i = 0; i < data_top; i++) {
				free_items(data[i]);
			}
			custom_free(retained);
			custom_free(data);
			//bench_end_thread();
		}
	};
	thread_entry_fun = InnerBlock::stress;
	// > mimalloc-test-stress [THREADS] [SCALE] [ITER]
	if(argc >= 2) {
		char* end;
		long n = strtol(argv[1], &end, 10);
		if(n > 0) 
			THREADS = n;
	}
	if(argc >= 3) {
		char * end;
		long n = (strtol(argv[2], &end, 10));
		if(n > 0) 
			SCALE = n;
	}
	if(argc >= 4) {
		char* end;
		long n = (strtol(argv[3], &end, 10));
		if(n > 0) 
			ITER = n;
	}
	printf("Using %d threads with a %d%% load-per-thread and %d iterations\n", THREADS, SCALE, ITER);
	//mi_reserve_os_memory(1024*1024*1024ULL, false, true);
	//int res = mi_reserve_huge_os_pages(4,1);
	//printf("(reserve huge: %i\n)", res);

	//bench_start_program();

	// Run ITER full iterations where half the objects in the transfer buffer survive to the next round.
	srand(0x7feb352d);
	// mi_stats_reset();
#ifdef STRESS
	InnerBlock::test_stress();
#else
	test_leak();
#endif
#ifndef USE_STD_MALLOC
  #ifndef NDEBUG
	mi_collect(true);
  #endif
	mi_stats_print(NULL);
#endif
	//bench_end_program();
	return 0;
}
// 
// Testing allocators is difficult as bugs may only surface after particular
// allocation patterns. The main approach to testing _mimalloc_ is therefore
// to have extensive internal invariant checking (see `page_is_valid` in `page.c`
// for example), which is enabled in debug mode with `-DMI_DEBUG_FULL=ON`.
// The main testing is then to run `mimalloc-bench` [1] using full invariant checking
// to catch any potential problems over a wide range of intensive allocation bench marks.
// 
// However, this does not test well for the entire API surface. In this test file
// we therefore test the API over various inputs. Please add more tests :-)
// 
// [1] https://github.com/daanx/mimalloc-bench
//
// Larger test functions
//
// Main testing
//
int MiMalloc_Test03()
{
	static int ok = 0;
	static int failed = 0;
	#undef CHECK_BODY
	#undef CHECK
	#define CHECK_BODY(name, body) \
		do { \
			fprintf(stderr, "test: %s...  ", name); \
			bool result = true;                                     \
			do { body } while(false);                                \
			if(!(result)) {                                        \
				failed++; \
				fprintf(stderr, "\n  FAILED: %s:%d:\n  %s\n", __FILE__, __LINE__, #body);                                       \
				/* exit(1); */ \
			} \
			else { \
				ok++;                               \
				fprintf(stderr, "ok.\n");                    \
			}                                             \
		} while(false)

	#define CHECK(name, expr) CHECK_BODY(name, { result = (expr); })

	struct some_struct { 
		int i; 
		int j; 
		double z; 
	};
	class InnerBlock {
	public:
		static bool test_heap1() 
		{
			mi_heap_t* heap = mi_heap_new();
			int* p1 = mi_heap_malloc_tp(heap, int);
			int* p2 = mi_heap_malloc_tp(heap, int);
			*p1 = *p2 = 43;
			mi_heap_destroy(heap);
			return true;
		}
		static bool test_heap2() 
		{
			mi_heap_t* heap = mi_heap_new();
			int* p1 = mi_heap_malloc_tp(heap, int);
			int* p2 = mi_heap_malloc_tp(heap, int);
			mi_heap_delete(heap);
			*p1 = 42;
			mi_free(p1);
			mi_free(p2);
			return true;
		}
		static bool test_stl_allocator1() 
		{
		#ifdef __cplusplus
			std::vector<int, mi_stl_allocator<int>> vec;
			vec.push_back(1);
			vec.pop_back();
			return vec.size() == 0;
		#else
			return true;
		#endif
		}
		static bool test_stl_allocator2() 
		{
		#ifdef __cplusplus
			std::vector<some_struct, mi_stl_allocator<some_struct>> vec;
			vec.push_back(some_struct());
			vec.pop_back();
			return vec.size() == 0;
		#else
			return true;
		#endif
		}
	};
	mi_option_disable(mi_option_verbose);
	// ---------------------------------------------------
	// Malloc
	// ---------------------------------------------------
	CHECK_BODY("malloc-zero", { void * p = mi_malloc(0); mi_free(p); });
	CHECK_BODY("malloc-nomem1", { result = (mi_malloc(SIZE_MAX/2) == NULL); });
	CHECK_BODY("malloc-null", { mi_free(NULL); });
	CHECK_BODY("calloc-overflow", {
		// use (size_t)&mi_calloc to get some number without triggering compiler warnings
		result = (mi_calloc((size_t)&mi_calloc, SIZE_MAX/1000) == NULL);
	});
	CHECK_BODY("calloc0", { result = (mi_usable_size(mi_calloc(0, 1000)) <= 16); });
	// ---------------------------------------------------
	// Extended
	// ---------------------------------------------------
	CHECK_BODY("posix_memalign1", {
		void * p = &p;
		int err = mi_posix_memalign(&p, sizeof(void *), 32);
		result = ((err==0 && (uintptr_t)p % sizeof(void *) == 0) || p==&p);
		mi_free(p);
	});
	CHECK_BODY("posix_memalign_no_align", {
		void * p = &p;
		int err = mi_posix_memalign(&p, 3, 32);
		result = (err==EINVAL && p==&p);
	});
	CHECK_BODY("posix_memalign_zero", {
		void * p = &p;
		int err = mi_posix_memalign(&p, sizeof(void *), 0);
		mi_free(p);
		result = (err==0);
	});
	CHECK_BODY("posix_memalign_nopow2", {
		void * p = &p;
		int err = mi_posix_memalign(&p, 3*sizeof(void *), 32);
		result = (err==EINVAL && p==&p);
	});
	CHECK_BODY("posix_memalign_nomem", {
		void * p = &p;
		int err = mi_posix_memalign(&p, sizeof(void *), SIZE_MAX);
		result = (err==ENOMEM && p==&p);
	});

	// ---------------------------------------------------
	// Aligned API
	// ---------------------------------------------------
	CHECK_BODY("malloc-aligned1", {
		void * p = mi_malloc_aligned(32, 32); result = (p != NULL && (uintptr_t)(p) % 32 == 0); mi_free(p);
	});
	CHECK_BODY("malloc-aligned2", {
		void * p = mi_malloc_aligned(48, 32); result = (p != NULL && (uintptr_t)(p) % 32 == 0); mi_free(p);
	});
	CHECK_BODY("malloc-aligned3", {
		void * p1 = mi_malloc_aligned(48, 32); bool result1 = (p1 != NULL && (uintptr_t)(p1) % 32 == 0);
		void * p2 = mi_malloc_aligned(48, 32); bool result2 = (p2 != NULL && (uintptr_t)(p2) % 32 == 0);
		mi_free(p2);
		mi_free(p1);
		result = (result1&&result2);
	});
	CHECK_BODY("malloc-aligned4", {
		void * p;
		bool ok = true;
		for(int i = 0; i < 8 && ok; i++) {
			p = mi_malloc_aligned(8, 16);
			ok = (p != NULL && (uintptr_t)(p) % 16 == 0); mi_free(p);
		}
		result = ok;
	});
	CHECK_BODY("malloc-aligned5", {
		void * p = mi_malloc_aligned(4097, 4096); size_t usable = mi_usable_size(
			p); result = usable >= 4097 && usable < 10000; mi_free(p);
	});
	CHECK_BODY("malloc-aligned-at1", {
		void * p = mi_malloc_aligned_at(48, 32, 0); result = (p != NULL && ((uintptr_t)(p) + 0) % 32 == 0); mi_free(p);
	});
	CHECK_BODY("malloc-aligned-at2", {
		void * p = mi_malloc_aligned_at(50, 32, 8); result = (p != NULL && ((uintptr_t)(p) + 8) % 32 == 0); mi_free(p);
	});
	CHECK_BODY("memalign1", {
		void * p;
		bool ok = true;
		for(int i = 0; i < 8 && ok; i++) {
			p = mi_memalign(16, 8);
			ok = (p != NULL && (uintptr_t)(p) % 16 == 0); mi_free(p);
		}
		result = ok;
	});

	// ---------------------------------------------------
	// Heaps
	// ---------------------------------------------------
	CHECK("heap_destroy", InnerBlock::test_heap1());
	CHECK("heap_delete", InnerBlock::test_heap2());

	//mi_stats_print(NULL);

	// ---------------------------------------------------
	// various
	// ---------------------------------------------------
	CHECK_BODY("realpath", {
		char* s = mi_realpath(".", NULL);
		// printf("realpath: %s\n",s);
		mi_free(s);
	});
	CHECK("stl_allocator1", InnerBlock::test_stl_allocator1());
	CHECK("stl_allocator2", InnerBlock::test_stl_allocator2());
	//
	// Done
	//
	fprintf(stderr, "\n\n---------------------------------------------\nsucceeded: %i\nfailed   : %i\n\n", ok, failed);
	return failed;
	#undef CHECK_BODY
	#undef CHECK
}

