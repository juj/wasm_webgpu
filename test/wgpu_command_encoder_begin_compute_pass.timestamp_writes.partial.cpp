// Tests wgpu_command_encoder_begin_compute_pass() with partial timestamp writes
// (only endOfPassWriteIndex is set, beginningOfPassWriteIndex is -1).
// This exercises the 'if ((i = HEAP32[...+1]) >= 0)' false branch in
// wgpuReadTimestampWrites for the compute pass beginning write.
// Requires WGPU_FEATURE_TIMESTAMP_QUERY; skipped gracefully if not supported.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  if (!wgpu_device_supports_feature(device, WGPU_FEATURE_TIMESTAMP_QUERY))
  {
    printf("WGPU_FEATURE_TIMESTAMP_QUERY not supported, skipping.\n");
    EM_ASM(window.close());
    return;
  }

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@compute @workgroup_size(1) fn main() {}",
  };
  WGpuShaderModule cs = wgpu_device_create_shader_module(device, &smdesc);
  assert(cs);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, cs, "main",
    WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);

  WGpuQuerySetDescriptor qsDesc = {};
  qsDesc.type = WGPU_QUERY_TYPE_TIMESTAMP;
  qsDesc.count = 1;
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &qsDesc);
  assert(querySet);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuComputePassDescriptor passDesc = {};
  passDesc.timestampWrites.querySet = querySet;
  passDesc.timestampWrites.beginningOfPassWriteIndex = -1; // NOT written (exercises false branch)
  passDesc.timestampWrites.endOfPassWriteIndex = 0;        // written at end only

  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(enc, &passDesc);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_compute_pass_encoder_dispatch_workgroups(pass, 1, 1, 1);
  wgpu_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_TIMESTAMP_QUERY;
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
