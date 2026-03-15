// Tests creating a WGpuBindGroupLayout with a non-filtering sampler binding
// (WGPU_SAMPLER_BINDING_TYPE_NON_FILTERING), exercising that branch in
// wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_FRAGMENT,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER,
    .layout.sampler = {
      .type = WGPU_SAMPLER_BINDING_TYPE_NON_FILTERING,
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a non-filtering sampler to bind
  WGpuSamplerDescriptor sdesc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  sdesc.magFilter = WGPU_FILTER_MODE_NEAREST;
  sdesc.minFilter = WGPU_FILTER_MODE_NEAREST;
  WGpuSampler sampler = wgpu_device_create_sampler(device, &sdesc);
  assert(sampler);

  WGpuBindGroupEntry bgentry = { .binding = 0, .resource = sampler };
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
