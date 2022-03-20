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
#include <google/protobuf/util/internal/object_writer.h>
#include <google/protobuf/util/internal/datapiece.h>

namespace google {
namespace protobuf {
namespace util {
namespace converter {
// static
void ObjectWriter::RenderDataPieceTo(const DataPiece& data,
    StringPiece name, ObjectWriter* ow) {
	switch(data.type()) {
		case DataPiece::TYPE_INT32: {
		    ow->RenderInt32(name, data.ToInt32().value());
		    break;
	    }
		case DataPiece::TYPE_INT64: {
		    ow->RenderInt64(name, data.ToInt64().value());
		    break;
	    }
		case DataPiece::TYPE_UINT32: {
		    ow->RenderUint32(name, data.ToUint32().value());
		    break;
	    }
		case DataPiece::TYPE_UINT64: {
		    ow->RenderUint64(name, data.ToUint64().value());
		    break;
	    }
		case DataPiece::TYPE_DOUBLE: {
		    ow->RenderDouble(name, data.ToDouble().value());
		    break;
	    }
		case DataPiece::TYPE_FLOAT: {
		    ow->RenderFloat(name, data.ToFloat().value());
		    break;
	    }
		case DataPiece::TYPE_BOOL: {
		    ow->RenderBool(name, data.ToBool().value());
		    break;
	    }
		case DataPiece::TYPE_STRING: {
		    ow->RenderString(name, data.ToString().value());
		    break;
	    }
		case DataPiece::TYPE_BYTES: {
		    ow->RenderBytes(name, data.ToBytes().value());
		    break;
	    }
		case DataPiece::TYPE_NULL: {
		    ow->RenderNull(name);
		    break;
	    }
		default:
		    break;
	}
}
}  // namespace converter
}  // namespace util
}  // namespace protobuf
}  // namespace google
