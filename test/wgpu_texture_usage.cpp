// Verifies that wgpu_texture_usage() returns the usage flags (COPY_SRC | TEXTURE_BINDING) that were specified in the WGpuTextureDescriptor at creation time.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);

  WGPU_TEXTURE_USAGE_FLAGS usage = wgpu_texture_usage(texture);
  assert(usage & WGPU_TEXTURE_USAGE_COPY_SRC);
  assert(usage & WGPU_TEXTURE_USAGE_TEXTURE_BINDING);

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
