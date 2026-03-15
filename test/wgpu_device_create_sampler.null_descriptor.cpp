// Tests wgpu_device_create_sampler() with a null descriptor pointer,
// exercising the 'let desc = descriptor ? {...} : void 0' null branch
// in wgpu_device_create_sampler, which creates a sampler with all default values.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Pass null descriptor -> sampler with all WebGPU defaults
  WGpuSampler sampler = wgpu_device_create_sampler(device, 0);
  assert(sampler);
  assert(wgpu_is_sampler(sampler));

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
