// unistr_case.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 1999-2014, International Business Machines Corporation and others.  All Rights Reserved.
// encoding:   UTF-8
// created on: 2004aug19
// created by: Markus W. Scherer
// 
// Case-mapping functions moved here from unistr.cpp
// 
#include <icu-internal.h>
#pragma hdrstop
#include "unicode/casemap.h"
#include "ucasemap_imp.h"
//#include "uelement.h"

U_NAMESPACE_BEGIN
//
// Read-only implementation
//
int8 UnicodeString::doCaseCompare(int32_t start, int32_t length, const char16_t * srcChars, int32_t srcStart, int32_t srcLength, uint32_t options) const
{
	// compare illegal string values
	// treat const char16_t *srcChars==NULL as an empty string
	if(isBogus()) {
		return -1;
	}
	// pin indices to legal values
	pinIndices(start, length);
	if(srcChars == NULL) {
		srcStart = srcLength = 0;
	}
	// get the correct pointer
	const char16_t * chars = getArrayStart();
	chars += start;
	if(srcStart!=0) {
		srcChars += srcStart;
	}
	if(chars != srcChars) {
		UErrorCode errorCode = U_ZERO_ERROR;
		int32_t result = u_strcmpFold(chars, length, srcChars, srcLength, options|U_COMPARE_IGNORE_CASE, &errorCode);
		if(result!=0) {
			return (int8)(result >> 24 | 1);
		}
	}
	else {
		// get the srcLength if necessary
		if(srcLength < 0)
			srcLength = sstrleni(srcChars + srcStart);
		if(length != srcLength) {
			return (int8)((length - srcLength) >> 24 | 1);
		}
	}
	return 0;
}
//
// Write implementation
//
UnicodeString & UnicodeString::caseMap(int32_t caseLocale, uint32_t options, UCASEMAP_BREAK_ITERATOR_PARAM UStringCaseMapper * stringCaseMapper) 
{
	if(isEmpty() || !isWritable()) {
		// nothing to do
		return *this;
	}
	char16_t oldBuffer[2 * US_STACKBUF_SIZE];
	char16_t * oldArray;
	int32_t oldLength = length();
	int32_t newLength;
	bool writable = isBufferWritable();
	UErrorCode errorCode = U_ZERO_ERROR;
#if !UCONFIG_NO_BREAK_ITERATION
	// Read-only alias to the original string contents for the titlecasing BreakIterator.
	// We cannot set the iterator simply to *this because *this is being modified.
	UnicodeString oldString;
#endif

	// Try to avoid heap-allocating a new character array for this string.
	if(writable ? oldLength <= SIZEOFARRAYi(oldBuffer) : oldLength < US_STACKBUF_SIZE) {
		// Short string: Copy the contents into a temporary buffer and
		// case-map back into the current array, or into the stack buffer.
		char16_t * buffer = getArrayStart();
		int32_t capacity;
		oldArray = oldBuffer;
		u_memcpy(oldBuffer, buffer, oldLength);
		if(writable) {
			capacity = getCapacity();
		}
		else {
			// Switch from the read-only alias or shared heap buffer to the stack buffer.
			if(!cloneArrayIfNeeded(US_STACKBUF_SIZE, US_STACKBUF_SIZE, /* doCopyArray= */ FALSE)) {
				return *this;
			}
			assert(fUnion.fFields.fLengthAndFlags & kUsingStackBuffer);
			buffer = fUnion.fStackFields.fBuffer;
			capacity = US_STACKBUF_SIZE;
		}
#if !UCONFIG_NO_BREAK_ITERATION
		if(iter != nullptr) {
			oldString.setTo(FALSE, oldArray, oldLength);
			iter->setText(oldString);
		}
#endif
		newLength = stringCaseMapper(caseLocale, options, UCASEMAP_BREAK_ITERATOR
			buffer, capacity,
			oldArray, oldLength, NULL, errorCode);
		if(U_SUCCESS(errorCode)) {
			setLength(newLength);
			return *this;
		}
		else if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
			// common overflow handling below
		}
		else {
			setToBogus();
			return *this;
		}
	}
	else {
		// Longer string or read-only buffer:
		// Collect only changes and then apply them to this string.
		// Case mapping often changes only small parts of a string,
		// and often does not change its length.
		oldArray = getArrayStart();
		Edits edits;
		char16_t replacementChars[200];
#if !UCONFIG_NO_BREAK_ITERATION
		if(iter != nullptr) {
			oldString.setTo(FALSE, oldArray, oldLength);
			iter->setText(oldString);
		}
#endif
		stringCaseMapper(caseLocale, options | U_OMIT_UNCHANGED_TEXT, UCASEMAP_BREAK_ITERATOR
		    replacementChars, SIZEOFARRAYi(replacementChars),
		    oldArray, oldLength, &edits, errorCode);
		if(U_SUCCESS(errorCode)) {
			// Grow the buffer at most once, not for multiple doReplace() calls.
			newLength = oldLength + edits.lengthDelta();
			if(newLength > oldLength && !cloneArrayIfNeeded(newLength, newLength)) {
				return *this;
			}
			for(Edits::Iterator ei = edits.getCoarseChangesIterator(); ei.next(errorCode);) {
				doReplace(ei.destinationIndex(), ei.oldLength(),
				    replacementChars, ei.replacementIndex(), ei.newLength());
			}
			if(U_FAILURE(errorCode)) {
				setToBogus();
			}
			return *this;
		}
		else if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
			// common overflow handling below
			newLength = oldLength + edits.lengthDelta();
		}
		else {
			setToBogus();
			return *this;
		}
	}

	// Handle buffer overflow, newLength is known.
	// We need to allocate a new buffer for the internal string case mapping function.
	// This is very similar to how doReplace() keeps the old array pointer
	// and deletes the old array itself after it is done.
	// In addition, we are forcing cloneArrayIfNeeded() to always allocate a new array.
	int32_t * bufferToDelete = 0;
	if(!cloneArrayIfNeeded(newLength, newLength, FALSE, &bufferToDelete, TRUE)) {
		return *this;
	}
	errorCode = U_ZERO_ERROR;
	// No need to iter->setText() again: The case mapper restarts via iter->first().
	newLength = stringCaseMapper(caseLocale, options, UCASEMAP_BREAK_ITERATOR
		getArrayStart(), getCapacity(),
		oldArray, oldLength, NULL, errorCode);
	if(bufferToDelete) {
		uprv_free(bufferToDelete);
	}
	if(U_SUCCESS(errorCode)) {
		setLength(newLength);
	}
	else {
		setToBogus();
	}
	return *this;
}

UnicodeString & UnicodeString::foldCase(uint32_t options) {
	return caseMap(UCASE_LOC_ROOT, options, UCASEMAP_BREAK_ITERATOR_NULL ustrcase_internalFold);
}

U_NAMESPACE_END

// Defined here to reduce dependencies on break iterator
U_CAPI int32_t U_EXPORT2 uhash_hashCaselessUnicodeString(const UElement key) {
	U_NAMESPACE_USE
	const UnicodeString * str = (const UnicodeString *)key.pointer;
	if(!str) {
		return 0;
	}
	// Inefficient; a better way would be to have a hash function in
	// UnicodeString that does case folding on the fly.
	UnicodeString copy(*str);
	return copy.foldCase().hashCode();
}

// Defined here to reduce dependencies on break iterator
U_CAPI bool U_EXPORT2 uhash_compareCaselessUnicodeString(const UElement key1, const UElement key2) {
	U_NAMESPACE_USE
	const UnicodeString * str1 = (const UnicodeString *)key1.pointer;
	const UnicodeString * str2 = (const UnicodeString *)key2.pointer;
	if(str1 == str2) {
		return TRUE;
	}
	if(str1 == NULL || str2 == NULL) {
		return FALSE;
	}
	return str1->caseCompare(*str2, U_FOLD_CASE_DEFAULT) == 0;
}
