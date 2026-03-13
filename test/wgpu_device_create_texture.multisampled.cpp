// Verifies that a 4x MSAA RGBA8UNORM render attachment texture can be created and that wgpu_texture_sample_count() returns 4.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a 4x MSAA render attachment texture
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
  desc.width = 64;
  desc.height = 64;
  desc.sampleCount = 4;   // 4x MSAA
  desc.mipLevelCount = 1; // multisampled textures cannot have mipmaps

  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);
  assert(wgpu_is_texture(texture));
  assert(wgpu_texture_sample_count(texture) == 4);

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
