// onig_init.c -  Oniguruma (regular expression library)
// Copyright (c) 2016-2019  K.Kosako  All rights reserved.
//
#include "regint.h"
#pragma hdrstop

// onig_init(): deprecated function 
extern int onig_init(void)
{
#if 0
	OnigEncoding encs[] = { ONIG_ENCODING_UTF8 };
	return onig_initialize(encs, sizeof(encs)/sizeof(encs[0]));
#else
	return onig_initialize(0, 0);
#endif
}
