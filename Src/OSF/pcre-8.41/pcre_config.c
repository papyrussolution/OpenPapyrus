//
// Perl-Compatible Regular Expressions
//
// PCRE is a library of functions to support regular expressions whose syntax
// and semantics are as close as possible to those of the Perl 5 language.
// Written by Philip Hazel Copyright (c) 1997-2016 University of Cambridge
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// -- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// -- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// -- Neither the name of the University of Cambridge nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
/* This module contains the external function pcre_config(). */
#include "pcre_internal.h"
#pragma hdrstop

static int real_link_size = LINK_SIZE; // Keep the original link size

/*************************************************
* Return info about what features are configured *
*************************************************/

/* This function has an extensible interface so that additional items can be
   added compatibly.

   Arguments:
   what             what information is required
   where            where to put the information

   Returns:           0 if data returned, negative on error
 */

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION pcre_config(int what, void * where)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION pcre16_config(int what, void * where)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION pcre32_config(int what, void * where)
#endif
{
	switch(what) {
		case PCRE_CONFIG_UTF8:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
		    *static_cast<int *>(where) = 0;
		    return PCRE_ERROR_BADOPTION;
#else
#if defined SUPPORT_UTF
		    *static_cast<int *>(where) = 1;
#else
		    *static_cast<int *>(where) = 0;
#endif
		    break;
#endif
		case PCRE_CONFIG_UTF16:
#if defined COMPILE_PCRE8 || defined COMPILE_PCRE32
		    *static_cast<int *>(where) = 0;
		    return PCRE_ERROR_BADOPTION;
#else
#if defined SUPPORT_UTF
		    *static_cast<int *>(where) = 1;
#else
		    *static_cast<int *>(where) = 0;
#endif
		    break;
#endif
		case PCRE_CONFIG_UTF32:
#if defined COMPILE_PCRE8 || defined COMPILE_PCRE16
		    *static_cast<int *>(where) = 0;
		    return PCRE_ERROR_BADOPTION;
#else
#if defined SUPPORT_UTF
		    *static_cast<int *>(where) = 1;
#else
		    *static_cast<int *>(where) = 0;
#endif
		    break;
#endif
		case PCRE_CONFIG_UNICODE_PROPERTIES:
#ifdef SUPPORT_UCP
		    *static_cast<int *>(where) = 1;
#else
		    *static_cast<int *>(where) = 0;
#endif
		    break;
		case PCRE_CONFIG_JIT:
#ifdef SUPPORT_JIT
		    *static_cast<int *>(where) = 1;
#else
		    *static_cast<int *>(where) = 0;
#endif
		    break;
		case PCRE_CONFIG_JITTARGET:
#ifdef SUPPORT_JIT
		    *((const char **)where) = PRIV(jit_get_target) ();
#else
		    *((const char **)where) = NULL;
#endif
		    break;
		case PCRE_CONFIG_NEWLINE:
		    *static_cast<int *>(where) = NEWLINE;
		    break;
		case PCRE_CONFIG_BSR:
#ifdef BSR_ANYCRLF
		    *static_cast<int *>(where) = 1;
#else
		    *static_cast<int *>(where) = 0;
#endif
		    break;
		case PCRE_CONFIG_LINK_SIZE:
		    *static_cast<int *>(where) = real_link_size;
		    break;
		case PCRE_CONFIG_POSIX_MALLOC_THRESHOLD:
		    *static_cast<int *>(where) = POSIX_MALLOC_THRESHOLD;
		    break;
		case PCRE_CONFIG_PARENS_LIMIT:
		    *((ulong *)where) = PARENS_NEST_LIMIT;
		    break;
		case PCRE_CONFIG_MATCH_LIMIT:
		    *((ulong *)where) = MATCH_LIMIT;
		    break;
		case PCRE_CONFIG_MATCH_LIMIT_RECURSION:
		    *((ulong *)where) = MATCH_LIMIT_RECURSION;
		    break;
		case PCRE_CONFIG_STACKRECURSE:
#ifdef NO_RECURSE
		    *static_cast<int *>(where) = 0;
#else
		    *static_cast<int *>(where) = 1;
#endif
		    break;
		default: return PCRE_ERROR_BADOPTION;
	}
	return 0;
}
/* End of pcre_config.c */
