// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// uniquecharstr.h
// created: 2020sep01 Frank Yung-Fong Tang

#ifndef __UNIQUECHARSTR_H__
#define __UNIQUECHARSTR_H__

#include "charstr.h"
//#include "uassert.h"
#include "uhash.h"

U_NAMESPACE_BEGIN

/**
 * Stores NUL-terminated strings with duplicate elimination.
 * Checks for unique UTF-16 string pointers and converts to invariant characters.
 *
 * Intended to be stack-allocated. Add strings, get a unique number for each,
 * freeze the object, get a char * pointer for each string,
 * call orphanCharStrings() to capture the string storage, and let this object go out of scope.
 */
class UniqueCharStrings {
public:
    UniqueCharStrings(UErrorCode & errorCode) : strings(nullptr) {
        // Note: We hash on string contents but store stable char16_t * pointers.
        // If the strings are stored in resource bundles which should be built with
        // duplicate elimination, then we should be able to hash on just the pointer values.
        uhash_init(&map, uhash_hashUChars, uhash_compareUChars, uhash_compareLong, &errorCode);
        if(U_FAILURE(errorCode)) { return; }
        strings = new CharString();
        if(strings == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    ~UniqueCharStrings() {
        uhash_close(&map);
        delete strings;
    }

    /** Returns/orphans the CharString that contains all strings. */
    CharString *orphanCharStrings() {
        CharString *result = strings;
        strings = nullptr;
        return result;
    }

    /**
  * Adds a string and returns a unique number for it.
  * The string's buffer contents must not change, nor move around in memory,
  * while this UniqueCharStrings is in use.
  * The string contents must be NUL-terminated exactly at s.length().
     *
  * Best used with read-only-alias UnicodeString objects that point to
  * stable storage, such as strings returned by resource bundle functions.
     */
    int32_t add(const UnicodeString & s, UErrorCode & errorCode) {
        if(U_FAILURE(errorCode)) { return 0; }
        if(isFrozen) {
            errorCode = U_NO_WRITE_PERMISSION;
            return 0;
        }
        // The string points into the resource bundle.
        const char16_t *p = s.getBuffer();
        int32_t oldIndex = uhash_geti(&map, p);
        if(oldIndex != 0) {  // found duplicate
            return oldIndex;
        }
        // Explicit NUL terminator for the previous string.
        // The strings object is also terminated with one implicit NUL.
        strings->append(0, errorCode);
        int32_t newIndex = strings->length();
        strings->appendInvariantChars(s, errorCode);
        uhash_puti(&map, const_cast<char16_t *>(p), newIndex, &errorCode);
        return newIndex;
    }

    void freeze() { isFrozen = true; }

    /**
  * Returns a string pointer for its unique number, if this object is frozen.
  * Otherwise nullptr.
     */
    const char *get(int32_t i) const {
        assert(isFrozen);
        return isFrozen && i > 0 ? strings->data() + i : nullptr;
    }

private:
    UHashtable map;
    CharString *strings;
    bool isFrozen = false;
};

U_NAMESPACE_END

#endif  // __UNIQUECHARSTR_H__
