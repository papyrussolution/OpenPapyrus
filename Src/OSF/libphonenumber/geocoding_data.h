// Copyright (C) 2012 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// This file is generated automatically, do not edit it manually.
//
#ifndef I18N_PHONENUMBERS_GEOCODING_DATA
#define I18N_PHONENUMBERS_GEOCODING_DATA

namespace i18n {
namespace phonenumbers {
struct CountryLanguages {
	const char** available_languages; // Sorted array of language codes.
	const int available_languages_size; // Number of elements in available_languages.
};

struct PrefixDescriptions {
	const int32* prefixes; // Sorted array of phone number prefixes.
	const int prefixes_size; // Number of elements in prefixes.
	const char** descriptions; // Array of phone number prefix descriptions, mapped one to one to prefixes.
	const int32* possible_lengths; // Sorted array of unique prefix lengths in base 10.
	const int possible_lengths_size; // Number of elements in possible_lengths.
};

// Returns a sorted array of country calling codes.
const int* get_country_calling_codes();

// Returns the number of country calling codes in
// get_country_calling_codes() array.
int get_country_calling_codes_size();

// Returns the CountryLanguages record for country at index, index
// being in [0, get_country_calling_codes_size()).
const CountryLanguages* get_country_languages(int index);

// Returns a sorted array of prefix language code pairs like
// "1_de" or "82_ko".
const char** get_prefix_language_code_pairs();

// Returns the number of elements in
// get_prefix_language_code_pairs()
int get_prefix_language_code_pairs_size();

// Returns the PrefixDescriptions for language/code pair at index,
// index being in [0, get_prefix_language_code_pairs_size()).
const PrefixDescriptions* get_prefix_descriptions(int index);
}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_GEOCODING_DATA
