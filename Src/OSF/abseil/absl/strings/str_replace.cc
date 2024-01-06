// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace strings_internal {
using FixedMapping = std::initializer_list<std::pair<absl::string_view, absl::string_view> >;

// Applies the ViableSubstitutions in subs_ptr to the absl::string_view s, and
// stores the result in *result_ptr. Returns the number of substitutions that
// occurred.
int ApplySubstitutions(absl::string_view s, std::vector <strings_internal::ViableSubstitution>* subs_ptr, std::string* result_ptr) 
{
	auto & subs = *subs_ptr;
	int substitutions = 0;
	size_t pos = 0;
	while(!subs.empty()) {
		auto & sub = subs.back();
		if(sub.offset >= pos) {
			if(pos <= s.size()) {
				StrAppend(result_ptr, s.substr(pos, sub.offset - pos), sub.replacement);
			}
			pos = sub.offset + sub.old.size();
			substitutions += 1;
		}
		sub.offset = s.find(sub.old, pos);
		if(sub.offset == s.npos) {
			subs.pop_back();
		}
		else {
			// Insertion sort to ensure the last ViableSubstitution continues to be
			// before all the others.
			size_t index = subs.size();
			while(--index && subs[index - 1].OccursBefore(subs[index])) {
				std::swap(subs[index], subs[index - 1]);
			}
		}
	}
	result_ptr->append(s.data() + pos, s.size() - pos);
	return substitutions;
}
}  // namespace strings_internal

// We can implement this in terms of the generic StrReplaceAll, but
// we must specify the template overload because C++ cannot deduce the type
// of an initializer_list parameter to a function, and also if we don't specify
// the type, we just call ourselves.
//
// Note that we implement them here, rather than in the header, so that they
// aren't inlined.

std::string StrReplaceAll(absl::string_view s, strings_internal::FixedMapping replacements) 
{
	return StrReplaceAll<strings_internal::FixedMapping>(s, replacements);
}

int StrReplaceAll(strings_internal::FixedMapping replacements, std::string* target) 
{
	return StrReplaceAll<strings_internal::FixedMapping>(replacements, target);
}

ABSL_NAMESPACE_END
}  // namespace absl
