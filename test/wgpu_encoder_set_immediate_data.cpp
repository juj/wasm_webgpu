// Verifies that wgpu_encoder_set_immediate_data() delivers data to a compute shader via var<immediate>.
// Two u32 values are uploaded and the shader writes them to a storage buffer for CPU readback.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main()
{
// TODO: setImmediateData() is not yet avaiable in browsers.
/*
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Compute shader reads two u32 values from immediate data and writes them to a storage buffer.
  WGpuShaderModuleDescriptor smdesc = {
    .code = "var<immediate> idata : vec2u;\n"
            "@group(0) @binding(0) var<storage, read_write> output : array<u32>;\n"
            "@compute @workgroup_size(1)\n"
            "fn main() {\n"
            "  output[0] = idata.x;\n"
            "  output[1] = idata.y;\n"
            "}\n"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(
    device, shader, "main", WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);

  WGpuBufferDescriptor sdesc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer sbuf = wgpu_device_create_buffer(device, &sdesc);
  assert(sbuf);

  WGpuBufferDescriptor rdesc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer rbuf = wgpu_device_create_buffer(device, &rdesc);
  assert(rbuf);

  WGpuBindGroupLayout bgl = wgpu_pipeline_get_bind_group_layout(pipeline, 0);
  WGpuBindGroupEntry bgentry = { .binding = 0, .resource = sbuf };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  wgpu_object_destroy(bgl);

  // Upload two u32 values as immediate data (8 bytes at offset 0).
  uint32_t immediateValues[2] = { 42u, 99u };

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder_simple(device);
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(enc, 0);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_encoder_set_bind_group(pass, 0, bg, 0, 0);
  wgpu_encoder_set_immediate_data(pass, 0, immediateValues, sizeof(immediateValues));
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
  uint32_t results[2] = {};
  wgpu_buffer_read_mapped_range(rbuf, 0, 0, results, sizeof(results));
  wgpu_buffer_unmap(rbuf);

  printf("results = { %u, %u } (expected { 42, 99 })\n", results[0], results[1]);
  assert(results[0] == 42u);
  assert(results[1] == 99u);
*/
  printf("Test OK\n");
  EM_ASM(window.close());
}
