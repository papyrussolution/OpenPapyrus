// Copyright (C) 2012 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Patrick Mezard
//
#include <libphonenumber-internal.h>
#pragma hdrstop
#include "absl/synchronization/mutex.h"

namespace i18n {
	namespace phonenumbers {
		using icu::UnicodeString;
		using std::string;

		namespace {
			// Returns true if s1 comes strictly before s2 in lexicographic order.
			bool IsLowerThan(const char* s1, const char* s2) { return strcmp(s1, s2) < 0; }
		}

		PhoneNumberOfflineGeocoder::PhoneNumberOfflineGeocoder() 
		{
			Init(get_country_calling_codes(), get_country_calling_codes_size(),
				get_country_languages, get_prefix_language_code_pairs(),
				get_prefix_language_code_pairs_size(), get_prefix_descriptions);
		}

		PhoneNumberOfflineGeocoder::PhoneNumberOfflineGeocoder(const int* country_calling_codes, int country_calling_codes_size,
			country_languages_getter get_country_languages,
			const char** prefix_language_code_pairs,
			int prefix_language_code_pairs_size,
			prefix_descriptions_getter get_prefix_descriptions) {
			Init(country_calling_codes, country_calling_codes_size,
				get_country_languages, prefix_language_code_pairs,
				prefix_language_code_pairs_size, get_prefix_descriptions);
		}

		void PhoneNumberOfflineGeocoder::Init(const int* country_calling_codes, int country_calling_codes_size,
			country_languages_getter get_country_languages, const char** prefix_language_code_pairs,
			int prefix_language_code_pairs_size, prefix_descriptions_getter get_prefix_descriptions) 
		{
			phone_util_ = PhoneNumberUtil::GetInstance();
			provider_.reset(new MappingFileProvider(country_calling_codes,
				country_calling_codes_size,
				get_country_languages));
			prefix_language_code_pairs_ = prefix_language_code_pairs;
			prefix_language_code_pairs_size_ = prefix_language_code_pairs_size;
			get_prefix_descriptions_ = get_prefix_descriptions;
		}
		PhoneNumberOfflineGeocoder::~PhoneNumberOfflineGeocoder() 
		{
			absl::MutexLock l(&mu_);
			gtl::STLDeleteContainerPairSecondPointers(available_maps_.begin(), available_maps_.end());
		}
		const AreaCodeMap* PhoneNumberOfflineGeocoder::GetPhonePrefixDescriptions(int prefix, const string& language, const string& script, const string& region) const 
		{
			string filename;
			provider_->GetFileName(prefix, language, script, region, &filename);
			if(filename.empty()) {
				return NULL;
			}
			AreaCodeMaps::const_iterator it = available_maps_.find(filename);
			if(it == available_maps_.end()) {
				it = LoadAreaCodeMapFromFile(filename);
				if(it == available_maps_.end()) {
					return NULL;
				}
			}
			return it->second;
		}
		PhoneNumberOfflineGeocoder::AreaCodeMaps::const_iterator PhoneNumberOfflineGeocoder::LoadAreaCodeMapFromFile(const string& filename) const 
		{
			const char** const prefix_language_code_pairs_end = prefix_language_code_pairs_ + prefix_language_code_pairs_size_;
			const char** const prefix_language_code_pair = std::lower_bound(prefix_language_code_pairs_,
				prefix_language_code_pairs_end, filename.c_str(), IsLowerThan);
			if(prefix_language_code_pair != prefix_language_code_pairs_end && filename.compare(*prefix_language_code_pair) == 0) {
				AreaCodeMap* const m = new AreaCodeMap();
				m->ReadAreaCodeMap(get_prefix_descriptions_(prefix_language_code_pair - prefix_language_code_pairs_));
				return available_maps_.insert(AreaCodeMaps::value_type(filename, m)).first;
			}
			return available_maps_.end();
		}

		string PhoneNumberOfflineGeocoder::GetCountryNameForNumber(const PhoneNumber& number, const Locale& language) const {
			string region_code;
			phone_util_->GetRegionCodeForNumber(number, &region_code);
			return GetRegionDisplayName(&region_code, language);
		}

		string PhoneNumberOfflineGeocoder::GetRegionDisplayName(const string* region_code, const Locale& language) const 
		{
			if(region_code == NULL || region_code->compare("ZZ") == 0 ||
				region_code->compare(PhoneNumberUtil::kRegionCodeForNonGeoEntity) == 0) {
				return "";
			}
			UnicodeString udisplay_country;
			icu::Locale("", region_code->c_str()).getDisplayCountry(language, udisplay_country);
			string display_country;
			udisplay_country.toUTF8String(display_country);
			return display_country;
		}

		string PhoneNumberOfflineGeocoder::GetDescriptionForValidNumber(const PhoneNumber& number, const Locale& language) const 
		{
			const char* const description = GetAreaDescription(number, language.getLanguage(), "", language.getCountry());
			return *description != '\0' ? description : GetCountryNameForNumber(number, language);
		}
		string PhoneNumberOfflineGeocoder::GetDescriptionForValidNumber(const PhoneNumber& number, const Locale& language,
			const string& user_region) const 
		{
			// If the user region matches the number's region, then we just show the
			// lower-level description, if one exists - if no description exists, we will
			// show the region(country) name for the number.
			string region_code;
			phone_util_->GetRegionCodeForNumber(number, &region_code);
			if(user_region.compare(region_code) == 0) {
				return GetDescriptionForValidNumber(number, language);
			}
			// Otherwise, we just show the region(country) name for now.
			return GetRegionDisplayName(&region_code, language);
		}
		string PhoneNumberOfflineGeocoder::GetDescriptionForNumber(const PhoneNumber& number, const Locale& locale) const 
		{
			PhoneNumberUtil::PhoneNumberType number_type = phone_util_->GetNumberType(number);
			if(number_type == PhoneNumberUtil::UNKNOWN) {
				return "";
			}
			else if(!phone_util_->IsNumberGeographical(number_type,
				number.country_code())) {
				return GetCountryNameForNumber(number, locale);
			}
			return GetDescriptionForValidNumber(number, locale);
		}
		string PhoneNumberOfflineGeocoder::GetDescriptionForNumber(const PhoneNumber& number, const Locale& language,
			const string& user_region) const 
		{
			PhoneNumberUtil::PhoneNumberType number_type = phone_util_->GetNumberType(number);
			if(number_type == PhoneNumberUtil::UNKNOWN) {
				return "";
			}
			else if(!phone_util_->IsNumberGeographical(number_type,
				number.country_code())) {
				return GetCountryNameForNumber(number, language);
			}
			return GetDescriptionForValidNumber(number, language, user_region);
		}
		const char* PhoneNumberOfflineGeocoder::GetAreaDescription(const PhoneNumber& number, const string& lang, const string& script,
			const string& region) const 
		{
			const int country_calling_code = number.country_code();
			// NANPA area is not split in C++ code.
			const int phone_prefix = country_calling_code;
			absl::MutexLock l(&mu_);
			const AreaCodeMap* const descriptions = GetPhonePrefixDescriptions(
				phone_prefix, lang, script, region);
			const char* description = descriptions ? descriptions->Lookup(number) : NULL;
			// When a location is not available in the requested language, fall back to
			// English.
			if((!description || *description == '\0') && MayFallBackToEnglish(lang)) {
				const AreaCodeMap* default_descriptions = GetPhonePrefixDescriptions(
					phone_prefix, "en", "", "");
				if(!default_descriptions) {
					return "";
				}
				description = default_descriptions->Lookup(number);
			}
			return description ? description : "";
		}

		// Don't fall back to English if the requested language is among the following:
		// - Chinese
		// - Japanese
		// - Korean
		bool PhoneNumberOfflineGeocoder::MayFallBackToEnglish(const string& lang) const 
		{
			return lang.compare("zh") && lang.compare("ja") && lang.compare("ko");
		}
	}  // namespace phonenumbers
}  // namespace i18n
