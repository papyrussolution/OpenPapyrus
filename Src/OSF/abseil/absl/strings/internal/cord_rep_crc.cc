// Copyright 2021 The Abseil Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace cord_internal {
CordRepCrc* CordRepCrc::New(CordRep* child, uint32_t crc) {
	assert(child != nullptr);
	if(child->IsCrc()) {
		if(child->refcount.IsOne()) {
			child->crc()->crc = crc;
			return child->crc();
		}
		CordRep* old = child;
		child = old->crc()->child;
		CordRep::Ref(child);
		CordRep::Unref(old);
	}
	auto* new_cordrep = new CordRepCrc;
	new_cordrep->length = child->length;
	new_cordrep->tag = cord_internal::CRC;
	new_cordrep->child = child;
	new_cordrep->crc = crc;
	return new_cordrep;
}

void CordRepCrc::Destroy(CordRepCrc* node) 
{
	CordRep::Unref(node->child);
	delete node;
}
}  // namespace cord_internal
ABSL_NAMESPACE_END
}  // namespace absl
