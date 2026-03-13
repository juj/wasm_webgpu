// Verifies that a 1D RGBA8UNORM texture can be created and that wgpu_texture_dimension(), wgpu_texture_width(), and wgpu_texture_format() return the correct values.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a 1D texture
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  desc.dimension = WGPU_TEXTURE_DIMENSION_1D;
  desc.width = 256;
  desc.height = 1;        // must be 1 for 1D
  desc.depthOrArrayLayers = 1;
  desc.mipLevelCount = 1;

  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);
  assert(wgpu_is_texture(texture));
  assert(wgpu_texture_dimension(texture) == WGPU_TEXTURE_DIMENSION_1D);
  assert(wgpu_texture_width(texture) == 256);
  assert(wgpu_texture_format(texture) == WGPU_TEXTURE_FORMAT_RGBA8UNORM);

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
