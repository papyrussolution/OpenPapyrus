// Copyright 2018 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/container/internal/raw_hash_set.h"
#include "absl/base/attributes.h"
#include "absl/base/config.h"
#include "absl/base/dynamic_annotations.h"
#include "absl/hash/hash.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace container_internal {
// We have space for `growth_left` before a single block of control bytes. A
// single block of empty control bytes for tables without any slots allocated.
// This enables removing a branch in the hot path of find(). In order to ensure
// that the control bytes are aligned to 16, we have 16 bytes before the control
// bytes even though growth_left only needs 8.
constexpr ctrl_t ZeroCtrlT() {
	return static_cast<ctrl_t>(0);
}

alignas(16) ABSL_CONST_INIT ABSL_DLL const ctrl_t kEmptyGroup[32] = {
	ZeroCtrlT(),       ZeroCtrlT(),    ZeroCtrlT(),    ZeroCtrlT(),
	ZeroCtrlT(),       ZeroCtrlT(),    ZeroCtrlT(),    ZeroCtrlT(),
	ZeroCtrlT(),       ZeroCtrlT(),    ZeroCtrlT(),    ZeroCtrlT(),
	ZeroCtrlT(),       ZeroCtrlT(),    ZeroCtrlT(),    ZeroCtrlT(),
	ctrl_t::kSentinel, ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty,
	ctrl_t::kEmpty,    ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty,
	ctrl_t::kEmpty,    ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty,
	ctrl_t::kEmpty,    ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty
};

#ifdef ABSL_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr size_t Group::kWidth;
#endif

namespace {
// Returns "random" seed.
inline size_t RandomSeed() {
#ifdef ABSL_HAVE_THREAD_LOCAL
	static thread_local size_t counter = 0;
	// On Linux kernels >= 5.4 the MSAN runtime has a false-positive when
	// accessing thread local storage data from loaded libraries
	// (https://github.com/google/sanitizers/issues/1265), for this reason counter
	// needs to be annotated as initialized.
	ABSL_ANNOTATE_MEMORY_IS_INITIALIZED(&counter, sizeof(size_t));
	size_t value = ++counter;
#else   // ABSL_HAVE_THREAD_LOCAL
	static std::atomic<size_t> counter(0);
	size_t value = counter.fetch_add(1, std::memory_order_relaxed);
#endif  // ABSL_HAVE_THREAD_LOCAL
	return value ^ static_cast<size_t>(reinterpret_cast<uintptr_t>(&counter));
}
}  // namespace

GenerationType* EmptyGeneration() 
{
	if(SwisstableGenerationsEnabled()) {
		constexpr size_t kNumEmptyGenerations = 1024;
		static constexpr GenerationType kEmptyGenerations[kNumEmptyGenerations]{};
		return const_cast<GenerationType*>(
			&kEmptyGenerations[RandomSeed() % kNumEmptyGenerations]);
	}
	return nullptr;
}

bool CommonFieldsGenerationInfoEnabled::should_rehash_for_bug_detection_on_insert(const ctrl_t* ctrl, size_t capacity) const 
{
	if(reserved_growth_ == kReservedGrowthJustRanOut) return true;
	if(reserved_growth_ > 0) return false;
	// Note: we can't use the abseil-random library because abseil-random
	// depends on swisstable. We want to return true with probability
	// `min(1, RehashProbabilityConstant() / capacity())`. In order to do this,
	// we probe based on a random hash and see if the offset is less than
	// RehashProbabilityConstant().
	return probe(ctrl, capacity, absl::HashOf(RandomSeed())).offset() <
	       RehashProbabilityConstant();
}

bool ShouldInsertBackwards(size_t hash, const ctrl_t* ctrl) 
{
	// To avoid problems with weak hashes and single bit tests, we use % 13.
	// TODO(kfm,sbenza): revisit after we do unconditional mixing
	return (H1(hash, ctrl) ^ RandomSeed()) % 13 > 6;
}

void ConvertDeletedToEmptyAndFullToDeleted(ctrl_t* ctrl, size_t capacity) 
{
	assert(ctrl[capacity] == ctrl_t::kSentinel);
	assert(IsValidCapacity(capacity));
	for(ctrl_t* pos = ctrl; pos < ctrl + capacity; pos += Group::kWidth) {
		Group{pos}.ConvertSpecialToEmptyAndFullToDeleted(pos);
	}
	// Copy the cloned ctrl bytes.
	memcpy(ctrl + capacity + 1, ctrl, NumClonedBytes());
	ctrl[capacity] = ctrl_t::kSentinel;
}

// Extern template instantiation for inline function.
template FindInfo find_first_non_full(const CommonFields&, size_t);

FindInfo find_first_non_full_outofline(const CommonFields& common, size_t hash) 
{
	return find_first_non_full(common, hash);
}

// Returns the address of the ith slot in slots where each slot occupies
// slot_size.
static inline void* SlotAddress(void* slot_array, size_t slot,
    size_t slot_size) {
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(slot_array) +
	       (slot * slot_size));
}

// Returns the address of the slot just after slot assuming each slot has the
// specified size.
static inline void* NextSlot(void* slot, size_t slot_size) {
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(slot) + slot_size);
}

// Returns the address of the slot just before slot assuming each slot has the
// specified size.
static inline void* PrevSlot(void* slot, size_t slot_size) {
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(slot) - slot_size);
}

void DropDeletesWithoutResize(CommonFields& common,
    const PolicyFunctions& policy, void* tmp_space) {
	void* set = &common;
	void* slot_array = common.slot_array();
	const size_t capacity = common.capacity();
	assert(IsValidCapacity(capacity));
	assert(!is_small(capacity));
	// Algorithm:
	// - mark all DELETED slots as EMPTY
	// - mark all FULL slots as DELETED
	// - for each slot marked as DELETED
	//     hash = Hash(element)
	//     target = find_first_non_full(hash)
	//     if target is in the same group
	//       mark slot as FULL
	//     else if target is EMPTY
	//       transfer element to target
	//       mark slot as EMPTY
	//       mark target as FULL
	//     else if target is DELETED
	//       swap current element with target element
	//       mark target as FULL
	//       repeat procedure for current slot with moved from element (target)
	ctrl_t* ctrl = common.control();
	ConvertDeletedToEmptyAndFullToDeleted(ctrl, capacity);
	auto hasher = policy.hash_slot;
	auto transfer = policy.transfer;
	const size_t slot_size = policy.slot_size;

	size_t total_probe_length = 0;
	void* slot_ptr = SlotAddress(slot_array, 0, slot_size);
	for(size_t i = 0; i != capacity;
	    ++i, slot_ptr = NextSlot(slot_ptr, slot_size)) {
		assert(slot_ptr == SlotAddress(slot_array, i, slot_size));
		if(!IsDeleted(ctrl[i])) continue;
		const size_t hash = (*hasher)(set, slot_ptr);
		const FindInfo target = find_first_non_full(common, hash);
		const size_t new_i = target.offset;
		total_probe_length += target.probe_length;

		// Verify if the old and new i fall within the same group wrt the hash.
		// If they do, we don't need to move the object as it falls already in the
		// best probe we can.
		const size_t probe_offset = probe(common, hash).offset();
		const auto probe_index = [probe_offset, capacity](size_t pos) {
			    return ((pos - probe_offset) & capacity) / Group::kWidth;
		    };

		// Element doesn't move.
		if(ABSL_PREDICT_TRUE(probe_index(new_i) == probe_index(i))) {
			SetCtrl(common, i, H2(hash), slot_size);
			continue;
		}

		void* new_slot_ptr = SlotAddress(slot_array, new_i, slot_size);
		if(IsEmpty(ctrl[new_i])) {
			// Transfer element to the empty spot.
			// SetCtrl poisons/unpoisons the slots so we have to call it at the
			// right time.
			SetCtrl(common, new_i, H2(hash), slot_size);
			(*transfer)(set, new_slot_ptr, slot_ptr);
			SetCtrl(common, i, ctrl_t::kEmpty, slot_size);
		}
		else {
			assert(IsDeleted(ctrl[new_i]));
			SetCtrl(common, new_i, H2(hash), slot_size);
			// Until we are done rehashing, DELETED marks previously FULL slots.

			// Swap i and new_i elements.
			(*transfer)(set, tmp_space, new_slot_ptr);
			(*transfer)(set, new_slot_ptr, slot_ptr);
			(*transfer)(set, slot_ptr, tmp_space);

			// repeat the processing of the ith slot
			--i;
			slot_ptr = PrevSlot(slot_ptr, slot_size);
		}
	}
	ResetGrowthLeft(common);
	common.infoz().RecordRehash(total_probe_length);
}

void EraseMetaOnly(CommonFields& c, ctrl_t* it, size_t slot_size) {
	assert(IsFull(*it) && "erasing a dangling iterator");
	c.decrement_size();
	const auto index = static_cast<size_t>(it - c.control());
	const size_t index_before = (index - Group::kWidth) & c.capacity();
	const auto empty_after = Group(it).MaskEmpty();
	const auto empty_before = Group(c.control() + index_before).MaskEmpty();

	// We count how many consecutive non empties we have to the right and to the
	// left of `it`. If the sum is >= kWidth then there is at least one probe
	// window that might have seen a full group.
	bool was_never_full = empty_before && empty_after &&
	    static_cast<size_t>(empty_after.TrailingZeros()) +
	    empty_before.LeadingZeros() <
	    Group::kWidth;

	SetCtrl(c, index, was_never_full ? ctrl_t::kEmpty : ctrl_t::kDeleted,
	    slot_size);
	c.set_growth_left(c.growth_left() + (was_never_full ? 1 : 0));
	c.infoz().RecordErase();
}

void ClearBackingArray(CommonFields& c, const PolicyFunctions& policy,
    bool reuse) {
	c.set_size(0);
	if(reuse) {
		ResetCtrl(c, policy.slot_size);
		c.infoz().RecordStorageChanged(0, c.capacity());
	}
	else {
		// We need to record infoz before calling dealloc, which will unregister
		// infoz.
		c.infoz().RecordClearedReservation();
		c.infoz().RecordStorageChanged(0, 0);
		(*policy.dealloc)(c, policy);
		c.set_control(EmptyGroup());
		c.set_generation_ptr(EmptyGeneration());
		c.set_slots(nullptr);
		c.set_capacity(0);
	}
}
}  // namespace container_internal
ABSL_NAMESPACE_END
}  // namespace absl
