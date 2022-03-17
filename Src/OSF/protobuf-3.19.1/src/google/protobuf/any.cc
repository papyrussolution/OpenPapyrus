// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
#include <protobuf-internal.h>
#pragma hdrstop
#include <google/protobuf/port_def.inc>

namespace google {
	namespace protobuf {
		namespace internal {
			bool AnyMetadata::PackFrom(Arena* arena, const Message& message) {
				return PackFrom(arena, message, kTypeGoogleApisComPrefix);
			}

			bool AnyMetadata::PackFrom(Arena* arena, const Message& message, StringPiece type_url_prefix) 
			{
				type_url_->Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString(), GetTypeUrl(message.GetDescriptor()->full_name(), type_url_prefix), arena);
				return message.SerializeToString(value_->Mutable(ArenaStringPtr::EmptyDefault{}, arena));
			}

			bool AnyMetadata::UnpackTo(Message* message) const 
			{
				if(!InternalIs(message->GetDescriptor()->full_name())) {
					return false;
				}
				return message->ParseFromString(value_->Get());
			}

			bool GetAnyFieldDescriptors(const Message& message, const FieldDescriptor** type_url_field, const FieldDescriptor** value_field) 
			{
				const Descriptor* descriptor = message.GetDescriptor();
				if(descriptor->full_name() != kAnyFullTypeName) {
					return false;
				}
				*type_url_field = descriptor->FindFieldByNumber(1);
				*value_field = descriptor->FindFieldByNumber(2);
				return (*type_url_field != nullptr && (*type_url_field)->type() == FieldDescriptor::TYPE_STRING && *value_field != nullptr &&
					   (*value_field)->type() == FieldDescriptor::TYPE_BYTES);
			}
		}
	}
}

#include <google/protobuf/port_undef.inc>
