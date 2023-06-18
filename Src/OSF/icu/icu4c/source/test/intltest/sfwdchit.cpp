// sfwdchit.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (c) 1997-2003, International Business Machines Corporation and others. All Rights Reserved.
// encoding:   UTF-8
//
#include <icu-internal.h>
#pragma hdrstop
#include "sfwdchit.h"
#include "unicode/ustring.h"

// A hash code of kInvalidHashCode indicates that the has code needs
// to be computed. A hash code of kEmptyHashCode is used for empty keys
// and for any key whose computed hash code is kInvalidHashCode.
const int32_t SimpleFwdCharIterator::kInvalidHashCode = 0;
const int32_t SimpleFwdCharIterator::kEmptyHashCode = 1;

#if 0 // not used
SimpleFwdCharIterator::SimpleFwdCharIterator(const UnicodeString & s) {

    fHashCode = kInvalidHashCode;
    fLen = s.length();
    fStart = new char16_t[fLen];
    if(fStart == NULL) {
        fBogus = TRUE;
    } else {
        fEnd = fStart+fLen;
        fCurrent = fStart;
        fBogus = FALSE;
        s.extract(0, fLen, fStart);          
    }
    
}
#endif

SimpleFwdCharIterator::SimpleFwdCharIterator(char16_t *s, int32_t len, bool adopt) {

    fHashCode = kInvalidHashCode;

    fLen = len==-1 ? u_strlen(s) : len;

    if(adopt == FALSE) {
        fStart = new char16_t[fLen];
        if(fStart == NULL) {
            fBogus = TRUE;
        } else {
            uprv_memcpy(fStart, s, fLen);
            fEnd = fStart+fLen;
            fCurrent = fStart;
            fBogus = FALSE;
        }
    } else { // adopt = TRUE
        fCurrent = fStart = s;
        fEnd = fStart + fLen;
        fBogus = FALSE;
    }

}

SimpleFwdCharIterator::~SimpleFwdCharIterator() {
    delete[] fStart;
}

#if 0 // not used
bool SimpleFwdCharIterator::operator==(const ForwardCharacterIterator& that) const {
    if(this == &that) {
        return true;
    }
/*
    if(that->fHashCode != kInvalidHashCode && this->fHashCode = that->fHashCode) {
        return true;
    }

    if(this->fStart == that->fStart) {
        return true;
    }

    if(this->fLen == that->fLen && memcmp(this->fStart, that->fStart, this->fLen) {
        return true;
    }
*/
    return false;
}
#endif

int32_t SimpleFwdCharIterator::hashCode() const {
    if(fHashCode == kInvalidHashCode)
    {
        UHashTok key;
        key.pointer = fStart;
        ((SimpleFwdCharIterator *)this)->fHashCode = uhash_hashUChars(key);
    }
    return fHashCode;
}
        
UClassID SimpleFwdCharIterator::getDynamicClassID() const {
    return NULL;
}

char16_t SimpleFwdCharIterator::nextPostInc() {
    if(fCurrent == fEnd) {
        return ForwardCharacterIterator::DONE;
    } else {
        return *(fCurrent)++;
    }
}
        
UChar32 SimpleFwdCharIterator::next32PostInc() {
    return ForwardCharacterIterator::DONE;
}
        
bool SimpleFwdCharIterator::hasNext() {
    return fCurrent < fEnd;
}
