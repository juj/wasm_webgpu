// flags: -sEXIT_RUNTIME=1

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.width = 4;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);

  WGpuTextureViewDescriptor viewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  WGpuTextureView view = wgpu_texture_create_view(texture, &viewDesc);
  assert(view);

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
