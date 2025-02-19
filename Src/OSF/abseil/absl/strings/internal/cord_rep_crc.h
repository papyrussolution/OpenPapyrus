// Copyright 2021 The Abseil Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
//
#ifndef ABSL_STRINGS_INTERNAL_CORD_REP_CRC_H_
#define ABSL_STRINGS_INTERNAL_CORD_REP_CRC_H_

#include "absl/base/config.h"
#include "absl/base/optimization.h"
#include "absl/strings/internal/cord_internal.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace cord_internal {
// CordRepCrc is a CordRep node intended only to appear at the top level of a
// cord tree.  It associates an "expected CRC" with the contained data, to allow
// for easy passage of checksum data in Cord data flows.
//
// From Cord's perspective, the crc value has no semantics; any validation of
// the contained checksum is the user's responsibility.
struct CordRepCrc : public CordRep {
	CordRep* child;
	uint32_t crc;

	// Consumes `child` and returns a CordRepCrc prefixed tree containing `child`.
	// If the specified `child` is itself a CordRepCrc node, then this method
	// either replaces the existing node, or directly updates the crc value in it
	// depending on the node being shared or not, i.e.: refcount.IsOne().
	// `child` must not be null. Never returns null.
	static CordRepCrc* New(CordRep* child, uint32_t crc);

	// Destroys (deletes) the provided node. `node` must not be null.
	static void Destroy(CordRepCrc* node);
};

// Consumes `rep` and returns a CordRep* with any outer CordRepCrc wrapper
// removed.  This is usually a no-op (returning `rep`), but this will remove and
// unref an outer CordRepCrc node.
inline CordRep* RemoveCrcNode(CordRep* rep) {
	assert(rep != nullptr);
	if(ABSL_PREDICT_FALSE(rep->IsCrc())) {
		CordRep* child = rep->crc()->child;
		if(rep->refcount.IsOne()) {
			delete rep->crc();
		}
		else {
			CordRep::Ref(child);
			CordRep::Unref(rep);
		}
		return child;
	}
	return rep;
}

// Returns `rep` if it is not a CordRepCrc node, or its child if it is.
// Does not consume or create a reference on `rep` or the returned value.
inline CordRep* SkipCrcNode(CordRep* rep) {
	assert(rep != nullptr);
	if(ABSL_PREDICT_FALSE(rep->IsCrc())) {
		return rep->crc()->child;
	}
	else {
		return rep;
	}
}

inline const CordRep* SkipCrcNode(const CordRep* rep) {
	assert(rep != nullptr);
	if(ABSL_PREDICT_FALSE(rep->IsCrc())) {
		return rep->crc()->child;
	}
	else {
		return rep;
	}
}

inline CordRepCrc* CordRep::crc() {
	assert(IsCrc());
	return static_cast<CordRepCrc*>(this);
}

inline const CordRepCrc* CordRep::crc() const {
	assert(IsCrc());
	return static_cast<const CordRepCrc*>(this);
}
}  // namespace cord_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_STRINGS_INTERNAL_CORD_REP_CRC_H_
