// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

// Disable LeakSanitizer when this file is linked in.
// This function overrides __lsan_is_turned_off from sanitizer/lsan_interface.h
extern "C" int __lsan_is_turned_off();
extern "C" int __lsan_is_turned_off() { return 1; }
