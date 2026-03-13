// Verifies that pipeline override constants (WGpuPipelineConstant) are applied when creating a compute pipeline, so the shader reads the overridden value rather than its default.
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

  // WGSL shader that uses an override constant.
  // The override's default is 1; we will override it to 99 via pipeline constants.
  WGpuShaderModuleDescriptor smdesc = {
    .code = "override kValue: u32 = 1u;"
            "@group(0) @binding(0) var<storage, read_write> data: array<u32>;"
            "@compute @workgroup_size(1) fn main() { data[0] = kValue; }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  // Specify the override constant
  WGpuPipelineConstant constants[] = {
    { .name = "kValue", .value = 99.0 },
  };

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(
    device, shader, "main",
    WGPU_AUTO_LAYOUT_MODE_AUTO,
    constants, 1);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));

  // Run the pipeline and verify the output
  WGpuBufferDescriptor bdesc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer sbuf = wgpu_device_create_buffer(device, &bdesc);
  assert(sbuf);

  WGpuBindGroupLayout bgl = wgpu_pipeline_get_bind_group_layout(pipeline, 0);
  WGpuBindGroupEntry bgentry = { .binding = 0, .resource = sbuf };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  wgpu_object_destroy(bgl);

  WGpuBufferDescriptor rdesc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
  };
  WGpuBuffer rbuf = wgpu_device_create_buffer(device, &rdesc);
  assert(rbuf);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder_simple(device);
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(enc, 0);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_encoder_set_bind_group(pass, 0, bg, 0, 0);
  wgpu_compute_pass_encoder_dispatch_workgroups(pass, 1, 1, 1);
  wgpu_encoder_end(pass);
  wgpu_command_encoder_copy_buffer_to_buffer(enc, sbuf, 0, rbuf, 0, 256);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  wgpu_buffer_map_sync(rbuf, WGPU_MAP_MODE_READ);
  wgpu_buffer_get_mapped_range(rbuf, 0);
  uint32_t result = 0;
  wgpu_buffer_read_mapped_range(rbuf, 0, 0, &result, sizeof(result));
  printf("result = %u (expected 99)\n", result);
  assert(result == 99u);
  wgpu_buffer_unmap(rbuf);

  printf("Test OK\n");
  EM_ASM(window.close());
}
