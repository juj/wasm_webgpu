// Verifies that a WGpuBindGroupLayout with a sampler at binding 0 and a 2D float texture at binding 1 can be created and used to form a valid bind group.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a BGL with a sampler at binding 0 and a 2D texture at binding 1
  WGpuBindGroupLayoutEntry entries[2] = {
    {
      .binding = 0,
      .visibility = WGPU_SHADER_STAGE_FRAGMENT,
      .type = WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER,
      .layout.sampler = {
        .type = WGPU_SAMPLER_BINDING_TYPE_FILTERING,
      },
    },
    {
      .binding = 1,
      .visibility = WGPU_SHADER_STAGE_FRAGMENT,
      .type = WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE,
      .layout.texture = {
        .sampleType = WGPU_TEXTURE_SAMPLE_TYPE_FLOAT,
        .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D,
        .multisampled = WGPU_FALSE,
      },
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, entries, 2);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a sampler (filtering) and a texture to bind
  WGpuSamplerDescriptor sdesc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  sdesc.magFilter = WGPU_FILTER_MODE_LINEAR;
  sdesc.minFilter = WGPU_FILTER_MODE_LINEAR;
  WGpuSampler sampler = wgpu_device_create_sampler(device, &sdesc);
  assert(sampler);

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);
  WGpuTextureView textureView = wgpu_texture_create_view_simple(texture);
  assert(textureView);

  WGpuBindGroupEntry bgentries[2] = {
    { .binding = 0, .resource = sampler },
    { .binding = 1, .resource = textureView },
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, bgentries, 2);
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
