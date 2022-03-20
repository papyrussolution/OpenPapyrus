// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
#include <protobuf-internal.h>
#pragma hdrstop

namespace google {
namespace protobuf {
namespace internal {
std::string GetTypeUrl(StringPiece message_name,
    StringPiece type_url_prefix) {
	if(!type_url_prefix.empty() &&
	    type_url_prefix[type_url_prefix.size() - 1] == '/') {
		return StrCat(type_url_prefix, message_name);
	}
	else {
		return StrCat(type_url_prefix, "/", message_name);
	}
}

const char kAnyFullTypeName[] = "google.protobuf.Any";
const char kTypeGoogleApisComPrefix[] = "type.googleapis.com/";
const char kTypeGoogleProdComPrefix[] = "type.googleprod.com/";

bool AnyMetadata::InternalPackFrom(Arena* arena, const MessageLite& message,
    StringPiece type_url_prefix,
    StringPiece type_name) {
	type_url_->Set(&::google::protobuf::internal::GetEmptyString(),
	    GetTypeUrl(type_name, type_url_prefix), arena);
	return message.SerializeToString(
		value_->Mutable(ArenaStringPtr::EmptyDefault{}, arena));
}

bool AnyMetadata::InternalUnpackTo(StringPiece type_name,
    MessageLite* message) const {
	if(!InternalIs(type_name)) {
		return false;
	}
	return message->ParseFromString(value_->Get());
}

bool AnyMetadata::InternalIs(StringPiece type_name) const {
	StringPiece type_url = type_url_->Get();
	return type_url.size() >= type_name.size() + 1 &&
	       type_url[type_url.size() - type_name.size() - 1] == '/' &&
	       HasSuffixString(type_url, type_name);
}

bool ParseAnyTypeUrl(StringPiece type_url, std::string* url_prefix,
    std::string* full_type_name) {
	size_t pos = type_url.find_last_of('/');
	if(pos == std::string::npos || pos + 1 == type_url.size()) {
		return false;
	}
	if(url_prefix) {
		*url_prefix = std::string(type_url.substr(0, pos + 1));
	}
	*full_type_name = std::string(type_url.substr(pos + 1));
	return true;
}

bool ParseAnyTypeUrl(StringPiece type_url, std::string* full_type_name) {
	return ParseAnyTypeUrl(type_url, nullptr, full_type_name);
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google
