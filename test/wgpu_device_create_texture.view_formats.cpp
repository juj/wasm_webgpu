// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM; // Specify minimum number of fields that are required for texture creation to succeed.
  WGPU_TEXTURE_FORMAT fmts[2] = { WGPU_TEXTURE_FORMAT_RGBA8UNORM, WGPU_TEXTURE_FORMAT_RGBA8UNORM_SRGB };
  desc.viewFormats = fmts;
  desc.numViewFormats = 2;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);

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
