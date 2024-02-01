/*
 * Copyright © 2017  Google, Inc.
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
#ifndef HB_STRING_ARRAY_HH
#if 0 /* Make checks happy. */
#define HB_STRING_ARRAY_HH
#endif

#include "hb.hh"

/* Based on Bruno Haible's code in Appendix B of Ulrich Drepper's dsohowto.pdf:
 * https://software.intel.com/sites/default/files/m/a/1/e/dsohowto.pdf */

#define HB_STRING_ARRAY_TYPE_NAME       HB_PASTE(HB_STRING_ARRAY_NAME, _msgstr_t)
#define HB_STRING_ARRAY_POOL_NAME       HB_PASTE(HB_STRING_ARRAY_NAME, _msgstr)
#define HB_STRING_ARRAY_OFFS_NAME       HB_PASTE(HB_STRING_ARRAY_NAME, _msgidx)
#define HB_STRING_ARRAY_LENG_NAME       HB_PASTE(HB_STRING_ARRAY_NAME, _length)

static const union HB_STRING_ARRAY_TYPE_NAME {
	struct {
/* I like to avoid storing the nul-termination byte since we don't need it,
 * but C++ does not allow that.
 * https://stackoverflow.com/q/28433862
 */
#define _S(s) char HB_PASTE(str, __LINE__)[sizeof(s)];
#include HB_STRING_ARRAY_LIST
#undef _S
	} st;

	char str[HB_VAR_ARRAY];
}

HB_STRING_ARRAY_POOL_NAME = {
	{
#define _S(s) s,
#include HB_STRING_ARRAY_LIST
#undef _S
	}
};
static const uint HB_STRING_ARRAY_OFFS_NAME[] = {
#define _S(s) offsetof(union HB_STRING_ARRAY_TYPE_NAME, st.HB_PASTE(str, __LINE__)),
#include HB_STRING_ARRAY_LIST
#undef _S
	sizeof(HB_STRING_ARRAY_TYPE_NAME)
};

static const uint HB_STRING_ARRAY_LENG_NAME = SIZEOFARRAY(HB_STRING_ARRAY_OFFS_NAME) - 1;

static inline hb_bytes_t HB_STRING_ARRAY_NAME(uint i)
{
	assert(i < ARRAY_LENGTH(HB_STRING_ARRAY_OFFS_NAME) - 1);
	return hb_bytes_t(HB_STRING_ARRAY_POOL_NAME.str + HB_STRING_ARRAY_OFFS_NAME[i],
		   HB_STRING_ARRAY_OFFS_NAME[i+1] - HB_STRING_ARRAY_OFFS_NAME[i] - 1);
}

#undef HB_STRING_ARRAY_TYPE_NAME
#undef HB_STRING_ARRAY_POOL_NAME
#undef HB_STRING_ARRAY_OFFS_NAME
#undef HB_STRING_ARRAY_LENG_NAME

#endif /* HB_STRING_ARRAY_HH */
