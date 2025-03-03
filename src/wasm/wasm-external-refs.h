// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !V8_ENABLE_WEBASSEMBLY
#error This header should only be included if WebAssembly is enabled.
#endif  // !V8_ENABLE_WEBASSEMBLY

#ifndef V8_WASM_WASM_EXTERNAL_REFS_H_
#define V8_WASM_WASM_EXTERNAL_REFS_H_

#include <stdint.h>

#include "src/base/macros.h"

namespace v8 {
namespace internal {

class Isolate;

namespace wasm {

using Address = uintptr_t;

V8_EXPORT_PRIVATE void f32_trunc_wrapper(Address data);

V8_EXPORT_PRIVATE void f32_floor_wrapper(Address data);

V8_EXPORT_PRIVATE void f32_ceil_wrapper(Address data);

V8_EXPORT_PRIVATE void f32_nearest_int_wrapper(Address data);

V8_EXPORT_PRIVATE void f64_trunc_wrapper(Address data);

V8_EXPORT_PRIVATE void f64_floor_wrapper(Address data);

V8_EXPORT_PRIVATE void f64_ceil_wrapper(Address data);

V8_EXPORT_PRIVATE void f64_nearest_int_wrapper(Address data);

V8_EXPORT_PRIVATE void int64_to_float32_wrapper(Address data);

V8_EXPORT_PRIVATE void uint64_to_float32_wrapper(Address data);

V8_EXPORT_PRIVATE void int64_to_float64_wrapper(Address data);

V8_EXPORT_PRIVATE void uint64_to_float64_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t float32_to_int64_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t float32_to_uint64_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t float64_to_int64_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t float64_to_uint64_wrapper(Address data);

V8_EXPORT_PRIVATE void float32_to_int64_sat_wrapper(Address data);

V8_EXPORT_PRIVATE void float32_to_uint64_sat_wrapper(Address data);

V8_EXPORT_PRIVATE void float64_to_int64_sat_wrapper(Address data);

V8_EXPORT_PRIVATE void float64_to_uint64_sat_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t int64_div_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t int64_mod_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t uint64_div_wrapper(Address data);

V8_EXPORT_PRIVATE int32_t uint64_mod_wrapper(Address data);

V8_EXPORT_PRIVATE uint32_t word32_ctz_wrapper(Address data);

V8_EXPORT_PRIVATE uint32_t word64_ctz_wrapper(Address data);

V8_EXPORT_PRIVATE uint32_t word32_popcnt_wrapper(Address data);

V8_EXPORT_PRIVATE uint32_t word64_popcnt_wrapper(Address data);

V8_EXPORT_PRIVATE uint32_t word32_rol_wrapper(Address data);

V8_EXPORT_PRIVATE uint32_t word32_ror_wrapper(Address data);

V8_EXPORT_PRIVATE void word64_rol_wrapper(Address data);

V8_EXPORT_PRIVATE void word64_ror_wrapper(Address data);

V8_EXPORT_PRIVATE void float64_pow_wrapper(Address data);

V8_EXPORT_PRIVATE void f64x2_ceil_wrapper(Address data);

V8_EXPORT_PRIVATE void f64x2_floor_wrapper(Address data);

V8_EXPORT_PRIVATE void f64x2_trunc_wrapper(Address data);

V8_EXPORT_PRIVATE void f64x2_nearest_int_wrapper(Address data);

V8_EXPORT_PRIVATE void f32x4_ceil_wrapper(Address data);

V8_EXPORT_PRIVATE void f32x4_floor_wrapper(Address data);

V8_EXPORT_PRIVATE void f32x4_trunc_wrapper(Address data);

V8_EXPORT_PRIVATE void f32x4_nearest_int_wrapper(Address data);

// The return type is {int32_t} instead of {bool} to enforce the compiler to
// zero-extend the result in the return register.
int32_t memory_init_wrapper(Address data);

// The return type is {int32_t} instead of {bool} to enforce the compiler to
// zero-extend the result in the return register.
int32_t memory_copy_wrapper(Address data);

// The return type is {int32_t} instead of {bool} to enforce the compiler to
// zero-extend the result in the return register.
int32_t memory_fill_wrapper(Address data);

// Assumes copy ranges are in-bounds and length > 0.
void array_copy_wrapper(Address raw_instance, Address raw_dst_array,
                        uint32_t dst_index, Address raw_src_array,
                        uint32_t src_index, uint32_t length);

// The initial value is passed as an int64_t on the stack. Cannot handle s128
// other than 0.
void array_fill_wrapper(Address raw_array, uint32_t index, uint32_t length,
                        uint32_t emit_write_barrier, uint32_t raw_type,
                        Address initial_value_addr);

double flat_string_to_f64(Address string_address);

// Update the stack limit after a stack switch,
// and preserve pending interrupts.
void sync_stack_limit(Isolate* isolate);

intptr_t switch_to_the_central_stack(Isolate* isolate, uintptr_t sp);
void switch_from_the_central_stack(Isolate* isolate);

using WasmTrapCallbackForTesting = void (*)();

V8_EXPORT_PRIVATE void set_trap_callback_for_testing(
    WasmTrapCallbackForTesting callback);

V8_EXPORT_PRIVATE void call_trap_callback_for_testing();

}  // namespace wasm
}  // namespace internal
}  // namespace v8

#endif  // V8_WASM_WASM_EXTERNAL_REFS_H_
