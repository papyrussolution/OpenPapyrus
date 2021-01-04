// GRAVITY_.H
//  Created by Marco Bambini on 01/04/16.
//  Copyright © 2016 CreoLabs. All rights reserved.
//  Modified by Anton Sobolev 2020
// 
#include <slib.h>
#include <inttypes.h>
//#include <unistd.h>
//#include <stdbool.h>
#if defined(__linux)
	#include <sys/time.h>
#endif
#if defined(__MACH__)
	#include <mach/mach_time.h>
#endif
#if defined(_WIN32)
	//#include <windows.h>
	#include <Shlwapi.h>
	//#include <tchar.h>
#endif
/* @v10.8.1 #ifdef _MSC_VER
	typedef int mode_t; // MSVC doesn't have mode_t 
	typedef SSIZE_T ssize_t;
#endif*/
struct GravityValue;
struct gravity_class_t;
struct gravity_closure_t;
struct gravity_hash_t;
struct gravity_string_t;
struct gravity_list_t;
struct ircode_t;
struct gravity_vm;

#define GRAVITY_VERSION                     "0.7.8"     // git tag 0.7.8
#define GRAVITY_VERSION_NUMBER              0x000708    // git push --tags
#define GRAVITY_BUILD_DATE                  __DATE__
#ifndef GRAVITY_ENABLE_DOUBLE
	#define GRAVITY_ENABLE_DOUBLE               1           // if 1 enable gravity_float_t to be a double (instead of a float)
#endif
#ifndef GRAVITY_ENABLE_INT64
	#define GRAVITY_ENABLE_INT64                1           // if 1 enable gravity_int_t to be a 64bit int (instead of a 32bit int)
#endif
#ifndef GRAVITY_COMPUTED_GOTO
	#define GRAVITY_COMPUTED_GOTO               1           // if 1 enable faster computed goto (instead of switch) for compilers that support it
#endif
#ifndef GRAVITY_NULL_SILENT
	#define GRAVITY_NULL_SILENT                 1           // if 1 then messages sent to null does not produce any runtime error
#endif
#ifndef GRAVITY_MAP_DOTSUGAR
	#define GRAVITY_MAP_DOTSUGAR                1           // if 1 then map objects can be accessed with both map[key] and map.key
#endif
#ifdef _MSC_VER
	#undef GRAVITY_COMPUTED_GOTO
	#define GRAVITY_COMPUTED_GOTO               0           // MSVC does not support it
#endif
#define MAIN_FUNCTION                       "main"
#define ITERATOR_INIT_FUNCTION              "iterate"
#define ITERATOR_NEXT_FUNCTION              "next"
#define INITMODULE_NAME                     "$moduleinit"
#define CLASS_INTERNAL_INIT_NAME            "$init"
#define CLASS_CONSTRUCTOR_NAME              "init"
#define CLASS_DESTRUCTOR_NAME               "deinit"
#define SELF_PARAMETER_NAME                 "self"
#define OUTER_IVAR_NAME                     "outer"
#define GETTER_FUNCTION_NAME                "get"
#define SETTER_FUNCTION_NAME                "set"
#define SETTER_PARAMETER_NAME               "value"
#define GLOBALS_DEFAULT_SLOT                4096
#define CPOOL_INDEX_MAX                     4096        // 2^12
#define CPOOL_VALUE_SUPER                   CPOOL_INDEX_MAX+1
#define CPOOL_VALUE_NULL                    CPOOL_INDEX_MAX+2
#define CPOOL_VALUE_UNDEFINED               CPOOL_INDEX_MAX+3
#define CPOOL_VALUE_ARGUMENTS               CPOOL_INDEX_MAX+4
#define CPOOL_VALUE_TRUE                    CPOOL_INDEX_MAX+5
#define CPOOL_VALUE_FALSE                   CPOOL_INDEX_MAX+6
#define CPOOL_VALUE_FUNC                    CPOOL_INDEX_MAX+7
#define MAX_INSTRUCTION_OPCODE              64              // 2^6
#define MAX_REGISTERS                       256             // 2^8
#define MAX_LOCALS                          200             // maximum number of local variables
#define MAX_UPVALUES                        200             // maximum number of upvalues
#define MAX_INLINE_INT                      131072          // 32 - 6 (OPCODE) - 8 (register) - 1 bit sign = 17
#define MAX_FIELDSxFLUSH                    64              // used in list/map serialization
#define MAX_IVARS                           768             // 2^10 - 2^8
#define MAX_ALLOCATION                      4194304         // 1024 * 1024 * 4 (about 4 millions entry)
#define MAX_CCALLS                          100             // default maximum number of nested C calls
#define MAX_MEMORY_BLOCK                    157286400       // 150MB
#define DEFAULT_CONTEXT_SIZE                256             // default VM context entries (can grow)
#define DEFAULT_MINSTRING_SIZE              32              // minimum string allocation size
#define DEFAULT_MINSTACK_SIZE               256             // sizeof(GravityValue) * 256     = 16 * 256 => 4 KB
#define DEFAULT_MINCFRAME_SIZE              32              // sizeof(gravity_callframe_t) * 48  = 32 * 48 => 1.5 KB
#define DEFAULT_CG_THRESHOLD                5*1024*1024     // 5MB
#define DEFAULT_CG_MINTHRESHOLD             1024*1024       // 1MB
#define DEFAULT_CG_RATIO                    0.5             // 50%
//#define MAXNUM(a, b)                         ((a) > (b) ? a : b)
//#define MINNUM(a, b)                         ((a) < (b) ? a : b)
#define EPSILON                             0.000001
#define MIN_LIST_RESIZE                     12              // value used when a List is resized
#define GRAVITY_DATA_REGISTER               UINT32_MAX
#define GRAVITY_FIBER_REGISTER              UINT32_MAX-1
#define GRAVITY_MSG_REGISTER                UINT32_MAX-2
#define GRAVITY_BRIDGE_INDEX                UINT16_MAX
#define GRAVITY_COMPUTED_INDEX              UINT16_MAX-1
//DLL export/import support for Windows
#if !defined(GRAVITY_API) && defined(_WIN32) && defined(BUILD_GRAVITY_API)
	#define GRAVITY_API __declspec(dllexport)
#else
	#define GRAVITY_API
#endif
// MARK: - STRUCT -
// FLOAT_MAX_DECIMALS FROM
// https://stackoverflow.com/questions/13542944/how-many-significant-digits-have-floats-and-doubles-in-java
#if GRAVITY_ENABLE_DOUBLE
	typedef double gravity_float_t;
	#define GRAVITY_FLOAT_MAX                   DBL_MAX
	#define GRAVITY_FLOAT_MIN                   DBL_MIN
	#define FLOAT_MAX_DECIMALS                  16
	#define FLOAT_EPSILON                       0.00001
#else
	typedef float gravity_float_t;
	#define GRAVITY_FLOAT_MAX                   FLT_MAX
	#define GRAVITY_FLOAT_MIN                   FLT_MIN
	#define FLOAT_MAX_DECIMALS                  7
	#define FLOAT_EPSILON                       0.00001
#endif
#if GRAVITY_ENABLE_INT64
	typedef int64_t gravity_int_t;
	#define GRAVITY_INT_MAX                     9223372036854775807
	#define GRAVITY_INT_MIN                     (-GRAVITY_INT_MAX-1LL)
#else
	typedef int32 gravity_int_t;
	#define GRAVITY_INT_MAX                     2147483647
	#define GRAVITY_INT_MIN                     -2147483648
#endif
//#include <debug_macros.h>
#define GRAVITY_LEXEM_DEBUG             0
#define GRAVITY_LEXER_DEBUG             0
#define GRAVITY_PARSER_DEBUG            0
#define GRAVITY_SEMA1_DEBUG             0
#define GRAVITY_SEMA2_DEBUG             0
#define GRAVITY_AST_DEBUG               0
#define GRAVITY_LOOKUP_DEBUG            0
#define GRAVITY_SYMTABLE_DEBUG          0
#define GRAVITY_CODEGEN_DEBUG           0
#define GRAVITY_OPCODE_DEBUG            0
#define GRAVITY_BYTECODE_DEBUG          0
#define GRAVITY_REGISTER_DEBUG          0
#define GRAVITY_FREE_DEBUG              0
#define GRAVITY_DESERIALIZE_DEBUG       0
#define PRINT_LINE(...)                 printf(__VA_ARGS__);printf("\n");fflush(stdout)
#if GRAVITY_LEXER_DEBUG
	#define DEBUG_LEXER(l)                  gravity_lexer_debug(l)
#else
	#define DEBUG_LEXER(...)
#endif
#if GRAVITY_LEXEM_DEBUG
	#define DEBUG_LEXEM(...) do { if (!lexer->peeking) { printf("(%03d, %03d, %02d) ", lexer->token.lineno, lexer->token.colno, lexer->token.position); PRINT_LINE(__VA_ARGS__);} } while(0)
#else
	#define DEBUG_LEXEM(...)
#endif
#if GRAVITY_PARSER_DEBUG
	#define DEBUG_PARSER(...)               PRINT_LINE(__VA_ARGS__)
#else
	#define gravity_parser_debug(p)
	#define DEBUG_PARSER(...)
#endif
#if GRAVITY_SEMA1_DEBUG
	#define DEBUG_SEMA1(...)                PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_SEMA1(...)
#endif
#if GRAVITY_SEMA2_DEBUG
	#define DEBUG_SEMA2(...)                PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_SEMA2(...)
#endif
#if GRAVITY_LOOKUP_DEBUG
	#define DEBUG_LOOKUP(...)               PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_LOOKUP(...)
#endif
#if GRAVITY_SYMTABLE_DEBUG
	#define DEBUG_SYMTABLE(...)             printf("%*s",ident*4," ");PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_SYMTABLE(...)
#endif
#if GRAVITY_CODEGEN_DEBUG
	#define DEBUG_CODEGEN(...)              PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_CODEGEN(...)
#endif
#if GRAVITY_OPCODE_DEBUG
	#define DEBUG_OPCODE(...)               PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_OPCODE(...)
#endif
#if GRAVITY_BYTECODE_DEBUG
	#define DEBUG_BYTECODE(...)             PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_BYTECODE(...)
#endif
#if GRAVITY_REGISTER_DEBUG
	#define DEBUG_REGISTER(...)             PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_REGISTER(...)
#endif
#if GRAVITY_FREE_DEBUG
	#define DEBUG_FREE(...)                 PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_FREE(...)
#endif
#if GRAVITY_DESERIALIZE_DEBUG
	#define DEBUG_DESERIALIZE(...)          PRINT_LINE(__VA_ARGS__)
#else
	#define DEBUG_DESERIALIZE(...)
#endif
#define DEBUG_ALWAYS(...)               PRINT_LINE(__VA_ARGS__)
//
//#include <gravity_memory.h>
#define GRAVITY_MEMORY_DEBUG            0 // memory debugger must be turned on ONLY with Xcode GuardMalloc ON
#ifndef GRAVITY_VM_DEFINED
	#define GRAVITY_VM_DEFINED
	//typedef struct gravity_vm                gravity_vm;
#endif
#if GRAVITY_MEMORY_DEBUG
	#define mem_init()                      memdebug_init()
	#define mem_stat()                      memdebug_stat()
	#define mem_alloc(_vm,_size)            memdebug_malloc0(_vm,_size)
	#define mem_calloc(_vm,_count,_size)    memdebug_calloc(_vm,_count,_size)
	#define mem_realloc(_vm,_ptr,_size)     memdebug_realloc(_vm,_ptr,_size)
	#define mem_free(v)                     memdebug_free((void *)v)
	#define mem_check(v)                    memdebug_setcheck(v)
	#define mem_status                      memdebug_status
	#define mem_leaks()                     memdebug_leaks()
	#define mem_remove                      memdebug_remove
#else
	#define mem_init()
	#define mem_stat()
	#define mem_alloc(_vm,_size)            gravity_calloc(_vm, 1, _size)
	#define mem_calloc(_vm,_count,_size)    gravity_calloc(_vm, _count, _size)
	#define mem_realloc(_vm,_ptr,_size)     gravity_realloc(_vm, _ptr, _size)
	#define mem_free(v)                     SAlloc::F((void *)v)
	#define mem_check(v)
	#define mem_status()                    0
	#define mem_leaks()                     0
	#define mem_remove(_v)
#endif
#if GRAVITY_MEMORY_DEBUG
	void   memdebug_init();
	void * memdebug_malloc(gravity_vm *vm, size_t size);
	void * memdebug_malloc0(gravity_vm *vm, size_t size);
	void * memdebug_calloc(gravity_vm *vm, size_t num, size_t size);
	void * memdebug_realloc(gravity_vm *vm, void *ptr, size_t new_size);
	void   memdebug_free(void *ptr);
	size_t  memdebug_leaks();
	size_t  memdebug_status();
	void    memdebug_setcheck(bool flag);
	void    memdebug_stat();
	bool    memdebug_remove(void *ptr);
#else
	void * FASTCALL gravity_calloc(gravity_vm *vm, size_t count, size_t size);
	void * FASTCALL gravity_realloc(gravity_vm *vm, void *ptr, size_t new_size);
#endif
//
//#include <gravity_utils.h>
#if defined(_WIN32)
	// @sobolev #include <windows.h>
	typedef unsigned __int64 nanotime_t;
	#define DIRREF HANDLE
#else
	#include <dirent.h>
	typedef uint64_t nanotime_t;
	#define DIRREF DIR*
#endif

// TIMER
nanotime_t  nanotime();
double      microtime(nanotime_t tstart, nanotime_t tend);
double      millitime(nanotime_t tstart, nanotime_t tend);
// CONSOLE
char        * readline(char * prompt, int * length);
// FILE
uint64_t    file_size(const char * path);
const char  * file_read(const char * path, size_t * len);
// @sobolev (replaced with SLIB fileExists(const char *)) bool file_exists(const char * path);
const char  * file_buildpath(const char * filename, const char * dirpath);
bool        file_write(const char * path, const char * buffer, size_t len);
// DIRECTORY
bool        is_directory(const char * path);
DIRREF      directory_init(const char * path);
// On Windows, you are expected to provied an output buffer of at least MAX_PATH in length
const char  * directory_read(DIRREF ref, char * out);
// STRING
int         string_nocasencmp(const char * s1, const char * s2, size_t n);
int         FASTCALL string_casencmp(const char * s1, const char * s2, size_t n);
//int         FASTCALL string_cmp(const char * s1, const char * s2);
//const char  * string_dup_(const char * s1);
const char  * string_ndup(const char * s1, size_t n);
void    string_reverse(char * p);
// @sobolev (replaced with sstrlen) uint32    string_size(const char * p);
char * string_strnstr(const char * s, const char * find, size_t slen);
char * string_replace(const char * str, const char * from, const char * to, size_t * rlen);
// UTF-8
uint32  FASTCALL utf8_charbytes(const char * s, uint32 i);
uint32  utf8_nbytes(uint32 n);
uint32  utf8_encode(char * buffer, uint32 value);
uint32  FASTCALL utf8_len(const char * s, uint32 nbytes);
bool    utf8_reverse(char * p);

// MATH and NUMBERS
uint32  power_of2_ceil(uint32 n);
int64_t number_from_hex(const char * s, uint32 len);
int64_t number_from_oct(const char * s, uint32 len);
int64_t number_from_bin(const char * s, uint32 len);
//
void Gravity_Implement_DebugAssert(const char * pFile, int line, const char * pFunc, const char * pMessage);
//
//#include <gravity_array.h>
// Inspired by https://github.com/attractivechaos/klib/blob/master/kvec.h
#define MARRAY_DEFAULT_SIZE         8

template <class T> class GravityArray {
public:
	GravityArray() : n(0), m(0), p(0)
	{
	}
	GravityArray & Z()
	{
		ZFREE(p);
		n = 0;
		m = 0;
		return *this;
	}
	int    resize(uint addendumLimit)
	{
		m += addendumLimit; 
		p = static_cast<T *>(SAlloc::R(p, sizeof(T) * m));
		if(p)
			return 1;
		else {
			m = 0;
			n = 0;
			return 0;
		}
	}
	T    & at(uint idx) const { assert(idx < n); return p[idx]; }
	void   FillUnusedEntries(const T & rValue)
	{
		for(uint i = n; i < m; i++) {
			assert(p);
			p[i] = rValue;
		}
	}
	T    & getLast() const { assert(n); return p[n-1]; }
	T    & pop()
	{ 
		//#define marray_pop(v)
		assert(n > 0); 
		return p[--n]; 
	}
	uint   getCount() const { return n; }
	int    insert(const T & rItem)
	{
		//marray_push(type, v, x)     
		int   ok = 1;
		if(n == m) { 
			m = m ? (m << 1) : MARRAY_DEFAULT_SIZE; 
			p = static_cast<T *>(SAlloc::R(p, sizeof(T) * m));
		} 
		p[n++] = rItem; 
		return ok;
	}
	uint32 n;
	uint32 m;
	T * p;
};

// @sobolev (replaced with GravityArray <type>) #define marray_t(type)              struct {uint32 n, m; type *p;} // @sobolev size_t-->uint32
#define marray_init(v)              ((v).n = (v).m = 0, (v).p = 0)
//#define marray_setnull(v, i)        ((v).p[(i)] = NULL)
//#define marray_nset(v,N)            ((v).n = N)
//#define marray_decl_init(_t,_v)     _t _v; marray_init(_v)
//#define marray_destroy(v)           if((v).p) SAlloc::F((v).p)
//#define marray_get(v, i)            ((v).p[(i)])
//#define marray_pop(v)               ((v).p[--(v).n])
//#define marray_last(v)              ((v).p[(v).n-1])
//#define marray_size(v)              ((v).n)
//#define marray_max(v)               ((v).m)
//#define marray_inc(v)               (++(v).n)
//#define marray_dec(v)               (--(v).n)
//#define marray_push(type, v, x)     { if((v).n == (v).m) { (v).m = (v).m ? (v).m<<1 : MARRAY_DEFAULT_SIZE; (v).p = (type *)SAlloc::R((v).p, sizeof(type) * (v).m);} (v).p[(v).n++] = (x); }
//#define marray_resize(type, v, n)   (v).m += n; (v).p = (type *)SAlloc::R((v).p, sizeof(type) * (v).m)
//#define marray_resize0(type, v, n)  (v).p = (type *)SAlloc::R((v).p, sizeof(type) * ((v).m+n)); (v).m ? memzero((v).p+(sizeof(type) * n), (sizeof(type) * n)) : memzero((v).p, (sizeof(type) * n)); (v).m += n
//#define marray_npop(v,k)            ((v).n -= k)
//#define marray_reset(v,k)           ((v).n = k)
//#define marray_reset0(v)            marray_reset(v, 0)
//#define marray_set(v,i,x)           (v).p[i] = (x)

// commonly used arrays
//typedef marray_t(uint16)       uint16_r;
//typedef marray_t(uint32)       uint32_r;
//typedef marray_t(void *)       void_r;
//typedef marray_t(const char *) cstring_r;
//typedef GravityArray <uint16> uint16_r;
//typedef GravityArray <uint32> uint32_r;
//typedef GravityArray <void *> void_r;
//typedef GravityArray <const char *> cstring_r;
//
//#include <gravity_json.h>
#ifndef __GRAVITY_JSON_SERIALIZER__
#define __GRAVITY_JSON_SERIALIZER__
// MARK: JSON serializer -

enum json_opt_mask {
	json_opt_none           =   0x00,
	json_opt_need_comma     =   0x01,
	json_opt_prettify       =   0x02,
	json_opt_no_maptype     =   0x04,
	json_opt_no_undef       =   0x08,
	json_opt_unused_1       =   0x10,
	json_opt_unused_2       =   0x20,
	json_opt_unused_3       =   0x40,
	json_opt_unused_4       =   0x80,
	json_opt_unused_5       =   0x100,
};

typedef struct GravityJson GravityJson;
GravityJson * json_new();
void   json_free(GravityJson * json);
void   json_begin_object(GravityJson * json, const char * key);
void   json_end_object(GravityJson * json);
void   json_begin_array(GravityJson * json, const char * key);
void   json_end_array(GravityJson * json);
void   json_add_cstring(GravityJson * json, const char * key, const char * value);
void   json_add_string(GravityJson * json, const char * key, const char * value, size_t len);
void   json_add_int(GravityJson * json, const char * key, int64_t value);
void   json_add_double(GravityJson * json, const char * key, double value);
void   json_add_bool(GravityJson * json, const char * key, bool value);
void   json_add_null(GravityJson * json, const char * key);
void   json_set_label(GravityJson * json, const char * key);
const char  * json_get_label(GravityJson * json, const char * key);
char * json_buffer(GravityJson * json, size_t * len);
bool   json_write_file(GravityJson * json, const char * path);
uint32 json_get_options(const GravityJson * json);
void   json_set_option(GravityJson * json, json_opt_mask option_value);
void   json_clear_option(GravityJson * json, json_opt_mask option_value);
bool   json_option_isset(const GravityJson * json, json_opt_mask option_value);
#endif

// MARK: - JSON Parser -
/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2012, 2013, 2014 James McLaughlin et al.  All rights reserved.
 * https://github.com/udp/json-parser
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef json_char
   #define json_char char
#endif
#ifndef json_int_t
   #ifndef _MSC_VER
      #define json_int_t int64_t
   #else
      #define json_int_t __int64
   #endif
#endif
//#ifdef __cplusplus
//extern "C" {
//#endif
struct json_settings {
	json_settings() : max_memory(0), settings(0), memory_alloc(0), memory_free(0), user_data(0), value_extra(0)
	{
	}
	ulong max_memory;
	int settings;
	// Custom allocator support (leave null to use malloc/free)
	void * (*memory_alloc)(size_t, int zero, void * user_data);
	void (* memory_free)(void *, void * user_data);
	void * user_data; /* will be passed to mem_alloc and mem_free */
	size_t value_extra; /* how much extra space to allocate for values? */
};

#define json_enable_comments  0x01

enum json_type {
	json_none,
	json_object,
	json_array,
	json_integer,
	json_double,
	json_string,
	json_boolean,
	json_null
};

struct json_value;
extern const json_value json_value_none;

struct json_object_entry {
	json_char * name;
	uint name_length;
	json_value * value;
};

struct json_value {
	json_value * parent;
	json_type type;
	union {
		int boolean;
		json_int_t integer;
		double dbl;

		struct {
			uint length;
			json_char * ptr; /* null terminated */
		} string;

		struct {
			uint length;
			json_object_entry * values;
	 #if defined(__cplusplus) && __cplusplus >= 201103L
			decltype(values)begin() const {  return values;}
			decltype(values)end() const {  return values + length;}
	 #endif
		} object;

		struct {
			uint length;
			json_value ** values;
	 #if defined(__cplusplus) && __cplusplus >= 201103L
			decltype(values)begin() const {  return values;}
			decltype(values)end() const {  return values + length;}
	 #endif
		} array;
	} u;

	union {
		json_value * next_alloc;
		void * object_mem;
	} _reserved;
   #ifdef JSON_TRACK_SOURCE
	uint line, col; // Location of the value in the source JSON
   #endif
	/* Some C++ operator sugar */
   #ifdef __cplusplus
public:
	inline json_value()
	{
		memzero(this, sizeof(json_value));
	}
	inline const json_value & operator [](int index) const 
	{
		if(type != json_array || index < 0 || ((uint)index) >= u.array.length) {
			return json_value_none;
		}
		return *u.array.values [index];
	}
	inline const json_value & operator [](const char * index) const 
	{
		if(type != json_object)
			return json_value_none;
		for(uint i = 0; i < u.object.length; ++i)
			if(sstreq(u.object.values[i].name, index))
				return *u.object.values[i].value;
		return json_value_none;
	}
	inline operator const char *() const
	{
		switch(type) {
			case json_string: return u.string.ptr;
			default: return "";
		};
	}
	inline operator json_int_t() const
	{
		switch(type) {
			case json_integer: return u.integer;
			case json_double: return (json_int_t)u.dbl;
			default: return 0;
		};
	}
	inline operator bool() const
	{
		return (type != json_boolean) ? false : (u.boolean != 0);
	}
	inline operator double() const {
		switch(type) {
			case json_integer: return (double)u.integer;
			case json_double: return u.dbl;
			default: return 0;
		};
	}

   #endif
};

// @ctr #define EMPTY_SETTINGS_STRUCT    {0, 0, 0, 0, 0, 0}
// @ctr #define EMPTY_STATE_STRUCT        {0, 0, 0, EMPTY_SETTINGS_STRUCT, 0, 0, 0, 0}

json_value * json_parse(const json_char * json, size_t length);
#define json_error_max 128
json_value * json_parse_ex(json_settings * settings, const json_char * json, size_t length, char * error);
void json_value_free(json_value *);
//
// Not usually necessary, unless you used a custom mem_alloc and now want to use a custom mem_free.
//
void json_value_free_ex(json_settings * settings, json_value *);

//#ifdef __cplusplus
//}
//#endif
//
//
//#include <gravity_value.h>
// Gravity is a dynamically typed language so a variable (GravityValue) can hold a value of any type.
//
// The representation of values in a dynamically typed language is very important since it can lead to a big
// difference in terms of performance. Such representation has several constraints:
// - fast access
// - must represent several kind of values
// - be able to cope with the garbage collector
// - low memory overhead (when allocating a lot of small values)
//
// In modern 64bit processor with OS that always returns aligned allocated memory blocks that means that each ptr is 8
// bytes.
// That means that passing a value as an argument or storing it involves copying these bytes around (requiring 2/4
// machine words).
// Values are not pointers but structures.
//
// The built-in types for booleans, numbers, floats, null, undefs are unboxed: their value is stored directly into
// GravityValue.
// Other types like classes, instances, functions, lists, and strings are all reference types. They are stored on the
// heap and
// the GravityValue just stores a pointer to it.
//
// So each value is a pointer to a FIXED size block of memory (16 bytes). Having all values of the same size greatly
// reduce the complexity of a memory pool and since allocating a large amount of values is very common is a dynamically typed language like
// Gravity.
// In a future update I could introduce NaN tagging and squeeze value size to 8 bytes (that would mean nearly double performance).
//
// Internal settings to set integer and float size.
// Default is to have both int and float as 64bit.
//
// In a 64bit OS:
// sizeof(float)        => 4 bytes
// sizeof(double)       => 8 bytes
// sizeof(void*)        => 8 bytes
// sizeof(int64_t)      => 8 bytes
//
// sizeof various structs in a 64bit OS:
// STRUCT                       BYTES
// ======                       =====
// gravity_function_t           104
// GravityValue              16
// gravity_upvalue_t            56
// gravity_closure_t            40
// gravity_list_t               48
// gravity_map_t                32
// gravity_callframe_t          48
// gravity_fiber_t              112
// gravity_class_t              88
// gravity_module_t             40
// gravity_instance_t           40
// gravity_string_t             48
// gravity_range_t              40

//#ifdef __cplusplus
//extern "C" {
//#endif

class GravityGlobals {
public:
	GravityGlobals()
	{
		THISZERO();
	}
	gravity_class_t * P_ClsObj;   //gravity_class_object_;
	gravity_class_t * P_ClsBool;  //gravity_class_bool_;
	gravity_class_t * P_ClsNull;  //gravity_class_null_;
	gravity_class_t * P_ClsInt;   //gravity_class_int_;
	gravity_class_t * P_ClsFloat; //gravity_class_float_;
	gravity_class_t * P_ClsFunc;  //gravity_class_function_;
	gravity_class_t * P_ClsClosure; //gravity_class_closure_;
	gravity_class_t * P_ClsFiber;   //gravity_class_fiber_;
	gravity_class_t * P_ClsClass;   //gravity_class_class_;
	gravity_class_t * P_ClsString;  //gravity_class_string_;
	gravity_class_t * P_ClsInstance; //gravity_class_instance_;
	gravity_class_t * P_ClsList;     //gravity_class_list_;
	gravity_class_t * P_ClsMap;      //gravity_class_map_;
	gravity_class_t * P_ClsModule;   //gravity_class_module_;
	gravity_class_t * P_ClsRange;    //gravity_class_range_;
	gravity_class_t * P_ClsUpValue;  //gravity_class_upvalue_;
	gravity_class_t * P_ClsSystem;   //gravity_class_system_;
};

extern GravityGlobals GravityEnv;

//
// Everything inside Gravity VM is a GravityValue struct
//
struct GravityValueBase {
	GravityValueBase() : isa(0)
	{
	}
	GravityValueBase(gravity_class_t * pCls) : isa(pCls)
	{
	}
	bool   operator !() { return (isa == 0); }
	bool   IsInt() const { return (isa == GravityEnv.P_ClsInt); }
	bool   IsFloat() const { return (isa == GravityEnv.P_ClsFloat); }
	bool   IsBool() const { return (isa == GravityEnv.P_ClsBool); }
	bool   IsString() const { return (isa == GravityEnv.P_ClsString); }
	bool   IsFunction() const { return (isa == GravityEnv.P_ClsFunc); }
	bool   IsRange() const { return (isa == GravityEnv.P_ClsRange); }
	bool   IsClass() const { return (isa == GravityEnv.P_ClsClass); }
	bool   IsClosure() const { return (isa == GravityEnv.P_ClsClosure); }
	bool   IsFiber() const { return (isa == GravityEnv.P_ClsFiber); }
	bool   IsMap() const { return (isa == GravityEnv.P_ClsMap); }
	bool   IsList() const { return (isa == GravityEnv.P_ClsList); }
	bool   IsInstance() const { return (isa == GravityEnv.P_ClsInstance); }
	bool   IsUpValue() const { return (isa == GravityEnv.P_ClsUpValue); }
	bool   IsModule() const { return (isa == GravityEnv.P_ClsModule); }
	bool   IsNullClass() const { return (isa == GravityEnv.P_ClsNull); }
	gravity_class_t * isa;    // EVERY object must have an ISA pointer (8 bytes on a 64bit system)
};

/*struct gravity_gc_t {
	gravity_gc_t() : GcIsDark(false), GcVisited(false), P_GcNext(0)
	{
	}
	bool   GcIsDark;  // flag to check if object is reachable
	bool   GcVisited; // flag to check if object has already been counted in memory size
	uint8  GcReserve[2]; // @alignment
	gravity_class_t * P_GcNext; // to track next object in the linked list
};*/

class GravityObjectBase : public GravityValueBase /*to be an object*/ { // @construction
public:
	GravityObjectBase() : Flags(0) /*GcIsDark(false), GcVisited(false)*/, P_GcNext(0)
	{
	}
	GravityObjectBase(gravity_class_t * pIsA) : GravityValueBase(pIsA), Flags(0)/*GcIsDark(false), GcVisited(false)*/, P_GcNext(0)
	{
	}
	enum {
		fGcIsDark         = 0x0001, // flag to check if object is reachable
		fGcVisited        = 0x0002, // flag to check if object has already been counted in memory size
		fHasOuter         = 0x0004, // flag used to automatically set ivar 0 to outer class (if any)
		fIsStruct         = 0x0008, // flag to mark class as a struct
		fIsInited         = 0x0010, // flag used to mark already init meta-classes (to be improved)
		fFiberTrying      = 0x0020  // set when the try flag is set by the user
	};
	//gravity_gc_t gc; // to be collectable by the garbage collector
	//bool   GcIsDark;  // flag to check if object is reachable
	//bool   GcVisited; // flag to check if object has already been counted in memory size
	//uint8  GcReserve[2]; // @alignment
	uint32 Flags;
	gravity_class_t * P_GcNext; // to track next object in the linked list
};

struct gravity_class_t : public GravityObjectBase {
	gravity_class_t * objclass; // meta class
	const char * identifier; // class name
	//bool has_outer; // flag used to automatically set ivar 0 to outer class (if any)
	//bool is_struct; // flag to mark class as a struct
	//bool is_inited; // flag used to mark already init meta-classes (to be improved)
	//bool unused;    // unused padding byte
	void * xdata;   // extra bridged data
	gravity_class_t * superclass;// reference to the super class
	const char * superlook;// when a superclass is set to extern a runtime lookup must be performed
	gravity_hash_t * htable;   // hash table
	uint32 nivars; // number of instance variables
	//gravity_value_r inames; // ivar names
	GravityValue * ivars;    // static variables
};

struct GravityValue : public GravityValueBase {
	GravityValue() : GravityValueBase()
	{
		n = 0;
	}
	GravityValue(gravity_int_t x) : GravityValueBase(GravityEnv.P_ClsInt) { n = x; }
	GravityValue(gravity_float_t x) : GravityValueBase(GravityEnv.P_ClsFloat) { f = x; }
	GravityValue(gravity_class_t * pCls, gravity_int_t x) : GravityValueBase(pCls) { n = x; }
	GravityValue(gravity_class_t * pCls, gravity_class_t * pObj) : GravityValueBase(pCls) { Ptr = pObj; }
	bool   IsObject() const
	{
		// was:
		// if (!v) return false;
		// if (v.IsInt()) return false;
		// if (v.IsFloat()) return false;
		// if (v.IsBool()) return false;
		// if (VALUE_ISA_NULL(v)) return false;
		// if (VALUE_ISA_UNDEFINED(v)) return false;
		// return true;
		if(!isa || oneof4(isa, GravityEnv.P_ClsInt, GravityEnv.P_ClsFloat, GravityEnv.P_ClsBool, GravityEnv.P_ClsNull) || !Ptr) 
			return false;
		// extra check to allow ONLY known objects
		else if(oneof12(isa, GravityEnv.P_ClsString, GravityEnv.P_ClsObj, GravityEnv.P_ClsFunc, GravityEnv.P_ClsClosure,
			GravityEnv.P_ClsFiber, GravityEnv.P_ClsClass, GravityEnv.P_ClsInstance, GravityEnv.P_ClsModule, GravityEnv.P_ClsList, 
			GravityEnv.P_ClsMap, GravityEnv.P_ClsRange, GravityEnv.P_ClsUpValue)) 
			return true;
		else
			return false;
	}
	bool   IsNull() const { return (IsNullClass() && (n == 0)); }
	bool   IsUndefined() const { return (IsNullClass() && (n == 1)); }
	//#define VALUE_ISA_NULL(v)                   ((v.isa == GravityEnv.P_ClsNull) && (v.n == 0))
	//#define VALUE_ISA_UNDEFINED(v)              ((v.isa == GravityEnv.P_ClsNull) && (v.n == 1))
	gravity_class_t * GetClass() 
	{
		if((isa == GravityEnv.P_ClsClass) && (Ptr->objclass == GravityEnv.P_ClsObj)) 
			return static_cast<gravity_class_t *>(Ptr);
		else if(oneof2(isa, GravityEnv.P_ClsInstance, GravityEnv.P_ClsClass)) 
			return Ptr ? Ptr->objclass : NULL;
		else
			return isa;
	}

	static GravityValue FASTCALL from_error(gravity_class_t * pMsg) { return GravityValue(static_cast<gravity_class_t *>(0), pMsg); }
	static GravityValue FASTCALL from_object(gravity_class_t * pObj) { return GravityValue(pObj->isa, pObj); }
	static GravityValue FASTCALL from_closure(gravity_closure_t * pClosure) { return from_object(reinterpret_cast<gravity_class_t *>(pClosure)); }
	static GravityValue FASTCALL from_int(gravity_int_t x) { return GravityValue(x); }
	static GravityValue FASTCALL from_float(gravity_float_t x) { return GravityValue(x); }
	static GravityValue from_null() { return GravityValue(GravityEnv.P_ClsNull, static_cast<gravity_int_t>(0)); }
	static GravityValue from_undefined() { return GravityValue(GravityEnv.P_ClsNull, 1); }
	static GravityValue FASTCALL from_bool(gravity_int_t x) { return GravityValue(GravityEnv.P_ClsBool, (x) ? 1 : 0); }
	static GravityValue FASTCALL from_node(gravity_class_t * x) { return GravityValue(static_cast<gravity_class_t *>(0), x); }
	// @sobolev(moved to GravityValueBase) gravity_class_t * isa;    // EVERY object must have an ISA pointer (8 bytes on a 64bit system)
	operator gravity_string_t * () { return reinterpret_cast<gravity_string_t *>(Ptr); }
	operator gravity_class_t  * () { return reinterpret_cast<gravity_class_t *>(Ptr); }
	operator gravity_list_t  * () { return reinterpret_cast<gravity_list_t *>(Ptr); }
	const char * GetZString();
	gravity_int_t GetInt() const { return n; }
	gravity_float_t GetFloat() const { return f; }
	union {                   // union takes 8 bytes on a 64bit system
		gravity_int_t n;      // integer slot
		gravity_float_t f;    // float/double slot
		gravity_class_t * Ptr; // ptr to object slot
	};
};

// All VM shares the same foundation classes

//typedef marray_t(GravityValue)        gravity_value_r;   // array of values
typedef GravityArray <GravityValue> gravity_value_r; // array of values

#ifndef GRAVITY_HASH_DEFINED
	#define GRAVITY_HASH_DEFINED
	typedef struct gravity_hash_t gravity_hash_t;               // forward declaration
#endif
#ifndef GRAVITY_VM_DEFINED
	#define GRAVITY_VM_DEFINED
	typedef struct gravity_vm gravity_vm;                       // vm is an opaque data type
#endif

typedef bool (* gravity_c_internal)(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex);

enum gravity_special_index {
	EXEC_TYPE_SPECIAL_GETTER = 0,   // index inside special gravity_function_t union to represent getter func
	EXEC_TYPE_SPECIAL_SETTER = 1,   // index inside special gravity_function_t union to represent setter func
};

enum gravity_exec_type {
	EXEC_TYPE_NATIVE,       // native gravity code (can change stack)
	EXEC_TYPE_INTERNAL,     // c internal code (can change stack)
	EXEC_TYPE_BRIDGED,      // external code to be executed by delegate (can change stack)
	EXEC_TYPE_SPECIAL       // special execution like getter and setter (can be NATIVE, INTERNAL)
};

struct gravity_function_t : public GravityObjectBase {
	gravity_function_t(uint16 paramCount, uint16 localCount, uint16 tempCount) : 
		GravityObjectBase(GravityEnv.P_ClsFunc), xdata(0), identifier(0), nparams(paramCount), nlocals(localCount), ntemps(tempCount),
		nupvalues(0), tag(EXEC_TYPE_NATIVE)
	{
		MEMSZERO(U);
	}
	void * xdata; // extra bridged data
	const char * identifier;// function name
	uint16 nparams;                   // number of formal parameters
	uint16 nlocals;                   // number of local variables
	uint16 ntemps;                    // number of temporary values used
	uint16 nupvalues;                 // number of up values (if any)
	gravity_exec_type tag;              // can be EXEC_TYPE_NATIVE (default), EXEC_TYPE_INTERNAL, EXEC_TYPE_BRIDGED or EXEC_TYPE_SPECIAL
	// tag == EXEC_TYPE_NATIVE
	struct NativeFunc {
		NativeFunc() : ninsts(0), bytecode(0), lineno(0), purity(0.0f), useargs(false)
		{
		}
		gravity_value_r cpool; // constant pool
		gravity_value_r pvalue; // default param value
		gravity_value_r pname; // param names
		uint32 ninsts;      // number of instructions in the bytecode
		uint32 * bytecode;  // bytecode as array of 32bit values
		uint32 * lineno;    // debug: line number <-> current instruction relation
		float  purity;      // experimental value
		bool   useargs;     // flag set by the compiler to optimize the creation of the arguments array only if needed
		uint8  Reserve[3];  // @alignment
	};
	// tag == EXEC_TYPE_SPECIAL
	struct SpecialFunc {
		SpecialFunc() : index(0)
		{
			special[0] = 0;
			special[1] = 0;
		}
		uint16 index;      // property index to speed-up default getter and setter
		uint16 Reserve2;   // @alignment
		void * special[2]; // getter/setter functions
	};
	union tagU {
		tagU() : internal(0)
		{
		}
		NativeFunc Nf; // tag == EXEC_TYPE_NATIVE
		gravity_c_internal internal; // function callback // tag == EXEC_TYPE_INTERNAL
		SpecialFunc Sf; // tag == EXEC_TYPE_SPECIAL
	} U;
};

struct gravity_upvalue_t : public GravityObjectBase {
	gravity_upvalue_t(GravityValue * pValue) : GravityObjectBase(GravityEnv.P_ClsUpValue), value(pValue), closed(GravityValue::from_null()), next(0)
	{
	}
	GravityValue * value;     // ptr to open value on the stack or to closed value on this struct
	GravityValue closed;      // copy of the value once has been closed
	gravity_upvalue_t * next; // ptr to the next open upvalue
};

struct gravity_closure_t : public GravityObjectBase {
	gravity_closure_t() : GravityObjectBase(GravityEnv.P_ClsClosure), f(0), context(0), upvalue(0), refcount(0)
	{
	}
	gravity_function_t * f;        // function prototype
	gravity_class_t  * context;  // context where the closure has been created (or object bound by the user)
	gravity_upvalue_t ** upvalue; // upvalue array
	uint32 refcount;                  // bridge language sometimes needs to protect closures from GC
};

struct gravity_list_t : public GravityObjectBase {
	gravity_list_t() : GravityObjectBase(GravityEnv.P_ClsList)
	{
	}
	gravity_value_r array;      // dynamic array of values
};

struct gravity_map_t : public GravityObjectBase {
	gravity_hash_t * hash;     // hash table
};

// Call frame used for function call
struct gravity_callframe_t {
	uint32 * ip;       // instruction pointer
	uint32 dest;       // destination register that will receive result
	uint16 nargs;      // number of effective arguments passed to the function
	bool   outloop;    // special case for events or native code executed from C that must be executed separately
	uint8  Reserve;    // @alignment
	gravity_list_t    * args;     // implicit special _args array
	gravity_closure_t * closure;  // closure being executed
	GravityValue   * stackstart; // first stack slot used by this call frame (receiver, plus parameters, locals and temporaries)
};

enum gravity_fiber_status {
	FIBER_NEVER_EXECUTED = 0,
	FIBER_ABORTED_WITH_ERROR = 1,
	FIBER_TERMINATED = 2,
	FIBER_RUNNING = 3,
	FIBER_TRYING = 4
};
//
// Fiber is the core executable model
//
struct gravity_fiber_t : public GravityObjectBase {
	gravity_fiber_t() : GravityObjectBase(GravityEnv.P_ClsFiber), stack(0), stacktop(0), stackalloc(0), frames(0), nframes(0), framesalloc(0),
		upvalues(0), error(0), /*trying(0),*/ caller(0), status(FIBER_NEVER_EXECUTED), lasttime(0), timewait(0), elapsedtime(0),
		result(GravityValue::from_null())
	{
	}
	~gravity_fiber_t()
	{
		mem_free(error);
		mem_free(stack);
		mem_free(frames);
	}
	gravity_fiber_t & Z()
	{
		caller = NULL;
		result = GravityValue::from_null();
		nframes = 0;
		upvalues = NULL;
		stacktop = stack;
		return *this;
	}
	GravityValue * stack;    // stack buffer (grown as needed and it holds locals and temps)
	GravityValue * stacktop; // current stack ptr
	uint32 stackalloc;                // number of allocated values
	gravity_callframe_t * frames;   // callframes buffer (grown as needed but never shrinks)
	uint32 nframes;                   // number of frames currently in use
	uint32 framesalloc;               // number of allocated frames
	gravity_upvalue_t * upvalues; // linked list used to keep track of open upvalues
	char * error;    // runtime error message
	//bool   trying;              // set when the try flag is set by the user
	//uint8  Reserve[3];          // @alignment
	gravity_fiber_t * caller;   // optional caller fiber
	GravityValue result;        // end result of the fiber
	gravity_fiber_status status;        // Fiber status (see enum)
	nanotime_t lasttime;                // last time Fiber has been called
	gravity_float_t timewait;           // used in yieldTime
	gravity_float_t elapsedtime;        // time passed since last execution
};

struct gravity_module_t : public GravityObjectBase {
	const char * identifier;// module name
	gravity_hash_t * htable;   // hash table
};

struct gravity_instance_t : public GravityObjectBase {
	gravity_class_t * objclass; // real instance class
	void * xdata;               // extra bridged data
	GravityValue * ivars;    // instance variables
};

struct gravity_string_t : public GravityObjectBase {
	gravity_string_t() : GravityObjectBase(GravityEnv.P_ClsString), P_StrBuf(0), hash(0), len(0), alloc(0)
	{
	}
	gravity_string_t(char * pS, uint32 l) : GravityObjectBase(GravityEnv.P_ClsString), P_StrBuf(pS), hash(0), len(l), alloc(0)
	{
	}
	const char * cptr() const { return P_StrBuf; }
	char * P_StrBuf/*s*/;        // pointer to NULL terminated string
	uint32 hash;     // string hash (type to be keept in sync with gravity_hash_size_t)
	uint32 len;      // actual string length
	uint32 alloc;    // bytes allocated for string
};

struct gravity_range_t : public GravityObjectBase {
	gravity_int_t from; // range start
	gravity_int_t to;   // range end
};

typedef void (* code_dump_function)(const void * code);
// 
// Returns: 
//   ALLWAYS false
//
//bool gravity_return_error(gravity_vm * vm, uint32 rindex, const char * pMessage);
bool gravity_return_errorv(gravity_vm * vm, uint32 rindex, const char * pFormat, ...);

// MARK: - MODULE -
GRAVITY_API gravity_module_t * gravity_module_new(gravity_vm * vm, const char * identifier);
GRAVITY_API void gravity_module_free(gravity_vm * vm, gravity_module_t * m);
GRAVITY_API void gravity_module_blacken(gravity_vm * vm, gravity_module_t * m);
GRAVITY_API uint32 gravity_module_size(gravity_vm * vm, gravity_module_t * m);

// MARK: - FUNCTION -
GRAVITY_API gravity_function_t  * gravity_function_new(gravity_vm * vm, const char * identifier, uint16 nparams, uint16 nlocals, uint16 ntemps, void * code);
GRAVITY_API gravity_function_t  * gravity_function_new_internal(gravity_vm * vm, const char * identifier, gravity_c_internal exec, uint16 nparams);
GRAVITY_API gravity_function_t  * gravity_function_new_special(gravity_vm * vm, const char * identifier, uint16 index, void * getter, void * setter);
GRAVITY_API gravity_function_t  * gravity_function_new_bridged(gravity_vm * vm, const char * identifier, void * xdata);
GRAVITY_API uint16 FASTCALL gravity_function_cpool_add(gravity_vm * vm, gravity_function_t * f, GravityValue v);
GRAVITY_API GravityValue     gravity_function_cpool_get(gravity_function_t * f, uint16 i);
GRAVITY_API void   gravity_function_dump(gravity_function_t * f, code_dump_function codef);
GRAVITY_API void   gravity_function_setouter(gravity_function_t * f, gravity_class_t * outer);
GRAVITY_API void   gravity_function_setxdata(gravity_function_t * f, void * xdata);
GRAVITY_API gravity_list_t * gravity_function_params_get(gravity_vm * vm, gravity_function_t * f);
GRAVITY_API void   gravity_function_serialize(gravity_function_t * f, GravityJson * json);
GRAVITY_API uint32 * gravity_bytecode_deserialize(const char * buffer, size_t len, uint32 * ninst);
GRAVITY_API gravity_function_t  * gravity_function_deserialize(gravity_vm * vm, json_value * json);
GRAVITY_API void   gravity_function_free(gravity_vm * vm, gravity_function_t * f);
GRAVITY_API void   gravity_function_blacken(gravity_vm * vm, gravity_function_t * f);
GRAVITY_API uint32 gravity_function_size(gravity_vm * vm, gravity_function_t * f);

// MARK: - CLOSURE -
GRAVITY_API gravity_closure_t * FASTCALL gravity_closure_new(gravity_vm * vm, gravity_function_t * f);
GRAVITY_API void FASTCALL gravity_closure_free(gravity_vm * vm, gravity_closure_t * closure);
GRAVITY_API uint32 gravity_closure_size(gravity_vm * vm, gravity_closure_t * closure);
GRAVITY_API void   gravity_closure_inc_refcount(gravity_vm * vm, gravity_closure_t * closure);
GRAVITY_API void   gravity_closure_dec_refcount(gravity_vm * vm, gravity_closure_t * closure);
GRAVITY_API void   gravity_closure_blacken(gravity_vm * vm, gravity_closure_t * closure);

// MARK: - UPVALUE -
GRAVITY_API gravity_upvalue_t * gravity_upvalue_new(gravity_vm * vm, GravityValue * value);
GRAVITY_API uint32 gravity_upvalue_size(gravity_vm * vm, gravity_upvalue_t * upvalue);
GRAVITY_API void   gravity_upvalue_blacken(gravity_vm * vm, gravity_upvalue_t * upvalue);
GRAVITY_API void   gravity_upvalue_free(gravity_vm * vm, gravity_upvalue_t * upvalue);

// MARK: - CLASS -
GRAVITY_API void FASTCALL gravity_class_bind(gravity_class_t * c, const char * key, GravityValue value);
GRAVITY_API void FASTCALL gravity_class_bind_outerproc(gravity_class_t * c, const char * key, gravity_c_internal proc);
GRAVITY_API void FASTCALL gravity_class_bind_property_outerproc(gravity_class_t * c, const char * key, gravity_c_internal getProc, gravity_c_internal setProc);
GRAVITY_API gravity_class_t * gravity_class_getsuper(gravity_class_t * c);
GRAVITY_API bool   gravity_class_grow(gravity_class_t * c, uint32 n);
GRAVITY_API bool   gravity_class_setsuper(gravity_class_t * subclass, gravity_class_t * superclass);
GRAVITY_API bool   gravity_class_setsuper_extern(gravity_class_t * baseclass, const char * identifier);
GRAVITY_API gravity_class_t * FASTCALL gravity_class_new_single(gravity_vm * vm, const char * identifier, uint32 nfields);
GRAVITY_API gravity_class_t * FASTCALL gravity_class_new_pair(gravity_vm * vm, const char * identifier, gravity_class_t * superclass, uint32 nivar, uint32 nsvar);
GRAVITY_API const  gravity_class_t * FASTCALL gravity_class_get_meta_const(const gravity_class_t * c);
GRAVITY_API gravity_class_t * FASTCALL gravity_class_get_meta(gravity_class_t * c);
GRAVITY_API bool   gravity_class_is_meta(const gravity_class_t * c);
GRAVITY_API bool   gravity_class_is_anon(const gravity_class_t * c);
GRAVITY_API uint32 gravity_class_count_ivars(const gravity_class_t * c);
GRAVITY_API void   gravity_class_dump(gravity_class_t * c);
GRAVITY_API void   gravity_class_setxdata(gravity_class_t * c, void * xdata);
GRAVITY_API int16  gravity_class_add_ivar(gravity_class_t * c, const char * identifier);
GRAVITY_API void   gravity_class_serialize(gravity_class_t * c, GravityJson * json);
GRAVITY_API gravity_class_t     * gravity_class_deserialize(gravity_vm * vm, json_value * json);
GRAVITY_API void   gravity_class_free(gravity_vm * vm, gravity_class_t * c);
GRAVITY_API void   gravity_class_free_core(gravity_vm * vm, gravity_class_t * c);
GRAVITY_API gravity_class_t  * FASTCALL gravity_class_lookup(gravity_class_t * c, GravityValue key);
GRAVITY_API gravity_closure_t * FASTCALL gravity_class_lookup_closure(gravity_class_t * c, GravityValue key);
GRAVITY_API gravity_closure_t * gravity_class_lookup_constructor(gravity_class_t * c, uint32 nparams);
GRAVITY_API gravity_class_t   * gravity_class_lookup_class_identifier(gravity_class_t * c, const char * identifier);
GRAVITY_API void   gravity_class_blacken(gravity_vm * vm, gravity_class_t * c);
GRAVITY_API uint32 gravity_class_size(gravity_vm * vm, gravity_class_t * c);

// MARK: - FIBER -
GRAVITY_API gravity_fiber_t     * gravity_fiber_new(gravity_vm * vm, gravity_closure_t * closure, uint32 nstack, uint32 nframes);
GRAVITY_API void gravity_fiber_reassign(gravity_fiber_t * fiber, gravity_closure_t * closure, uint16 nargs);
GRAVITY_API void FASTCALL gravity_fiber_seterror(gravity_fiber_t * fiber, const char * error);
//GRAVITY_API void gravity_fiber_reset(gravity_fiber_t * fiber);
GRAVITY_API void gravity_fiber_free(gravity_vm * vm, gravity_fiber_t * fiber);
GRAVITY_API void gravity_fiber_blacken(gravity_vm * vm, gravity_fiber_t * fiber);
GRAVITY_API uint32 gravity_fiber_size(gravity_vm * vm, gravity_fiber_t * fiber);

// MARK: - INSTANCE -
GRAVITY_API gravity_instance_t  * gravity_instance_new(gravity_vm * vm, gravity_class_t * c);
GRAVITY_API gravity_instance_t  * gravity_instance_clone(gravity_vm * vm, gravity_instance_t * src_instance);
GRAVITY_API void   gravity_instance_setivar(gravity_instance_t * instance, uint32 idx, GravityValue value);
GRAVITY_API void   gravity_instance_setxdata(gravity_instance_t * i, void * xdata);
GRAVITY_API void   gravity_instance_free(gravity_vm * vm, gravity_instance_t * i);
GRAVITY_API gravity_closure_t   * gravity_instance_lookup_event(gravity_instance_t * i, const char * name);
GRAVITY_API void   gravity_instance_blacken(gravity_vm * vm, gravity_instance_t * i);
GRAVITY_API uint32 gravity_instance_size(gravity_vm * vm, gravity_instance_t * i);
GRAVITY_API void   gravity_instance_serialize(gravity_instance_t * i, GravityJson * json);
GRAVITY_API bool   FASTCALL gravity_instance_isstruct(const gravity_instance_t * i);

// MARK: - VALUE -
GRAVITY_API bool   gravity_value_equals(GravityValue v1, GravityValue v2);
GRAVITY_API bool   gravity_value_vm_equals(gravity_vm * vm, GravityValue v1, GravityValue v2);
GRAVITY_API uint32 gravity_value_hash(GravityValue value);
// @sobolev replaced with GetClass() GRAVITY_API gravity_class_t * gravity_value_getclass(GravityValue v);
GRAVITY_API gravity_class_t * gravity_value_getsuper(GravityValue v);
GRAVITY_API void   FASTCALL gravity_value_free(gravity_vm * vm, GravityValue & rV);
GRAVITY_API void   gravity_value_serialize(const char * key, GravityValue v, GravityJson * json);
GRAVITY_API void   gravity_value_dump(gravity_vm * vm, GravityValue v, char * buffer, uint16 len);
// @sobolev replaced with GravityValue::IsObject() GRAVITY_API bool FASTCALL gravity_value_isobject(const GravityValue v);
GRAVITY_API void   * gravity_value_xdata(GravityValue value);
GRAVITY_API const char * gravity_value_name(GravityValue value);
GRAVITY_API void   gravity_value_blacken(gravity_vm * vm, GravityValue v);
GRAVITY_API uint32 gravity_value_size(gravity_vm * vm, GravityValue v);

// MARK: - OBJECT -
GRAVITY_API void   gravity_object_serialize(gravity_class_t * obj, GravityJson * json);
GRAVITY_API gravity_class_t * gravity_object_deserialize(gravity_vm * vm, json_value * entry);
GRAVITY_API void   gravity_object_free(gravity_vm * vm, gravity_class_t * obj);
GRAVITY_API void   gravity_object_blacken(gravity_vm * vm, gravity_class_t * obj);
GRAVITY_API uint32 gravity_object_size(gravity_vm * vm, gravity_class_t * obj);
GRAVITY_API const char * gravity_object_debug(gravity_class_t * obj, bool is_free);

// MARK: - LIST -
GRAVITY_API gravity_list_t * FASTCALL gravity_list_new(gravity_vm * vm, uint32 n);
GRAVITY_API gravity_list_t * gravity_list_from_array(gravity_vm * vm, uint32 n, GravityValue * p);
GRAVITY_API void   gravity_list_free(gravity_vm * vm, gravity_list_t * list);
GRAVITY_API void   gravity_list_append_list(gravity_vm * vm, gravity_list_t * list1, gravity_list_t * list2);
GRAVITY_API void   gravity_list_blacken(gravity_vm * vm, gravity_list_t * list);
GRAVITY_API uint32 gravity_list_size(gravity_vm * vm, gravity_list_t * list);

// MARK: - MAP -
GRAVITY_API gravity_map_t * gravity_map_new(gravity_vm * vm, uint32 n);
GRAVITY_API void   gravity_map_free(gravity_vm * vm, gravity_map_t * map);
GRAVITY_API void   gravity_map_append_map(gravity_vm * vm, gravity_map_t * map1, gravity_map_t * map2);
GRAVITY_API void   gravity_map_insert(gravity_vm * vm, gravity_map_t * map, GravityValue key, GravityValue value);
GRAVITY_API void   gravity_map_blacken(gravity_vm * vm, gravity_map_t * map);
GRAVITY_API uint32 gravity_map_size(gravity_vm * vm, gravity_map_t * map);

// MARK: - RANGE -
GRAVITY_API gravity_range_t * gravity_range_new(gravity_vm * vm, gravity_int_t from, gravity_int_t to, bool inclusive);
GRAVITY_API void   gravity_range_free(gravity_vm * vm, gravity_range_t * range);
GRAVITY_API void   gravity_range_blacken(gravity_vm * vm, gravity_range_t * range);
GRAVITY_API uint32 gravity_range_size(gravity_vm * vm, gravity_range_t * range);
GRAVITY_API void   gravity_range_serialize(gravity_range_t * r, GravityJson * json);
GRAVITY_API gravity_range_t * gravity_range_deserialize(gravity_vm * vm, json_value * json);

/// MARK: - STRING -
GRAVITY_API GravityValue gravity_string_to_value(gravity_vm * vm, const char * s, uint32 len);
GRAVITY_API GravityValue FASTCALL gravity_zstring_to_value(gravity_vm * vm, const char * s);
GRAVITY_API gravity_string_t * gravity_string_new(gravity_vm * vm, char * s, uint32 len, uint32 alloc);
//GRAVITY_API void   gravity_string_set(gravity_string_t * obj, char * s, uint32 len);
GRAVITY_API void   gravity_string_free(gravity_vm * vm, gravity_string_t * value);
GRAVITY_API void   gravity_string_blacken(gravity_vm * vm, gravity_string_t * string);
GRAVITY_API uint32 gravity_string_size(gravity_vm * vm, gravity_string_t * string);

// MARK: - CALLBACKS -
// HASH FREE CALLBACK FUNCTION
GRAVITY_API void gravity_hash_keyvaluefree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data);
GRAVITY_API void gravity_hash_keyfree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data);
GRAVITY_API void gravity_hash_valuefree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data);
GRAVITY_API void gravity_hash_finteralfree(gravity_hash_t * table, GravityValue key, GravityValue value, void * data);
//#ifdef __cplusplus
//}
//#endif
//
//#include <gravity_delegate.h>
//
// Descr: error type and code definitions
//
enum GravityErrorType {
	GRAVITY_ERROR_NONE = 0,
	GRAVITY_ERROR_SYNTAX,
	GRAVITY_ERROR_SEMANTIC,
	GRAVITY_ERROR_RUNTIME,
	GRAVITY_ERROR_IO,
	GRAVITY_WARNING,
};

struct GravityErrorDescription {
	GravityErrorDescription() : lineno(0), colno(0), fileid(0), offset(0)
	{
	}
	GravityErrorDescription(uint32 ln, uint32 cn, uint32 fid, uint32 offs) : lineno(ln), colno(cn), fileid(fid), offset(offs)
	{
	}
	uint32 lineno;
	uint32 colno;
	uint32 fileid;
	uint32 offset;
};

#define ERROR_DESC_NONE     GravityErrorDescription()

typedef void (* gravity_log_callback)(gravity_vm * vm, const char * message, void * xdata);
typedef void (* gravity_log_clear)(gravity_vm * vm, void * xdata);
typedef void (* gravity_error_callback)(gravity_vm * vm, GravityErrorType error_type, const char * description, const GravityErrorDescription * pErrorDesc, void * xdata);
typedef void (* gravity_unittest_callback)(gravity_vm * vm, GravityErrorType error_type, const char * desc, const char * note, GravityValue value, int32 row, int32 col, void * xdata);
typedef void (* gravity_parser_callback)(void * token, void * xdata);
typedef void (* gravity_type_callback)(void * token, const char * type, void * xdata);
typedef const char * (* gravity_precode_callback)(void * xdata);
typedef const char * (* gravity_loadfile_callback)(const char * file, size_t * size, uint32 * fileid, void * xdata, bool * is_static);
typedef const char * (* gravity_filename_callback)(uint32 fileid, void * xdata);
typedef const char ** (* gravity_optclass_callback)();
typedef bool (* gravity_bridge_initinstance)(gravity_vm * vm, void * xdata, GravityValue ctx, gravity_instance_t * instance, GravityValue args[], int16 nargs);
typedef bool (* gravity_bridge_setvalue)(gravity_vm * vm, void * xdata, GravityValue target, const char * key, GravityValue value);
typedef bool (* gravity_bridge_getvalue)(gravity_vm * vm, void * xdata, GravityValue target, const char * key, uint32 vindex);
typedef bool (* gravity_bridge_setundef)(gravity_vm * vm, void * xdata, GravityValue target, const char * key, GravityValue value);
typedef bool (* gravity_bridge_getundef)(gravity_vm * vm, void * xdata, GravityValue target, const char * key, uint32 vindex);
typedef bool (* gravity_bridge_execute)(gravity_vm * vm, void * xdata, GravityValue ctx, GravityValue args[], int16 nargs, uint32 vindex);
typedef bool (* gravity_bridge_equals)(gravity_vm * vm, void * obj1, void * obj2);
typedef const char * (* gravity_bridge_string)(gravity_vm * vm, void * xdata, uint32 * len);
typedef void * (* gravity_bridge_clone)(gravity_vm * vm, void * xdata);
typedef uint32 (* gravity_bridge_size)(gravity_vm * vm, gravity_class_t * obj);
typedef void (* gravity_bridge_free)(gravity_vm * vm, gravity_class_t * obj);
typedef void (* gravity_bridge_blacken)(gravity_vm * vm, void * xdata);

struct gravity_delegate_t {
	gravity_delegate_t(void * pData) : xdata(pData), report_null_errors(false), disable_gccheck_1(false),
		log_callback(0), log_clear(0), error_callback(0), unittest_callback(0), parser_callback(0), type_callback(0),
		precode_callback(0), loadfile_callback(0), filename_callback(0), optional_classes(0), bridge_initinstance(0),
		bridge_setvalue(0), bridge_getvalue(0), bridge_setundef(0), bridge_getundef(0), bridge_execute(0),
		bridge_blacken(0), bridge_string(0), bridge_equals(0), bridge_clone(0), bridge_size(0), bridge_free(0)
	{
	}
	// user data
	void * xdata;            // optional user data transparently passed between callbacks
	bool report_null_errors; // by default messages sent to null objects are silently ignored (if this flag is false)
	bool disable_gccheck_1;  // memory allocations are protected so it could be useful to automatically check gc when enabled is restored
	// callbacks
	gravity_log_callback log_callback;              // log reporting callback
	gravity_log_clear log_clear;                    // log reset callback
	gravity_error_callback error_callback;          // error reporting callback
	gravity_unittest_callback unittest_callback;    // special unit test callback
	gravity_parser_callback parser_callback;        // lexer callback used for syntax highlight
	gravity_type_callback type_callback;            // callback used to bind a token with a declared type
	gravity_precode_callback precode_callback;      // called at parse time in order to give the opportunity to add custom source code
	gravity_loadfile_callback loadfile_callback;    // callback to give the opportunity to load a file from an import statement
	gravity_filename_callback filename_callback;    // called while reporting an error in order to be able to convert a fileid to a real filename
	gravity_optclass_callback optional_classes;     // optional classes to be exposed to the semantic checker as extern (to be later registered)
	// bridge
	gravity_bridge_initinstance bridge_initinstance; // init class
	gravity_bridge_setvalue bridge_setvalue;        // setter
	gravity_bridge_getvalue bridge_getvalue;        // getter
	gravity_bridge_setundef bridge_setundef;        // setter not found
	gravity_bridge_getundef bridge_getundef;        // getter not found
	gravity_bridge_execute bridge_execute;          // execute a method/function
	gravity_bridge_blacken bridge_blacken;          // blacken obj to be GC friend
	gravity_bridge_string bridge_string;            // instance string conversion
	gravity_bridge_equals bridge_equals;            // check if two objects are equals
	gravity_bridge_clone bridge_clone;              // clone
	gravity_bridge_size bridge_size;                // size of obj
	gravity_bridge_free bridge_free;                // free obj
};
//
//#include <gravity_vm.h>
#define GRAVITY_VM_GCENABLED            "gcEnabled"
#define GRAVITY_VM_GCMINTHRESHOLD       "gcMinThreshold"
#define GRAVITY_VM_GCTHRESHOLD          "gcThreshold"
#define GRAVITY_VM_GCRATIO              "gcRatio"
#define GRAVITY_VM_MAXCALLS             "maxCCalls"
#define GRAVITY_VM_MAXBLOCK             "maxBlock"
#define GRAVITY_VM_MAXRECURSION         "maxRecursionDepth"

typedef bool (* vm_filter_cb) (gravity_class_t * obj);
typedef void (* vm_transfer_cb) (gravity_vm * vm, gravity_class_t * obj);
typedef void (* vm_cleanup_cb) (gravity_vm * vm);
//
// Opaque VM struct
//
struct gravity_vm {
	gravity_vm();
	//gravity_fiber_t * gravity_vm_fiber(gravity_vm * vm) { return vm->fiber; }
	gravity_fiber_t * GetFiber() { return fiber; }
	void FASTCALL SetSlot(const GravityValue & rValue, uint32 index);
	bool FASTCALL ReturnValue(const GravityValue & rValue, uint32 index)
	{
		SetSlot(rValue, index);
		return true;
	}
	//bool gravity_return_error(gravity_vm * vm, uint32 rindex, const char * pMessage)
	bool FASTCALL ReturnError(uint32 index, const char * pMessage)
	{
		gravity_fiber_seterror(GetFiber(), pMessage);
		SetSlot(GravityValue::from_null(), index);
		return false;
	}
	bool FASTCALL ReturnErrorSimple(uint32 index)
	{
		SetSlot(GravityValue::from_null(), index);
		return false;
	}
	bool FASTCALL ReturnNull(uint32 index)
	{
		SetSlot(GravityValue::from_null(), index);
		return true;
	}
	bool FASTCALL ReturnUndefined(uint32 index)
	{
		SetSlot(GravityValue::from_undefined(), index);
		return true;
	}
	bool FASTCALL ReturnClosure(const GravityValue & rValue, uint32 index)
	{
		SetSlot(rValue, index);
		return false;
	}
	bool FASTCALL ReturnFiber()
	{
		return false;
	}
	bool FASTCALL ReturnNoValue()
	{
		return true;
	}
	gravity_hash_t      * context;   // context hash table
	gravity_delegate_t  * delegate;  // registered runtime delegate
	gravity_fiber_t     * fiber;     // current fiber
	void   * data;                   // custom data optionally set by the user
	uint32 pc;                       // program counter
	double time;                     // useful timer for the main function
	bool   aborted;                  // set when VM has generated a runtime error
	uint8  Reserve[3]; // @alignment               
	uint32 maxccalls;                // maximum number of nested c calls
	uint32 nccalls;                  // current number of nested c calls
	// recursion
	uint32 maxrecursion;   // maximum recursive depth
	uint32 recursioncount; // recursion counter
	// anonymous names
	uint32 nanon;                    // counter for anonymous classes (used in object_bind)
	char temp[64];                   // temporary buffer used for anonymous names generator
	// callbacks
	vm_transfer_cb transfer;         // function called each time a gravity_class_t is allocated
	vm_cleanup_cb cleanup;           // function called when VM must be cleaned-up
	vm_filter_cb filter;             // function called to filter objects in the cleanup process
	// garbage collector
	int32 gcenabled;                 // flag to enable/disable garbage collector (was bool but it is now reference counted)
	gravity_int_t memallocated;      // total number of allocated memory
	gravity_int_t maxmemblock;       // maximum block memory size allowed to allocate
	gravity_class_t * gchead;       // head of garbage collected objects
	gravity_int_t gcminthreshold;    // minimum GC threshold size to avoid spending too much time in GC
	gravity_int_t gcthreshold;       // memory required to trigger a GC
	gravity_int_t gcthreshold_original; // gcthreshold is dynamically re-computed so I msut save the original value somewhere
	gravity_float_t gcratio;            // ratio used in automatic recomputation of the new gcthreshold value
	gravity_int_t gccount;              // number of objects into GC
	GravityArray <gravity_class_t *> graylist; // array of collected objects while GC is in process (gray list)
	GravityArray <gravity_class_t *> gctemp;   // array of temp objects that need to be saved from GC
	// internal stats fields
#if GRAVITY_VM_STATS
	uint32 nfrealloc;                             // to check how many frames reallocation occurred
	uint32 nsrealloc;                             // to check how many stack reallocation occurred
	uint32 nstat[GRAVITY_LATEST_OPCODE];          // internal used to collect opcode usage stats
	double tstat[GRAVITY_LATEST_OPCODE];            // internal used to collect microbenchmarks
	nanotime_t t;                                   // internal timer
#endif
};

GRAVITY_API gravity_vm * gravity_vm_new(gravity_delegate_t * delegate);
GRAVITY_API gravity_vm * gravity_vm_newmini();
GRAVITY_API void gravity_vm_set_callbacks(gravity_vm * vm, vm_transfer_cb vm_transfer, vm_cleanup_cb vm_cleanup);
GRAVITY_API void gravity_vm_free(gravity_vm * vm);
GRAVITY_API void gravity_vm_reset(gravity_vm * vm);
GRAVITY_API bool gravity_vm_runclosure(gravity_vm * vm, gravity_closure_t * closure, GravityValue sender, GravityValue params[], uint16 nparams);
GRAVITY_API bool gravity_vm_runmain(gravity_vm * vm, gravity_closure_t * closure);
GRAVITY_API void gravity_vm_loadclosure(gravity_vm * vm, gravity_closure_t * closure);
GRAVITY_API void gravity_vm_setvalue(gravity_vm * vm, const char * key, GravityValue value);
GRAVITY_API GravityValue gravity_vm_lookup(gravity_vm * vm, GravityValue key);
GRAVITY_API GravityValue gravity_vm_getvalue(gravity_vm * vm, const char * key, uint32 keylen);
GRAVITY_API double gravity_vm_time(const gravity_vm * vm);
GRAVITY_API GravityValue gravity_vm_result(gravity_vm * vm);
GRAVITY_API gravity_delegate_t  * gravity_vm_delegate(gravity_vm * vm);
//GRAVITY_API gravity_fiber_t     * gravity_vm_fiber(gravity_vm * vm);
GRAVITY_API void  gravity_vm_setfiber(gravity_vm* vm, gravity_fiber_t * fiber);
GRAVITY_API void  gravity_vm_seterror(gravity_vm * vm, const char * format, ...);
GRAVITY_API void  gravity_vm_seterror_string(gravity_vm* vm, const char * s);
GRAVITY_API bool  gravity_vm_ismini(const gravity_vm * vm);
GRAVITY_API GravityValue     gravity_vm_keyindex(gravity_vm * vm, uint32 index);
GRAVITY_API bool  gravity_vm_isaborted(const gravity_vm * vm);
GRAVITY_API void  gravity_vm_setaborted(gravity_vm * vm);
GRAVITY_API gravity_closure_t   * gravity_vm_getclosure(gravity_vm * vm);
GRAVITY_API void FASTCALL gravity_gray_value(gravity_vm* vm, GravityValue v);
GRAVITY_API void FASTCALL gravity_gray_object(gravity_vm* vm, gravity_class_t * obj);
GRAVITY_API void gravity_gc_start(gravity_vm* vm);
GRAVITY_API void FASTCALL gravity_gc_setenabled(gravity_vm* vm, bool enabled);
GRAVITY_API void gravity_gc_temppush(gravity_vm * vm, gravity_class_t * obj);
GRAVITY_API void gravity_gc_temppop(gravity_vm * vm);
GRAVITY_API void gravity_gc_tempnull(gravity_vm * vm, gravity_class_t * obj);
GRAVITY_API void gravity_gc_setvalues(gravity_vm * vm, gravity_int_t threshold, gravity_int_t minthreshold, gravity_float_t ratio);
GRAVITY_API void FASTCALL gravity_vm_transfer(gravity_vm* vm, gravity_class_t * obj);
GRAVITY_API void gravity_vm_cleanup(gravity_vm* vm);
GRAVITY_API void gravity_vm_filter(gravity_vm* vm, vm_filter_cb cleanup_filter);
GRAVITY_API gravity_closure_t   * gravity_vm_loadfile(gravity_vm * vm, const char * path);
GRAVITY_API gravity_closure_t   * gravity_vm_loadbuffer(gravity_vm * vm, const char * buffer, size_t len);
GRAVITY_API void gravity_vm_initmodule(gravity_vm * vm, gravity_function_t * f);
GRAVITY_API gravity_closure_t   * gravity_vm_fastlookup(gravity_vm * vm, gravity_class_t * c, int index);
//GRAVITY_API void FASTCALL gravity_vm_setslot(gravity_vm * vm, GravityValue value, uint32 index);
GRAVITY_API GravityValue     gravity_vm_getslot(gravity_vm * vm, uint32 index);
GRAVITY_API void gravity_vm_setdata(gravity_vm * vm, void * data);
GRAVITY_API void * gravity_vm_getdata(gravity_vm * vm);
GRAVITY_API void gravity_vm_memupdate(gravity_vm * vm, gravity_int_t value);
GRAVITY_API gravity_int_t FASTCALL gravity_vm_maxmemblock(const gravity_vm * vm);
GRAVITY_API GravityValue  gravity_vm_get(gravity_vm * vm, const char * key);
GRAVITY_API bool gravity_vm_set(gravity_vm * vm, const char * key, GravityValue value);
GRAVITY_API char * gravity_vm_anonymous(gravity_vm * vm);
GRAVITY_API bool gravity_isopt_class(const gravity_class_t * c);
GRAVITY_API void gravity_opt_register(gravity_vm * vm);
GRAVITY_API void gravity_opt_free();
//
//#include <gravity_core.h>
// core functions
GRAVITY_API void gravity_core_init();
GRAVITY_API void gravity_core_register(gravity_vm *vm);
GRAVITY_API bool gravity_iscore_class(gravity_class_t *c);
GRAVITY_API void gravity_core_free();
GRAVITY_API const char **gravity_core_identifiers();
GRAVITY_API gravity_class_t *gravity_core_class_from_name(const char *name);

// conversion functions
GravityValue convert_value2int(gravity_vm *vm, GravityValue v);
GravityValue convert_value2float(gravity_vm *vm, GravityValue v);
GravityValue convert_value2bool(gravity_vm *vm, GravityValue v);
GravityValue convert_value2string(gravity_vm *vm, GravityValue v);
// internal functions
gravity_closure_t * FASTCALL computed_property_create(gravity_vm *vm, gravity_function_t *getter_func, gravity_function_t *setter_func);
void computed_property_free(gravity_class_t *c, const char *name, bool remove_flag);
//
//#include <gravity_hash.h>
#define GRAVITYHASH_ENABLE_STATS    1               // if 0 then stats are not enabled
#define GRAVITYHASH_DEFAULT_SIZE    32              // default hash table size (used if 0 is passed in gravity_hash_create)
#define GRAVITYHASH_THRESHOLD       0.75            // threshold used to decide when re-hash the table
#define GRAVITYHASH_MAXENTRIES      1073741824      // please don't put more than 1 billion values in my hash table (2^30)
#ifndef GRAVITY_HASH_DEFINED
	#define GRAVITY_HASH_DEFINED
	typedef struct         gravity_hash_t gravity_hash_t;       // opaque hash table struct
#endif

//#ifdef __cplusplus
//extern "C" {
//#endif
	// CALLBACK functions
	typedef uint32 (* gravity_hash_compute_fn) (GravityValue key);
	typedef bool (* gravity_hash_isequal_fn) (GravityValue v1, GravityValue v2);
	typedef void (* gravity_hash_iterate_fn) (gravity_hash_t * hashtable, GravityValue key, GravityValue value, void * data);
	typedef void (* gravity_hash_iterate2_fn) (gravity_hash_t * hashtable, GravityValue key, GravityValue value, void * data1, void * data2);
	typedef void (* gravity_hash_iterate3_fn) (gravity_hash_t * hashtable, GravityValue key, GravityValue value, void * data1, void * data2, void * data3);
	typedef void (* gravity_hash_transform_fn) (gravity_hash_t * hashtable, GravityValue key, GravityValue * value, void * data);
	typedef bool (* gravity_hash_compare_fn) (GravityValue value1, GravityValue value2, void * data);

	// PUBLIC functions
	GRAVITY_API gravity_hash_t  * gravity_hash_create(uint32 size, gravity_hash_compute_fn compute, gravity_hash_isequal_fn isequal, gravity_hash_iterate_fn free, void * data);
	GRAVITY_API void gravity_hash_free(gravity_hash_t * hashtable);
	GRAVITY_API bool gravity_hash_isempty(gravity_hash_t * hashtable);
	GRAVITY_API bool gravity_hash_remove(gravity_hash_t * hashtable, GravityValue key);
	GRAVITY_API bool FASTCALL gravity_hash_insert(gravity_hash_t * hashtable, GravityValue key, GravityValue value);
	GRAVITY_API GravityValue * FASTCALL gravity_hash_lookup(gravity_hash_t * hashtable, GravityValue key);
	GRAVITY_API GravityValue * gravity_hash_lookup_cstring(gravity_hash_t * hashtable, const char * key);
	GRAVITY_API uint32 gravity_hash_memsize(const gravity_hash_t * hashtable);
	GRAVITY_API uint32 FASTCALL gravity_hash_count(const gravity_hash_t * hashtable);
	GRAVITY_API uint32 FASTCALL gravity_hash_compute_buffer(const char * key, uint32 len);
	GRAVITY_API uint32 gravity_hash_compute_int(gravity_int_t n);
	GRAVITY_API uint32 gravity_hash_compute_float(gravity_float_t f);
	GRAVITY_API void  gravity_hash_stat(gravity_hash_t * hashtable);
	GRAVITY_API void  gravity_hash_iterate(gravity_hash_t * hashtable, gravity_hash_iterate_fn iterate, void * data);
	GRAVITY_API void  gravity_hash_iterate2(gravity_hash_t * hashtable, gravity_hash_iterate2_fn iterate, void * data1, void * data2);
	GRAVITY_API void  gravity_hash_iterate3(gravity_hash_t * hashtable, gravity_hash_iterate3_fn iterate, void * data1, void * data2, void * data3);
	GRAVITY_API void  gravity_hash_transform(gravity_hash_t * hashtable, gravity_hash_transform_fn iterate, void * data);
	GRAVITY_API void  gravity_hash_dump(gravity_hash_t * hashtable);
	GRAVITY_API void  gravity_hash_append(gravity_hash_t * hashtable1, gravity_hash_t * hashtable2);
	GRAVITY_API void  gravity_hash_resetfree(gravity_hash_t * hashtable);
	GRAVITY_API bool  gravity_hash_compare(gravity_hash_t * hashtable1, gravity_hash_t * hashtable2, gravity_hash_compare_fn compare, void * data);
//#ifdef __cplusplus
//}
//#endif
//
//#include <gravity_macros.h>
#define AUTOLENGTH                          UINT32_MAX

// MARK: -
// pragma unused is not recognized by VC
#define UNUSED_PARAM(_x)                    (void)(_x)
#define UNUSED_PARAM2(_x,_y)                UNUSED_PARAM(_x),UNUSED_PARAM(_y)
#define UNUSED_PARAM3(_x,_y,_z)             UNUSED_PARAM(_x),UNUSED_PARAM(_y),UNUSED_PARAM(_z)

// MARK: -
#define VALUE_AS_OBJECT(x)                  ((x).Ptr)
#define VALUE_AS_FIBER(x)                   (reinterpret_cast<gravity_fiber_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_FUNCTION(x)                (reinterpret_cast<gravity_function_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_PROPERTY(x)                (reinterpret_cast<gravity_property_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_CLOSURE(x)                 (reinterpret_cast<gravity_closure_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_INSTANCE(x)                (reinterpret_cast<gravity_instance_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_MAP(x)                     (reinterpret_cast<gravity_map_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_RANGE(x)                   (reinterpret_cast<gravity_range_t *>(VALUE_AS_OBJECT(x)))
#define VALUE_AS_ERROR(x)                   (static_cast<const char *>((x).Ptr))
#define VALUE_AS_BOOL(x)                    LOGIC((x).n) // @sobolev LOGIC
//#define VALUE_AS_STRING(x)                  (reinterpret_cast<gravity_string_t *>(VALUE_AS_OBJECT(x)))
//#define VALUE_AS_CLASS(x)                   (reinterpret_cast<gravity_class_t *>(VALUE_AS_OBJECT(x)))
//#define VALUE_AS_LIST(x)                    (reinterpret_cast<gravity_list_t *>(VALUE_AS_OBJECT(x)))
//#define VALUE_AS_CSTRING(x)                 (VALUE_AS_STRING(x)->s)
//#define VALUE_AS_FLOAT(x)                   ((x).f)
//#define VALUE_AS_INT(x)                     ((x).n)

// MARK: -
#define VALUE_FROM_STRING(_vm,_s,_len)       (gravity_string_to_value(_vm, _s, _len))
//#define VALUE_FROM_CSTRING(_vm,_s)          (gravity_string_to_value(_vm, _s, AUTOLENGTH))
//#define VALUE_FROM_ERROR(msg)               GravityValue::from_error(msg)
//#define VALUE_FROM_OBJECT(obj)              GravityValue::from_object(obj)
//#define VALUE_FROM_BOOL(x)                  GravityValue::from_bool(x)
//#define VALUE_FROM_FALSE                    GravityValue::from_bool(false)
//#define VALUE_FROM_TRUE                     GravityValue::from_bool(true)
//#define VALUE_FROM_INT(x)                   GravityValue::from_int(x)
//#define VALUE_FROM_FLOAT(x)                 GravityValue::from_float(x)
//#define VALUE_FROM_NULL                     GravityValue::from_null()
//#define VALUE_FROM_UNDEFINED                GravityValue::from_undefined()
#define VALUE_NOT_VALID                     GravityValue::from_error(NULL)
//#define STATICVALUE_FROM_STRING(_v,_s,_l)   gravity_string_t __temp = {.isa = GravityEnv.P_ClsString, .s = (char *)_s, .len = (uint32)_l, }; \
//                                            __temp.hash = gravity_hash_compute_buffer(__temp.s, __temp.len); \
//                                            GravityValue _v = {.isa = GravityEnv.P_ClsString, .p = (gravity_class_t *)&__temp };
#define STATICVALUE_FROM_STRING(_v,_s,_l)   gravity_string_t __temp((char *)_s, (uint32)_l); \
                                            __temp.hash = gravity_hash_compute_buffer(__temp.cptr(), __temp.len); \
                                            GravityValue _v(GravityEnv.P_ClsString, (gravity_class_t *)&__temp);
// MARK: -
//#define VALUE_ISA_FUNCTION(v)               (v.isa == GravityEnv.P_ClsFunc)
//#define VALUE_ISA_NULLCLASS(v)              (v.isa == GravityEnv.P_ClsNull)
//#define VALUE_ISA_NULL(v)                   ((v.isa == GravityEnv.P_ClsNull) && (v.n == 0))
//#define VALUE_ISA_UNDEFINED(v)              ((v.isa == GravityEnv.P_ClsNull) && (v.n == 1))
//#define OBJECT_ISA_NULL(obj)                (obj->isa == GravityEnv.P_ClsNull)
//#define OBJECT_ISA_UPVALUE(obj)             (obj->isa == GravityEnv.P_ClsUpValue)
//#define OBJECT_ISA_MODULE(obj)              (obj->isa == GravityEnv.P_ClsModule)
//#define VALUE_ISA_INSTANCE(v)               (v.isa == GravityEnv.P_ClsInstance)
//#define VALUE_ISA_CLOSURE(v)                (v.isa == GravityEnv.P_ClsClosure)
//#define VALUE_ISA_FIBER(v)                  (v.isa == GravityEnv.P_ClsFiber)
//#define VALUE_ISA_CLASS(v)                  (v.isa == GravityEnv.P_ClsClass)
//#define VALUE_ISA_STRING(v_)                 (v_.isa == GravityEnv.P_ClsString)
//#define VALUE_ISA_INT(v)                    (v.isa == GravityEnv.P_ClsInt)
//#define VALUE_ISA_FLOAT(v_)                  (v_.isa == GravityEnv.P_ClsFloat)
//#define VALUE_ISA_BOOL(v_)                   (v_.isa == GravityEnv.P_ClsBool)
//#define VALUE_ISA_LIST(v)                   (v.isa == GravityEnv.P_ClsList)
//#define VALUE_ISA_MAP(v)                    (v.isa == GravityEnv.P_ClsMap)
//#define VALUE_ISA_RANGE(v)                  (v.isa == GravityEnv.P_ClsRange)
//#define VALUE_ISA_BASIC_TYPE(v)             (v.IsString() || v.IsInt() || v.IsFloat() || v.IsBool())
//#define VALUE_ISA_CLASS(v)                  (v.isa == GravityEnv.P_ClsClass)
//#define VALUE_ISA_CALLABLE(v)               (v.IsFunction() || v.IsClass() || v.IsFiber())
// replaced with (!!v) #define VALUE_ISA_VALID(v)                  (v.isa != NULL)
//#define VALUE_ISA_NOTVALID(v)               (v.isa == NULL)
//#define VALUE_ISA_ERROR(v)                  VALUE_ISA_NOTVALID(v)
//#define OBJECT_ISA_INT(obj)                 (obj->isa == GravityEnv.P_ClsInt)
//#define OBJECT_ISA_FLOAT(obj)               (obj->isa == GravityEnv.P_ClsFloat)
//#define OBJECT_ISA_BOOL(obj)                (obj->isa == GravityEnv.P_ClsBool)
//#define OBJECT_ISA_CLASS(obj)               (obj->isa == GravityEnv.P_ClsClass)
//#define OBJECT_ISA_FUNCTION(obj)            (obj->isa == GravityEnv.P_ClsFunc)
//#define OBJECT_ISA_CLOSURE(obj)             (obj->isa == GravityEnv.P_ClsClosure)
//#define OBJECT_ISA_INSTANCE(obj)            (obj->isa == GravityEnv.P_ClsInstance)
//#define OBJECT_ISA_LIST(obj)                (obj->isa == GravityEnv.P_ClsList)
//#define OBJECT_ISA_MAP(obj)                 (obj->isa == GravityEnv.P_ClsMap)
//#define OBJECT_ISA_STRING(obj)              (obj->isa == GravityEnv.P_ClsString)
//#define OBJECT_ISA_FIBER(obj)               (obj->isa == GravityEnv.P_ClsFiber)
//#define OBJECT_ISA_RANGE(obj)               (obj->isa == GravityEnv.P_ClsRange)
//#define OBJECT_IS_VALID(obj)                (obj->isa != NULL)

// MARK: -
//#define LIST_COUNT(v)                       (static_cast<gravity_list_t *>(v)->array.getCount())
//#define LIST_VALUE_AT_INDEX(v, idx)         (static_cast<gravity_list_t *>(v)->array.at(idx))

// MARK: -
#define GRAVITY_JSON_FUNCTION               "function"
#define GRAVITY_JSON_CLASS                  "class"
#define GRAVITY_JSON_RANGE                  "range"
#define GRAVITY_JSON_INSTANCE               "instance"
#define GRAVITY_JSON_ENUM                   "enum"
#define GRAVITY_JSON_MAP                    "map"
#define GRAVITY_JSON_VAR                    "var"
#define GRAVITY_JSON_GETTER                 "$get"
#define GRAVITY_JSON_SETTER                 "$set"

#define GRAVITY_JSON_LABELTAG               "tag"
#define GRAVITY_JSON_LABELNAME              "name"
#define GRAVITY_JSON_LABELTYPE              "type"
#define GRAVITY_JSON_LABELVALUE             "value"
#define GRAVITY_JSON_LABELIDENTIFIER        "identifier"
#define GRAVITY_JSON_LABELPOOL              "pool"
#define GRAVITY_JSON_LABELPVALUES           "pvalues"
#define GRAVITY_JSON_LABELPNAMES            "pnames"
#define GRAVITY_JSON_LABELMETA              "meta"
#define GRAVITY_JSON_LABELBYTECODE          "bytecode"
#define GRAVITY_JSON_LABELLINENO            "lineno"
#define GRAVITY_JSON_LABELNPARAM            "nparam"
#define GRAVITY_JSON_LABELNLOCAL            "nlocal"
#define GRAVITY_JSON_LABELNTEMP             "ntemp"
#define GRAVITY_JSON_LABELNUPV              "nup"
#define GRAVITY_JSON_LABELARGS              "args"
#define GRAVITY_JSON_LABELINDEX             "index"
#define GRAVITY_JSON_LABELSUPER             "super"
#define GRAVITY_JSON_LABELNIVAR             "nivar"
#define GRAVITY_JSON_LABELSIVAR             "sivar"
#define GRAVITY_JSON_LABELPURITY            "purity"
#define GRAVITY_JSON_LABELREADONLY          "readonly"
#define GRAVITY_JSON_LABELSTORE             "store"
#define GRAVITY_JSON_LABELINIT              "init"
#define GRAVITY_JSON_LABELSTATIC            "static"
#define GRAVITY_JSON_LABELPARAMS            "params"
#define GRAVITY_JSON_LABELSTRUCT            "struct"
#define GRAVITY_JSON_LABELFROM              "from"
#define GRAVITY_JSON_LABELTO                "to"
#define GRAVITY_JSON_LABELIVAR              "ivar"
#define GRAVITY_VM_ANONYMOUS_PREFIX         "$$"
// MARK: -
#if 1
#define DEBUG_ASSERT(condition, message)    do { if(!(condition)) { Gravity_Implement_DebugAssert(__FILE__, __LINE__, __func__, message); } } while(0)
#else
	#define DEBUG_ASSERT(condition, message)
#endif
//
//#include <gravity_vmmacros.h>
// MACROS used in VM
#if 0
	#define DEBUG_CALL(s, f)                                printf("%s %s\n", s, f->identifier)
#else
	#define DEBUG_CALL(s, f)
#endif

// signed operation decoding (OPCODE_GET_ONE8bit_SIGN_ONE17bit) from my question on stackoverflow
// http://stackoverflow.com/questions/37054769/optimize-a-bit-decoding-operation-in-c?noredirect=1#comment61673505_37054769

#define OPCODE_GET_OPCODE(op)                           ((op >> 26) & 0x3F)
#define OPCODE_GET_ONE8bit_FLAG_ONE17bit(op,r1,f,n)     r1 = (op >> 18) & 0xFF; f = (op >> 17) & 0x01; n = (int32)(op & 0x1FFFF)
#define OPCODE_GET_ONE8bit_SIGN_ONE17bit(op,r1,n)       r1 = (op >> 18) & 0xFF; n = ((int32)(op & 0x1FFFF) - (int32)(op & 0x20000))
#define OPCODE_GET_TWO8bit_ONE10bit(op,r1,r2,r3)        r1 = (op >> 18) & 0xFF; r2 = (op >> 10) & 0xFF; r3 = (op & 0x3FF)
#define OPCODE_GET_ONE8bit(op,r1)                       r1 = (op >> 18) & 0xFF;
#define OPCODE_GET_SIGN_ONE25bit(op, n)                 n = ((op >> 25) & 0x01) ? -(op & 0x1FFFFFF) : (op & 0x1FFFFFF)
#define OPCODE_GET_ONE8bit_ONE18bit(op,r1,n)            r1 = (op >> 18) & 0xFF; n = (op & 0x3FFFF)
#define OPCODE_GET_LAST18bit(op,n)                      n = (op & 0x3FFFF)
#define OPCODE_GET_ONE26bit(op, n)                      n = (op & 0x3FFFFFF)
#define OPCODE_GET_ONE8bit_ONE10bit(op,r1,r3)           r1 = (op >> 18) & 0xFF; r3 = (op & 0x3FF)
#define OPCODE_GET_THREE8bit(op,r1,r2,r3)               OPCODE_GET_TWO8bit_ONE10bit(op,r1,r2,r3)
#define OPCODE_GET_FOUR8bit(op,r1,r2,r3,r4)             r1 = (op >> 24) & 0xFF; r2 = (op >> 16) & 0xFF; r3 = (op >> 8) & 0xFF; r4 = (op & 0xFF)
#define OPCODE_GET_THREE8bit_ONE2bit(op,r1,r2,r3,r4)    r1 = (op >> 18) & 0xFF; r2 = (op >> 10) & 0xFF; r3 = (op >> 2) & 0xFF; r4 = (op & 0x03)

#define GRAVITY_VM_DEBUG                0               // print each VM instruction
#define GRAVITY_VM_STATS                0               // print VM related stats after each execution
#define GRAVITY_GC_STATS                0               // print useful stats each time GC runs
#define GRAVITY_GC_STRESSTEST           0               // force a GC run after each memory allocation
#define GRAVITY_GC_DEBUG                0               // print objects transferred and grayed
#define GRAVITY_STACK_DEBUG             0               // dump the stack at each CALL and in some other places
#define GRAVITY_TRUST_USERCODE          0               // set at 1 at your own risk!
                                                        // when 0 each time an internal or a bridge function is executed the GC is disabled
                                                        // in this way user does not have to completely understand how GC works under the hood

#if GRAVITY_STACK_DEBUG
	#define DEBUG_STACK()                   gravity_stack_dump(fiber)
#else
	#define DEBUG_STACK()
#endif
#if GRAVITY_GC_DEBUG
	//#define DEBUG_GC(...)                   printf(__VA_ARGS__);printf("\n");fflush(stdout)
#else
	//#define DEBUG_GC(...)
#endif
#if GRAVITY_VM_DEBUG
	#define DEBUG_VM(...)                   DEBUG_STACK();printf("%06u\t",vm->pc); printf(__VA_ARGS__);printf("\n");fflush(stdout)
	//#define DEBUG_VM_NOCR(...)              DEBUG_STACK();printf("%06u\t",vm->pc); printf(__VA_ARGS__);fflush(stdout)
	//#define DEBUG_VM_RAW(...)               printf(__VA_ARGS__);fflush(stdout)
	#define INC_PC                          ++vm->pc;
#else
	#define DEBUG_VM(...)
	//#define DEBUG_VM_NOCR(...)
	//#define DEBUG_VM_RAW(...)
	#define INC_PC
#endif

inline void DebugGc(const char * pMsg, gravity_class_t * pObj)
{
#if GRAVITY_GC_DEBUG
	printf(pMsg, gravity_object_debug(pObj, false));
	printf("\n");
	fflush(stdout);
#endif
}

inline void DebugVmRaw(gravity_function_t * pFunc)
{
#if GRAVITY_VM_DEBUG
	printf("******\tEXEC %s (%p) ******\n", pFunc->identifier, pFunc);
	fflush(stdout);
#endif
}
#if GRAVITY_TRUST_USERCODE
	#define BEGIN_TRUST_USERCODE(_vm)
	#define END_TRUST_USERCODE(_vm)
#else
	#define BEGIN_TRUST_USERCODE(_vm)       gravity_gc_setenabled(_vm, false)
	#define END_TRUST_USERCODE(_vm)         gravity_gc_setenabled(_vm, true)
#endif
#if GRAVITY_VM_STATS
	#define RESET_STATS(_vm)                bzero(_vm->nstat, sizeof(_vm->nstat)); bzero(_vm->tstat, sizeof(_vm->tstat))
	#define PRINT_STATS(_vm)                gravity_vm_stats(_vm)
	#define START_MICROBENCH(_vm)           _vm->t = nanotime()
	#define UPDATE_STATS(_vm,_op)           ++_vm->nstat[_op]; _vm->tstat[_op] += millitime(_vm->t, nanotime())
	#define STAT_FRAMES_REALLOCATED(_vm)    ++_vm->nfrealloc
	#define STAT_STACK_REALLOCATED(_vm)     ++_vm->nsrealloc
#else
	#define RESET_STATS(_vm)
	#define PRINT_STATS(_vm)
	#define START_MICROBENCH(_vm)
	#define UPDATE_STATS(_vm,_op)
	#define STAT_FRAMES_REALLOCATED(_vm)
	#define STAT_STACK_REALLOCATED(_vm)
#endif

// starting from version 0.6.3 a call to STORE_FRAME macro has been added in order to syncronize current IP
// for a better line number computation in case of runtime error (as a consequence ip and frame variables
// has been explicitly exposed in the gravity_vm_runclosure function and the infinite loop error message
// has been moved outside the gravity_check_stack function)
#define RUNTIME_ERROR(...) do { STORE_FRAME(); report_runtime_error(vm, GRAVITY_ERROR_RUNTIME, __VA_ARGS__); return false; } while (0)
#define RUNTIME_FIBER_ERROR(_err)       RUNTIME_ERROR("%s",_err)
#define RUNTIME_WARNING(...) do { report_runtime_error(vm, GRAVITY_WARNING, __VA_ARGS__); } while (0)
#define SETVALUE_BOOL(idx, x)           stackstart[idx]=GravityValue::from_bool(x)
#define SETVALUE_INT(idx, x)            stackstart[idx]=GravityValue::from_int(x)
#define SETVALUE_FLOAT(idx, x)          stackstart[idx]=GravityValue::from_float(x)
#define SETVALUE_NULL(idx)              stackstart[idx]=GravityValue::from_null()
#define SETVALUE(idx, x)                stackstart[idx]=x
#define GETVALUE_INT(v)                 v.n
#define GETVALUE_FLOAT(v)               v.f
#define STACK_GET(idx)                  stackstart[idx]
// macro the count number of registers needed by the _f function which is the sum of local variables, temp variables and formal parameters
#define FN_COUNTREG(_f,_nargs)          (MAX(_f->nparams,_nargs) + _f->nlocals + _f->ntemps)

/* @sobolev #if GRAVITY_COMPUTED_GOTO
#define DECLARE_DISPATCH_TABLE      static void* dispatchTable[] = {                                \
                                    &&RET0,         &&HALT,         &&NOP,          &&RET,          \
                                    &&CALL,         &&LOAD,         &&LOADS,        &&LOADAT,       \
                                    &&LOADK,        &&LOADG,        &&LOADI,        &&LOADU,        \
                                    &&MOVE,         &&STORE,        &&STOREAT,      &&STOREG,       \
                                    &&STOREU,       &&JUMP,         &&JUMPF,        &&SWITCH,       \
                                    &&ADD,          &&SUB,          &&DIV,          &&MUL,          \
                                    &&REM,          &&AND,          &&OR,           &&LT,           \
                                    &&GT,           &&EQ,           &&LEQ,          &&GEQ,          \
                                    &&NEQ,          &&EQQ,          &&NEQQ,         &&ISA,          \
                                    &&MATCH,        &&NEG,          &&NOT,          &&LSHIFT,       \
                                    &&RSHIFT,       &&BAND,         &&BOR,          &&BXOR,         \
                                    &&BNOT,         &&MAPNEW,       &&LISTNEW,      &&RANGENEW,     \
                                    &&SETLIST,      &&CLOSURE,      &&CLOSE,        &&CHECK,        \
                                    &&RESERVED2,    &&RESERVED3,    &&RESERVED4,    &&RESERVED5,    \
                                    &&RESERVED6                                                        };
#define INTERPRET_LOOP              DISPATCH();
#define CASE_CODE(name)             START_MICROBENCH(vm); name
#if GRAVITY_VM_STATS
	#define DISPATCH()                  DEBUG_STACK();INC_PC;inst = *ip++;op = (opcode_t)OPCODE_GET_OPCODE(inst);UPDATE_STATS(vm,op);goto *dispatchTable[op];
#else
	#define DISPATCH()                  DEBUG_STACK();INC_PC;inst = *ip++;goto *dispatchTable[op = (opcode_t)OPCODE_GET_OPCODE(inst)];
#endif
#else*/
	#define DECLARE_DISPATCH_TABLE
	#define INTERPRET_LOOP              inst = *ip++;op = (opcode_t)OPCODE_GET_OPCODE(inst);UPDATE_STATS(wm, op);switch (op) // @sobolev UPDATE_STATS(op)-->UPDATE_STATS(wm, op)
	#define CASE_CODE(name)             case name
	#define DISPATCH()                  break
// @sobolev #endif
#define INIT_PARAMS(n)              for(uint32 i=n; i<func->nparams; ++i) stackstart[i] = GravityValue::from_undefined();
#define STORE_FRAME()               frame->ip = ip

#define LOAD_FRAME()                if(vm->aborted) return false;                                                  \
                                    frame = &fiber->frames[fiber->nframes - 1];                                     \
                                    stackstart = frame->stackstart;                                                 \
                                    ip = frame->ip;                                                                 \
                                    func = frame->closure->f;                                                       \
									DebugVmRaw(func) //DEBUG_VM_RAW("******\tEXEC %s (%p) ******\n", func->identifier, func)
                                    

// SYNC_STACKTOP has been modified in version 0.5.8 (December 4th 2018)
// stack must be trashed ONLY in the fiber remains the same otherwise GC will collect stack values from a still active Fiber
#define SYNC_STACKTOP(_fiber_saved, _fiber,_n)      if (_fiber_saved && (_fiber_saved == _fiber)) _fiber_saved->stacktop -= _n
#define SETFRAME_OUTLOOP(cframe)                    (cframe)->outloop = true
#define COMPUTE_JUMP(value)                         (func->U.Nf.bytecode + (value))
// FAST MATH MACROS
#define FMATH_BIN_INT(_r1,_v2,_v3,_OP)              do {SETVALUE(_r1, GravityValue::from_int(_v2 _OP _v3)); DISPATCH();} while(0)
#define FMATH_BIN_FLOAT(_r1,_v2,_v3,_OP)            do {SETVALUE(_r1, GravityValue::from_float(_v2 _OP _v3)); DISPATCH();} while(0)
#define FMATH_BIN_BOOL(_r1,_v2,_v3,_OP)             do {SETVALUE(_r1, GravityValue::from_bool(_v2 _OP _v3)); DISPATCH();} while(0)
#define DEFINE_STACK_VARIABLE(_v,_r)                GravityValue _v = STACK_GET(_r)
#define DEFINE_INDEX_VARIABLE(_v,_r)                GravityValue _v = (_r < MAX_REGISTERS) ? STACK_GET(_r) : GravityValue::from_int(_r-MAX_REGISTERS)
#define NO_CHECK
#define CHECK_ZERO(_v)                              if((_v.IsInt() && (_v.n == 0)) || (_v.IsFloat() && (_v.f == 0.0)) || (_v.IsNull())) \
                                                    RUNTIME_ERROR("Division by 0 error.")

#define CHECK_FAST_BINARY_BOOL(r1,r2,r3,v2,v3,OP)   DEFINE_STACK_VARIABLE(v2,r2);                                                           \
                                                    DEFINE_STACK_VARIABLE(v3,r3);                                                           \
                                                    if(v2.IsBool() && v3.IsBool()) FMATH_BIN_BOOL(r1, v2.n, v3.n, OP)

#define CHECK_FAST_UNARY_BOOL(r1,r2,v2,OP)          DEFINE_STACK_VARIABLE(v2,r2);                                                           \
                                                    if(v2.IsBool()) {SETVALUE(r1, GravityValue::from_bool(OP v2.n)); DISPATCH();}

// fast math only for INT and FLOAT
#define CHECK_FAST_BINARY_MATH(r1,r2,r3,v2,v3,OP,_CHECK)                                                                                                        \
                                                    DEFINE_STACK_VARIABLE(v2,r2);                                                                               \
                                                    DEFINE_STACK_VARIABLE(v3,r3);                                                                               \
                                                    _CHECK;                                                                                                     \
                                                    if(v2.IsInt()) {                                                                                    \
                                                        if(v3.IsInt()) FMATH_BIN_INT(r1, v2.n, v3.n, OP);                                               \
                                                        if(v3.IsFloat()) FMATH_BIN_FLOAT(r1, v2.n, v3.f, OP);                                           \
                                                        if(v3.IsNull()) FMATH_BIN_INT(r1, v2.n, 0, OP);                                                 \
                                                        if(v3.IsString()) RUNTIME_ERROR("Right operand must be a number (use the number() method).");   \
                                                    } else if(v2.IsFloat()) {                                                                           \
                                                        if(v3.IsFloat()) FMATH_BIN_FLOAT(r1, v2.f, v3.f, OP);                                           \
                                                        if(v3.IsInt()) FMATH_BIN_FLOAT(r1, v2.f, v3.n, OP);                                             \
                                                        if(v3.IsNull()) FMATH_BIN_FLOAT(r1, v2.f, 0, OP);                                               \
                                                        if(v3.IsString()) RUNTIME_ERROR("Right operand must be a number (use the number() method).");   \
                                                    }

#define CHECK_FAST_UNARY_MATH(r1,r2,v2,OP)          DEFINE_STACK_VARIABLE(v2,r2);                                                   \
                                                    if(v2.IsInt()) {SETVALUE(r1, GravityValue::from_int(OP v2.n)); DISPATCH();}     \
                                                    if(v2.IsFloat()) {SETVALUE(r1, GravityValue::from_float(OP v2.f)); DISPATCH();}


#define CHECK_FAST_BINARY_REM(r1,r2,r3,v2,v3)       DEFINE_STACK_VARIABLE(v2,r2); DEFINE_STACK_VARIABLE(v3,r3); CHECK_ZERO(v3); \
                                                    if(v2.IsInt() && v3.IsInt()) FMATH_BIN_INT(r1, v2.n, v3.n, %)

#define CHECK_FAST_BINARY_BIT(r1,r2,r3,v2,v3,OP)    DEFINE_STACK_VARIABLE(v2,r2); DEFINE_STACK_VARIABLE(v3,r3); \
                                                    if(v2.IsInt() && v3.IsInt()) FMATH_BIN_INT(r1, v2.n, v3.n, OP)

#define CHECK_FAST_BINBOOL_BIT(r1,v2,v3,OP)         if(v2.IsBool() && v3.IsBool()) FMATH_BIN_BOOL(r1, v2.n, v3.n, OP)

#define DECODE_BINARY_OPERATION(r1,r2,r3)           OPCODE_GET_TWO8bit_ONE10bit(inst, const uint32 r1, const uint32 r2, const uint32 r3); \
                                                    DEBUG_VM("%s %d %d %d", opcode_name(op), r1, r2, r3)

#define PREPARE_FUNC_CALLN(_c,_i,_w,_N)             gravity_closure_t *_c = gravity_class_lookup_closure(v2.GetClass(), cache[_i]); \
                                                    if(!_c || !_c->f) RUNTIME_ERROR("Unable to perform operator %s on object", opcode_name(op));   \
                                                    uint32 _w = FN_COUNTREG(func, frame->nargs); \
                                                    uint32 _rneed = FN_COUNTREG(_c->f, _N);      \
													uint32 stacktopdelta = (uint32)MAX(stackstart + _w + _rneed - fiber->stacktop, 0); \
                                                    if(!gravity_check_stack(vm, fiber, stacktopdelta, &stackstart)) return false;              \
                                                    if(vm->aborted) return false

#define PREPARE_FUNC_CALL1(_c,_v1,_i,_w)            PREPARE_FUNC_CALLN(_c,_i,_w,1); SETVALUE(_w, _v1)
#define PREPARE_FUNC_CALL2(_c,_v1,_v2,_i,_w)        PREPARE_FUNC_CALLN(_c,_i,_w,2); SETVALUE(_w, _v1); SETVALUE(_w+1, _v2)
#define PREPARE_FUNC_CALL3(_c,_v1,_v2,_v3,_i,_w)    PREPARE_FUNC_CALLN(_c,_i,_w,3); SETVALUE(_w, _v1); SETVALUE(_w+1, _v2); SETVALUE(_w+2, _v3)


#define CALL_FUNC(_name,_c,r1,nargs,rwin)           gravity_fiber_t * current_fiber = fiber; \
                                                    STORE_FRAME();                                                      \
                                                    execute_op_##_name:                                                 \
                                                    switch(_c->f->tag) {                                                \
                                                    case EXEC_TYPE_NATIVE: {                                            \
                                                        current_fiber = NULL;                                           \
                                                        PUSH_FRAME(_c, &stackstart[rwin], r1, nargs);                   \
                                                    } break;                                                            \
                                                    case EXEC_TYPE_INTERNAL: {                                          \
                                                        BEGIN_TRUST_USERCODE(vm);                                       \
                                                        bool result = _c->f->U.internal(vm, &stackstart[rwin], nargs, r1);\
                                                        END_TRUST_USERCODE(vm);                                         \
                                                        if(!result) {       \
                                                            if(vm->aborted) return false;                              \
                                                            if(STACK_GET(r1).IsClosure()) {                     \
                                                                closure = VALUE_AS_CLOSURE(STACK_GET(r1));              \
                                                                SETVALUE(r1, GravityValue::from_null());                \
                                                                goto execute_op_##_name;                                \
                                                            }                                                           \
                                                            fiber = vm->fiber;                                          \
                                                            if(!fiber) return true; else if(fiber->error) RUNTIME_FIBER_ERROR(fiber->error); \
                                                        }                                                               \
                                                    } break;                                                            \
                                                    case EXEC_TYPE_BRIDGED:    {                                        \
                                                        DEBUG_ASSERT(delegate->bridge_execute, "bridge_execute delegate callback is mandatory");        \
                                                        BEGIN_TRUST_USERCODE(vm);                                       \
                                                        bool result = delegate->bridge_execute(vm, _c->f->xdata, STACK_GET(0), &stackstart[rwin], nargs, r1); \
                                                        END_TRUST_USERCODE(vm);                                         \
                                                        if(!result) {                                                  \
                                                            if(fiber->error) RUNTIME_FIBER_ERROR(fiber->error);        \
                                                        }                                                               \
                                                    } break;                                                            \
                                                    case EXEC_TYPE_SPECIAL:                                             \
                                                        RUNTIME_ERROR("Unable to handle a special function in current context");    \
                                                        break;                                                          \
                                                    }                                                                   \
                                                    LOAD_FRAME();                                                       \
                                                    SYNC_STACKTOP(current_fiber, fiber, stacktopdelta)

// MACROS used in core and optionals
//#define SETMETA_INITED(c)                           gravity_class_get_meta(c)->is_inited = true
#define SETMETA_INITED(c)                           (gravity_class_get_meta(c)->Flags |= GravityObjectBase::fIsInited)
//#define GET_VALUE(_idx)                             args[_idx]
// (replaced with gravity_vm::ReturnValue) #define RETURN_VALUE(_v,_i)                         do { vm->SetSlot(_v, _i); return true; } while(0)
// (replaced with gravity_vm::ReturnClosure) #define RETURN_CLOSURE(_v,_i)                       do { vm->SetSlot(_v, _i); return false; } while(0)
// (replaced with gravity_vm::ReturnFiber) #define RETURN_FIBER()                              return false
// (replaced with gravity_vm::ReturnNoValue()) #define RETURN_NOVALUE()                            return true
/* replaced with gravity_return_error and gravity_return_errorv
#define RETURN_ERROR(...)                           do {                                                           \
                                                        char buffer[4096];                                         \
                                                        snprintf(buffer, sizeof(buffer), __VA_ARGS__);             \
                                                        gravity_fiber_seterror(vm->GetFiber(), buffer);      \
                                                        vm->SetSlot(GravityValue::from_null(), rindex); \
                                                        return false;                                              \
                                                    } while(0)*/
//#define RETURN_ERROR_SIMPLE() do { vm->SetSlot(GravityValue::from_null(), rindex); return false; } while(0)
//#define CHECK_MEM_ALLOC(_ptr)                       if(!_ptr) return vm->ReturnErrorSimple(rindex);
//#define DECLARE_1VARIABLE(_v,_idx)                  GravityValue _v = GET_VALUE(_idx)
//#define DECLARE_2VARIABLES(_v1,_v2,_idx1,_idx2)     DECLARE_1VARIABLE(_v1,_idx1);DECLARE_1VARIABLE(_v2,_idx2)

#define NEW_FUNCTION(_fptr)                         (gravity_function_new_internal(NULL, NULL, _fptr, 0))
//#define NEW_CLOSURE_VALUE(_fptr)                    ((GravityValue){.isa = GravityEnv.P_ClsClosure,.p = (gravity_class_t *)gravity_closure_new(NULL, NEW_FUNCTION(_fptr))})
#define NEW_CLOSURE_VALUE(_fptr)                    (GravityValue(GravityEnv.P_ClsClosure, reinterpret_cast<gravity_class_t *>(gravity_closure_new(NULL, NEW_FUNCTION(_fptr)))))

#define FUNCTION_ISA_SPECIAL(_f)                    (_f->IsFunction() && (_f->tag == EXEC_TYPE_SPECIAL))
#define FUNCTION_ISA_DEFAULT_GETTER(_f)             ((_f->U.Sf.index < GRAVITY_COMPUTED_INDEX) && (_f->U.Sf.special[EXEC_TYPE_SPECIAL_GETTER] == NULL))
#define FUNCTION_ISA_DEFAULT_SETTER(_f)             ((_f->U.Sf.index < GRAVITY_COMPUTED_INDEX) && (_f->U.Sf.special[EXEC_TYPE_SPECIAL_SETTER] == NULL))
#define FUNCTION_ISA_GETTER(_f)                     (_f->U.Sf.special[EXEC_TYPE_SPECIAL_GETTER] != NULL)
#define FUNCTION_ISA_SETTER(_f)                     (_f->U.Sf.special[EXEC_TYPE_SPECIAL_SETTER] != NULL)
#define FUNCTION_ISA_BRIDGED(_f)                    (_f->index == GRAVITY_BRIDGE_INDEX)
//
//#include <gravity_opcodes.h>
/*
        Big-endian vs Little-endian machines

        ARM architecture runs both little & big endianess, but the android, iOS, and windows phone platforms run little endian.
        95% of modern desktop computers are little-endian.
        All x86 desktops (which is nearly all desktops with the demise of the PowerPC-based Macs several years ago) are little-endian.
        It's probably actually a lot more than 95% nowadays. PowerPC was the only non-x86 architecture that has been popular for desktop
        computers in the last 20 years and Apple abandoned it in favor of x86.
        Sparc, Alpha, and Itanium did exist, but they were all very rare in the desktop market.
 */

/*
        Instructions are 32bit in length

        // 2 registers and 1 register/constant
        +------------------------------------+
        |  OP  |   Ax   |   Bx   |    Cx/K   |
        +------------------------------------+

        // instructions with no parameters
        +------------------------------------+
        |  OP  |0                            |
        +------------------------------------+

        // unconditional JUMP
        +------------------------------------+
        |  OP  |             N1              |
        +------------------------------------+

        // LOADI and JUMPF
        +------------------------------------+
        |  OP  |   Ax   |S|       N2         |
        +------------------------------------+

        OP   =>  6 bits
        Ax   =>  8 bits
        Bx   =>  8 bits
        Cx/K =>  8/10 bits
        S    =>  1 bit
        N1   =>  26 bits
        N2   =>  17 bits
 */

typedef enum {

    //      ***********************************************************************************************************
    //      56 OPCODE INSTRUCTIONS (for a register based virtual machine)
    //      opcode is a 6 bit value so at maximum 2^6 = 64 opcodes can be declared
    //      ************************************************************************************************************
    //
    //      MNEMONIC        PARAMETERS          DESCRIPTION                                 OPERATION
    //      --------        ----------          ------------------------------------        ----------------------------
    //
                                                //  *** GENERAL COMMANDS (5) ***
            RET0 = 0,       //  NONE            //  return nothing from a function          MUST BE THE FIRST OPCODE (because an implicit 0 is added
                                                //                                          as a safeguard at the end of any bytecode
            HALT,           //  NONE            //  stop VM execution
            NOP,            //  NONE            //  NOP                                     http://en.wikipedia.org/wiki/NOP
            RET,            //  A               //  return from a function                  R(-1) = R(A)
            CALL,           //  A, B, C         //  call a function                         R(A) = B(C0...Cn) B is callable object and C is num args

                                                //  *** LOAD/STORE OPERATIONS (11) ***
            LOAD,           //  A, B, C         //  load C from B and store in A            R(A) = R(B)[C]
            LOADS,          //  A, B, C         //  load C from B and store in A            R(A) = R(B)[C] (super variant)
            LOADAT,         //  A, B, C         //  load C from B and store in A            R(A) = R(B)[C]
            LOADK,          //  A, B            //  load constant into register             R(A) = K(B)
            LOADG,          //  A, B            //  load global into register               R(A) = G[K(B)]
            LOADI,          //  A, B            //  load integer into register              R(A) = I
            LOADU,          //  A, B            //  load upvalue into register              R(A) = U(B)
            MOVE,           //  A, B            //  move registers                          R(A) = R(B)
            STORE,          //  A, B, C         //  store A into R(B)[C]                    R(B)[C] = R(A)
            STOREAT,        //  A, B, C         //  store A into R(B)[C]                    R(B)[C] = R(A)
            STOREG,         //  A, B            //  store global                            G[K(B)] = R(A)
            STOREU,         //  A, B            //  store upvalue                           U(B) = R(A)

                                                //  *** JUMP OPERATIONS (3) ***
            JUMP,           //  A               //  unconditional jump                      PC += A
            JUMPF,          //  A, B            //  jump if R(A) is false                   (R(A) == 0)    ? PC += B : 0
            SWITCH,         //                  //  switch statement

                                                //  *** MATH OPERATIONS (19) ***
            ADD,            //  A, B, C         //  add operation                           R(A) = R(B) + R(C)
            SUB,            //  A, B, C         //  sub operation                           R(A) = R(B) - R(C)
            DIV,            //  A, B, C         //  div operation                           R(A) = R(B) / R(C)
            MUL,            //  A, B, C         //  mul operation                           R(A) = R(B) * R(C)
            REM,            //  A, B, C         //  rem operation                           R(A) = R(B) % R(C)
            AND,            //  A, B, C         //  and operation                           R(A) = R(B) && R(C)
            OR,             //  A, B, C         //  or operation                            R(A) = R(B) || R(C)
            LT,             //  A, B, C         //  < comparison                            R(A) = R(B) < R(C)
            GT,             //  A, B, C         //  > comparison                            R(A) = R(B) > R(C)
            EQ,             //  A, B, C         //  == comparison                           R(A) = R(B) == R(C)
            LEQ,            //  A, B, C         //  <= comparison                           R(A) = R(B) <= R(C)
            GEQ,            //  A, B, C         //  >= comparison                           R(A) = R(B) >= R(C)
            NEQ,            //  A, B, C         //  != comparison                           R(A) = R(B) != R(C)
            EQQ,            //  A, B, C         //  === comparison                          R(A) = R(B) === R(C)
            NEQQ,           //  A, B, C         //  !== comparison                          R(A) = R(B) !== R(C)
            ISA,            //  A, B, C         //  isa comparison                          R(A) = R(A).class == R(B).class
            MATCH,          //  A, B, C         //  =~ pattern match                        R(A) = R(B) =~ R(C)
            NEG,            //  A, B            //  neg operation                           R(A) = -R(B)
            NOT,            //  A, B            //  not operation                           R(A) = !R(B)

                                                //  *** BIT OPERATIONS (6) ***
            LSHIFT,         //  A, B, C         //  shift left                              R(A) = R(B) << R(C)
            RSHIFT,         //  A, B, C         //  shift right                             R(A) = R(B) >> R(C)
            BAND,           //  A, B, C         //  bit and                                 R(A) = R(B) & R(C)
            BOR,            //  A, B, C         //  bit or                                  R(A) = R(B) | R(C)
            BXOR,           //  A, B, C         //  bit xor                                 R(A) = R(B) ^ R(C)
            BNOT,           //  A, B            //  bit not                                 R(A) = ~R(B)

                                                //  *** ARRAY/MAP/RANGE OPERATIONS (4) ***
            MAPNEW,         //  A, B            //  create a new map                        R(A) = Alloc a MAP(B)
            LISTNEW,        //  A, B            //  create a new array                      R(A) = Alloc a LIST(B)
            RANGENEW,       //  A, B, C, f      //  create a new range                      R(A) = Alloc a RANGE(B,C) f flag tells if B inclusive or exclusive
            SETLIST,        //  A, B, C         //  set list/map items

                                                //  *** CLOSURES (2) ***
            CLOSURE,        //  A, B            //  create a new closure                    R(A) = closure(K(B))
            CLOSE,          //  A               //  close all upvalues from R(A)

                                                //  *** UNUSED (6) ***
            CHECK,          //  A               //  checkpoint for structs                  R(A) = R(A).clone (if A is a struct)
            RESERVED2,      //                  //  reserved for future use
            RESERVED3,      //                  //  reserved for future use
            RESERVED4,      //                  //  reserved for future use
            RESERVED5,      //                  //  reserved for future use
            RESERVED6       //                  //  reserved for future use
} opcode_t;

#define GRAVITY_LATEST_OPCODE           RESERVED6    // used in some debug code so it is very useful to define the latest opcode here

enum GRAVITY_VTABLE_INDEX {
    GRAVITY_NOTFOUND_INDEX = 0,
    GRAVITY_ADD_INDEX,
    GRAVITY_SUB_INDEX,
    GRAVITY_DIV_INDEX,
    GRAVITY_MUL_INDEX,
    GRAVITY_REM_INDEX,
    GRAVITY_AND_INDEX,
    GRAVITY_OR_INDEX,
    GRAVITY_CMP_INDEX,
    GRAVITY_EQQ_INDEX,
    GRAVITY_IS_INDEX,
    GRAVITY_MATCH_INDEX,
    GRAVITY_NEG_INDEX,
    GRAVITY_NOT_INDEX,
    GRAVITY_LSHIFT_INDEX,
    GRAVITY_RSHIFT_INDEX,
    GRAVITY_BAND_INDEX,
    GRAVITY_BOR_INDEX,
    GRAVITY_BXOR_INDEX,
    GRAVITY_BNOT_INDEX,
    GRAVITY_LOAD_INDEX,
    GRAVITY_LOADS_INDEX,
    GRAVITY_LOADAT_INDEX,
    GRAVITY_STORE_INDEX,
    GRAVITY_STOREAT_INDEX,
    GRAVITY_INT_INDEX,
    GRAVITY_FLOAT_INDEX,
    GRAVITY_BOOL_INDEX,
    GRAVITY_STRING_INDEX,
    GRAVITY_EXEC_INDEX,
    GRAVITY_VTABLE_SIZE // MUST BE LAST ENTRY IN THIS ENUM
};

#define GRAVITY_OPERATOR_ADD_NAME       "+"
#define GRAVITY_OPERATOR_SUB_NAME       "-"
#define GRAVITY_OPERATOR_DIV_NAME       "/"
#define GRAVITY_OPERATOR_MUL_NAME       "*"
#define GRAVITY_OPERATOR_REM_NAME       "%"
#define GRAVITY_OPERATOR_AND_NAME       "&&"
#define GRAVITY_OPERATOR_OR_NAME        "||"
#define GRAVITY_OPERATOR_CMP_NAME       "=="
#define GRAVITY_OPERATOR_EQQ_NAME       "==="
#define GRAVITY_OPERATOR_NEQQ_NAME      "!=="
#define GRAVITY_OPERATOR_IS_NAME        "is"
#define GRAVITY_OPERATOR_MATCH_NAME     "=~"
#define GRAVITY_OPERATOR_NEG_NAME       "neg"
#define GRAVITY_OPERATOR_NOT_NAME        "!"
#define GRAVITY_OPERATOR_LSHIFT_NAME    "<<"
#define GRAVITY_OPERATOR_RSHIFT_NAME    ">>"
#define GRAVITY_OPERATOR_BAND_NAME      "&"
#define GRAVITY_OPERATOR_BOR_NAME       "|"
#define GRAVITY_OPERATOR_BXOR_NAME      "^"
#define GRAVITY_OPERATOR_BNOT_NAME      "~"
#define GRAVITY_INTERNAL_LOAD_NAME      "load"
#define GRAVITY_INTERNAL_LOADS_NAME     "loads"
#define GRAVITY_INTERNAL_STORE_NAME     "store"
#define GRAVITY_INTERNAL_LOADAT_NAME    "loadat"
#define GRAVITY_INTERNAL_STOREAT_NAME   "storeat"
#define GRAVITY_INTERNAL_NOTFOUND_NAME  "notfound"
#define GRAVITY_INTERNAL_EXEC_NAME      "exec"
#define GRAVITY_INTERNAL_LOOP_NAME      "loop"

#define GRAVITY_CLASS_INT_NAME          "Int"
#define GRAVITY_CLASS_FLOAT_NAME        "Float"
#define GRAVITY_CLASS_BOOL_NAME         "Bool"
#define GRAVITY_CLASS_STRING_NAME       "String"
#define GRAVITY_CLASS_OBJECT_NAME       "Object"
#define GRAVITY_CLASS_CLASS_NAME        "Class"
#define GRAVITY_CLASS_NULL_NAME         "Null"
#define GRAVITY_CLASS_FUNCTION_NAME     "Func"
#define GRAVITY_CLASS_FIBER_NAME        "Fiber"
#define GRAVITY_CLASS_INSTANCE_NAME     "Instance"
#define GRAVITY_CLASS_CLOSURE_NAME      "Closure"
#define GRAVITY_CLASS_LIST_NAME         "List"
#define GRAVITY_CLASS_MAP_NAME          "Map"
#define GRAVITY_CLASS_RANGE_NAME        "Range"
#define GRAVITY_CLASS_UPVALUE_NAME      "Upvalue"

#define GRAVITY_CLASS_SYSTEM_NAME       "System"
#define GRAVITY_SYSTEM_PRINT_NAME       "print"
#define GRAVITY_SYSTEM_PUT_NAME         "put"
#define GRAVITY_SYSTEM_NANOTIME_NAME    "nanotime"

#define GRAVITY_TOCLASS_NAME            "toClass"
#define GRAVITY_TOSTRING_NAME           "toString"
#define GRAVITY_TOINT_NAME              "toInt"
#define GRAVITY_TOFLOAT_NAME            "toFloat"
#define GRAVITY_TOBOOL_NAME             "toBool"
//
//#include <gravity_debug.h>
const char * opcode_constname(int n);
const char * opcode_name(opcode_t op);
const char * gravity_disassemble(gravity_vm *vm, gravity_function_t *f, const char *bcode, uint32 blen, bool deserialize);
//
//#include <gravity_token.h>
//    ================
//    PREFIX OPERATORS
//    ================
//    +         Unary PLUS
//    -         Unary MINUS
//    !         Logical NOT
//    ~         Bitwise NOT

//    ================
//    INFIX OPERATORS
//    ================
//    <<        Bitwise left shift (160)
//    >>        Bitwise right shift (160)
//    *         Multiply (150) (associativity left)
//    /         Divide (150) (associativity left)
//    %         Remainder (150) (associativity left)
//    &         Bitwise AND (150) (associativity left)
//    +         Add (140) (associativity left)
//    -         Subtract (140) (associativity left)
//    |         Bitwise OR (140) (associativity left)
//    ^         Bitwise XOR (140) (associativity left)
//    ..<       Half-open range (135)
//    ...       Closed range (135)
//    is        Type check (132)
//    <         Less than (130)
//    <=        Less than or equal (130)
//    >         Greater than (130)
//    >=        Greater than or equal (130)
//    ==        Equal (130)
//    !=        Not equal (130)
//    ===       Identical (130)
//    !==       Not identical (130)
//    ~=        Pattern match (130)
//    &&        Logical AND (120) (associativity left)
//    ||        Logical OR (110) (associativity left)
//    ?:        Ternary conditional (100) (associativity right)
//    =         Assign (90) (associativity right)
//    *=        Multiply and assign (90) (associativity right)
//    /=        Divide and assign (90) (associativity right)
//    %=        Remainder and assign (90) (associativity right)
//    +=        Add and assign (90) (associativity right)
//    -=        Subtract and assign (90) (associativity right)
//    <<=       Left bit shift and assign (90) (associativity right)
//    >>=       Right bit shift and assign (90) (associativity right)
//    &=        Bitwise AND and assign (90) (associativity right)
//    ^=        Bitwise XOR and assign (90) (associativity right)
//    |=        Bitwise OR and assign (90) (associativity right)

enum gtoken_t {
	// General (8)
	TOK_EOF    = 0, 
	TOK_ERROR, 
	TOK_COMMENT, 
	TOK_STRING, 
	TOK_NUMBER, 
	TOK_IDENTIFIER, 
	TOK_SPECIAL, 
	TOK_MACRO,
	// Keywords (36)
	// remember to keep in sync functions token_keywords_indexes and token_name
	TOK_KEY_FUNC, 
	TOK_KEY_SUPER, 
	TOK_KEY_DEFAULT, 
	TOK_KEY_TRUE, 
	TOK_KEY_FALSE, 
	TOK_KEY_IF,
	TOK_KEY_ELSE, 
	TOK_KEY_SWITCH, 
	TOK_KEY_BREAK, 
	TOK_KEY_CONTINUE, 
	TOK_KEY_RETURN, 
	TOK_KEY_WHILE,
	TOK_KEY_REPEAT, 
	TOK_KEY_FOR, 
	TOK_KEY_IN, 
	TOK_KEY_ENUM, 
	TOK_KEY_CLASS, 
	TOK_KEY_STRUCT, 
	TOK_KEY_PRIVATE,
	TOK_KEY_FILE, 
	TOK_KEY_INTERNAL, 
	TOK_KEY_PUBLIC, 
	TOK_KEY_STATIC, 
	TOK_KEY_EXTERN, 
	TOK_KEY_LAZY, 
	TOK_KEY_CONST,
	TOK_KEY_VAR, 
	TOK_KEY_MODULE, 
	TOK_KEY_IMPORT, 
	TOK_KEY_CASE, 
	TOK_KEY_EVENT, 
	TOK_KEY_NULL, 
	TOK_KEY_UNDEFINED,
	TOK_KEY_ISA, 
	TOK_KEY_CURRFUNC, 
	TOK_KEY_CURRARGS,

	// Operators (36)
	TOK_OP_SHIFT_LEFT, 
	TOK_OP_SHIFT_RIGHT, 
	TOK_OP_MUL, 
	TOK_OP_DIV, 
	TOK_OP_REM, 
	TOK_OP_BIT_AND, 
	TOK_OP_ADD, 
	TOK_OP_SUB,
	TOK_OP_BIT_OR, 
	TOK_OP_BIT_XOR, 
	TOK_OP_BIT_NOT, 
	TOK_OP_RANGE_EXCLUDED, 
	TOK_OP_RANGE_INCLUDED, 
	TOK_OP_LESS, 
	TOK_OP_LESS_EQUAL,
	TOK_OP_GREATER, 
	TOK_OP_GREATER_EQUAL, 
	TOK_OP_ISEQUAL, 
	TOK_OP_ISNOTEQUAL, 
	TOK_OP_ISIDENTICAL, 
	TOK_OP_ISNOTIDENTICAL,
	TOK_OP_PATTERN_MATCH, 
	TOK_OP_AND, 
	TOK_OP_OR, 
	TOK_OP_TERNARY, 
	TOK_OP_ASSIGN, 
	TOK_OP_MUL_ASSIGN, 
	TOK_OP_DIV_ASSIGN,
	TOK_OP_REM_ASSIGN, 
	TOK_OP_ADD_ASSIGN, 
	TOK_OP_SUB_ASSIGN, 
	TOK_OP_SHIFT_LEFT_ASSIGN, 
	TOK_OP_SHIFT_RIGHT_ASSIGN,
	TOK_OP_BIT_AND_ASSIGN, 
	TOK_OP_BIT_OR_ASSIGN, 
	TOK_OP_BIT_XOR_ASSIGN, 
	TOK_OP_NOT,
	// Punctuators (10)
	TOK_OP_SEMICOLON, 
	TOK_OP_OPEN_PARENTHESIS, 
	TOK_OP_COLON, 
	TOK_OP_COMMA, 
	TOK_OP_DOT, 
	TOK_OP_CLOSED_PARENTHESIS,
	TOK_OP_OPEN_SQUAREBRACKET, 
	TOK_OP_CLOSED_SQUAREBRACKET, 
	TOK_OP_OPEN_CURLYBRACE, 
	TOK_OP_CLOSED_CURLYBRACE,
	TOK_END // Mark end of tokens (1)
};

enum gliteral_t {
	LITERAL_STRING, 
	LITERAL_FLOAT, 
	LITERAL_INT, 
	LITERAL_BOOL, 
	LITERAL_STRING_INTERPOLATED,
	// @sobolev @construction {
	LITERAL_DATE,      // date '2020-14-03' || d '2020-14-03'
	LITERAL_TIME,      // time '12:46:30' || t '12:46:30'
	LITERAL_TIMESTAMP, // ts '2020-14-03T12:46:30'
	LITERAL_GUID       // guid '4FC737F1-C7A5-4376-A066-2A32D752A2FF'
	// } @construction 
};

struct gtoken_s {
	gtoken_s() : type(TOK_EOF), lineno(0), colno(0), position(0), bytes(0), length(0), fileid(0), builtin(BUILTIN_NONE), value(0)
	{
	}
	gtoken_s(gtoken_t t) : type(t), lineno(0), colno(0), position(0), bytes(0), length(0), fileid(0), builtin(BUILTIN_NONE), value(0)
	{
	}
	gtoken_t type;      // enum based token type
	uint32 lineno;      // token line number (1-based)
	uint32 colno;       // token column number (0-based) at the end of the token
	uint32 position;    // offset of the first character of the token
	uint32 bytes;       // token length in bytes
	uint32 length;      // token length (UTF-8)
	uint32 fileid;      // token file id
	enum gbuiltin_t {
		BUILTIN_NONE,
		BUILTIN_LINE,
		BUILTIN_COLUMN,
		BUILTIN_FILE,
		BUILTIN_FUNC,
		BUILTIN_CLASS
	};
	gbuiltin_t builtin; // builtin special identifier flag
	const char * value; // token value (not null terminated)
};

//typedef struct gtoken_s gtoken_s;

//#define NO_TOKEN                (gtoken_s){0, 0, 0, 0, 0, 0, 0, 0, NULL}
#define NO_TOKEN                gtoken_s()
//#define UNDEF_TOKEN             (gtoken_s){TOK_KEY_UNDEFINED, 0, 0, 0, 0, 0, 0, 0, NULL}
#define UNDEF_TOKEN              gtoken_s(TOK_KEY_UNDEFINED)
#define TOKEN_BYTES(_tok)       _tok.bytes
#define TOKEN_VALUE(_tok)       _tok.value

const char * FASTCALL token_string(gtoken_s token, uint32 * len);
const char * FASTCALL token_name(gtoken_t token);
gtoken_t token_keyword(const char * buffer, int32 len);
gtoken_t token_special_builtin(gtoken_s * token);
void   token_keywords_indexes(uint32 * idx_start, uint32 * idx_end);
const  char * token_literal_name(gliteral_t value);
bool   token_islabel_statement(gtoken_t token);
bool   token_isflow_statement(gtoken_t token);
bool   token_isloop_statement(gtoken_t token);
bool   token_isjump_statement(gtoken_t token);
bool   token_iscompound_statement(gtoken_t token);
bool   token_isdeclaration_statement(gtoken_t token);
bool   token_isempty_statement(gtoken_t token);
bool   token_isimport_statement(gtoken_t token);
bool   token_isspecial_statement(gtoken_t token);
bool   token_isoperator(gtoken_t token);
bool   token_ismacro(gtoken_t token);
bool   token_iserror(gtoken_t token);
bool   token_iseof(gtoken_t token);
bool   token_isidentifier(gtoken_t token);
bool   token_isvariable_declaration(gtoken_t token);
bool   token_isstatement(gtoken_t token);
bool   token_isassignment(gtoken_t token);
bool   token_isvariable_assignment(gtoken_t token);
bool   token_isaccess_specifier(gtoken_t token);
bool   token_isstorage_specifier(gtoken_t token);
bool   token_isprimary_expression(gtoken_t token);
bool   token_isexpression_statement(gtoken_t token);
//
//#include <gravity_ast.h>
/*
    AST can be uniform (the same data struct is used for all expressions/statements/declarations) or
    non-uniform. I choosed a non-uniform AST node implementation with a common base struct.
    It requires more work but design and usage is much more cleaner and we benefit from static check.
 */
enum gnode_n {
	// statements: 7
	NODE_LIST_STAT, 
	NODE_COMPOUND_STAT, 
	NODE_LABEL_STAT, 
	NODE_FLOW_STAT, 
	NODE_JUMP_STAT, 
	NODE_LOOP_STAT, 
	NODE_EMPTY_STAT,
	// declarations: 6
	NODE_ENUM_DECL, 
	NODE_FUNCTION_DECL, 
	NODE_VARIABLE_DECL, 
	NODE_CLASS_DECL, 
	NODE_MODULE_DECL, 
	NODE_VARIABLE,
	// expressions: 8
	NODE_BINARY_EXPR, 
	NODE_UNARY_EXPR, 
	NODE_FILE_EXPR, 
	NODE_LIST_EXPR, 
	NODE_LITERAL_EXPR, 
	NODE_IDENTIFIER_EXPR,
	NODE_POSTFIX_EXPR, 
	NODE_KEYWORD_EXPR,
	// postfix subexpression type
	NODE_CALL_EXPR, 
	NODE_SUBSCRIPT_EXPR, 
	NODE_ACCESS_EXPR
};

enum gnode_location_type {
	LOCATION_LOCAL,
	LOCATION_GLOBAL,
	LOCATION_UPVALUE,
	LOCATION_CLASS_IVAR_SAME,
	LOCATION_CLASS_IVAR_OUTER
};

// BASE NODE
struct gnode_t {
	//#define ISA(n1, _tag)                    ((n1) ? ((n1)->Tag == _tag) : 0)
	static bool IsA(const gnode_t * pN, gnode_n tag) { return pN ? (pN->Tag == tag) : false; }
	gnode_t() : Tag(static_cast<gnode_n>(0)), RefCount(0), BlockLength(0), IsAssignment(false), P_Decl(0)
	{
	}
	gnode_t(gnode_n tag, gtoken_s tok, void * pDecl) : Tag(tag), Token(tok), P_Decl(pDecl), RefCount(0), BlockLength(0), IsAssignment(false)
	{
	}
	gnode_n GetTag() const { return Tag; }
	gtoken_t GetTokenType() const { return Token.type; }
	void Setup(gnode_n _tag, gtoken_s _token, void * pDecl)
	{
		Tag = _tag;
		Token = _token;
		P_Decl = pDecl;
	}
	gnode_n Tag;           // node type from gnode_n enum
	uint32 RefCount;     // reference count to manage duplicated nodes
	uint32 BlockLength; // total length in bytes of the block (used in autocompletion)
	gtoken_s Token;        // token type and location
	bool   IsAssignment;    // flag to check if it is an assignment node
	uint8  Reserve[3];      // @alignment 
	void * P_Decl;           // enclosing declaration node
};

// UPVALUE STRUCT
struct gupvalue_t {
	gnode_t * node;     // reference to the original var node
	uint32 index;     // can be an index in the stack or in the upvalue list (depending on the is_direct flag)
	uint32 selfindex; // always index inside uplist
	bool   is_direct;     // flag to check if var is local to the direct enclosing func
	uint8  Reserve[3];  // @alignment
};

// shortcut for array of common structs
//typedef marray_t(gnode_t *)                 gnode_r;
//typedef marray_t(gupvalue_t *)              gupvalue_r;
//typedef GravityArray <gnode_t *> gnode_r;
//typedef GravityArray <gupvalue_t *> gupvalue_r;

#ifndef GRAVITY_SYMBOLTABLE_DEFINED
	#define GRAVITY_SYMBOLTABLE_DEFINED
	typedef struct symboltable_t symboltable_t;
#endif

// LOCATION
struct gnode_location_t {
	gnode_location_type type;           // location type
	uint16 index;                     // symbol index
	uint16 nup;                       // upvalue index or outer index
};

// STATEMENTS
struct gnode_compound_stmt_t : public gnode_t {
	//gnode_t base;                     // NODE_LIST_STAT | NODE_COMPOUND_STAT
	symboltable_t * symtable;         // node internal symbol table
	GravityArray <gnode_t *> * stmts; // array of statements node
	uint32 nclose;                    // initialized to UINT32_MAX
};

typedef gnode_compound_stmt_t gnode_list_stmt_t;

struct gnode_label_stmt_t : public gnode_t {
	//gnode_t base;                       // CASE or DEFAULT
	gnode_t * expr;         // expression in case of CASE
	gnode_t * stmt;         // common statement
};

struct gnode_flow_stmt_t : public gnode_t {
	//gnode_t base;                       // IF, SWITCH, TOK_OP_TERNARY
	gnode_t * cond;         // common condition (it's an expression)
	gnode_t * stmt;         // common statement
	gnode_t * elsestmt;     // optional else statement in case of IF
};

struct gnode_loop_stmt_t : public gnode_t {
	//gnode_t base;                       // WHILE, REPEAT or FOR
	gnode_t * cond;         // used in WHILE and FOR
	gnode_t * stmt;         // common statement
	gnode_t * expr;         // used in REPEAT and FOR
	uint32 nclose;                    // initialized to UINT32_MAX
};

struct gnode_jump_stmt_t : public gnode_t {
	gnode_jump_stmt_t(gtoken_s tok, void * pDecl, gnode_t * pExpr) : gnode_t(NODE_JUMP_STAT, tok, pDecl), expr(pExpr)
	{
	}
	//gnode_t base;                       // BREAK, CONTINUE or RETURN
	gnode_t * expr;         // optional expression in case of RETURN
};

// DECLARATIONS
struct gnode_function_decl_t : public gnode_t {
	//gnode_t base;                       // FUNCTION_DECL or FUNCTION_EXPR
	gnode_t * env;          // shortcut to node where function is declared
	gtoken_t access;                    // TOK_KEY_PRIVATE | TOK_KEY_INTERNAL | TOK_KEY_PUBLIC
	gtoken_t storage;                   // TOK_KEY_STATIC | TOK_KEY_EXTERN
	symboltable_t * symtable;     // function internal symbol table
	const char    * identifier;   // function name
	GravityArray <gnode_t *> * params;  // function params
	gnode_compound_stmt_t * block;      // internal function statements
	uint16 nlocals;                     // locals counter
	uint16 nparams;                     // formal parameters counter
	bool has_defaults;                  // flag set if parmas has default values
	bool is_closure;                    // flag to check if function is a closure
	uint8 Reserve[2];          // @alignment
	GravityArray <gupvalue_t *> * uplist;       // list of upvalues used in function (can be empty)
};

typedef gnode_function_decl_t gnode_function_expr_t;

struct gnode_variable_decl_t : public gnode_t {
	//gnode_t base;     // VARIABLE_DECL
	gtoken_t type;    // TOK_KEY_VAR | TOK_KEY_CONST
	gtoken_t access;  // TOK_KEY_PRIVATE | TOK_KEY_INTERNAL | TOK_KEY_PUBLIC
	gtoken_t storage; // TOK_KEY_STATIC | TOK_KEY_EXTERN
	GravityArray <gnode_t *> * decls;  // variable declarations list (gnode_var_t)
};

struct gnode_var_t : public gnode_t {
	//gnode_t base;                       // VARIABLE
	gnode_t * env;          // shortcut to node where variable is declared
	const char * identifier;   // variable name
	const char * annotation_type;// optional annotation type
	gnode_t * expr;         // optional assignment expression/declaration
	gtoken_t access;                    // optional access token (duplicated value from its gnode_variable_decl_t)
	uint16 index;                     // local variable index (if local)
	bool upvalue;                       // flag set if this variable is used as an upvalue
	bool iscomputed;                    // flag set is variable must not be backed
	gnode_variable_decl_t   * vdecl;    // reference to enclosing variable declaration (in order to be able to have access to storage and access fields)
};

struct gnode_enum_decl_t : public gnode_t {
	//gnode_t base;                       // ENUM_DECL
	gnode_t * env;          // shortcut to node where enum is declared
	gtoken_t access;                    // TOK_KEY_PRIVATE | TOK_KEY_INTERNAL | TOK_KEY_PUBLIC
	gtoken_t storage;                   // TOK_KEY_STATIC | TOK_KEY_EXTERN
	symboltable_t * symtable;     // enum internal hash table
	const char * identifier;   // enum name
};

struct gnode_class_decl_t : public gnode_t {
	//gnode_t base;              // CLASS_DECL
	bool   bridge;             // flag to check of a bridged class
	bool   is_struct;          // flag to mark the class as a struct
	bool   super_extern;       // flag set when a superclass is declared as extern
	uint8  Reserve;            // @alignment 
	gnode_t * env;             // shortcut to node where class is declared
	gtoken_t access;           // TOK_KEY_PRIVATE | TOK_KEY_INTERNAL | TOK_KEY_PUBLIC
	gtoken_t storage;          // TOK_KEY_STATIC | TOK_KEY_EXTERN
	const char * identifier;   // class name
	gnode_t    * superclass;   // super class ptr
	GravityArray <gnode_t *> * protocols; // array of protocols (currently unused)
	GravityArray <gnode_t *> * decls;     // class declarations list
	symboltable_t * symtable;  // class internal symbol table
	void * data;               // used to keep track of super classes
	uint32 nivar;              // instance variables counter
	uint32 nsvar;              // static variables counter
};

struct gnode_module_decl_t : public gnode_t {
	//gnode_t base;                       // MODULE_DECL
	gnode_t * env;          // shortcut to node where module is declared
	gtoken_t access;                    // TOK_KEY_PRIVATE | TOK_KEY_INTERNAL | TOK_KEY_PUBLIC
	gtoken_t storage;                   // TOK_KEY_STATIC | TOK_KEY_EXTERN
	const char * identifier;   // module name
	GravityArray <gnode_t *> * decls; // module declarations list
	symboltable_t * symtable;     // module internal symbol table
};

// EXPRESSIONS
struct gnode_binary_expr_t : public gnode_t {
	//gnode_t base;                       // BINARY_EXPR
	gtoken_t op;                        // operation
	gnode_t * left;         // left node
	gnode_t * right;        // right node
};

struct gnode_unary_expr_t : public gnode_t {
	//gnode_t base;                       // UNARY_EXPR
	gtoken_t op;                        // operation
	gnode_t * expr;         // node
};

struct gnode_file_expr_t : public gnode_t {
	//gnode_t base;                       // FILE
	GravityArray <const char *> * identifiers;  // identifier name
	gnode_location_t location;          // identifier location
};

struct gnode_literal_expr_t : public gnode_t {
	//gnode_t base;    // LITERAL
	gliteral_t type; // LITERAL_STRING, LITERAL_FLOAT, LITERAL_INT, LITERAL_BOOL, LITERAL_INTERPOLATION
	uint32 len;      // used only for TYPE_STRING
	union {
		char * str;  // LITERAL_STRING
		double d;    // LITERAL_FLOAT
		int64_t n64; // LITERAL_INT or LITERAL_BOOL
		GravityArray <gnode_t *> * r; // LITERAL_STRING_INTERPOLATED
	} value;
};

struct gnode_identifier_expr_t : public gnode_t {
	//gnode_t base;                       // IDENTIFIER or ID
	const char * value;        // identifier name
	const char * value2;       // NULL for IDENTIFIER (check if just one value or an array)
	gnode_t    * symbol;       // pointer to identifier declaration (if any)
	gnode_location_t location;          // location coordinates
	gupvalue_t * upvalue;      // upvalue location reference
};

struct gnode_keyword_expr_t : public gnode_t {
	//gnode_t base; // KEYWORD token
};

typedef gnode_keyword_expr_t gnode_empty_stmt_t;
typedef gnode_keyword_expr_t gnode_base_t;

struct gnode_postfix_expr_t : public gnode_t {
	//gnode_t base;                    // NODE_CALLFUNC_EXPR, NODE_SUBSCRIPT_EXPR, NODE_ACCESS_EXPR
	gnode_t * id;                    // id(...) or id[...] or id.
	GravityArray <gnode_t *> * list; // list of postfix_subexpr
};

struct gnode_postfix_subexpr_t : public gnode_t {
	//gnode_t base;                       // NODE_CALLFUNC_EXPR, NODE_SUBSCRIPT_EXPR, NODE_ACCESS_EXPR
	union {
		gnode_t * expr;                  // used in case of NODE_SUBSCRIPT_EXPR or NODE_ACCESS_EXPR
		GravityArray <gnode_t *> * args; // used in case of NODE_CALLFUNC_EXPR
	};
};

struct gnode_list_expr_t : public gnode_t {
	//gnode_t base;           // LIST_EXPR
	bool   ismap;           // flag to check if the node represents a map (otherwise it is a list)
	uint8  Reserve[3];      // @alignment
	GravityArray <gnode_t *> * list1; // node items (cannot use a symtable here because order is mandatory in array)
	GravityArray <gnode_t *> * list2; // used only in case of map
};

//gnode_t * gnode_jump_stat_create(gtoken_s token, gnode_t * expr, gnode_t * decl);
gnode_t * gnode_label_stat_create(gtoken_s token, gnode_t * expr, gnode_t * stmt, gnode_t * decl);
gnode_t * gnode_flow_stat_create(gtoken_s token, gnode_t * cond, gnode_t * stmt1, gnode_t * stmt2, gnode_t * decl, uint32 block_length);
gnode_t * gnode_loop_stat_create(gtoken_s token, gnode_t * cond, gnode_t * stmt, gnode_t * expr, gnode_t * decl, uint32 block_length);
gnode_t * gnode_block_stat_create(gnode_n type, gtoken_s token, GravityArray <gnode_t *> * stmts, gnode_t * decl, uint32 block_length);
gnode_t * gnode_empty_stat_create(gtoken_s token, gnode_t * decl);
gnode_t * gnode_enum_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier, gtoken_t storage_specifier, symboltable_t * symtable, gnode_t * decl);
gnode_t * gnode_class_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier, gtoken_t storage_specifier, gnode_t * superclass, GravityArray <gnode_t *> * protocols, GravityArray <gnode_t *> * declarations, bool is_struct, gnode_t * decl);
gnode_t * gnode_module_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier, gtoken_t storage_specifier, GravityArray <gnode_t *> * declarations, gnode_t * decl);
gnode_t * gnode_variable_decl_create(gtoken_s token, gtoken_t type, gtoken_t access_specifier, gtoken_t storage_specifier, GravityArray <gnode_t *> * declarations, gnode_t * decl);
gnode_t * gnode_variable_create(gtoken_s token, const char * identifier, const char * annotation_type, gnode_t * expr, gnode_t * decl, gnode_variable_decl_t * vdecl);
gnode_t * gnode_function_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier, gtoken_t storage_specifier, GravityArray <gnode_t *> * params, gnode_compound_stmt_t * block, gnode_t * decl);
gnode_t * gnode_binary_expr_create(gtoken_t op, gnode_t * left, gnode_t * right, gnode_t * decl);
gnode_t * gnode_unary_expr_create(gtoken_t op, gnode_t * expr, gnode_t * decl);
gnode_t * gnode_file_expr_create(gtoken_s token, GravityArray <const char *> * list, gnode_t * decl);
gnode_t * gnode_identifier_expr_create(gtoken_s token, const char * identifier, const char * identifier2, gnode_t * decl);
gnode_t * gnode_string_interpolation_create(gtoken_s token, GravityArray <gnode_t *> * r, gnode_t * decl);
gnode_t * gnode_literal_string_expr_create(gtoken_s token, char * s, uint32 len, bool allocated, gnode_t * decl);
gnode_t * gnode_literal_float_expr_create(gtoken_s token, double f, gnode_t * decl);
gnode_t * gnode_literal_int_expr_create(gtoken_s token, int64_t n, gnode_t * decl);
gnode_t * gnode_literal_bool_expr_create(gtoken_s token, int32 n, gnode_t * decl);
gnode_t * gnode_keyword_expr_create(gtoken_s token, gnode_t * decl);
gnode_t * gnode_postfix_subexpr_create(gtoken_s token, gnode_n type, gnode_t * expr, GravityArray <gnode_t *> * list, gnode_t * decl);
gnode_t * gnode_postfix_expr_create(gtoken_s token, gnode_t * id, GravityArray <gnode_t *> * list, gnode_t * decl);
gnode_t * gnode_list_expr_create(gtoken_s token, GravityArray <gnode_t *> * list1, GravityArray <gnode_t *> * list2, bool ismap, gnode_t * decl);
gnode_t * gnode_duplicate(gnode_t * node, bool deep);
GravityArray <gnode_t *> * gnode_array_create();
GravityArray <gnode_t *> * gnode_array_remove_byindex(GravityArray <gnode_t *> * list, size_t index);
gupvalue_t * gnode_function_add_upvalue(gnode_function_decl_t * f, gnode_var_t * symbol, uint16 n);
GravityArray <const char *> * cstring_array_create();
GravityArray <void *> * void_array_create();
void    gnode_array_sethead(GravityArray <gnode_t *> * list, gnode_t * node);
gnode_t * gnode2class(gnode_t * node, bool * isextern);
bool    gnode_is_equal(const gnode_t * node1, const gnode_t * node2);
bool    gnode_is_expression(const gnode_t * node);
bool    FASTCALL gnode_is_literal(const gnode_t * node);
bool    gnode_is_literal_int(const gnode_t * node);
bool    gnode_is_literal_number(gnode_t * node);
bool    gnode_is_literal_string(const gnode_t * node);
void    gnode_literal_dump(gnode_literal_expr_t * node, char * buffer, int buffersize);
void    FASTCALL gnode_free(gnode_t * node);

// MARK: -
//#define gnode_array_init(r)                 marray_init(*r)
#define gnode_array_size(r)                 ((r) ? (r)->getCount() : 0)
#define gnode_array_push(r, node)           (r)->insert(node)
#define gnode_array_pop(r)                  ((r)->getCount() ? r->pop() : NULL)
#define gnode_array_get(r, i)               (((i) >= 0 && (i) < (r)->getCount()) ? r->at(i) : NULL)
#define gnode_array_free(r)                 do { r->Z(); mem_free((void*)r); } while(0)
#define gtype_array_each(r, block, type)    { uint _len = gnode_array_size(r); for(uint _i = 0; _i < _len; ++_i) { type val = (type)gnode_array_get(r, _i); block; } }
#define gnode_array_each(r, block)          gtype_array_each(r, block, gnode_t*)
#define gnode_array_eachbase(r, block)      gtype_array_each(r, block, gnode_base_t*)
#define cstring_array_free(r)               (r)->Z()
#define cstring_array_push(r, s)            (r)->insert(s)
#define cstring_array_each(r, block)        gtype_array_each(r, block, const char*)
//#define NODE_TOKEN_TYPE(_node)              _node->Token.type
//#define NODE_TAG(_node)                     (_node)->Tag
//#define NODE_ISA(_node, _tag)               ((_node) && (_node)->GetTag() == _tag)
//#define NODE_ISA_FUNCTION(_node)            gnode_t::IsA(_node, NODE_FUNCTION_DECL)
//#define NODE_ISA_CLASS(_node)               gnode_t::IsA(_node, NODE_CLASS_DECL)
//#define NODE_SET_ENCLOSING(_node, _enc)      (((gnode_base_t *)_node)->base.enclosing = _enc)
//#define NODE_GET_ENCLOSING(_node)           ((gnode_base_t *)_node)->base.enclosing
//
//#include <gravity_visitor.h>

struct gvisitor_t {
	gvisitor_t(void * pData, void * pDelegate) : nerr(0), data(pData), bflag(0), delegate(pDelegate),
		visit_pre(0), visit_post(0), visit_list_stmt(0), visit_compound_stmt(0), visit_label_stmt(0),
		visit_flow_stmt(0), visit_jump_stmt(0), visit_loop_stmt(0), visit_empty_stmt(0), 
		visit_function_decl(0), visit_variable_decl(0), visit_enum_decl(0), visit_class_decl(0), 
		visit_module_decl(0), visit_binary_expr(0), visit_unary_expr(0), visit_file_expr(0), visit_literal_expr(0),
		visit_identifier_expr(0), visit_keyword_expr(0), visit_list_expr(0), visit_postfix_expr(0)
	{
	}
	uint32 nerr;       // to store err counter state
	void * data;       // to store a ptr state
	bool   bflag;      // to store a bool flag
	uint8  Reserve[3]; // @alignment
	void * delegate;   // delegate callback
	// COMMON
	void (* visit_pre)(gvisitor_t * self, gnode_t * node);
	void (* visit_post)(gvisitor_t * self, gnode_t * node);
	// count must be equal to enum gnode_n defined in gravity_ast.h less 3
	// STATEMENTS: 7
	void (* visit_list_stmt)(gvisitor_t * self, gnode_compound_stmt_t * node);
	void (* visit_compound_stmt)(gvisitor_t * self, gnode_compound_stmt_t * node);
	void (* visit_label_stmt)(gvisitor_t * self, gnode_label_stmt_t * node);
	void (* visit_flow_stmt)(gvisitor_t * self, gnode_flow_stmt_t * node);
	void (* visit_jump_stmt)(gvisitor_t * self, gnode_jump_stmt_t * node);
	void (* visit_loop_stmt)(gvisitor_t * self, gnode_loop_stmt_t * node);
	void (* visit_empty_stmt)(gvisitor_t * self, gnode_empty_stmt_t * node);

	// DECLARATIONS: 5+1 (NODE_VARIABLE handled by NODE_VARIABLE_DECL case)
	void (* visit_function_decl)(gvisitor_t * self, gnode_function_decl_t * node);
	void (* visit_variable_decl)(gvisitor_t * self, gnode_variable_decl_t * node);
	void (* visit_enum_decl)(gvisitor_t * self, gnode_enum_decl_t * node);
	void (* visit_class_decl)(gvisitor_t * self, gnode_class_decl_t * node);
	void (* visit_module_decl)(gvisitor_t * self, gnode_module_decl_t * node);

	// EXPRESSIONS: 7+3 (CALL EXPRESSIONS handled by one callback)
	void (* visit_binary_expr)(gvisitor_t * self, gnode_binary_expr_t * node);
	void (* visit_unary_expr)(gvisitor_t * self, gnode_unary_expr_t * node);
	void (* visit_file_expr)(gvisitor_t * self, gnode_file_expr_t * node);
	void (* visit_literal_expr)(gvisitor_t * self, gnode_literal_expr_t * node);
	void (* visit_identifier_expr)(gvisitor_t * self, gnode_identifier_expr_t * node);
	void (* visit_keyword_expr)(gvisitor_t * self, gnode_keyword_expr_t * node);
	void (* visit_list_expr)(gvisitor_t * self, gnode_list_expr_t * node);
	void (* visit_postfix_expr)(gvisitor_t * self, gnode_postfix_expr_t * node);
};

void FASTCALL gvisit(gvisitor_t * self, gnode_t * node);
//#define visit__(node) gvisit(self, node)
//
//#include <gravity_symboltable.h>
#ifndef GRAVITY_SYMBOLTABLE_DEFINED
	#define GRAVITY_SYMBOLTABLE_DEFINED
	typedef struct symboltable_t symboltable_t;
#endif

enum symtable_tag {
	SYMTABLE_TAG_GLOBAL = 0,
	SYMTABLE_TAG_FUNC = 1,
	SYMTABLE_TAG_CLASS = 2,
	SYMTABLE_TAG_MODULE = 3,
	SYMTABLE_TAG_ENUM = 4
};

symboltable_t * symboltable_create(symtable_tag tag);
gnode_t * symboltable_lookup(const symboltable_t * table, const char * identifier);
gnode_t * symboltable_global_lookup(symboltable_t * table, const char * identifier);
bool     symboltable_insert(symboltable_t * table, const char * identifier, gnode_t * node);
uint32 symboltable_count(symboltable_t * table, uint32 index);
symtable_tag symboltable_tag(symboltable_t * table);
uint16 symboltable_setivar(symboltable_t * table, bool is_static);
void     symboltable_enter_scope(symboltable_t * table);
uint32 symboltable_exit_scope(symboltable_t * table, uint32 * nlevel);
uint32 symboltable_local_index(const symboltable_t * table);
void FASTCALL symboltable_free(symboltable_t * table);
void symboltable_dump(symboltable_t * table);
void * symboltable_hash_atindex(symboltable_t * table, size_t n);
//
//#include <gravity_codegen.h>
gravity_function_t * gravity_codegen(gnode_t *node, gravity_delegate_t *delegate, gravity_vm *vm, bool add_debug);
//
//#include <gravity_optimizer.h>
gravity_function_t * gravity_optimizer(gravity_function_t *f, bool add_debug);
//
//#include <gravity_ircode.h>
// References:
// https://www.usenix.org/legacy/events/vee05/full_papers/p153-yunhe.pdf
// http://www.lua.org/doc/jucs05.pdf
//
// In a stack-based VM, a local variable is accessed using an index, and the operand stack is accessed via the stack
// pointer.
// In a register-based VM both the local variables and operand stack can be considered as virtual registers for the
// method.
// There is a simple mapping from stack locations to register numbers, because the height and contents of the VM operand
// stack
// are known at any point in a program.
//
// All values on the operand stack can be considered as temporary variables (registers) for a method and therefore are
// short-lived.
// Their scope of life is between the instructions that push them onto the operand stack and the instruction that
// consumes
// the value on the operand stack. On the other hand, local variables (also registers) are long-lived and their life
// scope is
// the time of method execution.

#define REGISTER_ERROR    UINT32_MAX

enum optag_t {
	NO_TAG = 0,
	INT_TAG,
	DOUBLE_TAG,
	LABEL_TAG,
	SKIP_TAG,
	RANGE_INCLUDE_TAG,
	RANGE_EXCLUDE_TAG,
	PRAGMA_MOVE_OPTIMIZATION
};

struct inst_t {
	inst_t(opcode_t aOp, optag_t aTag, int32 _p1, int32 _p2, int32 _p3, uint32 ln) : op(aOp), tag(aTag), p1(_p1), p2(_p2), p3(_p3), lineno(ln)
	{
		n = 0;
	}
	opcode_t op;
	optag_t tag;
	int32 p1;
	int32 p2;
	int32 p3;
	union {
		double d; //    tag is DOUBLE_TAG
		int64_t n; //    tag is INT_TAG
	};
	uint32 lineno;    //  debug info
};

ircode_t * ircode_create(uint16 nlocals);
void   ircode_free(ircode_t * code);
uint32 ircode_count(const ircode_t * code);
uint32 ircode_ntemps(const ircode_t * code);
inst_t * ircode_get(ircode_t * code, uint32 index);
void   ircode_dump(const void * code); // code_dump_function
void   ircode_push_context(ircode_t * code);
void   ircode_pop_context(ircode_t * code);
bool   ircode_iserror(const ircode_t * code);
void   ircode_patch_init(ircode_t * code, uint16 index);
uint32 FASTCALL ircode_newlabel(ircode_t * code);
void   ircode_setlabel_true(ircode_t * code, uint32 nlabel);
void   ircode_setlabel_false(ircode_t * code, uint32 nlabel);
void   ircode_setlabel_check(ircode_t * code, uint32 nlabel);
void   ircode_unsetlabel_true(ircode_t * code);
void   ircode_unsetlabel_false(ircode_t * code);
void   ircode_unsetlabel_check(ircode_t * code);
uint32    ircode_getlabel_true(ircode_t * code);
uint32    ircode_getlabel_false(ircode_t * code);
uint32    ircode_getlabel_check(ircode_t * code);
void ircode_marklabel(ircode_t * code, uint32 nlabel, uint32 lineno);
void    FASTCALL inst_setskip(inst_t * inst);
uint8_t FASTCALL opcode_numop(opcode_t op);
void ircode_pragma(ircode_t * code, optag_t tag, uint32 value, uint32 lineno);
void FASTCALL ircode_add(ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3, uint32 lineno);
void FASTCALL ircode_add_tag(ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3, optag_t tag, uint32 lineno);
//void ircode_add_array(ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3, GravityArray <uint32> r, uint32 lineno);
void ircode_add_double(ircode_t * code, double d, uint32 lineno);
void ircode_add_int(ircode_t * code, int64_t n, uint32 lineno);
void FASTCALL ircode_add_constant(ircode_t * code, uint32 index, uint32 lineno);
void ircode_add_skip(ircode_t * code, uint32 lineno);
void ircode_set_index(uint32 index, ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3);
void ircode_add_check(ircode_t * code);
// IMPORTANT NOTE
//
// The following functions can return REGISTER_ERROR and so an error check is mandatory
// ircode_register_pop
// ircode_register_pop_context_protect
// ircode_register_last
//
// The following functions can return 0 if no temp registers are available
// ircode_register_push_temp
//
bool   ircode_register_istemp(const ircode_t * code, uint32 n);
uint32 FASTCALL ircode_register_push_temp(ircode_t * code);
uint32 ircode_register_push_temp_protected(ircode_t * code);
uint32 ircode_register_push(ircode_t * code, uint32 nreg);
uint32 FASTCALL ircode_register_pop(ircode_t * code);
uint32 ircode_register_first_temp_available(ircode_t * code);
uint32 ircode_register_pop_context_protect(ircode_t * code, bool protect);
bool   ircode_register_protect_outside_context(ircode_t * code, uint32 nreg);
void   ircode_register_protect_in_context(ircode_t * code, uint32 nreg);
uint32 ircode_register_last(ircode_t * code);
uint32 ircode_register_count(const ircode_t * code);
void   ircode_register_clear(ircode_t * code, uint32 nreg);
void   ircode_register_set(ircode_t * code, uint32 nreg);
void   ircode_register_dump(ircode_t * code);
void   ircode_register_temp_protect(ircode_t * code, uint32 nreg);
void   ircode_register_temp_unprotect(ircode_t * code, uint32 nreg);
void   ircode_register_temps_clear(ircode_t * code);
//
//#include <gravity_compiler.h>
typedef struct gravity_compiler_t   gravity_compiler_t; // opaque compiler data type

GRAVITY_API gravity_compiler_t * gravity_compiler_create(gravity_delegate_t *delegate);
GRAVITY_API gravity_closure_t  * gravity_compiler_run(gravity_compiler_t *compiler, const char *source, size_t len, uint32 fileid, bool is_static, bool add_debug);
GRAVITY_API GravityJson  * gravity_compiler_serialize(gravity_compiler_t *compiler, gravity_closure_t *closure);
GRAVITY_API bool    gravity_compiler_serialize_infile(gravity_compiler_t *compiler, gravity_closure_t *closure, const char *path);
GRAVITY_API void    gravity_compiler_transfer(gravity_compiler_t *compiler, gravity_vm *vm);
GRAVITY_API gnode_t * gravity_compiler_ast(gravity_compiler_t *compiler);
GRAVITY_API void    gravity_compiler_free(gravity_compiler_t *compiler);
//
//#include <gravity_parser.h>
//
// Parser is responsible to build the AST, convert strings and number from tokens and
// implement syntax error recovery strategy.
//
// Notes about error recovery:
// Each parse* function can return NULL in case of error but each function is RESPONSIBLE
// to make appropriate actions in order to handle/recover errors.
//
// Error recovery techniques can be:
// Shallow Error Recovery
// Deep Error Recovery
// https://javacc.java.net/doc/errorrecovery.html
//
// opaque datatype
typedef struct gravity_parser_t    gravity_parser_t;

// public functions
gravity_parser_t * gravity_parser_create (const char *source, size_t len, uint32 fileid, bool is_static);
gnode_t * gravity_parser_run (gravity_parser_t *parser, gravity_delegate_t *delegate);
void gravity_parser_free (gravity_parser_t *parser);
//
//#include <gravity_semacheck1.h>
// Semantic check step 1 does not have the notion of context and scope.
// It just gathers non-local names into a symbol table and check for uniqueness.
//
// Only declarations (non-locals) are visited and a symbol table is created.
//
//
// In order to debug symbol table just enable the GRAVITY_SYMTABLE_DEBUG macro
// in debug_macros.h

// Semantic Check Step 1 enables to resolve cases like:
/*
        function foo() {
            return bar();
        }

        function bar() {
            ...
        }

        or

        class foo:bar {
            ...
        }

        class bar {
            ...
        }

        and

        class foo {
            var a;
            function bar() {
                return a + b;
            }
            var b;
        }
 */
bool gravity_semacheck1(gnode_t * node, gravity_delegate_t * delegate);
//
//#include <gravity_semacheck2.h>
// Responsible to gather and check local identifiers
// Complete check for all identifiers and report not found errors
bool gravity_semacheck2(gnode_t * node, gravity_delegate_t * delegate);
/*

    The following table summarizes what can be defined inside a declaration:

    -------+---------------------------------------------------------+
           |   func   |   var   |   enum   |   class   |   module    |
    -------+---------------------------------------------------------+
    func   |   YES    |   YES   |   NO     |   YES     |   YES       |
    -------+---------------------------------------------------------+
    var    |   YES    |   NO    |   NO     |   YES     |   YES       |
    -------+---------------------------------------------------------+
    enum   |   YES    |   NO    |   NO     |   YES     |   YES       |
    -------+---------------------------------------------------------+
    class  |   YES    |   NO    |   NO     |   YES     |   YES       |
    -------+---------------------------------------------------------+
    module |   NO     |   NO    |   NO     |   NO      |   NO        |
    -------+---------------------------------------------------------+

    Everything declared inside a func is a local, so for example:

    func foo {
        func a...;
        enum b...;
        class c..;
    }

    is converted by codegen to:

    func foo {
        var a = func...;
        var b = enum...;
        var c = class..;
    }

    Even if the ONLY valid syntax is anonymous func assignment, user will not be able
    to assign an anonymous enum or class to a variable. Restriction is applied by parser
    and reported as a syntax error.
    Define a module inside a function is not allowed (no real technical reason but I found
    it a very bad programming practice), restriction is applied by samantic checker.
 */
// TECH NOTE:
// At the end of semacheck2:
//
// Each declaration and compound statement will have its own symbol table (symtable field)
// symtable in:
// NODE_LIST_STAT and NODE_COMPOUND_STAT
// FUNCTION_DECL and FUNCTION_EXPR
// ENUM_DECL
// CLASS_DECL
// MODULE_DECL
//
// Each identifier will have a reference to its declaration (symbol field)
// symbol field in:
// NODE_FILE
// NODE_IDENTIFIER
// NODE_ID
//
// Each declaration will have a reference to its enclosing declaration (env field)
// env field in:
// FUNCTION_DECL and FUNCTION_EXPR
// VARIABLE
// ENUM_DECL
// CLASS_DECL
// MODULE_DECL
//
//#include <gravity_lexer.h>
//
// Lexer is built in such a way that no memory allocations are necessary during usage
// (except for the gravity_lexer_t opaque datatype allocated within gravity_lexer_create).
//
// Example:
// gravity_lexer *lexer = gravity_lexer_create(...);
// while(gravity_lexer_next(lexer)) {
//    // do something here
// }
// gravity_lexer_free(lexer);
//
// gravity_lexer_next (and gravity_lexer_peek) returns an int token (gtoken_t)
// which represents what has been currently scanned. When EOF is reached TOK_EOF is
// returned (with value 0) and the while loop exits.
//
// In order to have token details, gravity_lexer_token must be called.
// In case of a scan error TOK_ERROR is returned and error details can be extracted
// from the token itself. In order to be able to not allocate any memory during
// tokenization STRINGs and NUMBERs are just sanity checked but not converted.
// It is parser responsability to perform the right conversion.
//
// opaque datatype
typedef struct gravity_lexer_t gravity_lexer_t;

// public functions
gravity_lexer_t * gravity_lexer_create(const char * source, size_t len, uint32 fileid, bool is_static);
void gravity_lexer_setdelegate(gravity_lexer_t * lexer, gravity_delegate_t * delegate);
void gravity_lexer_free(gravity_lexer_t * lexer);
gtoken_t FASTCALL gravity_lexer_peek(gravity_lexer_t * lexer);
gtoken_t FASTCALL gravity_lexer_next(gravity_lexer_t * lexer);
gtoken_s FASTCALL gravity_lexer_token(const gravity_lexer_t * lexer);
gtoken_s FASTCALL gravity_lexer_token_next(const gravity_lexer_t * lexer);
gtoken_t FASTCALL gravity_lexer_token_type(const gravity_lexer_t * lexer);
void gravity_lexer_token_dump(gtoken_s token);
void gravity_lexer_skip_line(gravity_lexer_t * lexer);
uint32 FASTCALL gravity_lexer_lineno(const gravity_lexer_t * lexer);
#if GRAVITY_LEXER_DEBUG
	void gravity_lexer_debug(gravity_lexer_t * lexer);
#endif
//
//#include <gravity_opt_json.h>
#define GRAVITY_CLASS_JSON_NAME         "JSON"

void gravity_json_register(gravity_vm * vm);
void gravity_json_free();
bool gravity_isjson_class(const gravity_class_t * c);
const char * gravity_json_name();
//
//#include <gravity_opt_math.h>
#define GRAVITY_CLASS_MATH_NAME             "Math"

void gravity_math_register(gravity_vm *vm);
void gravity_math_free();
bool gravity_ismath_class(const gravity_class_t *c);
const char * gravity_math_name();
//
//#include <gravity_opt_env.h>
#define GRAVITY_CLASS_ENV_NAME "ENV"

void gravity_env_register(gravity_vm *vm);
void gravity_env_free();
bool gravity_isenv_class(const gravity_class_t *c);
const char * gravity_env_name();
//
//#include <gravity_optionals.h>
#ifndef GRAVITY_INCLUDE_MATH
	#define GRAVITY_INCLUDE_MATH
#endif
#ifdef GRAVITY_INCLUDE_MATH
	#define GRAVITY_MATH_REGISTER(_vm)          gravity_math_register(_vm)
	#define GRAVITY_MATH_FREE()                 gravity_math_free()
	#define GRAVITY_MATH_NAME()                 gravity_math_name()
	#define GRAVITY_ISMATH_CLASS(_c)            gravity_ismath_class(_c)
#else
	#define GRAVITY_MATH_REGISTER(_vm)
	#define GRAVITY_MATH_FREE()
	#define GRAVITY_MATH_NAME()                 NULL
	#define GRAVITY_ISMATH_CLASS(_c)            false
#endif
#ifndef GRAVITY_INCLUDE_JSON
	#define GRAVITY_INCLUDE_JSON
#endif
#ifdef GRAVITY_INCLUDE_JSON
	#define GRAVITY_JSON_REGISTER(_vm)          gravity_json_register(_vm)
	#define GRAVITY_JSON_FREE()                 gravity_json_free()
	#define GRAVITY_JSON_NAME()                 gravity_json_name()
	#define GRAVITY_ISJSON_CLASS(_c)            gravity_isjson_class(_c)
#else
	#define GRAVITY_JSON_REGISTER(_vm)
	#define GRAVITY_JSON_FREE()
	#define GRAVITY_JSON_NAME()                 NULL
	#define GRAVITY_ISJSON_CLASS(_c)            false
#endif
#ifndef GRAVITY_INCLUDE_ENV
	#define GRAVITY_INCLUDE_ENV
#endif
#ifdef GRAVITY_INCLUDE_ENV
	#define GRAVITY_ENV_REGISTER(_vm)           gravity_env_register(_vm)
	#define GRAVITY_ENV_FREE()                  gravity_env_free()
	#define GRAVITY_ENV_NAME()                  gravity_env_name()
	#define GRAVITY_ISENV_CLASS(_c)             gravity_isenv_class(_c)
	//#include "gravity_opt_env.h"
#else
	#define GRAVITY_ENV_REGISTER(_vm)
	#define GRAVITY_ENV_FREE()
	#define GRAVITY_ENV_NAME()                  NULL
	#define GRAVITY_ISENV_CLASS(_c)             false
#endif
#ifdef _MSC_VER
	#define INLINE								__inline
#else
	#define INLINE								inline
#endif

INLINE static const char ** gravity_optional_identifiers() 
{
    static const char *list[] = {
        #ifdef GRAVITY_INCLUDE_MATH
			GRAVITY_CLASS_MATH_NAME,
        #endif
        #ifdef GRAVITY_INCLUDE_ENV
			GRAVITY_CLASS_ENV_NAME,
        #endif
        #ifdef GRAVITY_INCLUDE_JSON
			GRAVITY_CLASS_JSON_NAME,
        #endif
        NULL
	};
    return list;
}

// @construction {
class GravityClassImplementation {
public:
	GravityClassImplementation(const char * pName, long flags) : P_Name(pName), P_Cls(0), RefCount(0), Flags(flags)
	{
	}
	virtual int Bind(gravity_class_t * pMeta)
	{
		int    ok = 1;
		/*
		// .get(key) and .set(key, value)
		gravity_class_bind(pMeta, "get", NEW_CLOSURE_VALUE(gravity_env_get));
		gravity_class_bind(pMeta, "set", NEW_CLOSURE_VALUE(gravity_env_set));
		gravity_class_bind(pMeta, "keys", NEW_CLOSURE_VALUE(gravity_env_keys));
		// Allow map-access
		gravity_class_bind(pMeta, GRAVITY_INTERNAL_LOADAT_NAME, NEW_CLOSURE_VALUE(gravity_env_get));
		gravity_class_bind(pMeta, GRAVITY_INTERNAL_STOREAT_NAME, NEW_CLOSURE_VALUE(gravity_env_set));
		*/
		return ok;
	}
	virtual void DestroyMeta(gravity_class_t * pMeta)
	{
	}
	int    Register(gravity_vm * pVm)
	{
		int    ok = -1;
		if(!P_Cls) {
			P_Cls = gravity_class_new_pair(NULL, P_Name, NULL, 0, 0);
			gravity_class_t * p_meta = gravity_class_get_meta(P_Cls);
			{
				Bind(p_meta); //create_optional_class();
			}
			SETMETA_INITED(P_Cls);
			ok = 1;
		}
		++RefCount;
		if(pVm && !gravity_vm_ismini(pVm)) 
			gravity_vm_setvalue(pVm, P_Name, GravityValue::from_object(P_Cls));
		return ok;
	}
	int    UnRegister()
	{
		int    ok = -1;
		if(P_Cls) {
			if((--RefCount) == 0) {
				gravity_class_t * p_meta = gravity_class_get_meta(P_Cls);
				DestroyMeta(p_meta);
				if(Flags & fCore) {
					gravity_class_free_core(NULL, p_meta);
					gravity_class_free_core(NULL, P_Cls);
				}
				else {
					gravity_class_free(NULL, p_meta);
					gravity_class_free(NULL, P_Cls);
				}
				P_Cls = 0;
				ok = 1;
			}
		}
		return ok;
	}
	enum {
		fCore = 0x0001
	};
	const  char * P_Name;
	const  long   Flags;
	gravity_class_t * P_Cls;
	uint32 RefCount;
protected:
	static bool CreateInstance(gravity_vm * vm, GravityValue * args, uint16 nargs, uint32 rindex) 
	{
		gravity_class_t * c = static_cast<gravity_class_t *>(args[0].Ptr);
		gravity_instance_t * p_instance = gravity_instance_new(vm, c);
		//Rectangle * r = new Rectangle();
		void * p_cls_imp = 0; // new Cls;
		gravity_instance_setxdata(p_instance, p_cls_imp);
		return vm->ReturnValue(GravityValue::from_object(reinterpret_cast<gravity_class_t *>(p_instance)), rindex);
	}
};
// } @construction 
