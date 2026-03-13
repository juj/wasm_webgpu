// Verifies that a texture view covering a subset of mip levels (baseMipLevel=1, mipLevelCount=2) of a 4-mip-level texture can be created successfully.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a mipmapped texture (4 mip levels, 8x8 base)
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  tdesc.width = 8;
  tdesc.height = 8;
  tdesc.mipLevelCount = 4;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  // Create a view covering only mip levels 1–2 (baseMipLevel=1, mipLevelCount=2)
  WGpuTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  vdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  vdesc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
  vdesc.aspect = WGPU_TEXTURE_ASPECT_ALL;
  vdesc.baseMipLevel = 1;
  vdesc.mipLevelCount = 2;
  vdesc.baseArrayLayer = 0;
  vdesc.arrayLayerCount = 1;
  WGpuTextureView view = wgpu_texture_create_view(texture, &vdesc);
  assert(view);
  assert(wgpu_is_texture_view(view));

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
