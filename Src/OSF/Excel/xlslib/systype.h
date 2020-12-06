/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2011 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//#define __DEBUG__

#ifndef SYSTYPE_H
#define SYSTYPE_H

#if defined(_MSC_VER) && defined(WIN32) && 0
#pragma warning(disable: 4786) // Long debug information related to STL
#pragma warning(disable: 4290) // C++ Exception Specification ignored
#endif

//#define PHPEXT_TSRMLS_C  tsrm_ls
//#define PHPEXT_TSRMLS_D  void ***tsrm_ls

//#define INLINE inline

//#define NULL ((void*)(0))

#define NO_ERRORS         0
#define FILE_ERROR        (-1)
#define GENERAL_ERROR     (-100)

#define PTYPE_DIRECTORY   0x01
#define PTYPE_FILE        0x02
#define PTYPE_ROOT        0x05

#define PCOLOR_BLACK      0x01
#define PCOLOR_RED		  0x00

#define PLINK_EMPTY       (-1)
#define PBLOCK_END        (-2)


#if USE_ALLOCATOR==ALLOCATOR_STL
#define XLSLIB_DFLT_ALLOCATOR

#elif USE_ALLOCATOR==ALLOCATOR_BTI
#include "bti_thread_alloc.h"
#define XLSLIB_DFLT_ALLOCATOR ,bti::thr_alloc

#else
#   error ERROR: Invalid allocator option
#endif

/* [i_a]   better to get the definition from a single place */

#if defined(__cplusplus)					// ALL C++ users

typedef std::list<std::string* XLSLIB_DFLT_ALLOCATOR> StringList_t;
typedef StringList_t::const_iterator StringListItor_t;

#endif

// Macros
#define MASK_BYTE0 0x00000000000000ffU
#define MASK_BYTE1 0x000000000000ff00U
#define MASK_BYTE2 0x0000000000ff0000U
#define MASK_BYTE3 0x00000000ff000000U

#if defined(_MSC_VER) && defined(WIN32) && 0 // [i_a]

#define MASK_BYTE4 0x000000ff00000000
#define MASK_BYTE5 0x0000ff0000000000
#define MASK_BYTE6 0x00ff000000000000
#define MASK_BYTE7 0xff00000000000000

#else

#define MASK_BYTE4 ((uint64)0x000000ff00000000LL)
#define MASK_BYTE5 ((uint64)0x0000ff0000000000LL)
#define MASK_BYTE6 ((uint64)0x00ff000000000000LL)
#define MASK_BYTE7 ((uint64)0xff00000000000000ULL)

#endif

#define BYTE_0(num) ((uint8) (num & MASK_BYTE0))
#define BYTE_1(num) ((uint8)((num & MASK_BYTE1)>>0x08))
#define BYTE_2(num) ((uint8)((num & MASK_BYTE2)>>0x10))
#define BYTE_3(num) ((uint8)((num & MASK_BYTE3)>>0x18))

#define BYTE_4(num) ((uint8)((num & MASK_BYTE4)>>0x20))
#define BYTE_5(num) ((uint8)((num & MASK_BYTE5)>>0x28))
#define BYTE_6(num) ((uint8)((num & MASK_BYTE6)>>0x30))
#define BYTE_7(num) ((uint8)((num & MASK_BYTE7)>>0x38))



#define PROPERTY_DFLT_NOTUSED		0x00
#define PROPERTY_MAX_NAME_LENGTH	32

#define PROPERTY_TYPE_DIR			0x01
#define PROPERTY_TYPE_FILE			0x02
#define PROPERTY_TYPE_ROOT			0x05

#define PROPERTY_COLOR_NODE_RED		0x00
#define PROPERTY_COLOR_NODE_BLACK	0x01
#define PROPERTY_LINK_EMPTY			(-1)

#define PPTPOS_NAME					0x00
#define PPTPOS_NAMELENGTH			0x40
#define PPTPOS_TYPE					0x42
#define PPTPOS_NODECOLOR			0x43
#define PPTPOS_PREVIOUS				0x44
#define PPTPOS_NEXT					0x48
#define PPTPOS_CHILD				0x4C
#define PPTPOS_UNUSED_EMPTY0		0x50


#define PPTPOS_SECS1				0x64
#define PPTPOS_SECS2				0x68
#define PPTPOS_DAYS1				0x6c
#define PPTPOS_DAYS2				0x70

#define PPTPOS_STARTBLOCK			0x74
#define PPTPOS_SIZE					0x78


#define PPT_DFLT_SECS1				0
#define PPT_DFLT_SECS2				0
#define PPT_DFLT_DAYS1				0
#define PPT_DFLT_DAYS2				0


// More macros

/*  
****************************************
****************************************
*/
#define Delete_Pointer(pt) {                    \
  if(pt != NULL)                                \
    {                                           \
      delete pt;                                \
      pt = NULL;                                \
    }                                           \
}


/*  
****************************************
****************************************
*/
#ifdef __DEBUG__

#define XTRACE(str) {                           \
  std::cerr<<"DEBUG: "<<str<<std::endl;         \
}
#define XTRACE2(str, arg) {                     \
  std::cerr<<"DEBUG: "<<str<<arg<<std::endl;    \
}

#else

#define XTRACE(str)
#define XTRACE2(str, arg)

#endif


#ifdef __PHP_DEBUG__

#define PHPTRACE(str) {                         \
	std::cerr<<"PHPDEBUG: "<<str<<std::endl;    \
}

#else

#define PHPTRACE(str)

#endif


#if defined(__cplusplus)
extern "C"
{
#endif

#if defined(HAVE___FUNCTION__)
#define XL_FUNCNAME()                   __FUNCTION__
#elif defined(HAVE___FUNC__)
#define XL_FUNCNAME()                   __func__
#else
#define XL_FUNCNAME()                   "???"
#endif

#ifndef CLANG_ANALYZER_NORETURN
	#if 0 // @sobolev __has_feature(attribute_analyzer_noreturn)
		#define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
	#else
		#define CLANG_ANALYZER_NORETURN
	#endif
#endif

/**
Custom ASSERT macro; since we create a library, we'ld better allow the user of that
lib to set up how [s]he wants to have her/his assertion failures reported.

stdlib's version, which dumps to stderr and calls abort() isn't exactly what you'd 
wish for in a UI or other non-UNIX/WIN-consolish environment!

@note
Users of xlslib may want to call xlslib_register_assert_reporter() at program initialization.

@note
The macro is constructed in such a way that the expression is only evaluated once (no
side effects from double invocations) and consumes the semicolon which follows it, so
that code like this will compile as expected in all conditions:

  if (cond)
	XL_ASSERT(some_expression);
  else
    do_something();

*/
#define XL_ASSERTS(str)																	\
	do																					\
	{																					\
		xlslib_report_failed_assertion(#str, __FILE__, __LINE__, XL_FUNCNAME());		\
	}  while (0)

#define XL_ASSERT(expr)																	\
	do																					\
	{																					\
		if (!(expr))																	\
		{																				\
			xlslib_report_failed_assertion(#expr, __FILE__, __LINE__, XL_FUNCNAME());	\
		}																				\
	}  while (0)

#define XL_VERIFY(expr)																	\
	do																					\
	{																					\
		if (!(expr))																	\
		{																				\
			xlslib_report_failed_assertion(#expr, __FILE__, __LINE__, XL_FUNCNAME());	\
		}																				\
	}  while (0)

#define XL_WITH_ASSERTIONS			1

/*
override for 'release' type builds: the compiler optimizer will make sure this
empty statement will be discarded, while we still ensure the trailing semicolon 
will be properly 'munched'.
*/
#if defined(XLSLIB_DONT_ASSERT)

#undef XL_ASSERT
#define XL_ASSERT(expr)																\
	(void)0

#undef XL_VERIFY
#define XL_VERIFY(expr)																\
	(void)(expr)

#undef XL_WITH_ASSERTIONS

#endif

typedef void xlslib_userdef_assertion_reporter(const char *expr, const char *fname, int lineno, const char *funcname);

void xlslib_report_failed_assertion(const char *expr, const char *fname, int lineno, const char *funcname) CLANG_ANALYZER_NORETURN;

/**
override the default (C++ exception throwing) assertion failure reporting function within xlslib.
*/
void xlslib_register_assert_reporter(xlslib_userdef_assertion_reporter *user_func);

#if defined(__cplusplus)
};
#endif

#endif
