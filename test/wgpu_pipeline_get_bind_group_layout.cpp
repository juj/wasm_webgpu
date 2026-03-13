// Verifies that wgpu_pipeline_get_bind_group_layout() returns a valid WGpuBindGroupLayout for group index 0 of an auto-layout compute pipeline.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);

  // getBindGroupLayout(0) returns the auto-generated layout for group 0
  WGpuBindGroupLayout layout = wgpu_pipeline_get_bind_group_layout(pipeline, 0);
  assert(layout);
  assert(wgpu_is_bind_group_layout(layout));

  wgpu_object_destroy(layout);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
