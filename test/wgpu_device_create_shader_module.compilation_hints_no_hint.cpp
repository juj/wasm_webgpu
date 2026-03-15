// Tests wgpu_device_create_shader_module() with a compilation hint using
// WGPU_AUTO_LAYOUT_MODE_NO_HINT (layout == 0), exercising the
// 'layout ? GPUAutoLayoutMode : null' false branch (layout == 0 → null)
// in $wgpuReadShaderModuleCompilationHints.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleCompilationHint hint = {
    .entryPoint = "main",
    .layout = WGPU_AUTO_LAYOUT_MODE_NO_HINT, // value 0 → exercises 'layout ? ... : null' false branch
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
