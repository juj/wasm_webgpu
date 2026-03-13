// Verifies that a 2D RGBA8UNORM texture with 4 mip levels (base 8x8) can be created and that wgpu_texture_mip_level_count() returns 4.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a mipmapped texture with 4 mip levels (base 8x8 → 4x4 → 2x2 → 1x1)
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
  desc.width = 8;
  desc.height = 8;
  desc.mipLevelCount = 4;

  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);
  assert(wgpu_is_texture(texture));
  assert(wgpu_texture_mip_level_count(texture) == 4);
  assert(wgpu_texture_width(texture) == 8);
  assert(wgpu_texture_height(texture) == 8);

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
