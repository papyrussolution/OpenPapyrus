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
#include <google/protobuf/util/internal/type_info_test_helper.h>
#include <google/protobuf/util/internal/default_value_objectwriter.h>
#include <google/protobuf/util/internal/type_info.h>
#include <google/protobuf/util/internal/constants.h>
#include <google/protobuf/util/internal/protostream_objectsource.h>
#include <google/protobuf/util/internal/protostream_objectwriter.h>
#include <google/protobuf/util/type_resolver.h>
#include <google/protobuf/util/type_resolver_util.h>

namespace google {
namespace protobuf {
namespace util {
namespace converter {
namespace testing {
void TypeInfoTestHelper::ResetTypeInfo(const std::vector<const Descriptor*>& descriptors) {
	switch(type_) {
		case USE_TYPE_RESOLVER: {
		    const DescriptorPool* pool = descriptors[0]->file()->pool();
		    for(int i = 1; i < descriptors.size(); ++i) {
			    GOOGLE_CHECK(pool == descriptors[i]->file()->pool())
				    << "Descriptors from different pools are not supported.";
		    }
		    type_resolver_.reset(
			    NewTypeResolverForDescriptorPool(kTypeServiceBaseUrl, pool));
		    typeinfo_.reset(TypeInfo::NewTypeInfo(type_resolver_.get()));
		    return;
	    }
	}
	GOOGLE_LOG(FATAL) << "Can not reach here.";
}

void TypeInfoTestHelper::ResetTypeInfo(const Descriptor* descriptor) {
	std::vector<const Descriptor*> descriptors;
	descriptors.push_back(descriptor);
	ResetTypeInfo(descriptors);
}

void TypeInfoTestHelper::ResetTypeInfo(const Descriptor* descriptor1,
    const Descriptor* descriptor2) {
	std::vector<const Descriptor*> descriptors;
	descriptors.push_back(descriptor1);
	descriptors.push_back(descriptor2);
	ResetTypeInfo(descriptors);
}

TypeInfo* TypeInfoTestHelper::GetTypeInfo() {
	return typeinfo_.get();
}

ProtoStreamObjectSource* TypeInfoTestHelper::NewProtoSource(io::CodedInputStream* coded_input, const std::string& type_url,
    ProtoStreamObjectSource::RenderOptions render_options) {
	const google::protobuf::Type* type = typeinfo_->GetTypeByTypeUrl(type_url);
	switch(type_) {
		case USE_TYPE_RESOLVER: {
		    return new ProtoStreamObjectSource(coded_input, type_resolver_.get(),
			       *type, render_options);
	    }
	}
	GOOGLE_LOG(FATAL) << "Can not reach here.";
	return nullptr;
}

ProtoStreamObjectWriter* TypeInfoTestHelper::NewProtoWriter(const std::string& type_url, strings::ByteSink* output,
    ErrorListener* listener, const ProtoStreamObjectWriter::Options& options) {
	const google::protobuf::Type* type = typeinfo_->GetTypeByTypeUrl(type_url);
	switch(type_) {
		case USE_TYPE_RESOLVER: {
		    return new ProtoStreamObjectWriter(type_resolver_.get(), *type, output,
			       listener, options);
	    }
	}
	GOOGLE_LOG(FATAL) << "Can not reach here.";
	return nullptr;
}

DefaultValueObjectWriter* TypeInfoTestHelper::NewDefaultValueWriter(const std::string& type_url, ObjectWriter* writer) {
	const google::protobuf::Type* type = typeinfo_->GetTypeByTypeUrl(type_url);
	switch(type_) {
		case USE_TYPE_RESOLVER: {
		    return new DefaultValueObjectWriter(type_resolver_.get(), *type, writer);
	    }
	}
	GOOGLE_LOG(FATAL) << "Can not reach here.";
	return nullptr;
}
}  // namespace testing
}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google
