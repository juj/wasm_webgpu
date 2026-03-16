// Verifies that wgpu_device_create_sampler() correctly marshals WGPU_ADDRESS_MODE_MIRROR_REPEAT
// (index 3 in $GPUAddressModes) for all three address mode axes. All other sampler tests use
// the default WGPU_ADDRESS_MODE_CLAMP_TO_EDGE, leaving the mirror-repeat mapping untested.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.addressModeU = WGPU_ADDRESS_MODE_MIRROR_REPEAT;
  desc.addressModeV = WGPU_ADDRESS_MODE_MIRROR_REPEAT;
  desc.addressModeW = WGPU_ADDRESS_MODE_MIRROR_REPEAT;

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
