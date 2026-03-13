// Verifies that a sampler with non-default options (linear filtering, repeat address modes, anisotropy x4, LOD clamp 0-8) can be created via wgpu_device_create_sampler().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a sampler with non-default options:
  // - linear mag/min filters
  // - repeat address mode
  // - anisotropic filtering
  WGpuSamplerDescriptor desc = {
    .addressModeU = WGPU_ADDRESS_MODE_REPEAT,
    .addressModeV = WGPU_ADDRESS_MODE_REPEAT,
    .addressModeW = WGPU_ADDRESS_MODE_MIRROR_REPEAT,
    .magFilter = WGPU_FILTER_MODE_LINEAR,
    .minFilter = WGPU_FILTER_MODE_LINEAR,
    .mipmapFilter = WGPU_MIPMAP_FILTER_MODE_LINEAR,
    .lodMinClamp = 0.0f,
    .lodMaxClamp = 8.0f,
    .compare = WGPU_COMPARE_FUNCTION_INVALID,
    .maxAnisotropy = 4,
  };

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
