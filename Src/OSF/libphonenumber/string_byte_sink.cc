// Copyright (C) 2012 The Libphonenumber Authors
// @license Apache License 2.0
//
#include <libphonenumber-internal.h>
#pragma hdrstop
#include "string_byte_sink.h"

using std::string;

namespace i18n {
	namespace phonenumbers {
		StringByteSink::StringByteSink(string* dest) : dest_(dest) 
		{
		}
		StringByteSink::~StringByteSink() 
		{
		}
		void StringByteSink::Append(const char* data, int32_t n) 
		{
			dest_->append(data, n);
		}
	}  // namespace phonenumbers
}  // namespace i18n
