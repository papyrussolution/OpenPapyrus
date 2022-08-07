// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 */
#include <icu-internal.h>
#pragma hdrstop

U_NAMESPACE_BEGIN

static const char * const gPluralForms[] = {
	"other", "zero", "one", "two", "few", "many"
};

PluralMapBase::Category PluralMapBase::toCategory(const char * pluralForm) {
	for(int32_t i = 0; i < SIZEOFARRAYi(gPluralForms); ++i) {
		if(strcmp(pluralForm, gPluralForms[i]) == 0) {
			return static_cast<Category>(i);
		}
	}
	return NONE;
}

PluralMapBase::Category PluralMapBase::toCategory(const UnicodeString & pluralForm) {
	CharString cCategory;
	UErrorCode status = U_ZERO_ERROR;
	cCategory.appendInvariantChars(pluralForm, status);
	return U_FAILURE(status) ? NONE : toCategory(cCategory.data());
}

const char * PluralMapBase::getCategoryName(Category c) {
	int32_t index = c;
	return (index < 0 || index >= SIZEOFARRAYi(gPluralForms)) ?
	       NULL : gPluralForms[index];
}

U_NAMESPACE_END
