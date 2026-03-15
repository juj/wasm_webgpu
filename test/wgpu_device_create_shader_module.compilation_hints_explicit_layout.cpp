// Tests wgpu_device_create_shader_module() with a compilation hint that specifies
// an explicit GPUPipelineLayout (layout > 1 case), exercising the
// 'layout > 1 ? wgpu[layout] : (layout ? GPUAutoLayoutMode : null)' branch
// in wgpuReadShaderModuleCompilationHints.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a pipeline layout (empty - no bind group layouts)
  WGpuPipelineLayout pipelineLayout = wgpu_device_create_pipeline_layout(device, 0, 0);
  assert(pipelineLayout);
  assert(wgpu_is_pipeline_layout(pipelineLayout));

  // Create a shader module with a compilation hint specifying the explicit layout
  WGpuShaderModuleCompilationHint hint = {
    .entryPoint = "main",
    .layout = pipelineLayout, // explicit pipeline layout (> 1)
  };

  WGpuShaderModuleDescriptor desc = {
    .code = "@compute @workgroup_size(1) fn main() {}",
    .hints = &hint,
    .numHints = 1,
  };

  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &desc);
  assert(shader);
  assert(wgpu_is_shader_module(shader));

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
