// Tests wgpu_texture_create_view() with WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY
// (index 5), exercising that dimension value in $wgpuReadRenderPipelineDescriptor
// and wgpu_texture_create_view. Requires a 2D array texture with N*6 layers.
// Uses an error scope since cube-array textures may require a feature.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // 12 layers = 2 cube faces * 6 sides
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format              = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage               = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  tdesc.width               = 4;
  tdesc.height              = 4;
  tdesc.depthOrArrayLayers  = 12; // 2 cubes × 6 faces
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  WGpuTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  vdesc.format          = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  vdesc.dimension       = WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY; // index 5 — exercises 'cube-array'
  vdesc.aspect          = WGPU_TEXTURE_ASPECT_ALL;
  vdesc.baseMipLevel    = 0;
  vdesc.mipLevelCount   = 1;
  vdesc.baseArrayLayer  = 0;
  vdesc.arrayLayerCount = 12;

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuTextureView view = wgpu_texture_create_view(texture, &vdesc);
  (void)view;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("cube-array texture view not supported: %s\n", m ? m : "");
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
