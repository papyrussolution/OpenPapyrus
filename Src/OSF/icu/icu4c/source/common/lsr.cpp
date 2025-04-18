// © 2019 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// lsr.cpp
// created: 2019may08 Markus W. Scherer

#include <icu-internal.h>
#pragma hdrstop
#include "lsr.h"
#include "ustr_imp.h"

U_NAMESPACE_BEGIN

LSR::LSR(char prefix, const char * lang, const char * scr, const char * r, int32_t f,
    UErrorCode & errorCode) :
	language(nullptr), script(nullptr), region(r),
	regionIndex(indexForRegion(region)), flags(f) {
	if(U_SUCCESS(errorCode)) {
		CharString langScript;
		langScript.append(prefix, errorCode).append(lang, errorCode).append('\0', errorCode);
		int32_t scriptOffset = langScript.length();
		langScript.append(prefix, errorCode).append(scr, errorCode);
		owned = langScript.cloneData(errorCode);
		if(U_SUCCESS(errorCode)) {
			language = owned;
			script = owned + scriptOffset;
		}
	}
}

LSR::LSR(LSR &&other) U_NOEXCEPT :
	language(other.language), script(other.script), region(other.region), owned(other.owned),
	regionIndex(other.regionIndex), flags(other.flags),
	hashCode(other.hashCode) {
	if(owned != nullptr) {
		other.language = other.script = "";
		other.owned = nullptr;
		other.hashCode = 0;
	}
}

void LSR::deleteOwned() {
	uprv_free(owned);
}

LSR &LSR::operator = (LSR &&other) U_NOEXCEPT {
	this->~LSR();
	language = other.language;
	script = other.script;
	region = other.region;
	regionIndex = other.regionIndex;
	flags = other.flags;
	owned = other.owned;
	hashCode = other.hashCode;
	if(owned != nullptr) {
		other.language = other.script = "";
		other.owned = nullptr;
		other.hashCode = 0;
	}
	return *this;
}

bool LSR::isEquivalentTo(const LSR &other) const 
{
	return sstreq(language, other.language) && sstreq(script, other.script) &&
		regionIndex == other.regionIndex &&
	        // Compare regions if both are ill-formed (and their indexes are 0).
		(regionIndex > 0 || sstreq(region, other.region));
}

bool LSR::operator == (const LSR &other) const 
{
	return sstreq(language, other.language) && sstreq(script, other.script) && regionIndex == other.regionIndex &&
	        // Compare regions if both are ill-formed (and their indexes are 0).
		(regionIndex > 0 || sstreq(region, other.region)) && flags == other.flags;
}

int32_t LSR::indexForRegion(const char * region) 
{
	int32_t c = region[0];
	int32_t a = c - '0';
	if(0 <= a && a <= 9) { // digits: "419"
		int32_t b = region[1] - '0';
		if(b < 0 || 9 < b) {
			return 0;
		}
		c = region[2] - '0';
		if(c < 0 || 9 < c || region[3] != 0) {
			return 0;
		}
		return (10 * a + b) * 10 + c + 1;
	}
	else { // letters: "DE"
		a = uprv_upperOrdinal(c);
		if(a < 0 || 25 < a) {
			return 0;
		}
		int32_t b = uprv_upperOrdinal(region[1]);
		if(b < 0 || 25 < b || region[2] != 0) {
			return 0;
		}
		return 26 * a + b + 1001;
	}
	return 0;
}

LSR &LSR::setHashCode() 
{
	if(hashCode == 0) {
		uint32_t h = ustr_hashCharsN(language, static_cast<int32_t>(strlen(language)));
		h = h * 37 + ustr_hashCharsN(script, static_cast<int32_t>(strlen(script)));
		h = h * 37 + regionIndex;
		hashCode = h * 37 + flags;
	}
	return *this;
}

U_NAMESPACE_END
