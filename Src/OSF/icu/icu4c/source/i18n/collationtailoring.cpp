// collationtailoring.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 * Copyright (C) 2013-2015, International Business Machines Corporation and others.  All Rights Reserved.
 * created on: 2013mar12
 * created by: Markus W. Scherer
 */
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_COLLATION

#include "collationsettings.h"
#include "collationtailoring.h"

U_NAMESPACE_BEGIN

CollationTailoring::CollationTailoring(const CollationSettings * baseSettings) : data(NULL), settings(baseSettings), actualLocale(""),
	ownedData(NULL), builder(NULL), memory(NULL), bundle(NULL), trie(NULL), unsafeBackwardSet(NULL), maxExpansions(NULL) 
{
	if(baseSettings) {
		U_ASSERT(baseSettings->reorderCodesLength == 0);
		U_ASSERT(baseSettings->reorderTable == NULL);
		U_ASSERT(baseSettings->minHighNoReorder == 0);
	}
	else {
		settings = new CollationSettings();
	}
	CALLPTRMEMB(settings, addRef());
	rules.getTerminatedBuffer(); // ensure NUL-termination
	version[0] = version[1] = version[2] = version[3] = 0;
	maxExpansionsInitOnce.reset();
}

CollationTailoring::~CollationTailoring() 
{
	SharedObject::clearPtr(settings);
	delete ownedData;
	delete builder;
	udata_close(memory);
	ures_close(bundle);
	utrie2_close(trie);
	delete unsafeBackwardSet;
	uhash_close(maxExpansions);
	maxExpansionsInitOnce.reset();
}

bool CollationTailoring::ensureOwnedData(UErrorCode & errorCode) 
{
	if(U_FAILURE(errorCode)) {
		return FALSE;
	}
	if(ownedData == NULL) {
		const Normalizer2Impl * nfcImpl = Normalizer2Factory::getNFCImpl(errorCode);
		if(U_FAILURE(errorCode)) {
			return FALSE;
		}
		ownedData = new CollationData(*nfcImpl);
		if(ownedData == NULL) {
			errorCode = U_MEMORY_ALLOCATION_ERROR;
			return FALSE;
		}
	}
	data = ownedData;
	return TRUE;
}

void CollationTailoring::makeBaseVersion(const UVersionInfo ucaVersion, UVersionInfo version) 
{
	version[0] = UCOL_BUILDER_VERSION;
	version[1] = (ucaVersion[0] << 3) + ucaVersion[1];
	version[2] = ucaVersion[2] << 6;
	version[3] = 0;
}

void CollationTailoring::setVersion(const UVersionInfo baseVersion, const UVersionInfo rulesVersion) 
{
	version[0] = UCOL_BUILDER_VERSION;
	version[1] = baseVersion[1];
	version[2] = (baseVersion[2] & 0xc0) + ((rulesVersion[0] + (rulesVersion[0] >> 6)) & 0x3f);
	version[3] = (rulesVersion[1] << 3) + (rulesVersion[1] >> 5) + rulesVersion[2] +
	    (rulesVersion[3] << 4) + (rulesVersion[3] >> 4);
}

int32_t CollationTailoring::getUCAVersion() const { return ((int32_t)version[1] << 4) | (version[2] >> 6); }

CollationCacheEntry::~CollationCacheEntry() 
{
	SharedObject::clearPtr(tailoring);
}

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
