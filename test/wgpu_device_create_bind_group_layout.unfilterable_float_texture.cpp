// Tests creating a WGpuBindGroupLayout with an unfilterable-float texture
// sample type (WGPU_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT), exercising that
// branch in wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_FRAGMENT,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE,
    .layout.texture = {
      .sampleType = WGPU_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT,
      .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D,
      .multisampled = WGPU_FALSE,
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create an R32FLOAT texture (unfilterable-float format) to bind
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_R32FLOAT;
  tdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);
  WGpuTextureView textureView = wgpu_texture_create_view_simple(texture);
  assert(textureView);

  WGpuBindGroupEntry bgentry = { .binding = 0, .resource = textureView };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  assert(bg);

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
