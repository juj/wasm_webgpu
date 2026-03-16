// Tests wgpu_device_create_render_pipeline_async() with an explicit WGpuPipelineLayout
// (pipelineLayoutId > 1), exercising the 'wgpu[pipelineLayoutId]' branch in
// $wgpuReadRenderPipelineDescriptor via the async creation path.
// Complements wgpu_device_create_render_pipeline.explicit_layout.cpp (the sync variant)
// and wgpu_device_create_compute_pipeline_async.explicit_layout.cpp.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(!error);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));
  assert(userData == (void*)88);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create an explicit empty pipeline layout to exercise the pipelineLayoutId > 1 branch.
  WGpuPipelineLayout layout = wgpu_device_create_pipeline_layout(device, /*bindGroupLayouts=*/0, /*numLayouts=*/0);
  assert(layout);

  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f { return vec4f(0,0,0,1); }\n"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.layout = layout; // explicit layout — exercises pipelineLayoutId > 1 in the async path

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = navigator_gpu_get_preferred_canvas_format();
  desc.fragment.numTargets = 1;
  desc.fragment.targets = &colorTarget;

  wgpu_device_create_render_pipeline_async(device, &desc, PipelineCreated, (void*)88);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
