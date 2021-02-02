// SREGEXP2.CPP
// Copyright (c) A.Sobolev 2021
//
#include <slib-internal.h>
#pragma hdrstop
#include <..\OSF\oniguruma\src\oniguruma.h>

SRegExp2::SRegExp2() : H(0)
{
}

SRegExp2::SRegExp2(const char * pPattern) : H(0)
{
	const size_t pattern_len = sstrlen(pPattern);
	if(pattern_len) {
		OnigErrorInfo einfo;
		OnigRegexType * p_reg = 0;
		int r = onig_new(&p_reg, (uchar *)pPattern, (uchar *)(pPattern + pattern_len), ONIG_OPTION_DEFAULT, ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT, &einfo);
		if(r == ONIG_NORMAL) {
			H = p_reg;
		}
	}
}
	
SRegExp2::SRegExp2(const SRegExp2 & rS) : H(0)
{
}
	
SRegExp2::~SRegExp2()
{
	if(H) {
		onig_free(static_cast<OnigRegexType *>(H));
		H = 0;
	}
}
	
int SRegExp2::Compile(const char * pPattern) // Compiles char* --> regexp
{
	return 0;
}
	
int SRegExp2::Find(const char * pText) // TRUE if regexp in char* arg
{
	return 0;
}
	
int SRegExp2::Find(SStrScan *)
{
	return 0;
}

int SRegExp2::GetLastErr() const
{
	return 0;
}
	
int SRegExp2::IsValid() const
{
	return (H != 0);
}

long SRegExp2::Start() const
{
	return 0;
}
	
long SRegExp2::End() const
{
	return 0;
}
	
int FASTCALL SRegExp2::operator == (const SRegExp2 & r) const
{
	return 0;
}
	
int FASTCALL SRegExp2::DeepEqual(const SRegExp2 & r) const // Same regexp and state?
{
	return 0;
}

void SRegExp2::SetInvalid()
{
}
