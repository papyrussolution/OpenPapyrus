// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//
//   Copyright (C) 2012 International Business Machines Corporation
//   and others. All rights reserved.
//
//   file:  regeximp.cpp
//
//           ICU Regular Expressions,
//             miscellaneous implementation functions.
//
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_REGULAR_EXPRESSIONS
#include "regeximp.h"

U_NAMESPACE_BEGIN

CaseFoldingUTextIterator::CaseFoldingUTextIterator(UText &text) :
	fUText(text), fFoldChars(NULL), fFoldLength(0) {
}

CaseFoldingUTextIterator::~CaseFoldingUTextIterator() {
}

UChar32 CaseFoldingUTextIterator::next() {
	UChar32 foldedC;
	UChar32 originalC;
	if(fFoldChars == NULL) {
		// We are not in a string folding of an earlier character.
		// Start handling the next char from the input UText.
		originalC = UTEXT_NEXT32(&fUText);
		if(originalC == U_SENTINEL) {
			return originalC;
		}
		fFoldLength = ucase_toFullFolding(originalC, &fFoldChars, U_FOLD_CASE_DEFAULT);
		if(fFoldLength >= UCASE_MAX_STRING_LENGTH || fFoldLength < 0) {
			// input code point folds to a single code point, possibly itself.
			// See comment in ucase.h for explanation of return values from ucase_toFullFoldings.
			if(fFoldLength < 0) {
				fFoldLength = ~fFoldLength;
			}
			foldedC = (UChar32)fFoldLength;
			fFoldChars = NULL;
			return foldedC;
		}
		// String foldings fall through here.
		fFoldIndex = 0;
	}

	U16_NEXT(fFoldChars, fFoldIndex, fFoldLength, foldedC);
	if(fFoldIndex >= fFoldLength) {
		fFoldChars = NULL;
	}
	return foldedC;
}

bool CaseFoldingUTextIterator::inExpansion() {
	return fFoldChars != NULL;
}

CaseFoldingUCharIterator::CaseFoldingUCharIterator(const char16_t * chars, int64_t start, int64_t limit) :
	fChars(chars), fIndex(start), fLimit(limit), fFoldChars(NULL), fFoldLength(0) {
}

CaseFoldingUCharIterator::~CaseFoldingUCharIterator() {
}

UChar32 CaseFoldingUCharIterator::next() {
	UChar32 foldedC;
	UChar32 originalC;
	if(fFoldChars == NULL) {
		// We are not in a string folding of an earlier character.
		// Start handling the next char from the input UText.
		if(fIndex >= fLimit) {
			return U_SENTINEL;
		}
		U16_NEXT(fChars, fIndex, fLimit, originalC);

		fFoldLength = ucase_toFullFolding(originalC, &fFoldChars, U_FOLD_CASE_DEFAULT);
		if(fFoldLength >= UCASE_MAX_STRING_LENGTH || fFoldLength < 0) {
			// input code point folds to a single code point, possibly itself.
			// See comment in ucase.h for explanation of return values from ucase_toFullFoldings.
			if(fFoldLength < 0) {
				fFoldLength = ~fFoldLength;
			}
			foldedC = (UChar32)fFoldLength;
			fFoldChars = NULL;
			return foldedC;
		}
		// String foldings fall through here.
		fFoldIndex = 0;
	}

	U16_NEXT(fFoldChars, fFoldIndex, fFoldLength, foldedC);
	if(fFoldIndex >= fFoldLength) {
		fFoldChars = NULL;
	}
	return foldedC;
}

bool CaseFoldingUCharIterator::inExpansion() {
	return fFoldChars != NULL;
}

int64_t CaseFoldingUCharIterator::getIndex() {
	return fIndex;
}

U_NAMESPACE_END

#endif
