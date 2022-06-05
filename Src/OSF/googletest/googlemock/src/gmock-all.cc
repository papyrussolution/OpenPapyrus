// Copyright 2008, Google Inc.
// All rights reserved.
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
// Google C++ Mocking Framework (Google Mock)
//
// This file #includes all Google Mock implementation .cc files.  The
// purpose is to allow a user to build Google Mock by compiling this
// file alone.

// This line ensures that gmock.h can be compiled on its own, even
// when it's fused.
#include "gmock/internal/gmock-internal.h"
#pragma hdrstop
#include "gmock/gmock.h"

// The following lines pull in the real gmock *.cc files.
//#include "src/gmock-cardinalities.cc"
//#include "src/gmock-internal-utils.cc"
//#include "src/gmock-matchers.cc"
//#include "src/gmock-spec-builders.cc"
//#include "src/gmock.cc"
