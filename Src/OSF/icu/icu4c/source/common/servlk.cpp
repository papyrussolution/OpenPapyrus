// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/**
 *******************************************************************************
 * Copyright (C) 2001-2014, International Business Machines Corporation and    *
 * others. All Rights Reserved.
 *******************************************************************************
 *
 *******************************************************************************
 */
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_SERVICE

#include "uresimp.h"
#include "servloc.h"
#include "ustrfmt.h"

#define UNDERSCORE_CHAR ((char16_t)0x005f)
#define AT_SIGN_CHAR    ((char16_t)64)
#define PERIOD_CHAR     ((char16_t)46)

U_NAMESPACE_BEGIN

LocaleKey* LocaleKey::createWithCanonicalFallback(const UnicodeString * primaryID,
    const UnicodeString * canonicalFallbackID,
    UErrorCode & status)
{
	return LocaleKey::createWithCanonicalFallback(primaryID, canonicalFallbackID, KIND_ANY, status);
}

LocaleKey* LocaleKey::createWithCanonicalFallback(const UnicodeString * primaryID,
    const UnicodeString * canonicalFallbackID,
    int32_t kind,
    UErrorCode & status)
{
	if(primaryID == NULL || U_FAILURE(status)) {
		return NULL;
	}
	UnicodeString canonicalPrimaryID;
	LocaleUtility::canonicalLocaleString(primaryID, canonicalPrimaryID);
	return new LocaleKey(*primaryID, canonicalPrimaryID, canonicalFallbackID, kind);
}

LocaleKey::LocaleKey(const UnicodeString & primaryID,
    const UnicodeString & canonicalPrimaryID,
    const UnicodeString * canonicalFallbackID,
    int32_t kind)
	: ICUServiceKey(primaryID)
	, _kind(kind)
	, _primaryID(canonicalPrimaryID)
	, _fallbackID()
	, _currentID()
{
	_fallbackID.setToBogus();
	if(_primaryID.length() != 0) {
		if(canonicalFallbackID != NULL && _primaryID != *canonicalFallbackID) {
			_fallbackID = *canonicalFallbackID;
		}
	}

	_currentID = _primaryID;
}

LocaleKey::~LocaleKey() {
}

UnicodeString &LocaleKey::prefix(UnicodeString & result) const {
	if(_kind != KIND_ANY) {
		char16_t buffer[64];
		uprv_itou(buffer, 64, _kind, 10, 0);
		UnicodeString temp(buffer);
		result.append(temp);
	}
	return result;
}

int32_t LocaleKey::kind() const {
	return _kind;
}

UnicodeString &LocaleKey::canonicalID(UnicodeString & result) const {
	return result.append(_primaryID);
}

UnicodeString &LocaleKey::currentID(UnicodeString & result) const {
	if(!_currentID.isBogus()) {
		result.append(_currentID);
	}
	return result;
}

UnicodeString &LocaleKey::currentDescriptor(UnicodeString & result) const {
	if(!_currentID.isBogus()) {
		prefix(result).append(PREFIX_DELIMITER).append(_currentID);
	}
	else {
		result.setToBogus();
	}
	return result;
}

Locale&LocaleKey::canonicalLocale(Locale & result) const {
	return LocaleUtility::initLocaleFromName(_primaryID, result);
}

Locale&LocaleKey::currentLocale(Locale & result) const {
	return LocaleUtility::initLocaleFromName(_currentID, result);
}

bool LocaleKey::fallback() {
	if(!_currentID.isBogus()) {
		int x = _currentID.lastIndexOf(UNDERSCORE_CHAR);
		if(x != -1) {
			_currentID.remove(x); // truncate current or fallback, whichever we're pointing to
			return TRUE;
		}

		if(!_fallbackID.isBogus()) {
			_currentID = _fallbackID;
			_fallbackID.setToBogus();
			return TRUE;
		}

		if(_currentID.length() > 0) {
			_currentID.remove(0); // completely truncate
			return TRUE;
		}

		_currentID.setToBogus();
	}

	return FALSE;
}

bool LocaleKey::isFallbackOf(const UnicodeString & id) const {
	UnicodeString temp(id);
	parseSuffix(temp);
	return temp.indexOf(_primaryID) == 0 &&
	       (temp.length() == _primaryID.length() ||
	       temp.charAt(_primaryID.length()) == UNDERSCORE_CHAR);
}

#ifdef SERVICE_DEBUG
UnicodeString &LocaleKey::debug(UnicodeString & result) const
{
	ICUServiceKey::debug(result);
	result.append((UnicodeString)" kind: ");
	result.append(_kind);
	result.append((UnicodeString)" primaryID: ");
	result.append(_primaryID);
	result.append((UnicodeString)" fallbackID: ");
	result.append(_fallbackID);
	result.append((UnicodeString)" currentID: ");
	result.append(_currentID);
	return result;
}

UnicodeString &LocaleKey::debugClass(UnicodeString & result) const
{
	return result.append((UnicodeString)"LocaleKey ");
}

#endif

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(LocaleKey)

U_NAMESPACE_END

/* !UCONFIG_NO_SERVICE */
#endif
