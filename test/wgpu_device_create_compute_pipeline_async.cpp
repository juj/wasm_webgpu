// Verifies that wgpu_device_create_compute_pipeline_async() delivers a valid compute pipeline (and no error) to the callback, passing the user-data pointer through correctly.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(!error);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));
  assert(userData == (void*)42);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  wgpu_device_create_compute_pipeline_async(device, shader, "main", WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0, PipelineCreated, (void*)42);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
