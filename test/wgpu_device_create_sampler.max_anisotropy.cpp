// Tests wgpu_device_create_sampler() with maxAnisotropy=16 (maximum anisotropic
// filtering), exercising the non-default maxAnisotropy code path.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.magFilter = WGPU_FILTER_MODE_LINEAR;
  desc.minFilter = WGPU_FILTER_MODE_LINEAR;
  desc.mipmapFilter = WGPU_MIPMAP_FILTER_MODE_LINEAR;
  desc.maxAnisotropy = 16; // exercises maxAnisotropy > 1 code path

  WGpuSampler sampler = wgpu_device_create_sampler(device, &desc);
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
