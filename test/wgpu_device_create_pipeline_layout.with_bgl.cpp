// Verifies that an explicit WGpuPipelineLayout built from a single WGpuBindGroupLayout can be successfully passed to wgpu_device_create_compute_pipeline().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create BGL with one uniform buffer slot
  WGpuBindGroupLayoutEntry bglEntry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_STORAGE,
    },
  };
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &bglEntry, 1);
  assert(bgl);

  // Create a pipeline layout that uses the explicit BGL
  WGpuPipelineLayout layout = wgpu_device_create_pipeline_layout(device, &bgl, 1);
  assert(layout);
  assert(wgpu_is_pipeline_layout(layout));

  // Use the explicit layout in a compute pipeline
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@group(0) @binding(0) var<storage, read_write> data: array<u32>;"
            "@compute @workgroup_size(1) fn main() { data[0] = 42u; }",
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
