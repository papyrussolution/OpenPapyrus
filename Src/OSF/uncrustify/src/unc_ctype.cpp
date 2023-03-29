// unc_ctype.cpp
//
#include <uncrustify-internal.h>
#pragma hdrstop
//#include "options.h"

/* @sobolev (replaced with sfixctype) int unc_fix_ctype(int ch)
{
	if(ch >= -1 && ch <= 255) {
		return (ch);
	}
	return 0; // Issue #3025
}*/

int unc_isspace(int ch)
{
	if((ch == 12)/*Issue #2386*/ && uncrustify::options::use_form_feed_no_more_as_whitespace_character()) {
		return 0;
	}
	else {
		return (isspace(sfixctype(ch)));
	}
}

int unc_isprint(int ch) { return (isprint(sfixctype(ch))); }
int unc_isalpha(int ch) { return (isalpha(sfixctype(ch))); }
int unc_isalnum(int ch) { return (isalnum(sfixctype(ch))); }
int unc_toupper(int ch) { return (toupper(sfixctype(ch))); }
int unc_tolower(int ch) { return (tolower(sfixctype(ch))); }
// @sobolev (replaced with ishex) int unc_isxdigit(int ch) { return (isxdigit(sfixctype(ch))); }
// @sobolev (replaced with isdec) int unc_isdigit(int ch) { return (isdigit(sfixctype(ch))); }
int unc_isupper(int ch) { return (isalpha(sfixctype(ch)) && (unc_toupper(sfixctype(ch)) == ch)); }
