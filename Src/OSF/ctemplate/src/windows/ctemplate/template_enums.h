// Copyright (c) 2006, Google Inc. All rights reserved.
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
// Alas that we can't forward-declare enums!  These are the ones
// used by multiple files

#ifndef TEMPLATE_TEMPLATE_ENUMS_H_
#define TEMPLATE_TEMPLATE_ENUMS_H_

namespace ctemplate {

// Enums for GetTemplate flag values
enum Strip { DO_NOT_STRIP, STRIP_BLANK_LINES, STRIP_WHITESPACE,
             NUM_STRIPS };   // sentinel value

}

#endif  // TEMPLATE_TEMPLATE_ENUMS_H_
