// Tests wgpu_texture_create_view() with aspect=WGPU_TEXTURE_ASPECT_STENCIL_ONLY,
// exercising the stencil-only aspect code path in texture view creation.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a depth+stencil texture
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8;
  tdesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  tdesc.width = 64;
  tdesc.height = 64;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  // Create a stencil-only view
  WGpuTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  vdesc.format = WGPU_TEXTURE_FORMAT_STENCIL8;
  vdesc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
  vdesc.aspect = WGPU_TEXTURE_ASPECT_STENCIL_ONLY; // exercises stencil-only aspect path
  vdesc.baseMipLevel = 0;
  vdesc.mipLevelCount = 1;
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
