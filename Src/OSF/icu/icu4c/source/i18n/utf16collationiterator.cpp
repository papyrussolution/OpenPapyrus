// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 * Copyright (C) 2010-2014, International Business Machines
 * Corporation and others.  All Rights Reserved.
 *******************************************************************************
 * utf16collationiterator.cpp
 *
 * created on: 2010oct27
 * created by: Markus W. Scherer
 */
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_COLLATION

#include "collationfcd.h"
#include "collationiterator.h"
#include "utf16collationiterator.h"

U_NAMESPACE_BEGIN

UTF16CollationIterator::UTF16CollationIterator(const UTF16CollationIterator &other,
    const char16_t * newText)
	: CollationIterator(other),
	start(newText),
	pos(newText + (other.pos - other.start)),
	limit(other.limit == NULL ? NULL : newText + (other.limit - other.start)) {
}

UTF16CollationIterator::~UTF16CollationIterator() {
}

bool UTF16CollationIterator::operator == (const CollationIterator &other) const {
	if(!CollationIterator::operator == (other)) {
		return false;
	}
	const UTF16CollationIterator &o = static_cast<const UTF16CollationIterator &>(other);
	// Compare the iterator state but not the text: Assume that the caller does that.
	return (pos - start) == (o.pos - o.start);
}

void UTF16CollationIterator::resetToOffset(int32_t newOffset) {
	reset();
	pos = start + newOffset;
}

int32_t UTF16CollationIterator::getOffset() const {
	return (int32_t)(pos - start);
}

uint32_t UTF16CollationIterator::handleNextCE32(UChar32 &c, UErrorCode & /*errorCode*/) {
	if(pos == limit) {
		c = U_SENTINEL;
		return Collation::FALLBACK_CE32;
	}
	c = *pos++;
	return UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, c);
}

char16_t UTF16CollationIterator::handleGetTrailSurrogate() {
	if(pos == limit) {
		return 0;
	}
	char16_t trail;
	if(U16_IS_TRAIL(trail = *pos)) {
		++pos;
	}
	return trail;
}

bool UTF16CollationIterator::foundNULTerminator() {
	if(limit == NULL) {
		limit = --pos;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

UChar32 UTF16CollationIterator::nextCodePoint(UErrorCode & /*errorCode*/) {
	if(pos == limit) {
		return U_SENTINEL;
	}
	UChar32 c = *pos;
	if(c == 0 && limit == NULL) {
		limit = pos;
		return U_SENTINEL;
	}
	++pos;
	char16_t trail;
	if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(trail = *pos)) {
		++pos;
		return U16_GET_SUPPLEMENTARY(c, trail);
	}
	else {
		return c;
	}
}

UChar32 UTF16CollationIterator::previousCodePoint(UErrorCode & /*errorCode*/) {
	if(pos == start) {
		return U_SENTINEL;
	}
	UChar32 c = *--pos;
	char16_t lead;
	if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(lead = *(pos - 1))) {
		--pos;
		return U16_GET_SUPPLEMENTARY(lead, c);
	}
	else {
		return c;
	}
}

void UTF16CollationIterator::forwardNumCodePoints(int32_t num, UErrorCode & /*errorCode*/) {
	while(num > 0 && pos != limit) {
		UChar32 c = *pos;
		if(c == 0 && limit == NULL) {
			limit = pos;
			break;
		}
		++pos;
		--num;
		if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(*pos)) {
			++pos;
		}
	}
}

void UTF16CollationIterator::backwardNumCodePoints(int32_t num, UErrorCode & /*errorCode*/) {
	while(num > 0 && pos != start) {
		UChar32 c = *--pos;
		--num;
		if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(*(pos-1))) {
			--pos;
		}
	}
}

// FCDUTF16CollationIterator ----------------------------------------------- ***

FCDUTF16CollationIterator::FCDUTF16CollationIterator(const FCDUTF16CollationIterator &other,
    const char16_t * newText)
	: UTF16CollationIterator(other),
	rawStart(newText),
	segmentStart(newText + (other.segmentStart - other.rawStart)),
	segmentLimit(other.segmentLimit == NULL ? NULL : newText + (other.segmentLimit - other.rawStart)),
	rawLimit(other.rawLimit == NULL ? NULL : newText + (other.rawLimit - other.rawStart)),
	nfcImpl(other.nfcImpl),
	normalized(other.normalized),
	checkDir(other.checkDir) {
	if(checkDir != 0 || other.start == other.segmentStart) {
		start = newText + (other.start - other.rawStart);
		pos = newText + (other.pos - other.rawStart);
		limit = other.limit == NULL ? NULL : newText + (other.limit - other.rawStart);
	}
	else {
		start = normalized.getBuffer();
		pos = start + (other.pos - other.start);
		limit = start + normalized.length();
	}
}

FCDUTF16CollationIterator::~FCDUTF16CollationIterator() {
}

bool FCDUTF16CollationIterator::operator == (const CollationIterator &other) const {
	// Skip the UTF16CollationIterator and call its parent.
	if(!CollationIterator::operator == (other)) {
		return false;
	}
	const FCDUTF16CollationIterator &o = static_cast<const FCDUTF16CollationIterator &>(other);
	// Compare the iterator state but not the text: Assume that the caller does that.
	if(checkDir != o.checkDir) {
		return false;
	}
	if(checkDir == 0 && (start == segmentStart) != (o.start == o.segmentStart)) {
		return false;
	}
	if(checkDir != 0 || start == segmentStart) {
		return (pos - rawStart) == (o.pos - o.rawStart);
	}
	else {
		return (segmentStart - rawStart) == (o.segmentStart - o.rawStart) &&
		       (pos - start) == (o.pos - o.start);
	}
}

void FCDUTF16CollationIterator::resetToOffset(int32_t newOffset) {
	reset();
	start = segmentStart = pos = rawStart + newOffset;
	limit = rawLimit;
	checkDir = 1;
}

int32_t FCDUTF16CollationIterator::getOffset() const {
	if(checkDir != 0 || start == segmentStart) {
		return (int32_t)(pos - rawStart);
	}
	else if(pos == start) {
		return (int32_t)(segmentStart - rawStart);
	}
	else {
		return (int32_t)(segmentLimit - rawStart);
	}
}

uint32_t FCDUTF16CollationIterator::handleNextCE32(UChar32 &c, UErrorCode & errorCode) {
	for(;;) {
		if(checkDir > 0) {
			if(pos == limit) {
				c = U_SENTINEL;
				return Collation::FALLBACK_CE32;
			}
			c = *pos++;
			if(CollationFCD::hasTccc(c)) {
				if(CollationFCD::maybeTibetanCompositeVowel(c) ||
				    (pos != limit && CollationFCD::hasLccc(*pos))) {
					--pos;
					if(!nextSegment(errorCode)) {
						c = U_SENTINEL;
						return Collation::FALLBACK_CE32;
					}
					c = *pos++;
				}
			}
			break;
		}
		else if(checkDir == 0 && pos != limit) {
			c = *pos++;
			break;
		}
		else {
			switchToForward();
		}
	}
	return UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, c);
}

bool FCDUTF16CollationIterator::foundNULTerminator() {
	if(limit == NULL) {
		limit = rawLimit = --pos;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

UChar32 FCDUTF16CollationIterator::nextCodePoint(UErrorCode & errorCode) {
	UChar32 c;
	for(;;) {
		if(checkDir > 0) {
			if(pos == limit) {
				return U_SENTINEL;
			}
			c = *pos++;
			if(CollationFCD::hasTccc(c)) {
				if(CollationFCD::maybeTibetanCompositeVowel(c) ||
				    (pos != limit && CollationFCD::hasLccc(*pos))) {
					--pos;
					if(!nextSegment(errorCode)) {
						return U_SENTINEL;
					}
					c = *pos++;
				}
			}
			else if(c == 0 && limit == NULL) {
				limit = rawLimit = --pos;
				return U_SENTINEL;
			}
			break;
		}
		else if(checkDir == 0 && pos != limit) {
			c = *pos++;
			break;
		}
		else {
			switchToForward();
		}
	}
	char16_t trail;
	if(U16_IS_LEAD(c) && pos != limit && U16_IS_TRAIL(trail = *pos)) {
		++pos;
		return U16_GET_SUPPLEMENTARY(c, trail);
	}
	else {
		return c;
	}
}

UChar32 FCDUTF16CollationIterator::previousCodePoint(UErrorCode & errorCode) {
	UChar32 c;
	for(;;) {
		if(checkDir < 0) {
			if(pos == start) {
				return U_SENTINEL;
			}
			c = *--pos;
			if(CollationFCD::hasLccc(c)) {
				if(CollationFCD::maybeTibetanCompositeVowel(c) ||
				    (pos != start && CollationFCD::hasTccc(*(pos - 1)))) {
					++pos;
					if(!previousSegment(errorCode)) {
						return U_SENTINEL;
					}
					c = *--pos;
				}
			}
			break;
		}
		else if(checkDir == 0 && pos != start) {
			c = *--pos;
			break;
		}
		else {
			switchToBackward();
		}
	}
	char16_t lead;
	if(U16_IS_TRAIL(c) && pos != start && U16_IS_LEAD(lead = *(pos - 1))) {
		--pos;
		return U16_GET_SUPPLEMENTARY(lead, c);
	}
	else {
		return c;
	}
}

void FCDUTF16CollationIterator::forwardNumCodePoints(int32_t num, UErrorCode & errorCode) {
	// Specify the class to avoid a virtual-function indirection.
	// In Java, we would declare this class final.
	while(num > 0 && FCDUTF16CollationIterator::nextCodePoint(errorCode) >= 0) {
		--num;
	}
}

void FCDUTF16CollationIterator::backwardNumCodePoints(int32_t num, UErrorCode & errorCode) {
	// Specify the class to avoid a virtual-function indirection.
	// In Java, we would declare this class final.
	while(num > 0 && FCDUTF16CollationIterator::previousCodePoint(errorCode) >= 0) {
		--num;
	}
}

void FCDUTF16CollationIterator::switchToForward() {
	assert(checkDir < 0 || (checkDir == 0 && pos == limit));
	if(checkDir < 0) {
		// Turn around from backward checking.
		start = segmentStart = pos;
		if(pos == segmentLimit) {
			limit = rawLimit;
			checkDir = 1; // Check forward.
		}
		else { // pos < segmentLimit
			checkDir = 0; // Stay in FCD segment.
		}
	}
	else {
		// Reached the end of the FCD segment.
		if(start == segmentStart) {
			// The input text segment is FCD, extend it forward.
		}
		else {
			// The input text segment needed to be normalized.
			// Switch to checking forward from it.
			pos = start = segmentStart = segmentLimit;
			// Note: If this segment is at the end of the input text,
			// then it might help to return FALSE to indicate that, so that
			// we do not have to re-check and normalize when we turn around and go backwards.
			// However, that would complicate the call sites for an optimization of an unusual case.
		}
		limit = rawLimit;
		checkDir = 1;
	}
}

bool FCDUTF16CollationIterator::nextSegment(UErrorCode & errorCode) {
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	assert(checkDir > 0 && pos != limit);
	// The input text [segmentStart..pos[ passes the FCD check.
	const char16_t * p = pos;
	uint8 prevCC = 0;
	for(;;) {
		// Fetch the next character's fcd16 value.
		const char16_t * q = p;
		uint16 fcd16 = nfcImpl.nextFCD16(p, rawLimit);
		uint8 leadCC = (uint8)(fcd16 >> 8);
		if(leadCC == 0 && q != pos) {
			// FCD boundary before the [q, p[ character.
			limit = segmentLimit = q;
			break;
		}
		if(leadCC != 0 && (prevCC > leadCC || CollationFCD::isFCD16OfTibetanCompositeVowel(fcd16))) {
			// Fails FCD check. Find the next FCD boundary and normalize.
			do {
				q = p;
			} while(p != rawLimit && nfcImpl.nextFCD16(p, rawLimit) > 0xff);
			if(!normalize(pos, q, errorCode)) {
				return FALSE;
			}
			pos = start;
			break;
		}
		prevCC = (uint8)fcd16;
		if(p == rawLimit || prevCC == 0) {
			// FCD boundary after the last character.
			limit = segmentLimit = p;
			break;
		}
	}
	assert(pos != limit);
	checkDir = 0;
	return TRUE;
}

void FCDUTF16CollationIterator::switchToBackward() {
	assert(checkDir > 0 || (checkDir == 0 && pos == start));
	if(checkDir > 0) {
		// Turn around from forward checking.
		limit = segmentLimit = pos;
		if(pos == segmentStart) {
			start = rawStart;
			checkDir = -1; // Check backward.
		}
		else { // pos > segmentStart
			checkDir = 0; // Stay in FCD segment.
		}
	}
	else {
		// Reached the start of the FCD segment.
		if(start == segmentStart) {
			// The input text segment is FCD, extend it backward.
		}
		else {
			// The input text segment needed to be normalized.
			// Switch to checking backward from it.
			pos = limit = segmentLimit = segmentStart;
		}
		start = rawStart;
		checkDir = -1;
	}
}

bool FCDUTF16CollationIterator::previousSegment(UErrorCode & errorCode) {
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	assert(checkDir < 0 && pos != start);
	// The input text [pos..segmentLimit[ passes the FCD check.
	const char16_t * p = pos;
	uint8 nextCC = 0;
	for(;;) {
		// Fetch the previous character's fcd16 value.
		const char16_t * q = p;
		uint16 fcd16 = nfcImpl.previousFCD16(rawStart, p);
		uint8 trailCC = (uint8)fcd16;
		if(trailCC == 0 && q != pos) {
			// FCD boundary after the [p, q[ character.
			start = segmentStart = q;
			break;
		}
		if(trailCC != 0 && ((nextCC != 0 && trailCC > nextCC) ||
		    CollationFCD::isFCD16OfTibetanCompositeVowel(fcd16))) {
			// Fails FCD check. Find the previous FCD boundary and normalize.
			do {
				q = p;
			} while(fcd16 > 0xff && p != rawStart &&
			    (fcd16 = nfcImpl.previousFCD16(rawStart, p)) != 0);
			if(!normalize(q, pos, errorCode)) {
				return FALSE;
			}
			pos = limit;
			break;
		}
		nextCC = (uint8)(fcd16 >> 8);
		if(p == rawStart || nextCC == 0) {
			// FCD boundary before the following character.
			start = segmentStart = p;
			break;
		}
	}
	assert(pos != start);
	checkDir = 0;
	return TRUE;
}

bool FCDUTF16CollationIterator::normalize(const char16_t * from, const char16_t * to, UErrorCode & errorCode) {
	// NFD without argument checking.
	assert(U_SUCCESS(errorCode));
	nfcImpl.decompose(from, to, normalized, (int32_t)(to - from), errorCode);
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	// Switch collation processing into the FCD buffer
	// with the result of normalizing [segmentStart, segmentLimit[.
	segmentStart = from;
	segmentLimit = to;
	start = normalized.getBuffer();
	limit = start + normalized.length();
	return TRUE;
}

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
