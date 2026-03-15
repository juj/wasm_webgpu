// Tests wgpu_device_create_bind_group_layout() with a texture binding that has
// multisampled == true, exercising the 'multisampled': !!HEAPU32[entries+2]
// true branch in the type==3 (TEXTURE) path of wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // BGL with a multisampled texture binding
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_FRAGMENT,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE,
    .layout.texture = {
      .sampleType = WGPU_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT,
      .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D,
      .multisampled = WGPU_TRUE, // exercises the true branch
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a matching 4x multisampled texture and bind it
  WGpuTextureDescriptor texDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  texDesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  texDesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  texDesc.width = 64;
  texDesc.height = 64;
  texDesc.sampleCount = 4;
  WGpuTexture msaaTex = wgpu_device_create_texture(device, &texDesc);
  assert(msaaTex);
  WGpuTextureView msaaView = wgpu_texture_create_view_simple(msaaTex);
  assert(msaaView);

  WGpuBindGroupEntry bgEntry = {
    .binding = 0,
    .resource = msaaView,
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgEntry, 1);
  assert(bg);
  assert(wgpu_is_bind_group(bg));

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
