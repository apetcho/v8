// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/common/globals.h"
#include "src/heap/basic-memory-chunk.h"
#include "src/heap/heap-inl.h"
#include "src/objects/cell.h"
#include "src/objects/feedback-cell.h"
#include "src/objects/script.h"
#include "src/roots/roots-inl.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace v8 {
namespace internal {

using RootsTest = TestWithIsolate;

namespace {
AllocationSpace GetSpaceFromObject(Object object) {
  DCHECK(IsHeapObject(object));
  BasicMemoryChunk* chunk =
      BasicMemoryChunk::FromHeapObject(HeapObject::cast(object));
  if (chunk->InReadOnlySpace()) return RO_SPACE;
  return chunk->owner()->identity();
}
}  // namespace

#define CHECK_IN_RO_SPACE(type, name, CamelName) \
  HeapObject name = roots.name();                \
  CHECK_EQ(RO_SPACE, GetSpaceFromObject(name));

// The following tests check that all the roots accessible via ReadOnlyRoots are
// in RO_SPACE.
TEST_F(RootsTest, TestReadOnlyRoots) {
  ReadOnlyRoots roots(i_isolate());

  READ_ONLY_ROOT_LIST(CHECK_IN_RO_SPACE)
}

#undef CHECK_IN_RO_SPACE

namespace {
bool IsInitiallyMutable(Factory* factory, Address object_address) {
// Entries in this list are in STRONG_MUTABLE_MOVABLE_ROOT_LIST, but may
// initially point to objects that are in RO_SPACE.
#define INITIALLY_READ_ONLY_ROOT_LIST(V)  \
  V(api_private_symbol_table)             \
  V(api_symbol_table)                     \
  V(basic_block_profiling_data)           \
  V(builtins_constants_table)             \
  V(current_microtask)                    \
  V(detached_contexts)                    \
  V(feedback_vectors_for_profiling_tools) \
  V(shared_wasm_memories)                 \
  V(materialized_objects)                 \
  V(public_symbol_table)                  \
  V(retaining_path_targets)               \
  V(serialized_global_proxy_sizes)        \
  V(serialized_objects)                   \
  IF_WASM(V, js_to_wasm_wrappers)         \
  IF_WASM(V, wasm_canonical_rtts)         \
  V(weak_refs_keep_during_job)

#define TEST_CAN_BE_READ_ONLY(name) \
  if (factory->name().address() == object_address) return false;
  INITIALLY_READ_ONLY_ROOT_LIST(TEST_CAN_BE_READ_ONLY)
#undef TEST_CAN_BE_READ_ONLY
#undef INITIALLY_READ_ONLY_ROOT_LIST
  return true;
}
}  // namespace

// The CHECK_EQ line is there just to ensure that the root is publicly
// accessible from Heap, but ultimately the factory is used as it provides
// handles that have the address in the root table.
#define CHECK_NOT_IN_RO_SPACE(type, name, CamelName)                        \
  Handle<Object> name = factory->name();                                    \
  CHECK_EQ(*name, heap->name());                                            \
  if (IsHeapObject(*name) && IsInitiallyMutable(factory, name.address()) && \
      !IsUndefined(*name, i_isolate())) {                                   \
    CHECK_NE(RO_SPACE, GetSpaceFromObject(HeapObject::cast(*name)));        \
  }

// The following tests check that all the roots accessible via public Heap
// accessors are not in RO_SPACE with the exception of the objects listed in
// INITIALLY_READ_ONLY_ROOT_LIST.
TEST_F(RootsTest, TestHeapRootsNotReadOnly) {
  Factory* factory = i_isolate()->factory();
  Heap* heap = i_isolate()->heap();

  MUTABLE_ROOT_LIST(CHECK_NOT_IN_RO_SPACE)
}

TEST_F(RootsTest, TestHeapNumberList) {
  ReadOnlyRoots roots(isolate());
  for (auto pos = RootIndex::kFirstReadOnlyRoot;
       pos <= RootIndex::kLastReadOnlyRoot; ++pos) {
    auto obj = roots.object_at(pos);
    bool in_nr_range = pos >= RootIndex::kFirstHeapNumberRoot &&
                       pos <= RootIndex::kLastHeapNumberRoot;
    CHECK_EQ(IsHeapNumber(obj), in_nr_range);
  }
}

#undef CHECK_NOT_IN_RO_SPACE

}  // namespace internal
}  // namespace v8
