// Verifies that wgpu_encoder_set_bind_group() with a dynamic offset correctly targets the second slot of a storage buffer, and that the compute shader writes the expected value there.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <emscripten/heap.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Get alignment requirement
  WGpuSupportedLimits limits;
  wgpu_device_get_limits(device, &limits);
  uint32_t align = limits.minStorageBufferOffsetAlignment;

  // BGL with dynamic storage buffer offset
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

  // Compute shader: writes the magic value at data[0]
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@group(0) @binding(0) var<storage, read_write> data: array<u32>;"
            "@compute @workgroup_size(1) fn main() { data[0] = 0xBEEFu; }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", pipelineLayout, 0, 0);
  assert(pipeline);

  // Storage buffer with two "slots", each `align` bytes. We will dynamically offset into the second slot.
  WGpuBufferDescriptor bdesc = {
    .size = (uint64_t)align * 2,
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer sbuf = wgpu_device_create_buffer(device, &bdesc);
  assert(sbuf);

  // Bind group binds the first 'align' bytes (base offset 0)
  WGpuBindGroupEntry bgentry = {
    .binding = 0,
    .resource = sbuf,
    .bufferBindOffset = 0,
    .bufferBindSize = align,
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  assert(bg);

  // No Wasm4GB/Wasm64 support in Firefox: https://bugzilla.mozilla.org/show_bug.cgi?id=2022805
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}) || emscripten_get_heap_max() <= (size_t)0x7FFFFFFF)
  {
    // Dispatch compute using dynamic offset = align (targets second slot)
    uint32_t dynamicOffset = align;
    WGpuCommandEncoder enc = wgpu_device_create_command_encoder_simple(device);
    WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(enc, 0);
    wgpu_encoder_set_pipeline(pass, pipeline);
    wgpu_encoder_set_bind_group(pass, 0, bg, &dynamicOffset, 1);
    wgpu_compute_pass_encoder_dispatch_workgroups(pass, 1, 1, 1);
    wgpu_encoder_end(pass);

    // Readback buffer
    WGpuBufferDescriptor rdesc = {
      .size = (uint64_t)align * 2,
      .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
    };
    WGpuBuffer rbuf = wgpu_device_create_buffer(device, &rdesc);
    assert(rbuf);
    wgpu_command_encoder_copy_buffer_to_buffer(enc, sbuf, 0, rbuf, 0, (uint64_t)align * 2);
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

    // Check error scope
    char msg[512];
    WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
    if (strlen(msg) > 0) printf("%s\n", msg);
    assert(!error);

    // Read back and verify: second slot (at offset=align) should have 0xBEEF
    wgpu_buffer_map_sync(rbuf, WGPU_MAP_MODE_READ);
    wgpu_buffer_get_mapped_range(rbuf, 0);

    uint32_t val = 0;
    wgpu_buffer_read_mapped_range(rbuf, 0, align, &val, sizeof(val));
    printf("second slot data[0] = 0x%x (expected 0xBEEF)\n", val);
    assert(val == 0xBEEFu);
    wgpu_buffer_unmap(rbuf);
  }

  printf("Test OK\n");
  EM_ASM(window.close());
}
