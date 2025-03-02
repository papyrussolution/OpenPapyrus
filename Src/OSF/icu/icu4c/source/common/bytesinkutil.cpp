// BYTESINKUTIL.CPP
// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// created: 2017sep14 Markus W. Scherer
//
#include <icu-internal.h>
#pragma hdrstop

U_NAMESPACE_BEGIN

bool ByteSinkUtil::appendChange(int32_t length, const char16_t * s16, int32_t s16Length, ByteSink &sink, Edits * edits, UErrorCode & errorCode) 
{
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	char scratch[200];
	int32_t s8Length = 0;
	for(int32_t i = 0; i < s16Length;) {
		int32_t capacity;
		int32_t desiredCapacity = s16Length - i;
		if(desiredCapacity < (INT32_MAX / 3)) {
			desiredCapacity *= 3; // max 3 UTF-8 bytes per UTF-16 code unit
		}
		else if(desiredCapacity < (INT32_MAX / 2)) {
			desiredCapacity *= 2;
		}
		else {
			desiredCapacity = INT32_MAX;
		}
		char * buffer = sink.GetAppendBuffer(U8_MAX_LENGTH, desiredCapacity, scratch, SIZEOFARRAYi(scratch), &capacity);
		capacity -= U8_MAX_LENGTH - 1;
		int32_t j = 0;
		for(; i < s16Length && j < capacity;) {
			UChar32 c;
			U16_NEXT_UNSAFE(s16, i, c);
			U8_APPEND_UNSAFE(buffer, j, c);
		}
		if(j > (INT32_MAX - s8Length)) {
			errorCode = U_INDEX_OUTOFBOUNDS_ERROR;
			return FALSE;
		}
		sink.Append(buffer, j);
		s8Length += j;
	}
	CALLPTRMEMB(edits, addReplace(length, s8Length));
	return TRUE;
}

bool ByteSinkUtil::appendChange(const uint8 * s, const uint8 * limit, const char16_t * s16, int32_t s16Length, ByteSink &sink, Edits * edits, UErrorCode & errorCode) 
{
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	if((limit - s) > INT32_MAX) {
		errorCode = U_INDEX_OUTOFBOUNDS_ERROR;
		return FALSE;
	}
	return appendChange((int32_t)(limit - s), s16, s16Length, sink, edits, errorCode);
}

void ByteSinkUtil::appendCodePoint(int32_t length, UChar32 c, ByteSink &sink, Edits * edits) 
{
	char s8[U8_MAX_LENGTH];
	int32_t s8Length = 0;
	U8_APPEND_UNSAFE(s8, s8Length, c);
	CALLPTRMEMB(edits, addReplace(length, s8Length));
	sink.Append(s8, s8Length);
}

namespace {
	// See unicode/utf8.h U8_APPEND_UNSAFE().
	inline uint8 getTwoByteLead(UChar32 c) { return (uint8)((c >> 6) | 0xc0); }
	inline uint8 getTwoByteTrail(UChar32 c) { return (uint8)((c & 0x3f) | 0x80); }
}

void ByteSinkUtil::appendTwoBytes(UChar32 c, ByteSink &sink) 
{
	assert(0x80 <= c && c <= 0x7ff); // 2-byte UTF-8
	char s8[2] = { (char)getTwoByteLead(c), (char)getTwoByteTrail(c) };
	sink.Append(s8, 2);
}

void ByteSinkUtil::appendNonEmptyUnchanged(const uint8 * s, int32_t length, ByteSink &sink, uint32_t options, Edits * edits) 
{
	assert(length > 0);
	CALLPTRMEMB(edits, addUnchanged(length));
	if((options & U_OMIT_UNCHANGED_TEXT) == 0) {
		sink.Append(reinterpret_cast<const char *>(s), length);
	}
}

bool ByteSinkUtil::appendUnchanged(const uint8 * s, const uint8 * limit, ByteSink &sink, uint32_t options, Edits * edits, UErrorCode & errorCode) 
{
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	if((limit - s) > INT32_MAX) {
		errorCode = U_INDEX_OUTOFBOUNDS_ERROR;
		return FALSE;
	}
	int32_t length = (int32_t)(limit - s);
	if(length > 0) {
		appendNonEmptyUnchanged(s, length, sink, options, edits);
	}
	return TRUE;
}

CharStringByteSink::CharStringByteSink(CharString* dest) : dest_(*dest) 
{
}

CharStringByteSink::~CharStringByteSink() = default;

void CharStringByteSink::Append(const char * bytes, int32_t n) 
{
	UErrorCode status = U_ZERO_ERROR;
	dest_.append(bytes, n, status);
	// Any errors are silently ignored.
}

char * CharStringByteSink::GetAppendBuffer(int32_t min_capacity, int32_t desired_capacity_hint, char * scratch,
    int32_t scratch_capacity, int32_t* result_capacity) 
{
	if(min_capacity < 1 || scratch_capacity < min_capacity) {
		*result_capacity = 0;
		return nullptr;
	}
	UErrorCode status = U_ZERO_ERROR;
	char * result = dest_.getAppendBuffer(min_capacity, desired_capacity_hint, *result_capacity, status);
	if(U_SUCCESS(status)) {
		return result;
	}
	*result_capacity = scratch_capacity;
	return scratch;
}

U_NAMESPACE_END
