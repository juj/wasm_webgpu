// Tests creating a WGpuBindGroupLayout with a depth texture sample type
// (WGPU_TEXTURE_SAMPLE_TYPE_DEPTH), exercising that branch in
// wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayoutEntry entries[2] = {
    {
      .binding = 0,
      .visibility = WGPU_SHADER_STAGE_FRAGMENT,
      .type = WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE,
      .layout.texture = {
        .sampleType = WGPU_TEXTURE_SAMPLE_TYPE_DEPTH,
        .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D,
        .multisampled = WGPU_FALSE,
      },
    },
    {
      // comparison sampler needed for depth texture sampling
      .binding = 1,
      .visibility = WGPU_SHADER_STAGE_FRAGMENT,
      .type = WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER,
      .layout.sampler = {
        .type = WGPU_SAMPLER_BINDING_TYPE_COMPARISON,
      },
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, entries, 2);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a depth32float texture (supports depth sampling)
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  tdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  // Create a comparison sampler
  WGpuSamplerDescriptor sdesc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  sdesc.compare = WGPU_COMPARE_FUNCTION_LESS;
  WGpuSampler sampler = wgpu_device_create_sampler(device, &sdesc);
  assert(sampler);

  WGpuBindGroupEntry bgentries[2] = {
    { .binding = 0, .resource = wgpu_texture_create_view_simple(texture) },
    { .binding = 1, .resource = sampler },
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, bgentries, 2);
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
