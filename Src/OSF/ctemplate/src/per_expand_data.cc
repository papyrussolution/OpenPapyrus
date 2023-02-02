// Copyright (c) 2009, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// This contains some implementation of PerExpandData that is still simple
// but is not conveniently defined in the header file, e.g., because it would
// introduce new include dependencies.

#include <ctemplate-internal.h>
#pragma hdrstop

namespace ctemplate {
using std::string;

bool PerExpandData::DataEq::operator()(const char* s1, const char* s2) const 
{
	return ((s1 == 0 && s2 == 0) || (s1 && s2 && *s1 == *s2 && sstreq(s1, s2)));
}

PerExpandData::~PerExpandData() 
{
	delete map_;
}

TemplateAnnotator* PerExpandData::annotator() const 
{
	if(annotator_)
		return annotator_;
	else {
		// TextTemplateAnnotator has no static state.  So direct static definition should be safe.
		static TextTemplateAnnotator g_default_annotator;
		return &g_default_annotator;
	}
}

void PerExpandData::InsertForModifiers(const char* key, const void * value) 
{
	SETIFZ(map_, new DataMap);
	(*map_)[key] = value;
}

// Retrieve data specific to this Expand call. Returns NULL if key
// is not found.  This should only be used by template modifiers.
const void * PerExpandData::LookupForModifiers(const char* key) const { return map_ ? find_ptr2(*map_, key) : NULL; }
}
