// Scintilla source code edit control
/** @file CaseFolder.cxx
** Classes for case folding.
**/
// Copyright 1998-2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop
//#include <stdexcept>
//#include <vector>
//#include <algorithm>
//#include "CaseFolder.h"
#include "CaseConvert.h"
//#include "UniConversion.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

CaseFolder::~CaseFolder()
{
}

CaseFolderTable::CaseFolderTable()
{
	for(size_t iChar = 0; iChar<sizeof(mapping); iChar++) {
		mapping[iChar] = static_cast<char>(iChar);
	}
}

CaseFolderTable::~CaseFolderTable()
{
}

size_t CaseFolderTable::Fold(char * folded, size_t sizeFolded, const char * mixed, size_t lenMixed)
{
	size_t result = 0;
	if(lenMixed <= sizeFolded) {
		for(size_t i = 0; i < lenMixed; i++)
			folded[i] = mapping[static_cast<uchar>(mixed[i])];
		result = lenMixed;
	}
	return result;
}

void CaseFolderTable::SetTranslation(char ch, char chTranslation)
{
	mapping[static_cast<uchar>(ch)] = chTranslation;
}

void CaseFolderTable::StandardASCII()
{
	for(size_t iChar = 0; iChar < sizeof(mapping); iChar++) {
		mapping[iChar] = (iChar >= 'A' && iChar <= 'Z') ? static_cast<char>(iChar - 'A' + 'a') : static_cast<char>(iChar);
	}
}

CaseFolderUnicode::CaseFolderUnicode()
{
	StandardASCII();
	converter = ConverterFor(CaseConversionFold);
}

size_t CaseFolderUnicode::Fold(char * folded, size_t sizeFolded, const char * mixed, size_t lenMixed)
{
	size_t result = 1;
	if((lenMixed == 1) && (sizeFolded > 0))
		folded[0] = mapping[static_cast<uchar>(mixed[0])];
	else
		result = converter->CaseConvertString(folded, sizeFolded, mixed, lenMixed);
	return result;
}

