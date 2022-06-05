// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
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
// Google C++ Testing and Mocking Framework (Google Test)
//
// Sometimes it's desirable to build Google Test by compiling a single file.
// This file serves this purpose.

// This line ensures that gtest.h can be compiled on its own, even
// when it's fused.
#include "gtest/internal/gtest-build-internal.h"
#pragma hdrstop
//#include "gtest/gtest.h"
// The following lines pull in the real gtest *.cc files.
//#include "src/gtest-assertion-result.cc"
//#include "src/gtest-death-test.cc"
//#include "src/gtest-filepath.cc"
//#include "src/gtest-matchers.cc"
//#include "src/gtest-port.cc"
//#include "src/gtest-printers.cc"
//#include "src/gtest-test-part.cc"
//#include "src/gtest-typed-test.cc"
//#include "src/gtest.cc"
