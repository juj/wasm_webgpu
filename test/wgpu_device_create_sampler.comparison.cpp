// Verifies that a comparison sampler with WGPU_COMPARE_FUNCTION_LESS and nearest-filter modes can be created via wgpu_device_create_sampler().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // A comparison sampler: non-filtering mode required when compare != INVALID
  WGpuSamplerDescriptor desc = {
    .addressModeU = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
    .magFilter = WGPU_FILTER_MODE_NEAREST,
    .minFilter = WGPU_FILTER_MODE_NEAREST,
    .mipmapFilter = WGPU_MIPMAP_FILTER_MODE_NEAREST,
    .lodMinClamp = 0.0f,
    .lodMaxClamp = 32.0f,
    .compare = WGPU_COMPARE_FUNCTION_LESS,
    .maxAnisotropy = 1,
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
