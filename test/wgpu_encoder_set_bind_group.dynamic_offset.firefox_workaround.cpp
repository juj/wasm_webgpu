// Tests that wgpu_encoder_set_bind_group() passes dynamic offsets with the correct element type
// through the Firefox/Wasm4GB workaround path.
//
// Background: when compiled with CAN_ADDRESS_2GB or MEMORY64, Firefox cannot accept the
// (Uint32Array, start, length) overload of setBindGroup(), so the library creates a fresh
// TypedArray view instead.  The bug was that this view was constructed as Uint8Array rather
// than Uint32Array:
//
//   // buggy
//   new Uint8Array(HEAPU32.buffer, shiftPtr(dynamicOffsets, 2), numDynamicOffsets)
//   // fixed
//   new Uint32Array(HEAPU32.buffer, shiftPtr(dynamicOffsets, 2), numDynamicOffsets)
//
// With Uint8Array the dynamic offset values are read as individual bytes rather than as
// 32-bit integers, so the wrong offset is supplied to the GPU.  In practice the offset
// value becomes 0 (or some other garbage byte), causing the shader to write into the
// first buffer slot instead of the intended second slot.
//
// The test dispatches a compute shader that writes the magic value 0xBEEF at data[0] of
// a dynamically-offset storage buffer binding, then reads back the full buffer and
// asserts that:
//   • the SECOND slot (at byte offset = align) received 0xBEEF  (the correct target), and
//   • the FIRST  slot (at byte offset = 0)     is still 0       (was NOT written).
//
// With the bug present the offset collapses to 0 (or triggers a GPUValidationError), so
// at least one of those assertions fails.
//
// The test passes trivially when built without CAN_ADDRESS_2GB / MEMORY64 (the #else
// branch passes HEAPU32 directly and is always correct). Run with --wasm4gb or --wasm64
// to exercise the workaround path.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Get alignment requirement for dynamic storage buffer offsets.
  WGpuSupportedLimits limits;
  wgpu_device_get_limits(device, &limits);
  uint32_t align = limits.minStorageBufferOffsetAlignment;

  // BGL with a single dynamic-offset storage buffer binding.
  WGpuBindGroupLayoutEntry bglEntry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_STORAGE,
      .hasDynamicOffset = WGPU_TRUE,
      .minBindingSize = 0,
    },
  };
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &bglEntry, 1);
  assert(bgl);

  WGpuPipelineLayout pipelineLayout = wgpu_device_create_pipeline_layout(device, &bgl, 1);
  assert(pipelineLayout);

  // Compute shader: writes the magic value at data[0] of the bound storage buffer.
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@group(0) @binding(0) var<storage, read_write> data: array<u32>;"
            "@compute @workgroup_size(1) fn main() { data[0] = 0xBEEFu; }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", pipelineLayout, 0, 0);
  assert(pipeline);

  // Storage buffer with two alignment-sized slots.  The bind group covers slot 0
  // (base offset = 0); the dynamic offset shifts the binding into slot 1 (base + align).
  WGpuBufferDescriptor bdesc = {
    .size = (uint64_t)align * 2,
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer sbuf = wgpu_device_create_buffer(device, &bdesc);
  assert(sbuf);

  WGpuBindGroupEntry bgentry = {
    .binding = 0,
    .resource = sbuf,
    .bufferBindOffset = 0,
    .bufferBindSize = align,
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  assert(bg);

  // Dispatch with dynamicOffset = align so the shader targets the SECOND slot.
  // With the Uint8Array bug the offset collapses to 0 (or garbage), targeting slot 0
  // or producing a GPUValidationError instead.
  uint32_t dynamicOffset = align;
  WGpuCommandEncoder enc = wgpu_device_create_command_encoder_simple(device);
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(enc, 0);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_encoder_set_bind_group(pass, 0, bg, &dynamicOffset, 1);
  wgpu_compute_pass_encoder_dispatch_workgroups(pass, 1, 1, 1);
  wgpu_encoder_end(pass);

  // Readback buffer covering both slots.
  WGpuBufferDescriptor rdesc = {
    .size = (uint64_t)align * 2,
    .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
  };
  WGpuBuffer rbuf = wgpu_device_create_buffer(device, &rdesc);
  assert(rbuf);
  wgpu_command_encoder_copy_buffer_to_buffer(enc, sbuf, 0, rbuf, 0, (uint64_t)align * 2);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

  // No validation error should have occurred.
  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  wgpu_buffer_map_sync(rbuf, WGPU_MAP_MODE_READ);
  wgpu_buffer_get_mapped_range(rbuf, 0);

  // Slot 0 (at byte offset 0): must NOT have been written — dynamic offset should
  // have redirected the shader to slot 1.  With the Uint8Array bug the offset is
  // wrong and this slot is written instead, so slot0_val == 0xBEEF.
  uint32_t slot0_val = 0;
  wgpu_buffer_read_mapped_range(rbuf, 0, 0, &slot0_val, sizeof(slot0_val));
  printf("slot 0 data[0] = 0x%x (expected 0x0)\n", slot0_val);
  assert(slot0_val == 0u);

  // Slot 1 (at byte offset = align): must have received the magic value.
  uint32_t slot1_val = 0;
  wgpu_buffer_read_mapped_range(rbuf, 0, align, &slot1_val, sizeof(slot1_val));
  printf("slot 1 data[0] = 0x%x (expected 0xBEEF)\n", slot1_val);
  assert(slot1_val == 0xBEEFu);

  wgpu_buffer_unmap(rbuf);

  printf("Test OK\n");
  EM_ASM(window.close());
}
