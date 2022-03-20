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
#include <google/protobuf/util/internal/json_objectwriter.h>
#include <google/protobuf/stubs/casts.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/util/internal/utility.h>
#include <google/protobuf/util/internal/json_escaping.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace util {
namespace converter {
JsonObjectWriter::~JsonObjectWriter() {
	if(element_ && !element_->is_root()) {
		GOOGLE_LOG(WARNING) << "JsonObjectWriter was not fully closed.";
	}
}

JsonObjectWriter* JsonObjectWriter::StartObject(StringPiece name) {
	WritePrefix(name);
	WriteChar('{');
	PushObject();
	return this;
}

JsonObjectWriter* JsonObjectWriter::EndObject() {
	Pop();
	WriteChar('}');
	if(element() && element()->is_root()) NewLine();
	return this;
}

JsonObjectWriter* JsonObjectWriter::StartList(StringPiece name) {
	WritePrefix(name);
	WriteChar('[');
	PushArray();
	return this;
}

JsonObjectWriter* JsonObjectWriter::EndList() {
	Pop();
	WriteChar(']');
	if(element()->is_root()) NewLine();
	return this;
}

JsonObjectWriter* JsonObjectWriter::RenderBool(StringPiece name,
    bool value) {
	return RenderSimple(name, value ? "true" : "false");
}

JsonObjectWriter* JsonObjectWriter::RenderInt32(StringPiece name,
    int32_t value) {
	return RenderSimple(name, StrCat(value));
}

JsonObjectWriter* JsonObjectWriter::RenderUint32(StringPiece name,
    uint32_t value) {
	return RenderSimple(name, StrCat(value));
}

JsonObjectWriter* JsonObjectWriter::RenderInt64(StringPiece name,
    int64_t value) {
	WritePrefix(name);
	WriteChar('"');
	WriteRawString(StrCat(value));
	WriteChar('"');
	return this;
}

JsonObjectWriter* JsonObjectWriter::RenderUint64(StringPiece name,
    uint64_t value) {
	WritePrefix(name);
	WriteChar('"');
	WriteRawString(StrCat(value));
	WriteChar('"');
	return this;
}

JsonObjectWriter* JsonObjectWriter::RenderDouble(StringPiece name,
    double value) {
	if(std::isfinite(value)) {
		return RenderSimple(name, SimpleDtoa(value));
	}

	// Render quoted with NaN/Infinity-aware DoubleAsString.
	return RenderString(name, DoubleAsString(value));
}

JsonObjectWriter* JsonObjectWriter::RenderFloat(StringPiece name,
    float value) {
	if(std::isfinite(value)) {
		return RenderSimple(name, SimpleFtoa(value));
	}

	// Render quoted with NaN/Infinity-aware FloatAsString.
	return RenderString(name, FloatAsString(value));
}

JsonObjectWriter* JsonObjectWriter::RenderString(StringPiece name,
    StringPiece value) {
	WritePrefix(name);
	WriteChar('"');
	JsonEscaping::Escape(value, &sink_);
	WriteChar('"');
	return this;
}

JsonObjectWriter* JsonObjectWriter::RenderBytes(StringPiece name,
    StringPiece value) {
	WritePrefix(name);
	std::string base64;

	if(use_websafe_base64_for_bytes_)
		WebSafeBase64EscapeWithPadding(std::string(value), &base64);
	else
		Base64Escape(value, &base64);

	WriteChar('"');
	// TODO(wpoon): Consider a ByteSink solution that writes the base64 bytes
	//              directly to the stream, rather than first putting them
	//              into a string and then writing them to the stream.
	stream_->WriteRaw(base64.data(), base64.size());
	WriteChar('"');
	return this;
}

JsonObjectWriter* JsonObjectWriter::RenderNull(StringPiece name) {
	return RenderSimple(name, "null");
}

JsonObjectWriter* JsonObjectWriter::RenderNullAsEmpty(StringPiece name) {
	return RenderSimple(name, "");
}

void JsonObjectWriter::WritePrefix(StringPiece name) {
	bool not_first = !element()->is_first();
	if(not_first) WriteChar(',');
	if(not_first || !element()->is_root()) NewLine();
	if(!name.empty() || element()->is_json_object()) {
		WriteChar('"');
		if(!name.empty()) {
			JsonEscaping::Escape(name, &sink_);
		}
		WriteRawString("\":");
		if(!indent_string_.empty()) WriteChar(' ');
	}
}
}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google
