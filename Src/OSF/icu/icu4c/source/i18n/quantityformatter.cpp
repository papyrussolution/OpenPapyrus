// quantityformatter.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2014-2016, International Business Machines Corporation and others.  All Rights Reserved.
//
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_FORMATTING

#include "quantityformatter.h"
#include "standardplural.h"
#include "number_decimalquantity.h"
#include "number_utypes.h"
#include "formatted_string_builder.h"

U_NAMESPACE_BEGIN

QuantityFormatter::QuantityFormatter() 
{
	for(uint i = 0; i < SIZEOFARRAY(formatters); ++i) {
		formatters[i] = NULL;
	}
}

QuantityFormatter::QuantityFormatter(const QuantityFormatter &other) 
{
	for(uint i = 0; i < SIZEOFARRAY(formatters); ++i) {
		formatters[i] = other.formatters[i] ? new SimpleFormatter(*other.formatters[i]) : 0;
	}
}

QuantityFormatter &QuantityFormatter::operator = (const QuantityFormatter & other) 
{
	if(this == &other) {
		return *this;
	}
	for(int32_t i = 0; i < SIZEOFARRAYi(formatters); ++i) {
		delete formatters[i];
		formatters[i] = other.formatters[i] ? new SimpleFormatter(*other.formatters[i]) : 0;
	}
	return *this;
}

QuantityFormatter::~QuantityFormatter() 
{
	for(uint i = 0; i < SIZEOFARRAY(formatters); ++i) {
		delete formatters[i];
	}
}

void QuantityFormatter::reset() 
{
	for(uint i = 0; i < SIZEOFARRAY(formatters); ++i) {
		ZDELETE(formatters[i]);
	}
}

bool QuantityFormatter::addIfAbsent(const char * variant, const UnicodeString & rawPattern, UErrorCode & status) 
{
	int32_t pluralIndex = StandardPlural::indexFromString(variant, status);
	if(U_FAILURE(status)) {
		return FALSE;
	}
	if(formatters[pluralIndex] != NULL) {
		return TRUE;
	}
	SimpleFormatter * newFmt = new SimpleFormatter(rawPattern, 0, 1, status);
	if(newFmt == NULL) {
		status = U_MEMORY_ALLOCATION_ERROR;
		return FALSE;
	}
	if(U_FAILURE(status)) {
		delete newFmt;
		return FALSE;
	}
	formatters[pluralIndex] = newFmt;
	return TRUE;
}

bool QuantityFormatter::isValid() const { return formatters[StandardPlural::OTHER] != NULL; }

const SimpleFormatter * QuantityFormatter::getByVariant(const char * variant) const 
{
	assert(isValid());
	int32_t pluralIndex = StandardPlural::indexOrOtherIndexFromString(variant);
	const SimpleFormatter * pattern = formatters[pluralIndex];
	SETIFZQ(pattern, formatters[StandardPlural::OTHER]);
	return pattern;
}

UnicodeString & QuantityFormatter::format(const Formattable &number, const NumberFormat &fmt, const PluralRules &rules, UnicodeString & appendTo, FieldPosition &pos, UErrorCode & status) const 
{
	UnicodeString formattedNumber;
	StandardPlural::Form p = selectPlural(number, fmt, rules, formattedNumber, pos, status);
	if(U_FAILURE(status)) {
		return appendTo;
	}
	const SimpleFormatter * pattern = formatters[p];
	if(!pattern) {
		pattern = formatters[StandardPlural::OTHER];
		if(!pattern) {
			status = U_INVALID_STATE_ERROR;
			return appendTo;
		}
	}
	return format(*pattern, formattedNumber, appendTo, pos, status);
}

// The following methods live here so that class PluralRules does not depend on number formatting,
// and the SimpleFormatter does not depend on FieldPosition.

StandardPlural::Form QuantityFormatter::selectPlural(const Formattable &number, const NumberFormat &fmt, const PluralRules &rules,
    UnicodeString & formattedNumber, FieldPosition &pos, UErrorCode & status) 
{
	if(U_FAILURE(status)) {
		return StandardPlural::OTHER;
	}
	UnicodeString pluralKeyword;
	const DecimalFormat * decFmt = dynamic_cast<const DecimalFormat *>(&fmt);
	if(decFmt != NULL) {
		number::impl::DecimalQuantity dq;
		decFmt->formatToDecimalQuantity(number, dq, status);
		if(U_FAILURE(status)) {
			return StandardPlural::OTHER;
		}
		pluralKeyword = rules.select(dq);
		decFmt->format(number, formattedNumber, pos, status);
	}
	else {
		if(number.getType() == Formattable::kDouble) {
			pluralKeyword = rules.select(number.getDouble());
		}
		else if(number.getType() == Formattable::kLong) {
			pluralKeyword = rules.select(number.getLong());
		}
		else if(number.getType() == Formattable::kInt64) {
			pluralKeyword = rules.select((double)number.getInt64());
		}
		else {
			status = U_ILLEGAL_ARGUMENT_ERROR;
			return StandardPlural::OTHER;
		}
		fmt.format(number, formattedNumber, pos, status);
	}
	return StandardPlural::orOtherFromString(pluralKeyword);
}

void QuantityFormatter::formatAndSelect(double quantity, const NumberFormat& fmt, const PluralRules& rules,
    FormattedStringBuilder& output, StandardPlural::Form& pluralForm, UErrorCode & status) 
{
	UnicodeString pluralKeyword;
	const DecimalFormat* df = dynamic_cast<const DecimalFormat*>(&fmt);
	if(df != nullptr) {
		number::impl::UFormattedNumberData fn;
		fn.quantity.setToDouble(quantity);
		const number::LocalizedNumberFormatter* lnf = df->toNumberFormatter(status);
		if(U_FAILURE(status)) {
			return;
		}
		lnf->formatImpl(&fn, status);
		if(U_FAILURE(status)) {
			return;
		}
		output = std::move(fn.getStringRef());
		pluralKeyword = rules.select(fn.quantity);
	}
	else {
		UnicodeString result;
		fmt.format(quantity, result, status);
		if(U_FAILURE(status)) {
			return;
		}
		// This code path is probably RBNF. Use the generic numeric field.
		output.append(result, kGeneralNumericField, status);
		if(U_FAILURE(status)) {
			return;
		}
		pluralKeyword = rules.select(quantity);
	}
	pluralForm = StandardPlural::orOtherFromString(pluralKeyword);
}

UnicodeString & QuantityFormatter::format(const SimpleFormatter &pattern, const UnicodeString & value,
    UnicodeString & appendTo, FieldPosition &pos, UErrorCode & status) 
{
	if(U_FAILURE(status)) {
		return appendTo;
	}
	const UnicodeString * param = &value;
	int32_t offset;
	pattern.formatAndAppend(&param, 1, appendTo, &offset, 1, status);
	if(pos.getBeginIndex() != 0 || pos.getEndIndex() != 0) {
		if(offset >= 0) {
			pos.setBeginIndex(pos.getBeginIndex() + offset);
			pos.setEndIndex(pos.getEndIndex() + offset);
		}
		else {
			pos.setBeginIndex(0);
			pos.setEndIndex(0);
		}
	}
	return appendTo;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
