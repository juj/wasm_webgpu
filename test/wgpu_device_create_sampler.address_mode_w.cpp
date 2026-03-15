// Tests wgpu_device_create_sampler() with a non-default addressModeW
// (WGPU_ADDRESS_MODE_REPEAT), exercising the W-axis address mode field
// which is relevant for 3D texture sampling.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.addressModeU = WGPU_ADDRESS_MODE_REPEAT;
  desc.addressModeV = WGPU_ADDRESS_MODE_REPEAT;
  desc.addressModeW = WGPU_ADDRESS_MODE_MIRROR_REPEAT; // non-default W-axis address mode

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
