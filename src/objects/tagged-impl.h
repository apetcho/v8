// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_TAGGED_IMPL_H_
#define V8_OBJECTS_TAGGED_IMPL_H_

#include "include/v8-internal.h"
#include "src/base/macros.h"
#include "src/common/checks.h"
#include "src/common/globals.h"

namespace v8 {
namespace internal {

#ifdef V8_EXTERNAL_CODE_SPACE
// When V8_EXTERNAL_CODE_SPACE is enabled comparing InstructionStream and
// non-InstructionStream objects by looking only at compressed values it not
// correct. Full pointers must be compared instead.
bool V8_EXPORT_PRIVATE CheckObjectComparisonAllowed(Address a, Address b);
#endif

// An TaggedImpl is a base class for Object (which is either a Smi or a strong
// reference to a HeapObject) and MaybeObject (which is either a Smi, a strong
// reference to a HeapObject, a weak reference to a HeapObject, or a cleared
// weak reference.
// This class provides storage and one canonical implementation of various
// predicates that check Smi and heap object tags' values and also take into
// account whether the tagged value is expected to be weak reference to a
// HeapObject or cleared weak reference.
template <HeapObjectReferenceType kRefType, typename StorageType>
class TaggedImpl {
 public:
  // Compressed TaggedImpl are never used for external InstructionStream
  // pointers, so we can use this shorter alias for calling decompression
  // functions.
  using CompressionScheme = V8HeapCompressionScheme;

  static_assert(std::is_same<StorageType, Address>::value ||
                    std::is_same<StorageType, Tagged_t>::value,
                "StorageType must be either Address or Tagged_t");

  // True for those TaggedImpl instantiations that represent uncompressed
  // tagged values and false for TaggedImpl instantiations that represent
  // compressed tagged values.
  static const bool kIsFull = sizeof(StorageType) == kSystemPointerSize;

  static const bool kCanBeWeak = kRefType == HeapObjectReferenceType::WEAK;

  constexpr TaggedImpl() : ptr_{} {}
  explicit constexpr TaggedImpl(StorageType ptr) : ptr_(ptr) {}

  // Make clang on Linux catch what MSVC complains about on Windows:
  operator bool() const = delete;

  // Don't use this operator for comparing with stale or invalid pointers
  // because CheckObjectComparisonAllowed() might crash when trying to access
  // the object's page header. Use SafeEquals() instead.
  template <typename U>
  constexpr bool operator==(TaggedImpl<kRefType, U> other) const {
    static_assert(
        std::is_same<U, Address>::value || std::is_same<U, Tagged_t>::value,
        "U must be either Address or Tagged_t");
#ifdef V8_EXTERNAL_CODE_SPACE
    // When comparing two full pointer values ensure that it's allowed.
    if (std::is_same<StorageType, Address>::value &&
        std::is_same<U, Address>::value) {
      SLOW_DCHECK(CheckObjectComparisonAllowed(ptr_, other.ptr()));
    }
#endif  // V8_EXTERNAL_CODE_SPACE
    return static_cast<Tagged_t>(ptr_) == static_cast<Tagged_t>(other.ptr());
  }

  // Don't use this operator for comparing with stale or invalid pointers
  // because CheckObjectComparisonAllowed() might crash when trying to access
  // the object's page header. Use SafeEquals() instead.
  template <typename U>
  constexpr bool operator!=(TaggedImpl<kRefType, U> other) const {
    static_assert(
        std::is_same<U, Address>::value || std::is_same<U, Tagged_t>::value,
        "U must be either Address or Tagged_t");
#ifdef V8_EXTERNAL_CODE_SPACE
    // When comparing two full pointer values ensure that it's allowed.
    if (std::is_same<StorageType, Address>::value &&
        std::is_same<U, Address>::value) {
      SLOW_DCHECK(CheckObjectComparisonAllowed(ptr_, other.ptr()));
    }
#endif  // V8_EXTERNAL_CODE_SPACE
    return static_cast<Tagged_t>(ptr_) != static_cast<Tagged_t>(other.ptr());
  }

  // A variant of operator== which allows comparing InstructionStream object
  // with non-InstructionStream objects even if the V8_EXTERNAL_CODE_SPACE is
  // enabled.
  constexpr bool SafeEquals(TaggedImpl other) const {
    static_assert(std::is_same<StorageType, Address>::value,
                  "Safe comparison is allowed only for full tagged values");
    if (V8_EXTERNAL_CODE_SPACE_BOOL) {
      return ptr_ == other.ptr();
    }
    return this->operator==(other);
  }

  // For using in std::set and std::map.
  constexpr bool operator<(TaggedImpl other) const {
#ifdef V8_EXTERNAL_CODE_SPACE
    // When comparing two full pointer values ensure that it's allowed.
    if (std::is_same<StorageType, Address>::value) {
      SLOW_DCHECK(CheckObjectComparisonAllowed(ptr_, other.ptr()));
    }
#endif  // V8_EXTERNAL_CODE_SPACE
    return static_cast<Tagged_t>(ptr_) < static_cast<Tagged_t>(other.ptr());
  }

  constexpr StorageType ptr() const { return ptr_; }

  // Returns true if this tagged value is a strong pointer to a HeapObject or
  // Smi.
  constexpr inline bool IsObject() const { return !IsWeakOrCleared(); }

  // Returns true if this tagged value is a Smi.
  constexpr bool IsSmi() const { return HAS_SMI_TAG(ptr_); }
  inline bool ToSmi(Smi* value) const;
  inline Smi ToSmi() const;

  // Returns true if this tagged value is a strong pointer to a HeapObject.
  constexpr inline bool IsHeapObject() const { return IsStrong(); }

  // Returns true if this tagged value is a cleared weak reference.
  constexpr inline bool IsCleared() const {
    return kCanBeWeak &&
           (static_cast<uint32_t>(ptr_) == kClearedWeakHeapObjectLower32);
  }

  // Returns true if this tagged value is a strong or weak pointer to a
  // HeapObject.
  constexpr inline bool IsStrongOrWeak() const {
    return !IsSmi() && !IsCleared();
  }

  // Returns true if this tagged value is a strong pointer to a HeapObject.
  constexpr inline bool IsStrong() const {
    DCHECK(kCanBeWeak || (!IsSmi() == HAS_STRONG_HEAP_OBJECT_TAG(ptr_)));
    return kCanBeWeak ? HAS_STRONG_HEAP_OBJECT_TAG(ptr_) : !IsSmi();
  }

  // Returns true if this tagged value is a weak pointer to a HeapObject.
  constexpr inline bool IsWeak() const {
    return IsWeakOrCleared() && !IsCleared();
  }

  // Returns true if this tagged value is a weak pointer to a HeapObject or
  // cleared weak reference.
  constexpr inline bool IsWeakOrCleared() const {
    return kCanBeWeak && HAS_WEAK_HEAP_OBJECT_TAG(ptr_);
  }

  //
  // The following set of methods get HeapObject out of the tagged value
  // which may involve decompression in which case the isolate root is required.
  // If the pointer compression is not enabled then the variants with
  // isolate parameter will be exactly the same as the ones witout isolate
  // parameter.
  //

  // If this tagged value is a strong pointer to a HeapObject, returns true and
  // sets *result. Otherwise returns false.
  inline bool GetHeapObjectIfStrong(HeapObject* result) const;
  inline bool GetHeapObjectIfStrong(Isolate* isolate, HeapObject* result) const;

  // DCHECKs that this tagged value is a strong pointer to a HeapObject and
  // returns the HeapObject.
  inline HeapObject GetHeapObjectAssumeStrong() const;
  inline HeapObject GetHeapObjectAssumeStrong(Isolate* isolate) const;

  // If this tagged value is a weak pointer to a HeapObject, returns true and
  // sets *result. Otherwise returns false.
  inline bool GetHeapObjectIfWeak(HeapObject* result) const;
  inline bool GetHeapObjectIfWeak(Isolate* isolate, HeapObject* result) const;

  // DCHECKs that this tagged value is a weak pointer to a HeapObject and
  // returns the HeapObject.
  inline HeapObject GetHeapObjectAssumeWeak() const;
  inline HeapObject GetHeapObjectAssumeWeak(Isolate* isolate) const;

  // If this tagged value is a strong or weak pointer to a HeapObject, returns
  // true and sets *result. Otherwise returns false.
  inline bool GetHeapObject(HeapObject* result) const;
  inline bool GetHeapObject(Isolate* isolate, HeapObject* result) const;

  inline bool GetHeapObject(HeapObject* result,
                            HeapObjectReferenceType* reference_type) const;
  inline bool GetHeapObject(Isolate* isolate, HeapObject* result,
                            HeapObjectReferenceType* reference_type) const;

  // DCHECKs that this tagged value is a strong or a weak pointer to a
  // HeapObject and returns the HeapObject.
  inline HeapObject GetHeapObject() const;
  inline HeapObject GetHeapObject(Isolate* isolate) const;

  // DCHECKs that this tagged value is a strong or a weak pointer to a
  // HeapObject or a Smi and returns the HeapObject or Smi.
  inline Object GetHeapObjectOrSmi() const;
  inline Object GetHeapObjectOrSmi(Isolate* isolate) const;

  // Cast operation is available only for full non-weak tagged values.
  template <typename T>
  T cast() const {
    CHECK(kIsFull);
    DCHECK(!HAS_WEAK_HEAP_OBJECT_TAG(ptr_));
    return T::cast(Object(ptr_));
  }

 private:
  friend class CompressedObjectSlot;
  friend class FullObjectSlot;

  StorageType ptr_;
};

// Prints this object without details.
template <HeapObjectReferenceType kRefType, typename StorageType>
void ShortPrint(TaggedImpl<kRefType, StorageType> ptr, FILE* out = stdout);

// Prints this object without details to a message accumulator.
template <HeapObjectReferenceType kRefType, typename StorageType>
void ShortPrint(TaggedImpl<kRefType, StorageType> ptr,
                StringStream* accumulator);

template <HeapObjectReferenceType kRefType, typename StorageType>
void ShortPrint(TaggedImpl<kRefType, StorageType> ptr, std::ostream& os);

#ifdef OBJECT_PRINT
template <HeapObjectReferenceType kRefType, typename StorageType>
void Print(TaggedImpl<kRefType, StorageType> ptr);
template <HeapObjectReferenceType kRefType, typename StorageType>
void Print(TaggedImpl<kRefType, StorageType> ptr, std::ostream& os);
#else
template <HeapObjectReferenceType kRefType, typename StorageType>
void Print(TaggedImpl<kRefType, StorageType> ptr) {
  ShortPrint(ptr);
}
template <HeapObjectReferenceType kRefType, typename StorageType>
void Print(TaggedImpl<kRefType, StorageType> ptr, std::ostream& os) {
  ShortPrint(ptr, os);
}
#endif

}  // namespace internal
}  // namespace v8

#endif  // V8_OBJECTS_TAGGED_IMPL_H_
