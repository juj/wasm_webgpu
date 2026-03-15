// Tests wgpu_encoder_set_bind_group() with a null bind group (bindGroup == 0),
// exercising the 'wgpu[bindGroup]' == undefined path that unbinds the bind group
// at the given index.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Simple compute pipeline with one bind group
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@group(0) @binding(0) var<storage, read_write> buf: array<u32>;"
            "@compute @workgroup_size(1) fn main() { buf[0] = 42u; }",
  };
  WGpuShaderModule cs = wgpu_device_create_shader_module(device, &smdesc);
  assert(cs);

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

  WGpuPipelineLayout pipeLayout = wgpu_device_create_pipeline_layout(device, &bgl, 1);
  assert(pipeLayout);

  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(
    device, cs, "main", pipeLayout, 0, 0);
  assert(pipeline);

  WGpuBufferDescriptor bufDesc = {};
  bufDesc.size = 256;
  bufDesc.usage = WGPU_BUFFER_USAGE_STORAGE;
  WGpuBuffer buf = wgpu_device_create_buffer(device, &bufDesc);
  assert(buf);

  WGpuBindGroupEntry bgEntry = { .binding = 0, .resource = buf };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgEntry, 1);
  assert(bg);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(enc, 0);
  wgpu_encoder_set_pipeline(pass, pipeline);

  // First set a valid bind group, then set null (unbind) at the same index
  wgpu_encoder_set_bind_group(pass, 0, bg, 0, 0);
  wgpu_encoder_set_bind_group(pass, 0, 0, 0, 0); // null = unbind

  // Set it back to something valid before dispatching to avoid validation errors
  wgpu_encoder_set_bind_group(pass, 0, bg, 0, 0);
  wgpu_compute_pass_encoder_dispatch_workgroups(pass, 1, 1, 1);
  wgpu_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

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
