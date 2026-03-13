// Verifies that wgpu_is_compute_pipeline() returns true for a WGpuComputePipeline and false for a WGpuShaderModule.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));
  assert(!wgpu_is_compute_pipeline(shader)); // a shader module is not a compute pipeline

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
