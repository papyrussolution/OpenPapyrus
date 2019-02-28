/* common.h - Header for all common functions in common.c */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2016 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

/* Used in some logic */
#ifndef __COMMON_H
#define __COMMON_H

#include <slib.h>
/*
#ifndef FALSE
#define FALSE		0
#endif
#ifndef TRUE
#define TRUE		1
#endif
*/
#define NEON	"0123456789" // The most commonly used set

#include "zint.h"
//#include <stdlib.h>
#include <locale.h>
#ifdef _MSC_VER
	#include <malloc.h>
	//#include <fcntl.h>
	#define inline _inline
#endif

//#define ustrcpy_Removed(target,source) strcpy((char *)target,(const char*)source)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    //extern size_t ustrlen_(const uchar source[]);
    extern int  FASTCALL ctoi_ReplacedWith_hex(const char source);
    extern char FASTCALL itoc(const int source);
    extern void to_upper(uchar source[]);
    extern int  FASTCALL is_sane(const char test_string[], const uchar source[], const size_t length);
    extern void lookup(const char set_string[], const char *table[], const char data, char dest[]);
    extern int posn(const char set_string[], const char data);
    extern void FASTCALL expand(struct ZintSymbol *symbol, const char data[]);
    extern int is_stackable(const int symbology);
    extern int is_extendable(const int symbology);
    extern int roundup(const float input);
    extern int module_is_set(const struct ZintSymbol *symbol, const int y_coord, const int x_coord);
    extern void FASTCALL set_module(struct ZintSymbol *symbol, const int y_coord, const int x_coord);
    extern void FASTCALL unset_module(struct ZintSymbol *symbol, const int y_coord, const int x_coord);
    extern int istwodigits(const uchar source[], const int position);
    extern double froundup(const double input);
    extern int parunmodd(const uchar llyth);
    extern int utf8toutf16(struct ZintSymbol *symbol, const uchar source[], int vals[], int *length);
    extern void set_minimum_height(struct ZintSymbol *symbol, int min_height);
	//
	// Simple Reed-Solomon encoder
	//
	extern void rs_init_gf(const int poly);
	extern void rs_init_code(const int nsym,int index);
	extern void rs_encode(const int len,const uchar *data, uchar *res);
	extern void rs_encode_long(const int len,const uint *data, uint *res);
	extern void rs_free(void);
	//
	// Verifies GS1 data
	//
    extern int gs1_verify(struct ZintSymbol *symbol, const uchar source[], const size_t src_len, char reduced[]);
    extern int ugs1_verify(struct ZintSymbol *symbol, const uchar source[], const uint src_len, uchar reduced[]);
	//
	// Handles binary manipulation of large numbers
	//
	extern void binary_load(short reg[], const char data[], const uint src_len);
	extern void binary_add(short accumulator[], short input_buffer[]);
	extern void binary_subtract(short accumulator[], short input_buffer[]);
	extern void shiftdown(short buffer[]);
	extern void shiftup(short buffer[]);
	extern short islarger(short accum[], short reg[]);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __COMMON_H */
