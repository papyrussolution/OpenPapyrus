// regversion.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2020  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop

extern const char * onig_version(void)
{
	static char s[12];
	/*xsnprintf*/slsprintf_s(s, sizeof(s), "%d.%d.%d", ONIGURUMA_VERSION_MAJOR, ONIGURUMA_VERSION_MINOR, ONIGURUMA_VERSION_TEENY);
	return s;
}

extern const char * onig_copyright(void)
{
	static char s[58];
	/*xsnprintf*/slsprintf_s(s, sizeof(s), "Oniguruma %d.%d.%d : Copyright (C) 2002-2018 K.Kosako", ONIGURUMA_VERSION_MAJOR, ONIGURUMA_VERSION_MINOR, ONIGURUMA_VERSION_TEENY);
	return s;
}
