// Scintilla source code edit control
/** @file CharacterSet.cxx
** Simple case functions for ASCII.
** Lexer infrastructure.
**/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop
#ifdef SCI_NAMESPACE
	using namespace Scintilla;
#endif
#ifdef SCI_NAMESPACE
	namespace Scintilla {
#endif

int CompareCaseInsensitive(const char * a, const char * b)
{
	while(*a && *b) {
		if(*a != *b) {
			char upperA = static_cast<char>(MakeUpperCase(*a));
			char upperB = static_cast<char>(MakeUpperCase(*b));
			if(upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
	}
	// Either *a or *b is nul
	return *a - *b;
}

int CompareNCaseInsensitive(const char * a, const char * b, size_t len)
{
	while(*a && *b && len) {
		if(*a != *b) {
			char upperA = static_cast<char>(MakeUpperCase(*a));
			char upperB = static_cast<char>(MakeUpperCase(*b));
			if(upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
		len--;
	}
	if(!len)
		return 0;
	else
		return (*a - *b); // Either *a or *b is nul
}
//
// Functions for classifying characters
//
bool FASTCALL IsASpace(int ch) { return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d)); }
bool FASTCALL IsASpaceOrTab(int ch) { return oneof2(ch, ' ', '\t'); }
bool FASTCALL IsADigit(int ch) { return (ch >= '0') && (ch <= '9'); }
bool FASTCALL IsADigit(int ch, int base)
{
	if(base <= 10)
		return (ch >= '0') && (ch < '0' + base);
	else
		return ((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch < 'A' + base - 10)) || ((ch >= 'a') && (ch < 'a' + base - 10));
}
bool FASTCALL IsASCII(int ch) { return (ch >= 0) && (ch < 0x80); }
bool FASTCALL IsLowerCase(int ch) { return (ch >= 'a') && (ch <= 'z'); }
bool FASTCALL IsUpperCase(int ch) { return (ch >= 'A') && (ch <= 'Z'); }
// @v10.9.6 (replaced with isasciialnum) bool FASTCALL IsAlphaNumeric(int ch) { return ((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')); }
// 
// Check if a character is a space.
// This is ASCII specific but is safe with chars >= 0x80.
// 
bool FASTCALL isspacechar(int ch) { return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d)); }
bool FASTCALL iswordchar(int ch) { return isasciialnum(ch) || ch == '.' || ch == '_'; }
bool FASTCALL iswordstart(int ch) { return isasciialnum(ch) || ch == '_'; }
bool FASTCALL isoperator(int ch)
{
	if(isasciialnum(ch))
		return false;
	else if(oneof8(ch, '%', '^', '&', '*',  '(', ')', '-', '+') || oneof8(ch, '=', '|', '{', '}', '[', ']', ':', ';') || oneof8(ch, '<', '>', ',', '/', '?', '!', '.', '~'))
		return true;
	else
		return false;
}
//
// Simple case functions for ASCII.
//
int FASTCALL MakeUpperCase(int ch) { return (ch < 'a' || ch > 'z') ? ch : static_cast<char>(ch - 'a' + 'A'); }
int FASTCALL MakeLowerCase(int ch) { return (ch < 'A' || ch > 'Z') ? ch : (ch - 'A' + 'a'); }
//
//
//
CharacterSet::CharacterSet(setBase base /*= setNone*/, const char * initialSet /*= ""*/, int size_ /*= 0x80*/, bool valueAfter_ /*= false*/)
{
	size = size_;
	valueAfter = valueAfter_;
	bset = new bool[size];
	for(int i = 0; i < size; i++)
		bset[i] = false;
	AddString(initialSet);
	if(base & setLower)
		AddString(STextConst::Get(STextConst::cAlphabetEngL, 0));
	if(base & setUpper)
		AddString(STextConst::Get(STextConst::cAlphabetEngU, 0));
	if(base & setDigits)
		AddString(STextConst::P_Digits);
}

CharacterSet::CharacterSet(const CharacterSet &other)
{
	size = other.size;
	valueAfter = other.valueAfter;
	bset = new bool[size];
	for(int i = 0; i < size; i++)
		bset[i] = other.bset[i];
}

CharacterSet::~CharacterSet()
{
	delete []bset;
	bset = 0;
	size = 0;
}

CharacterSet & FASTCALL CharacterSet::operator = (const CharacterSet &other)
{
	if(this != &other) {
		bool * bsetNew = new bool[other.size];
		for(int i = 0; i < other.size; i++)
			bsetNew[i] = other.bset[i];
		delete []bset;
		size = other.size;
		valueAfter = other.valueAfter;
		bset = bsetNew;
	}
	return *this;
}

void FASTCALL CharacterSet::Add(int val)
{
	assert(val >= 0);
	assert(val < size);
	bset[val] = true;
}

void FASTCALL CharacterSet::AddString(const char * setToAdd)
{
	for(const char * cp = setToAdd; *cp; cp++) {
		int val = static_cast<uchar>(*cp);
		assert(val >= 0);
		assert(val < size);
		bset[val] = true;
	}
}

bool FASTCALL CharacterSet::Contains(int val) const
{
	assert(val >= 0);
	if(val < 0) 
		return false;
	else
		return (val < size) ? bset[val] : valueAfter;
}

#ifdef SCI_NAMESPACE
}
#endif
