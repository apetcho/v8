// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_COMPILATION_ENVIRONMENT_H_
#define V8_WASM_COMPILATION_ENVIRONMENT_H_

#include <memory>

#include "src/wasm/wasm-features.h"
#include "src/wasm/wasm-limits.h"
#include "src/wasm/wasm-module.h"
#include "src/wasm/wasm-tier.h"

namespace v8 {

class JobHandle;

namespace internal {

class Counters;

namespace wasm {

class NativeModule;
class WasmCode;
class WasmEngine;
class WasmError;

enum RuntimeExceptionSupport : bool {
  kRuntimeExceptionSupport = true,
  kNoRuntimeExceptionSupport = false
};

enum DynamicTiering : bool {
  kDynamicTiering = true,
  kNoDynamicTiering = false
};

// The Arm architecture does not specify the results in memory of
// partially-in-bound writes, which does not align with the wasm spec. This
// affects when trap handlers can be used for OOB detection; however, Mac
// systems with Apple silicon currently do provide trapping beahviour for
// partially-out-of-bound writes, so we assume we can rely on that on MacOS,
// since doing so provides better performance for writes.
#if V8_TARGET_ARCH_ARM64 && !V8_OS_MACOS
constexpr bool kPartialOOBWritesAreNoops = false;
#else
constexpr bool kPartialOOBWritesAreNoops = true;
#endif

// The {CompilationEnv} encapsulates the module data that is used during
// compilation. CompilationEnvs are shareable across multiple compilations.
struct CompilationEnv {
  // A pointer to the decoded module's static representation.
  const WasmModule* const module;

  // If the runtime doesn't support exception propagation,
  // we won't generate stack checks, and trap handling will also
  // be generated differently.
  const RuntimeExceptionSupport runtime_exception_support;

  // Features enabled for this compilation.
  const WasmFeatures enabled_features;

  const DynamicTiering dynamic_tiering;

  constexpr CompilationEnv(const WasmModule* module,
                           RuntimeExceptionSupport runtime_exception_support,
                           const WasmFeatures& enabled_features,
                           DynamicTiering dynamic_tiering)
      : module(module),
        runtime_exception_support(runtime_exception_support),
        enabled_features(enabled_features),
        dynamic_tiering(dynamic_tiering) {}
};

// The wire bytes are either owned by the StreamingDecoder, or (after streaming)
// by the NativeModule. This class abstracts over the storage location.
class WireBytesStorage {
 public:
  virtual ~WireBytesStorage() = default;
  virtual base::Vector<const uint8_t> GetCode(WireBytesRef) const = 0;
  // Returns the ModuleWireBytes corresponding to the underlying module if
  // available. Not supported if the wire bytes are owned by a StreamingDecoder.
  virtual base::Optional<ModuleWireBytes> GetModuleBytes() const = 0;
};

// Callbacks will receive either {kFailedCompilation} or
// {kFinishedBaselineCompilation}.
enum class CompilationEvent : uint8_t {
  kFinishedBaselineCompilation,
  kFinishedExportWrappers,
  kFinishedCompilationChunk,
  kFailedCompilation,
};

class V8_EXPORT_PRIVATE CompilationEventCallback {
 public:
  virtual ~CompilationEventCallback() = default;

  virtual void call(CompilationEvent event) = 0;

  enum ReleaseAfterFinalEvent : bool {
    kReleaseAfterFinalEvent = true,
    kKeepAfterFinalEvent = false
  };

  // Tells the module compiler whether to keep or to release a callback when the
  // compilation state finishes all compilation units. Most callbacks should be
  // released, that's why there is a default implementation, but the callback
  // for code caching with dynamic tiering has to stay alive.
  virtual ReleaseAfterFinalEvent release_after_final_event() {
    return kReleaseAfterFinalEvent;
  }
};

// The implementation of {CompilationState} lives in module-compiler.cc.
// This is the PIMPL interface to that private class.
class V8_EXPORT_PRIVATE CompilationState {
 public:
  ~CompilationState();

  void InitCompileJob();

  void CancelCompilation();

  void CancelInitialCompilation();

  void SetError();

  void SetWireBytesStorage(std::shared_ptr<WireBytesStorage>);

  std::shared_ptr<WireBytesStorage> GetWireBytesStorage() const;

  void AddCallback(std::unique_ptr<CompilationEventCallback> callback);

  void InitializeAfterDeserialization(base::Vector<const int> lazy_functions,
                                      base::Vector<const int> eager_functions);

  // Set a higher priority for the compilation job.
  void SetHighPriority();

  void TierUpAllFunctions();

  // By default, only one top-tier compilation task will be executed for each
  // function. These functions allow resetting that counter, to be used when
  // optimized code is intentionally thrown away and should be re-created.
  void AllowAnotherTopTierJob(uint32_t func_index);
  void AllowAnotherTopTierJobForAllFunctions();

  bool failed() const;
  bool baseline_compilation_finished() const;

  void set_compilation_id(int compilation_id);

  DynamicTiering dynamic_tiering() const;

  // Override {operator delete} to avoid implicit instantiation of {operator
  // delete} with {size_t} argument. The {size_t} argument would be incorrect.
  void operator delete(void* ptr) { ::operator delete(ptr); }

  CompilationState() = delete;

  size_t EstimateCurrentMemoryConsumption() const;

 private:
  // NativeModule is allowed to call the static {New} method.
  friend class NativeModule;

  // The CompilationState keeps a {std::weak_ptr} back to the {NativeModule}
  // such that it can keep it alive (by regaining a {std::shared_ptr}) in
  // certain scopes.
  static std::unique_ptr<CompilationState> New(
      const std::shared_ptr<NativeModule>&, std::shared_ptr<Counters>,
      DynamicTiering dynamic_tiering);
};

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_COMPILATION_ENVIRONMENT_H_
