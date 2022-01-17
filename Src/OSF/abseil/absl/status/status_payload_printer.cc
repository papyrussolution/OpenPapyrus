// Copyright 2019 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/status/status_payload_printer.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace status_internal {
ABSL_INTERNAL_ATOMIC_HOOK_ATTRIBUTES
static absl::base_internal::AtomicHook<StatusPayloadPrinter> storage;

void SetStatusPayloadPrinter(StatusPayloadPrinter printer) {
	storage.Store(printer);
}

StatusPayloadPrinter GetStatusPayloadPrinter() {
	return storage.Load();
}
}  // namespace status_internal
ABSL_NAMESPACE_END
}  // namespace absl
