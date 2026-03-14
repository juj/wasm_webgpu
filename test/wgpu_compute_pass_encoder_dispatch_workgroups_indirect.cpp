// Verifies that wgpu_compute_pass_encoder_dispatch_workgroups_indirect() dispatches compute work using workgroup counts read from an indirect buffer and produces the expected results.
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

  // Compute shader that writes global invocation ID to storage buffer
  WGpuShaderModuleDescriptor shaderDesc = {
    .code =
      "@group(0) @binding(0) var<storage, read_write> output: array<u32>;\n"
      "@compute @workgroup_size(1)\n"
      "fn main(@builtin(global_invocation_id) gid: vec3u) {\n"
      "  output[gid.x] = gid.x + 10u;\n"
      "}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);

  // Storage buffer for output
  WGpuBufferDescriptor storageDesc = {
    .size = 3 * sizeof(uint32_t),
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer storageBuf = wgpu_device_create_buffer(device, &storageDesc);

  WGpuBufferDescriptor readbackDesc = {
    .size = 3 * sizeof(uint32_t),
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer readbackBuf = wgpu_device_create_buffer(device, &readbackDesc);

  // Indirect dispatch buffer: [workgroupCountX=3, workgroupCountY=1, workgroupCountZ=1]
  uint32_t indirectArgs[] = { 3, 1, 1 };
  WGpuBufferDescriptor indDesc = {
    .size = sizeof(indirectArgs),
    .usage = WGPU_BUFFER_USAGE_INDIRECT,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer indirectBuf = wgpu_device_create_buffer(device, &indDesc);
  wgpu_buffer_get_mapped_range(indirectBuf, 0);
  wgpu_buffer_write_mapped_range(indirectBuf, 0, 0, indirectArgs, sizeof(indirectArgs));
  wgpu_buffer_unmap(indirectBuf);

  // Bind group
  WGpuBindGroupLayout layout = wgpu_pipeline_get_bind_group_layout(pipeline, 0);
  WGpuBindGroupEntry entry = {
    .binding = 0,
    .resource = storageBuf,
  };
  WGpuBindGroup bindGroup = wgpu_device_create_bind_group(device, layout, &entry, 1);
  wgpu_object_destroy(layout);

  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}) || emscripten_get_heap_max() <= (size_t)0x7FFFFFFF)
  {
    // Dispatch indirect
    WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
    WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(encoder, 0);
    wgpu_compute_pass_encoder_set_pipeline(pass, pipeline);
    wgpu_encoder_set_bind_group(pass, 0, bindGroup, 0, 0);
    wgpu_compute_pass_encoder_dispatch_workgroups_indirect(pass, indirectBuf, 0);
    wgpu_compute_pass_encoder_end(pass);

    wgpu_command_encoder_copy_buffer_to_buffer(encoder, storageBuf, 0, readbackBuf, 0, 3 * sizeof(uint32_t));
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

    char msg[512];
    WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
    if (strlen(msg) > 0) printf("%s\n", msg);
    assert(!error);

    uint32_t results[3] = {};
    wgpu_buffer_map_sync(readbackBuf, WGPU_MAP_MODE_READ);
    wgpu_buffer_get_mapped_range(readbackBuf, 0);
    wgpu_buffer_read_mapped_range(readbackBuf, 0, 0, results, sizeof(results));

    printf("Results: %u %u %u\n", results[0], results[1], results[2]);
    assert(results[0] == 10);
    assert(results[1] == 11);
    assert(results[2] == 12);
  }

  printf("Test OK\n");
  EM_ASM(window.close());
}
