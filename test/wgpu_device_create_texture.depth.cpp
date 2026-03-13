// Verifies that a DEPTH32FLOAT 2D texture usable as a render attachment and texture binding can be created with the correct format, width, and height.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a DEPTH32FLOAT texture for use as a depth attachment
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  desc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
  desc.width = 128;
  desc.height = 128;
  desc.mipLevelCount = 1;
  desc.sampleCount = 1;

  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);
  assert(wgpu_is_texture(texture));
  assert(wgpu_texture_format(texture) == WGPU_TEXTURE_FORMAT_DEPTH32FLOAT);
  assert(wgpu_texture_width(texture) == 128);
  assert(wgpu_texture_height(texture) == 128);

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
