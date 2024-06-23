// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2001-2007, International Business Machines Corporation and others.  All Rights Reserved.
// Date        Name        Description
// 05/24/01    aliu        Creation.
// 
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_TRANSLITERATION

#include "toupptrn.h"
#include "ustr_imp.h"
#include "cpputils.h"

U_NAMESPACE_BEGIN UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UppercaseTransliterator)

UppercaseTransliterator::UppercaseTransliterator() : CaseMapTransliterator(UNICODE_STRING("Any-Upper", 9), ucase_toFullUpper)
{
}

UppercaseTransliterator::~UppercaseTransliterator() 
{
}

UppercaseTransliterator::UppercaseTransliterator(const UppercaseTransliterator& o) : CaseMapTransliterator(o)
{
}

/*UppercaseTransliterator& UppercaseTransliterator::operator = (const UppercaseTransliterator& o) 
{
    CaseMapTransliterator::operator = (o);
    return *this;
}*/

/**
 * Transliterator API.
 */
UppercaseTransliterator* UppercaseTransliterator::clone() const { return new UppercaseTransliterator(*this); }

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
