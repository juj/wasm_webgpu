// Verifies that wgpu_is_texture_view() returns true for a WGpuTextureView and false for the WGpuTexture it was created from.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);

  WGpuTextureView view = wgpu_texture_create_view(texture, 0);
  assert(view);
  assert(wgpu_is_texture_view(view));
  assert(!wgpu_is_texture_view(texture)); // a texture is not a texture view

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
