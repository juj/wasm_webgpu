// Verifies that wgpu_compute_pass_encoder_dispatch_workgroups() executes a compute shader across 4 workgroups and produces the expected values in a readback buffer.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <emscripten/heap.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Simple compute shader that writes a value to a storage buffer
  WGpuShaderModuleDescriptor shaderDesc = {
    .code =
      "@group(0) @binding(0) var<storage, read_write> output: array<u32>;\n"
      "@compute @workgroup_size(1)\n"
      "fn main(@builtin(global_invocation_id) gid: vec3u) {\n"
      "  output[gid.x] = gid.x + 1u;\n"
      "}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);

  // Create storage buffer for output
  WGpuBufferDescriptor storageDesc = {
    .size = 4 * sizeof(uint32_t),
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer storageBuf = wgpu_device_create_buffer(device, &storageDesc);

  WGpuBufferDescriptor readbackDesc = {
    .size = 4 * sizeof(uint32_t),
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer readbackBuf = wgpu_device_create_buffer(device, &readbackDesc);

  // Create bind group
  WGpuBindGroupLayout layout = wgpu_pipeline_get_bind_group_layout(pipeline, 0);
  WGpuBindGroupEntry entry = {
    .binding = 0,
    .resource = storageBuf,
  };
  WGpuBindGroup bindGroup = wgpu_device_create_bind_group(device, layout, &entry, 1);
  wgpu_object_destroy(layout);

  // No Wasm4GB/Wasm64 support in Firefox: https://bugzilla.mozilla.org/show_bug.cgi?id=2022805
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}) || emscripten_get_heap_max() <= (size_t)0x7FFFFFFF)
  {
    // Dispatch
    WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
    WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(encoder, 0);
    wgpu_compute_pass_encoder_set_pipeline(pass, pipeline);
    wgpu_encoder_set_bind_group(pass, 0, bindGroup, 0, 0);
    wgpu_compute_pass_encoder_dispatch_workgroups(pass, 4, 1, 1);
    wgpu_compute_pass_encoder_end(pass);

    wgpu_command_encoder_copy_buffer_to_buffer(encoder, storageBuf, 0, readbackBuf, 0, 4 * sizeof(uint32_t));
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

    char msg[512];
    WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
    if (strlen(msg) > 0) printf("%s\n", msg);
    assert(!error);

    uint32_t results[4] = {};
    wgpu_buffer_map_sync(readbackBuf, WGPU_MAP_MODE_READ);
    wgpu_buffer_get_mapped_range(readbackBuf, 0);
    wgpu_buffer_read_mapped_range(readbackBuf, 0, 0, results, sizeof(results));

    printf("Results: %u %u %u %u\n", results[0], results[1], results[2], results[3]);
    assert(results[0] == 1);
    assert(results[1] == 2);
    assert(results[2] == 3);
    assert(results[3] == 4);
  }

  printf("Test OK\n");
  EM_ASM(window.close());
}
