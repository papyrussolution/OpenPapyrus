// Scintilla source code edit control
/** @file CharacterSet.cxx
** Simple case functions for ASCII.
** Lexer infrastructure.
**/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop
#include "CharacterSet.h"
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
	if(len == 0)
		return 0;
	else
		return (*a - *b); // Either *a or *b is nul
}

CharacterSet::CharacterSet(setBase base /*= setNone*/, const char * initialSet /*= ""*/, int size_ /*= 0x80*/, bool valueAfter_ /*= false*/)
{
	size = size_;
	valueAfter = valueAfter_;
	bset = new bool[size];
	for(int i = 0; i < size; i++)
		bset[i] = false;
	AddString(initialSet);
	if(base & setLower)
		AddString("abcdefghijklmnopqrstuvwxyz");
	if(base & setUpper)
		AddString("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	if(base & setDigits)
		AddString("0123456789");
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
