// Copyright 2020 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace cord_internal {
ABSL_CONST_INIT std::atomic<bool> cord_btree_enabled(kCordEnableBtreeDefault);
ABSL_CONST_INIT std::atomic<bool> cord_ring_buffer_enabled(kCordEnableRingBufferDefault);
ABSL_CONST_INIT std::atomic<bool> shallow_subcords_enabled(kCordShallowSubcordsDefault);
ABSL_CONST_INIT std::atomic<bool> cord_btree_exhaustive_validation(false);

void CordRep::Destroy(CordRep* rep) 
{
	assert(rep != nullptr);
	absl::InlinedVector<CordRep*, Constants::kInlinedVectorSize> pending;
	while(true) {
		assert(!rep->refcount.IsImmortal());
		if(rep->tag == CONCAT) {
			CordRepConcat* rep_concat = rep->concat();
			CordRep* right = rep_concat->right;
			if(!right->refcount.Decrement()) {
				pending.push_back(right);
			}
			CordRep* left = rep_concat->left;
			delete rep_concat;
			rep = nullptr;
			if(!left->refcount.Decrement()) {
				rep = left;
				continue;
			}
		}
		else if(rep->tag == BTREE) {
			CordRepBtree::Destroy(rep->btree());
			rep = nullptr;
		}
		else if(rep->tag == RING) {
			CordRepRing::Destroy(rep->ring());
			rep = nullptr;
		}
		else if(rep->tag == EXTERNAL) {
			CordRepExternal::Delete(rep);
			rep = nullptr;
		}
		else if(rep->tag == SUBSTRING) {
			CordRepSubstring* rep_substring = rep->substring();
			CordRep* child = rep_substring->child;
			delete rep_substring;
			rep = nullptr;
			if(!child->refcount.Decrement()) {
				rep = child;
				continue;
			}
		}
		else if(rep->tag == CRC) {
			CordRepCrc::Destroy(rep->crc());
			rep = nullptr;
		}
		else {
			CordRepFlat::Delete(rep);
			rep = nullptr;
		}

		if(!pending.empty()) {
			rep = pending.back();
			pending.pop_back();
		}
		else {
			break;
		}
	}
}
}  // namespace cord_internal
ABSL_NAMESPACE_END
}  // namespace absl
