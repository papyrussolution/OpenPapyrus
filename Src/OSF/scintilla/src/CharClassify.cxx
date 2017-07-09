// Scintilla source code edit control
/** @file CharClassify.cxx
** Character classifications used by Document and RESearch.
**/
// Copyright 2006 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop
//#include <stdexcept>
#include "CharClassify.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

CharClassify::CharClassify() 
{
	SetDefaultCharClasses(true);
}

void CharClassify::SetDefaultCharClasses(bool includeWordClass) 
{
	// Initialize all char classes to default values
	for(int ch = 0; ch < 256; ch++) {
		if(ch == '\r' || ch == '\n')
			charClass[ch] = ccNewLine;
		else if(ch < 0x20 || ch == ' ')
			charClass[ch] = ccSpace;
		else if(includeWordClass && (ch >= 0x80 || isalnum(ch) || ch == '_'))
			charClass[ch] = ccWord;
		else
			charClass[ch] = ccPunctuation;
	}
}

void CharClassify::SetCharClasses(const uchar * chars, cc newCharClass) 
{
	// Apply the newCharClass to the specifed chars
	if(chars) {
		while(*chars) {
			charClass[*chars] = static_cast<uchar>(newCharClass);
			chars++;
		}
	}
}

int CharClassify::GetCharsOfClass(cc characterClass, uchar * buffer) const 
{
	// Get characters belonging to the given char class; return the number
	// of characters (if the buffer is NULL, don't write to it).
	int count = 0;
	for(int ch = maxChar - 1; ch >= 0; --ch) {
		if(charClass[ch] == characterClass) {
			++count;
			if(buffer) {
				*buffer = static_cast<uchar>(ch);
				buffer++;
			}
		}
	}
	return count;
}

