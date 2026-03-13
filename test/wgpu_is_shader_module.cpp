// Verifies that wgpu_is_shader_module() returns true for a WGpuShaderModule and false for a WGpuDevice.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor desc = { .code = "" };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &desc);
  assert(shader);
  assert(wgpu_is_shader_module(shader));
  assert(!wgpu_is_shader_module(device)); // a device is not a shader module

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
