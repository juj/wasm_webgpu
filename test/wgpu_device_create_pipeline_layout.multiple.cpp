// Verifies that wgpu_device_create_pipeline_layout() can be created from two distinct bind group layouts and used as an explicit layout for a compute pipeline.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create two different BGLs
  WGpuBindGroupLayoutEntry entry0 = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM,
    },
  };
  WGpuBindGroupLayout bgl0 = wgpu_device_create_bind_group_layout(device, &entry0, 1);
  assert(bgl0);

  WGpuBindGroupLayoutEntry entry1 = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_STORAGE,
    },
  };
  WGpuBindGroupLayout bgl1 = wgpu_device_create_bind_group_layout(device, &entry1, 1);
  assert(bgl1);

  // Create a pipeline layout using both BGLs
  WGpuBindGroupLayout bgls[2] = { bgl0, bgl1 };
  WGpuPipelineLayout layout = wgpu_device_create_pipeline_layout(device, bgls, 2);
  assert(layout);
  assert(wgpu_is_pipeline_layout(layout));

  // Build a shader that uses group 0 (uniform) and group 1 (storage)
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@group(0) @binding(0) var<uniform> udata: vec4f;"
            "@group(1) @binding(0) var<storage, read_write> sdata: array<f32>;"
            "@compute @workgroup_size(1) fn main() { sdata[0] = udata.x; }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(device, shader, "main", layout, 0, 0);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));

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
