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
//
// This file contains a private PCRE function that converts an ordinal
// character value into a UTF8 string. 
//
// @sobolev (у нас и так режим COMPILE_PCRE8, а эта директива ломает precompiled header) 
#define COMPILE_PCRE8
#include "pcre_internal.h"
#pragma hdrstop

/*************************************************
*       Convert character value to UTF-8         *
*************************************************/

/* This function takes an integer value in the range 0 - 0x10ffff
   and encodes it as a UTF-8 character in 1 to 4 pcre_uchars.

   Arguments:
   cvalue     the character value
   buffer     pointer to buffer for result - at least 6 pcre_uchars long

   Returns:     number of characters placed in the buffer
 */
uint PRIV(ord2utf) (uint32 cvalue, pcre_uchar *buffer)
{
#ifdef SUPPORT_UTF
	int i, j;
	for(i = 0; i < PRIV(utf8_table1_size); i++)
		if((int)cvalue <= PRIV(utf8_table1)[i]) break;
	buffer += i;
	for(j = i; j > 0; j--) {
		*buffer-- = 0x80 | (cvalue & 0x3f);
		cvalue >>= 6;
	}
	*buffer = PRIV(utf8_table2)[i] | cvalue;
	return i + 1;
#else
	(void)(cvalue); /* Keep compiler happy; this function won't ever be */
	(void)(buffer); /* called when SUPPORT_UTF is not defined. */
	return 0;
#endif
}

/* End of pcre_ord2utf8.c */
