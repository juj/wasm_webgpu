// Verifies that a 2D array texture with 6 layers can be created and that wgpu_texture_depth_or_array_layers() returns 6 along with the correct width and height.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a 2D array texture with 6 layers
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
  desc.width = 32;
  desc.height = 32;
  desc.depthOrArrayLayers = 6;
  desc.mipLevelCount = 1;

  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);
  assert(wgpu_is_texture(texture));
  assert(wgpu_texture_dimension(texture) == WGPU_TEXTURE_DIMENSION_2D);
  assert(wgpu_texture_width(texture) == 32);
  assert(wgpu_texture_height(texture) == 32);
  assert(wgpu_texture_depth_or_array_layers(texture) == 6);

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
